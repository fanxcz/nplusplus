#include "native/native_generator.hpp"

#include <stdexcept>

namespace npp::native {
    namespace detail {

        Generator::Generator(CodeImage& image, NativeTarget target)
            : image_(image), out_(image), target_(target) {}

        Kind Generator::typeKind(const Type& t) {
            return kindOf(t);
        }

        void Generator::generate(const Program& program) {
            for (const auto& s : program.structs)
                structs_[s.name] = &s;
            for (const auto& e : program.enums)
                enums_[e.name] = &e;
            for (const auto& f : program.functions) {
                FnSig sig;
                for (const auto& p : f.params)
                    sig.params.push_back(p.type);
                sig.ret = f.ret;
                sigs_[f.name] = sig;
            }
            for (const auto& f : program.functions)
                genFunction(f);
            genWrite();
            genWriteErr();
            genWriteCstr();
            genStrEq();
            genStringLen();
            genStringByteAt();
            genStrCat();
            genStrI64();
            genInput();
            genStrToI64();
            genInputI64();
            genExit();
            genTimeMs();
            genSleep();
            genRngNext();
            genRandomI64();
            genFileExists();
            genStrF64();
            genPrintF64();
            genStrToF64();
            genInputF64();
            genAlloc();
            genDup();
            genOob();
            genStrlen();
            genArgsInit();
            genOsArgsCount();
            genOsArgsGet();
            genEnv();
            genFileRead();
            genFileReadChecked();
            genFileWrite();
            genMemAlloc();
            genMemFree();
            genMemRead();
            genMemSet();
            genMemCopy();
            genBeep();
            genKeyPressed();
            genReadKey();
            genUtf8Dec();
            genStrChars();
            genStrCharAt();
            genStrValidUtf8();
            genStrFromChar();
            genStrTrim();
            genStrUpper();
            genStrLower();
            genStrFind();
            genStrContains();
            genStrSlice();
            genStrSplitCount();
            genStrSplitAt();
            genLcNew();
            genLcGrow();
            genLcLen();
            genLcGet();
            genLcSet(false);
            genLcSet(true);
            genLcPush(false);
            genLcPush(true);
            genLcReserve();
            genLcClear(false);
            genLcClear(true);
            genLcInsert();
            genLcTryGet();
            genLcRemove();
            genLcTryPop();
            genMathSqrt();
            genMathSin();
            genMathCos();
            genMathTan();
            genMathFloor();
            genMathCeil();
            genMathAtan2();
            genMathPow();
            genPhysicsAabb();
            genPhysicsCircle();
            genOom();
            genDivZero();
            genDivOverflow();
            genPrintI64();
            genPrintU64();
            genPrintBool();
            genEntry();
        }

        std::int64_t Generator::slotOff(std::size_t slot) {
            return -static_cast<std::int64_t>(8 * (slot + 1));
        }

        std::size_t Generator::internString(const std::string& text) {
            for (std::size_t i = 0; i < image_.strings.size(); ++i)
                if (image_.strings[i] == text)
                    return i;
            image_.strings.push_back(text);
            return image_.strings.size() - 1;
        }

        void Generator::genFunction(const Function& f) {
            currentFn_ = &f;
            slots_.clear();
            forLimitNames_.clear();
            forIterNames_.clear();
            compoundNames_.clear();
            scratchTypes_.clear();
            forLimitCounter_ = 0;
            forIterCounter_ = 0;
            compoundCounter_ = 0;
            slotCount_ = 0;
            for (const auto& p : f.params)
                slots_[p.name] = slotCount_++;
            // Pre-scan let bindings so every local owns a fixed frame slot.
            collectLocals(f.body.get());
            const std::size_t frame = (slotCount_ * 8 + 15) / 16 * 16;

            out_.def(f.name);
            out_.pushRbp();
            out_.movRbpRsp();
            if (frame > 0)
                out_.subRsp(static_cast<std::uint32_t>(frame));
            // Arguments arrive on the stack, pushed left-to-right, so argument k
            // sits at rbp+16+8*(nargs-1-k) once the frame is established.
            const std::size_t nargs = f.params.size();
            for (std::size_t k = 0; k < nargs; ++k) {
                const std::int64_t src = 16 + static_cast<std::int64_t>(8 * (nargs - 1 - k));
                const std::int64_t dst = slotOff(slots_[f.params[k].name]);
                if (typeKind(f.params[k].type) == Kind::F64) {
                    out_.movsdXmm0FromRbp(src);
                    out_.movsdXmm0ToRbp(dst);
                } else {
                    out_.movR11FromRbp(src);
                    out_.movRbpFromR11(dst);
                }
            }
            endLabel_ = out_.label(f.name + "$end");
            breakStack_.clear();
            continueStack_.clear();
            fnDefers_.clear();
            for (const auto& s : f.body->statements) {
                if (const auto* d = dynamic_cast<const DeferStmt*>(s.get()))
                    fnDefers_.push_back(d->call.get());
            }
            genBlock(f.body.get());
            out_.def(endLabel_);
            for (auto it = fnDefers_.rbegin(); it != fnDefers_.rend(); ++it)
                genExpr(*it);
            if (f.name == "main" && f.ret.kind == TypeKind::Void)
                out_.xorEax();
            out_.movRspRbp();
            out_.popRbp();
            out_.ret();
            currentFn_ = nullptr;
        }

