#include "native/native_generator.hpp"

namespace npp::native {
    namespace detail {

        // String builtins mirroring src/runtime/strings/utf8.inc.
        // Argument convention: rax = arg0, rcx = arg1, rdx = arg2.
        // Strings are NUL-terminated; results are arena-allocated via __nx_alloc.

        // Emits the ASCII whitespace test on al: jumps to spLabel for
        // ' ', '\t', '\r', '\n', '\v', '\f', else to nsLabel.
        void Generator::emitIsSpaceByte(const std::string& spLabel, const std::string& nsLabel) {
            out_.cmpAlImm8(0x20);
            out_.jcc(J_E, spLabel);
            out_.cmpAlImm8(0x09);
            out_.jcc(J_E, spLabel);
            out_.cmpAlImm8(0x0D);
            out_.jcc(J_E, spLabel);
            out_.cmpAlImm8(0x0A);
            out_.jcc(J_E, spLabel);
            out_.cmpAlImm8(0x0B);
            out_.jcc(J_E, spLabel);
            out_.cmpAlImm8(0x0C);
            out_.jcc(J_E, spLabel);
            out_.jmp(nsLabel);
        }

        // __nx_u8dec: rsi = ptr, rdx = remaining length.
        // Returns rax = code point (raw byte when invalid),
        //         rcx = bytes consumed, r8 = 1 valid / 0 invalid.
        void Generator::genUtf8Dec() {
            const std::string done = out_.label("u8d.done");
            const std::string zero = out_.label("u8d.zero");
            const std::string ascii = out_.label("u8d.ascii");
            const std::string need2 = out_.label("u8d.need2");
            const std::string need3 = out_.label("u8d.need3");
            const std::string need4 = out_.label("u8d.need4");
            const std::string invalid = out_.label("u8d.invalid");
            const std::string kLoop = out_.label("u8d.kloop");
            const std::string checks = out_.label("u8d.checks");
            const std::string chk2 = out_.label("u8d.chk2");
            const std::string chk3 = out_.label("u8d.chk3");
            const std::string chk4 = out_.label("u8d.chk4");
            const std::string range = out_.label("u8d.range");
            const std::string surrLo = out_.label("u8d.surrlo");
            const std::string valid = out_.label("u8d.valid");
            // Slots: -8 first, -16 need, -24 val, -32 k, -40 byte
            out_.def("__nx_u8dec");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.testRdxRdx();
            out_.jcc(J_LE, zero);
            out_.movAlFromRsi();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-8); // first byte
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0x80);
            out_.jcc(J_B, ascii);
            // classify lead byte
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0xE0);
            out_.cmpRaxImm32(0xC0);
            out_.jcc(J_E, need2);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0xF0);
            out_.cmpRaxImm32(0xE0);
            out_.jcc(J_E, need3);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0xF8);
            out_.cmpRaxImm32(0xF0);
            out_.jcc(J_E, need4);
            out_.jmp(invalid);
            // need = 2, val = first & 0x1F
            out_.def(need2);
            out_.movRaxImm64(2);
            out_.movRbpFromRax(-16);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x1F);
            out_.movRbpFromRax(-24);
            out_.jmp(checks);
            out_.def(need3);
            out_.movRaxImm64(3);
            out_.movRbpFromRax(-16);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x0F);
            out_.movRbpFromRax(-24);
            out_.jmp(checks);
            out_.def(need4);
            out_.movRaxImm64(4);
            out_.movRbpFromRax(-16);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x07);
            out_.movRbpFromRax(-24);
            out_.def(checks);
            // need remaining >= need? (rdx >= [rbp-16])
            out_.movRaxFromRbp(-16);
            out_.movRcxRdx();
            out_.cmpRcxRax(); // rcx(len) - rax(need)
            out_.jcc(J_B, invalid);
            // k = 1 (continuation bytes start after the lead byte)
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-32);
            out_.def(kLoop);
            out_.movRaxFromRbp(-32);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_AE, range);
            // b = [rsi + k]
            out_.movRcxRsi();
            out_.movRdxFromRbp(-32);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-40);
            // (b & 0xC0) == 0x80 ?
            out_.movRaxFromRbp(-40);
            out_.andRaxImm32(0xC0);
            out_.cmpRaxImm32(0x80);
            out_.jcc(J_NE, invalid);
            // val = (val << 6) | (b & 0x3F)
            out_.movRaxFromRbp(-40);
            out_.andRaxImm32(0x3F);
            out_.movRcxRax();
            out_.movRaxFromRbp(-24);
            out_.shlRaxImm8(6);
            out_.orRaxRcx();
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-32);
            out_.incRax();
            out_.movRbpFromRax(-32);
            out_.jmp(kLoop);
            out_.def(range);
            // overlong checks
            out_.movRaxFromRbp(-16);
            out_.cmpRaxImm32(2);
            out_.jcc(J_E, chk2);
            out_.movRaxFromRbp(-16);
            out_.cmpRaxImm32(3);
            out_.jcc(J_E, chk3);
            out_.movRaxFromRbp(-16);
            out_.cmpRaxImm32(4);
            out_.jcc(J_E, chk4);
            out_.jmp(surrLo);
            out_.def(chk2);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0x80);
            out_.jcc(J_B, invalid);
            out_.jmp(surrLo);
            out_.def(chk3);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0x800);
            out_.jcc(J_B, invalid);
            out_.jmp(surrLo);
            out_.def(chk4);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0x10000);
            out_.jcc(J_B, invalid);
            out_.def(surrLo);
            // val > 0x10FFFF -> invalid
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0x110000);
            out_.jcc(J_AE, invalid);
            // surrogate 0xD800..0xDFFF -> invalid
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0xD800);
            out_.jcc(J_B, valid);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxImm32(0xE000);
            out_.jcc(J_B, invalid);
            out_.def(valid);
            out_.movRaxFromRbp(-24);
            out_.movRcxFromRbp(-16);
            out_.movR8Imm32(1);
            out_.jmp(done);
            out_.def(invalid);
            out_.movRaxFromRbp(-8);
            out_.movRcxImm64(1);
            out_.xorR8d();
            out_.jmp(done);
            out_.def(ascii);
            out_.movRaxFromRbp(-8);
            out_.movRcxImm64(1);
            out_.movR8Imm32(1);
            out_.jmp(done);
            out_.def(zero);
            out_.xorEax();
            out_.xorEcx();
            out_.xorR8d();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_chars: rax = s -> number of UTF-8 code points in rax.
        void Generator::genStrChars() {
            const std::string loop = out_.label("chars.loop");
            const std::string done = out_.label("chars.done");
            const std::string empty = out_.label("chars.empty");
            out_.def("__nx_str_chars");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // s
            // len -> [rbp-16]
            out_.movRcxRax();
            out_.xorEdx();
            const std::string ll = out_.label("chars.ll");
            const std::string ld = out_.label("chars.ld");
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRdx();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRdxToRbp(-16);
            out_.xorEax();
            out_.movRbpFromRax(-24); // pos = 0
            out_.xorEax();
            out_.movRbpFromRax(-32); // count = 0
            out_.def(loop);
            // pos < len ?
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_AE, done);
            // rsi = s + pos ; rdx = len - pos
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-24);
            out_.addRcxRdx();
            out_.movRsiRcx();
            out_.movRaxFromRbp(-16);
            out_.subRaxFromRbp(-24);
            out_.movRdxRax();
            out_.call("__nx_u8dec");
            // pos += rcx ; count++
            out_.movRaxFromRbp(-24);
            out_.addRaxRcx();
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-32);
            out_.incRax();
            out_.movRbpFromRax(-32);
            out_.jmp(loop);
            out_.def(done);
            out_.movRaxFromRbp(-32);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_char_at: rax = s, rcx = char index -> code point or -1.
        void Generator::genStrCharAt() {
            const std::string loop = out_.label("cat.loop");
            const std::string done = out_.label("cat.done");
            const std::string neg = out_.label("cat.neg");
            const std::string oob = out_.label("cat.oob");
            const std::string notMatch = out_.label("cat.notmatch");
            out_.def("__nx_str_char_at");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // s
            out_.movRcxToRbp(-16);  // index
            // index < 0 -> -1
            out_.movRaxFromRbp(-16);
            out_.testRaxRax();
            out_.jcc(J_S, neg);
            // len -> [rbp-24]
            out_.movRcxRax(); // careful: rax=index now! reload s
            out_.movRcxFromRbp(-8);
            out_.xorEdx();
            const std::string ll = out_.label("cat.ll");
            const std::string ld = out_.label("cat.ld");
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRdx();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRdxToRbp(-24);
            out_.xorEax();
            out_.movRbpFromRax(-32); // pos = 0
            out_.xorEax();
            out_.movRbpFromRax(-40); // charIndex = 0
            out_.def(loop);
            out_.movRaxFromRbp(-32);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_AE, oob);
            // rsi = s+pos ; rdx = len-pos
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-32);
            out_.addRcxRdx();
            out_.movRsiRcx();
            out_.movRaxFromRbp(-24);
            out_.subRaxFromRbp(-32);
            out_.movRdxRax();
            out_.call("__nx_u8dec");
            out_.movRbpFromRax(-48); // cp
            // charIndex == index ? return cp
            out_.movRaxFromRbp(-40);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_NE, notMatch);
            out_.movRaxFromRbp(-48);
            out_.jmp(done);
            out_.def(notMatch);
            // pos += consumed ; charIndex++
            out_.movRaxFromRbp(-32);
            out_.addRaxRcx();
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-40);
            out_.incRax();
            out_.movRbpFromRax(-40);
            out_.jmp(loop);
            out_.def(oob);
            out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL); // -1
            out_.jmp(done);
            out_.def(neg);
            out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL);
            out_.jmp(done);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_valid_utf8: rax = s -> 1 when fully valid UTF-8.
        void Generator::genStrValidUtf8() {
            const std::string loop = out_.label("utf8.loop");
            const std::string done = out_.label("utf8.done");
            const std::string bad = out_.label("utf8.bad");
            const std::string good = out_.label("utf8.good");
            const std::string ll = out_.label("utf8.ll");
            const std::string ld = out_.label("utf8.ld");
            out_.def("__nx_str_valid_utf8");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8); // s
            // len -> [rbp-16]
            out_.movRcxRax();
            out_.xorEdx();
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRdx();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRdxToRbp(-16);
            out_.xorEax();
            out_.movRbpFromRax(-24); // pos
            out_.def(loop);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_AE, good);
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-24);
            out_.addRcxRdx();
            out_.movRsiRcx();
            out_.movRaxFromRbp(-16);
            out_.subRaxFromRbp(-24);
            out_.movRdxRax();
            out_.call("__nx_u8dec");
            // r8 == 0 -> invalid
            out_.testR8R8();
            out_.jcc(J_E, bad);
            out_.movRaxFromRbp(-24);
            out_.addRaxRcx();
            out_.movRbpFromRax(-24);
            out_.jmp(loop);
            out_.def(bad);
            out_.xorEax();
            out_.jmp(done);
            out_.def(good);
            out_.movRaxImm64(1);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_from_char: rax = code point -> arena UTF-8 string.
        // __nx_str_from_char: rax = code point -> arena UTF-8 string.
        // Invalid values (negative, > U+10FFFF, surrogates) become U+FFFD.
        void Generator::genStrFromChar() {
            const std::string done = out_.label("fc.done");
            const std::string fix = out_.label("fc.fix");
            const std::string n1 = out_.label("fc.n1");
            const std::string n2 = out_.label("fc.n2");
            const std::string n3 = out_.label("fc.n3");
            const std::string n4 = out_.label("fc.n4");
            const std::string cl = out_.label("fc.cl");
            const std::string cn = out_.label("fc.cn");
            const std::string sizeSel = out_.label("fc.sizesel");
            const std::string copyOut = out_.label("fc.copyout");
            // Slots: -8 code, -16 n, -24..-21 encoded bytes, -32 dst, -40 i
            out_.def("__nx_str_from_char");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            // validate
            out_.movRaxFromRbp(-8);
            out_.testRaxRax();
            out_.jcc(J_S, fix);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0x110000);
            out_.jcc(J_AE, fix);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0xD800);
            out_.jcc(J_B, sizeSel);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0xE000);
            out_.jcc(J_B, fix);
            out_.jmp(sizeSel);
            out_.def(fix);
            out_.movRaxImm64(0xFFFD);
            out_.movRbpFromRax(-8);
            // ---- pick length ----
            out_.def(sizeSel);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0x80);
            out_.jcc(J_B, n1);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0x800);
            out_.jcc(J_B, n2);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(0x10000);
            out_.jcc(J_B, n3);
            out_.jmp(n4);
            // ---- encode ----
            out_.def(n1);
            out_.movRaxFromRbp(-8);
            out_.movBytePtrRbpFromAl(-24);
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-16);
            out_.jmp(copyOut);
            out_.def(n2);
            // b0 = 0xC0 | (cp >> 6); b1 = 0x80 | (cp & 0x3F)
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(6);
            out_.movRcxImm64(0xC0);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-24);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-23);
            out_.movRaxImm64(2);
            out_.movRbpFromRax(-16);
            out_.jmp(copyOut);
            out_.def(n3);
            // b0 = 0xE0|(cp>>12); b1 = 0x80|((cp>>6)&0x3F); b2 = 0x80|(cp&0x3F)
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(12);
            out_.movRcxImm64(0xE0);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-24);
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(6);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-23);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-22);
            out_.movRaxImm64(3);
            out_.movRbpFromRax(-16);
            out_.jmp(copyOut);
            out_.def(n4);
            // b0 = 0xF0|(cp>>18); b1 = 0x80|((cp>>12)&0x3F);
            // b2 = 0x80|((cp>>6)&0x3F);  b3 = 0x80|(cp&0x3F)
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(18);
            out_.movRcxImm64(0xF0);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-24);
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(12);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-23);
            out_.movRaxFromRbp(-8);
            out_.movRcxRax();
            out_.shrRaxImm8(6);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-22);
            out_.movRaxFromRbp(-8);
            out_.andRaxImm32(0x3F);
            out_.movRcxImm64(0x80);
            out_.orRaxRcx();
            out_.movzxEaxAl();
            out_.movBytePtrRbpFromAl(-21);
            out_.movRaxImm64(4);
            out_.movRbpFromRax(-16);
            // ---- copy n bytes to fresh arena string ----
            out_.def(copyOut);
            out_.movRaxFromRbp(-16);
            out_.addRaxImm32(1);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-32);
            out_.xorEax();
            out_.movRbpFromRax(-40);
            out_.def(cl);
            out_.movRaxFromRbp(-40);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_AE, cn);
            out_.leaRcxFromRbp(-24);
            out_.movRdxFromRbp(-40);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movBytePtrRdiFromAl();
            out_.incRdi();
            out_.movRaxFromRbp(-40);
            out_.incRax();
            out_.movRbpFromRax(-40);
            out_.jmp(cl);
            out_.def(cn);
            out_.storeByteRdiImm0();
            out_.movRaxFromRbp(-32);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_trim: rax = s -> arena string without ASCII whitespace ends.
        void Generator::genStrTrim() {
            const std::string done = out_.label("trim.done");
            const std::string ll = out_.label("trim.ll");
            const std::string ld = out_.label("trim.ld");
            const std::string bLoop = out_.label("trim.bloop");
            const std::string bDone = out_.label("trim.bdone");
            const std::string eLoop = out_.label("trim.eloop");
            const std::string eDone = out_.label("trim.edone");
            // Slots: -8 s, -16 len, -24 b, -32 e, -40 dst
            out_.def("__nx_str_trim");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            // len -> [rbp-16]
            out_.movRcxRax();
            out_.xorEdx();
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRdx();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRdxToRbp(-16);
            out_.xorEax();
            out_.movRbpFromRax(-24); // b = 0
            out_.movRaxFromRbp(-16);
            out_.movRbpFromRax(-32); // e = len
            // leading whitespace
            out_.def(bLoop);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, bDone);
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-24);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            const std::string bsp = out_.label("trim.bsp");
            const std::string bns = out_.label("trim.bns");
            emitIsSpaceByte(bsp, bns);
            out_.def(bsp);
            out_.movRaxFromRbp(-24);
            out_.incRax();
            out_.movRbpFromRax(-24);
            out_.jmp(bLoop);
            out_.def(bns);
            out_.jmp(bDone);
            out_.def(bDone);
            // trailing whitespace
            out_.def(eLoop);
            out_.movRaxFromRbp(-32);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_BE, eDone);
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-32);
            out_.decRdx();
            out_.addRcxRdx();
            out_.movAlFromRcx();
            const std::string esp = out_.label("trim.esp");
            const std::string ens = out_.label("trim.ens");
            emitIsSpaceByte(esp, ens);
            out_.def(esp);
            out_.movRaxFromRbp(-32);
            out_.decRax();
            out_.movRbpFromRax(-32);
            out_.jmp(eLoop);
            out_.def(ens);
            out_.jmp(eDone);
            out_.def(eDone);
            // n = e - b ; alloc n+1 ; copy s[b..e)
            out_.movRaxFromRbp(-32);
            out_.subRaxFromRbp(-24);
            out_.addRaxImm32(1);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-40);
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-24);
            out_.addRcxRdx();
            out_.movRsiRcx();
            out_.movRcxFromRbp(-32);
            out_.subRcxFromRbp(-24);
            out_.cld();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRaxFromRbp(-40);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_upper / __nx_str_lower: rax = s -> arena string, ASCII case.
        void Generator::genStrCase(bool upper) {
            const std::string ll = out_.label("case.ll");
            const std::string ld = out_.label("case.ld");
            const std::string cl = out_.label("case.cl");
            const std::string cd = out_.label("case.cd");
            const std::string done = out_.label("case.done");
            const std::string conv = out_.label("case.conv");
            const std::string skip = out_.label("case.skip");
            const std::string store = out_.label("case.store");
            // Slots: -8 s, -16 len, -24 dst
            out_.def(upper ? "__nx_str_upper" : "__nx_str_lower");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            // len -> [rbp-16]
            out_.movRcxRax();
            out_.xorEdx();
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRdx();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRdxToRbp(-16);
            out_.movRaxFromRbp(-16);
            out_.addRaxImm32(1);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-24);
            out_.movRsiFromRbp(-8);
            out_.movRdxFromRbp(-16);
            out_.def(cl);
            out_.testRdxRdx();
            out_.jcc(J_LE, cd);
            out_.movAlFromRsi();
            if (upper) {
                out_.cmpAlImm8(0x61);
                out_.jcc(J_B, skip);
                out_.cmpAlImm8(0x7B);
                out_.jcc(J_AE, skip);
            } else {
                out_.cmpAlImm8(0x41);
                out_.jcc(J_B, skip);
                out_.cmpAlImm8(0x5B);
                out_.jcc(J_AE, skip);
            }
            out_.def(conv);
            if (upper)
                out_.subAlImm8(0x20);
            else
                out_.addAlImm8(0x20);
            out_.jmp(store);
            out_.def(skip);
            out_.movAlFromRsi();
            out_.def(store);
            out_.movBytePtrRdiFromAl();
            out_.incRsi();
            out_.incRdi();
            out_.decRdx();
            out_.jmp(cl);
            out_.def(cd);
            out_.storeByteRdiImm0();
            out_.movRaxFromRbp(-24);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        void Generator::genStrUpper() {
            genStrCase(true);
        }
        void Generator::genStrLower() {
            genStrCase(false);
        }

        // __nx_str_find: rax = s, rcx = sub -> byte index or -1.
        // __nx_str_contains: same search, returns 0/1.
        void Generator::genStrFindCore(bool contains) {
            const std::string done = out_.label("find.done");
            const std::string nomatch = out_.label("find.nomatch");
            const std::string outer = out_.label("find.outer");
            const std::string inner = out_.label("find.inner");
            const std::string imatch = out_.label("find.imatch");
            const std::string ibad = out_.label("find.ibad");
            const std::string empty = out_.label("find.empty");
            // Slots: -8 s, -16 sub, -24 slen, -32 sublen, -40 i, -48 j,
            //        -56 maxI, -64 b1, -72 b2
            out_.def(contains ? "__nx_str_contains" : "__nx_str_find");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            // sublen -> [rbp-32]
            out_.movRcxFromRbp(-16);
            out_.xorEax();
            const std::string sll = out_.label("find.sll");
            const std::string sld = out_.label("find.sld");
            out_.def(sll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, sld);
            out_.incRcx();
            out_.incRax();
            out_.jmp(sll);
            out_.def(sld);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-32);
            out_.testRaxRax();
            out_.jcc(J_NE, empty);
            // empty sub: contains -> 1, find -> 0
            if (contains) {
                out_.movRaxImm64(1);
            } else {
                out_.xorEax();
            }
            out_.jmp(done);
            out_.def(empty);
            // slen -> [rbp-24]
            out_.movRcxFromRbp(-8);
            out_.xorEax();
            const std::string l1 = out_.label("find.l1");
            const std::string l2 = out_.label("find.l2");
            out_.def(l1);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, l2);
            out_.incRcx();
            out_.incRax();
            out_.jmp(l1);
            out_.def(l2);
            out_.movRbpFromRax(-24);
            out_.xorEax();
            out_.movRbpFromRax(-40); // i = 0
            // maxI = slen - sublen (unsigned wrap makes the bound safe)
            out_.movRaxFromRbp(-24);
            out_.subRaxFromRbp(-32);
            out_.movRbpFromRax(-56);
            out_.def(outer);
            out_.movRaxFromRbp(-40);
            out_.cmpRaxFromRbp(-56);
            out_.jcc(J_A, nomatch);
            out_.xorEax();
            out_.movRbpFromRax(-48); // j = 0
            out_.def(inner);
            out_.movRaxFromRbp(-48);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, imatch);
            // b1 = s[i+j], b2 = sub[j]
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-40);
            out_.addRcxRdx();
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-64);
            out_.movRcxFromRbp(-16);
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-72);
            out_.movRaxFromRbp(-64);
            out_.cmpRaxFromRbp(-72);
            out_.jcc(J_NE, ibad);
            out_.movRaxFromRbp(-48);
            out_.incRax();
            out_.movRbpFromRax(-48);
            out_.jmp(inner);
            out_.def(imatch);
            if (contains) {
                out_.movRaxImm64(1);
            } else {
                out_.movRaxFromRbp(-40);
            }
            out_.jmp(done);
            out_.def(ibad);
            out_.movRaxFromRbp(-40);
            out_.incRax();
            out_.movRbpFromRax(-40);
            out_.jmp(outer);
            out_.def(nomatch);
            if (contains) {
                out_.xorEax();
            } else {
                out_.movRaxImm64(0xFFFFFFFFFFFFFFFFULL);
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        void Generator::genStrFind() {
            genStrFindCore(false);
        }
        void Generator::genStrContains() {
            genStrFindCore(true);
        }

        // __nx_str_slice: rax = s, rcx = start, rdx = end -> arena substring.
        void Generator::genStrSlice() {
            const std::string done = out_.label("slice.done");
            const std::string empty = out_.label("slice.empty");
            const std::string ll = out_.label("slice.ll");
            const std::string ld = out_.label("slice.ld");
            const std::string sneg = out_.label("slice.sneg");
            const std::string sok = out_.label("slice.sok");
            const std::string eok = out_.label("slice.eok");
            // Slots: -8 s, -16 start, -24 end, -32 len, -48 dst
            out_.def("__nx_str_slice");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            // len -> [rbp-32]
            out_.movRcxFromRbp(-8);
            out_.xorEax();
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRax();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRbpFromRax(-32);
            // start < 0 -> 0
            out_.movRaxFromRbp(-16);
            out_.testRaxRax();
            out_.jcc(J_S, sneg);
            out_.jmp(sok);
            out_.def(sneg);
            out_.xorEax();
            out_.movRbpFromRax(-16);
            out_.def(sok);
            // start > len -> empty (end <= len < start)
            out_.movRaxFromRbp(-16);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_A, empty);
            // end > len -> len
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_BE, eok);
            out_.movRaxFromRbp(-32);
            out_.movRbpFromRax(-24);
            out_.def(eok);
            // start >= end -> empty
            out_.movRaxFromRbp(-16);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_AE, empty);
            // n = end - start ; alloc n+1 ; copy s[start..end)
            out_.movRaxFromRbp(-24);
            out_.subRaxFromRbp(-16);
            out_.addRaxImm32(1);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-48);
            out_.movRcxFromRbp(-8);
            out_.movRdxFromRbp(-16);
            out_.addRcxRdx();
            out_.movRsiRcx();
            out_.movRcxFromRbp(-24);
            out_.subRcxFromRbp(-16);
            out_.cld();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRaxFromRbp(-48);
            out_.jmp(done);
            out_.def(empty);
            const std::size_t es = internString("");
            out_.leaRaxRip("str" + std::to_string(es));
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_split_count: rax = s, rcx = sep -> field count (>=1).
        void Generator::genStrSplitCount() {
            const std::string done = out_.label("sc.done");
            const std::string one = out_.label("sc.one");
            const std::string loop = out_.label("sc.loop");
            const std::string nomore = out_.label("sc.nomore");
            const std::string mj = out_.label("sc.mj");
            const std::string mok = out_.label("sc.mok");
            const std::string mbad = out_.label("sc.mbad");
            // Slots: -8 s, -16 sep, -24 slen, -40 count, -48 j, -56 b1, -64 b2
            out_.def("__nx_str_split_count");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            // sep empty -> 1
            out_.movRcxFromRbp(-16);
            out_.movAlFromRcx();
            out_.testAlAl();
            out_.jcc(J_E, one);
            // slen = strlen(sep) -> [rbp-24]
            out_.movRcxFromRbp(-16);
            out_.xorEax();
            const std::string ll = out_.label("sc.ll");
            const std::string ld = out_.label("sc.ld");
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRax();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRbpFromRax(-24);
            out_.movRaxImm64(1);
            out_.movRbpFromRax(-40); // count = 1
            out_.movRsiFromRbp(-8);  // p = s
            out_.def(loop);
            // try to match sep at rsi
            out_.xorEax();
            out_.movRbpFromRax(-48); // j = 0
            out_.def(mj);
            out_.movRaxFromRbp(-48);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_AE, mok);
            out_.movRcxRsi();
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-56);
            out_.movRcxFromRbp(-16);
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-64);
            out_.movRaxFromRbp(-56);
            out_.cmpRaxFromRbp(-64);
            out_.jcc(J_NE, mbad);
            out_.movRaxFromRbp(-48);
            out_.incRax();
            out_.movRbpFromRax(-48);
            out_.jmp(mj);
            out_.def(mok);
            // match: count++, p += slen, rescan
            out_.movRaxFromRbp(-40);
            out_.incRax();
            out_.movRbpFromRax(-40);
            out_.movRaxFromRbp(-24);
            out_.addRaxRsi();
            out_.movRsiRax();
            out_.jmp(loop);
            out_.def(mbad);
            // no match at rsi: if *rsi == 0 -> done, else rsi++ rescan
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, nomore);
            out_.incRsi();
            out_.xorEax();
            out_.movRbpFromRax(-48);
            out_.jmp(mj);
            out_.def(nomore);
            out_.movRaxFromRbp(-40);
            out_.jmp(done);
            out_.def(one);
            out_.movRaxImm64(1);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_str_split_at: rax = s, rcx = sep, rdx = index -> field `index`.
        void Generator::genStrSplitAt() {
            const std::string done = out_.label("sa.done");
            const std::string empty = out_.label("sa.empty");
            const std::string ll = out_.label("sa.ll");
            const std::string ld = out_.label("sa.ld");
            const std::string skip = out_.label("sa.skip");
            const std::string skipj = out_.label("sa.skipj");
            const std::string sm = out_.label("sa.sm");
            const std::string sb = out_.label("sa.sb");
            const std::string fld = out_.label("sa.fld");
            const std::string fj = out_.label("sa.fj");
            const std::string fend = out_.label("sa.fend");
            const std::string fm = out_.label("sa.fm");
            const std::string fb = out_.label("sa.fb");
            // Slots: -8 s, -16 sep, -24 index, -32 slen, -40 i, -48 j, -56 k,
            //        -64 b1, -72 b2, -80 dst
            out_.def("__nx_str_split_at");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            // sep empty or index < 0 -> ""
            out_.movRcxFromRbp(-16);
            out_.movAlFromRcx();
            out_.testAlAl();
            out_.jcc(J_E, empty);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, empty);
            // slen = strlen(sep) -> [rbp-32]
            out_.movRcxFromRbp(-16);
            out_.xorEax();
            out_.def(ll);
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, ld);
            out_.incRcx();
            out_.incRax();
            out_.jmp(ll);
            out_.def(ld);
            out_.movRbpFromRax(-32);
            out_.movRsiFromRbp(-8); // p = s
            out_.xorEax();
            out_.movRbpFromRax(-40); // i = 0
            // ---- skip `index` separators ----
            out_.def(skip);
            out_.movRaxFromRbp(-40);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_AE, fld);
            out_.xorEax();
            out_.movRbpFromRax(-48); // j = 0
            out_.def(skipj);
            out_.movRaxFromRbp(-48);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, sm);
            out_.movRcxRsi();
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-56);
            out_.movRcxFromRbp(-16);
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-64);
            out_.movRaxFromRbp(-56);
            out_.cmpRaxFromRbp(-64);
            out_.jcc(J_NE, sb);
            out_.movRaxFromRbp(-48);
            out_.incRax();
            out_.movRbpFromRax(-48);
            out_.jmp(skipj);
            out_.def(sm);
            out_.movRaxFromRbp(-32);
            out_.addRaxRsi();
            out_.movRsiRax();
            out_.movRaxFromRbp(-40);
            out_.incRax();
            out_.movRbpFromRax(-40);
            out_.jmp(skip);
            out_.def(sb);
            out_.movAlFromRsi();
            out_.testAlAl();
            out_.jcc(J_E, empty);
            out_.incRsi();
            out_.xorEax();
            out_.movRbpFromRax(-48);
            out_.jmp(skipj);
            // ---- field = [rsi, next sep or NUL) ----
            out_.def(fld);
            out_.xorEax();
            out_.movRbpFromRax(-48); // j = 0
            out_.def(fj);
            // NUL check first: [rsi+j] == 0 -> end
            out_.movRcxRsi();
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.cmpBytePtrRcx(0);
            out_.jcc(J_E, fend);
            // sep match check at rsi+j (k loop)
            out_.xorEax();
            out_.movRbpFromRax(-56); // k = 0
            out_.def(fm);
            out_.movRaxFromRbp(-56);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, fend); // full match -> field ends at j
            out_.movRcxRsi();
            out_.movRdxFromRbp(-48);
            out_.addRcxRdx();
            out_.movRdxFromRbp(-56);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-64);
            out_.movRcxFromRbp(-16);
            out_.movRdxFromRbp(-56);
            out_.addRcxRdx();
            out_.movAlFromRcx();
            out_.movzxEaxAl();
            out_.movRbpFromRax(-72);
            out_.movRaxFromRbp(-64);
            out_.cmpRaxFromRbp(-72);
            out_.jcc(J_NE, fb); // mismatch -> advance j
            out_.movRaxFromRbp(-56);
            out_.incRax();
            out_.movRbpFromRax(-56);
            out_.jmp(fm);
            out_.def(fb);
            out_.movRaxFromRbp(-48);
            out_.incRax();
            out_.movRbpFromRax(-48);
            out_.jmp(fj);
            out_.def(fend);
            // len = j ; alloc len+1 ; copy rsi[0..len)
            out_.movRaxFromRbp(-48);
            out_.addRaxImm32(1);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-80);
            out_.movRcxRsi();
            out_.movRsiRcx();
            out_.movRcxFromRbp(-48);
            out_.cld();
            out_.repMovsb();
            out_.storeByteRdiImm0();
            out_.movRaxFromRbp(-80);
            out_.jmp(done);
            out_.def(empty);
            const std::size_t es = internString("");
            out_.leaRaxRip("str" + std::to_string(es));
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

    } // namespace detail
} // namespace npp::native
