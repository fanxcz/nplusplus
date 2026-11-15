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
            std::map<std::string, std::size_t> slots_;
            std::map<const ForStmt*, std::string> forLimitNames_;
            std::size_t forLimitCounter_ = 0;
            std::size_t slotCount_ = 0;
            std::string endLabel_;
            std::vector<std::string> breakStack_;
            std::vector<std::string> continueStack_;
            const Function* currentFn_ = nullptr;

            static std::int64_t slotOff(std::size_t slot);

            std::size_t internString(const std::string& text);

            void genFunction(const Function& f);

            void collectLocals(const Block* b);

            void genBlock(const Block* b);

            void genStmt(const Stmt* s);

            void genExpr(const Expr* e);

            void genPrint(const Expr* arg);

            // True for string-typed expressions (literals, string vars/params, string
            // concatenation, and the input/str_i64 builtins).
            bool exprIsString(const Expr* e);

            // Mirrors the checker's kind rules to select the right print helper.
            bool exprIsBool(const Expr* e);

            bool exprIsF64(const Expr* e);

            // Best-effort result kind of an expression, mirroring the subset
            // checker. Used to pick xmm0 (f64) vs rax (everything else).
            Kind exprKind(const Expr* e);

            Kind localKind(const std::string& name);

            void findLet(const Block* b, const std::string& name, Kind& found, bool& ok);

            Kind initKind(const Expr* e);

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

            // Clean runtime failures for allocation and integer division.
            void genOom();
            void genDivZero();
            void genDivOverflow();

            void genEntry();
        };

    } // namespace detail
} // namespace npp::native
