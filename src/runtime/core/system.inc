// Shell-based builtins. Kept for compatibility: `system` / `capture_output`
// route their input through a command interpreter and therefore interpret
// shell metacharacters. Use `system_argv` / `capture_output_argv` for any
// command that contains data the program did not construct itself.
int64_t npp_system(const char* c) {
    return c ? std::system(c) : -1;
}

// Maximum bytes retained by the capture builtins. Output beyond this limit is
// discarded and the child is terminated (matches npp_http_get's 16 MiB cap).
static const std::size_t nx_capture_limit = 16u * 1024u * 1024u;

// Split a command line into an argv vector without invoking a shell.
// Single/double quotes group arguments; backslash escapes the next character.
// Shell metacharacters (';', '|', '&&', '$', ...) stay literal arguments.
static std::vector<std::string> nx_argv_split(const char* text) {
    std::vector<std::string> out;
    if (!text)
        return out;
    std::string cur;
    bool started = false;
    bool in_single = false;
    bool in_double = false;
    for (const char* p = text; *p; ++p) {
        const char ch = *p;
        if (!in_single && !in_double &&
            (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')) {
            if (started) {
                out.push_back(cur);
                cur.clear();
                started = false;
            }
            continue;
        }
        started = true;
        if (ch == '\\' && !in_single) {
            const char nxt = p[1];
            if (nxt && (!in_double || nxt == '"' || nxt == '\\')) {
                cur.push_back(nxt);
                ++p;
                continue;
            }
        }
        if (ch == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (ch == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        cur.push_back(ch);
    }
    if (started)
        out.push_back(cur);
    return out;
}

#if defined(_WIN32)
static std::wstring nx_argv_widen(const std::string& s) {
    if (s.empty())
        return std::wstring();
    const int n = MultiByteToWideChar(
        CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

// Quote a single element following CommandLineToArgvW rules.
static std::wstring nx_argv_quote(const std::string& arg) {
    std::wstring w = nx_argv_widen(arg);
    bool need = w.empty();
    for (const wchar_t c : w) {
        if (c == L' ' || c == L'\t' || c == L'"' || c == L'\n') {
            need = true;
            break;
        }
    }
    if (!need)
        return w;
    std::wstring out;
    out.push_back(L'"');
    std::size_t slashes = 0;
    for (const wchar_t c : w) {
        if (c == L'\\') {
            ++slashes;
            out.push_back(c);
        } else if (c == L'"') {
            for (std::size_t i = 0; i < slashes; ++i)
                out.push_back(L'\\');
            slashes = 0;
            out.push_back(L'\\');
            out.push_back(L'"');
        } else {
            slashes = 0;
            out.push_back(c);
        }
    }
    for (std::size_t i = 0; i < slashes; ++i)
        out.push_back(L'\\');
    out.push_back(L'"');
    return out;
}

static std::wstring nx_argv_command_line(const std::vector<std::string>& argv) {
    std::wstring line;
    for (std::size_t i = 0; i < argv.size(); ++i) {
        if (i)
            line.push_back(L' ');
        line += nx_argv_quote(argv[i]);
    }
    return line;
}
#endif

// Run a command without a shell. Returns the child exit code, or -1 on failure.
int64_t npp_system_argv(const char* cmdline) {
    const std::vector<std::string> argv = nx_argv_split(cmdline);
    if (argv.empty())
        return -1;
#if defined(_WIN32)
    std::wstring line = nx_argv_command_line(argv);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    // CreateProcessW is allowed to modify the command-line buffer.
    if (!CreateProcessW(nullptr, line.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi))
        return -1;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    if (!GetExitCodeProcess(pi.hProcess, &code))
        code = 1;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int64_t>(code);
#else
    std::vector<char*> ptrs;
    for (const auto& a : argv)
        ptrs.push_back(const_cast<char*>(a.c_str()));
    ptrs.push_back(nullptr);
    const pid_t pid = fork();
    if (pid == 0) {
        execvp(ptrs[0], ptrs.data());
        _exit(127);
    }
    if (pid < 0)
        return -1;
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return -1;
#endif
}

// Run a command without a shell and return stdout (empty string on failure).
char* npp_capture_output_argv(const char* cmdline) {
    const std::vector<std::string> argv = nx_argv_split(cmdline);
    if (argv.empty())
        return nx_strdup("");
    std::string out;
#if defined(_WIN32)
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE rd = INVALID_HANDLE_VALUE;
    HANDLE wr = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&rd, &wr, &sa, 0))
        return nx_strdup("");
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::wstring line = nx_argv_command_line(argv);
    const BOOL started = CreateProcessW(nullptr, line.data(), nullptr, nullptr,
                                        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(wr);
    if (!started) {
        CloseHandle(rd);
        return nx_strdup("");
    }
    char buf[4096];
    DWORD got = 0;
    while (ReadFile(rd, buf, sizeof(buf), &got, nullptr) && got > 0) {
        if (out.size() + static_cast<std::size_t>(got) > nx_capture_limit) {
            TerminateProcess(pi.hProcess, 1);
            break;
        }
        out.append(buf, static_cast<std::size_t>(got));
    }
    CloseHandle(rd);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    int fds[2];
    if (pipe(fds) != 0)
        return nx_strdup("");
    const pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0)
            _exit(127);
        close(fds[1]);
        std::vector<char*> ptrs;
        for (const auto& a : argv)
            ptrs.push_back(const_cast<char*>(a.c_str()));
        ptrs.push_back(nullptr);
        execvp(ptrs[0], ptrs.data());
        _exit(127);
    }
    close(fds[1]);
    if (pid < 0) {
        close(fds[0]);
        return nx_strdup("");
    }
    char buf[4096];
    ssize_t got = 0;
    while ((got = read(fds[0], buf, sizeof(buf))) > 0) {
        if (out.size() + static_cast<std::size_t>(got) > nx_capture_limit) {
            kill(pid, SIGKILL);
            break;
        }
        out.append(buf, static_cast<std::size_t>(got));
    }
    close(fds[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
#endif
    char* result = (char*)nx_managed_alloc(out.size() + 1);
    if (!result)
        return nx_strdup("");
    std::memcpy(result, out.data(), out.size());
    result[out.size()] = 0;
    return result;
}
