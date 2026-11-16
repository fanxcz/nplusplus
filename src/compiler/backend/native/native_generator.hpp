#pragma once

#include "native/native.hpp"

#include "lexer/token.hpp"
#include "native/image.hpp"
#include "native/native_internal.hpp"
#include "native/x64_emitter.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace npp::native {
    namespace detail {

        // Lowers the builtin native v0 subset to x86-64 machine code. The
        // declaration lives here; the implementation is split by topic under
        // native_gen_*.cpp so no single file grows without bound.
        class Generator {
        public:
            Generator(CodeImage& image, NativeTarget target);

            static Kind typeKind(const Type& t);

            void generate(const Program& program);

        private:
            CodeImage& image_;
            Emitter out_;
            NativeTarget target_;
            std::map<std::string, FnSig> sigs_;
            std::map<std::string, const StructDecl*> structs_;
            std::map<std::string, const EnumDecl*> enums_;
            // Match statements: hidden slot holding the matched value (enum
            // block pointer) plus per-arm payload binding types.
            std::map<const MatchStmt*, std::string> matchValueNames_;
            std::map<std::string, Type> matchBindTypes_;
            std::map<std::string, std::size_t> slots_;
            std::map<const ForStmt*, std::string> forLimitNames_;
            // Collection for-loops (`for x in arr`): hidden pointer + index slots.
            std::map<const ForStmt*, std::pair<std::string, std::string>> forIterNames_;
            std::size_t forIterCounter_ = 0;
            // Compound member/index assignments (`p.x += 1`): two hidden
            // scratch slots (current value, rhs value) per statement.
            std::map<const Stmt*, std::pair<std::string, std::string>> compoundNames_;
            std::size_t compoundCounter_ = 0;
            // Types of hidden scratch slots, consulted by localType.
            std::map<std::string, Type> scratchTypes_;
            std::size_t forLimitCounter_ = 0;
            std::size_t matchCounter_ = 0;
            std::size_t slotCount_ = 0;
            std::string endLabel_;
            std::vector<std::string> breakStack_;
            std::vector<std::string> continueStack_;
            // Top-level `defer` calls of the current function; emitted in
            // reverse order at the function epilogue (endLabel_).
            std::vector<const Expr*> fnDefers_;
            const Function* currentFn_ = nullptr;

            static std::int64_t slotOff(std::size_t slot);

            std::size_t internString(const std::string& text);

            void genFunction(const Function& f);

            void collectLocals(const Block* b);

            void genBlock(const Block* b);

            void genStmt(const Stmt* s);

            // `for x in array`: index loop over an arena block. The element
            // variable receives a copy (aggregates are deep-copied).
            void genForCollection(const ForStmt* f);

            // `match value { ... }` over enum values: tag compares per arm,
            // payload bindings stored into per-arm slots.
            void genMatch(const MatchStmt* m);

            // Index of an enum variant within its declaration (tag value).
            std::int64_t enumTag(const std::string& enumName, const std::string& variant);

            void genExpr(const Expr* e);

            // Emit code leaving the address of an lvalue in rax (used by
            // unary `&`). Handles variables, member/index chains, and `*p`.
            void genAddr(const Expr* e);

            void genPrint(const Expr* arg);

            // True for string-typed expressions (literals, string vars/params, string
            // concatenation, and the input/str_i64 builtins).
            bool exprIsString(const Expr* e);

            // Mirrors the checker's kind rules to select the right print helper.
            bool exprIsBool(const Expr* e);

            bool exprIsF64(const Expr* e);

            // Best-effort result type of an expression, mirroring the subset
            // checker. Used to pick xmm0 (f64) vs rax (everything else) and to
            // resolve array element / struct field types.
            Type exprType(const Expr* e);

            Kind exprKind(const Expr* e);

            Type localType(const std::string& name);

            Kind localKind(const std::string& name);

            void findLet(const Block* b, const std::string& name, Type& found, bool& ok);

            Type initType(const Expr* e);

            // Load/store a value through rcx (computed aggregate address).
            void genLoadFromRcx(Kind k);
            void genStoreToRcx(Kind k);

            // Emit an unsigned bounds check: rax = index, jumps to __nx_oob
            // when index >= count (negative indices wrap and fail too).
            void genIndexCheck(std::size_t count);

            // True when an aggregate-typed expression needs a defensive copy
            // before being stored (fresh literals already own their block).
            bool needsDup(const Expr* e);

            // Index of `member` in the named struct's field list (-1 if absent).
            int structFieldIndex(const std::string& typeName, const std::string& member);

            // Emit a deep copy of the aggregate in rax (fresh arena block,
            // aggregate fields recursively duplicated). Returns rax = copy.
            void genDupValue(const Type& t);

            // Type of aggregate slot `i` (array element or struct field).
            Type aggregateFieldType(const Type& t, std::size_t i);

            // Compound member/index assignment (`p.x += v`, `xs[i] -= v`).
            // Expects the target address on top of the machine stack
            // ([rsp] = &slot) and consumes it. `bop` is the binary operator
            // equivalent of the compound token (see compoundBinOp).
            void genCompoundAssign(const Stmt* stmt,
                                   const Expr* value,
                                   const Type& targetType,
                                   TokenKind bop);

            // Call the shared write helper. r10/r11 do NOT survive OS calls
            // (the syscall instruction itself clobbers rcx+r11, and ntdll
            // trampolines clobber r10), so the stack pointer is sheltered in the
            // callee-saved rbx instead. Nothing else in the stubs uses rbx.
            void stubCall(const std::string& target);

            // __nx_write: rcx = pointer, rdx = length. Writes to stdout.
            void genWrite();

            // __nx_write_err: rcx = pointer, rdx = length. Writes to stderr.
            void genWriteErr();

            // __nx_print_i64: rax = value. Prints decimal + newline.
            void genPrintI64();

            // __nx_print_bool: rax = 0/1. Prints "true"/"false" + newline.
            void genPrintBool();

            // __nx_write_cstr: rax = NUL-terminated string. Writes it + newline.
            void genWriteCstr();

            // __nx_str_eq: rcx = a, rdx = b (NUL-terminated). Returns 0/1 in rax.
            // Implemented with repne/repe string instructions over strlen(a)+1 bytes
            // (the trailing NUL decides prefix cases correctly).
            void genStrEq();

            // __nx_string_len: rax = NUL-terminated string. Returns byte length in rax.
            void genStringLen();

            // __nx_string_byte_at: rax = string, rcx = index. Returns byte (0-255) in rax, or -1 if
            // out of bounds.
            void genStringByteAt();

            // __nx_str_cat: rcx = a, rdx = b. Returns a fresh arena string in rax.
            void genStrCat();

            // __nx_str_i64: rax = value. Returns a fresh arena string in rax.
            void genStrI64();

            // __nx_input: rax = prompt (NUL-terminated). Prints it, reads one line
            // from stdin, and returns a fresh arena string without the newline.
            void genInput();

            // __nx_input_i64: rax = prompt. Prints it, reads a line and parses
            // it as i64 (same semantics as the LLVM runtime input_i64).
            void genInputI64();

            // __nx_str_to_i64: rax = NUL-terminated string. Parses an optional
            // sign and decimal digits; trailing garbage stops the parse.
            void genStrToI64();

            // __nx_exit: rcx = exit code. Terminates the process.
            void genExit();

            // __nx_time_ms: returns monotonic milliseconds in rax.
            void genTimeMs();

            // __nx_sleep: rcx = milliseconds. Sleeps (ms <= 0 returns at once).
            void genSleep();

            // __nx_rng_next: SplitMix64 step over the __nx_rng state word.
            // Returns the next pseudo-random u64 in rax.
            void genRngNext();

            // __nx_random_i64: rcx = min, rdx = max. Returns a value in
            // [min, max] using unbiased rejection sampling, matching the
            // runtime's npp_random_i64 semantics.
            void genRandomI64();

            // __nx_file_exists: rcx = NUL-terminated path. Returns 0/1 in rax.
            void genFileExists();

            // __nx_str_f64: xmm0 = value. Returns a fresh arena string in rax
            // and its length in rdx. Shortest-round-trip is approximated by up
            // to 15 significant digits with round-half-up, %g-style notation.
            void genStrF64();

            // __nx_print_f64: xmm0 = value. Prints the %g-style rendering + '\n'.
            void genPrintF64();

            // __nx_str_to_f64: rax = NUL-terminated string. Parses a decimal
            // float; returns the value in xmm0 and validity in edx.
            void genStrToF64();

            // __nx_input_f64: rax = prompt. Prints it, reads a line and parses
            // it as f64, re-prompting on invalid input. Returns xmm0.
            void genInputF64();

            // __nx_alloc: rax = size in bytes (multiple of 8). Bumps the
            // string arena and returns a raw pointer in rax. Used for
            // array/struct storage.
            void genAlloc();

            // __nx_dup: rax = src pointer, rdx = size bytes. Returns a fresh
            // arena copy of the block in rax (aggregate copy semantics).
            void genDup();

            // __nx_oob: index-out-of-bounds runtime failure (stderr + exit 1).
            void genOob();

            // System/OS helpers (native_gen_sys.cpp).
            void genStrlen();
            void genArgsInit();
            void genOsArgsCount();
            void genOsArgsGet();
            void genEnv();
            void genFileRead();
            void genFileReadChecked();
            void genFileWrite();
            void genMemAlloc();
            void genMemFree();
            void genMemRead();
            void genMemSet();
            void genMemCopy();
            void genBeep();
            void genKeyPressed();
            void genReadKey();

            // String builtins (native_gen_str.cpp). Strings stay
            // NUL-terminated arena pointers; helpers return fresh arena
            // strings or scalars per the LLVM runtime utf8.inc semantics.
            void emitIsSpaceByte(const std::string& spLabel, const std::string& nsLabel);
            void genUtf8Dec();
            void genStrChars();
            void genStrCharAt();
            void genStrValidUtf8();
            void genStrFromChar();
            void genStrTrim();
            void genStrCase(bool upper);
            void genStrUpper();
            void genStrLower();
            void genStrFindCore(bool contains);
            void genStrFind();
            void genStrContains();
            void genStrSlice();
            void genStrSplitCount();
            void genStrSplitAt();

            // Growable list helpers (native_gen_list.cpp). Handles are
            // opaque i64 pointers to arena header blocks; tag 1 = int
            // list, tag 2 = string list (mirrors lists.inc).
            void emitLcValid(const std::string& bad);
            void genLcNew();
            void genLcGrow();
            void genLcLen();
            void genLcGet();
            void genLcSet(bool checked);
            void genLcPush(bool checked);
            void genLcReserve();
            void genLcClear(bool freeIt);
            void genLcInsert();
            void genLcTryGet();
            void genLcRemove();
            void genLcTryPop();

            // f64 math builtins (native_gen_math.cpp). Unary helpers take
            // xmm0 in/out; atan2/pow take raw f64 bits on the stack like
            // user-function parameters.
            void genMathUnaryX87(const char* name, int op);
            void genMathSqrt();
            void genMathRound(bool isFloor);
            void genMathAtan2();
            void genMathPow();
            void genMathSin();
            void genMathCos();
            void genMathTan();
            void genMathFloor();
            void genMathCeil();

            // Game-physics predicates (native_gen_phys.cpp): f64 args as
            // raw bits on the stack, bool result in rax.
            void genPhysicsAabb();
            void genPhysicsCircle();

            // Clean runtime failures for allocation and integer division.
            void genOom();
            void genDivZero();
            void genDivOverflow();

            void genEntry();
        };

    } // namespace detail
} // namespace npp::native
