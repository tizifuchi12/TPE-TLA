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
	DECLARATION_ENTITY,
	DECLARATION_PREFERENCE
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
	};
	int isInt;				// flag para saber qué tipo tiene
	struct Attribute *next; // para lista enlazada simple
} Attribute;

typedef struct
{
	Time openFrom;
	Time openTo;
} UniversityOpen;

typedef struct
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

typedef struct Declaration
{
	DeclarationType type;
	union
	{
		Entity *entity;
		Preference *preference;
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
void releaseAttribute(struct Attribute *attribute);
void releaseDeclaration(Declaration *declaration);
void releaseProgram(Program *program);

#endif
