#include "scanner.h"

#include <string.h>

#include "common.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.current != expected) return false;
    scanner.current++;
    return true;
}

static char peek() { return *scanner.current; }
static char peekNext() {
    if (isAtEnd()) return '\0';
    return scanner.current[1];
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType checkKeyword(int start, int length, const char* rest,
                              TokenType type) {
    int tokenLength = scanner.current - scanner.start;
    const expectedLength = start + length;

    if (tokenLength == expectedLength &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identiferType() {
    switch (scanner.start[0]) {
        case 'a':
            return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e':
            return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i':
            return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n':
            return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o':
            return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p':
            return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h':
                        return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r':
                        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v':
            return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return checkKeyword(1, 4, "hile", TOKEN_WHILE);

        default:
            return TOKEN_IDENTIFIER;
    }
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek())) {
        advance();
    }
    return makeToken(identiferType());
}

static Token number() {
    while (isDigit(peek())) {
        advance();
    }
    if (peek() == '.' && isDigit(peekNext())) advance();
    while (isDigit(peek())) {
        advance();
    }
    return makeToken(TOKEN_NUMBER);
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }
    // eat the closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

void skipWhitespace() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
            continue;
        }
        if (c == '\n') {
            scanner.line += 1;
            advance();
            continue;
        }
        if (c == '/') {
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) {
                    advance();
                }
                continue;
            } else {
                return;
            }
        }
        return;
    }
}

Token errorToken(const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

bool isAtEnd() { return *(scanner.current) == '\0'; }

Token makeToken(TokenType type) {
    Token token;
    token.line = scanner.line;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    return token;
}

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
    if (isAlpha(c)) {
        return identifier();
    }
    if (isDigit(c)) {
        return number();
    }
    switch (c) {
        case '(':
            return makeToken(TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(TOKEN_SEMICOLON);
        case ',':
            return makeToken(TOKEN_COMMA);
        case '.':
            return makeToken(TOKEN_DOT);
        case '-':
            return makeToken(TOKEN_MINUS);
        case '+':
            return makeToken(TOKEN_PLUS);
        case '/':
            return makeToken(TOKEN_SLASH);
        case '*':
            return makeToken(TOKEN_STAR);
        case '!':
            return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '<':
            return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '"':
            return string();
        default:
            break;
    }
    return errorToken("Unexpected character.");
}