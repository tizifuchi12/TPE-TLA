#include "Generator.h"

/* MODULE INTERNAL STATE */

const char _indentationCharacter = ' ';
const char _indentationSize = 4;
static Logger *_logger = NULL;

void initializeGeneratorModule()
{
	_logger = createLogger("Generator");
}

void shutdownGeneratorModule()
{
	if (_logger != NULL)
	{
		destroyLogger(_logger);
	}
}

/** PRIVATE FUNCTIONS */

/** PUBLIC FUNCTIONS */

void generate(CompilerState *compilerState)
{
	logDebugging(_logger, "Generating final output...");
	//_generatePrologue();
	//_generateProgram(compilerState->abstractSyntaxtTree);
	//_generateEpilogue(compilerState->value);
	logDebugging(_logger, "Generation is done.");
}
