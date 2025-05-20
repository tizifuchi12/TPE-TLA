#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../shared/CompilerState.h"
#include "../../shared/Logger.h"
#include "../../shared/Type.h"
#include "AbstractSyntaxTree.h"
#include "SyntacticAnalyzer.h"
#include <stdlib.h>

/** Initialize module's internal state. */
void initializeBisonActionsModule();

/** Shutdown module's internal state. */
void shutdownBisonActionsModule();

/**
 * Bison semantic actions.
 */

Program *newProgram(CompilerState *compilerState, Configuration configuration, Declaration *declarations);

Declaration *newDeclarationList();
Declaration *appendDeclaration(Declaration *head, Declaration *newDeclaration);
Declaration *createEntityDeclaration(Entity *entity);
Declaration *createPreferenceDeclaration(Preference *preference);
Declaration *createDemandDeclaration(Demand *demand);

Demand *createDemand(char *courseId, int students);

Preference *createHardPreference();
Preference *createSoftPreference();

void setPreferenceProfessor(Preference *preference, char *professorId);
void setPreferenceCourse(Preference *preference, char *courseId);
void setPreferenceClassroom(Preference *preference, char *classroomId);
void setPreferenceTime(Preference *preference, Time startTime, Time endTime);
void setPreferenceDay(Preference *preference, DayOfWeek days);

Entity *createProfessor(char *id, Attribute *attributes);
Entity *createCourse(char *id, Attribute *attributes);
Entity *createClassroom(char *id, Attribute *attributes);

Attribute *newAttributeList();
Attribute *appendAttribute(Attribute *head, Attribute *newAttribute);
Attribute *createIntAttribute(char *key, int value);
Attribute *createStringAttribute(char *key, char *value);
Attribute *createIntervalAttribute(char *key, IntervalDayOfWeek interval);

Configuration createConfiguration(UniversityOpen universityOpen, ClassDuration classDuration);
Configuration createConfigurationWithoutClassDuration(UniversityOpen universityOpen);

#endif
