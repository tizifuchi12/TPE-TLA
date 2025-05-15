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

void releaseProfessor(Professor *professor)
{
	if (!professor)
		return;
	if (professor->attributes)
		releaseAttribute(professor->attributes);
	free(professor);
}

void releaseCourse(Course *course)
{
	if (!course)
		return;
	if (course->attributes)
		releaseAttribute(course->attributes);
	free(course);
}

void releaseEntity(Entity *entity)
{
	if (!entity)
		return;
	switch (entity->type)
	{
	case ENTITY_PROFESSOR:
		releaseProfessor(entity->professor);
		break;
	case ENTITY_COURSE:
		releaseCourse(entity->course);
		break;
		// TODO: agregar otros tipos si los hubiera
	}
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
