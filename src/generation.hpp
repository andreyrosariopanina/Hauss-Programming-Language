#pragma once

#include <sstream>
#include <unordered_map>   
#include <cassert>
#include <map>
#include <algorithm>

#include "./parser.hpp"

// The Generator class is responsible for generating x86-64 assembly code
// from the AST (Abstract Syntax Tree) nodes produced by the parser.
class Generator {
public:
    // Constructor: stores the root program node
    inline Generator(NodeProg prog)
        : m_prog(std::move(prog))
    {
    }

    // Generate assembly for a terminal expression
    void gen_term(const NodeTerm* term){
        struct TermVisitor {
            Generator& gen;

            // Integer literal (e.g. 42)
            void operator()(const NodeTermIntLit* term_int_lit) const {
                gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }

            // Identifier (e.g. variable x)
            void operator()(const NodeTermIdent* term_ident) const {
                auto it = std::find_if(
                        gen.m_vars.cbegin(),
                        gen.m_vars.cend(),
                        [&](const Var& var) { return var.name == term_ident->ident.value.value(); });

                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                gen.push(offset.str()); 
            }

            // Unary negation (e.g. -x)
            void operator()(const NodeTermNeg* term_neg) const {
                gen.gen_term(term_neg->term);
                gen.pop("rax");
                gen.m_output << "    neg rax\n";
                gen.push("rax");
            }

            // Parenthesized expression (e.g. (x + 1))
            void operator()(const NodeTermParen* term_paren) const {
                gen.gen_expr(term_paren->expr);
            }
        };

        TermVisitor visitor({.gen = *this});
        std::visit(visitor, term->var);
    }

