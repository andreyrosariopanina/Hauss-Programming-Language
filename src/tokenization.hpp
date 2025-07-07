#pragma once

#include <string>
#include <vector>
#include <optional>
#include <iostream>

/**
 * Enum representing all possible token types in the language.
 */
enum class TokenType {
    exit, 
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq, 
    plus,
    star,
    sub,
    div,
    open_curly,
    close_curly,
    if_,
    elif, 
    else_,
    print,
    gt,        // >
    ge,        // >=
    eq_eq,     // ==
    lt,        // <
    le         // <=
};

/**
 * Utility function to determine if a token is a binary operator.
 */
bool is_bin_op(TokenType type) {
    switch (type) {
        case TokenType::div:
        case TokenType::sub:
        case TokenType::plus:
        case TokenType::star:
            return true;
        default:
            return false;
    }
}

/**
 * Converts a TokenType to a human-readable string (used for debugging and error messages).
 */
inline std::string to_string(const TokenType type)
{
    switch (type) {
    case TokenType::exit: return "`exit`";
    case TokenType::int_lit: return "int literal";
    case TokenType::semi: return "`;`";
    case TokenType::open_paren: return "`(`";
    case TokenType::close_paren: return "`)`";
    case TokenType::ident: return "identifier";
    case TokenType::let: return "`let`";
    case TokenType::eq: return "`=`";
    case TokenType::plus: return "`+`";
    case TokenType::star: return "`*`";
    case TokenType::sub: return "`-`";
    case TokenType::div: return "`/`";
    case TokenType::open_curly: return "`{`";
    case TokenType::close_curly: return "`}`";
    case TokenType::if_: return "`if`";
    case TokenType::elif: return "`elif`";
    case TokenType::else_: return "`else`";
    case TokenType::print: return "`print`";
    case TokenType::gt: return "`>`";
    case TokenType::ge: return "`>=`";
    case TokenType::eq_eq: return "`==`";
    case TokenType::lt: return "`<`";
    case TokenType::le: return "`<=`";
    }
    assert(false); // should never be reached
}

/**
 * Returns the precedence of a binary operator, if applicable.
 */
std::optional<int> bin_prec(TokenType type) {
    switch (type) {
    case TokenType::sub:
    case TokenType::plus:
    case TokenType::gt:
    case TokenType::ge:
    case TokenType::lt:
    case TokenType::le:
    case TokenType::eq_eq:
        return 0; // lower precedence
    case TokenType::div:
    case TokenType::star:
        return 1; // higher precedence
    default:
        return {}; // not a binary operator
    }
}

/**
 * Struct representing a single token with type, line number, and optional value (e.g., identifier or literal).
 */
struct Token {
    TokenType type;
    int line;
    std::optional<std::string> value;
};

/**
 * Tokenizer class that converts a source string into a list of tokens.
 */
class Tokenizer {
public:
    /**
     * Constructor initializes the tokenizer with source code.
     */
    inline explicit Tokenizer(std::string src)
        : m_src(std::move(src)) {
    }

    /**
     * Tokenizes the input string and returns a vector of Token objects.
     */
    inline std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string buf;
        int line_cnt = 1;

        while (peek().has_value()) {
            if (std::isalpha(peek().value())) {
                // Handle keywords and identifiers
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buf.push_back(consume());
                }
                if (buf == "exit") tokens.push_back({TokenType::exit, line_cnt});
                else if (buf == "let") tokens.push_back({TokenType::let, line_cnt});
                else if (buf == "if") tokens.push_back({TokenType::if_, line_cnt});
                else if (buf == "elif") tokens.push_back({TokenType::elif, line_cnt});
                else if (buf == "else") tokens.push_back({TokenType::else_, line_cnt});
                else if (buf == "print") tokens.push_back({TokenType::print, line_cnt});
                else tokens.push_back({TokenType::ident, line_cnt, buf});
                buf.clear();
            }
            else if (peek().value() == '-' && peek(1).has_value() && std::isdigit(peek(1).value())) {
                // Handle negative integer literals
                consume();
                buf.push_back('-');
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({TokenType::int_lit, line_cnt, buf});
                buf.clear();
            }
            else if (std::isdigit(peek().value())) {
                // Handle positive integer literals
                buf.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({TokenType::int_lit, line_cnt, buf});
                buf.clear();
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/') {
                // Handle single-line comment
                consume(); consume();
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*') {
                // Handle multi-line comment
                consume(); consume();
                while (peek().has_value()) {
                    if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                        break;
                    }
                    consume();
                }
                if (peek().has_value()) consume();
                if (peek().has_value()) consume();
            }
            // Handle punctuation and operators
            else if (peek().value() == '(') { consume(); tokens.push_back({TokenType::open_paren, line_cnt}); }
            else if (peek().value() == ')') { consume(); tokens.push_back({TokenType::close_paren, line_cnt}); }
            else if (peek().value() == '{') { consume(); tokens.push_back({TokenType::open_curly, line_cnt}); }
            else if (peek().value() == '}') { consume(); tokens.push_back({TokenType::close_curly, line_cnt}); }
            else if (peek().value() == ';') { consume(); tokens.push_back({TokenType::semi, line_cnt}); }
            else if (peek().value() == '+' ) { consume(); tokens.push_back({TokenType::plus, line_cnt}); }
            else if (peek().value() == '*' ) { consume(); tokens.push_back({TokenType::star, line_cnt}); }
            else if (peek().value() == '-' ) { consume(); tokens.push_back({TokenType::sub, line_cnt}); }
            else if (peek().value() == '/' ) { consume(); tokens.push_back({TokenType::div, line_cnt}); }
            else if (peek().value() == '>' && peek(1).has_value() && peek(1).value() == '=') { consume(); consume(); tokens.push_back({TokenType::ge, line_cnt}); }
            else if (peek().value() == '<' && peek(1).has_value() && peek(1).value() == '=') { consume(); consume(); tokens.push_back({TokenType::le, line_cnt}); }
            else if (peek().value() == '>' ) { consume(); tokens.push_back({TokenType::gt, line_cnt}); }
            else if (peek().value() == '<' ) { consume(); tokens.push_back({TokenType::lt, line_cnt}); }
            else if (peek().value() == '=' && peek(1).has_value() && peek(1).value() == '=') { consume(); consume(); tokens.push_back({TokenType::eq_eq, line_cnt}); }
            else if (peek().value() == '=' ) { consume(); tokens.push_back({TokenType::eq, line_cnt}); }
            else if (peek().value() == '\n') { consume(); line_cnt++; }
            else if (std::isspace(peek().value())) { consume(); }
            else {
                std::cerr << "Invalid token\n";
                exit(EXIT_FAILURE);
            }
        }

        m_index = 0; // reset index after tokenization
        return tokens;
    }

private:
    /**
     * Peeks at the character at the current index + offset.
     * Returns nullopt if out of bounds.
     */
    inline std::optional<char> peek(int offset = 0) const {
        if (m_index + offset >= m_src.length()) {
            return {};
        } else {
            return m_src.at(m_index + offset);
        }
    }

    /**
     * Consumes the current character and advances the index.
     */
    inline char consume() {
        return m_src.at(m_index++);
    }

    const std::string m_src;  // source code to tokenize
    size_t m_index = 0;       // current position in the source string
};
