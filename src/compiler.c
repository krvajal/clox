#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "object.h"
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
    printf("Error %s\n", message);
    parser.hadError = true;
    parser.panicMode = true;
}

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();

    return true;
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
    while (precedence < getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }
static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(global);
}

static void statement();
static void declaration();
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
        case TOKEN_BANG:
            emitByte(OP_NOT);
        default:
            break;
    }
}

static void string() {
    char* stringStart = parser.previous.start + 1;
    int stringLength = parser.previous.length - 2;
    emitConstant(OBJ_VAL(copyString(stringStart, stringLength)));
}

static void namedVariable(Token name) {
    uint8_t arg = identifierConstant(&name);
    emitBytes(OP_GET_GLOBAL, arg);
}

static void variable() { namedVariable(parser.previous); }

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
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
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

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    // [TOKEN_SEMICOLON] ://
};

static ParseRule* getRule(TokenType tokenType) { return &rules[tokenType]; }

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_PRINT);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_POP);
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else {
        expressionStatement();
    }
}

void synchronize() {
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: {
                ;  // do nothing
            }
        }
        advance();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
};

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    compilingChunk = chunk;
    advance();
    while (!match(TOKEN_EOF)) {
        printf("parsing loop\n");
        declaration();
    }
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
    // consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
