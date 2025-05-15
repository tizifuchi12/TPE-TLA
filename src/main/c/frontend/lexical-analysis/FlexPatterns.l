%{

#include "FlexActions.h"

%}

/**
 * Enable start condition manipulation functions.
 */
%option stack

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

DIGIT           [0-9]
HOUR            0?{DIGIT}|1{DIGIT}|2[0-3]
MINUTE          [0-5]{DIGIT}
TIME            {HOUR}:{MINUTE}
DAY             (MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY|SUNDAY|EVERYDAY)
ID              [a-zA-Z_][a-zA-Z0-9_]*
STRING          \"[^\"]*\"
INT              {DIGIT}+
FLOAT            {DIGIT}+"."{DIGIT}+
DURATION_HOURS   {INT}h

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */

%%

"/*"								{ BEGIN(MULTILINE_COMMENT); BeginMultilineCommentLexemeAction(createLexicalAnalyzerContext()); }
<MULTILINE_COMMENT>"*/"				{ EndMultilineCommentLexemeAction(createLexicalAnalyzerContext()); BEGIN(INITIAL); }
<MULTILINE_COMMENT>[[:space:]]+		{ IgnoredLexemeAction(createLexicalAnalyzerContext()); }
<MULTILINE_COMMENT>[^*]+			{ IgnoredLexemeAction(createLexicalAnalyzerContext()); }
<MULTILINE_COMMENT>.				{ IgnoredLexemeAction(createLexicalAnalyzerContext()); }

"//".*                              { IgnoredLexemeAction(createLexicalAnalyzerContext()); }

"course"                        { return KeywordLexemeAction(createLexicalAnalyzerContext(), COURSE); }
"professor"                     { return KeywordLexemeAction(createLexicalAnalyzerContext(), PROFESSOR); }
"name"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), NAME); }
"hours"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), HOURS); }

"university"                    { return KeywordLexemeAction(createLexicalAnalyzerContext(), UNIVERSITY); }
"open"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), OPEN); }
"from"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), FROM); }
"to"                            { return KeywordLexemeAction(createLexicalAnalyzerContext(), TO); }

"class"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), CLASS); }
"duration"                      { return KeywordLexemeAction(createLexicalAnalyzerContext(), DURATION); }
"between"                       { return KeywordLexemeAction(createLexicalAnalyzerContext(), BETWEEN); }
"and"                           { return KeywordLexemeAction(createLexicalAnalyzerContext(), AND); }

{ID}                            { return IdentifierLexemeAction(createLexicalAnalyzerContext()); }
{STRING}                        { return StringLexemeAction(createLexicalAnalyzerContext()); }  
{INT}                           { return IntegerLexemeAction(createLexicalAnalyzerContext()); }

{TIME}                          { return TimeLexemeAction(createLexicalAnalyzerContext()); }
{DURATION_HOURS}                { return DurationLexemeAction(createLexicalAnalyzerContext()); }

"{"                             { return SymbolLexemeAction(createLexicalAnalyzerContext(), LBRACE); }  
"}"                             { return SymbolLexemeAction(createLexicalAnalyzerContext(), RBRACE); }
":"                             { return SymbolLexemeAction(createLexicalAnalyzerContext(), COLON); }
";"                             { return SymbolLexemeAction(createLexicalAnalyzerContext(), SEMICOLON); }

[[:space:]]+						{ IgnoredLexemeAction(createLexicalAnalyzerContext()); }
.									{ return UnknownLexemeAction(createLexicalAnalyzerContext()); }

%%

#include "FlexExport.h"
