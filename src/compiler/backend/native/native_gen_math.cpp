#include "native/native_generator.hpp"

#include <cstring>

namespace npp::native {
    namespace detail {

        // f64 math builtins. Unary helpers take the argument in xmm0 and
        // return the result in xmm0 (same convention as __nx_str_f64).
        // Binary helpers (atan2, pow) receive arguments as raw f64 bits on
        // the stack, exactly like user-function parameters: [rbp+24] is
        // arg0 and [rbp+16] is arg1.
        //
        // Transcendentals use the x87 FPU (fsin/fcos/fptan/fpatan/fyl2x/
        // f2xm1), sqrt uses SSE2 sqrtsd, and floor/ceil use the
        // cvttsd2si truncation trick.

        void Generator::genMathUnaryX87(const char* name, int op) {
            out_.def(name);
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movsdXmm0ToRbp(-8);
            out_.fldRbp(-8);
            if (op == 0)
                out_.fsin();
            else if (op == 1)
                out_.fcos();
            else {
                // fptan leaves 1.0 on top of the tangent.
                out_.fptan();
                out_.fstpSt0();
            }
            out_.fstpRbp(-8);
            out_.movsdXmm0FromRbp(-8);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        void Generator::genMathSqrt() {
            out_.def("__nx_math_sqrt");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.sqrtsdXmm0Xmm0();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // floor/ceil share one skeleton: t = (f64)(i64)x; when the
        // truncated value overshoots the true value in the rounding
        // direction, step by 1.0.
        void Generator::genMathRound(bool isFloor) {
            const auto f64c = [this](double v) {
                std::string s(8, '\0');
                std::memcpy(s.data(), &v, 8);
                return "str" + std::to_string(internString(s));
            };
            const std::string one = f64c(1.0);

            out_.def(isFloor ? "__nx_math_floor" : "__nx_math_ceil");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();

            const std::string done = out_.label("round.done");
            const std::string adjust = out_.label("round.adjust");

            out_.movsdXmm2Xmm0();    // xmm2 = x
            out_.cvttsd2siRaxXmm0(); // rax = trunc(x)
            out_.cvtsi2sdXmm0Rax();  // xmm0 = (f64)trunc(x)
            out_.ucomisdXmm0Xmm2();  // trunc vs x
            // floor adjusts when trunc > x; ceil when trunc < x.
            out_.jcc(isFloor ? J_A : J_B, adjust);
            out_.jmp(done);
            out_.def(adjust);
            out_.sseArithMem(isFloor ? 0x5C : 0x58, 0x05, one); // subsd/addsd xmm0,[rip+one]
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_math_atan2: [rbp+24] = y (arg0), [rbp+16] = x (arg1).
        // fpatan computes atan2(ST1, ST0); arrange ST1=y, ST0=x.
        void Generator::genMathAtan2() {
            out_.def("__nx_math_atan2");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.fldRbp(24); // y -> ST0
            out_.fldRbp(16); // x -> ST0, y -> ST1
            out_.fpatan();   // ST0 = atan2(y, x)
            out_.fstpRbp(-8);
            out_.movsdXmm0FromRbp(-8);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_math_pow: [rbp+24] = base (arg0), [rbp+16] = exp (arg1).
        // x^y = 2^(y*log2(x)). f2xm1 only accepts |x| <= 1, so the exponent
        // p = y*log2(x) is split as p = n + frac with n = trunc(p):
        // 2^frac comes from f2xm1 (frac is always in (-1, 1)) and 2^n is
        // built directly in the IEEE-754 exponent field.
        void Generator::genMathPow() {
            const std::string done = out_.label("pow.done");
            const std::string zero = out_.label("pow.zero");
            const std::string inf = out_.label("pow.inf");
            const std::string normal = out_.label("pow.normal");

            out_.def("__nx_math_pow");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();

            // p = exp * log2(base)  (ST1=exp, ST0=base for fyl2x)
            out_.fldRbp(16);
            out_.fldRbp(24);
            out_.fyl2x();
            out_.fstpRbp(-8); // [rbp-8] = p

            // frac = p - trunc(p), kept as raw bits at [rbp-24]
            out_.movsdXmm0FromRbp(-8);
            out_.cvttsd2siRaxXmm0(); // rax = n = trunc(p)
            out_.movRbpFromRax(-32); // [rbp-32] = n
            out_.cvtsi2sdXmm1Rax();  // xmm1 = (f64)n
            out_.movsdXmm2Xmm0();    // xmm2 = p
            out_.subsdXmm0Xmm1();    // xmm0 = frac
            out_.movqRaxXmm0();
            out_.movRbpFromRax(-24);

            // 2^frac via x87: f2xm1(frac) + 1 -> [rbp-40]
            out_.fldRbp(-24);
            out_.f2xm1();
            out_.fld1();
            out_.faddp();
            out_.fstpRbp(-40);

            // 2^n via exponent bits: (n + 1023) << 52
            out_.movRaxFromRbp(-32);
            out_.movRcxImm64(1023);
            out_.addRaxRcx();
            out_.testRaxRax();
            out_.jcc(J_S, zero); // n + 1023 < 0 -> underflow to 0
            out_.cmpRaxImm32(2046);
            out_.jcc(J_A, inf); // exponent field overflow -> inf
            out_.def(normal);
            out_.shlRaxImm8(52);
            out_.movRbpFromRax(-48);
            out_.movsdXmm0FromRbp(-48); // xmm0 = 2^n
            out_.mulsdXmm0FromRbp(-40); // xmm0 *= 2^frac
            out_.jmp(done);
            out_.def(zero);
            out_.xorpdXmm0Xmm0();
            out_.jmp(done);
            out_.def(inf);
            out_.movRaxImm64(0x7FF0000000000000ULL);
            out_.movqXmm0Rax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        void Generator::genMathSin() {
            genMathUnaryX87("__nx_math_sin", 0);
        }
        void Generator::genMathCos() {
            genMathUnaryX87("__nx_math_cos", 1);
        }
        void Generator::genMathTan() {
            genMathUnaryX87("__nx_math_tan", 2);
        }
        void Generator::genMathFloor() {
            genMathRound(true);
        }
        void Generator::genMathCeil() {
            genMathRound(false);
        }

    } // namespace detail
} // namespace npp::native
