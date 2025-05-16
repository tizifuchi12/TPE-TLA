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
static Preference *_createPreference(PreferenceType type);

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
	attribute->attributeType = ATTR_INT;
	attribute->next = NULL;
	return attribute;
}

Attribute *createStringAttribute(char *key, char *value)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Attribute *attribute = calloc(1, sizeof(Attribute));
	attribute->key = key;
	attribute->strValue = value;
	attribute->attributeType = ATTR_STRING;
	attribute->next = NULL;
	return attribute;
}

Attribute *createIntervalAttribute(char *key, IntervalDayOfWeek interval)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Attribute *attribute = calloc(1, sizeof(Attribute));
	attribute->key = key;
	attribute->intervalValue = interval;
	attribute->attributeType = ATTR_INTERVAL;
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

Program *newProgram(CompilerState *compilerState, Configuration configuration, Declaration *declarations)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program *program = calloc(1, sizeof(Program));
	program->configuration = configuration;
	program->declarations = declarations;
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

Declaration *newDeclarationList()
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return NULL;
}

Declaration *appendDeclaration(Declaration *head, Declaration *newDeclaration)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (head == NULL)
	{
		return newDeclaration;
	}
	Declaration *current = head;
	while (current->next != NULL)
	{
		current = current->next;
	}
	current->next = newDeclaration;
	return head;
}

Declaration *createEntityDeclaration(Entity *entity)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Declaration *declaration = calloc(1, sizeof(Declaration));
	declaration->type = DECLARATION_ENTITY;
	declaration->entity = entity;
	declaration->next = NULL;
	return declaration;
}

Declaration *createPreferenceDeclaration(Preference *preference)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Declaration *declaration = calloc(1, sizeof(Declaration));
	declaration->type = DECLARATION_PREFERENCE;
	declaration->preference = preference;
	declaration->next = NULL;
	return declaration;
}

Preference *createHardPreference()
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Preference *preference = _createPreference(HARD_PREFERENCE);
	return preference;
}
Preference *createSoftPreference()
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Preference *preference = _createPreference(SOFT_PREFERENCE);
	return preference;
}

void setPreferenceProfessor(Preference *preference, char *professorId)
{
	preference->details->professorId = professorId;
}

void setPreferenceCourse(Preference *preference, char *courseId)
{
	preference->details->courseId = courseId;
}

void setPreferenceClassroom(Preference *preference, char *classroomId)
{
	preference->details->classroomId = classroomId;
}

void setPreferenceTime(Preference *preference, Time startTime, Time endTime)
{
	preference->details->startTime = startTime;
	preference->details->endTime = endTime;
	preference->details->hasTime = true;
}

void setPreferenceDay(Preference *preference, DayOfWeek day)
{
	preference->details->day = day;
	preference->details->hasDay = true;
}

Declaration *createDemandDeclaration(Demand *demand)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Declaration *declaration = calloc(1, sizeof(Declaration));
	declaration->type = DECLARATION_DEMAND;
	declaration->demand = demand;
	declaration->next = NULL;
	return declaration;
}

Demand *createDemand(char *courseId, int students)
{
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Demand *demand = calloc(1, sizeof(Demand));
	demand->courseId = courseId;
	demand->students = students;
	return demand;
}

static Preference *_createPreference(PreferenceType type)
{
	Preference *preference = calloc(1, sizeof(Preference));
	preference->type = type;
	preference->details = calloc(1, sizeof(PreferenceDetails));
	preference->details->hasTime = false;
	preference->details->hasDay = false;
	preference->details->professorId = NULL;
	preference->details->courseId = NULL;
	preference->details->classroomId = NULL;
	return preference;
}

static Entity *_createEntity(EntityType type, char *id, Attribute *attributes)
{
	Entity *entity = calloc(1, sizeof(Entity));
	entity->id = id;
	entity->attributes = attributes;
	entity->type = type;
	return entity;
}