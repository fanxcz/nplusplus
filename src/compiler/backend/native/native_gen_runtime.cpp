#include "native/native_generator.hpp"

#include <cstring>
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

        // IEEE-754 constants are interned as raw 8-byte strings; the image
        // writer appends a NUL terminator that SSE memory operands never read.
        // (The lambda lives inside each generator so it can call internString.)

        // __nx_str_f64: xmm0 = value. Returns arena string in rax, length in rdx.
        // Renders like printf %.15g: up to 15 significant digits, round-half-up,
        // fixed notation for decimal exponents in [-4, 14], scientific otherwise.
        void Generator::genStrF64() {
            const auto f64c = [this](double v) {
                std::string s(8, '\0');
                std::memcpy(s.data(), &v, 8);
                return "str" + std::to_string(internString(s));
            };
            const std::string ten = f64c(10.0);
            const std::string one = f64c(1.0);

            out_.def("__nx_str_f64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(320);
            out_.andRspMinus16();

            const std::string done = out_.label("f64.done");

            // xmm0 = |v|; sign bit saved to [rbp-24].
            out_.movqRaxXmm0();
            out_.movRcxRax();
            out_.shrRcxImm8(63);
            out_.movRbpFromRcx(-24);
            out_.movR11Imm64(0x7FFFFFFFFFFFFFFFULL);
            out_.andRaxR11();
            out_.movqXmm0Rax();

            // Zero (the saved sign still produces "-0" for negative zero).
            out_.xorpdXmm2Xmm2();
            out_.ucomisdXmm0Xmm2();
            const std::string zeroPath = out_.label("f64.zero");
            out_.jcc(J_E, zeroPath);

            // inf / nan: biased exponent field all ones.
            out_.movqRaxXmm0();
            out_.shrRaxImm8(52);
            out_.andRaxImm32(0x7FF);
            out_.cmpRaxImm32(0x7FF);
            const std::string normalPath = out_.label("f64.normal");
            out_.jcc(J_NE, normalPath);
            out_.movqRaxXmm0();
            out_.movR11Imm64(0x000FFFFFFFFFFFFFULL);
            out_.andRaxR11();
            out_.testRaxRax();
            const std::string infPath = out_.label("f64.inf");
            const std::string nanPath = out_.label("f64.nan");
            out_.jcc(J_NE, nanPath);
            out_.def(infPath);
            {
                const std::string l1 = out_.label("f64.s1");
                out_.leaRdiFromRbp(-224);
                out_.movRcxFromRbp(-24);
                out_.testRcxRcx();
                out_.jcc(J_E, l1);
                out_.storeByteRdiImm8(0x2D); // '-'
                out_.incRdi();
                out_.def(l1);
                out_.storeByteRdiImm8(0x69); // 'i'
                out_.incRdi();
                out_.storeByteRdiImm8(0x6E); // 'n'
                out_.incRdi();
                out_.storeByteRdiImm8(0x66); // 'f'
                out_.incRdi();
            }
            out_.jmp(done);
            out_.def(nanPath);
            {
                const std::string l1 = out_.label("f64.s1");
                out_.leaRdiFromRbp(-224);
                out_.movRcxFromRbp(-24);
                out_.testRcxRcx();
                out_.jcc(J_E, l1);
                out_.storeByteRdiImm8(0x2D);
                out_.incRdi();
                out_.def(l1);
                out_.storeByteRdiImm8(0x6E); // 'n'
                out_.incRdi();
                out_.storeByteRdiImm8(0x61); // 'a'
                out_.incRdi();
                out_.storeByteRdiImm8(0x6E); // 'n'
                out_.incRdi();
            }
            out_.jmp(done);
            out_.def(zeroPath);
            {
                const std::string l1 = out_.label("f64.s1");
                out_.leaRdiFromRbp(-224);
                out_.movRcxFromRbp(-24);
                out_.testRcxRcx();
                out_.jcc(J_E, l1);
                out_.storeByteRdiImm8(0x2D);
                out_.incRdi();
                out_.def(l1);
                out_.storeByteRdiImm8(0x30); // '0'
                out_.incRdi();
            }
            out_.jmp(done);

            out_.def(normalPath);
            // xmm2 = t normalized to [1,10); r10 = decimal exponent e10.
            out_.movsdXmm2Xmm0();
            out_.xorR10d();
            const std::string ge10 = out_.label("f64.ge10");
            const std::string ge10done = out_.label("f64.ge10d");
            out_.def(ge10);
            out_.ucomisdXmm2Mem(ten);
            out_.jcc(J_B, ge10done); // t < 10 -> done
            out_.divsdXmm2Mem(ten);
            out_.incR10();
            out_.jmp(ge10);
            out_.def(ge10done);
            const std::string lt1 = out_.label("f64.lt1");
            const std::string lt1done = out_.label("f64.lt1d");
            out_.def(lt1);
            out_.ucomisdXmm2Mem(one);
            out_.jcc(J_AE, lt1done); // t >= 1 -> done
            out_.mulsdXmm2Mem(ten);
            out_.decR10();
            out_.jmp(lt1);
            out_.def(lt1done);

            // Extract 16 digits (15 significant + 1 rounding digit) into
            // [rbp-160 .. rbp-145].
            out_.leaRsiFromRbp(-160);
            out_.movR8Imm32(16);
            const std::string digLoop = out_.label("f64.dig");
            out_.def(digLoop);
            out_.cvttsd2siRaxXmm2();
            out_.movRbpFromRax(-8);
            out_.cvtsi2sdXmm0FromRbp(-8);
            out_.subsdXmm2Xmm0();
            out_.mulsdXmm2Mem(ten);
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRsi();
            out_.incRsi();
            out_.decR8();
            out_.testR8R8();
            out_.jcc(J_NE, digLoop);

            // Round half-up on the 16th digit, with carry into sig digits.
            const std::string trimStart = out_.label("f64.trim");
            out_.leaRsiFromRbp(-145);
            out_.movAlFromRsi();
            out_.cmpAlImm8(0x35); // '5'
            out_.jcc(J_B, trimStart);
            out_.leaRsiFromRbp(-146); // last significant digit
            out_.movR9Imm32(15);
            const std::string carryLoop = out_.label("f64.carry");
            const std::string carryInc = out_.label("f64.carryinc");
            out_.def(carryLoop);
            out_.movAlFromRsi();
            out_.cmpAlImm8(0x39); // '9'
            out_.jcc(J_NE, carryInc);
            out_.storeByteRsiImm8(0x30);
            out_.decRsi();
            out_.decR9();
            out_.cmpR9Imm8(0);
            out_.jcc(J_NE, carryLoop);
            // All nines carried out: digits become '1' + 14 zeros, e10 += 1.
            out_.leaRsiFromRbp(-160);
            out_.storeByteRsiImm8(0x31); // '1'
            out_.leaRdiFromRbp(-159);
            out_.movEaxImm32(0x30); // '0'
            out_.movRcxImm32(14);
            out_.repStosb();
            out_.incR10();
            out_.jmp(trimStart);
            out_.def(carryInc);
            out_.addAlImm8(1);
            out_.storeAlRsi();
            out_.def(trimStart);

            // Trim trailing zero digits; r9 = significant digit count.
            out_.movR9Imm32(15);
            out_.leaRsiFromRbp(-146);
            const std::string trimLoop = out_.label("f64.trimloop");
            const std::string trimDone = out_.label("f64.trimdone");
            out_.def(trimLoop);
            out_.cmpR9Imm8(1);
            out_.jcc(J_E, trimDone);
            out_.cmpBytePtrRsi(0x30);
            out_.jcc(J_NE, trimDone);
            out_.decR9();
            out_.decRsi();
            out_.jmp(trimLoop);
            out_.def(trimDone);

            // Output buffer at [rbp-224]; rdi walks forward.
            out_.leaRdiFromRbp(-224);
            out_.movRcxFromRbp(-24);
            out_.testRcxRcx();
            const std::string noSignOut = out_.label("f64.nosignout");
            out_.jcc(J_E, noSignOut);
            out_.storeByteRdiImm8(0x2D);
            out_.incRdi();
            out_.def(noSignOut);

            // %g rule: scientific when e10 < -4 or e10 >= 15.
            const std::string sciPath = out_.label("f64.sci");
            out_.cmpR10Imm8(0xFC); // -4
            out_.jcc(J_L, sciPath);
            out_.cmpR10Imm8(15);
            out_.jcc(J_GE, sciPath);

            // ---- fixed notation ----
            const std::string fixedPos = out_.label("f64.fpos");
            const std::string fixedNeg = out_.label("f64.fneg");
            out_.testR10d();
            out_.jcc(J_NS, fixedPos);
            out_.def(fixedNeg);          // e10 in [-4, -1]: "0." + zeros + digits
            out_.storeByteRdiImm8(0x30); // '0'
            out_.incRdi();
            out_.storeByteRdiImm8(0x2E); // '.'
            out_.incRdi();
            out_.movRaxR10();
            out_.negRax();
            out_.decRax(); // zeros = -e10 - 1
            out_.movR8Rax();
            const std::string zerosDone = out_.label("f64.zerosdone");
            out_.testR8R8();
            out_.jcc(J_E, zerosDone);
            const std::string zeroLoop = out_.label("f64.zeros");
            out_.def(zeroLoop);
            out_.storeByteRdiImm8(0x30);
            out_.incRdi();
            out_.decR8();
            out_.testR8R8();
            out_.jcc(J_NE, zeroLoop);
            out_.def(zerosDone);
            out_.movR8R9();
            out_.leaRsiFromRbp(-160);
            {
                const std::string cp = out_.label("f64.cpn");
                out_.def(cp);
                out_.movAlFromRsi();
                out_.storeAlRdi();
                out_.incRsi();
                out_.incRdi();
                out_.decR8();
                out_.testR8R8();
                out_.jcc(J_NE, cp);
            }
            out_.jmp(done);

            out_.def(fixedPos); // e10 in [0, 14]
            out_.incR10();      // intdigits = e10 + 1
            const std::string padPath = out_.label("f64.pad");
            out_.cmpR10R9(); // intdigits vs n
            out_.jcc(J_G, padPath);
            // intdigits <= n: int part, optional "." + fraction.
            out_.movR8R10();
            out_.leaRsiFromRbp(-160);
            {
                const std::string cp = out_.label("f64.cpi");
                out_.def(cp);
                out_.movAlFromRsi();
                out_.storeAlRdi();
                out_.incRsi();
                out_.incRdi();
                out_.decR8();
                out_.testR8R8();
                out_.jcc(J_NE, cp);
            }
            // frac digits = n - intdigits
            out_.movRaxR9();
            out_.subRaxR10();
            out_.testRaxRax();
            out_.jcc(J_E, done);
            out_.storeByteRdiImm8(0x2E); // '.'
            out_.incRdi();
            out_.movR8Rax();
            {
                const std::string cp = out_.label("f64.cpf");
                out_.def(cp);
                out_.movAlFromRsi();
                out_.storeAlRdi();
                out_.incRsi();
                out_.incRdi();
                out_.decR8();
                out_.testR8R8();
                out_.jcc(J_NE, cp);
            }
            out_.jmp(done);

            out_.def(padPath); // intdigits > n: digits then zero padding
            out_.movR8R9();
            out_.leaRsiFromRbp(-160);
            {
                const std::string cp = out_.label("f64.cpp");
                out_.def(cp);
                out_.movAlFromRsi();
                out_.storeAlRdi();
                out_.incRsi();
                out_.incRdi();
                out_.decR8();
                out_.testR8R8();
                out_.jcc(J_NE, cp);
            }
            out_.movRaxR10();
            out_.subRaxR9(); // zeros = intdigits - n
            out_.movR8Rax();
            const std::string padZero = out_.label("f64.pz");
            out_.def(padZero);
            out_.storeByteRdiImm8(0x30);
            out_.incRdi();
            out_.decR8();
            out_.testR8R8();
            out_.jcc(J_NE, padZero);
            out_.jmp(done);

            // ---- scientific notation ----
            out_.def(sciPath);
            out_.leaRsiFromRbp(-160);
            out_.movAlFromRsi();
            out_.storeAlRdi();
            out_.incRsi();
            out_.incRdi();
            // fraction digits = n - 1
            out_.movRaxR9();
            out_.decRax();
            out_.testRaxRax();
            const std::string sciNoFrac = out_.label("f64.scinofrac");
            out_.jcc(J_E, sciNoFrac);
            out_.storeByteRdiImm8(0x2E); // '.'
            out_.incRdi();
            out_.movR8Rax();
            {
                const std::string cp = out_.label("f64.scif");
                out_.def(cp);
                out_.movAlFromRsi();
                out_.storeAlRdi();
                out_.incRsi();
                out_.incRdi();
                out_.decR8();
                out_.testR8R8();
                out_.jcc(J_NE, cp);
            }
            out_.def(sciNoFrac);
            out_.storeByteRdiImm8(0x65); // 'e'
            out_.incRdi();
            // exponent sign
            out_.testR10d();
            const std::string expPos = out_.label("f64.exppos");
            out_.jcc(J_NS, expPos);
            out_.storeByteRdiImm8(0x2D);
            out_.incRdi();
            out_.movRaxR10();
            out_.negRax();
            out_.movR10Rax();
            out_.def(expPos);
            // print r10 as decimal, at least two digits (%g style).
            out_.cmpR10Imm8(100);
            const std::string exp3 = out_.label("f64.exp3");
            out_.jcc(J_AE, exp3);
            out_.cmpR10Imm8(10);
            const std::string exp2 = out_.label("f64.exp2");
            out_.jcc(J_AE, exp2);
            // single digit: pad with '0' first
            out_.storeByteRdiImm8(0x30);
            out_.incRdi();
            out_.movRaxR10();
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.jmp(done);
            out_.def(exp2); // 10..99
            out_.movRaxR10();
            out_.movRcxImm32(10);
            out_.xorEdx();
            out_.divRcx();        // rax = tens, rdx = ones
            out_.movRdxToRbp(-8); // save remainder: dl writes below clobber rdx
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.movRaxFromRbp(-8);
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.jmp(done);
            out_.def(exp3); // >= 100
            out_.movRaxR10();
            out_.movRcxImm32(100);
            out_.xorEdx();
            out_.divRcx();        // rax = hundreds, rdx = rest
            out_.movRdxToRbp(-8); // save rest before dl writes clobber rdx
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.movRaxFromRbp(-8); // rax = rest
            out_.movRcxImm32(10);
            out_.xorEdx();
            out_.divRcx();        // rax = tens, rdx = ones
            out_.movRdxToRbp(-8); // save ones
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.movRaxFromRbp(-8);
            out_.movDlFromRax();
            out_.addDl48();
            out_.storeDlRdi();
            out_.incRdi();
            out_.jmp(done);

            out_.def(done);
            // Copy the NUL-terminated buffer [rbp-224 .. rdi) into the arena.
            out_.storeByteRdiImm0();
            // rax = rdi - (rbp - 224) = length
            out_.movRaxRdi();
            out_.movR11Rbp();
            out_.subRaxR11();
            out_.addRaxImm32(224);
            out_.movRdxRax();
            out_.leaR10Rip("__nx_arena");
            out_.movRdiFromR10Disp8(8); // rdi = arena pos
            out_.movRaxRdx();
            out_.incRax();
            out_.addRaxRdi();
            out_.movR11FromR10();
            out_.addR11Imm32(kArenaSize);
            out_.cmpRaxR11();
            const std::string f64oom = out_.label("f64.oom");
            out_.jcc(J_A, f64oom);
            out_.movR10Disp8FromRax(8); // arena pos += len + 1
            out_.movRaxRdi();           // rax = old pos
            out_.leaRsiFromRbp(-224);   // src
            out_.movRcxRdx();           // rcx = len
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            out_.def(f64oom);
            out_.jmp("__nx_oom");
        }

        // __nx_print_f64: xmm0 = value. Prints the %g-style rendering + '\n'.
        void Generator::genPrintF64() {
            const std::size_t nl = internString("\n");
            out_.def("__nx_print_f64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            stubCall("__nx_str_f64"); // xmm0 -> rax = ptr, rdx = len
            out_.movRcxRax();
            stubCall("__nx_write");
            out_.leaRcxRip("str" + std::to_string(nl));
            out_.movEdxImm32(1);
            stubCall("__nx_write");
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_to_f64: rax = NUL-terminated string. Parses an optionally
        // signed decimal float with optional fraction and exponent. Returns
        // the value in xmm0 and a validity flag in edx (1 = valid).
        void Generator::genStrToF64() {
            const auto f64c = [this](double v) {
                std::string s(8, '\0');
                std::memcpy(s.data(), &v, 8);
                return "str" + std::to_string(internString(s));
            };
            const std::string ten = f64c(10.0);
            const std::string one = f64c(1.0);

            out_.def("__nx_str_to_f64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRsiRax(); // cursor
            out_.xorpdXmm0Xmm0();
            out_.xorpdXmm1Xmm1();
            out_.xorpdXmm2Xmm2();
            // [rbp-16] = number sign, [rbp-32] = exponent marker seen,
            // [rbp-28] = exponent digits seen.
            out_.xorEax();
            out_.movRbpFromRax(-16);
            out_.movRbpFromRax(-32);
            out_.movRbpFromRax(-28);
            out_.xorR9d(); // frac digit count
            out_.xorEax();
            out_.movR11Rax(); // sawDigit = 0

            const std::string lead = out_.label("f64p.lead");
            const std::string advLead = out_.label("f64p.advlead");
            const std::string signP = out_.label("f64p.sign");
            const std::string plusP = out_.label("f64p.plus");
            const std::string minusP = out_.label("f64p.minus");
            const std::string intStart = out_.label("f64p.int");
            const std::string intDig = out_.label("f64p.idig");
            const std::string intDone = out_.label("f64p.idone");
            const std::string fracDig = out_.label("f64p.fdig");
            const std::string fracDone = out_.label("f64p.fdone");
            const std::string expCheck = out_.label("f64p.expcheck");
            const std::string expMark = out_.label("f64p.expmark");
            const std::string expPlus = out_.label("f64p.expplus");
            const std::string expMinus = out_.label("f64p.expminus");
            const std::string expLoop = out_.label("f64p.exploop");
            const std::string expDig = out_.label("f64p.expdig");
            const std::string expDone = out_.label("f64p.expdone");
            const std::string noClamp = out_.label("f64p.noclamp");
            const std::string expPos = out_.label("f64p.exppos");
            const std::string applyLoop = out_.label("f64p.apply");
            const std::string applyPos = out_.label("f64p.applypos");
            const std::string applyDone = out_.label("f64p.applydone");
            const std::string trail = out_.label("f64p.trail");
            const std::string advTrail = out_.label("f64p.advtrail");
            const std::string tend = out_.label("f64p.tend");
            const std::string invalid = out_.label("f64p.invalid");
            const std::string applySign = out_.label("f64p.applysign");
            const std::string posOut = out_.label("f64p.pos");
            const std::string out = out_.label("f64p.out");

            // Skip leading spaces/tabs.
            out_.def(lead);
            out_.cmpBytePtrRsi(0x20);
            out_.jcc(J_E, advLead);
            out_.cmpBytePtrRsi(0x09);
            out_.jcc(J_NE, signP);
            out_.def(advLead);
            out_.incRsi();
            out_.jmp(lead);
            // Optional sign.
            out_.def(signP);
            out_.cmpBytePtrRsi(0x2B); // '+'
            out_.jcc(J_E, plusP);
            out_.cmpBytePtrRsi(0x2D); // '-'
            out_.jcc(J_E, minusP);
            out_.jmp(intStart);
            out_.def(plusP);
            out_.incRsi();
            out_.jmp(intStart);
            out_.def(minusP);
            out_.incRsi();
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-16);
            out_.def(intStart);

            // Integer digits: xmm0 = xmm0 * 10 + d.
            const std::string intLoop = out_.label("f64p.il");
            out_.def(intLoop);
            out_.movAlFromRsi();
            out_.subAlImm8(0x30);
            out_.cmpAlImm8(9);
            out_.jcc(J_BE, intDig);
            out_.jmp(intDone);
            out_.def(intDig);
            out_.movzxEaxAl();
            out_.mulsdXmm0Mem(ten);
            out_.cvtsi2sdXmm1Rax();
            out_.addsdXmm0Xmm1();
            out_.incRsi();
            out_.movR11Imm64(1); // sawDigit
            out_.jmp(intLoop);
            out_.def(intDone);

            // Optional fraction: xmm1 = xmm1 * 10 + d, r9 counts digits.
            out_.cmpBytePtrRsi(0x2E); // '.'
            out_.jcc(J_NE, expCheck);
            out_.incRsi();
            out_.xorpdXmm1Xmm1(); // int loop leaves the last digit in xmm1
            const std::string fracLoop = out_.label("f64p.fl");
            out_.def(fracLoop);
            out_.movAlFromRsi();
            out_.subAlImm8(0x30);
            out_.cmpAlImm8(9);
            out_.jcc(J_BE, fracDig);
            out_.jmp(fracDone);
            out_.def(fracDig);
            out_.movzxEaxAl();
            out_.mulsdXmm1Mem(ten);
            out_.movRbpFromRax(-8);
            out_.cvtsi2sdXmm2FromRbp(-8);
            out_.addsdXmm1Xmm2();
            out_.incRsi();
            out_.incR9();
            out_.movR11Imm64(1);
            out_.jmp(fracLoop);
            out_.def(fracDone);
            // xmm1 /= 10^fracCount; xmm0 += xmm1.
            out_.movsdXmm2FromRip(one);
            const std::string scaleLoop = out_.label("f64p.scale");
            const std::string scaleDone = out_.label("f64p.scaledone");
            out_.def(scaleLoop);
            out_.cmpR9Imm8(0);
            out_.jcc(J_E, scaleDone);
            out_.mulsdXmm2Mem(ten);
            out_.decR9();
            out_.jmp(scaleLoop);
            out_.def(scaleDone);
            out_.divsdXmm1Xmm2();
            out_.addsdXmm0Xmm1();

            // Optional exponent: [eE][+-]?digits, magnitude clamped to 400.
            out_.def(expCheck);
            out_.cmpBytePtrRsi(0x65); // 'e'
            out_.jcc(J_E, expMark);
            out_.cmpBytePtrRsi(0x45); // 'E'
            out_.jcc(J_NE, trail);
            out_.def(expMark);
            out_.incRsi();
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-32); // expSeen
            out_.xorEdx();           // expVal = 0
            out_.xorR10d();          // r10d = exponent negative flag
            out_.cmpBytePtrRsi(0x2B);
            out_.jcc(J_E, expPlus);
            out_.cmpBytePtrRsi(0x2D);
            out_.jcc(J_E, expMinus);
            out_.jmp(expLoop);
            out_.def(expPlus);
            out_.incRsi();
            out_.jmp(expLoop);
            out_.def(expMinus);
            out_.incRsi();
            out_.movR10Imm32(1);
            out_.jmp(expLoop);
            out_.def(expLoop);
            out_.movAlFromRsi();
            out_.subAlImm8(0x30);
            out_.cmpAlImm8(9);
            out_.jcc(J_BE, expDig);
            out_.jmp(expDone);
            out_.def(expDig);
            out_.movzxEaxAl();
            out_.movRcxImm32(10);
            out_.imulRdxRcx();
            out_.addRdxRax();
            out_.incRsi();
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-28); // expDigits seen
            out_.jmp(expLoop);
            out_.def(expDone);
            out_.movRaxImm64(400);
            out_.cmpRdxRax();
            out_.jcc(J_LE, noClamp);
            out_.movRdxRax();
            out_.def(noClamp);
            out_.testR10d();
            out_.jcc(J_E, expPos);
            out_.negRdx();
            out_.def(expPos);
            out_.movRaxRdx();
            out_.movR10Rax(); // r10 = signed exponent
            // Apply 10^exp by repeated mul/div (|exp| <= 400).
            out_.def(applyLoop);
            out_.testR10d();
            out_.jcc(J_E, applyDone);
            out_.jcc(J_NS, applyPos);
            out_.divsdXmm0Mem(ten);
            out_.incR10();
            out_.jmp(applyLoop);
            out_.def(applyPos);
            out_.mulsdXmm0Mem(ten);
            out_.decR10();
            out_.jmp(applyLoop);
            out_.def(applyDone);
            out_.jmp(trail);

            // Skip trailing spaces/tabs; next byte must be NUL.
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
            out_.testR11d();
            out_.jcc(J_E, invalid);
            // "1e" with no exponent digits is invalid.
            out_.movRaxFromRbp(-32);
            out_.testRaxRax();
            out_.jcc(J_E, applySign);
            out_.movRaxFromRbp(-28);
            out_.testRaxRax();
            out_.jcc(J_E, invalid);
            out_.def(applySign);
            out_.movRcxFromRbp(-16);
            out_.testRcxRcx();
            out_.jcc(J_E, posOut);
            out_.xorpdXmm2Xmm2();
            out_.subsdXmm2Xmm0(); // xmm2 = -xmm0
            out_.movsdXmm0Xmm2();
            out_.def(posOut);
            out_.movEdxImm32(1);
            out_.jmp(out);
            out_.def(invalid);
            out_.xorpdXmm0Xmm0();
            out_.xorEdx();
            out_.def(out);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_input_f64: rax = prompt. Prints it, reads a line and parses it
        // as f64, re-prompting on invalid input. Returns the value in xmm0.
        void Generator::genInputF64() {
            out_.def("__nx_input_f64");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // prompt
            const std::string loop = out_.label("inf64.loop");
            const std::string done = out_.label("inf64.done");
            out_.def(loop);
            out_.movRaxFromRbp(-8);
            out_.call("__nx_input");
            out_.call("__nx_str_to_f64");
            out_.testRdxRdx();
            out_.jcc(J_NE, done);
            const std::size_t msg = internString("Invalid number. Try again.\n");
            out_.leaRcxRip("str" + std::to_string(msg));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[msg].size()));
            stubCall("__nx_write_err");
            out_.jmp(loop);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_alloc: rax = size in bytes (multiple of 8). Bumps the string
        // arena and returns the raw pointer in rax for array/struct storage.
        void Generator::genAlloc() {
            const std::string oom = out_.label("alloc.oom");
            out_.def("__nx_alloc");
            out_.movRdxRax(); // rdx = size
            out_.leaR10Rip("__nx_arena");
            out_.movR11FromR10Disp8(8); // r11 = pos offset
            out_.movRaxFromR10();       // rax = arena base
            out_.addRaxR11();           // rax = base + pos = result ptr
            out_.addR11Rdx();           // r11 = pos + size = new pos
            out_.cmpR11Imm32(kArenaSize);
            out_.jcc(J_A, oom);
            out_.movR10Disp8FromR11(8); // store new pos
            out_.ret();
            out_.def(oom);
            out_.jmp("__nx_oom");
        }

        // __nx_dup: rax = source pointer, rdx = size in bytes (multiple of 8).
        // Allocates a fresh arena block and copies the contents; returns the
        // new pointer in rax. Used to give aggregate values copy semantics.
        void Generator::genDup() {
            const std::string loop = out_.label("dup.loop");
            const std::string done = out_.label("dup.done");
            out_.def("__nx_dup");
            out_.movR8Rdx();  // r8 = size
            out_.movRcxRax(); // rcx = src
            out_.movRaxRdx(); // rax = size arg
            out_.call("__nx_alloc");
            out_.movRdxRax(); // rdx = dst base
            out_.shrR8Imm8(3);
            out_.def(loop);
            out_.testR8R8();
            out_.jcc(J_E, done);
            out_.movR9FromRcx();   // r9 = [rcx]
            out_.movToRaxFromR9(); // [rax] = r9
            out_.addRaxImm8(8);
            out_.addRcxImm8(8);
            out_.decR8();
            out_.jmp(loop);
            out_.def(done);
            out_.movRaxRdx();
            out_.ret();
        }

        // __nx_oob: clean index-out-of-bounds failure (stderr + exit 1).
        void Generator::genOob() {
            const std::size_t m = internString("N++ runtime error: index out of bounds\n");
            out_.def("__nx_oob");
            out_.leaRcxRip("str" + std::to_string(m));
            out_.movRdxImm32(static_cast<std::uint32_t>(image_.strings[m].size()));
            stubCall("__nx_write_err");
            if (target_ == NativeTarget::WindowsX64) {
                out_.andRspMinus16();
                out_.movEcxImm32(1);
                out_.subRsp(32);
                out_.callIAT("__iat_ExitProcess");
                out_.ud2();
            } else {
                out_.movEaxImm32(60); // sys_exit
                out_.movEdiImm32(1);
                out_.syscall();
            }
        }

    } // namespace detail
} // namespace npp::native
