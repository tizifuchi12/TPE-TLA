%{

#include "BisonActions.h"

%}

// You touch this, and you die.
%define api.value.union.name SemanticValue

%union {
	/** Terminals. */

	int integer;
	Token token;
	Time time;
	DayOfWeek dayOfWeek;
	char * string;

	/** Non-terminals. */

	Program * program; // general program
	Declaration * declarationList; // general declaration
	Declaration * declaration; // general declaration
	Preference * preference; // general preference
	Entity * entity; // professor, course or classroom
	Attribute * attribute; // attribute for course or professor (hours, name, available, etc)
	Attribute * attributeList; // list of attributes for course or professor

	Configuration configuration; // configuration of the university
	UniversityOpen universityOpen; // university open
	ClassDuration classDuration; // class duration
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach. To use this mechanism, the AST must be translated into
 * another structure.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */

/*
%destructor { releaseAttribute($$); } <attribute>
%destructor { releaseEntity($$); } <entity>
%destructor { releaseEntity($$); } <entityList>
%destructor { releaseAttribute($$); } <attributeList>
%destructor { releaseProgram($$); } <program>
*/

/* Terminals. */
%token <token> COURSE
%token <token> PROFESSOR
%token <token> NAME
%token <token> HOURS

%token <token> UNIVERSITY
%token <token> OPEN
%token <token> FROM
%token <token> TO

%token <token> CLASS
%token <token> DURATION
%token <token> BETWEEN
%token <token> AND

%token <token> CLASSROOM
%token <token> BUILDING
%token <token> CAPACITY
%token <token> HAS

%token <token> MONDAY
%token <token> TUESDAY
%token <token> WEDNESDAY
%token <token> THURSDAY
%token <token> FRIDAY
%token <token> EVERYDAY

%token <token> TEACHES
%token <token> TEACH
%token <token> PREFERS
%token <token> ON
%token <token> IN

%token <integer> INTEGER
%token <string> STRING
%token <string> IDENTIFIER

%token <integer> DURATION_HOURS
%token <time> TIME

%token <token> LBRACE
%token <token> RBRACE
%token <token> COLON
%token <token> SEMICOLON

%token <token> UNKNOWN

/** Non-terminals. */
%type <dayOfWeek> dayOfWeek
%type <dayOfWeek> dayOfWeekOrEveryday

%type <declarationList> declarationList
%type <declaration> declaration

%type <entity> entity

%type <preference> preference

/* Attribute validation */
%type <attribute> professorAttribute
%type <attribute> courseAttribute
%type <attribute> classroomAttribute
%type <attribute> nameAttribute

%type <attributeList> professorAttributeList
%type <attributeList> courseAttributeList
%type <attributeList> classroomAttributeList

%type <universityOpen> universityOpen
%type <classDuration> classDuration
%type <configuration> configuration
%type <program> program

/**
 * Precedence and associativity.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program:
	configuration declarationList
	{
		// The program is a list of entities (professors and courses).
		$$ = newProgram(currentCompilerState(), $1, $2);
	}
;

configuration:
	universityOpen classDuration
	{
		$$ = createConfiguration($1, $2);
	}
	| universityOpen
	{
		$$ = createConfigurationWithoutClassDuration($1);
	}
	| classDuration universityOpen
	{
		$$ = createConfiguration($2, $1);
	}
;

preference: 
    IDENTIFIER TEACHES IDENTIFIER FROM TIME TO TIME ON dayOfWeek IN IDENTIFIER SEMICOLON
	{
		Preference *p = createHardPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $3);
		setPreferenceClassroom(p, $11);
		setPreferenceTime(p, $5, $7);
		setPreferenceDay(p, $9);
		$$ = p;
	}
	| IDENTIFIER TEACHES IDENTIFIER FROM TIME TO TIME ON dayOfWeek SEMICOLON
	{
		Preference *p = createHardPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $3);
		setPreferenceTime(p, $5, $7);
		setPreferenceDay(p, $9);
		$$ = p;

	}
	| IDENTIFIER TEACHES IDENTIFIER ON dayOfWeek SEMICOLON
	{
		Preference *p = createHardPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $3);
		setPreferenceDay(p, $5);
		$$ = p;

	}
	| IDENTIFIER PREFERS TO TEACH IDENTIFIER FROM TIME TO TIME ON dayOfWeek IN IDENTIFIER SEMICOLON
	{
		Preference *p = createSoftPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $5);
		setPreferenceClassroom(p, $13);
		setPreferenceTime(p, $7, $9);
		setPreferenceDay(p, $11);
		$$ = p;

	}
	| IDENTIFIER PREFERS TO TEACH IDENTIFIER FROM TIME TO TIME ON dayOfWeek SEMICOLON
	{
		Preference *p = createSoftPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $5);
		setPreferenceTime(p, $7, $9);
		setPreferenceDay(p, $11);
		$$ = p;

	}
	| IDENTIFIER PREFERS TO TEACH IDENTIFIER ON dayOfWeek SEMICOLON
	{
		Preference *p = createSoftPreference();
		setPreferenceProfessor(p, $1);
		setPreferenceCourse(p, $5);
		setPreferenceDay(p, $7);
		$$ = p;
	}
;

universityOpen:
	UNIVERSITY OPEN FROM TIME TO TIME SEMICOLON
	{
		$$.openFrom = $4;
		$$.openTo = $6;
	}
;

classDuration:
	CLASS DURATION BETWEEN DURATION_HOURS AND DURATION_HOURS SEMICOLON
	{
		$$.minHours = $4;
		$$.maxHours = $6;
	}
;

declarationList:
	declarationList declaration
	{
		$$ = appendDeclaration($1, $2);
	}
	| 
	{
		$$ = newDeclarationList();
	}
;

declaration:
	entity
	{
		$$ = createEntityDeclaration($1);
	}
	| preference
	{
		$$ = createPreferenceDeclaration($1);
	}
;

entity:
	PROFESSOR IDENTIFIER LBRACE professorAttributeList RBRACE
	{
		$$ = createProfessor($2, $4);
	}
	| COURSE IDENTIFIER LBRACE courseAttributeList RBRACE
	{
		$$ = createCourse($2, $4);
	}
	| CLASSROOM IDENTIFIER LBRACE classroomAttributeList RBRACE
	{
		$$ = createClassroom($2, $4);
	}
;

professorAttributeList:
	professorAttributeList professorAttribute
	{
		$$ = appendAttribute($1, $2);
	}
	| 
	{
		$$ = newAttributeList();
	}
;

courseAttributeList:
	courseAttributeList courseAttribute
	{
		$$ = appendAttribute($1, $2);
	}
	|
	{
		$$ = newAttributeList();
	}
;

classroomAttributeList:
	classroomAttributeList classroomAttribute
	{
		$$ = appendAttribute($1, $2);
	}
	| 
	{
		$$ = newAttributeList();
	}
;

nameAttribute:
	NAME COLON STRING SEMICOLON
	{
		$$ = createStringAttribute("name", $3);
	}

professorAttribute:
	nameAttribute
;

courseAttribute:
	nameAttribute
	| HOURS COLON INTEGER SEMICOLON
	{
		$$ = createIntAttribute("hours", $3);
	}
;

classroomAttribute:
	nameAttribute
	| BUILDING COLON STRING SEMICOLON
	{
		$$ = createStringAttribute("building", $3);
	}
	| CAPACITY COLON INTEGER SEMICOLON
	{
		$$ = createIntAttribute("capacity", $3);
	}
	| HAS STRING SEMICOLON
	{
		$$ = createStringAttribute("has", $2);
	}
;

dayOfWeek:
    MONDAY    { $$ = DAY_MONDAY; }
  | TUESDAY   { $$ = DAY_TUESDAY; }
  | WEDNESDAY { $$ = DAY_WEDNESDAY; }
  | THURSDAY  { $$ = DAY_THURSDAY; }
  | FRIDAY    { $$ = DAY_FRIDAY; }
;

dayOfWeekOrEveryday:
    dayOfWeek { $$ = $1; }
  | EVERYDAY { $$ = DAY_EVERYDAY; }
;

%%
