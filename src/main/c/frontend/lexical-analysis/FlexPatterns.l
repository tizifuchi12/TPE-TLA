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
MINUTE          0?{DIGIT}|[1-5]{DIGIT}
TIME            {HOUR}:{MINUTE}
DAY             (monday|tuesday|wednesday|thursday|friday|saturday|sunday)
ID              [a-zA-Z_][a-zA-Z0-9_]*
STRING          \"[^\"]*\"
INT              {DIGIT}+
FLOAT            {DIGIT}+"."{DIGIT}+
DURATION_HOURS   ({FLOAT}|{INT})h

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

"//".*                           { IgnoredLexemeAction(createLexicalAnalyzerContext()); }

"university"                       { return KeywordLexemeAction(createLexicalAnalyzerContext(), UNIVERSITY); }
"is"                               { return KeywordLexemeAction(createLexicalAnalyzerContext(), IS); }
"open"                             { return KeywordLexemeAction(createLexicalAnalyzerContext(), OPEN); }
"from"                             { return KeywordLexemeAction(createLexicalAnalyzerContext(), FROM); }
"to"                               { return KeywordLexemeAction(createLexicalAnalyzerContext(), TO); }
"on"                               { return KeywordLexemeAction(createLexicalAnalyzerContext(), ON); }

"course"                           { return KeywordLexemeAction(createLexicalAnalyzerContext(), COURSE); }
"requires"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), REQUIRES); }
"in"                               { return KeywordLexemeAction(createLexicalAnalyzerContext(), IN); }
"at"                               { return KeywordLexemeAction(createLexicalAnalyzerContext(), AT); }

"classroom"                        { return KeywordLexemeAction(createLexicalAnalyzerContext(), CLASSROOM); }
"has"                              { return KeywordLexemeAction(createLexicalAnalyzerContext(), HAS); }

"professor"                        { return KeywordLexemeAction(createLexicalAnalyzerContext(), PROFESSOR); }
"available"                        { return KeywordLexemeAction(createLexicalAnalyzerContext(), AVAILABLE); }

"duration"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), DURATION); }
"credits"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), CREDITS); }

"preference"                       { return KeywordLexemeAction(createLexicalAnalyzerContext(), PREFERENCE); }
"teaches"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), TEACHES); }

"students"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), STUDENTS); }
"require"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), REQUIRE); }
"can"                              { return KeywordLexemeAction(createLexicalAnalyzerContext(), CAN); }
"teach"                            { return KeywordLexemeAction(createLexicalAnalyzerContext(), TEACH); }
"everyday"                         { return KeywordLexemeAction(createLexicalAnalyzerContext(), EVERYDAY); }
"after"                            { return KeywordLexemeAction(createLexicalAnalyzerContext(), AFTER); }
"preferences"                      { return KeywordLexemeAction(createLexicalAnalyzerContext(), PREFERENCES); }
"prefers"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), PREFERS); }

"class"                            { return KeywordLexemeAction(createLexicalAnalyzerContext(), CLASS); }
"between"                          { return KeywordLexemeAction(createLexicalAnalyzerContext(), BETWEEN); }
"and"                              { return KeywordLexemeAction(createLexicalAnalyzerContext(), AND); }

{DAY}                              { return DayLexemeAction(createLexicalAnalyzerContext()); }
{TIME}                             { return TimeLexemeAction(createLexicalAnalyzerContext()); }
{STRING}                           { return StringLiteralLexemeAction(createLexicalAnalyzerContext()); }
{DURATION_HOURS}                   { return DurationLexemeAction(createLexicalAnalyzerContext()); }

"="                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), EQUAL); }
"{"                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), OPEN_BRACE); }
"}"                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), CLOSE_BRACE); }
","                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), COMMA); }
":"                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), COLON); }
"."                                { return SymbolLexemeAction(createLexicalAnalyzerContext(), DOT); }

{ID}                               { return IdentifierLexemeAction(createLexicalAnalyzerContext()); }

[[:space:]]+						{ IgnoredLexemeAction(createLexicalAnalyzerContext()); }
.									{ return UnknownLexemeAction(createLexicalAnalyzerContext()); }

%%

#include "FlexExport.h"