        void Generator::collectLocals(const Block* b) {
            for (const auto& s : b->statements) {
                if (const auto* nested = dynamic_cast<const Block*>(s.get())) {
                    collectLocals(nested);
                } else if (const auto* l = dynamic_cast<const LetStmt*>(s.get())) {
                    if (!slots_.contains(l->name))
                        slots_[l->name] = slotCount_++;
                } else if (const auto* i = dynamic_cast<const IfStmt*>(s.get())) {
                    collectLocals(i->thenBlock.get());
                    if (i->elseBlock)
                        collectLocals(i->elseBlock.get());
                } else if (const auto* w = dynamic_cast<const WhileStmt*>(s.get())) {
                    collectLocals(w->body.get());
                } else if (const auto* f = dynamic_cast<const ForStmt*>(s.get())) {
                    if (f->rangeEnd) {
                        if (!slots_.contains(f->name))
                            slots_[f->name] = slotCount_++;
                        const std::string limitName = std::string("\x01native_for_limit_") +
                                                      std::to_string(forLimitCounter_++);
                        forLimitNames_[f] = limitName;
                        slots_[limitName] = slotCount_++;
                    } else if (f->collection) {
                        // `for x in arr`: element var + hidden ptr/index slots.
                        if (!slots_.contains(f->name))
                            slots_[f->name] = slotCount_++;
                        const std::string base =
                            std::string("\x01native_for_iter_") + std::to_string(forIterCounter_++);
                        const std::string ptrName = base + "_ptr";
                        const std::string idxName = base + "_idx";
                        forIterNames_[f] = {ptrName, idxName};
                        slots_[ptrName] = slotCount_++;
                        slots_[idxName] = slotCount_++;
                    }
                    collectLocals(f->body.get());
                } else if (const auto* mm = dynamic_cast<const MatchStmt*>(s.get())) {
                    const std::string base =
                        std::string("\x01native_match_") + std::to_string(matchCounter_++);
                    matchValueNames_[mm] = base + "_val";
                    slots_[base + "_val"] = slotCount_++;
                    for (const auto& arm : mm->arms) {
                        if (!arm.binding.empty() && !slots_.contains(arm.binding))
                            slots_[arm.binding] = slotCount_++;
                        collectLocals(arm.body.get());
                    }
                } else if (const auto* ma = dynamic_cast<const MemberAssignStmt*>(s.get())) {
                    if (ma->op != TokenKind::Equal) {
                        const std::string base =
                            std::string("\x01native_cmp_") + std::to_string(compoundCounter_++);
                        compoundNames_[ma] = {base + "_cur", base + "_rhs"};
                        slots_[base + "_cur"] = slotCount_++;
                        slots_[base + "_rhs"] = slotCount_++;
                    }
                } else if (const auto* ia = dynamic_cast<const IndexAssignStmt*>(s.get())) {
                    if (ia->op != TokenKind::Equal) {
                        const std::string base =
                            std::string("\x01native_cmp_") + std::to_string(compoundCounter_++);
                        compoundNames_[ia] = {base + "_cur", base + "_rhs"};
                        slots_[base + "_cur"] = slotCount_++;
                        slots_[base + "_rhs"] = slotCount_++;
                    }
                } else if (const auto* da = dynamic_cast<const DerefAssignStmt*>(s.get())) {
                    if (da->op != TokenKind::Equal) {
                        const std::string base =
                            std::string("\x01native_cmp_") + std::to_string(compoundCounter_++);
                        compoundNames_[da] = {base + "_cur", base + "_rhs"};
                        slots_[base + "_cur"] = slotCount_++;
                        slots_[base + "_rhs"] = slotCount_++;
                    }
                }
            }
        }

        void Generator::genBlock(const Block* b) {
            for (const auto& s : b->statements)
                genStmt(s.get());
        }

    } // namespace detail
} // namespace npp::native
