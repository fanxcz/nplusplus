#pragma once

#include "native/image.hpp"

#include <cstdint>
#include <string>

namespace npp::native {
    namespace detail {

        // Minimal x86-64 instruction encoder. Every method appends bytes to a
        // CodeImage; labels and fixups are resolved by the object writer.
        class Emitter {
        public:
            explicit Emitter(CodeImage& image) : image_(image) {}

            std::size_t pos() const {
                return image_.bytes.size();
            }
            void def(const std::string& name) {
                image_.labels[name] = pos();
            }

            void b(std::uint8_t v) {
                image_.bytes.push_back(v);
            }
            void i32(std::uint32_t v) {
                for (int i = 0; i < 4; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }
            void i64(std::uint64_t v) {
                for (int i = 0; i < 8; ++i)
                    image_.bytes.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFF));
            }

            // 32-bit relative fixup: field = symbol - (codeBase + at + 4) + addend.
            void rel(const std::string& symbol, std::int32_t addend = 0) {
                image_.fixups.push_back({pos(), symbol, addend});
                i32(0);
            }

            std::string label(const std::string& prefix) {
                return prefix + "$" + std::to_string(nextLabel_++);
            }

            void pushRbp() {
                b(0x55);
            }
            void pushRax() {
                b(0x50);
            }
            void pushRbx() {
                b(0x53);
            }
            void popRax() {
                b(0x58);
            }
            void popRcx() {
                b(0x59);
            }
            void pushRcx() {
                b(0x51);
            }
            void movRcxFromRsp() { // mov rcx, [rsp]
                b(0x48);
                b(0x8B);
                b(0x0C);
                b(0x24);
            }
            void movRcxFromRspOff(std::uint8_t d) { // mov rcx, [rsp+d]
                b(0x48);
                b(0x8B);
                b(0x4C);
                b(0x24);
                b(d);
            }
            void popRbx() {
                b(0x5B);
            }
            void popRbp() {
                b(0x5D);
            }
            void movRbxRsp() {
                b(0x48);
                b(0x89);
                b(0xE3);
            }
            void movRspRbx() {
                b(0x48);
                b(0x89);
                b(0xDC);
            }
            void ret() {
                b(0xC3);
            }
            void ud2() {
                b(0x0F);
                b(0x0B);
            }
            void repMovsb() {
                b(0xF3);
                b(0xA4);
            }
            void movRbpRsp() {
                b(0x48);
                b(0x89);
                b(0xE5);
            }
            void movRspRbp() {
                b(0x48);
                b(0x89);
                b(0xEC);
            }
            void movR11Rsp() {
                b(0x49);
                b(0x89);
                b(0xE3);
            }
            void movRspR11() {
                b(0x4C);
                b(0x89);
                b(0xDC);
            }
            void subRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xEC);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xEC);
                    i32(n);
                }
            }
            void addRsp(std::uint32_t n) {
                if (n <= 127) {
                    b(0x48);
                    b(0x83);
                    b(0xC4);
                    b(static_cast<std::uint8_t>(n));
                } else {
                    b(0x48);
                    b(0x81);
                    b(0xC4);
                    i32(n);
                }
            }
            void andRspMinus16() {
                b(0x48);
                b(0x83);
                b(0xE4);
                b(0xF0);
            }
            void movRaxImm64(std::uint64_t v) {
                b(0x48);
                b(0xB8);
                i64(v);
            }
            void movR11Imm64(std::uint64_t v) {
                b(0x49);
                b(0xBB);
                i64(v);
            }
            void movEcxImm32(std::uint32_t v) {
                b(0xB9);
                i32(v);
            }
            void movEaxImm32(std::uint32_t v) {
                b(0xB8);
                i32(v);
            }
            void movEdxImm32(std::uint32_t v) {
                b(0xBA);
                i32(v);
            }
            void movEdiEax() {
                b(0x89);
                b(0xC7);
            }
            void movEcxEax() {
                b(0x89);
                b(0xC1);
            }
            void movRcxRax() {
                b(0x48);
                b(0x89);
                b(0xC1);
            }
            void movRdxRax() {
                b(0x48);
                b(0x89);
                b(0xC2);
            }
            void movRsiRcx() {
                b(0x48);
                b(0x89);
                b(0xCE);
            }
            void movRaxRdx() {
                b(0x48);
                b(0x89);
                b(0xD0);
            }
            void movRaxRdi() {
                b(0x48);
                b(0x89);
                b(0xF8);
            }
            void movRaxRsi() {
                b(0x48);
                b(0x89);
                b(0xF0);
            }
            void movRsiRax() {
                b(0x48);
                b(0x89);
                b(0xC6);
            }
            void movRsiRdx() {
                b(0x48);
                b(0x89);
                b(0xD6);
            }
            void movRaxR8() {
                b(0x4C);
                b(0x89);
                b(0xC0);
            }
            void movRdxRsi() {
                b(0x48);
                b(0x89);
                b(0xF2);
            }
            void movRcxRdx() {
                b(0x48);
                b(0x89);
                b(0xD1);
            }
            void movRcxRsi() {
                b(0x48);
                b(0x89);
                b(0xF1);
            }
            void movRcxR8() {
                b(0x4C);
                b(0x89);
                b(0xC1);
            }
            void movRcxR9() {
                b(0x4C);
                b(0x89);
                b(0xC9);
            }
            void movRcxR11() {
                b(0x4C);
                b(0x89);
                b(0xD9);
            }
            void movRdiRsi() {
                b(0x48);
                b(0x89);
                b(0xF7);
            }
            void movRsiR11() {
                b(0x4C);
                b(0x89);
                b(0xDE);
            }
            void movEaxFromRsp8(std::uint8_t d) {
                b(0x8B);
                b(0x44);
                b(0x24);
                b(d);
            }
            void movRsiImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC6);
                i32(v);
            }
            void movRdxImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC2);
                i32(v);
            }
            void movR8Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC0);
                i32(v);
            }
            void movR9Imm32(std::uint32_t v) {
                b(0x49);
                b(0xC7);
                b(0xC1);
                i32(v);
            }
            void movR10Imm32(std::uint32_t v) {
                b(0x41);
                b(0xBA);
                i32(v);
            }
            void movRcxImm10() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(10);
            }
            void movR10Imm1() {
                b(0x41);
                b(0xBA);
                i32(1);
            }
            void xorEdx() {
                b(0x31);
                b(0xD2);
            }
            void xorEax() {
                b(0x31);
                b(0xC0);
            }
            void xorRax() {
                b(0x48);
                b(0x31);
                b(0xC0);
            }
            void xorEcx() {
                b(0x31);
                b(0xC9);
            }
            void xorEdi() {
                b(0x31);
                b(0xFF);
            }
            void xorR8d() {
                b(0x45);
                b(0x31);
                b(0xC0);
            }
            void xorR9d() {
                b(0x45);
                b(0x31);
                b(0xC9);
            }
            void xorR10d() {
                b(0x45);
                b(0x31);
                b(0xD2);
            }
            void addRaxImm32(std::uint32_t v) {
                b(0x48);
                b(0x05);
                i32(v);
            }
            void addRaxRcx() {
                b(0x48);
                b(0x01);
                b(0xC8);
            }
            void addRaxRdi() {
                b(0x48);
                b(0x01);
                b(0xF8);
            }
            void addRaxRsi() {
                b(0x48);
                b(0x01);
                b(0xF0);
            }
            void addRaxR9() {
                b(0x4C);
                b(0x01);
                b(0xC8);
            }
            void addRsiRdx() {
                b(0x48);
                b(0x01);
                b(0xD6);
            }
            void addRsiRcx() {
                b(0x48);
                b(0x01);
                b(0xCE);
            }
            void addR11Imm32(std::uint32_t v) {
                b(0x49);
                b(0x81);
                b(0xC3);
                i32(v);
            }
            void incRax() {
                b(0x48);
                b(0xFF);
                b(0xC0);
            }
            void incRcx() {
                b(0x48);
                b(0xFF);
                b(0xC1);
            }
            void incRdx() {
                b(0x48);
                b(0xFF);
                b(0xC2);
            }
            void incRsi() {
                b(0x48);
                b(0xFF);
                b(0xC6);
            }
            void incR8() {
                b(0x49);
                b(0xFF);
                b(0xC0);
            }
            void incR9() {
                b(0x49);
                b(0xFF);
                b(0xC1);
            }
            void decEcx() {
                b(0xFF);
                b(0xC9);
            }
            void decR8() {
                b(0x49);
                b(0xFF);
                b(0xC8);
            }
            void subRaxRcx() {
                b(0x48);
                b(0x29);
                b(0xC8);
            }
            void imulRaxRcx() {
                b(0x48);
                b(0x0F);
                b(0xAF);
                b(0xC1);
            }
            void cqo() {
                b(0x48);
                b(0x99);
            }
            void idivRcx() {
                b(0x48);
                b(0xF7);
                b(0xF9);
            }
            void divRcx() {
                b(0x48);
                b(0xF7);
                b(0xF1);
            }
            void negRax() {
                b(0x48);
                b(0xF7);
                b(0xD8);
            }
            void notBoolRax() {
                b(0x48);
                b(0x83);
                b(0xF0);
                b(0x01);
            }
            void andRaxRcx() {
                b(0x48);
                b(0x21);
                b(0xC8);
            }
            void orRaxRcx() {
                b(0x48);
                b(0x09);
                b(0xC8);
            }
            void xorRaxRcx() {
                b(0x48);
                b(0x31);
                b(0xC8);
            }
            void shlRaxCl() {
                b(0x48);
                b(0xD3);
                b(0xE0);
            }
            void sarRaxCl() {
                b(0x48);
                b(0xD3);
                b(0xF8);
            }
            void cmpRaxRcx() {
                b(0x48);
                b(0x39);
                b(0xC8);
            }
            void cmpRcxRax() {
                b(0x48);
                b(0x39);
                b(0xC1);
            }
            void cmpRaxR11() {
                b(0x4C);
                b(0x39);
                b(0xD8);
            }
            void cmpRaxImm8(std::uint8_t v) {
                b(0x48);
                b(0x83);
                b(0xF8);
                b(v);
            }
            void cmpRcxImm8(std::uint8_t v) {
                b(0x48);
                b(0x83);
                b(0xF9);
                b(v);
            }
            void cmpBytePtrRcx(std::uint8_t v) {
                b(0x80);
                b(0x39);
                b(v);
            }
            void cmpBytePtrRsi(std::uint8_t v) {
                b(0x80);
                b(0x3E);
                b(v);
            }
            void cmpAlDl() {
                b(0x38);
                b(0xC2);
            }
            void cmpAlImm10() {
                b(0x3C);
                b(0x0A);
            }
            void cmpAlImm13() {
                b(0x3C);
                b(0x0D);
            }
            void cmpRdxRax() {
                b(0x48);
                b(0x39);
                b(0xC2);
            }
            void testRaxRax() {
                b(0x48);
                b(0x85);
                b(0xC0);
            }
            void testRdxRdx() {
                b(0x48);
                b(0x85);
                b(0xD2);
            }
            void testRcxRcx() {
                b(0x48);
                b(0x85);
                b(0xC9);
            }
            void testEcxEcx() {
                b(0x85);
                b(0xC9);
            }
            void testAlAl() {
                b(0x84);
                b(0xC0);
            }
            void testR8R8() {
                b(0x4D);
                b(0x85);
                b(0xC0);
            }
            void testR10d() {
                b(0x45);
                b(0x85);
                b(0xD2);
            }
            void setcc(std::uint8_t cc) {
                b(0x0F);
                b(cc);
                b(0xC0);
            }
            void movzxEaxAl() {
                b(0x0F);
                b(0xB6);
                b(0xC0);
            }
            void addDl48() {
                b(0x80);
                b(0xC2);
                b(0x30);
            }
            void decRsi() {
                b(0x48);
                b(0xFF);
                b(0xCE);
            }
            void subRdxRsi() {
                b(0x48);
                b(0x29);
                b(0xF2);
            }
            void syscall() {
                b(0x0F);
                b(0x05);
            }
            void cld() {
                b(0xFC);
            }
            void repneScasb() {
                b(0xF2);
                b(0xAE);
            }
            void repeCmpsb() {
                b(0xF3);
                b(0xA6);
            }
            void notRcx() {
                b(0x48);
                b(0xF7);
                b(0xD1);
            }
            void decRcx() {
                b(0x48);
                b(0xFF);
                b(0xC9);
            }
            void movRdiRcx() {
                b(0x48);
                b(0x89);
                b(0xCF);
            }
            void movRcxImmNeg1() {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(0xFFFFFFFFu);
            }
            void storeByteRsiImm8(std::uint8_t v) {
                b(0xC6);
                b(0x06);
                b(v);
            }
            void storeByteRsiImm0() {
                b(0xC6);
                b(0x06);
                b(0x00);
            }
            void storeByteRdiImm0() {
                b(0xC6);
                b(0x07);
                b(0x00);
            }
            void storeDlRsi() {
                b(0x88);
                b(0x16);
            }
            void movAlFromRcx() {
                b(0x8A);
                b(0x01);
            }
            void movAlFromRsi() {
                b(0x8A);
                b(0x06);
            }
            void movAlFromRsiMinus1() {
                b(0x8A);
                b(0x46);
                b(0xFF);
            }
            void movDlFromRdx() {
                b(0x8A);
                b(0x12);
            }
            void movEdiImm32(std::uint32_t v) {
                b(0xBF);
                i32(v);
            }
            void leaR9FromRsp8(std::uint8_t d) {
                b(0x4C);
                b(0x8D);
                b(0x4C);
                b(0x24);
                b(d);
            }
            void storeQwordRsp32Imm0() {
                b(0x48);
                b(0xC7);
                b(0x44);
                b(0x24);
                b(0x20);
                i32(0);
            }
            void jmp(const std::string& target) {
                b(0xE9);
                rel(target);
            }
            void jcc(std::uint8_t cc, const std::string& target) {
                b(0x0F);
                b(cc);
                rel(target);
            }
            void call(const std::string& target) {
                b(0xE8);
                rel(target);
            }
            void callIAT(const std::string& importSym) {
                b(0xFF);
                b(0x15);
                rel(importSym);
            }
            void leaRcxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x0D);
                rel(s);
            }
            void leaRaxRip(const std::string& s) {
                b(0x48);
                b(0x8D);
                b(0x05);
                rel(s);
            }
            void leaR10Rip(const std::string& s) {
                b(0x4C);
                b(0x8D);
                b(0x15);
                rel(s);
            }
            // Arena word access: __nx_arena[0] = base, __nx_arena[1] = pos.
            void movR11FromR10() {
                b(0x4D);
                b(0x8B);
                b(0x1A);
            }
            void movR11FromR10Disp8(std::uint8_t d) {
                b(0x4D);
                b(0x8B);
                b(0x5A);
                b(d);
            }
            void movRdiFromR10Disp8(std::uint8_t d) {
                b(0x49);
                b(0x8B);
                b(0x7A);
                b(d);
            }
            void movR10FromRax() {
                b(0x49);
                b(0x89);
                b(0x02);
            }
            void movR10Disp8FromRax(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x42);
                b(d);
            }
            void movR10Disp8FromRsi(std::uint8_t d) {
                b(0x49);
                b(0x89);
                b(0x72);
                b(d);
            }
            void leaRdxFromRbp(std::int64_t off) {
                leaFromRbp(0x55, 0x95, off);
            }
            void leaRsiFromRbp(std::int64_t off) {
                leaFromRbp(0x75, 0xB5, off);
            }

            // rbp-relative loads/stores. Negative offsets address locals.
            void movRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x45, 0x85, off);
            }
            void subRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x2B, 0x45, 0x85, off);
            }
            void movRsiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x75, 0xB5, off);
            }
            void movRbpFromRsi(std::int64_t off) {
                memToReg(0x48, 0x89, 0x75, 0xB5, off);
            }
            void movRbpFromRax(std::int64_t off) {
                memToReg(0x48, 0x89, 0x45, 0x85, off);
            }
            void movRcxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x4D, 0x8D, off);
            }
            void movRdxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x55, 0x95, off);
            }
            void movRdiFromRbp(std::int64_t off) {
                memToReg(0x48, 0x8B, 0x7D, 0xBD, off);
            }
            void movR8FromRbp(std::int64_t off) {
                memToReg(0x4C, 0x8B, 0x45, 0x85, off);
            }
            void movRcxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x4D, 0x8D, off);
            }
            void movRdxToRbp(std::int64_t off) {
                memToReg(0x48, 0x89, 0x55, 0x95, off);
            }
            void
            movR11FromRbp(std::int64_t off) { // Positive offsets address incoming stack arguments.
                if (off >= 0) {
                    if (off <= 127) {
                        b(0x4C);
                        b(0x8B);
                        b(0x5D);
                        b(static_cast<std::uint8_t>(off));
                    } else {
                        b(0x4C);
                        b(0x8B);
                        b(0x9D);
                        i32(static_cast<std::uint32_t>(off));
                    }
                    return;
                }
                memToReg(0x4C, 0x8B, 0x5D, 0x9D, off);
            }
            void movRbpFromR11(std::int64_t off) {
                memToReg(0x4C, 0x89, 0x5D, 0x9D, off);
            }
            void movRbpFromRcx(std::int64_t off) {
                memToReg(0x48, 0x89, 0x4D, 0x8D, off);
            }
            void movRbpFromRdx(std::int64_t off) {
                memToReg(0x48, 0x89, 0x55, 0x95, off);
            }
            void cmpRaxFromRbp(std::int64_t off) {
                memToReg(0x48, 0x3B, 0x45, 0x85, off);
            }
            void movRaxFromR10() {
                b(0x49);
                b(0x8B);
                b(0x02);
            }
            void movRdiRax() {
                b(0x48);
                b(0x89);
                b(0xC7);
            }
            void movRaxRcx() {
                b(0x48);
                b(0x89);
                b(0xC8);
            }
            void movRcxImm64(std::uint64_t v) {
                b(0x48);
                b(0xB9);
                i64(v);
            }
            void movRcxImm32(std::uint32_t v) {
                b(0x48);
                b(0xC7);
                b(0xC1);
                i32(v);
            }
            void shrRcxImm8(std::uint8_t v) {
                b(0x48);
                b(0xC1);
                b(0xE9);
                b(v);
            }
            void shlRdxImm8(std::uint8_t v) {
                b(0x48);
                b(0xC1);
                b(0xE2);
                b(v);
            }
            void addRaxR11() {
                b(0x4C);
                b(0x01);
                b(0xD8);
            }
            void addRaxRdx() {
                b(0x48);
                b(0x01);
                b(0xD0);
            }
            void addRdxRax() {
                b(0x48);
                b(0x01);
                b(0xC2);
            }
            void imulRdxRcx() {
                b(0x48);
                b(0x0F);
                b(0xAF);
                b(0xD1); // imul rdx, rcx (reg=rdx, rm=rcx)
            }
            void negRdx() {
                b(0x48);
                b(0xF7);
                b(0xDA);
            }
            void subAlImm8(std::uint8_t v) {
                b(0x2C);
                b(v);
            }
            void cmpAlImm8(std::uint8_t v) {
                b(0x3C);
                b(v);
            }
            void cmpEaxImm8(std::uint8_t v) {
                b(0x83);
                b(0xF8);
                b(v);
            }
            void xorEsi() {
                b(0x31);
                b(0xF6);
            }
            void rdtsc() {
                b(0x0F);
                b(0x31);
            }
            void movR11Rax() {
                b(0x49);
                b(0x89);
                b(0xC3);
            }
            void testR10dImm32(std::uint32_t v) {
                b(0x41);
                b(0xF7);
                b(0xC2);
                i32(v);
            }
            void leaRdiFromRbp(std::int64_t off) {
                leaFromRbp(0x7D, 0xBD, off);
            }

            // ---- SSE2 / x87-free scalar double support ---------------------
            // f64 values travel in xmm0 between expressions; memory operands
            // use the same modrm layout as the integer rbp helpers.

            void sseLoadXmm0(std::uint8_t op, std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(op);
                sseMemModrm(0x45, 0x85, off);
            }
            void sseStoreXmm0(std::uint8_t op, std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(op);
                sseMemModrm(0x45, 0x85, off);
            }
            void sseLoadXmm2(std::uint8_t op, std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(op);
                sseMemModrm(0x55, 0x95, off);
            }
            void movsdXmm0FromRbp(std::int64_t off) {
                sseLoadXmm0(0x10, off);
            }
            void movsdXmm0ToRbp(std::int64_t off) {
                sseStoreXmm0(0x11, off);
            }
            void movsdXmm2FromRbp(std::int64_t off) {
                sseLoadXmm2(0x10, off);
            }
            void movsdXmm2ToRbp(std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(0x11);
                sseMemModrm(0x55, 0x95, off);
            }
            void movsdXmm0FromRsp() {
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0x04);
                b(0x24);
            }
            void movsdXmm0ToRsp() {
                b(0xF2);
                b(0x0F);
                b(0x11);
                b(0x04);
                b(0x24);
            }
            void movsdXmm0Xmm1() { // movsd xmm0, xmm1
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0xC1);
            }
            void movsdXmm1Xmm0() { // movsd xmm1, xmm0
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0xC8);
            }
            void movsdXmm2Xmm0() { // movsd xmm2, xmm0
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0xD0);
            }
            void movsdXmm0Xmm2() { // movsd xmm0, xmm2
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0xC2);
            }
            // SSE arithmetic with a RIP-relative 8-byte constant operand.
            void sseArithMem(std::uint8_t op, std::uint8_t modrm, const std::string& sym) {
                b(0xF2);
                b(0x0F);
                b(op);
                b(modrm);
                rel(sym);
            }
            void mulsdXmm0Mem(const std::string& sym) {
                sseArithMem(0x59, 0x05, sym); // mulsd xmm0, [rip+sym]
            }
            void divsdXmm0Mem(const std::string& sym) {
                sseArithMem(0x5E, 0x05, sym); // divsd xmm0, [rip+sym]
            }
            void mulsdXmm2Mem(const std::string& sym) {
                sseArithMem(0x59, 0x15, sym); // mulsd xmm2, [rip+sym]
            }
            void divsdXmm2Mem(const std::string& sym) {
                sseArithMem(0x5E, 0x15, sym); // divsd xmm2, [rip+sym]
            }
            void sseArithXmm0Xmm1(std::uint8_t op) { // F2 0F op /r, xmm0 op= xmm1
                b(0xF2);
                b(0x0F);
                b(op);
                b(0xC1);
            }
            void subsdXmm0Xmm1() { // subsd xmm0, xmm1
                b(0xF2);
                b(0x0F);
                b(0x5C);
                b(0xC1);
            }
            void subsdXmm2Xmm0() { // subsd xmm2, xmm0
                b(0xF2);
                b(0x0F);
                b(0x5C);
                b(0xD0);
            }
            void addsdXmm0Xmm1() { // addsd xmm0, xmm1
                b(0xF2);
                b(0x0F);
                b(0x58);
                b(0xC1);
            }
            void addsdXmm0Xmm2() { // addsd xmm0, xmm2
                b(0xF2);
                b(0x0F);
                b(0x58);
                b(0xC2);
            }
            void addsdXmm2Xmm0() { // addsd xmm2, xmm0
                b(0xF2);
                b(0x0F);
                b(0x58);
                b(0xD0);
            }
            void divsdXmm1Xmm2() { // divsd xmm1, xmm2
                b(0xF2);
                b(0x0F);
                b(0x5E);
                b(0xCA);
            }
            void ucomisdXmm0Xmm1() { // ucomisd xmm0, xmm1
                b(0x66);
                b(0x0F);
                b(0x2E);
                b(0xC1);
            }
            void ucomisdXmm0Xmm2() { // ucomisd xmm0, xmm2
                b(0x66);
                b(0x0F);
                b(0x2E);
                b(0xC2);
            }
            void ucomisdXmm2Mem(const std::string& sym) {
                b(0x66);
                b(0x0F);
                b(0x2E);
                b(0x15);
                rel(sym);
            }
            void cvtsi2sdXmm0Rax() { // cvtsi2sd xmm0, rax
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2A);
                b(0xC0);
            }
            void cvtsi2sdXmm1Rax() { // cvtsi2sd xmm1, rax
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2A);
                b(0xC8);
            }
            void cvtsi2sdXmm0FromRbp(std::int64_t off) {
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2A);
                sseMemModrm(0x45, 0x85, off);
            }
            void cvtsi2sdXmm2FromRbp(std::int64_t off) {
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2A);
                sseMemModrm(0x55, 0x95, off);
            }
            void cvttsd2siRaxXmm0() { // cvttsd2si rax, xmm0
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2C);
                b(0xC0);
            }
            void cvttsd2siRaxXmm2() { // cvttsd2si rax, xmm2
                b(0xF2);
                b(0x48);
                b(0x0F);
                b(0x2C);
                b(0xC2);
            }
            void movqXmm0Rax() { // movq xmm0, rax
                b(0x66);
                b(0x48);
                b(0x0F);
                b(0x6E);
                b(0xC0);
            }
            void movqRaxXmm0() { // movq rax, xmm0
                b(0x66);
                b(0x48);
                b(0x0F);
                b(0x7E);
                b(0xC0);
            }
            void xorpdXmm0Xmm0() { // xorpd xmm0, xmm0 (zero)
                b(0x66);
                b(0x0F);
                b(0x57);
                b(0xC0);
            }
            void xorpdXmm2Xmm2() {
                b(0x66);
                b(0x0F);
                b(0x57);
                b(0xD2);
            }
            void movDlFromRax() { // mov dl, al
                b(0x88);
                b(0xC2);
            }
            void incR10() {
                b(0x49);
                b(0xFF);
                b(0xC2);
            }
            void incRdi() {
                b(0x48);
                b(0xFF);
                b(0xC7);
            }
            void decR10() {
                b(0x49);
                b(0xFF);
                b(0xCA);
            }
            void decR9() {
                b(0x49);
                b(0xFF);
                b(0xC9);
            }
            void cmpR10Imm8(std::uint8_t v) {
                b(0x49);
                b(0x83);
                b(0xFA);
                b(v);
            }
            void cmpR9Imm8(std::uint8_t v) {
                b(0x49);
                b(0x83);
                b(0xF9);
                b(v);
            }
            void shrRaxImm8(std::uint8_t v) {
                b(0x48);
                b(0xC1);
                b(0xE8);
                b(v);
            }
            void andRaxImm32(std::uint32_t v) {
                b(0x48);
                b(0x25);
                i32(v);
            }
            void cmpRaxImm32(std::uint32_t v) {
                b(0x48);
                b(0x3D);
                i32(v);
            }
            void cmpRsiR11() {
                b(0x4C);
                b(0x39);
                b(0xDE);
            }
            void xorRaxR11() {
                b(0x4C);
                b(0x31);
                b(0xD8);
            }
            void andRaxR11() {
                b(0x4C);
                b(0x21);
                b(0xD8);
            }
            void testRaxR11() {
                b(0x4C);
                b(0x85);
                b(0xD8);
            }
            void movR8Rcx() { // mov r8, rcx
                b(0x49);
                b(0x89);
                b(0xC8);
            }
            void movR8R10() { // mov r8, r10
                b(0x4D);
                b(0x89);
                b(0xD0);
            }
            void movR8R9() { // mov r8, r9
                b(0x4D);
                b(0x89);
                b(0xC8);
            }
            void movR8Rax() { // mov r8, rax
                b(0x49);
                b(0x89);
                b(0xC0);
            }
            void movRaxR9() { // mov rax, r9
                b(0x4C);
                b(0x89);
                b(0xC8);
            }
            void movRaxR10() { // mov rax, r10
                b(0x4C);
                b(0x89);
                b(0xD0);
            }
            void movR10Rax() { // mov r10, rax
                b(0x49);
                b(0x89);
                b(0xC2);
            }
            void subRaxR10() { // sub rax, r10
                b(0x4C);
                b(0x29);
                b(0xD0);
            }
            void subRaxR9() { // sub rax, r9
                b(0x4C);
                b(0x29);
                b(0xC8);
            }
            void cmpR10R9() { // cmp r10, r9
                b(0x4D);
                b(0x39);
                b(0xCA);
            }
            void decRax() {
                b(0x48);
                b(0xFF);
                b(0xC8);
            }
            void addAlImm8(std::uint8_t v) {
                b(0x04);
                b(v);
            }
            void storeAlRsi() { // mov [rsi], al
                b(0x88);
                b(0x06);
            }
            void storeAlRdi() { // mov [rdi], al
                b(0x88);
                b(0x07);
            }
            void storeByteRdiImm8(std::uint8_t v) {
                b(0xC6);
                b(0x07);
                b(v);
            }
            void repStosb() {
                b(0xF3);
                b(0xAA);
            }
            void leaR11FromRbp(std::int64_t off) {
                b(0x4C);
                b(0x8D);
                if (off >= -128 && off < 0) {
                    b(0x5D);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(0x9D);
                    i32(static_cast<std::uint32_t>(off));
                }
            }
            void movR11Rbp() { // mov r11, rbp
                b(0x49);
                b(0x89);
                b(0xEB);
            }
            void subRaxR11() { // sub rax, r11
                b(0x4C);
                b(0x29);
                b(0xD8);
            }
            void storeDlRdi() { // mov [rdi], dl
                b(0x88);
                b(0x17);
            }
            void xorpdXmm1Xmm1() {
                b(0x66);
                b(0x0F);
                b(0x57);
                b(0xC9);
            }
            void addsdXmm0Xmm0() { // addsd xmm0, xmm0
                b(0xF2);
                b(0x0F);
                b(0x58);
                b(0xC0);
            }
            void addsdXmm1Xmm2() { // addsd xmm1, xmm2
                b(0xF2);
                b(0x0F);
                b(0x58);
                b(0xCA);
            }
            void mulsdXmm1Mem(const std::string& sym) { // mulsd xmm1, [rip+sym]
                b(0xF2);
                b(0x0F);
                b(0x59);
                b(0x0D);
                rel(sym);
            }
            void movsdXmm2FromRip(const std::string& sym) { // movsd xmm2, [rip+sym]
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0x15);
                rel(sym);
            }
            void testR11d() { // test r11d, r11d
                b(0x45);
                b(0x85);
                b(0xDB);
            }
            void imulR10Rcx() { // imul r10, rcx
                b(0x4C);
                b(0x0F);
                b(0xAF);
                b(0xD1);
            }
            void addR10Rax() { // add r10, rax
                b(0x49);
                b(0x01);
                b(0xC2);
            }
            void cmpR10Imm32(std::uint32_t v) {
                b(0x49);
                b(0x81);
                b(0xFA);
                i32(v);
            }

            // --- aggregate (array/struct) element access via rcx -----------
            // rcx holds a computed element/field address.
            void movRaxFromRcx() { // mov rax, [rcx]
                b(0x48);
                b(0x8B);
                b(0x01);
            }
            void movToRcxFromRax() { // mov [rcx], rax
                b(0x48);
                b(0x89);
                b(0x01);
            }
            void movsdXmm0FromRcx() { // movsd xmm0, [rcx]
                b(0xF2);
                b(0x0F);
                b(0x10);
                b(0x01);
            }
            void movsdXmm0ToRcx() { // movsd [rcx], xmm0
                b(0xF2);
                b(0x0F);
                b(0x11);
                b(0x01);
            }
            void rcxDisp8(std::uint8_t modrm, std::int64_t off) {
                b(modrm);
                b(static_cast<std::uint8_t>(256 + off));
            }
            void rcxDisp32(std::uint8_t modrm, std::int64_t off) {
                b(modrm | 0x80);
                i32(static_cast<std::uint32_t>(off));
            }
            void movRaxFromRcxOff(std::int64_t off) {
                b(0x48);
                b(0x8B);
                if (off >= -128 && off <= 127)
                    rcxDisp8(0x41, off);
                else
                    rcxDisp32(0x81, off);
            }
            void movToRcxFromRaxOff(std::int64_t off) {
                b(0x48);
                b(0x89);
                if (off >= -128 && off <= 127)
                    rcxDisp8(0x41, off);
                else
                    rcxDisp32(0x81, off);
            }
            void movsdXmm0FromRcxOff(std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(0x10);
                if (off >= -128 && off <= 127)
                    rcxDisp8(0x41, off);
                else
                    rcxDisp32(0x81, off);
            }
            void movsdXmm0ToRcxOff(std::int64_t off) {
                b(0xF2);
                b(0x0F);
                b(0x11);
                if (off >= -128 && off <= 127)
                    rcxDisp8(0x41, off);
                else
                    rcxDisp32(0x81, off);
            }
            void addRcxRdx() { // add rcx, rdx
                b(0x48);
                b(0x01);
                b(0xD1);
            }
            void addRcxImm32(std::uint32_t v) { // add rcx, imm32
                b(0x48);
                b(0x81);
                b(0xC1);
                i32(v);
            }
            // --- arena bump allocation ------------------------------------
            void addR11Rdx() { // add r11, rdx
                b(0x49);
                b(0x01);
                b(0xD3);
            }
            void cmpR11Imm32(std::uint32_t v) { // cmp r11, imm32
                b(0x49);
                b(0x81);
                b(0xFB);
                i32(v);
            }
            void movR10Disp8FromR11(std::uint8_t d) { // mov [r10+d], r11
                b(0x4D);
                b(0x89);
                b(0x5A);
                b(d);
            }
            // --- __nx_dup copy loop ----------------------------------------
            void movR8Rdx() { // mov r8, rdx
                b(0x49);
                b(0x89);
                b(0xD0);
            }
            void movR9FromRcx() { // mov r9, [rcx]
                b(0x4C);
                b(0x8B);
                b(0x09);
            }
            void movToRaxFromR9() { // mov [rax], r9
                b(0x4C);
                b(0x89);
                b(0x08);
            }
            void addRaxImm8(std::uint8_t v) { // add rax, imm8
                b(0x48);
                b(0x83);
                b(0xC0);
                b(v);
            }
            void addRcxImm8(std::uint8_t v) { // add rcx, imm8
                b(0x48);
                b(0x83);
                b(0xC1);
                b(v);
            }
            void shrR8Imm8(std::uint8_t v) { // shr r8, imm8
                b(0x49);
                b(0xC1);
                b(0xE8);
                b(v);
            }

        private:
            void memToReg(std::uint8_t rex,
                          std::uint8_t op,
                          std::uint8_t modrm8,
                          std::uint8_t modrm32,
                          std::int64_t off) {
                b(rex);
                b(op);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }
            void leaFromRbp(std::uint8_t modrm8, std::uint8_t modrm32, std::int64_t off) {
                b(0x48);
                b(0x8D);
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }
            // modrm+disp for xmm ops: [rbp+off] with the register field already
            // baked into modrm8/modrm32 (same scheme as memToReg).
            void sseMemModrm(std::uint8_t modrm8, std::uint8_t modrm32, std::int64_t off) {
                if (off >= -128 && off < 0) {
                    b(modrm8);
                    b(static_cast<std::uint8_t>(256 + off));
                } else {
                    b(modrm32);
                    i32(static_cast<std::uint32_t>(off));
                }
            }

            CodeImage& image_;
            std::size_t nextLabel_ = 0;
        };

        // Condition codes (0F-prefixed setcc/jcc second byte).
        constexpr std::uint8_t CC_E = 0x94, CC_NE = 0x95, CC_L = 0x9C, CC_LE = 0x9E;
        constexpr std::uint8_t CC_G = 0x9F, CC_GE = 0x9D;
        constexpr std::uint8_t CC_B = 0x92, CC_BE = 0x96, CC_A = 0x97, CC_AE = 0x93;
        constexpr std::uint8_t J_E = 0x84, J_NE = 0x85, J_NS = 0x89;
        constexpr std::uint8_t J_L = 0x8C, J_AE = 0x83, J_B = 0x82, J_G = 0x8F;
        constexpr std::uint8_t J_A = 0x87, J_LE = 0x8E, J_PE = 0x8A;
        constexpr std::uint8_t J_BE = 0x86, J_GE = 0x8D;
        constexpr std::uint32_t kArenaSize = 0x1000000; // 16 MiB bump arena for strings

    } // namespace detail
} // namespace npp::native
