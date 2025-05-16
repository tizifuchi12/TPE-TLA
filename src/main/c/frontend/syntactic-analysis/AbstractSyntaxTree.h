#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../shared/Logger.h"
#include <stdlib.h>

/** Initialize module's internal state. */
void initializeAbstractSyntaxTreeModule();

/** Shutdown module's internal state. */
void shutdownAbstractSyntaxTreeModule();

typedef enum
{
	ENTITY_PROFESSOR,
	ENTITY_COURSE,
	ENTITY_CLASSROOM
} EntityType;

typedef enum
{
	HARD_PREFERENCE,
	SOFT_PREFERENCE
} PreferenceType;

typedef enum
{
	ATTR_INTERVAL,
	ATTR_STRING,
	ATTR_INT
} AttributeType;

typedef enum
{
	DECLARATION_ENTITY,
	DECLARATION_PREFERENCE,
	DECLARATION_DEMAND
} DeclarationType;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

typedef struct Attribute
{
	char *key; // "name", "hours", "available"
	union
	{
		char *strValue;
		int intValue;
		IntervalDayOfWeek intervalValue;
	};
	AttributeType attributeType;
	struct Attribute *next; // para lista enlazada simple
} Attribute;

typedef struct UniversityOpen
{
	Time openFrom;
	Time openTo;
} UniversityOpen;

typedef struct ClassDuration
{
	int minHours;
	int maxHours;
} ClassDuration;

typedef struct
{
	UniversityOpen universityOpen;
	ClassDuration classDuration;
	boolean hasClassDuration;
} Configuration;

typedef struct Entity
{
	char *id;			   // id de la entidad (nombre del profesor, nombre del curso o nombre del aula)
	Attribute *attributes; // lista enlazada de atributos
	EntityType type;	   // tipo de entidad (profesor, curso o aula)
} Entity;

typedef struct PreferenceDetails
{
	char *professorId;
	char *courseId;
	char *classroomId;
	Time startTime;
	Time endTime;
	DayOfWeek day;
	boolean hasTime;
	boolean hasDay;
} PreferenceDetails;

typedef struct Preference
{
	PreferenceDetails *details; // detalles de la preferencia
	PreferenceType type;		// tipo de preferencia (dura o blanda)
} Preference;

typedef struct Demand
{
	char *courseId;
	int students;
} Demand;

typedef struct Declaration
{
	DeclarationType type;
	union
	{
		Entity *entity;
		Preference *preference;
		Demand *demand;
	};
	struct Declaration *next; // para lista enlazada simple
} Declaration;

typedef struct Program
{
	Configuration configuration; // configuracion de la universidad
	Declaration *declarations;	 // lista enlazada de declaraciones
} Program;

/**
 * Node recursive destructors.
 */
void releaseAttribute(Attribute *attribute);
void releasePreference(Preference *preference);
void releaseEntity(Entity *entity);
void releaseDeclaration(Declaration *declaration);
void releaseProgram(Program *program);

#endif
