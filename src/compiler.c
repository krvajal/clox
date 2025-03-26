#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();
typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static Chunk* currentChunk() { return compilingChunk; }

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
    }
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t b1, uint8_t b2) {
    emitByte(b1);
    emitByte(b2);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        // error()
        return 0;
    }
    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void errorAtCurrent(const char* message) {
    // TODO
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void expression();
static ParseRule* getRule(TokenType type);

void emitReturn() { emitByte(OP_RETURN); }
void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void error(const char* message) {
    parser.hadError = true;
    printf("%s\n", message);
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expected expression.");
        return;
    }
    prefixRule();

    // get the rule for the current token
    // if the rule precedence is higher that the current precedence
    // parse the infix rule
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ') after expression.");
}

static void unary() {
    TokenType operatorType = parser.previous.type;
    expression();
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            break;
    }
}

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));
    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        default:
            return;
    }
}

void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            return emitByte(OP_FALSE);

        case TOKEN_TRUE:
            return emitByte(OP_TRUE);
        case TOKEN_NIL:
            return emitByte(OP_NIL);
    }
}

ParseRule rules[] = {[TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
                     [TOKEN_SLASH] = {NULL, binary, PREC_NONE},
                     [TOKEN_MINUS] = {unary, binary, PREC_TERM},
                     [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
                     [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
                     [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
                     [TOKEN_BANG] = {unary, NULL, PREC_NONE},
                     [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
                     [TOKEN_IF] = {NULL, NULL, PREC_NONE},
                     [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
                     [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
                     [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
                     [TOKEN_NIL] = {literal, NULL, PREC_NONE}};

static ParseRule* getRule(TokenType tokenType) { return &rules[tokenType]; }

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    advance();
    expression();
    // int line = -1;
    // for (;;) {
    //     Token token = scanToken();
    //     if (token.line != line) {
    //         printf("%4d ", token.line);
    //         line = token.line;
    //     } else {
    //         printf("   | ");
    //     }
    //     printf("%2d '%.*s'\n", token.type, token.length, token.start);
    //     if (token.type == TOKEN_EOF) {
    //         break;
    //     }
    // }
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
