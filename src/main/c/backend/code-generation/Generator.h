#ifndef GENERATOR_HEADER
#define GENERATOR_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../shared/CompilerState.h"
#include "../../shared/Logger.h"
#include "../../shared/String.h"
#include "SymbolTable.h"
#include <stdarg.h>
#include <stdio.h>

/** Initialize module's internal state. */
void initializeGeneratorModule();

/** Shutdown module's internal state. */
void shutdownGeneratorModule();

/**
 * Performs semantic analysis on the program.
 */
boolean validateSemantic(Program *program, SymbolTable *table, GlobalConfig *config, ScheduleEntry **schedule, int *scheduleCount);

/**
 * Generates the final HTML output.
 */
void generateCode(Program *program, SymbolTable *table, ScheduleEntry *schedule, int scheduleCount, const char *outputFile);

/**
 * Generates the final output using the current compiler state.
 */
void generate(CompilerState *compilerState);

#endif