#include "native/native_generator.hpp"

#include <cstring>
#include <stdexcept>

namespace npp::native {
    namespace detail {

        void Generator::genStmt(const Stmt* s) {
            if (const auto* nested = dynamic_cast<const Block*>(s)) {
                genBlock(nested);
                return;
            }
            if (const auto* l = dynamic_cast<const LetStmt*>(s)) {
                genExpr(l->init.get());
                if (needsDup(l->init.get()))
                    genDupValue(exprType(l->init.get()));
                if (exprKind(l->init.get()) == Kind::F64)
                    out_.movsdXmm0ToRbp(slotOff(slots_.at(l->name)));
                else
                    out_.movRbpFromRax(slotOff(slots_.at(l->name)));
                return;
            }
            if (const auto* a = dynamic_cast<const AssignStmt*>(s)) {
                genExpr(a->value.get());
                if (needsDup(a->value.get()))
                    genDupValue(exprType(a->value.get()));
                if (exprKind(a->value.get()) == Kind::F64)
                    out_.movsdXmm0ToRbp(slotOff(slots_.at(a->name)));
                else
                    out_.movRbpFromRax(slotOff(slots_.at(a->name)));
                return;
            }
            if (const auto* ma = dynamic_cast<const MemberAssignStmt*>(s)) {
                // Evaluate the object, form the field address, then evaluate
                // the value and store it through rcx.
                genExpr(ma->object.get());
                out_.movRcxRax();
                const Type ot = exprType(ma->object.get());
                const int fi = structFieldIndex(ot.name, ma->member);
                const std::int64_t off = static_cast<std::int64_t>(fi) * 8;
                if (off > 0)
                    out_.addRcxImm32(static_cast<std::uint32_t>(off));
                out_.pushRcx(); // [rsp] = &field
                if (ma->op != TokenKind::Equal) {
                    const auto sit = structs_.find(ot.name);
                    const Type ft = sit != structs_.end()
                                        ? sit->second->fields[static_cast<std::size_t>(fi)].type
                                        : Type::i64();
                    genCompoundAssign(ma, ma->value.get(), ft, ma->op);
                    return;
                }
                genExpr(ma->value.get());
                if (needsDup(ma->value.get()))
                    genDupValue(exprType(ma->value.get()));
                out_.popRcx();
                genStoreToRcx(kindOf(exprType(ma->value.get())));
                return;
            }
            if (const auto* ia = dynamic_cast<const IndexAssignStmt*>(s)) {
                const Type ot = exprType(ia->object.get());
                genExpr(ia->object.get());
                out_.pushRax();
                genExpr(ia->index.get());
                genIndexCheck(ot.arraySize);
                out_.movRdxRax();
                out_.shlRdxImm8(3);
                out_.popRcx();
                out_.addRcxRdx();
                out_.pushRcx(); // [rsp] = &element
                if (ia->op != TokenKind::Equal) {
                    genCompoundAssign(ia, ia->value.get(), *ot.element, ia->op);
                    return;
                }
                genExpr(ia->value.get());
                if (needsDup(ia->value.get()))
                    genDupValue(exprType(ia->value.get()));
                out_.popRcx();
                genStoreToRcx(kindOf(*ot.element));
                return;
            }
            if (const auto* da = dynamic_cast<const DerefAssignStmt*>(s)) {
                // `*p = v`: p evaluates to the slot address; store v through it.
                const Type tt = exprType(da->target.get());
                const Type pt = tt.pointee ? *tt.pointee : Type::i64();
                genExpr(da->target.get());
                out_.pushRax(); // [rsp] = &pointee slot
                if (da->op != TokenKind::Equal) {
                    genCompoundAssign(da, da->value.get(), pt, da->op);
                    return;
                }
                genExpr(da->value.get());
                out_.popRcx();
                genStoreToRcx(kindOf(pt));
                return;
            }
            if (const auto* e = dynamic_cast<const ExprStmt*>(s)) {
                genExpr(e->expr.get());
                return;
            }
            if (const auto* d = dynamic_cast<const DeferStmt*>(s)) {
                // Top-level defers are emitted at the function epilogue
                // (see genFunction); nested defers never reach codegen.
                (void)d;
                return;
            }
            if (const auto* r = dynamic_cast<const ReturnStmt*>(s)) {
                if (r->expr)
                    genExpr(r->expr.get());
                out_.jmp(endLabel_);
                return;
            }
            if (const auto* i = dynamic_cast<const IfStmt*>(s)) {
                genExpr(i->cond.get());
                out_.testRaxRax();
                const std::string els = out_.label("if.else");
                const std::string end = out_.label("if.end");
                out_.jcc(J_E, i->elseBlock ? els : end);
                genBlock(i->thenBlock.get());
                if (i->elseBlock) {
                    out_.jmp(end);
                    out_.def(els);
                    genBlock(i->elseBlock.get());
                }
                out_.def(end);
                return;
            }
            if (const auto* f = dynamic_cast<const ForStmt*>(s)) {
                if (!f->rangeEnd) {
                    if (!f->collection)
                        throw std::runtime_error(
                            "builtin native v0: unsupported for-loop form reached codegen");
                    genForCollection(f);
                    return;
                }
                const std::string limitName = forLimitNames_.at(f);
                const std::string cond = out_.label("for.cond");
                const std::string body = out_.label("for.body");
                const std::string increment = out_.label("for.increment");
                const std::string end = out_.label("for.end");

                // Evaluate both bounds exactly once, before entering the loop.
                genExpr(f->start.get());
                out_.movRbpFromRax(slotOff(slots_.at(f->name)));
                genExpr(f->rangeEnd.get());
                out_.movRbpFromRax(slotOff(slots_.at(limitName)));

                out_.jmp(cond);
                out_.def(cond);
                BinaryExpr test(TokenKind::Less,
                                std::make_unique<VarExpr>(f->name),
                                std::make_unique<VarExpr>(limitName));
                genExpr(&test);
                out_.testRaxRax();
                out_.jcc(J_E, end);
                out_.def(body);
                breakStack_.push_back(end);
                continueStack_.push_back(increment);
                genBlock(f->body.get());
                breakStack_.pop_back();
                continueStack_.pop_back();

                out_.def(increment);
                BinaryExpr nextValue(TokenKind::Plus,
                                     std::make_unique<VarExpr>(f->name),
                                     std::make_unique<IntExpr>(1));
                genExpr(&nextValue);
                out_.movRbpFromRax(slotOff(slots_.at(f->name)));
                out_.jmp(cond);
                out_.def(end);
                return;
            }
            if (const auto* w = dynamic_cast<const WhileStmt*>(s)) {
                const std::string cond = out_.label("while.cond");
                const std::string body = out_.label("while.body");
                const std::string end = out_.label("while.end");
                out_.jmp(cond);
                out_.def(cond);
                genExpr(w->cond.get());
                out_.testRaxRax();
                out_.jcc(J_E, end);
                out_.def(body);
                breakStack_.push_back(end);
                continueStack_.push_back(cond);
                genBlock(w->body.get());
                breakStack_.pop_back();
                continueStack_.pop_back();
                out_.jmp(cond);
                out_.def(end);
                return;
            }
            if (dynamic_cast<const BreakStmt*>(s)) {
                out_.jmp(breakStack_.back());
                return;
            }
            if (dynamic_cast<const ContinueStmt*>(s)) {
                out_.jmp(continueStack_.back());
                return;
            }
            throw std::runtime_error("builtin native v0: unsupported statement reached codegen");
        }

