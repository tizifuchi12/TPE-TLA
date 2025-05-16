#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger *_logger = NULL;

void initializeAbstractSyntaxTreeModule()
{
	_logger = createLogger("AbstractSyntxTree");
}

void shutdownAbstractSyntaxTreeModule()
{
	if (_logger != NULL)
	{
		destroyLogger(_logger);
	}
}

/** PUBLIC FUNCTIONS */

void releaseAttribute(Attribute *attribute)
{
	if (!attribute)
		return;
	if (attribute->next)
		releaseAttribute(attribute->next);

	free(attribute);
}

void releaseEntity(Entity *entity)
{
	if (!entity)
		return;
	if (entity->attributes)
		releaseAttribute(entity->attributes);
	if (entity->next)
		releaseEntity(entity->next);

	free(entity);
}

void releaseProgram(Program *program)
{
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL)
	{
		releaseEntity(program->entities);
		free(program);
	}
}
