#pragma once

#include <variant>

#include "./arena.hpp"
#include "./tokenization.hpp"

struct NodeTerm;

struct NodeTermNeg{
    NodeTerm* term;
};

struct NodeTermIntLit{
    Token int_lit;
};

struct NodeTermIdent{
    Token ident;
};

struct NodeExpr;

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeBinExprAdd{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMulti{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub{
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprGt {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprGe {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprLt {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprLe {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprEqEq {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr{
    std::variant<
        NodeBinExprAdd*, 
        NodeBinExprMulti*, 
        NodeBinExprDiv*, 
        NodeBinExprSub*,
        NodeBinExprGt*,
        NodeBinExprGe*,
        NodeBinExprLt*,
        NodeBinExprLe*,
        NodeBinExprEqEq*> var;
};

struct NodeTerm{
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*, NodeTermNeg*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtPrint {
    NodeExpr* expr;
};

struct NodeStmtExit{
    NodeExpr* expr;
};

struct NodeStmtLet{
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt;

struct NodeScope{
    std::vector<NodeStmt*> stmts;
};

struct NodeIfPred;

struct NodeIfPredElif {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElif*, NodeIfPredElse*> var;
};

struct NodeStmtIf{
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeStmtAssign{
    Token ident;
    NodeExpr* expr;
};

struct NodeStmt{
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeScope*, NodeStmtIf*, NodeStmtAssign*, NodeStmtPrint*> var;
};

struct NodeProg{
    std::vector<NodeStmt*> stmts;
};

struct NodeExit {
    NodeExpr* expr;
};

class Parser{
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4) // 4 MB
    {
    }

    void error_expected(const std::string& msg){
        std::cerr << "[Parse Error] Expected " << msg << " on line " << peek(-1).value().line << "\n";
        exit(EXIT_FAILURE);
    }

    std::optional<NodeBinExpr*> parse_bin_expr(){
        if (auto lhs = parse_expr()){
            auto bin_expr = m_allocator.alloc<NodeBinExpr>();
            if (peek().has_value() && peek().value().type == TokenType::plus){
                auto bin_expr_add = m_allocator.alloc<NodeBinExprAdd>();
                bin_expr_add->lhs = lhs.value();
                consume();
                if (auto rhs = parse_expr()){
                    bin_expr_add->rhs = rhs.value();
                    bin_expr->var = bin_expr_add;
                    return bin_expr;
                } else{
                    error_expected("expression");
                }
            } else{
                std::cerr << "Unsupported binary operator\n";
                exit(EXIT_FAILURE);
            }
        } else{
            return {};
        }
    }

    std::optional<NodeTerm*> parse_term(){
        if (auto int_lit = try_consume(TokenType::int_lit)){
            auto node_term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            node_term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = node_term_int_lit;
            return term;
        } 
        if (auto ident = try_consume(TokenType::ident)){
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;
            return term;
        } 
        if (auto open_paren = try_consume(TokenType::open_paren)){
            auto expr = parse_expr();
            if (!expr.has_value()){
                error_expected("expression");
            } else {
                try_consume_err(TokenType::close_paren);
                auto term_paren = m_allocator.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var = term_paren;
                return term;
            }
        } 

        if (auto sub_token = try_consume(TokenType::sub)) {
            auto term_neg = m_allocator.alloc<NodeTermNeg>();
            auto term = parse_term();
            if (!term.has_value()) {
                error_expected("term after unary '-'");
            }
            term_neg->term = term.value();
            auto node_term = m_allocator.alloc<NodeTerm>();
            node_term->var = term_neg;
            return node_term;
        }
        return {};
    }

    std::optional<NodeExpr*> parse_expr(int min_prec = 0){
        std::optional<NodeTerm*> term_lhs = parse_term();
        if (!term_lhs.has_value()){
            return {};
        }
        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();
        while (true){
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            if (curr_tok.has_value()){
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec){
                    break;
                }
            } else{
                break;
            }
            Token op = consume();
            int next_min_prec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_prec);
            if (!expr_rhs.has_value()){
                error_expected("expression");
            }
            auto expr = m_allocator.alloc<NodeBinExpr>();
            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();

            if (op.type == TokenType::gt) {
                auto gt = m_allocator.alloc<NodeBinExprGt>();
                expr_lhs2->var = expr_lhs->var;
                gt->lhs = expr_lhs2;
                gt->rhs = expr_rhs.value();
                expr->var = gt;
            } else if (op.type == TokenType::ge) {
                auto ge = m_allocator.alloc<NodeBinExprGe>();
                expr_lhs2->var = expr_lhs->var;
                ge->lhs = expr_lhs2;
                ge->rhs = expr_rhs.value();
                expr->var = ge;
            } else if (op.type == TokenType::lt) {
                auto lt = m_allocator.alloc<NodeBinExprLt>();
                expr_lhs2->var = expr_lhs->var;
                lt->lhs = expr_lhs2;
                lt->rhs = expr_rhs.value();
                expr->var = lt;
            } else if (op.type == TokenType::le) {
                auto le = m_allocator.alloc<NodeBinExprLe>();
                expr_lhs2->var = expr_lhs->var;
                le->lhs = expr_lhs2;
                le->rhs = expr_rhs.value();
                expr->var = le;
            } else if (op.type == TokenType::eq_eq) {
                auto eq_eq = m_allocator.alloc<NodeBinExprEqEq>();
                expr_lhs2->var = expr_lhs->var;
                eq_eq->lhs = expr_lhs2;
                eq_eq->rhs = expr_rhs.value();
                expr->var = eq_eq;
            } else if (op.type == TokenType::plus){
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                expr_lhs2->var = expr_lhs->var;
                add->lhs =expr_lhs2;
                add->rhs = expr_rhs.value();
                expr->var = add;
            } else if (op.type == TokenType::star){
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                expr_lhs2->var = expr_lhs->var;
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();
                expr->var = multi;
            } else if (op.type == TokenType::sub){
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                expr_lhs2->var = expr_lhs->var;
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();
                expr->var = sub;
            } else if (op.type == TokenType::div){
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                expr_lhs2->var = expr_lhs->var;
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();
                expr->var = div;
            } 

            expr_lhs->var = expr;
        }
        return expr_lhs;
    }

    std::optional<NodeScope*> parse_scope(){
        if (!try_consume(TokenType::open_curly).has_value()){
            return {};
        }
        auto scope = m_allocator.alloc<NodeScope>();
        while (auto stmt = parse_stmt()){
            scope->stmts.push_back(stmt.value());
        }
        try_consume_err(TokenType::close_curly);
        return scope;
    }

    std::optional<NodeIfPred*> parse_if_pred(){
        if (try_consume(TokenType::elif)){
            try_consume_err(TokenType::open_paren);
            auto elif = m_allocator.alloc<NodeIfPredElif>();
            
            if (auto expr = parse_expr()){
                elif->expr = expr.value();

            } else{
                error_expected("expression");
            }

            try_consume_err(TokenType::close_paren);

            if (auto scope = parse_scope()){
                elif->scope = scope.value();
            } else{
                error_expected("scope");
            }

            elif->pred = parse_if_pred();
            auto pred = m_allocator.emplace<NodeIfPred>(elif);
            return pred;
        }
        if (try_consume(TokenType::else_)){
            auto else_ = m_allocator.alloc<NodeIfPredElse>();
            if (auto scope = parse_scope()){
                else_->scope = scope.value();
            } else{
                error_expected("scope");
            }
            auto pred = m_allocator.emplace<NodeIfPred>(else_);
            return pred;
        }
        return {};
    }

    std::optional<NodeStmt*> parse_stmt(){
        if (peek().has_value() && peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren){
            consume();
            consume();
            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
            if (auto node_expr = parse_expr()){
                stmt_exit->expr = node_expr.value();
            } else{
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;
            return stmt;
        } 
        if (peek().has_value() && peek().value().type == TokenType::let &&
                    peek(1).has_value() && peek(1).value().type == TokenType::ident && 
                    peek(2).has_value() && peek(2).value().type == TokenType::eq){
                        consume();
                        auto stmt_let = m_allocator.alloc<NodeStmtLet>();
                        stmt_let->ident = consume();
                        consume();
                        if (auto expr = parse_expr()){
                            stmt_let->expr = expr.value();
                        } else{
                            error_expected("expression");
                        }
                        try_consume_err(TokenType::semi);
                        auto expr = m_allocator.alloc<NodeStmt>();
                        expr->var = stmt_let;
                        return expr;
        } 
        if (peek().has_value() && peek().value().type == TokenType::ident 
            && peek(1).has_value() && peek(1).value().type == TokenType::eq){
            
            auto assign = m_allocator.alloc<NodeStmtAssign>();
            assign->ident = consume();
            consume();
            if (auto expr = parse_expr()){
                assign->expr = expr.value();
            } else{
                error_expected("expression");
            }
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.emplace<NodeStmt>(assign);
            return stmt;
        }

        if (peek().has_value() && peek().value().type == TokenType::open_curly){
            if (auto scope = parse_scope()){
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            } else{
                error_expected("expression");
            }
        } 
        if (auto if_ = try_consume(TokenType::if_)){
            try_consume_err(TokenType::open_paren);
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();
            if (auto expr = parse_expr()){
                stmt_if->expr = expr.value();
            } else{
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            if (auto scope = parse_scope()){
                stmt_if->scope = scope.value();
            } else{
                error_expected("scope");
            }
            stmt_if->pred = parse_if_pred();
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_if;
            return stmt;
        }
        if (peek().has_value() && peek().value().type == TokenType::print &&
            peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
            consume(); // Consume 'print'
            consume(); // Consume '('
            auto print = m_allocator.alloc<NodeStmtPrint>();
            if (auto expr = parse_expr()) {
                print->expr = expr.value();
            } else {
                error_expected("expression");
            }
            try_consume_err(TokenType::close_paren);
            try_consume_err(TokenType::semi);
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = print;
            return stmt;
        }
        return {};
    }

    std::optional<NodeProg> parse_prog(){
        NodeProg prog;
        while (peek().has_value()){
            if (auto stmt = parse_stmt()){
                prog.stmts.push_back(stmt.value());
            } else{
                error_expected("statement");
            }
        }
        return prog;
    }

private:
    inline std::optional<Token> peek(int offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        } else {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume(){
        return m_tokens.at(m_index++);
    }

    inline Token try_consume_err(TokenType type){
        if (peek().has_value() && peek().value().type == type){
            return consume();
        } 
        error_expected(to_string(type));
        return {};
    }

    inline std::optional<Token> try_consume(TokenType type){
        if (peek().has_value() && peek().value().type == type){
            return consume();
        } 
        return {};
    }
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};