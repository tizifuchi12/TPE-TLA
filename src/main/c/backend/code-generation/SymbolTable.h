#ifndef SYMBOL_TABLE_HEADER
#define SYMBOL_TABLE_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../shared/Type.h"

typedef struct {
    char *id;
    EntityType type; // ENTITY_PROFESSOR, ENTITY_COURSE, ENTITY_CLASSROOM
    void *data; // Puntero a ProfessorData, CourseData, o ClassroomData
} Symbol;

typedef struct {
    char *name;
    IntervalDayOfWeek *availability;
    int availabilityCount;
    char **canTeach; // Lista de IDs de cursos
    int canTeachCount;
} ProfessorData;

typedef struct {
    char *name;
    int hours;
    char *requires; // Requisito (e.g., "projector")
} CourseData;

typedef struct {
    char *name;
    char *building;
    int capacity;
    char **has; // Lista de equipamientos
    int hasCount;
} ClassroomData;

typedef struct {
    Symbol *symbols;
    int count;
    int capacity;
} SymbolTable;

typedef struct {
    Time openFrom;
    Time openTo;
    int minHours;
    int maxHours;
    boolean hasClassDuration;
} GlobalConfig;

typedef struct {
    char *courseId;
    char *professorId;
    char *classroomId;
    Time startTime;
    Time endTime;
    DayOfWeek day;
    boolean isHard; // true para teaches, false para prefers
} ScheduleEntry;

void initSymbolTable(SymbolTable *table);
void addSymbol(SymbolTable *table, char *id, EntityType type, void *data);
Symbol *findSymbol(SymbolTable *table, char *id);
void freeSymbolTable(SymbolTable *table);

#endif