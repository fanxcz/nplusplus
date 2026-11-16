#include "native/native_generator.hpp"

namespace npp::native {
    namespace detail {

        // Growable i64/string lists behind opaque i64 handles, mirroring
        // src/runtime/collections/lists.inc. A handle is a pointer to a
        // header block allocated from the string arena:
        //   +0  magic  = 0x4E504C53 ("NPLS")
        //   +8  type   = 1 (int list) / 2 (string list)
        //   +16 len    (element count)
        //   +24 cap    (element capacity)
        //   +32 data   (pointer to cap*8 bytes of elements)
        // Growth allocates a fresh arena block and copies; old blocks are
        // never reclaimed (the arena is bump-only), matching the managed
        // lifetime model closely enough for the native subset.
        //
        // Register convention for __nx_lc_* helpers:
        //   rax = handle, rcx = expected type tag, rdx = arg1, r8 = arg2.

        namespace {
            constexpr std::uint32_t kListMagic = 0x4E504C53u;
            constexpr std::int64_t kListHdrSize = 48;
        } // namespace

        // Emits the shared handle validation. Expects the handle in slot
        // -8 and the expected type tag in slot -16; jumps to `bad` on any
        // mismatch. Clobbers rax and rcx.
        void Generator::emitLcValid(const std::string& bad) {
            out_.movRaxFromRbp(-8);
            out_.testRaxRax();
            out_.jcc(J_E, bad);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(0);
            out_.cmpRaxImm32(kListMagic);
            out_.jcc(J_NE, bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(8);
            out_.movRcxFromRbp(-16);
            out_.cmpRaxRcx();
            out_.jcc(J_NE, bad);
        }

        // __nx_lc_new: rax = type tag -> rax = handle (0 on bad tag).
        void Generator::genLcNew() {
            const std::string bad = out_.label("lcn.bad");
            const std::string init = out_.label("lcn.init");
            const std::string done = out_.label("lcn.done");
            // Slot: -8 type
            out_.def("__nx_lc_new");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(1);
            out_.jcc(J_E, init);
            out_.movRaxFromRbp(-8);
            out_.cmpRaxImm32(2);
            out_.jcc(J_E, init);
            out_.def(bad);
            out_.xorEax();
            out_.jmp(done);
            out_.def(init);
            out_.movRaxImm64(kListHdrSize);
            out_.call("__nx_alloc");
            out_.movRcxRax(); // rcx = header
            out_.movRaxImm64(kListMagic);
            out_.movToRcxFromRaxOff(0);
            out_.movRaxFromRbp(-8);
            out_.movToRcxFromRaxOff(8);
            out_.xorEax();
            out_.movToRcxFromRaxOff(16);
            out_.movToRcxFromRaxOff(24);
            out_.movToRcxFromRaxOff(32);
            out_.movRaxRcx();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_grow: rax = h, rdx = needed capacity. Doubles cap until
        // it fits, allocates a fresh element block and copies len entries.
        void Generator::genLcGrow() {
            const std::string done = out_.label("lcg.done");
            const std::string have = out_.label("lcg.have");
            const std::string growSet = out_.label("lcg.growset");
            const std::string need = out_.label("lcg.need");
            const std::string alloc = out_.label("lcg.alloc");
            // Slots: -8 h, -16 needed, -24 newcap, -32 newdata, -40 len
            out_.def("__nx_lc_grow");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRdxToRbp(-16);
            // newcap = cap
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(24);
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-16);
            out_.cmpRaxFromRbp(-24);
            out_.jcc(J_BE, done); // needed <= cap
            out_.def(need);
            // newcap = cap == 0 ? 8 : cap * 2
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_NE, have);
            out_.movRaxImm64(8);
            out_.jmp(growSet);
            out_.def(have);
            out_.movRaxFromRbp(-24);
            out_.movRcxFromRbp(-24);
            out_.addRaxRcx();
            out_.def(growSet);
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-16);
            out_.jcc(J_AE, alloc);
            out_.jmp(need);
            out_.def(alloc);
            // newdata = __nx_alloc(newcap * 8)
            out_.movRaxFromRbp(-24);
            out_.shlRaxImm8(3);
            out_.call("__nx_alloc");
            out_.movRdiRax();
            out_.movRdiToRbp(-32);
            // copy len*8 bytes: rsi = old data, rdi = newdata, rcx = len*8
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-40); // len
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRsiFromRax(); // old data ptr
            out_.movRdiFromRbp(-32);
            out_.movRaxFromRbp(-40);
            out_.shlRaxImm8(3);
            out_.movRcxRax();
            out_.cld();
            out_.repMovsb();
            // [h+32] = newdata ; [h+24] = newcap
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRbp(-32);
            out_.movToRcxFromRaxOff(32);
            out_.movRaxFromRbp(-24);
            out_.movToRcxFromRaxOff(24);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_len: rax = h, rcx = type -> rax = len or 0.
        void Generator::genLcLen() {
            const std::string bad = out_.label("lcl.bad");
            const std::string ok = out_.label("lcl.ok");
            const std::string done = out_.label("lcl.done");
            out_.def("__nx_lc_len");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            emitLcValid(bad);
            out_.jmp(ok);
            out_.def(bad);
            out_.xorEax();
            out_.jmp(done);
            out_.def(ok);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_get: rax = h, rcx = type, rdx = index -> rax = element or 0.
        void Generator::genLcGet() {
            const std::string bad = out_.label("lcx.bad");
            const std::string done = out_.label("lcx.done");
            // Slots: -8 h, -16 type, -24 index, -32 len
            out_.def("__nx_lc_get");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            emitLcValid(bad);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, bad);
            // rax = data + index*8 -> rcx ; rax = [rcx]
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcx();
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_set: rax = h, rcx = type, rdx = index, r8 = value -> void.
        // __nx_lc_set_checked: same -> bool (1 stored / 0 rejected).
        void Generator::genLcSet(bool checked) {
            const std::string bad = out_.label("lcs.bad");
            const std::string ok = out_.label("lcs.ok");
            const std::string done = out_.label("lcs.done");
            // Slots: -8 h, -16 type, -24 index, -32 len, -48 value
            out_.def(checked ? "__nx_lc_set_checked" : "__nx_lc_set");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            out_.movRbpFromR8(-48); // save incoming r8 (value) arg
            emitLcValid(bad);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, bad);
            out_.def(ok);
            // rcx = data + index*8 ; [rcx] = value
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRbp(-48);
            out_.movToRcxFromRax();
            if (checked) {
                out_.movRaxImm64(1);
            } else {
                out_.xorEax();
            }
            out_.jmp(done);
            out_.def(bad);
            if (checked) {
                out_.xorEax();
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_push: rax = h, rcx = type, rdx = value -> void.
        // __nx_lc_push_checked: same -> bool.
        void Generator::genLcPush(bool checked) {
            const std::string bad = out_.label("lcp.bad");
            const std::string done = out_.label("lcp.done");
            // Slots: -8 h, -16 type, -24 value
            out_.def(checked ? "__nx_lc_push_checked" : "__nx_lc_push");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            emitLcValid(bad);
            // grow(len + 1): rax = h, rdx = len+1
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.incRax();
            out_.movRdxRax();
            out_.movRaxFromRbp(-8);
            out_.call("__nx_lc_grow");
            // data[len] = value ; len++
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16); // len
            out_.movRdxRax();
            out_.shlRdxImm8(3);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32); // data
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRbp(-24);
            out_.movToRcxFromRax();
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.incRax();
            out_.movToRcxFromRaxOff(16);
            if (checked) {
                out_.movRaxImm64(1);
            }
            out_.jmp(done);
            out_.def(bad);
            if (checked) {
                out_.xorEax();
            }
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_reserve: rax = h, rcx = type, rdx = capacity -> bool.
        void Generator::genLcReserve() {
            const std::string bad = out_.label("lcr.bad");
            const std::string done = out_.label("lcr.done");
            // Slots: -8 h, -16 type
            out_.def("__nx_lc_reserve");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            // capacity < 0 -> false
            out_.movRdxToRbp(-24);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            emitLcValid(bad);
            out_.movRaxFromRbp(-8);
            out_.movRdxFromRbp(-24);
            out_.call("__nx_lc_grow");
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_clear: rax = h, rcx = type -> bool.
        // __nx_lc_free: rax = h, rcx = type -> bool (magic zeroed).
        void Generator::genLcClear(bool freeIt) {
            const std::string bad = out_.label("lcc.bad");
            const std::string done = out_.label("lcc.done");
            out_.def(freeIt ? "__nx_lc_free" : "__nx_lc_clear");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            emitLcValid(bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            if (freeIt) {
                out_.xorEax();
                out_.movToRcxFromRaxOff(0); // magic = 0 -> dead handle
            } else {
                out_.xorEax();
                out_.movToRcxFromRaxOff(16); // len = 0
            }
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_insert: rax = h, rcx = type, rdx = index, r8 = value -> bool.
        void Generator::genLcInsert() {
            const std::string bad = out_.label("lci.bad");
            const std::string done = out_.label("lci.done");
            const std::string mvLoop = out_.label("lci.mvlo");
            const std::string mvDone = out_.label("lci.mvd");
            // Slots: -8 h, -16 type, -24 index, -32 len, -40 value, -48 j
            out_.def("__nx_lc_insert");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            out_.movRbpFromR8(-40); // save incoming r8 (value) arg
            emitLcValid(bad);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            // len
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_A, bad); // index > len
            // grow(len + 1)
            out_.movRaxFromRbp(-8);
            out_.movRaxFromRcxOff(16);
            out_.incRax();
            out_.movRdxRax();
            out_.movRaxFromRbp(-8);
            out_.call("__nx_lc_grow");
            // shift elements [index..len) one slot right, backwards
            out_.movRaxFromRbp(-32);
            out_.subRaxFromRbp(-24); // count = len - index
            out_.movRbpFromRax(-48); // j = count
            out_.def(mvLoop);
            out_.movRaxFromRbp(-48);
            out_.testRaxRax();
            out_.jcc(J_E, mvDone);
            // rdx = (index + j - 1) * 8
            out_.movRdxFromRbp(-24);
            out_.movRaxFromRbp(-48);
            out_.addRdxRax();
            out_.decRdx();
            out_.shlRdxImm8(3);
            // rax = data + rdx (src) ; rcx = src + 8 (dst)
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcx(); // value
            out_.addRcxImm32(8);
            out_.movToRcxFromRax();
            out_.movRaxFromRbp(-48);
            out_.decRax();
            out_.movRbpFromRax(-48);
            out_.jmp(mvLoop);
            out_.def(mvDone);
            // data[index] = value ; len++
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRbp(-40);
            out_.movToRcxFromRax();
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.incRax();
            out_.movToRcxFromRaxOff(16);
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_try_get: rax = h, rcx = type, rdx = index, r8 = out -> bool.
        void Generator::genLcTryGet() {
            const std::string bad = out_.label("lct.bad");
            const std::string done = out_.label("lct.done");
            // Slots: -8 h, -16 type, -24 index, -32 len
            out_.def("__nx_lc_try_get");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(64);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            emitLcValid(bad);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, bad);
            // rax = data[index] ; *out = rax
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcx();
            out_.movRcxR8();
            out_.movToRcxFromRax();
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_remove: rax = h, rcx = type, rdx = index, r8 = out -> bool.
        void Generator::genLcRemove() {
            const std::string bad = out_.label("lcr2.bad");
            const std::string done = out_.label("lcr2.done");
            const std::string mvLoop = out_.label("lcr2.mvlo");
            const std::string mvDone = out_.label("lcr2.mvd");
            // Slots: -8 h, -16 type, -24 index, -32 len, -40 j, -48 src
            out_.def("__nx_lc_remove");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(80);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            out_.movRdxToRbp(-24);
            emitLcValid(bad);
            out_.movRaxFromRbp(-24);
            out_.testRaxRax();
            out_.jcc(J_S, bad);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.movRbpFromRax(-32);
            out_.movRaxFromRbp(-24);
            out_.cmpRaxFromRbp(-32);
            out_.jcc(J_AE, bad);
            // *out = data[index]
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcx();
            out_.movRcxR8();
            out_.movToRcxFromRax();
            // shift [index+1..len) left by one
            out_.movRaxFromRbp(-32);
            out_.subRaxFromRbp(-24);
            out_.decRax();
            out_.movRbpFromRax(-40); // j = len - index - 1
            out_.def(mvLoop);
            out_.movRaxFromRbp(-40);
            out_.testRaxRax();
            out_.jcc(J_E, mvDone);
            // rdx = (index + j) * 8
            out_.movRdxFromRbp(-24);
            out_.movRaxFromRbp(-40);
            out_.addRdxRax();
            out_.shlRdxImm8(3);
            // rcx = data + rdx (dst) ; rax = [rcx + 8] (src)
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcxOff(8); // src = [rcx+8]
            out_.movToRcxFromRax();   // [rcx] = src
            out_.movRaxFromRbp(-40);
            out_.decRax();
            out_.movRbpFromRax(-40);
            out_.jmp(mvLoop);
            out_.def(mvDone);
            // len--
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.decRax();
            out_.movToRcxFromRaxOff(16);
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

        // __nx_lc_try_pop: rax = h, rcx = type, r8 = out -> bool.
        void Generator::genLcTryPop() {
            const std::string bad = out_.label("lcp2.bad");
            const std::string done = out_.label("lcp2.done");
            // Slots: -8 h, -16 type
            out_.def("__nx_lc_try_pop");
            out_.pushRbp();
            out_.movRbpRsp();
            out_.subRsp(48);
            out_.andRspMinus16();
            out_.movRbpFromRax(-8);
            out_.movRcxToRbp(-16);
            emitLcValid(bad);
            // len == 0 -> false
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(16);
            out_.testRaxRax();
            out_.jcc(J_E, bad);
            // newlen = len - 1 -> slot -24 ; store back to [h+16]
            out_.decRax();
            out_.movRbpFromRax(-24);
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRbp(-24);
            out_.movToRcxFromRaxOff(16);
            // rax = data[newlen]
            out_.movRaxFromRbp(-8);
            out_.movRcxFromRax();
            out_.movRaxFromRcxOff(32);
            out_.movRdxFromRbp(-24);
            out_.shlRdxImm8(3);
            out_.addRaxRdx();
            out_.movRcxRax();
            out_.movRaxFromRcx();
            out_.movRcxR8();
            out_.movToRcxFromRax(); // *out = element
            out_.movRaxImm64(1);
            out_.jmp(done);
            out_.def(bad);
            out_.xorEax();
            out_.def(done);
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
        }

    } // namespace detail
} // namespace npp::native
