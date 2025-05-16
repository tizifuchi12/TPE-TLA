#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static Logger *_logger = NULL;

void initializeBisonActionsModule()
{
	_logger = createLogger("BisonActions");
}

void shutdownBisonActionsModule()
{
	if (_logger != NULL)
	{
		destroyLogger(_logger);
	}
}

/** IMPORTED FUNCTIONS */

extern unsigned int flexCurrentContext(void);

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char *functionName);
static Entity *_createEntity(EntityType type, char *id, Attribute *attributes);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char *functionName)
{
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

Attribute *createIntAttribute(char *key, int value)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Attribute *attribute = calloc(1, sizeof(Attribute));
	attribute->key = key;
	attribute->intValue = value;
	attribute->isInt = true;
	attribute->next = NULL;
	return attribute;
}

Attribute *createStringAttribute(char *key, char *value)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Attribute *attribute = calloc(1, sizeof(Attribute));
	attribute->key = key;
	attribute->strValue = value;
	attribute->isInt = false;
	attribute->next = NULL;
	return attribute;
}

Attribute *appendAttribute(Attribute *head, Attribute *newAttribute)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (head == NULL)
	{
		return newAttribute;
	}
	Attribute *current = head;
	while (current->next != NULL)
	{
		current = current->next;
	}
	current->next = newAttribute;
	return head;
}

Attribute *newAttributeList()
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return NULL;
}

Entity *newEntityList()
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return NULL;
}

Entity *createCourse(char *id, Attribute *attributes)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return _createEntity(ENTITY_COURSE, id, attributes);
}

Entity *createProfessor(char *id, Attribute *attributes)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return _createEntity(ENTITY_PROFESSOR, id, attributes);
}

Entity *createClassroom(char *id, Attribute *attributes)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return _createEntity(ENTITY_CLASSROOM, id, attributes);
}

Entity *appendEntity(Entity *head, Entity *newEntity)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (head == NULL)
	{
		return newEntity;
	}
	Entity *current = head;
	while (current->next != NULL)
	{
		current = current->next;
	}
	current->next = newEntity;
	return head;
}

Configuration createConfiguration(UniversityOpen universityOpen, ClassDuration classDuration)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Configuration configuration;
	configuration.universityOpen = universityOpen;
	configuration.classDuration = classDuration;
	configuration.hasClassDuration = true;
	return configuration;
}

Configuration createConfigurationWithoutClassDuration(UniversityOpen universityOpen)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Configuration configuration;
	configuration.universityOpen = universityOpen;
	configuration.hasClassDuration = false;
	return configuration;
}

Program *newProgram(CompilerState *compilerState, Configuration configuration, Entity *entities)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program *program = calloc(1, sizeof(Program));
	program->configuration = configuration;
	program->entities = entities;
	compilerState->abstractSyntaxtTree = program;
	if (0 < flexCurrentContext())
	{
		logError(_logger, "The final context is not the default (0): %d", flexCurrentContext());
		compilerState->succeed = false;
	}
	else
	{
		compilerState->succeed = true;
	}
	return program;
}

static Entity *_createEntity(EntityType type, char *id, Attribute *attributes)
{
	Entity *entity = calloc(1, sizeof(Entity));
	entity->id = id;
	entity->attributes = attributes;
	entity->type = type;
	entity->next = NULL;
	return entity;
}