    // Generate assembly for a binary expression
    void gen_bin_expr(const NodeBinExpr* bin_expr){
        struct BinExprVisitor {
            Generator& gen;

            // Binary operations: all follow the pattern
            // 1. evaluate RHS and LHS
            // 2. pop values
            // 3. perform operation
            // 4. push result

            void operator()(const NodeBinExprSub* sub) const {
                gen.gen_expr(sub->rhs);
                gen.gen_expr(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    sub rax, rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprAdd* add) const {
                gen.gen_expr(add->rhs);
                gen.gen_expr(add->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprMulti* multi) const {
                gen.gen_expr(multi->rhs);
                gen.gen_expr(multi->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    mul rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprDiv* div) const {
                gen.gen_expr(div->rhs);
                gen.gen_expr(div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    div rbx\n";
                gen.push("rax");
            }

            // Comparison operators
            void operator()(const NodeBinExprGt* gt) const {
                gen.gen_expr(gt->lhs);
                gen.gen_expr(gt->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.m_output << "    cmp rax, rbx\n";
                gen.m_output << "    setg al\n";
                gen.m_output << "    movzx rax, al\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprGe* ge) const {
                gen.gen_expr(ge->lhs);
                gen.gen_expr(ge->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.m_output << "    cmp rax, rbx\n";
                gen.m_output << "    setge al\n";
                gen.m_output << "    movzx rax, al\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprLt* lt) const {
                gen.gen_expr(lt->lhs);
                gen.gen_expr(lt->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.m_output << "    cmp rax, rbx\n";
                gen.m_output << "    setl al\n";
                gen.m_output << "    movzx rax, al\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprLe* le) const {
                gen.gen_expr(le->lhs);
                gen.gen_expr(le->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.m_output << "    cmp rax, rbx\n";
                gen.m_output << "    setle al\n";
                gen.m_output << "    movzx rax, al\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprEqEq* eq_eq) const {
                gen.gen_expr(eq_eq->lhs);
                gen.gen_expr(eq_eq->rhs);
                gen.pop("rbx");
                gen.pop("rax");
                gen.m_output << "    cmp rax, rbx\n";
                gen.m_output << "    sete al\n";
                gen.m_output << "    movzx rax, al\n";
                gen.push("rax");
            }
        };

        BinExprVisitor visitor {.gen = *this};
        std::visit(visitor, bin_expr->var);
    }

    // Generate assembly for any expression node
    void gen_expr(const NodeExpr* expr) {
        struct ExprVisitor {
            Generator& gen;

            void operator()(const NodeTerm* term) const {
                gen.gen_term(term);
            }

            void operator()(const NodeBinExpr* bin_expr) const {
                gen.gen_bin_expr(bin_expr);
            }
        };

        ExprVisitor visitor {.gen = *this};
        std::visit(visitor, expr->var);
    }

    // Generate assembly for a scope (block of statements)
    void gen_scope(const NodeScope* scope){
        begin_scope();
        for (const NodeStmt* stmt : scope->stmts){
            gen_stmt(stmt);
        }
        end_scope();
    }

    // Generate code for an if-elif-else chain
    void gen_if_pred(const NodeIfPred* pred, const std::string& end_label){
        struct PredVisitor {
            Generator& gen;
            const std::string& end_label;

            void operator()(const NodeIfPredElif* elif) const {
                gen.m_output << "    ;; elif\n";
                gen.gen_expr(elif->expr);
                gen.pop("rax");
                const std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.gen_scope(elif->scope);
                gen.m_output << "    jmp " << end_label << "\n";

                if (elif->pred.has_value()) {
                    gen.m_output << label << ":\n";
                    gen.gen_if_pred(elif->pred.value(), end_label);
                }
            }

            void operator()(const NodeIfPredElse* else_) const {
                gen.gen_scope(else_->scope);
            }
        };

        PredVisitor visitor{.gen = *this, .end_label = end_label};
        std::visit(visitor, pred->var);
    }

    // Generate assembly for a statement node
    void gen_stmt(const NodeStmt* stmt) {
        struct StmtVisitor {
            Generator& gen;

            // Exit program with value
            void operator()(const NodeStmtExit* stmt_exit) const {
                gen.gen_expr(stmt_exit->expr);
                gen.m_output << "    mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "    syscall\n";
            }

            // Variable declaration (let)
            void operator()(const NodeStmtLet* stmt_let) const {
                auto it = std::find_if(
                    gen.m_vars.cbegin(), gen.m_vars.cend(),
                    [&](const Var& var) { return var.name == stmt_let->ident.value.value(); });

                if (it != gen.m_vars.cend()) {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }

                gen.m_vars.push_back({.name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size});
                gen.gen_expr(stmt_let->expr); 
            }

            // Assignment (x = ...)
            void operator()(const NodeStmtAssign* stmt_assign) const {
                auto it = std::find_if(gen.m_vars.begin(), gen.m_vars.end(),
                    [&](const Var& var) { return var.name == stmt_assign->ident.value.value(); });

                if (it == gen.m_vars.end()){
                    std::cerr << "Undeclared identifier: " << stmt_assign->ident.value.value() << "\n";
                    exit(EXIT_FAILURE);
                }

                gen.gen_expr(stmt_assign->expr);
                gen.pop("rax");
                gen.m_output << "    mov [rsp + " << (gen.m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
            }

            // Nested scope
            void operator()(const NodeScope* scope) const {
                gen.m_output << "    ;; scope\n";
                gen.gen_scope(scope);
                gen.m_output << "    ;; /scope\n";
            }

            // If statement
            void operator()(const NodeStmtIf* stmt_if) {
                gen.gen_expr(stmt_if->expr);
                gen.pop("rax");
                std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.gen_scope(stmt_if->scope);

                if (stmt_if->pred.has_value()) {
                    const std::string end_label = gen.create_label();
                    gen.m_output << "    jmp " << end_label << "\n";
                    gen.m_output << label << ":\n";
                    gen.gen_if_pred(stmt_if->pred.value(), end_label);
                    gen.m_output << end_label << ":\n";
                } else {
                    gen.m_output << label << ":\n";
                }
            }

            // Print integer value
            void operator()(const NodeStmtPrint* stmt_print) const {
                gen.gen_expr(stmt_print->expr);
                gen.pop("rdi");
                gen.m_output << "    call print_int\n";
            }
        };

        StmtVisitor visitor {.gen = *this};
        std::visit(visitor, stmt->var);
    }

    // Generate the full program's assembly
    std::string gen_prog() {
        m_output << "global _start\n_start:\n";
        for (const NodeStmt* stmt : m_prog.stmts){
            gen_stmt(stmt);
        }

        // Default exit if not explicitly exited
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall";

        // Print integer routine
        m_output << R"(
            print_int:
                push rbp
                mov rbp, rsp
                sub rsp, 32
                ; Check if number is negative
                test rdi, rdi
                jns .positive
                ; Handle negative number
                mov byte [rsp], '-'
                mov rax, 1
                mov rsi, rsp
                mov rdx, 1
                push rdi
                mov rdi, 1
                syscall
                pop rdi
                neg rdi
            .positive:
                test rdi, rdi
                jnz .non_zero
                mov byte [rsp], '0'
                mov rsi, rsp
                mov rdx, 1
                jmp .print
            .non_zero:
                mov rax, rdi
                lea rsi, [rsp+31]
                mov rcx, 0
            .convert_loop:
                xor rdx, rdx
                mov r10, 10
                div r10
                add dl, '0'
                dec rsi
                mov [rsi], dl
                inc rcx
                test rax, rax
                jnz .convert_loop
            .print:
                mov rax, 1
                mov rdi, 1
                mov rdx, rcx
                syscall

                ; Print newline
                mov byte [rsp], 10
                mov rax, 1
                mov rdi, 1
                mov rsi, rsp
                mov rdx, 1
                syscall

                mov rsp, rbp
                pop rbp
                ret
            )";

        return m_output.str();
    }

private:
    // Utility: push register or value onto stack
    void push(const std::string& reg){
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    // Utility: pop into a register
    void pop(const std::string& reg){
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    // Begin a new variable scope
    void begin_scope(){
        m_scopes.push_back(m_vars.size());
    }

    // End current scope and deallocate local variables
    void end_scope(){
        size_t pop_count = m_vars.size() - m_scopes.back();
        m_output << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for (int i = 0; i < pop_count; i++){
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    // Generate a unique label for control flow
    std::string create_label(){
        std::stringstream ss;
        ss << "label" << m_label_count++;
        return ss.str();
    }

    // Struct for tracking local variables
    struct Var {
        std::string name;
        size_t stack_loc;
    };

    // Internal state
    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::vector<Var> m_vars;
    std::vector<size_t> m_scopes;
    int m_label_count = 0;
};
