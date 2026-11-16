#include "native/native_generator.hpp"

namespace npp::native {
    namespace detail {

        // __nx_args_init: parses process arguments once into arena strings.
        // __nx_args_raw = {raw_argc, raw_argv} (Linux only, saved at _start),
        // __nx_args = {ptrs, count, state}. User args exclude argv[0].
        void Generator::genArgsInit() {
            const std::string done = out_.label("argsinit.done");
            out_.def("__nx_args_init");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(192);
            out_.andRspMinus16();
            out_.leaR10Rip("__nx_args");
            out_.movRaxFromR10Disp8(16); // state
            out_.testRaxRax();
            out_.jcc(J_NE, done);

            // Slots: -8 cursor/cmdline/argv, -16 array, -24 count,
            // -32 index, -40 token start, -48 len, -56 dst, -64 inquote.
            auto scanToken = [&](const std::string& endLabel) {
                const std::string lp = out_.label("tok.lp");
                const std::string q = out_.label("tok.q");
                const std::string sp = out_.label("tok.sp");
                const std::string inc = out_.label("tok.inc");
                out_.def(lp);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, endLabel);
                out_.cmpAlImm8(0x22); // '"'
                out_.jcc(J_E, q);
                out_.cmpAlImm8(0x20); // ' '
                out_.jcc(J_E, sp);
                out_.cmpAlImm8(0x09); // '\t'
                out_.jcc(J_E, sp);
                out_.jmp(inc);
                out_.def(q);
                out_.movRaxFromRbp(-64);
                out_.movR11Imm64(1);
                out_.xorRaxR11();
                out_.movRbpFromRax(-64);
                out_.jmp(inc);
                out_.def(sp);
                out_.movRaxFromRbp(-64);
                out_.testRaxRax();
                out_.jcc(J_NE, inc); // quoted space/tab is content
                out_.jmp(endLabel);
                out_.def(inc);
                out_.incRsi();
                out_.jmp(lp);
            };
            auto skipDelims = [&](const std::string& nulLabel, const std::string& tokLabel) {
                const std::string lp = out_.label("delim.lp");
                const std::string nx = out_.label("delim.nx");
                out_.def(lp);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, nulLabel);
                out_.cmpAlImm8(0x20);
                out_.jcc(J_E, nx);
                out_.cmpAlImm8(0x09);
                out_.jcc(J_E, nx);
                out_.jmp(tokLabel);
                out_.def(nx);
                out_.incRsi();
                out_.jmp(lp);
            };
            // Emits array[i] = dst ([rbp-56]) and i++.
            auto storeArg = [&] {
                const std::string str = out_.label("arg.store");
                out_.def(str);
                out_.movRcxFromRbp(-16);
                out_.movRdxFromRbp(-32);
                out_.shlRdxImm8(3);
                out_.addRcxRdx();
                out_.movRaxFromRbp(-56);
                out_.movToRcxFromRax();
                out_.movRaxFromRbp(-32);
                out_.incRax();
                out_.movRbpFromRax(-32);
            };

