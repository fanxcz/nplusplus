#include "native/native_internal.hpp"

#include "lexer/token.hpp"

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace npp::native {
    namespace detail {

        namespace {
            constexpr const char* kSubset = "builtin native subset";

            bool isScalar(const Type& t) {
                return t.kind == TypeKind::I64 || t.kind == TypeKind::Bool ||
                       t.kind == TypeKind::String || t.kind == TypeKind::F64;
            }

            Type voidT() {
                return Type::void_();
            }
            Type i64T() {
                return Type::i64();
            }
            Type boolT() {
                return Type::boolean();
            }
            Type strT() {
                return Type::string();
            }
            Type f64T() {
                return Type::f64();
            }

            const StructDecl* findStruct(const std::map<std::string, const StructDecl*>& structs,
                                         const std::string& name) {
                const auto it = structs.find(name);
                return it == structs.end() ? nullptr : it->second;
            }

            const StructField* findField(const StructDecl& decl, const std::string& name) {
                for (const auto& f : decl.fields) {
                    if (f.name == name)
                        return &f;
                }
                return nullptr;
            }

            // Scalar type combinations allowed for `lhs <binop>= rhs`.
            bool compoundTypeOk(TokenKind bop, const Type& lt, const Type& rt) {
                if (lt.kind == TypeKind::I64 && rt.kind == TypeKind::I64)
                    return bop == TokenKind::Plus || bop == TokenKind::Minus ||
                           bop == TokenKind::Star || bop == TokenKind::Slash ||
                           bop == TokenKind::Percent || bop == TokenKind::Ampersand ||
                           bop == TokenKind::Pipe || bop == TokenKind::Caret ||
                           bop == TokenKind::ShiftLeft || bop == TokenKind::ShiftRight;
                if (lt.kind == TypeKind::F64 && rt.kind == TypeKind::F64)
                    return bop == TokenKind::Plus || bop == TokenKind::Minus ||
                           bop == TokenKind::Star || bop == TokenKind::Slash;
                if (lt.kind == TypeKind::String && rt.kind == TypeKind::String)
                    return bop == TokenKind::Plus;
                return false;
            }
        } // namespace

        bool supportedType(const Type& t,
                           const std::map<std::string, const StructDecl*>& structs,
                           std::string& reason,
                           std::size_t depth) {
            if (depth > 32) {
                reason = "recursive or excessively nested aggregate type is outside the " +
                         std::string(kSubset) + " (use --backend llvm)";
                return false;
            }
            switch (t.kind) {
            case TypeKind::I64:
            case TypeKind::Bool:
            case TypeKind::String:
            case TypeKind::F64:
                return true;
            case TypeKind::Array:
                if (!t.element || t.arraySize == 0) {
                    reason = "empty or malformed array type is outside the " +
                             std::string(kSubset) + " (use --backend llvm)";
                    return false;
                }
                return supportedType(*t.element, structs, reason, depth + 1);
            case TypeKind::Struct: {
                const StructDecl* decl = findStruct(structs, t.name);
                if (!decl) {
                    reason = "struct '" + t.name + "' is outside the " + std::string(kSubset) +
                             " (undeclared or unsupported; use --backend "
                             "llvm)";
                    return false;
                }
                for (const auto& f : decl->fields) {
                    if (!supportedType(f.type, structs, reason, depth + 1)) {
                        reason = "field '" + f.name + "' of struct '" + t.name + "': " + reason;
                        return false;
                    }
                }
                return true;
            }
            default:
                reason = "type '" + std::to_string(static_cast<int>(t.kind)) + "' is outside the " +
                         std::string(kSubset) +
                         " (pointers, slices, enums, and mutable references need --backend "
                         "llvm)";
                return false;
            }
        }

        std::size_t aggregateSlots(const Type& t,
                                   const std::map<std::string, const StructDecl*>& structs) {
            if (t.kind == TypeKind::Array)
                return t.arraySize;
            if (t.kind == TypeKind::Struct) {
                const StructDecl* decl = findStruct(structs, t.name);
                return decl ? decl->fields.size() : 0;
            }
            return 0;
        }

        Kind kindOf(const Type& t) {
            switch (t.kind) {
            case TypeKind::I64:
                return Kind::I64;
            case TypeKind::Bool:
                return Kind::Bool;
            case TypeKind::String:
                return Kind::Str;
            case TypeKind::F64:
                return Kind::F64;
            case TypeKind::Array:
            case TypeKind::Struct:
                return Kind::Ptr;
            default:
                return Kind::Void;
            }
        }

        Type checkCall(const CallExpr* c,
                       const std::map<std::string, Type>& env,
                       const std::map<std::string, FnSig>& fns,
                       const std::map<std::string, const StructDecl*>& structs,
                       std::string& reason) {
            if (c->callee == "print") {
                if (c->args.size() != 1) {
                    reason = "print() with " + std::to_string(c->args.size()) +
                             " arguments is outside the builtin native v0 subset (exactly 1 "
                             "argument is supported)";
                    return voidT();
                }
                const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                if (!reason.empty())
                    return voidT();
                if (!isScalar(a)) {
                    reason = "print() of this type is outside the builtin native v0 subset "
                             "(only i64, bool, string, and f64 are printable)";
                    return voidT();
                }
                return voidT();
            }
            if (c->callee == "len") {
                if (c->args.size() != 1) {
                    reason = "len() takes exactly 1 argument";
                    return i64T();
                }
                const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                if (!reason.empty())
                    return i64T();
                if (a.kind == TypeKind::String || a.kind == TypeKind::Array)
                    return i64T();
                reason = "len() expects a string or fixed-size array";
                return i64T();
            }
            const auto it = fns.find(c->callee);
            if (it == fns.end()) {
                // Narrow single-purpose builtins with inline runtime helpers.
                if (c->callee == "str_i64" || c->callee == "str_f64" || c->callee == "str_bool") {
                    const TypeKind want = c->callee == "str_i64"   ? TypeKind::I64
                                          : c->callee == "str_f64" ? TypeKind::F64
                                                                   : TypeKind::Bool;
                    if (c->args.size() != 1) {
                        reason = c->callee + "() takes exactly 1 argument";
                        return strT();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return strT();
                    if (a.kind != want) {
                        reason = c->callee + "() argument type is not supported here";
                        return strT();
                    }
                    return strT();
                }
                if (c->callee == "input" || c->callee == "input_i64" || c->callee == "input_f64") {
                    const TypeKind ret = c->callee == "input"       ? TypeKind::String
                                         : c->callee == "input_i64" ? TypeKind::I64
                                                                    : TypeKind::F64;
                    if (c->args.size() != 1) {
                        reason = c->callee + "() takes exactly 1 argument (the prompt)";
                        return ret == TypeKind::String ? strT()
                               : ret == TypeKind::I64  ? i64T()
                                                       : f64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return ret == TypeKind::String ? strT()
                               : ret == TypeKind::I64  ? i64T()
                                                       : f64T();
                    if (a.kind != TypeKind::String) {
                        reason = c->callee + "() takes a string prompt";
                        return ret == TypeKind::String ? strT()
                               : ret == TypeKind::I64  ? i64T()
                                                       : f64T();
                    }
                    return ret == TypeKind::String ? strT()
                           : ret == TypeKind::I64  ? i64T()
                                                   : f64T();
                }
                if (c->callee == "os.args_count" || c->callee == "time_ms") {
                    if (c->args.size() != 0) {
                        reason = c->callee + "() takes no arguments";
                        return i64T();
                    }
                    return i64T();
                }
                if (c->callee == "os.args_get") {
                    if (c->args.size() != 1) {
                        reason = "os.args_get() takes exactly 1 argument (index)";
                        return strT();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return strT();
                    if (a.kind != TypeKind::I64) {
                        reason = "os.args_get() takes an i64 index";
                        return strT();
                    }
                    return strT();
                }
                if (c->callee == "string.len") {
                    if (c->args.size() != 1) {
                        reason = "string.len() takes exactly 1 argument";
                        return i64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    if (a.kind != TypeKind::String) {
                        reason = "string.len() takes a string argument";
                        return i64T();
                    }
                    return i64T();
                }
                if (c->callee == "string.byte_at") {
                    if (c->args.size() != 2) {
                        reason = "string.byte_at() takes exactly 2 arguments (string, index)";
                        return i64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    if (a.kind != TypeKind::String) {
                        reason = "string.byte_at() first argument must be a string";
                        return i64T();
                    }
                    const Type b = checkExpr(c->args[1].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    if (b.kind != TypeKind::I64) {
                        reason = "string.byte_at() second argument must be an i64 index";
                        return i64T();
                    }
                    return i64T();
                }
                if (c->callee == "exit" || c->callee == "sleep") {
                    if (c->args.size() != 1) {
                        reason = c->callee + "() takes exactly 1 argument (i64)";
                        return voidT();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return voidT();
                    if (a.kind != TypeKind::I64) {
                        reason = c->callee + "() takes an i64 argument";
                        return voidT();
                    }
                    return voidT();
                }
                if (c->callee == "random_i64") {
                    if (c->args.size() != 2) {
                        reason = "random_i64() takes exactly 2 arguments (min, max)";
                        return i64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    const Type b = checkExpr(c->args[1].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    if (a.kind != TypeKind::I64 || b.kind != TypeKind::I64) {
                        reason = "random_i64() takes two i64 arguments";
                        return i64T();
                    }
                    return i64T();
                }
                if (c->callee == "to_f64") {
                    if (c->args.size() != 1) {
                        reason = "to_f64() takes exactly 1 argument (i64)";
                        return f64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return f64T();
                    if (a.kind != TypeKind::I64) {
                        reason = "to_f64() takes an i64 argument";
                        return f64T();
                    }
                    return f64T();
                }
                if (c->callee == "to_i64") {
                    if (c->args.size() != 1) {
                        reason = "to_i64() takes exactly 1 argument (f64)";
                        return i64T();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return i64T();
                    if (a.kind != TypeKind::F64) {
                        reason = "to_i64() takes an f64 argument";
                        return i64T();
                    }
                    return i64T();
                }
                if (c->callee == "file_exists") {
                    if (c->args.size() != 1) {
                        reason = "file_exists() takes exactly 1 argument (path)";
                        return boolT();
                    }
                    const Type a = checkExpr(c->args[0].get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return boolT();
                    if (a.kind != TypeKind::String) {
                        reason = "file_exists() takes a string path";
                        return boolT();
                    }
                    return boolT();
                }
                reason = "call to '" + c->callee +
                         "' is outside the builtin native v0 subset (unknown function or "
                         "unsupported builtin; use --backend llvm)";
                return voidT();
            }
            if (c->args.size() != it->second.params.size()) {
                reason = "call to '" + c->callee + "' has the wrong argument count";
                return voidT();
            }
            for (std::size_t i = 0; i < c->args.size(); ++i) {
                const Type a = checkExpr(c->args[i].get(), env, fns, structs, reason);
                if (!reason.empty())
                    return voidT();
                if (!sameType(a, it->second.params[i])) {
                    reason = "call to '" + c->callee +
                             "' has an argument of an unsupported "
                             "or mismatched type";
                    return voidT();
                }
            }
            return it->second.ret;
        }

        Type checkExpr(const Expr* e,
                       const std::map<std::string, Type>& env,
                       const std::map<std::string, FnSig>& fns,
                       const std::map<std::string, const StructDecl*>& structs,
                       std::string& reason) {
            if (dynamic_cast<const IntExpr*>(e))
                return i64T();
            if (dynamic_cast<const BoolExpr*>(e))
                return boolT();
            if (dynamic_cast<const StringExpr*>(e))
                return strT();
            if (dynamic_cast<const FloatExpr*>(e))
                return f64T();
            if (const auto* v = dynamic_cast<const VarExpr*>(e)) {
                const auto it = env.find(v->name);
                if (it == env.end()) {
                    reason = "variable '" + v->name + "' is outside the builtin native v0 subset";
                    return Type::void_();
                }
                return it->second;
            }
            if (const auto* u = dynamic_cast<const UnaryExpr*>(e)) {
                const Type t = checkExpr(u->rhs.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                if (u->op == TokenKind::Minus &&
                    (t.kind == TypeKind::I64 || t.kind == TypeKind::F64))
                    return t;
                if (u->op == TokenKind::Bang && t.kind == TypeKind::Bool)
                    return boolT();
                reason = "this unary operator is outside the builtin native v0 subset";
                return Type::void_();
            }
            if (const auto* b = dynamic_cast<const BinaryExpr*>(e)) {
                const Type l = checkExpr(b->lhs.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                const Type r = checkExpr(b->rhs.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                const bool same = sameType(l, r);
                switch (b->op) {
                case TokenKind::Plus:
                    if (l.kind == TypeKind::I64 && r.kind == TypeKind::I64)
                        return i64T();
                    if (l.kind == TypeKind::F64 && r.kind == TypeKind::F64)
                        return f64T();
                    if (l.kind == TypeKind::String && r.kind == TypeKind::String)
                        return strT();
                    break;
                case TokenKind::Minus:
                case TokenKind::Star:
                case TokenKind::Slash:
                    if (l.kind == TypeKind::I64 && r.kind == TypeKind::I64)
                        return i64T();
                    if (l.kind == TypeKind::F64 && r.kind == TypeKind::F64)
                        return f64T();
                    break;
                case TokenKind::Percent:
                    if (l.kind == TypeKind::I64 && r.kind == TypeKind::I64)
                        return i64T();
                    break;
                case TokenKind::Ampersand:
                case TokenKind::Pipe:
                case TokenKind::Caret:
                case TokenKind::ShiftLeft:
                case TokenKind::ShiftRight:
                    if (l.kind == TypeKind::I64 && r.kind == TypeKind::I64)
                        return i64T();
                    break;
                case TokenKind::EqualEqual:
                case TokenKind::BangEqual:
                    if (same && isScalar(l))
                        return boolT();
                    break;
                case TokenKind::Less:
                case TokenKind::LessEqual:
                case TokenKind::Greater:
                case TokenKind::GreaterEqual:
                    if (same && (l.kind == TypeKind::I64 || l.kind == TypeKind::F64))
                        return boolT();
                    break;
                case TokenKind::AndAnd:
                case TokenKind::OrOr:
                    if (l.kind == TypeKind::Bool && r.kind == TypeKind::Bool)
                        return boolT();
                    break;
                default:
                    break;
                }
                reason = "this operator/type combination is outside the builtin native v0 subset";
                return Type::void_();
            }
            if (const auto* c = dynamic_cast<const CallExpr*>(e))
                return checkCall(c, env, fns, structs, reason);
            if (const auto* a = dynamic_cast<const ArrayExpr*>(e)) {
                if (a->elements.empty()) {
                    reason = "empty array literals are outside the builtin native v0 subset";
                    return Type::void_();
                }
                const Type elem = checkExpr(a->elements[0].get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                for (const auto& el : a->elements) {
                    const Type t = checkExpr(el.get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return Type::void_();
                    if (!sameType(t, elem)) {
                        reason = "array literal elements must all have the same type";
                        return Type::void_();
                    }
                }
                return Type::array(elem, a->elements.size());
            }
            if (const auto* x = dynamic_cast<const IndexExpr*>(e)) {
                const Type obj = checkExpr(x->object.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                const Type idx = checkExpr(x->index.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                if (obj.kind != TypeKind::Array) {
                    reason = "indexing is only supported on fixed-size arrays in the builtin "
                             "native v0 subset (slices and strings need --backend llvm)";
                    return Type::void_();
                }
                if (idx.kind != TypeKind::I64) {
                    reason = "array index must be an i64";
                    return Type::void_();
                }
                return *obj.element;
            }
            if (const auto* m = dynamic_cast<const MemberExpr*>(e)) {
                const Type obj = checkExpr(m->object.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return Type::void_();
                if (obj.kind != TypeKind::Struct) {
                    reason = "member access requires a struct value in the builtin native v0 "
                             "subset";
                    return Type::void_();
                }
                const StructDecl* decl = findStruct(structs, obj.name);
                if (!decl) {
                    reason = "struct '" + obj.name +
                             "' is unknown to the builtin native "
                             "backend";
                    return Type::void_();
                }
                const StructField* f = findField(*decl, m->member);
                if (!f) {
                    reason = "struct '" + obj.name + "' has no member '" + m->member + "'";
                    return Type::void_();
                }
                return f->type;
            }
            if (const auto* s = dynamic_cast<const StructInitExpr*>(e)) {
                const StructDecl* decl = findStruct(structs, s->typeName);
                if (!decl) {
                    reason = "struct '" + s->typeName + "' is outside the builtin native v0 subset";
                    return Type::void_();
                }
                for (const auto& [fname, fexpr] : s->fields) {
                    const StructField* f = findField(*decl, fname);
                    if (!f) {
                        reason = "struct '" + s->typeName + "' has no member '" + fname + "'";
                        return Type::void_();
                    }
                    const Type t = checkExpr(fexpr.get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return Type::void_();
                    if (!sameType(t, f->type)) {
                        reason = "field '" + fname + "' of struct '" + s->typeName +
                                 "' has a mismatched or unsupported type";
                        return Type::void_();
                    }
                }
                return Type::named(s->typeName);
            }
            reason = "this expression is outside the builtin native v0 subset "
                     "(slices, pointers, enums, and match values need --backend llvm)";
            return Type::void_();
        }

        bool blockBindsName(const Block* block, const std::string& name) {
            if (!block)
                return false;
            for (const auto& statement : block->statements) {
                if (const auto* let = dynamic_cast<const LetStmt*>(statement.get())) {
                    if (let->name == name)
                        return true;
                } else if (const auto* nested = dynamic_cast<const Block*>(statement.get())) {
                    if (blockBindsName(nested, name))
                        return true;
                } else if (const auto* branch = dynamic_cast<const IfStmt*>(statement.get())) {
                    if (blockBindsName(branch->thenBlock.get(), name) ||
                        blockBindsName(branch->elseBlock.get(), name))
                        return true;
                } else if (const auto* loop = dynamic_cast<const WhileStmt*>(statement.get())) {
                    if (blockBindsName(loop->body.get(), name))
                        return true;
                } else if (const auto* loop = dynamic_cast<const ForStmt*>(statement.get())) {
                    if (loop->name == name || blockBindsName(loop->body.get(), name))
                        return true;
                }
            }
            return false;
        }

        void checkBlock(const Block* b,
                        std::map<std::string, Type>& env,
                        const std::map<std::string, FnSig>& fns,
                        const std::map<std::string, const StructDecl*>& structs,
                        bool inLoop,
                        std::string& reason);

        void checkStmt(const Stmt* s,
                       std::map<std::string, Type>& env,
                       const std::map<std::string, FnSig>& fns,
                       const std::map<std::string, const StructDecl*>& structs,
                       bool inLoop,
                       std::string& reason) {
            if (!reason.empty())
                return;
            if (const auto* nested = dynamic_cast<const Block*>(s)) {
                auto saved = env;
                checkBlock(nested, env, fns, structs, inLoop, reason);
                env = std::move(saved);
                return;
            }
            if (const auto* l = dynamic_cast<const LetStmt*>(s)) {
                if (!supportedType(l->type, structs, reason)) {
                    reason =
                        "variable '" + l->name +
                        "' has a declared type outside the builtin native v0 subset: " + reason;
                    return;
                }
                const Type actual = checkExpr(l->init.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (!sameType(actual, l->type)) {
                    reason = "initializer for variable '" + l->name +
                             "' does not match its declared type";
                    return;
                }
                env[l->name] = l->type;
                return;
            }
            if (const auto* a = dynamic_cast<const AssignStmt*>(s)) {
                const auto it = env.find(a->name);
                if (it == env.end()) {
                    reason = "assignment to unknown variable '" + a->name + "'";
                    return;
                }
                const Type v = checkExpr(a->value.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (!sameType(v, it->second)) {
                    reason = "assignment to '" + a->name + "' has a mismatched or unsupported type";
                    return;
                }
                return;
            }
            if (dynamic_cast<const DerefAssignStmt*>(s)) {
                reason = "pointer assignment is outside the builtin native v0 subset; use "
                         "--backend llvm";
                return;
            }
            if (const auto* ma = dynamic_cast<const MemberAssignStmt*>(s)) {
                const Type obj = checkExpr(ma->object.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (obj.kind != TypeKind::Struct) {
                    reason = "member assignment requires a struct value";
                    return;
                }
                const StructDecl* decl = findStruct(structs, obj.name);
                const StructField* f = decl ? findField(*decl, ma->member) : nullptr;
                if (!f) {
                    reason = "struct '" + obj.name + "' has no member '" + ma->member + "'";
                    return;
                }
                const Type v = checkExpr(ma->value.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (ma->op == TokenKind::Equal) {
                    if (!sameType(v, f->type)) {
                        reason = "member '" + ma->member + "' assignment has a mismatched type";
                        return;
                    }
                    return;
                }
                if (!compoundTypeOk(ma->op, f->type, v)) {
                    reason = "compound member assignment '" + ma->member +
                             "' has an unsupported type/operator combination";
                    return;
                }
                return;
            }
            if (const auto* ia = dynamic_cast<const IndexAssignStmt*>(s)) {
                const Type obj = checkExpr(ia->object.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                const Type idx = checkExpr(ia->index.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (obj.kind != TypeKind::Array) {
                    reason = "index assignment is only supported on fixed-size arrays";
                    return;
                }
                if (idx.kind != TypeKind::I64) {
                    reason = "array index must be an i64";
                    return;
                }
                const Type v = checkExpr(ia->value.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (ia->op == TokenKind::Equal) {
                    if (!sameType(v, *obj.element)) {
                        reason = "index assignment has a mismatched element type";
                        return;
                    }
                    return;
                }
                if (!compoundTypeOk(ia->op, *obj.element, v)) {
                    reason = "compound index assignment has an unsupported type/operator "
                             "combination";
                    return;
                }
                return;
            }
            if (const auto* e = dynamic_cast<const ExprStmt*>(s)) {
                checkExpr(e->expr.get(), env, fns, structs, reason);
                return;
            }
            if (dynamic_cast<const DeferStmt*>(s)) {
                reason = "defer is outside the builtin native v0 subset; use --backend llvm";
                return;
            }
            if (const auto* r = dynamic_cast<const ReturnStmt*>(s)) {
                if (r->expr)
                    checkExpr(r->expr.get(), env, fns, structs, reason);
                return;
            }
            if (const auto* i = dynamic_cast<const IfStmt*>(s)) {
                const Type t = checkExpr(i->cond.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (t.kind != TypeKind::Bool) {
                    reason = "if condition must be bool in the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                checkBlock(i->thenBlock.get(), env, fns, structs, inLoop, reason);
                env = saved;
                if (i->elseBlock) {
                    checkBlock(i->elseBlock.get(), env, fns, structs, inLoop, reason);
                    env = saved;
                }
                return;
            }
            if (const auto* loop = dynamic_cast<const ForStmt*>(s)) {
                Type elemType = Type::i64();
                if (!loop->rangeEnd || !loop->start) {
                    // Collection iteration: only fixed-size arrays are supported.
                    if (!loop->collection) {
                        reason = "this for-loop form is outside the builtin native v0 subset; "
                                 "use --backend llvm";
                        return;
                    }
                    const Type ct = checkExpr(loop->collection.get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return;
                    if (ct.kind != TypeKind::Array) {
                        reason = "for-in iteration is only supported over fixed-size arrays in "
                                 "the builtin native v0 subset (strings and slices need "
                                 "--backend llvm)";
                        return;
                    }
                    elemType = *ct.element;
                } else {
                    const Type start = checkExpr(loop->start.get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return;
                    const Type limit = checkExpr(loop->rangeEnd.get(), env, fns, structs, reason);
                    if (!reason.empty())
                        return;
                    if (start.kind != TypeKind::I64 || limit.kind != TypeKind::I64) {
                        reason = "numeric for-loop bounds must be i64 in the builtin native v0 "
                                 "subset";
                        return;
                    }
                }
                if (env.contains(loop->name)) {
                    reason = "shadowed for-loop variable '" + loop->name +
                             "' is outside the builtin native v0 subset";
                    return;
                }
                if (blockBindsName(loop->body.get(), loop->name)) {
                    reason = "shadowing the for-loop variable '" + loop->name +
                             "' inside its body is outside the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                env[loop->name] = elemType;
                checkBlock(loop->body.get(), env, fns, structs, true, reason);
                env = std::move(saved);
                return;
            }
            if (const auto* w = dynamic_cast<const WhileStmt*>(s)) {
                const Type t = checkExpr(w->cond.get(), env, fns, structs, reason);
                if (!reason.empty())
                    return;
                if (t.kind != TypeKind::Bool) {
                    reason = "while condition must be bool in the builtin native v0 subset";
                    return;
                }
                auto saved = env;
                checkBlock(w->body.get(), env, fns, structs, true, reason);
                env = saved;
                return;
            }
            if (dynamic_cast<const BreakStmt*>(s) || dynamic_cast<const ContinueStmt*>(s)) {
                if (!inLoop)
                    reason = "break/continue outside a loop";
                return;
            }
            reason = "this statement (match or unsupported form) is outside the builtin native v0 "
                     "subset";
        }

        void checkBlock(const Block* b,
                        std::map<std::string, Type>& env,
                        const std::map<std::string, FnSig>& fns,
                        const std::map<std::string, const StructDecl*>& structs,
                        bool inLoop,
                        std::string& reason) {
            for (const auto& s : b->statements) {
                checkStmt(s.get(), env, fns, structs, inLoop, reason);
                if (!reason.empty())
                    return;
            }
        }

        // ---------------------------------------------------------------------------

    } // namespace detail
} // namespace npp::native
