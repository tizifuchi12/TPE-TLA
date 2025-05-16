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
	char * string;

	/** Non-terminals. */

	Program * program; // general program
	Declaration * declarationList; // general declaration
	Declaration * declaration; // general declaration
	Entity * entityList; // list of entities (professors and courses)
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
%type <declarationList> declarationList
%type <declaration> declaration

%type <entity> entity

/* Attribute validation */
%type <attribute> professorAttribute
%type <attribute> courseAttribute
%type <attribute> classroomAttribute
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

professorAttribute:
	NAME COLON STRING SEMICOLON
	{
		$$ = createStringAttribute("name", $3);
	}
;

courseAttribute:
	NAME COLON STRING SEMICOLON
	{
		$$ = createStringAttribute("name", $3);
	}
	| HOURS COLON INTEGER SEMICOLON
	{
		$$ = createIntAttribute("hours", $3);
	}
;

classroomAttribute:
	NAME COLON STRING SEMICOLON
    {
        $$ = createStringAttribute("name", $3);
    }
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

%%