            if (target_ == NativeTarget::WindowsX64) {
                out_.subRsp(32);
                out_.callIAT("__iat_GetCommandLineA");
                out_.addRsp(32);
                out_.movRbpFromRax(-8); // cmdline ptr (rax)
                out_.movRsiFromRbp(-8);
                out_.xorEax();
                out_.movRbpFromRax(-64);
                const std::string s0end = out_.label("args.s0end");
                scanToken(s0end); // skip argv[0]
                out_.def(s0end);

                // Pass 1: count.
                out_.xorEax();
                out_.movRbpFromRax(-24);
                const std::string cloop = out_.label("args.cloop");
                const std::string cdone = out_.label("args.cdone");
                const std::string ctok = out_.label("args.ctok");
                const std::string ctn = out_.label("args.ctn");
                out_.def(cloop);
                skipDelims(cdone, ctok);
                out_.def(ctok);
                out_.xorEax();
                out_.movRbpFromRax(-64);
                scanToken(ctn);
                out_.def(ctn);
                out_.movRaxFromRbp(-24);
                out_.incRax();
                out_.movRbpFromRax(-24);
                out_.jmp(cloop);
                out_.def(cdone);
                // array = alloc(count*8, min 8)
                out_.movRaxFromRbp(-24);
                out_.testRaxRax();
                const std::string zc = out_.label("args.zc");
                out_.jcc(J_E, zc);
                out_.shlRaxImm8(3);
                out_.jmp(zc);
                out_.def(zc);
                out_.testRaxRax();
                const std::string nzc = out_.label("args.nzc");
                out_.jcc(J_NE, nzc);
                out_.movRaxImm64(8);
                out_.jmp(nzc);
                out_.def(nzc);
                out_.call("__nx_alloc");
                out_.movRbpFromRax(-16);

                // Pass 2: fill.
                out_.movRsiFromRbp(-8);
                out_.xorEax();
                out_.movRbpFromRax(-64);
                const std::string g0end = out_.label("args.g0end");
                scanToken(g0end); // skip argv[0] again
                out_.def(g0end);
                out_.xorEax();
                out_.movRbpFromRax(-32); // i = 0
                const std::string flp = out_.label("args.flp");
                const std::string fdone = out_.label("args.fdone");
                const std::string ftok = out_.label("args.ftok");
                const std::string ftn = out_.label("args.ftn");
                const std::string fcopy = out_.label("args.fcopy");
                const std::string fcq = out_.label("args.fcq");
                const std::string fcsp = out_.label("args.fcsp");
                const std::string fcst = out_.label("args.fcst");
                const std::string fcd = out_.label("args.fcd");
                out_.def(flp);
                skipDelims(fdone, ftok);
                out_.def(ftok);
                out_.movRbpFromRsi(-40); // token start
                out_.xorEax();
                out_.movRbpFromRax(-64);
                scanToken(ftn);
                out_.def(ftn);
                // span = rsi - start + 1
                out_.movRaxRsi();
                out_.subRaxFromRbp(-40);
                out_.addRaxImm32(1);
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-56);
                // copy non-quote chars
                out_.movRsiFromRbp(-40);
                out_.xorEax();
                out_.movRbpFromRax(-64);
                out_.def(fcopy);
                out_.movAlFromRsi();
                out_.testAlAl();
                out_.jcc(J_E, fcd);
                out_.cmpAlImm8(0x22);
                out_.jcc(J_E, fcq);
                out_.cmpAlImm8(0x20);
                out_.jcc(J_E, fcsp);
                out_.cmpAlImm8(0x09);
                out_.jcc(J_E, fcsp);
                out_.jmp(fcst);
                out_.def(fcq);
                out_.movRaxFromRbp(-64);
                out_.movR11Imm64(1);
                out_.xorRaxR11();
                out_.movRbpFromRax(-64);
                out_.incRsi();
                out_.jmp(fcopy);
                out_.def(fcsp);
                out_.movRaxFromRbp(-64);
                out_.testRaxRax();
                out_.jcc(J_NE, fcst); // quoted space is content
                out_.jmp(fcd);
                out_.def(fcst);
                out_.movAlFromRsi();
                out_.movBytePtrRdiFromAl();
                out_.incRdi();
                out_.incRsi();
                out_.jmp(fcopy);
                out_.def(fcd);
                out_.storeByteRdiImm0();
                storeArg();
                out_.jmp(flp);
                out_.def(fdone);
            } else {
                // Linux: __nx_args_raw = {argc (incl argv0), argv}.
                out_.leaR10Rip("__nx_args_raw");
                out_.movRaxFromR10();       // argc
                out_.movRcxFromR10Disp8(8); // argv
                out_.movRcxToRbp(-8);
                out_.decRax();
                out_.movRbpFromRax(-24);
                // array = alloc(max(count*8, 8))
                out_.movRaxFromRbp(-24);
                out_.testRaxRax();
                const std::string lz = out_.label("args.lz");
                out_.jcc(J_E, lz);
                out_.shlRaxImm8(3);
                out_.jmp(lz);
                out_.def(lz);
                out_.testRaxRax();
                const std::string lnz = out_.label("args.lnz");
                out_.jcc(J_NE, lnz);
                out_.movRaxImm64(8);
                out_.jmp(lnz);
                out_.def(lnz);
                out_.call("__nx_alloc");
                out_.movRbpFromRax(-16);
                out_.xorEax();
                out_.movRbpFromRax(-32); // i
                const std::string llp = out_.label("args.llp");
                const std::string lbody = out_.label("args.lbody");
                const std::string ldone = out_.label("args.ldone");
                out_.def(llp);
                out_.movRaxFromRbp(-32);
                out_.cmpRaxFromRbp(-24);
                out_.jcc(J_AE, ldone);
                out_.def(lbody);
                // src = argv[i]
                out_.movRaxFromRbp(-8);
                out_.movRdxFromRbp(-32);
                out_.shlRdxImm8(3);
                out_.addRaxRdx();
                out_.movRsiFromRaxPtr();
                out_.call("__nx_strlen");
                out_.movRdxRax();
                out_.movRdxToRbp(-48); // len
                out_.movRaxFromRbp(-48);
                out_.addRaxImm32(1);
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-56);
                // reload src, copy len bytes
                out_.movRaxFromRbp(-8);
                out_.movRdxFromRbp(-32);
                out_.shlRdxImm8(3);
                out_.addRaxRdx();
                out_.movRsiFromRaxPtr();
                out_.movRdiFromRbp(-56);
                out_.movRcxFromRbp(-48);
                out_.cld();
                out_.repMovsb();
                out_.storeByteRdiImm0();
                storeArg();
                out_.jmp(llp);
                out_.def(ldone);
            }
            // publish
            out_.leaR10Rip("__nx_args");
            out_.movRcxFromRbp(-16);
            out_.movR10FromRcx(); // [r10+0] = ptrs
            out_.movRaxFromRbp(-24);
            out_.movR10Disp8FromRax(8); // [r10+8] = count
            out_.movRaxImm64(1);
            out_.movR10Disp8FromRax(16); // state
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_os_args_count: user argument count in rax.
        void Generator::genOsArgsCount() {
            out_.def("__nx_os_args_count");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(32);
            out_.andRspMinus16();
            out_.call("__nx_args_init");
            out_.leaR10Rip("__nx_args");
            out_.movRaxFromR10Disp8(8);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_os_args_get: rax = index -> arena string in rax ("" if OOB).
        void Generator::genOsArgsGet() {
            const std::string oob = out_.label("argsget.oob");
            const std::string ok = out_.label("argsget.ok");
            const std::size_t empty = internString("");
            out_.def("__nx_os_args_get");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // index arrives in rax
            out_.call("__nx_args_init");
            out_.leaR10Rip("__nx_args");
            out_.movR11FromR10Disp8(8); // count
            out_.movRaxFromRbp(-8);
            out_.cmpRaxR11(); // unsigned: negatives are huge
            out_.jcc(J_AE, oob);
            out_.movR11FromR10(); // ptrs base
            out_.movRdxFromRbp(-8);
            out_.shlRdxImm8(3);
            out_.addR11Rdx();
            out_.movRaxFromR11();
            out_.jmp(ok);
            out_.def(oob);
            out_.leaRaxRip("str" + std::to_string(empty));
            out_.def(ok);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_env: rcx = NUL-terminated name -> string value in rax ("" if unset).
        void Generator::genEnv() {
            const std::string empty = out_.label("env.empty");
            const std::string done = out_.label("env.done");
            const std::size_t es = internString("");
            out_.def("__nx_env");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRcxToRbp(-8); // name
            if (target_ == NativeTarget::WindowsX64) {
                // GetEnvironmentVariableA(name, NULL, 0) -> required size.
                out_.xorEdx();
                out_.xorR8d();
                out_.subRsp(32);
                out_.callIAT("__iat_GetEnvironmentVariableA");
                out_.addRsp(32);
                out_.testEaxEax();
                out_.jcc(J_E, empty);
                out_.movEaxEax();      // clean upper rax bits
                out_.movRdxRax();      // rdx = size
                out_.movRdxToRbp(-24); // capacity
                out_.addRdxImm8(16);   // alloc slack
                out_.movRaxRdx();
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-16); // buffer
                out_.movRcxFromRbp(-8);
                out_.movRdxFromRbp(-16);
                out_.movR8FromRbp(-24);
                out_.subRsp(32);
                out_.callIAT("__iat_GetEnvironmentVariableA");
                out_.addRsp(32);
                out_.movRaxFromRbp(-16);
                out_.jmp(done);
            } else {
                // envp = argv + (argc + 1) * 8 on the initial stack.
                out_.leaR10Rip("__nx_args_raw");
                out_.movRaxFromR10();       // argc
                out_.movR11FromR10Disp8(8); // argv
                out_.incRax();
                out_.shlRaxImm8(3);
                out_.addR11Rax();
                out_.movRbpFromR11(-24); // envp cursor (char**)
                const std::string eloop = out_.label("env.loop");
                const std::string enext = out_.label("env.next");
                const std::string ecmp = out_.label("env.cmp");
                const std::string eend = out_.label("env.endcheck");
                const std::string ematch = out_.label("env.match");
                const std::string enomatch = out_.label("env.nomatch");
                const std::string edone = out_.label("env.found");
                out_.def(eloop);
                out_.movRcxFromRbp(-24);
                out_.movRaxFromRcx(); // rax = *envp = entry
                out_.testRaxRax();
                out_.jcc(J_E, empty);
                out_.movRsiRax();
                out_.movRcxFromRbp(-8);
                out_.def(ecmp);
                out_.movAlFromRsi(); // al = entry char
                out_.movDlFromRcx(); // dl = name char
                out_.testDlDl();
                out_.jcc(J_E, eend);
                out_.testAlAl();
                out_.jcc(J_E, enomatch);
                out_.cmpAlDl();
                out_.jcc(J_NE, enomatch);
                out_.incRsi();
                out_.incRcx();
                out_.jmp(ecmp);
                out_.def(eend);
                out_.cmpAlImm8(0x3D); // '='
                out_.jcc(J_E, ematch);
                out_.jmp(enomatch);
                out_.def(ematch);
                out_.incRsi(); // skip '='
                out_.movRaxRsi();
                out_.jmp(edone);
                out_.def(enomatch);
                out_.movRaxFromRbp(-24);
                out_.addRaxImm32(8);
                out_.movRbpFromRax(-24);
                out_.jmp(eloop);
                out_.def(edone);
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(empty);
            out_.leaRaxRip("str" + std::to_string(es));
            out_.jmp(done);
        }

        // __nx_file_read_checked: rcx = path, rdx = out slot (*string).
        // Returns 0/1 in rax. *out = arena string on success, NULL on failure.
        void Generator::genFileReadChecked() {
            const std::string fail = out_.label("fread.fail");
            const std::string failret = out_.label("fread.failret");
            const std::string done = out_.label("fread.done");
            const std::size_t empty = internString("");
            out_.def("__nx_file_read_checked");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(192);
            out_.andRspMinus16();
            out_.movRcxToRbp(-8);  // path
            out_.movRdxToRbp(-16); // out ptr
            out_.xorEax();
            out_.movRbpFromRax(-24); // handle = 0
            // *out = "" (safe default; N++ strings are never NULL)
            out_.leaRaxRip("str" + std::to_string(empty));
            out_.movRcxFromRbp(-16);
            out_.movToRcxFromRax();
            if (target_ == NativeTarget::WindowsX64) {
                // CreateFileA(path, GENERIC_READ, share RW, NULL, OPEN_EXISTING,
                //             FILE_ATTRIBUTE_NORMAL, NULL)
                out_.movRcxFromRbp(-8);
                out_.movEdxImm32(0x80000000u);
                out_.movR8Imm32(3);
                out_.xorR9d();
                out_.subRsp(64);
                out_.movRaxImm64(3); // OPEN_EXISTING
                out_.movRspDisp8FromRax(32);
                out_.movRaxImm64(0x80); // FILE_ATTRIBUTE_NORMAL
                out_.movRspDisp8FromRax(40);
                out_.xorEax();
                out_.movRspDisp8FromRax(48);
                out_.callIAT("__iat_CreateFileA");
                out_.addRsp(64);
                out_.movRbpFromRax(-24); // handle
                // INVALID_HANDLE_VALUE == -1
                out_.movR11Imm64(0xFFFFFFFFFFFFFFFFULL);
                out_.cmpRaxR11();
                out_.jcc(J_E, failret);
                // GetFileSize(handle, NULL)
                out_.movRcxFromRbp(-24);
                out_.xorEdx();
                out_.subRsp(32);
                out_.callIAT("__iat_GetFileSize");
                out_.addRsp(32);
                out_.movEaxEax();
                out_.cmpRaxImm32(0xFFFFFFFFu); // INVALID_FILE_SIZE
                out_.jcc(J_E, fail);
                out_.movRbpFromRax(-32); // size
                // buf = alloc(size + 1)
                out_.movRaxFromRbp(-32);
                out_.addRaxImm32(1);
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-40); // buf
                // ReadFile(h, buf, size, &n, NULL)
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-40);
                out_.movR8FromRbp(-32);
                out_.leaR9FromRbp(-48); // &n
                out_.xorEax();
                out_.movRbpFromRax(-48); // zero n slot (DWORD write leaves upper garbage)
                out_.subRsp(48);
                out_.xorEax();
                out_.movRspDisp8FromRax(32); // lpOverlapped = NULL
                out_.callIAT("__iat_ReadFile");
                out_.addRsp(48);
                out_.testEaxEax();
                out_.jcc(J_E, fail);
                // buf[n] = 0
                out_.movRaxFromRbp(-48); // n
                out_.movRdxFromRbp(-40); // buf
                out_.addRdxRax();
                out_.movBytePtrRdxImm0();
                // *out = buf; close; return 1
                out_.movRaxFromRbp(-40);
                out_.movRcxFromRbp(-16);
                out_.movToRcxFromRax();
                out_.movRcxFromRbp(-24);
                out_.subRsp(32);
                out_.callIAT("__iat_CloseHandle");
                out_.addRsp(32);
                out_.movRaxImm64(1);
                out_.jmp(done);
                out_.def(fail);
                out_.movRcxFromRbp(-24);
                out_.subRsp(32);
                out_.callIAT("__iat_CloseHandle");
                out_.addRsp(32);
                out_.def(failret);
                out_.xorEax();
                out_.jmp(done);
            } else {
                // open(path, O_RDONLY) = syscall 2: rdi=path, rsi=flags, rdx=mode
                out_.movRdiFromRbp(-8);
                out_.xorEsi();
                out_.xorEdx();
                out_.movEaxImm32(2);
                out_.syscall();
                out_.movRbpFromRax(-24); // fd
                out_.testRaxRax();
                out_.jcc(J_S, failret); // fd < 0
                // fstat(fd, statbuf) = syscall 5; st_size at statbuf+48.
                // statbuf at [rbp-112] (144 bytes), size lands at [rbp-64].
                out_.movRdiFromRbp(-24);
                out_.leaRsiFromRbp(-112);
                out_.movEaxImm32(5);
                out_.syscall();
                out_.testRaxRax();
                out_.jcc(J_NE, fail);
                out_.movRaxFromRbp(-64); // st_size
                out_.movRbpFromRax(-32); // size
                // buf = alloc(size + 1)
                out_.movRaxFromRbp(-32);
                out_.addRaxImm32(1);
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-40);
                // read(fd, buf, size) = syscall 0
                out_.movRdiFromRbp(-24);
                out_.movRsiFromRbp(-40);
                out_.movRdxFromRbp(-32);
                out_.xorEax(); // sys_read = 0
                out_.syscall();
                out_.testRaxRax();
                out_.jcc(J_S, fail); // read error
                // buf[n] = 0
                out_.movRdxRax();        // n
                out_.movRaxFromRbp(-40); // buf
                out_.addRaxRdx();
                out_.movBytePtrRaxImm0();
                // close(fd) = syscall 3
                out_.movRdiFromRbp(-24);
                out_.movEaxImm32(3);
                out_.syscall();
                // *out = buf; return 1
                out_.movRaxFromRbp(-40);
                out_.movRcxFromRbp(-16);
                out_.movToRcxFromRax();
                out_.movRaxImm64(1);
                out_.jmp(done);
                out_.def(fail);
                out_.movRdiFromRbp(-24);
                out_.movEaxImm32(3);
                out_.syscall();
                out_.def(failret);
                out_.xorEax();
                out_.jmp(done);
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_file_read: rcx = path -> arena string ("" on failure).
        void Generator::genFileRead() {
            const std::string done = out_.label("fread2.done");
            const std::size_t empty = internString("");
            out_.def("__nx_file_read");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRcxToRbp(-8);
            // local *string slot at [rbp-16]
            out_.xorEax();
            out_.movRbpFromRax(-16);
            out_.leaRdxFromRbp(-16);
            out_.movRcxFromRbp(-8);
            out_.call("__nx_file_read_checked");
            out_.testRaxRax();
            const std::string ok = out_.label("fread2.ok");
            out_.jcc(J_NE, ok);
            out_.leaRaxRip("str" + std::to_string(empty));
            out_.jmp(done);
            out_.def(ok);
            out_.movRaxFromRbp(-16);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_file_write: rcx = path, rdx = data -> bytes written or -1.
        void Generator::genFileWrite() {
            const std::string fail = out_.label("fwrite.fail");
            const std::string failret = out_.label("fwrite.failret");
            const std::string failret2 = out_.label("fwrite.failret2");
            const std::string done = out_.label("fwrite.done");
            out_.def("__nx_file_write");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRcxToRbp(-8);  // path
            out_.movRdxToRbp(-16); // data
            out_.xorEax();
            out_.movRbpFromRax(-24); // handle
            if (target_ == NativeTarget::WindowsX64) {
                // strlen(data)
                out_.movRsiFromRbp(-16);
                out_.call("__nx_strlen");
                out_.movRbpFromRax(-32); // len
                // CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                //             FILE_ATTRIBUTE_NORMAL, NULL)
                out_.movRcxFromRbp(-8);
                out_.movEdxImm32(0x40000000u);
                out_.xorR8d();
                out_.xorR9d();
                out_.subRsp(64);
                out_.movRaxImm64(2); // CREATE_ALWAYS
                out_.movRspDisp8FromRax(32);
                out_.movRaxImm64(0x80);
                out_.movRspDisp8FromRax(40);
                out_.xorEax();
                out_.movRspDisp8FromRax(48);
                out_.callIAT("__iat_CreateFileA");
                out_.addRsp(64);
                out_.movRbpFromRax(-24);
                out_.movR11Imm64(0xFFFFFFFFFFFFFFFFULL);
                out_.cmpRaxR11();
                out_.jcc(J_E, fail);
                // WriteFile(h, data, len, &n, NULL)
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-16);
                out_.movR8FromRbp(-32);
                out_.leaR9FromRbp(-40);
                out_.xorEax();
                out_.movRbpFromRax(-40); // zero n slot
                out_.subRsp(48);
                out_.xorEax();
                out_.movRspDisp8FromRax(32);
                out_.callIAT("__iat_WriteFile");
                out_.addRsp(48);
                out_.testEaxEax();
                const std::string wok = out_.label("fwrite.wok");
                out_.jcc(J_NE, wok);
                out_.jmp(fail);
                out_.def(wok);
                out_.movRcxFromRbp(-24);
                out_.subRsp(32);
                out_.callIAT("__iat_CloseHandle");
                out_.addRsp(32);
                out_.movRaxFromRbp(-40); // n (CloseHandle clobbers rax)
                out_.jmp(done);
                out_.def(fail);
                out_.movRcxFromRbp(-24);
                out_.testRcxRcx();
                out_.jcc(J_E, failret);
                out_.subRsp(32);
                out_.callIAT("__iat_CloseHandle");
                out_.addRsp(32);
                out_.def(failret);
                out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL); // -1
                out_.jmp(done);
            } else {
                // strlen(data)
                out_.movRsiFromRbp(-16);
                out_.call("__nx_strlen");
                out_.movRbpFromRax(-32);
                // open(path, O_WRONLY|O_CREAT|O_TRUNC=577, 0666) = syscall 2
                out_.movRdiFromRbp(-8);
                out_.movRsiImm32(577);
                out_.movRdxImm32(438); // 0666
                out_.movEaxImm32(2);
                out_.syscall();
                out_.movRbpFromRax(-24);
                out_.testRaxRax();
                out_.jcc(J_S, failret2);
                // write(fd, data, len) = syscall 1
                out_.movRdiFromRbp(-24);
                out_.movRsiFromRbp(-16);
                out_.movRdxFromRbp(-32);
                out_.movEaxImm32(1);
                out_.syscall();
                // close(fd) = syscall 3
                out_.movRdiFromRbp(-24);
                out_.movEaxImm32(3);
                out_.syscall();
                out_.jmp(done);
                out_.def(failret2);
                out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL);
                out_.jmp(done);
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_mem_alloc: rax = bytes -> arena pointer as i64 (0 if <= 0).
        void Generator::genMemAlloc() {
            const std::string small = out_.label("memalloc.small");
            const std::string go = out_.label("memalloc.go");
            out_.def("__nx_mem_alloc");
            out_.testRaxRax();
            out_.jcc(J_LE, small);
            out_.addRaxImm32(7);
            out_.andRaxImm32(0xFFFFFFF8u); // round up to 8
            out_.jmp(go);
            out_.def(small);
            out_.xorEax();
            out_.ret();
            out_.def(go);
            out_.call("__nx_alloc");
            out_.ret();
        }

        // __nx_mem_free: rax = pointer. Arena memory is released at exit.
        void Generator::genMemFree() {
            out_.def("__nx_mem_free");
            out_.ret();
        }

        // __nx_mem_read: rax = address -> i64 at address (0 if NULL).
        void Generator::genMemRead() {
            const std::string zero = out_.label("memread.zero");
            const std::string done = out_.label("memread.done");
            out_.def("__nx_mem_read");
            out_.testRaxRax();
            out_.jcc(J_E, zero);
            out_.movRcxRax();
            out_.movRaxFromRcx();
            out_.jmp(done);
            out_.def(zero);
            out_.xorEax();
            out_.def(done);
            out_.ret();
        }

        // __nx_mem_set: rax = ptr, rcx = value, rdx = bytes.
        void Generator::genMemSet() {
            const std::string loop = out_.label("memset.loop");
            const std::string done = out_.label("memset.done");
            out_.def("__nx_mem_set");
            out_.movRdiRax(); // rdi = ptr
            out_.testRdxRdx();
            out_.jcc(J_LE, done);
            // value byte = cl; keep rcx intact (loop counter is rdx)
            out_.def(loop);
            out_.movBytePtrRdiFromCl();
            out_.incRdi();
            out_.decRdx();
            out_.testRdxRdx();
            out_.jcc(J_NE, loop);
            out_.def(done);
            out_.ret();
        }

        // __nx_mem_copy: rax = dst, rcx = src, rdx = bytes.
        void Generator::genMemCopy() {
            const std::string done = out_.label("memcopy.done");
            out_.def("__nx_mem_copy");
            out_.testRdxRdx();
            out_.jcc(J_LE, done);
            out_.movRdiRax();
            out_.movRsiRcx();
            out_.movRcxRdx();
            out_.cld();
            out_.repMovsb();
            out_.def(done);
            out_.ret();
        }

        // __nx_beep: writes BEL (\a) to stdout, matching npp_beep.
        void Generator::genBeep() {
            const std::size_t bell = internString("\a");
            out_.def("__nx_beep");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.leaRcxRip("str" + std::to_string(bell));
            out_.movRdxImm64(1);
            out_.call("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_key_pressed: non-blocking check for pending console input.
        // Windows: GetNumberOfConsoleInputEvents > 0 (any event record,
        // matching _kbhit's "something is waiting" contract closely enough
        // for the native subset). Linux: always false.
        void Generator::genKeyPressed() {
            const std::string done = out_.label("keyp.done");
            const std::string noev = out_.label("keyp.noev");
            out_.def("__nx_key_pressed");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            if (target_ == NativeTarget::WindowsX64) {
                // handle = GetStdHandle(STD_INPUT_HANDLE = -10)
                out_.movRcxImm64(0xFFFFFFFFFFFFFFF6ULL);
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-8);
                // zero count slot, then GetNumberOfConsoleInputEvents(h,&n)
                out_.xorEax();
                out_.movRbpFromRax(-16);
                out_.movRcxFromRbp(-8);
                out_.leaRdxFromRbp(-16);
                out_.subRsp(32);
                out_.callIAT("__iat_GetNumberOfConsoleInputEvents");
                out_.addRsp(32);
                out_.testEaxEax();
                out_.jcc(J_E, noev);
                out_.movRaxFromRbp(-16); // count (upper bits were zeroed)
                out_.testRaxRax();
                out_.jcc(J_E, noev);
                out_.movRaxImm64(1);
                out_.jmp(done);
                out_.def(noev);
                out_.xorEax();
            } else {
                out_.xorEax();
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_read_key: blocking read of one console key.
        // Windows: ReadConsoleInputA loop that skips non-key records and
        // key-up events; returns UnicodeChar, or the virtual-key code when
        // the character is NUL (arrows etc.). Linux: returns -1.
        void Generator::genReadKey() {
            const std::string loop = out_.label("readkey.loop");
            const std::string retCh = out_.label("readkey.retch");
            const std::string retVk = out_.label("readkey.retvk");
            const std::string done = out_.label("readkey.done");
            out_.def("__nx_read_key");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            if (target_ == NativeTarget::WindowsX64) {
                // buf = alloc(32) for one INPUT_RECORD (20 bytes)
                out_.movRaxImm64(32);
                out_.call("__nx_alloc");
                out_.movRdiRax();
                out_.movRdiToRbp(-8);
                // zero the n slot once (DWORD writes leave upper bits)
                out_.xorEax();
                out_.movRbpFromRax(-16);
                // handle = GetStdHandle(-10)
                out_.movRcxImm64(0xFFFFFFFFFFFFFFF6ULL);
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-24);
                out_.def(loop);
                // ReadConsoleInputA(h, buf, 1, &n)
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-8);
                out_.movR8Imm32(1);
                out_.leaR9FromRbp(-16);
                out_.subRsp(32);
                out_.callIAT("__iat_ReadConsoleInputA");
                out_.addRsp(32);
                // EventType (WORD at buf+0) must be KEY_EVENT (1)
                out_.movRcxFromRbp(-8);
                out_.movRaxFromRcx();
                out_.andRaxImm32(0xFFFFu);
                out_.cmpRaxImm32(1);
                out_.jcc(J_NE, loop);
                // bKeyDown (DWORD at buf+4) must be nonzero
                out_.movRaxFromRcxOff(4);
                out_.andRaxImm32(0xFFFFFFFFu);
                out_.testRaxRax();
                out_.jcc(J_E, loop);
                // UnicodeChar (WORD at buf+14)
                out_.movRaxFromRcxOff(14);
                out_.andRaxImm32(0xFFFFu);
                out_.movRbpFromRax(-32);
                out_.testRaxRax();
                out_.jcc(J_NE, retCh);
                // fall back to wVirtualKeyCode (WORD at buf+10)
                out_.movRaxFromRcxOff(10);
                out_.andRaxImm32(0xFFFFu);
                out_.jmp(retVk);
                out_.def(retCh);
                out_.movRaxFromRbp(-32);
                out_.jmp(done);
                out_.def(retVk);
                out_.jmp(done);
            } else {
                out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL); // -1
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

    } // namespace detail
} // namespace npp::native
