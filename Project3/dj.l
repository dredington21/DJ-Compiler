/* LEXER for DJ by Daniel Redington*/

%option yylineno
%option noyywrap

%{
  #define DEBUG 0
  typedef int Token;
  Token scanned(Token t);
%}

/* Regular Expressions */
digit      0|([1-9][0-9]*)
identifier [a-zA-Z_][a-zA-Z0-9_]*
wspace     [ \t\r\n]+

%%
/* This section detects each of the possible token types */
"final" { return scanned(FINAL); }
"class" { return scanned(CLASS); }
"extends" { return scanned(EXTENDS); }
"main" { return scanned(MAIN); }
"nat" { return scanned(NATTYPE); }
{digit}+ { return scanned(NATLITERAL); }
"printNat" { return scanned(PRINTNAT); }
"readNat" { return scanned(READNAT); }
"\+" { return scanned(PLUS); }
"-" { return scanned(MINUS); }
"\*" { return scanned(TIMES); }
"==" { return scanned(EQUALITY); }
"<" { return scanned(LESS); }
"assert" { return scanned(ASSERT); }
"||" { return scanned(OR); }
"!" { return scanned(NOT); }
"if" { return scanned(IF); }
"else" { return scanned(ELSE); }
"while" { return scanned(WHILE); }
"=" { return scanned(ASSIGN); }
"null" { return scanned(NUL); }
"new" { return scanned(NEW); }
"this" { return scanned(THIS); }
"\." { return scanned(DOT); }
";" { return scanned(SEMICOLON); }
"{" { return scanned(LBRACE); }
"}" { return scanned(RBRACE); }
"\(" { return scanned(LPAREN); }
"\)" { return scanned(RPAREN); }
{identifier} { return scanned(ID); }
<<EOF>> { return scanned(ENDOFFILE); }

{wspace}    { /* skip whitespace */ }
"//".*      { /* skip single-line comments */ }
.           { if(DEBUG) printf("\n");
              printf("Lex error on line %d: Illegal character %s\n", yylineno, yytext);
              exit(-1);
            }
%%

/* process and return scanned token t */
Token scanned(Token t) {
  if(DEBUG==0) return t;
  switch(t) {
    case FINAL: printf("FINAL "); return t;
    case CLASS: printf("CLASS "); return t;
    case EXTENDS: printf("EXTENDS "); return t;
    case MAIN: printf("MAIN "); return t;
    case NATTYPE: printf("NATTYPE "); return t;
    case NATLITERAL: printf("NATLITERAL(%s) ", yytext); return t;
    case PRINTNAT: printf("PRINTNAT "); return t;
    case READNAT: printf("READNAT "); return t;
    case PLUS: printf("PLUS "); return t;
    case MINUS: printf("MINUS "); return t;
    case TIMES: printf("TIMES "); return t;
    case EQUALITY: printf("EQUALITY "); return t;
    case LESS: printf("LESS "); return t;
    case ASSERT: printf("ASSERT "); return t;
    case OR: printf("OR "); return t;
    case NOT: printf("NOT "); return t;
    case IF: printf("IF "); return t;
    case ELSE: printf("ELSE "); return t;
    case WHILE: printf("WHILE "); return t;
    case ASSIGN: printf("ASSIGN "); return t;
    case NUL: printf("NUL "); return t;
    case NEW: printf("NEW "); return t;
    case THIS: printf("THIS "); return t;
    case DOT: printf("DOT "); return t;
    case SEMICOLON: printf("SEMICOLON "); return t;
    case LBRACE: printf("LBRACE "); return t;
    case RBRACE: printf("RBRACE "); return t;
    case LPAREN: printf("LPAREN "); return t;
    case RPAREN: printf("RPAREN "); return t;
    case ID: printf("ID(%s) ", yytext); return t;
    case ENDOFFILE: printf("ENDOFFILE\n"); return t;
    default: printf("ERROR: invalid token in scanned().\n"); exit(-1);
  }
}