        void Generator::genExpr(const Expr* e) {
            if (const auto* x = dynamic_cast<const IntExpr*>(e)) {
                const auto bits = x->negative ? (std::uint64_t{0} - x->value) : x->value;
                out_.movRaxImm64(bits);
                return;
            }
            if (const auto* x = dynamic_cast<const BoolExpr*>(e)) {
                out_.movRaxImm64(x->value ? 1 : 0);
                return;
            }
            if (const auto* x = dynamic_cast<const FloatExpr*>(e)) {
                std::uint64_t bits = 0;
                std::memcpy(&bits, &x->value, sizeof(bits));
                out_.movRaxImm64(bits);
                out_.movqXmm0Rax();
                return;
            }
            if (const auto* x = dynamic_cast<const VarExpr*>(e)) {
                if (localKind(x->name) == Kind::F64) {
                    out_.movsdXmm0FromRbp(slotOff(slots_.at(x->name)));
                    return;
                }
                out_.movRaxFromRbp(slotOff(slots_.at(x->name)));
                return;
            }
            if (dynamic_cast<const StringExpr*>(e)) {
                const auto* s = static_cast<const StringExpr*>(e);
                const std::size_t id = internString(s->value);
                out_.leaRaxRip("str" + std::to_string(id));
                return;
            }
            if (const auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                if (u->op == TokenKind::Ampersand) {
                    genAddr(u->rhs.get());
                    return;
                }
                if (u->op == TokenKind::Star) {
                    // `*p`: the pointer value is the slot address; load the
                    // pointee (f64 lands in xmm0, everything else in rax).
                    genExpr(u->rhs.get());
                    const Type pt = exprType(u->rhs.get());
                    const Type pt2 = pt.pointee ? *pt.pointee : Type::i64();
                    out_.movRcxRax();
                    genLoadFromRcx(kindOf(pt2));
                    return;
                }
                genExpr(u->rhs.get());
                if (u->op == TokenKind::Minus) {
                    if (exprKind(u->rhs.get()) == Kind::F64) {
                        // Flip the sign bit: xmm0 ^= 0x8000000000000000.
                        out_.movqRaxXmm0();
                        out_.movR11Imm64(0x8000000000000000ULL);
                        out_.xorRaxR11();
                        out_.movqXmm0Rax();
                    } else {
                        out_.negRax();
                    }
                } else {
                    out_.notBoolRax();
                }
                return;
            }
            if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                // String concatenation and comparison go through arena helpers.
                if (b->op == TokenKind::Plus && exprIsString(b->lhs.get())) {
                    genExpr(b->lhs.get());
                    out_.pushRax();
                    genExpr(b->rhs.get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.call("__nx_str_cat");
                    return;
                }
                if ((b->op == TokenKind::EqualEqual || b->op == TokenKind::BangEqual) &&
                    exprIsString(b->lhs.get())) {
                    genExpr(b->lhs.get());
                    out_.pushRax();
                    genExpr(b->rhs.get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.call("__nx_str_eq");
                    if (b->op == TokenKind::BangEqual)
                        out_.notBoolRax();
                    return;
                }
                // `&&`/`||` must not evaluate the right-hand side when the
                // left-hand side already decides the result.
                if (b->op == TokenKind::AndAnd || b->op == TokenKind::OrOr) {
                    const std::string end = out_.label("logic.end");
                    genExpr(b->lhs.get());
                    out_.testRaxRax();
                    out_.jcc(b->op == TokenKind::AndAnd ? J_E : J_NE, end);
                    genExpr(b->rhs.get());
                    out_.def(end);
                    return;
                }
                // f64 arithmetic and comparisons run in xmm registers; the
                // boolean result of a comparison still lands in rax.
                if (exprKind(b->lhs.get()) == Kind::F64) {
                    genExpr(b->lhs.get());
                    out_.subRsp(16);
                    out_.movsdXmm0ToRsp();
                    genExpr(b->rhs.get());
                    out_.movsdXmm1Xmm0();    // xmm1 = rhs
                    out_.movsdXmm0FromRsp(); // xmm0 = lhs
                    out_.addRsp(16);
                    switch (b->op) {
                    case TokenKind::Plus:
                        out_.addsdXmm0Xmm1();
                        return;
                    case TokenKind::Minus:
                        out_.subsdXmm0Xmm1();
                        return;
                    case TokenKind::Star:
                        // mulsd xmm0, xmm1
                        out_.sseArithXmm0Xmm1(0x59);
                        return;
                    case TokenKind::Slash:
                        out_.sseArithXmm0Xmm1(0x5E);
                        return;
                    case TokenKind::EqualEqual:
                    case TokenKind::BangEqual:
                    case TokenKind::Less:
                    case TokenKind::LessEqual:
                    case TokenKind::Greater:
                    case TokenKind::GreaterEqual: {
                        // Ordered comparisons only: unordered (NaN) yields 0,
                        // matching the LLVM backend's oeq/olt/… predicates.
                        out_.ucomisdXmm0Xmm1();
                        const std::string unordered = out_.label("fcmp.unordered");
                        const std::string done = out_.label("fcmp.done");
                        out_.jcc(J_PE, unordered);
                        switch (b->op) {
                        case TokenKind::EqualEqual:
                            out_.setcc(CC_E);
                            break;
                        case TokenKind::BangEqual:
                            out_.setcc(CC_NE);
                            break;
                        case TokenKind::Less:
                            out_.setcc(CC_B);
                            break;
                        case TokenKind::LessEqual:
                            out_.setcc(CC_BE);
                            break;
                        case TokenKind::Greater:
                            out_.setcc(CC_A);
                            break;
                        default:
                            out_.setcc(CC_AE);
                            break;
                        }
                        out_.movzxEaxAl();
                        out_.jmp(done);
                        out_.def(unordered);
                        out_.xorEax();
                        out_.def(done);
                        return;
                    }
                    default:
                        throw std::runtime_error(
                            "builtin native backend: unsupported f64 operator reached codegen");
                    }
                }
                genExpr(b->lhs.get());
                out_.pushRax();
                genExpr(b->rhs.get());
                out_.movRcxRax();
                out_.popRax();
                switch (b->op) {
                case TokenKind::Plus:
                    out_.addRaxRcx();
                    break;
                case TokenKind::Minus:
                    out_.subRaxRcx();
                    break;
                case TokenKind::Star:
                    out_.imulRaxRcx();
                    break;
                case TokenKind::Slash:
                case TokenKind::Percent: {
                    // Use the same checked failure path as the LLVM backend.
                    const std::string nonzero = out_.label("division.nonzero");
                    const std::string noOverflow = out_.label("division.nooverflow");
                    out_.testRcxRcx();
                    out_.jcc(J_NE, nonzero);
                    out_.jmp("__nx_divzero");
                    out_.def(nonzero);
                    // x86 idiv traps for INT64_MIN / -1 (including remainder).
                    // Check the pair explicitly so the native backend reports the
                    // same controlled runtime error as the LLVM backend.
                    out_.movR11Imm64(0x8000000000000000ULL);
                    out_.cmpRaxR11();
                    out_.jcc(J_NE, noOverflow);
                    out_.cmpRcxImm8(0xFF); // divisor == -1
                    out_.jcc(J_NE, noOverflow);
                    out_.jmp("__nx_divoverflow");
                    out_.def(noOverflow);
                    out_.cqo();
                    out_.idivRcx();
                    if (b->op == TokenKind::Percent)
                        out_.movRaxRdx();
                    break;
                }
                case TokenKind::EqualEqual:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_E);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::BangEqual:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_NE);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::Less:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_L);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::LessEqual:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_LE);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::Greater:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_G);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::GreaterEqual:
                    out_.cmpRaxRcx();
                    out_.setcc(CC_GE);
                    out_.movzxEaxAl();
                    break;
                case TokenKind::AndAnd:
                    out_.andRaxRcx();
                    break;
                case TokenKind::OrOr:
                    out_.orRaxRcx();
                    break;
                case TokenKind::Ampersand:
                    out_.andRaxRcx();
                    break;
                case TokenKind::Pipe:
                    out_.orRaxRcx();
                    break;
                case TokenKind::Caret:
                    out_.xorRaxRcx();
                    break;
                case TokenKind::ShiftLeft:
                    out_.shlRaxCl();
                    break;
                case TokenKind::ShiftRight:
                    out_.sarRaxCl();
                    break;
                default:
                    throw std::runtime_error(
                        "builtin native v0: unsupported operator reached codegen");
                }
                return;
            }
            if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                if (c->callee == "print") {
                    genPrint(c->args[0].get());
                    return;
                }
                if (c->callee == "input" || c->callee == "str_i64") {
                    genExpr(c->args[0].get());
                    out_.call(c->callee == "input" ? "__nx_input" : "__nx_str_i64");
                    return;
                }
                if (c->callee == "os.args_count") {
                    out_.call("__nx_os_args_count");
                    return;
                }
                if (c->callee == "beep" || c->callee == "key_pressed" || c->callee == "read_key") {
                    out_.call(c->callee == "beep"          ? "__nx_beep"
                              : c->callee == "key_pressed" ? "__nx_key_pressed"
                                                           : "__nx_read_key");
                    return;
                }
                if (c->callee == "os.args_get") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_os_args_get");
                    return;
                }
                if (c->callee == "string.len") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_string_len");
                    return;
                }
                if (c->callee == "string.byte_at") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.movRcxRax();
                    out_.popRax();
                    out_.call("__nx_string_byte_at");
                    return;
                }
                if (c->callee == "exit" || c->callee == "sleep") {
                    genExpr(c->args[0].get());
                    out_.movRcxRax();
                    out_.call(c->callee == "exit" ? "__nx_exit" : "__nx_sleep");
                    return;
                }
                if (c->callee == "time_ms") {
                    out_.call("__nx_time_ms");
                    return;
                }
                if (c->callee == "random_i64") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.call("__nx_random_i64");
                    return;
                }
                if (c->callee == "str_bool") {
                    genExpr(c->args[0].get());
                    const std::size_t t = internString("true");
                    const std::size_t f = internString("false");
                    out_.testRaxRax();
                    const std::string isFalse = out_.label("sbool.false");
                    const std::string go = out_.label("sbool.go");
                    out_.jcc(J_E, isFalse);
                    out_.leaRaxRip("str" + std::to_string(t));
                    out_.jmp(go);
                    out_.def(isFalse);
                    out_.leaRaxRip("str" + std::to_string(f));
                    out_.def(go);
                    return;
                }
                if (c->callee == "input_i64") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_input_i64");
                    return;
                }
                if (c->callee == "file_exists") {
                    genExpr(c->args[0].get());
                    out_.movRcxRax();
                    out_.call("__nx_file_exists");
                    return;
                }
                if (c->callee == "env") {
                    genExpr(c->args[0].get());
                    out_.movRcxRax();
                    out_.call("__nx_env");
                    return;
                }
                if (c->callee == "file_read") {
                    genExpr(c->args[0].get());
                    out_.movRcxRax();
                    out_.call("__nx_file_read");
                    return;
                }
                if (c->callee == "file_read_checked") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.call("__nx_file_read_checked");
                    return;
                }
                if (c->callee == "file_write") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.call("__nx_file_write");
                    return;
                }
                if (c->callee == "mem_alloc") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_mem_alloc");
                    return;
                }
                if (c->callee == "mem_free") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_mem_free");
                    return;
                }
                if (c->callee == "mem_read") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_mem_read");
                    return;
                }
                if (c->callee == "mem_set") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.pushRax();
                    genExpr(c->args[2].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.popRax();
                    out_.call("__nx_mem_set");
                    return;
                }
                if (c->callee == "mem_copy") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.pushRax();
                    genExpr(c->args[2].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.popRax();
                    out_.call("__nx_mem_copy");
                    return;
                }
                if (c->callee == "str_f64") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_str_f64");
                    return;
                }
                if (c->callee == "input_f64") {
                    genExpr(c->args[0].get());
                    out_.call("__nx_input_f64");
                    return;
                }
                if (c->callee == "to_f64") {
                    genExpr(c->args[0].get());
                    out_.cvtsi2sdXmm0Rax();
                    return;
                }
                if (c->callee == "len") {
                    const Type at = exprType(c->args[0].get());
                    if (at.kind == TypeKind::Array) {
                        out_.movRaxImm64(at.arraySize);
                        return;
                    }
                    genExpr(c->args[0].get());
                    out_.call("__nx_string_len");
                    return;
                }
                if (c->callee == "to_i64") {
                    genExpr(c->args[0].get());
                    out_.cvttsd2siRaxXmm0();
                    return;
                }
                // f64 math builtins: unary helpers take xmm0 in/out;
                // atan2/pow take raw f64 bits pushed left-to-right.
                if (c->callee == "sqrt" || c->callee == "sin" || c->callee == "cos" ||
                    c->callee == "tan" || c->callee == "floor" || c->callee == "ceil") {
                    genExpr(c->args[0].get());
                    out_.call(c->callee == "sqrt"    ? "__nx_math_sqrt"
                              : c->callee == "sin"   ? "__nx_math_sin"
                              : c->callee == "cos"   ? "__nx_math_cos"
                              : c->callee == "tan"   ? "__nx_math_tan"
                              : c->callee == "floor" ? "__nx_math_floor"
                                                     : "__nx_math_ceil");
                    return;
                }
                if (c->callee == "atan2" || c->callee == "pow") {
                    for (const auto& a : c->args) {
                        genExpr(a.get());
                        out_.movqRaxXmm0();
                        out_.pushRax();
                    }
                    out_.call(c->callee == "atan2" ? "__nx_math_atan2" : "__nx_math_pow");
                    out_.addRsp(static_cast<std::uint32_t>(8 * c->args.size()));
                    return;
                }
                // Physics predicates: all-f64 args pushed as raw bits,
                // bool result in rax.
                if (c->callee == "physics_aabb" || c->callee == "physics_circle") {
                    for (const auto& a : c->args) {
                        genExpr(a.get());
                        out_.movqRaxXmm0();
                        out_.pushRax();
                    }
                    out_.call(c->callee == "physics_aabb" ? "__nx_physics_aabb"
                                                          : "__nx_physics_circle");
                    out_.addRsp(static_cast<std::uint32_t>(8 * c->args.size()));
                    return;
                }
                // Growable list builtins (ilist_*/slist_*): helpers take
                // rax = handle, rcx = type tag (1 = int, 2 = string),
                // rdx = arg1, r8 = arg2.
                const std::string& lc = c->callee;
                const bool isI = lc.rfind("ilist_", 0) == 0;
                const bool isS = !isI && lc.rfind("slist_", 0) == 0;
                if (isI || isS) {
                    const std::int64_t tag = isI ? 1 : 2;
                    const std::string op = lc.substr(6);
                    const auto tagReg = [&] { out_.movRcxImm64(tag); };
                    if (op == "new") {
                        out_.movRaxImm64(tag);
                        out_.call("__nx_lc_new");
                        return;
                    }
                    if (op == "len" || op == "clear" || op == "free") {
                        genExpr(c->args[0].get());
                        tagReg();
                        out_.call(op == "len"     ? "__nx_lc_len"
                                  : op == "clear" ? "__nx_lc_clear"
                                                  : "__nx_lc_free");
                        return;
                    }
                    if (op == "push" || op == "push_checked" || op == "get" || op == "reserve") {
                        genExpr(c->args[0].get());
                        out_.pushRax();
                        genExpr(c->args[1].get());
                        out_.movRdxRax();
                        out_.popRax();
                        tagReg();
                        out_.call(op == "push"           ? "__nx_lc_push"
                                  : op == "push_checked" ? "__nx_lc_push_checked"
                                  : op == "get"          ? "__nx_lc_get"
                                                         : "__nx_lc_reserve");
                        return;
                    }
                    if (op == "try_pop") {
                        genExpr(c->args[0].get());
                        out_.pushRax();
                        genExpr(c->args[1].get());
                        out_.movR8Rax();
                        out_.popRax();
                        tagReg();
                        out_.call("__nx_lc_try_pop");
                        return;
                    }
                    // set / set_checked / insert / try_get / remove:
                    // (handle, index, value|out)
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.pushRax();
                    genExpr(c->args[2].get());
                    out_.movR8Rax();
                    out_.popRcx();
                    out_.movRdxRcx();
                    out_.popRax();
                    tagReg();
                    out_.call(op == "set"           ? "__nx_lc_set"
                              : op == "set_checked" ? "__nx_lc_set_checked"
                              : op == "insert"      ? "__nx_lc_insert"
                              : op == "try_get"     ? "__nx_lc_try_get"
                                                    : "__nx_lc_remove");
                    return;
                }
                // UTF-8 string builtins: helpers take rax = arg0,
                // rcx = arg1, rdx = arg2.
                const std::string& sc = c->callee;
                if (sc == "string.chars" || sc == "string.valid_utf8" || sc == "string.trim" ||
                    sc == "string.upper" || sc == "string.lower" || sc == "string.from_char") {
                    genExpr(c->args[0].get());
                    out_.call(sc == "string.chars"        ? "__nx_str_chars"
                              : sc == "string.valid_utf8" ? "__nx_str_valid_utf8"
                              : sc == "string.trim"       ? "__nx_str_trim"
                              : sc == "string.upper"      ? "__nx_str_upper"
                              : sc == "string.lower"      ? "__nx_str_lower"
                                                          : "__nx_str_from_char");
                    return;
                }
                if (sc == "string.char_at" || sc == "string.contains" || sc == "string.find" ||
                    sc == "string.split_count") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.movRcxRax();
                    out_.popRax();
                    out_.call(sc == "string.char_at"    ? "__nx_str_char_at"
                              : sc == "string.contains" ? "__nx_str_contains"
                              : sc == "string.find"     ? "__nx_str_find"
                                                        : "__nx_str_split_count");
                    return;
                }
                if (sc == "string.slice" || sc == "string.split_at") {
                    genExpr(c->args[0].get());
                    out_.pushRax();
                    genExpr(c->args[1].get());
                    out_.pushRax();
                    genExpr(c->args[2].get());
                    out_.movRdxRax();
                    out_.popRcx();
                    out_.popRax();
                    out_.call(sc == "string.slice" ? "__nx_str_slice" : "__nx_str_split_at");
                    return;
                }
                for (std::size_t ai = 0; ai < c->args.size(); ++ai) {
                    genExpr(c->args[ai].get());
                    const Kind ak = exprKind(c->args[ai].get());
                    if (ak == Kind::F64)
                        out_.movqRaxXmm0(); // pass the raw double bits on the stack
                    else if (ak == Kind::Ptr) {
                        // Aggregate arguments are copied so the callee cannot
                        // mutate the caller's block (value semantics).
                        const auto sit = sigs_.find(c->callee);
                        if (sit != sigs_.end() && ai < sit->second.params.size())
                            genDupValue(sit->second.params[ai]);
                    }
                    out_.pushRax();
                }
                out_.call(c->callee);
                if (!c->args.empty())
                    out_.addRsp(static_cast<std::uint32_t>(8 * c->args.size()));
                return;
            }
            if (const auto* arr = dynamic_cast<const ArrayExpr*>(e)) {
                const std::size_t n = arr->elements.size();
                out_.movRaxImm64(n * 8);
                out_.call("__nx_alloc");
                out_.pushRax(); // base stays at [rsp] across element evaluation
                for (std::size_t i = 0; i < n; ++i) {
                    genExpr(arr->elements[i].get());
                    out_.movRcxFromRsp();
                    if (i > 0)
                        out_.addRcxImm32(static_cast<std::uint32_t>(i * 8));
                    genStoreToRcx(kindOf(exprType(arr->elements[i].get())));
                }
                out_.popRax();
                return;
            }
            if (const auto* x = dynamic_cast<const IndexExpr*>(e)) {
                const Type ot = exprType(x->object.get());
                genExpr(x->object.get());
                out_.pushRax();
                genExpr(x->index.get());
                genIndexCheck(ot.arraySize);
                out_.movRdxRax();
                out_.shlRdxImm8(3);
                out_.popRcx();
                out_.addRcxRdx();
                genLoadFromRcx(kindOf(*ot.element));
                return;
            }
            if (const auto* m = dynamic_cast<const MemberExpr*>(e)) {
                const Type ot = exprType(m->object.get());
                genExpr(m->object.get());
                out_.movRcxRax();
                const int fi = structFieldIndex(ot.name, m->member);
                const std::int64_t off = static_cast<std::int64_t>(fi) * 8;
                if (off > 0)
                    out_.addRcxImm32(static_cast<std::uint32_t>(off));
                const StructDecl* decl = nullptr;
                const auto sit = structs_.find(ot.name);
                if (sit != structs_.end())
                    decl = sit->second;
                const Kind fk = decl && fi >= 0
                                    ? kindOf(decl->fields[static_cast<std::size_t>(fi)].type)
                                    : Kind::I64;
                genLoadFromRcx(fk);
                return;
            }
            if (const auto* s = dynamic_cast<const StructInitExpr*>(e)) {
                const auto sit = structs_.find(s->typeName);
                if (sit == structs_.end())
                    throw std::runtime_error("builtin native backend: unknown struct '" +
                                             s->typeName + "'");
                const StructDecl* decl = sit->second;
                out_.movRaxImm64(decl->fields.size() * 8);
                out_.call("__nx_alloc");
                out_.pushRax();
                for (const auto& [fname, fexpr] : s->fields) {
                    const int fi = structFieldIndex(s->typeName, fname);
                    if (fi < 0)
                        throw std::runtime_error("builtin native backend: struct '" + s->typeName +
                                                 "' has no member '" + fname + "'");
                    genExpr(fexpr.get());
                    out_.movRcxFromRsp();
                    const std::int64_t off = static_cast<std::int64_t>(fi) * 8;
                    if (off > 0)
                        out_.addRcxImm32(static_cast<std::uint32_t>(off));
                    genStoreToRcx(kindOf(decl->fields[static_cast<std::size_t>(fi)].type));
                }
                out_.popRax();
                return;
            }
            throw std::runtime_error("builtin native v0: unsupported expression reached codegen");
        }

        void Generator::genForCollection(const ForStmt* f) {
            const Type ct = exprType(f->collection.get());
            const Type et = ct.element ? *ct.element : Type::i64();
            const Kind ek = kindOf(et);
            const auto& names = forIterNames_.at(f);
            const std::string& ptrName = names.first;
            const std::string& idxName = names.second;
            const std::string cond = out_.label("forc.cond");
            const std::string body = out_.label("forc.body");
            const std::string incr = out_.label("forc.incr");
            const std::string end = out_.label("forc.end");

            // Evaluate the collection once; the pointer and index live in
            // hidden slots for the loop's duration.
            genExpr(f->collection.get());
            out_.movRbpFromRax(slotOff(slots_.at(ptrName)));
            out_.xorEax();
            out_.movRbpFromRax(slotOff(slots_.at(idxName)));

            out_.jmp(cond);
            out_.def(cond);
            out_.movRaxFromRbp(slotOff(slots_.at(idxName)));
            out_.cmpRaxImm32(static_cast<std::uint32_t>(ct.arraySize));
            out_.jcc(J_AE, end);
            out_.def(body);
            // &elem = ptr + idx*8
            out_.movRcxFromRbp(slotOff(slots_.at(ptrName)));
            out_.movRdxFromRbp(slotOff(slots_.at(idxName)));
            out_.shlRdxImm8(3);
            out_.addRcxRdx();
            if (ek == Kind::F64) {
                out_.movsdXmm0FromRcx();
                out_.movsdXmm0ToRbp(slotOff(slots_.at(f->name)));
            } else {
                out_.movRaxFromRcx();
                if (ek == Kind::Ptr)
                    genDupValue(et); // value semantics for aggregate elements
                out_.movRbpFromRax(slotOff(slots_.at(f->name)));
            }
            breakStack_.push_back(end);
            continueStack_.push_back(incr);
            genBlock(f->body.get());
            breakStack_.pop_back();
            continueStack_.pop_back();

            out_.def(incr);
            out_.movRaxFromRbp(slotOff(slots_.at(idxName)));
            out_.addRaxImm8(1);
            out_.movRbpFromRax(slotOff(slots_.at(idxName)));
            out_.jmp(cond);
            out_.def(end);
        }

        void Generator::genPrint(const Expr* arg) {
            if (const auto* s = dynamic_cast<const StringExpr*>(arg)) {
                // Fast path: the literal plus its newline is emitted once.
                const std::string withNewline = s->value + "\n";
                const std::size_t id = internString(withNewline);
                out_.leaRcxRip("str" + std::to_string(id));
                out_.movRdxImm32(static_cast<std::uint32_t>(withNewline.size()));
                out_.call("__nx_write");
                return;
            }
            genExpr(arg);
            if (exprIsString(arg))
                out_.call("__nx_write_cstr");
            else if (exprIsBool(arg))
                out_.call("__nx_print_bool");
            else if (exprIsF64(arg))
                out_.call("__nx_print_f64");
            else
                out_.call("__nx_print_i64");
        }

        // True for string-typed expressions (literals, vars/params, member and
        // index accesses, concatenation, and the string builtins).
        bool Generator::exprIsString(const Expr* e) {
            return exprType(e).kind == TypeKind::String;
        }

        bool Generator::exprIsF64(const Expr* e) {
            return exprKind(e) == Kind::F64;
        }

        Type Generator::exprType(const Expr* e) {
            if (dynamic_cast<const IntExpr*>(e))
                return Type::i64();
            if (dynamic_cast<const FloatExpr*>(e))
                return Type::f64();
            if (dynamic_cast<const BoolExpr*>(e))
                return Type::boolean();
            if (dynamic_cast<const StringExpr*>(e))
                return Type::string();
            if (const auto* v = dynamic_cast<const VarExpr*>(e))
                return localType(v->name);
            if (const auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                if (u->op == TokenKind::Bang)
                    return Type::boolean();
                if (u->op == TokenKind::Ampersand) {
                    const Type t = exprType(u->rhs.get());
                    if (t.kind == TypeKind::Void)
                        return Type::void_();
                    return Type::ptr(t);
                }
                if (u->op == TokenKind::Star) {
                    const Type t = exprType(u->rhs.get());
                    if (t.kind == TypeKind::Pointer && t.pointee)
                        return *t.pointee;
                    return Type::void_();
                }
                return exprType(u->rhs.get());
            }
            if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                switch (b->op) {
                case TokenKind::EqualEqual:
                case TokenKind::BangEqual:
                case TokenKind::Less:
                case TokenKind::LessEqual:
                case TokenKind::Greater:
                case TokenKind::GreaterEqual:
                case TokenKind::AndAnd:
                case TokenKind::OrOr:
                    return Type::boolean();
                case TokenKind::Plus:
                    if (exprIsString(b->lhs.get()))
                        return Type::string();
                    return exprType(b->lhs.get());
                default:
                    return exprType(b->lhs.get());
                }
            }
            if (const auto* c = dynamic_cast<const CallExpr*>(e)) {
                if (c->callee == "print" || c->callee == "exit" || c->callee == "sleep")
                    return Type::void_();
                if (c->callee == "len" || c->callee == "os.args_count" || c->callee == "read_key" ||
                    c->callee == "string.len" || c->callee == "string.byte_at" ||
                    c->callee == "string.chars" || c->callee == "string.char_at" ||
                    c->callee == "string.find" || c->callee == "string.split_count" ||
                    c->callee == "time_ms" || c->callee == "random_i64" || c->callee == "to_i64")
                    return Type::i64();
                if (c->callee == "os.args_get" || c->callee == "input" || c->callee == "str_i64" ||
                    c->callee == "str_bool" || c->callee == "str_f64" ||
                    c->callee == "string.trim" || c->callee == "string.upper" ||
                    c->callee == "string.lower" || c->callee == "string.slice" ||
                    c->callee == "string.split_at" || c->callee == "string.from_char")
                    return Type::string();
                if (c->callee == "file_exists" || c->callee == "file_read_checked" ||
                    c->callee == "key_pressed" || c->callee == "string.valid_utf8" ||
                    c->callee == "string.contains" || c->callee == "physics_aabb" ||
                    c->callee == "physics_circle")
                    return Type::boolean();
                if (c->callee == "env" || c->callee == "file_read")
                    return Type::string();
                if (c->callee == "file_write" || c->callee == "mem_alloc" ||
                    c->callee == "mem_read")
                    return Type::i64();
                if (c->callee == "mem_free" || c->callee == "mem_set" || c->callee == "mem_copy")
                    return Type::void_();
                if (c->callee == "input_f64" || c->callee == "to_f64" || c->callee == "sqrt" ||
                    c->callee == "sin" || c->callee == "cos" || c->callee == "tan" ||
                    c->callee == "floor" || c->callee == "ceil" || c->callee == "atan2" ||
                    c->callee == "pow")
                    return Type::f64();
                // ilist_*/slist_* mirrors builtins_collections.hpp.
                const std::string& cc = c->callee;
                const bool ci = cc.rfind("ilist_", 0) == 0;
                const bool cs = cc.rfind("slist_", 0) == 0;
                if (ci || cs) {
                    const std::string op = cc.substr(6);
                    if (op == "get")
                        return cs ? Type::string() : Type::i64();
                    if (op == "new" || op == "len")
                        return Type::i64();
                    if (op == "push" || op == "set")
                        return Type::void_();
                    return Type::boolean();
                }
                const auto it = sigs_.find(c->callee);
                if (it != sigs_.end())
                    return it->second.ret;
                return Type::void_();
            }
            if (const auto* a = dynamic_cast<const ArrayExpr*>(e)) {
                if (a->elements.empty())
                    return Type::void_();
                return Type::array(exprType(a->elements[0].get()), a->elements.size());
            }
            if (const auto* x = dynamic_cast<const IndexExpr*>(e)) {
                const Type ot = exprType(x->object.get());
                if (ot.kind == TypeKind::Array && ot.element)
                    return *ot.element;
                return Type::void_();
            }
            if (const auto* m = dynamic_cast<const MemberExpr*>(e)) {
                const Type ot = exprType(m->object.get());
                if (ot.kind != TypeKind::Struct)
                    return Type::void_();
                const int fi = structFieldIndex(ot.name, m->member);
                const auto sit = structs_.find(ot.name);
                if (fi < 0 || sit == structs_.end())
                    return Type::void_();
                return sit->second->fields[static_cast<std::size_t>(fi)].type;
            }
            if (const auto* s = dynamic_cast<const StructInitExpr*>(e))
                return Type::named(s->typeName);
            return Type::void_();
        }

        Kind Generator::exprKind(const Expr* e) {
            return kindOf(exprType(e));
        }

        // Mirrors the checker's kind rules to select the right print helper.
        bool Generator::exprIsBool(const Expr* e) {
            return exprKind(e) == Kind::Bool;
        }

        Type Generator::localType(const std::string& name) {
            const auto scratch = scratchTypes_.find(name);
            if (scratch != scratchTypes_.end())
                return scratch->second;
            if (currentFn_) {
                for (const auto& p : currentFn_->params)
                    if (p.name == name)
                        return p.type;
                Type found = Type::void_();
                bool ok = false;
                findLet(currentFn_->body.get(), name, found, ok);
                if (ok)
                    return found;
            }
            return Type::i64();
        }

        Kind Generator::localKind(const std::string& name) {
            return kindOf(localType(name));
        }

        void Generator::findLet(const Block* b, const std::string& name, Type& found, bool& ok) {
            for (const auto& s : b->statements) {
                if (ok)
                    return;
                if (const auto* nested = dynamic_cast<const Block*>(s.get())) {
                    findLet(nested, name, found, ok);
                } else if (const auto* l = dynamic_cast<const LetStmt*>(s.get())) {
                    if (l->name == name) {
                        found = initType(l->init.get());
                        ok = true;
                    }
                } else if (const auto* i = dynamic_cast<const IfStmt*>(s.get())) {
                    findLet(i->thenBlock.get(), name, found, ok);
                    if (!ok && i->elseBlock)
                        findLet(i->elseBlock.get(), name, found, ok);
                } else if (const auto* w = dynamic_cast<const WhileStmt*>(s.get())) {
                    findLet(w->body.get(), name, found, ok);
                } else if (const auto* f = dynamic_cast<const ForStmt*>(s.get())) {
                    if (f->name == name) {
                        if (f->collection) {
                            const Type ct = exprType(f->collection.get());
                            if (ct.kind == TypeKind::Array && ct.element) {
                                found = *ct.element;
                                ok = true;
                            }
                        } else {
                            found = Type::i64();
                            ok = true;
                        }
                    }
                    if (!ok)
                        findLet(f->body.get(), name, found, ok);
                }
            }
        }

        Type Generator::initType(const Expr* e) {
            return exprType(e);
        }

        void Generator::genLoadFromRcx(Kind k) {
            if (k == Kind::F64)
                out_.movsdXmm0FromRcx();
            else
                out_.movRaxFromRcx();
        }

        void Generator::genStoreToRcx(Kind k) {
            if (k == Kind::F64)
                out_.movsdXmm0ToRcx();
            else
                out_.movToRcxFromRax();
        }

        void Generator::genIndexCheck(std::size_t count) {
            const std::string ok = out_.label("idx.ok");
            out_.cmpRaxImm32(static_cast<std::uint32_t>(count));
            out_.jcc(J_B, ok); // unsigned below: 0 <= index < count
            out_.jmp("__nx_oob");
            out_.def(ok);
        }

        bool Generator::needsDup(const Expr* e) {
            if (exprKind(e) != Kind::Ptr)
                return false;
            return !dynamic_cast<const ArrayExpr*>(e) && !dynamic_cast<const StructInitExpr*>(e);
        }

        int Generator::structFieldIndex(const std::string& typeName, const std::string& member) {
            const auto it = structs_.find(typeName);
            if (it == structs_.end())
                return -1;
            const StructDecl* decl = it->second;
            for (std::size_t i = 0; i < decl->fields.size(); ++i)
                if (decl->fields[i].name == member)
                    return static_cast<int>(i);
            return -1;
        }

        Type Generator::aggregateFieldType(const Type& t, std::size_t i) {
            if (t.kind == TypeKind::Array && t.element)
                return *t.element;
            if (t.kind == TypeKind::Struct) {
                const auto it = structs_.find(t.name);
                if (it != structs_.end() && i < it->second->fields.size())
                    return it->second->fields[i].type;
            }
            return Type::void_();
        }

        void Generator::genCompoundAssign(const Stmt* stmt,
                                          const Expr* value,
                                          const Type& targetType,
                                          TokenKind bop) {
            // [rsp] = &target. Load current into a scratch slot, evaluate the
            // rhs into a second scratch slot, then reuse the binary operator
            // codegen (which already handles f64, div-by-zero, string concat).
            const auto& pr = compoundNames_.at(stmt);
            const Kind k = kindOf(targetType);
            scratchTypes_[pr.first] = targetType;
            scratchTypes_[pr.second] = targetType;
            out_.movRcxFromRsp();
            genLoadFromRcx(k);
            if (k == Kind::F64)
                out_.movsdXmm0ToRbp(slotOff(slots_.at(pr.first)));
            else
                out_.movRbpFromRax(slotOff(slots_.at(pr.first)));
            genExpr(value);
            if (k == Kind::F64)
                out_.movsdXmm0ToRbp(slotOff(slots_.at(pr.second)));
            else
                out_.movRbpFromRax(slotOff(slots_.at(pr.second)));
            BinaryExpr combined(
                bop, std::make_unique<VarExpr>(pr.first), std::make_unique<VarExpr>(pr.second));
            genExpr(&combined);
            out_.movRcxFromRsp();
            genStoreToRcx(k);
            out_.addRsp(8); // drop the saved address
        }

        void Generator::genDupValue(const Type& t) {
            // rax = source aggregate pointer -> rax = fresh deep copy.
            // Stack layout inside the loop: [rsp] = dst, [rsp+8] = src.
            const std::size_t n = aggregateSlots(t, structs_);
            out_.pushRax(); // src
            out_.movRaxImm64(n * 8);
            out_.call("__nx_alloc");
            out_.pushRax(); // dst
            for (std::size_t i = 0; i < n; ++i) {
                const Type ft = aggregateFieldType(t, i);
                const bool nested = ft.kind == TypeKind::Array || ft.kind == TypeKind::Struct;
                out_.movRcxFromRspOff(8); // src base
                out_.movRaxFromRcxOff(static_cast<std::int64_t>(i * 8));
                if (nested) {
                    genDupValue(ft); // rax = deep copy of the inner block
                }
                out_.movRcxFromRsp(); // dst base
                if (i > 0)
                    out_.addRcxImm32(static_cast<std::uint32_t>(i * 8));
                out_.movToRcxFromRax();
            }
            out_.popRax();  // dst
            out_.addRsp(8); // drop src
        }

        void Generator::genAddr(const Expr* e) {
            // Leaves the address of the lvalue's 8-byte slot in rax.
            if (const auto* v = dynamic_cast<const VarExpr*>(e)) {
                out_.leaRaxFromRbp(slotOff(slots_.at(v->name)));
                return;
            }
            if (const auto* m = dynamic_cast<const MemberExpr*>(e)) {
                // &obj.field = (struct block pointer) + field*8. The object
                // expression evaluates to the block pointer like a load does.
                const Type ot = exprType(m->object.get());
                genExpr(m->object.get());
                out_.movRcxRax();
                const int fi = structFieldIndex(ot.name, m->member);
                const std::int64_t off = static_cast<std::int64_t>(fi) * 8;
                if (off > 0)
                    out_.addRcxImm32(static_cast<std::uint32_t>(off));
                out_.movRaxRcx();
                return;
            }
            if (const auto* x = dynamic_cast<const IndexExpr*>(e)) {
                const Type ot = exprType(x->object.get());
                genExpr(x->object.get());
                out_.pushRax();
                genExpr(x->index.get());
                genIndexCheck(ot.arraySize);
                out_.movRdxRax();
                out_.shlRdxImm8(3);
                out_.popRcx();
                out_.addRcxRdx();
                out_.movRaxRcx();
                return;
            }
            if (const auto* u = dynamic_cast<const UnaryExpr*>(e); u && u->op == TokenKind::Star) {
                // &*p == p: the pointer value already is the address.
                genExpr(u->rhs.get());
                return;
            }
            throw std::runtime_error(
                "builtin native v0: address-of on a non-lvalue reached codegen");
        }

        // Call the shared write helper. r10/r11 do NOT survive OS calls
        // (the syscall instruction itself clobbers rcx+r11, and ntdll
        // trampolines clobber r10), so the stack pointer is sheltered in the
        // callee-saved rbx instead. Nothing else in the stubs uses rbx.
        void Generator::stubCall(const std::string& target) {
            if (target_ == NativeTarget::WindowsX64) {
                out_.pushRbx();
                out_.movRbxRsp();
                out_.andRspMinus16();
                out_.subRsp(32);
                out_.call(target);
                out_.movRspRbx();
                out_.popRbx();
            } else {
                out_.call(target);
            }
        }

    } // namespace detail
} // namespace npp::native
