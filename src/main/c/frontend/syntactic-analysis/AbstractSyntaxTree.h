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
	ENTITY_COURSE
} EntityType;

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

typedef struct Professor
{
	char *id;
	Attribute *attributes;
	struct Professor *next; // para lista de profesores
} Professor;

typedef struct Course
{
	char *id;
	Attribute *attributes;
	struct Course *next; // para lista de cursos
} Course;

typedef struct Entity
{
	union
	{
		Professor *professor;
		Course *course;
	};
	struct Entity *next; // lista general de entidades
	EntityType type;	 // tipo de entidad (profesor o curso)
} Entity;

typedef struct Program
{
	Entity *entities; // lista enlazada de entidades
} Program;

/**
 * Node recursive destructors.
 */
void releaseAttribute(struct Attribute *attribute);
void releaseProfessor(struct Professor *professor);
void releaseCourse(struct Course *course);
void releaseEntity(struct Entity *entity);
void releaseProgram(Program *program);

#endif
