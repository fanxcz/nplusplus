#include "native/native_generator.hpp"

#include <stdexcept>

namespace npp::native {
    namespace detail {

        // __nx_write: rcx = pointer, rdx = length. Writes to stdout.
        void Generator::genWrite() {
            out_.def("__nx_write");
            if (target_ == NativeTarget::WindowsX64) {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.movRcxToRbp(-8);
                out_.movRdxToRbp(-16);
                // handle = GetStdHandle(STD_OUTPUT_HANDLE = -11)
                out_.movEcxImm32(static_cast<std::uint32_t>(-11));
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-24);
                // WriteFile(handle, ptr, len, NULL, NULL)
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-8);
                out_.movR8FromRbp(-16);
                out_.xorR9d();
                out_.subRsp(48);
                out_.storeQwordRsp32Imm0();
                out_.callIAT("__iat_WriteFile");
                out_.addRsp(48);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            } else {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiEax();    // fd = 1 (stdout)
                out_.syscall();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }
        }

        // __nx_write_err: rcx = pointer, rdx = length. Writes to stderr.
        void Generator::genWriteErr() {
            out_.def("__nx_write_err");
            if (target_ == NativeTarget::WindowsX64) {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.subRsp(96);
                out_.andRspMinus16();
                out_.movRcxToRbp(-8);
                out_.movRdxToRbp(-16);
                // handle = GetStdHandle(STD_ERROR_HANDLE = -12)
                out_.movEcxImm32(static_cast<std::uint32_t>(-12));
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-24);
                out_.movRcxFromRbp(-24);
                out_.movRdxFromRbp(-8);
                out_.movR8FromRbp(-16);
                out_.xorR9d();
                out_.subRsp(48);
                out_.storeQwordRsp32Imm0();
                out_.callIAT("__iat_WriteFile");
                out_.addRsp(48);
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            } else {
                out_.pushRbp();
                out_.movRbpRsp();
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // fd = 2 (stderr)
                out_.syscall();
                out_.movRspRbp();
                out_.popRbp();
                out_.ret();
            }
        }

        // __nx_print_i64: rax = value. Prints decimal + newline.
        void Generator::genPrintI64() {
            out_.def("__nx_print_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.leaRsiFromRbp(-48);
            out_.decRsi();
            out_.storeByteRsiImm8(0x0A);
            out_.xorR10d();
            out_.testRaxRax();
            const std::string positive = out_.label("i64.pos");
            out_.jcc(J_NS, positive);
            out_.negRax();
            out_.movR10Imm1();
            out_.def(positive);
            out_.movRcxImm10();
            const std::string digit = out_.label("i64.digit");
            out_.def(digit);
            out_.xorEdx();
            out_.divRcx();
            out_.addDl48();
            out_.decRsi();
            out_.storeDlRsi();
            out_.testRaxRax();
            out_.jcc(J_NE, digit);
            out_.testR10d();
            const std::string noSign = out_.label("i64.nosign");
            out_.jcc(J_E, noSign);
            out_.decRsi();
            out_.storeByteRsiImm8(0x2D);
            out_.def(noSign);
            out_.leaRdxFromRbp(-48);
            out_.subRdxRsi();
            out_.movRcxRsi();
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_print_bool: rax = 0/1. Prints "true"/"false" + newline.
        void Generator::genPrintBool() {
            const std::size_t t = internString("true\n");
            const std::size_t f = internString("false\n");
            out_.def("__nx_print_bool");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.testRaxRax();
            const std::string isFalse = out_.label("bool.false");
            const std::string go = out_.label("bool.go");
            out_.jcc(J_E, isFalse);
            out_.leaRcxRip("str" + std::to_string(t));
            out_.movEdxImm32(5);
            out_.jmp(go);
            out_.def(isFalse);
            out_.leaRcxRip("str" + std::to_string(f));
            out_.movEdxImm32(6);
            out_.def(go);
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_write_cstr: rax = NUL-terminated string. Writes it + newline.
        void Generator::genWriteCstr() {
            const std::size_t nl = internString("\n");
            out_.def("__nx_write_cstr");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.movRcxRax();
            out_.movRsiRcx();
            out_.xorEdx();
            const std::string loop = out_.label("cstr.len");
            const std::string done = out_.label("cstr.done");
            out_.def(loop);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, done);
            out_.incRsi();
            out_.incRdx();
            out_.jmp(loop);
            out_.def(done);
            stubCall("__nx_write");
            out_.leaRcxRip("str" + std::to_string(nl));
            out_.movEdxImm32(1);
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_eq: rcx = a, rdx = b (NUL-terminated). Returns 0/1 in rax.
        // Implemented with repne/repe string instructions over strlen(a)+1 bytes
        // (the trailing NUL decides prefix cases correctly).
        void Generator::genStrEq() {
            out_.def("__nx_str_eq");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(32);
            out_.movRcxToRbp(-8);
            out_.movRdxToRbp(-16);
            out_.cld();
            out_.movRdiRcx();
            out_.movRcxImmNeg1();
            out_.xorEax();
            out_.repneScasb();
            out_.notRcx();
            out_.decRcx(); // rcx = strlen(a)
            out_.incRcx(); // include the NUL terminator
            out_.movRsiFromRbp(-8);
            out_.movRdiFromRbp(-16);
            out_.repeCmpsb();
            out_.setcc(CC_E);
            out_.movzxEaxAl();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_string_len: rax = NUL-terminated string. Returns byte length in rax.
        void Generator::genStringLen() {
            out_.def("__nx_string_len");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.movRcxRax();
            out_.xorRax();
            const std::string loop = out_.label("strlen.loop");
            const std::string done = out_.label("strlen.done");
            out_.def(loop);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, done);
            out_.incRcx();
            out_.incRax();
            out_.jmp(loop);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_string_byte_at: rax = string, rcx = index. Returns byte (0-255) in rax, or -1 if
        // out of bounds.
        void Generator::genStringByteAt() {
            out_.def("__nx_string_byte_at");
            out_.pushRbp();
            out_.movRbpRsp();
            // rax = string, rcx = index
            // Save string pointer in rdx
            out_.movRdxRax();
            // Check if index < 0 (signed comparison)
            out_.testRcxRcx();
            const std::string neg = out_.label("sba.neg");
            out_.jcc(J_L, neg);
            // Get string length
            out_.movRsiRdx(); // string -> rsi
            out_.xorRax();    // length counter
            const std::string lenLoop = out_.label("sba.len");
            const std::string lenDone = out_.label("sba.lendone");
            out_.def(lenLoop);
            out_.cmpBytePtrRsi(0);
            out_.jcc(J_E, lenDone);
            out_.incRsi();
            out_.incRax();
            out_.jmp(lenLoop);
            out_.def(lenDone);
            // Compare index (rcx) with length (rax)
            out_.cmpRcxRax();
            out_.jcc(J_AE, neg); // index >= length -> out of bounds
            // In bounds: load byte at index (string pointer in rdx)
            out_.movRsiRdx(); // string -> rsi
            out_.addRsiRcx(); // rsi += index
            out_.movAlFromRsi();
            out_.movzxEaxAl();
            const std::string ret = out_.label("sba.ret");
            out_.jmp(ret);
            out_.def(neg);
            out_.movRaxImm64(0xFFFFFFFFFFFFFFFFu); // -1
            out_.def(ret);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_cat: rcx = a, rdx = b. Returns a fresh arena string in rax.
        void Generator::genStrCat() {
            out_.def("__nx_str_cat");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.movRcxToRbp(-8);
            out_.movRdxToRbp(-16);
            out_.movRsiFromRbp(-8);
            out_.xorR8d();
            const std::string lenA = out_.label("cat.lenA");
            const std::string gotA = out_.label("cat.gotA");
            const std::string lenB = out_.label("cat.lenB");
            const std::string gotB = out_.label("cat.gotB");
            const std::string oom = out_.label("cat.oom");
            out_.def(lenA);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, gotA);
            out_.incRsi();
            out_.incR8();
            out_.jmp(lenA);
            out_.def(gotA);
            out_.movRsiFromRbp(-16);
            out_.xorR9d();
            out_.def(lenB);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, gotB);
            out_.incRsi();
            out_.incR9();
            out_.jmp(lenB);
            out_.def(gotB);
            out_.movRaxR8();
            out_.addRaxR9();
            out_.incRax();
            out_.leaR10Rip("__nx_arena");
            out_.movR11FromR10Disp8(8);
            out_.movRsiR11();
            out_.addRaxRsi();
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            out_.movR10Disp8FromRax(8);
            out_.movRaxRsi();
            out_.movRdiRsi();
            out_.movRsiFromRbp(-8);
            out_.movRcxR8();
            out_.repMovsb();
            out_.movRsiFromRbp(-16);
            out_.movRcxR9();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

        // __nx_str_i64: rax = value. Returns a fresh arena string in rax.
        void Generator::genStrI64() {
            out_.def("__nx_str_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.leaRsiFromRbp(-32);
            out_.xorR10d();
            out_.testRaxRax();
            const std::string positive = out_.label("s64.pos");
            out_.jcc(J_NS, positive);
            out_.negRax();
            out_.movR10Imm1();
            out_.def(positive);
            out_.movRcxImm10();
            const std::string digit = out_.label("s64.digit");
            const std::string noSign = out_.label("s64.nosign");
            const std::string oom = out_.label("s64.oom");
            out_.def(digit);
            out_.xorEdx();
            out_.divRcx();
            out_.addDl48();
            out_.decRsi();
            out_.storeDlRsi();
            out_.testRaxRax();
            out_.jcc(J_NE, digit);
            out_.testR10d();
            out_.jcc(J_E, noSign);
            out_.decRsi();
            out_.storeByteRsiImm8(0x2D);
            out_.def(noSign);
            out_.leaRdxFromRbp(-32);
            out_.subRdxRsi();
            out_.leaR10Rip("__nx_arena");
            out_.movRdiFromR10Disp8(8);
            out_.movRaxRdx();
            out_.incRax();
            out_.addRaxRdi();
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            out_.movR10Disp8FromRax(8);
            out_.movRaxRdi();
            out_.movRcxRdx();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

        // __nx_input: rax = prompt (NUL-terminated). Prints it, reads one line
        // from stdin, and returns a fresh arena string without the newline.
        void Generator::genInput() {
            out_.def("__nx_input");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRcxRax();
            out_.movRsiRcx();
            out_.xorEdx();
            const std::string plen = out_.label("in.plen");
            const std::string pdone = out_.label("in.pdone");
            out_.def(plen);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, pdone);
            out_.incRsi();
            out_.incRdx();
            out_.jmp(plen);
            out_.def(pdone);
            stubCall("__nx_write");
            // rsi = arena cursor (callee-saved, survives calls); [rbp-8] = start.
            // r10 (arena base) is reloaded at the top of every iteration because
            // OS calls destroy volatile registers.
            out_.leaR10Rip("__nx_arena");
            out_.movR11FromR10Disp8(8);
            out_.movRsiR11();
            out_.movRbpFromRsi(-8);
            if (target_ == NativeTarget::WindowsX64) {
                out_.movEcxImm32(static_cast<std::uint32_t>(-10)); // STD_INPUT_HANDLE
                out_.subRsp(32);
                out_.callIAT("__iat_GetStdHandle");
                out_.addRsp(32);
                out_.movRbpFromRax(-16);
            }
            // Read one byte at a time straight into the arena. Chunked reads
            // would silently drop bytes past the first '\n', which breaks
            // multi-line piped input and input_i64 retries.
            const std::string readLoop = out_.label("in.read");
            const std::string foundNl = out_.label("in.nl");
            const std::string eof = out_.label("in.eof");
            const std::string oom = out_.label("in.oom");
            out_.def(readLoop);
            out_.leaR10Rip("__nx_arena");
            out_.movRaxRsi();
            out_.addRaxImm32(2);
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            out_.jcc(J_A, oom);
            if (target_ == NativeTarget::WindowsX64) {
                out_.movRcxFromRbp(-16); // stdin handle
                out_.movRdxRsi();        // one byte straight into the arena
                out_.movR8Imm32(1);
                out_.subRsp(64);
                out_.storeQwordRsp32Imm0();
                out_.leaR9FromRsp8(0x28);
                out_.callIAT("__iat_ReadFile");
                out_.movEaxFromRsp8(0x28);
                out_.addRsp(64);
                out_.testRaxRax();
                out_.jcc(J_E, eof);
            } else {
                out_.movEaxImm32(0); // sys_read
                out_.xorEdi();       // fd 0
                out_.movRdxImm32(1);
                out_.syscall();
                out_.testRaxRax();
                out_.jcc(J_LE, eof);
            }
            out_.movAlFromRsi();
            out_.cmpAlImm10();
            out_.jcc(J_E, foundNl);
            out_.incRsi();
            out_.jmp(readLoop);
            out_.def(foundNl);
            // total = cursor - start; drop a preceding '\r' (Windows console).
            out_.movRaxRsi();
            out_.subRaxFromRbp(-8);
            out_.testRaxRax();
            out_.jcc(J_E, eof);
            out_.movAlFromRsiMinus1();
            out_.cmpAlImm13();
            out_.jcc(J_NE, eof);
            out_.decRsi();
            out_.def(eof);
            out_.storeByteRsiImm0();
            out_.incRsi();
            out_.leaR10Rip("__nx_arena"); // r10 died in the read call
            out_.movR10Disp8FromRsi(8);
            out_.movRaxFromRbp(-8);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

        // __nx_str_to_i64: rax = NUL-terminated string. Parses optional
        // whitespace, an optional '+'/'-' sign and decimal digits. Returns the
        // value in rax and a validity flag in edx (1 = valid, 0 = invalid),
        // mirroring strtoll + full-consumption semantics of npp_input_i64.
        void Generator::genStrToI64() {
            out_.def("__nx_str_to_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(32);
            out_.movRsiRax(); // cursor
            const std::string lead = out_.label("atoi.lead");
            const std::string advLead = out_.label("atoi.advlead");
            const std::string sign = out_.label("atoi.sign");
            const std::string plus = out_.label("atoi.plus");
            const std::string minus = out_.label("atoi.minus");
            const std::string digits = out_.label("atoi.digits");
            const std::string dig = out_.label("atoi.dig");
            const std::string fin = out_.label("atoi.fin");
            const std::string trail = out_.label("atoi.trail");
            const std::string advTrail = out_.label("atoi.advtrail");
            const std::string tend = out_.label("atoi.tend");
            const std::string apply = out_.label("atoi.apply");
            const std::string noNeg = out_.label("atoi.noneg");
            const std::string invalid = out_.label("atoi.invalid");
            const std::string out = out_.label("atoi.out");
            // Skip leading spaces/tabs.
            out_.def(lead);
            out_.cmpBytePtrRsi(0x20);
            out_.jcc(J_E, advLead);
            out_.cmpBytePtrRsi(0x09);
            out_.jcc(J_NE, sign);
            out_.def(advLead);
            out_.incRsi();
            out_.jmp(lead);
            // Optional sign: r10d = 0 none, 1 '+', 2 '-'.
            out_.def(sign);
            out_.xorR10d();
            out_.cmpBytePtrRsi(0x2B);
            out_.jcc(J_E, plus);
            out_.cmpBytePtrRsi(0x2D);
            out_.jcc(J_E, minus);
            out_.jmp(digits);
            out_.def(plus);
            out_.incRsi();
            out_.movR10Imm32(1);
            out_.jmp(digits);
            out_.def(minus);
            out_.incRsi();
            out_.movR10Imm32(2);
            out_.def(digits);
            out_.xorEdx(); // result = 0
            out_.xorR8d(); // digit count = 0
            out_.def(dig);
            out_.movAlFromRsi();
            out_.cmpAlImm8(0x30); // '0'
            out_.jcc(J_B, fin);
            out_.cmpAlImm8(0x39); // '9'
            out_.jcc(J_A, fin);
            out_.subAlImm8(0x30);
            out_.movzxEaxAl();
            out_.movRcxImm32(10);
            out_.imulRdxRcx();
            out_.addRdxRax();
            out_.incRsi();
            out_.incR8();
            out_.jmp(dig);
            out_.def(fin);
            // Skip trailing spaces/tabs; the next byte must be NUL.
            out_.def(trail);
            out_.cmpBytePtrRsi(0x20);
            out_.jcc(J_E, advTrail);
            out_.cmpBytePtrRsi(0x09);
            out_.jcc(J_NE, tend);
            out_.def(advTrail);
            out_.incRsi();
            out_.jmp(trail);
            out_.def(tend);
            out_.cmpBytePtrRsi(0);
            out_.jcc(J_NE, invalid);
            // Valid iff digits were seen, or no sign was consumed at all
            // (empty/whitespace-only input parses as 0, like strtoll).
            out_.testR8R8();
            out_.jcc(J_NE, apply);
            out_.testR10d();
            out_.jcc(J_NE, invalid);
            out_.def(apply);
            out_.testR10dImm32(2);
            out_.jcc(J_E, noNeg);
            out_.negRdx();
            out_.def(noNeg);
            out_.movRaxRdx();
            out_.movEdxImm32(1);
            out_.jmp(out);
            out_.def(invalid);
            out_.xorEax();
            out_.xorEdx();
            out_.def(out);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_input_i64: rax = prompt. Prints it, reads lines until a valid
        // integer is entered, and returns the parsed i64 in rax.
        void Generator::genInputI64() {
            out_.def("__nx_input_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // prompt
            const std::string loop = out_.label("in64.loop");
            const std::string done = out_.label("in64.done");
            out_.def(loop);
            out_.movRaxFromRbp(-8);
            out_.call("__nx_input");
            out_.call("__nx_str_to_i64");
            out_.testRdxRdx();
            out_.jcc(J_NE, done);
            const std::size_t msg = internString("Invalid integer. Try again.\n");
            out_.leaRcxRip("str" + std::to_string(msg));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[msg].size()));
            stubCall("__nx_write_err");
            out_.jmp(loop);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_exit: rcx = exit code. Terminates the process.
        void Generator::genExit() {
            out_.def("__nx_exit");
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRdiRcx();
                out_.movEaxImm32(60); // sys_exit
                out_.syscall();
            }
        }

        // __nx_time_ms: returns monotonic milliseconds in rax.
        void Generator::genTimeMs() {
            out_.def("__nx_time_ms");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            if (target_ == NativeTarget::WindowsX64) {
                out_.subRsp(32);
                out_.callIAT("__iat_GetTickCount64");
                out_.addRsp(32);
            } else {
                // clock_gettime(CLOCK_MONOTONIC=1, &ts at rbp-16) = syscall 228.
                out_.movEdiImm32(1);
                out_.leaRsiFromRbp(-16);
                out_.movEaxImm32(228);
                out_.syscall();
                out_.movRaxFromRbp(-16); // tv_sec
                out_.movRcxImm32(1000);
                out_.imulRaxRcx();
                out_.movR11Rax();
                out_.movRaxFromRbp(-8); // tv_nsec
                out_.movRcxImm32(1000000);
                out_.xorEdx();
                out_.divRcx();
                out_.addRaxR11();
            }
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_sleep: rcx = milliseconds. ms <= 0 returns immediately.
        void Generator::genSleep() {
            out_.def("__nx_sleep");
            out_.testRcxRcx();
            const std::string done = out_.label("sleep.done");
            out_.jcc(J_LE, done);
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            if (target_ == NativeTarget::WindowsX64) {
                out_.subRsp(32);
                out_.callIAT("__iat_Sleep"); // ecx = low 32 bits of ms
                out_.addRsp(32);
            } else {
                // nanosleep(&ts, NULL) = syscall 35; ts at rbp-16.
                out_.movRaxRcx(); // ms
                out_.movRcxImm32(1000);
                out_.xorEdx();
                out_.divRcx(); // rax = sec, rdx = rem
                out_.movRbpFromRax(-16);
                out_.movRaxRdx(); // rem (< 1000)
                out_.movRcxImm32(1000000);
                out_.imulRaxRcx();
                out_.movRbpFromRax(-8);
                out_.leaRdiFromRbp(-16);
                out_.xorEsi();
                out_.movEaxImm32(35);
                out_.syscall();
            }
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(done);
            out_.ret();
        }

        // __nx_rng_next: SplitMix64 step over the __nx_rng state word (the
        // second qword of the writable TOC page). Returns rax.
        void Generator::genRngNext() {
            out_.def("__nx_rng_next");
            out_.leaR10Rip("__nx_rng");
            out_.movRaxFromR10();
            out_.testRaxRax();
            const std::string have = out_.label("rng.have");
            out_.jcc(J_NE, have);
            // First call: seed from the time-stamp counter.
            out_.rdtsc(); // eax = low, edx = high
            out_.shlRdxImm8(32);
            out_.movRcxRdx();
            out_.orRaxRcx();
            out_.movRcxImm64(0x9E3779B97F4A7C15ULL);
            out_.addRaxRcx();
            out_.movR10FromRax();
            out_.def(have);
            out_.movRaxFromR10();
            out_.movRcxImm64(0x9E3779B97F4A7C15ULL);
            out_.addRaxRcx();
            out_.movR10FromRax(); // state += golden gamma
            // SplitMix64 finalizer.
            out_.movRcxRax();
            out_.shrRcxImm8(30);
            out_.xorRaxRcx();
            out_.movRcxImm64(0xBF58476D1CE4E5B9ULL);
            out_.imulRaxRcx();
            out_.movRcxRax();
            out_.shrRcxImm8(27);
            out_.xorRaxRcx();
            out_.movRcxImm64(0x94D049BB133111EBULL);
            out_.imulRaxRcx();
            out_.movRcxRax();
            out_.shrRcxImm8(31);
            out_.xorRaxRcx();
            out_.ret();
        }

        // __nx_random_i64: rcx = min, rdx = max. Unbiased value in [min, max],
        // matching npp_random_i64 (swap on inverted bounds, rejection sampling,
        // wrapping add for the full i64 range).
        void Generator::genRandomI64() {
            out_.def("__nx_random_i64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.movRcxToRbp(-8);  // min
            out_.movRdxToRbp(-16); // max
            // Swap when min > max (signed).
            out_.movRaxRcx();
            out_.cmpRaxFromRbp(-16);
            const std::string cont = out_.label("rnd.cont");
            out_.jcc(J_LE, cont);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRbp(-16);
            out_.movRbpFromRcx(-8);
            out_.movRbpFromRax(-16);
            out_.def(cont);
            // span = max - min + 1 (unsigned; 0 means the full i64 range).
            out_.movRaxFromRbp(-16);
            out_.subRaxFromRbp(-8);
            out_.incRax();
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            const std::string full = out_.label("rnd.full");
            out_.jcc(J_E, full);
            // threshold = (0 - span) % span; resample while value < threshold.
            out_.xorRax();
            out_.subRaxFromRbp(-24);
            out_.movRcxFromRbp(-24);
            out_.xorEdx();
            out_.divRcx();
            out_.movRaxRdx(); // threshold = remainder, not quotient
            out_.movRbpFromRax(-32);
            const std::string again = out_.label("rnd.again");
            const std::string have = out_.label("rnd.have");
            out_.def(again);
            out_.call("__nx_rng_next");
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_B, again);
            out_.movRcxFromRbp(-24);
            out_.xorEdx();
            out_.divRcx(); // rdx = value % span
            out_.movRaxRdx();
            const std::string done = out_.label("rnd.done");
            out_.jmp(done);
            out_.def(full);
            out_.call("__nx_rng_next");
            out_.def(done);
            // result = min + offset; wrapping add matches the runtime.
            out_.movRdxRax();
            out_.movRaxFromRbp(-8);
            out_.addRaxRdx();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_file_exists: rcx = NUL-terminated path. Returns 0/1 in rax.
        void Generator::genFileExists() {
            out_.def("__nx_file_exists");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            if (target_ == NativeTarget::WindowsX64) {
                // GetFileAttributesA(path): rcx already holds the path.
                out_.subRsp(32);
                out_.callIAT("__iat_GetFileAttributesA");
                out_.addRsp(32);
                out_.cmpEaxImm8(0xFF); // INVALID_FILE_ATTRIBUTES == -1
                out_.setcc(CC_NE);
                out_.movzxEaxAl();
            } else {
                // faccessat(AT_FDCWD=-100, path, F_OK=0, 0) = syscall 269.
                out_.movRaxImm64(0xFFFFFFFFFFFFFF9CULL);
                out_.movRdiRax();
                out_.movRsiRcx();
                out_.xorEdx();
                out_.xorR10d();
                out_.movEaxImm32(269);
                out_.syscall();
                out_.testRaxRax();
                out_.setcc(CC_E);
                out_.movzxEaxAl();
            }
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_oom: clean out-of-string-memory failure (stderr + exit 1).
        void Generator::genOom() {
            const std::size_t m = internString("N++ runtime error: out of string memory\n");
            out_.def("__nx_oom");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genDivZero() {
            const std::size_t m = internString("N++ runtime error: division by zero\n");
            out_.def("__nx_divzero");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genDivOverflow() {
            const std::size_t m =
                internString("N++ runtime error: signed integer division overflow\n");
            out_.def("__nx_divoverflow");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                stubCall("__nx_write");
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movRsiRcx();
                out_.movEaxImm32(1); // sys_write
                out_.movEdiImm32(2); // stderr
                out_.syscall();
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

        void Generator::genEntry() {
            out_.def("_start");
            // Reserve the string bump arena before user code runs: 16 MiB of
            // committed/mapped memory, tracked in the __nx_arena words.
            const std::string noArena = out_.label("start.noarena");
            if (target_ == NativeTarget::WindowsX64) {
                // VirtualAlloc(NULL, 16 MiB, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE).
                // OS entry rsp is 8 (mod 16): reserve 8 bytes padding + 32 shadow.
                out_.xorEcx();
                out_.movRdxImm32(kArenaSize);
                out_.movR8Imm32(0x3000);
                out_.movR9Imm32(0x04);
                out_.subRsp(40);
                out_.callIAT("__iat_VirtualAlloc");
                out_.addRsp(40);
                out_.testRaxRax();
                out_.jcc(J_E, noArena);
            } else {
                // mmap(NULL, 16 MiB, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0).
                out_.xorEdi();
                out_.movRsiImm32(kArenaSize);
                out_.movRdxImm32(3);
                out_.movR10Imm32(0x22);
                out_.movR8Imm32(0xFFFFFFFFu);
                out_.movR9Imm32(0);
                out_.movEaxImm32(9);
                out_.syscall();
                out_.cmpRaxImm8(0xFF); // MAP_FAILED == -1
                out_.jcc(J_E, noArena);
            }
            out_.leaR10Rip("__nx_arena");
            out_.movR10FromRax();
            out_.movR10Disp8FromRax(8);
            out_.call("main");
            if (target_ == NativeTarget::WindowsX64) {
                // ExitProcess(exitCode). The caller's stack is balanced again, so
                // rsp is 8 (mod 16); reserve 8 bytes padding + 32 bytes shadow.
                out_.movEcxEax();
                out_.subRsp(40);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movEdiEax();
                out_.movEaxImm32(60); // sys_exit
                out_.syscall();
            }
            out_.def(noArena);
            out_.jmp("__nx_oom");
        }

    } // namespace detail
} // namespace npp::native
