#ifndef clox_compiler_h
#define clox_compiler_h
#include "chunk.h"
#include "common.h"

bool compile(const char* code, Chunk* chunk);

#endif