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
	Entity * entityList; // list of entities (professors and courses)
	Entity * entity; // professor or course
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
%token <token> COMMA
%token <token> LBRACKET
%token <token> RBRACKET

%token <token> UNKNOWN

/** Non-terminals. */
%type <entityList> entityList
%type <entity> entity
%type <attribute> attribute
%type <attributeList> attributeList
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
	configuration entityList
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

entityList:
	entityList entity
	{
		$$ = appendEntity($1, $2);
	}
	| /* empty */
	{
		$$ = newEntityList();
	}
;

entity:
	PROFESSOR IDENTIFIER LBRACE attributeList RBRACE
	{
		$$ = createProfessor($2, $4);
	}
	| COURSE IDENTIFIER LBRACE attributeList RBRACE
	{
		$$ = createCourse($2, $4);
	}
	| CLASSROOM IDENTIFIER LBRACE attributeList RBRACE
	{
		$$ = createClassroom($2, $4);
	}
;

attributeList:
	attributeList attribute
	{
		$$ = appendAttribute($1, $2);
	}
	| 
	{
		$$ = newAttributeList();
	}
;

attribute:
    NAME COLON STRING SEMICOLON
    {
        $$ = createStringAttribute("name", $3);
    }
  	| HOURS COLON INTEGER SEMICOLON
    {
        $$ = createIntAttribute("hours", $3);
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
