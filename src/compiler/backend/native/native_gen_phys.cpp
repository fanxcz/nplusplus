#include "native/native_generator.hpp"

#include <cstring>

namespace npp::native {
    namespace detail {

        // Game-physics predicates. Both take f64 arguments as raw bits on
        // the stack, exactly like user-function parameters: for n args,
        // arg k sits at [rbp + 16 + 8*(n-1-k)].
        //
        // physics_aabb(ax, ay, aw, ah, bx, by, bw, bh): strict overlap of
        // two positive-size rectangles; mirrors npp_physics_aabb in
        // src/runtime/game/physics.inc (non-finite inputs reject).
        // physics_circle(ax, ay, ar, bx, by, br): distance <= r1 + r2 with
        // non-negative radii; mirrors npp_physics_circle.

        void Generator::genPhysicsAabb() {
            const std::string fail = out_.label("aabb.fail");
            const std::string ok = out_.label("aabb.ok");
            const std::string done = out_.label("aabb.done");

            out_.def("__nx_physics_aabb");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();

            // NaN guard: every argument must equal itself (ucomisd sets PF
            // on unordered). Offsets: arg k -> rbp+16+8*(7-k).
            static constexpr std::int64_t kOff[8] = {72, 64, 56, 48, 40, 32, 24, 16};
            for (const std::int64_t off : kOff) {
                out_.movsdXmm0FromRbp(off);
                out_.ucomisdXmm0Xmm0();
                out_.jcc(J_PE, fail);
            }

            // Derived edges: ar=ax+aw->[rbp-8], ab=ay+ah->[-16],
            // br=bx+bw->[-24], bb=by+bh->[-32]
            out_.movsdXmm0FromRbp(72); // ax
            out_.movsdXmm2FromRbp(56); // aw
            out_.addsdXmm0Xmm2();
            out_.movsdXmm0ToRbp(-8);
            out_.movsdXmm0FromRbp(64); // ay
            out_.movsdXmm2FromRbp(48); // ah
            out_.addsdXmm0Xmm2();
            out_.movsdXmm0ToRbp(-16);
            out_.movsdXmm0FromRbp(40); // bx
            out_.movsdXmm2FromRbp(24); // bw
            out_.addsdXmm0Xmm2();
            out_.movsdXmm0ToRbp(-24);
            out_.movsdXmm0FromRbp(32); // by
            out_.movsdXmm2FromRbp(16); // bh
            out_.addsdXmm0Xmm2();
            out_.movsdXmm0ToRbp(-32);

            // Dimension checks: aw, ah, bw, bh must be > 0. Unordered
            // (NaN) already rejected above; compare against 0.0 constant.
            const auto f64c = [this](double v) {
                std::string s(8, '\0');
                std::memcpy(s.data(), &v, 8);
                return "str" + std::to_string(internString(s));
            };
            const std::string zero = f64c(0.0);
            for (const std::int64_t off : {56, 48, 24, 16}) {
                out_.movsdXmm0FromRbp(off);
                out_.movsdXmm2FromRip(zero);
                out_.ucomisdXmm0Xmm2();
                out_.jcc(J_BE, fail); // dim <= 0 -> reject
            }

            // Overlap: ax < br && ar > bx && ay < bb && ab > by.
            // Each test: load lhs into xmm0, rhs into xmm2, ucomisd.
            auto cmp = [&](std::int64_t lhsOff, std::int64_t rhsOff, std::uint8_t jccFail) {
                out_.movsdXmm0FromRbp(lhsOff);
                out_.movsdXmm2FromRbp(rhsOff);
                out_.ucomisdXmm0Xmm2();
                out_.jcc(jccFail, fail);
            };
            cmp(72, -24, J_AE); // ax < br
            cmp(-8, 40, J_BE);  // ar > bx
            cmp(64, -32, J_AE); // ay < bb
            cmp(-16, 32, J_BE); // ab > by

            out_.def(ok);
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(fail);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        void Generator::genPhysicsCircle() {
            const std::string fail = out_.label("circ.fail");
            const std::string ok = out_.label("circ.ok");
            const std::string done = out_.label("circ.done");

            out_.def("__nx_physics_circle");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(96);
            out_.andRspMinus16();

            // NaN guard on all six args (arg k -> rbp+16+8*(5-k)).
            for (const std::int64_t off : {56, 48, 40, 32, 24, 16}) {
                out_.movsdXmm0FromRbp(off);
                out_.ucomisdXmm0Xmm0();
                out_.jcc(J_PE, fail);
            }

            const auto f64c = [this](double v) {
                std::string s(8, '\0');
                std::memcpy(s.data(), &v, 8);
                return "str" + std::to_string(internString(s));
            };
            const std::string zero = f64c(0.0);

            // Radii must be >= 0 (ar at arg2=rbp+40, br at arg5=rbp+16).
            for (const std::int64_t off : {40, 16}) {
                out_.movsdXmm0FromRbp(off);
                out_.movsdXmm2FromRip(zero);
                out_.ucomisdXmm0Xmm2();
                out_.jcc(J_B, fail); // radius < 0 or unordered -> reject
            }

            // dx = ax - bx -> [rbp-8] ; dy = ay - by -> xmm0
            out_.movsdXmm0FromRbp(56); // ax
            out_.movsdXmm2FromRbp(32); // bx
            out_.subsdXmm0Xmm2();
            out_.movsdXmm0ToRbp(-8);
            out_.movsdXmm0FromRbp(48); // ay
            out_.movsdXmm2FromRbp(24); // by
            out_.subsdXmm0Xmm2();      // dy

            // dist^2 = dx^2 + dy^2
            out_.mulsdXmm0Xmm0();      // dy^2
            out_.movsdXmm2FromRbp(-8); // dx
            out_.mulsdXmm2Xmm2();      // dx^2
            out_.addsdXmm0Xmm2();      // dy^2 + dx^2
            out_.sqrtsdXmm0Xmm0();     // dist
            out_.movsdXmm0ToRbp(-16);  // dist

            // r = ar + br -> [rbp-24]
            out_.movsdXmm0FromRbp(40); // ar
            out_.movsdXmm2FromRbp(16); // br
            out_.addsdXmm0Xmm2();      // r
            out_.movsdXmm0ToRbp(-24);
            out_.movsdXmm0FromRbp(-16); // dist
            out_.movsdXmm2FromRbp(-24); // r
            out_.ucomisdXmm0Xmm2();     // dist vs r
            out_.jcc(J_A, fail);        // dist > r -> reject
            out_.def(ok);
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(fail);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

    } // namespace detail
} // namespace npp::native
