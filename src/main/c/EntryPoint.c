#include "backend/code-generation/Generator.h" 
#include "frontend/lexical-analysis/FlexActions.h" 
#include "frontend/syntactic-analysis/AbstractSyntaxTree.h" 
#include "frontend/syntactic-analysis/BisonActions.h" 
#include "frontend/syntactic-analysis/SyntacticAnalyzer.h" 
#include "shared/CompilerState.h" 
#include "shared/Environment.h" 
#include "shared/Logger.h" 
#include "shared/String.h"

const int main(const int argc, const char **argv) 
{ Logger *logger = createLogger("EntryPoint"); 
	initializeFlexActionsModule(); 
	initializeBisonActionsModule(); initializeSyntacticAnalyzerModule(); 
	initializeAbstractSyntaxTreeModule(); initializeGeneratorModule();

for (int i = 0; i < argc; ++i) {
    logDebugging(logger, "Argument %d: \"%s\"", i, argv[i]);
}

CompilerState compilerState = {
    .abstractSyntaxtTree = NULL,
    .succeed = false,
    .value = 0
};
const SyntacticAnalysisStatus syntacticAnalysisStatus = parse(&compilerState);
CompilationStatus compilationStatus = 1;

if (syntacticAnalysisStatus == ACCEPT) {
    logDebugging(logger, "Starting backend...");
    generate(&compilerState);
    if (compilerState.succeed) {
        compilationStatus = 0;
    } else {
        logError(logger, "Backend generation failed.");
    }
    logDebugging(logger, "Releasing AST resources...");
    releaseProgram(compilerState.abstractSyntaxtTree);
} else {
    logError(logger, "Syntactic analysis phase rejected the input program.");
    compilationStatus = 1;
}

logDebugging(logger, "Releasing modules...");
shutdownGeneratorModule();
shutdownAbstractSyntaxTreeModule();
shutdownSyntacticAnalyzerModule();
shutdownBisonActionsModule();
shutdownFlexActionsModule();
logDebugging(logger, "Compilation completed.");
destroyLogger(logger);
return compilationStatus;

}