/* DJ PARSER */
%code provides {
  #include "lex.yy.c"
  #include "ast.h"
  #include "stdio.h"
  #define YYSTYPE ASTree *


  ASTree *pgmAST;

  /* Function for printing generic syntax-error messages */
  void yyerror(const char *str) {
    printf("Syntax error on line %d at token %s\n", yylineno, yytext);
    printf("(This version of the compiler exits after finding the first ");
    printf("syntax error.)\n");
    exit(-1);
  }

}

%token FINAL CLASS ID EXTENDS MAIN NATTYPE 
%token NATLITERAL PRINTNAT READNAT PLUS MINUS TIMES EQUALITY LESS
%token ASSERT OR NOT IF ELSE WHILE
%token ASSIGN NUL NEW THIS DOT 
%token SEMICOLON LBRACE RBRACE LPAREN RPAREN
%token ENDOFFILE

%start pgm

%right ASSERT
%right ASSIGN
%nonassoc LESS
%nonassoc EQUALITY 
%right NOT 
%left OR
%left PLUS MINUS
%left TIMES
%left DOT

%%

pgm:
    class_declarations main_block ENDOFFILE
    {$$ = newAST(PROGRAM, $1, 0, NULL, yylineno); appendToChildrenList($$, $2);}
    {pgmAST =$1; return 0;}
    ;

class_declarations:
    /* empty */ { $$ = newAST(CLASS_DECL_LIST, NULL, 0, NULL, yylineno); }
    | class_declarations class_declaration
      { $$ = newAST(CLASS_DECL_LIST, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
    ;

class_declaration:
    final_class_declaration
    {$$ = newAST(FINAL_CLASS_DECL, $1, 0, NULL, yylineno);}
    | nonfinal_class_declaration
    {$$ = newAST(NONFINAL_CLASS_DECL, $1, 0, NULL, yylineno);}
    ;

final_class_declaration:
    FINAL CLASS id EXTENDS id LBRACE var_declarations method_decl_check RBRACE
    { $$ = newAST(FINAL_CLASS_DECL, $7, 0, $3, yylineno); appendToChildrenList($$, $8); }
    ;

nonfinal_class_declaration:
    CLASS id EXTENDS id LBRACE var_declarations method_decl_check RBRACE
    { $$ = newAST(NONFINAL_CLASS_DECL, $6, 0, $2, yylineno); appendToChildrenList($$, $7); }
    ;

var_declarations:
    /* empty */ { $$ = newAST(VAR_DECL_LIST, NULL, 0, NULL, yylineno); }
    | var_declarations var_declaration
      { $$ = newAST(VAR_DECL_LIST, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
    ;

var_declaration:
    type id SEMICOLON
    { $$ = newAST(VAR_DECL, NULL, 0, $2, yylineno); }
    ;

type:
    NATTYPE { $$ = newAST(NAT_TYPE, NULL, atoi(yytext), NULL, yylineno); }
    | id { $$ = newAST(AST_ID, NULL, 0, $1, yylineno); }
    ;

method_decl_list:
    method_declaration 
    | method_decl_list method_declaration
      { $$ = newAST(METHOD_DECL_LIST, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
    ;

method_decl_check:
    /* empty */ { $$ = newAST(METHOD_DECL_LIST, NULL, 0, NULL, yylineno); }
    | method_decl_list
    ;

method_declaration:
    final_method_declaration
    | nonfinal_method_declaration
    ;

final_method_declaration:
    FINAL type id LPAREN type id RPAREN var_expr_block
    { $$ = newAST(FINAL_METHOD_DECL, $8, 0, $3, yylineno); appendToChildrenList($$, $2); }
    ;

nonfinal_method_declaration:
    type id LPAREN type id RPAREN var_expr_block
    { $$ = newAST(NONFINAL_METHOD_DECL, $7, 0, $2, yylineno); appendToChildrenList($$, $1); }
    ;

var_expr_block:
    LBRACE var_declarations expr_list RBRACE
    { $$ = newAST(EXPR_LIST, $2, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    ;

id:
    ID
    {$$ = newAST(AST_ID, NULL, 0 yytext, yylineno)}
    ;

main_block:
    MAIN var_expr_block
    {$$ = $2}
    ;

expr_list:
    expr SEMICOLON { $$ = newAST(EXPR_LIST, $1, 0, NULL, yylineno); }
    | expr_list expr SEMICOLON
      { appendToChildrenList($1, $2); $$ = $1; }
    ;

expr:
    expr PLUS expr { $$ = newAST(PLUS_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | expr MINUS expr { $$ = newAST(MINUS_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | expr TIMES expr { $$ = newAST(TIMES_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | expr EQUALITY expr { $$ = newAST(EQUALITY_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | expr LESS expr { $$ = newAST(LESS_THAN_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | ASSERT expr { $$ = newAST(ASSERT_EXPR, $2, 0, NULL, yylineno); }
    | NOT expr { $$ = newAST(NOT_EXPR, $2, 0, NULL, yylineno); }
    | expr OR expr { $$ = newAST(OR_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
    | NATLITERAL { $$ = newAST(NAT_LITERAL_EXPR, NULL, atoi(yytext), NULL, yylineno); }
    | NUL { $$ = newAST(NULL_EXPR, NULL, 0, NULL, yylineno); }
    | IF LPAREN expr RPAREN LBRACE expr_list RBRACE ELSE LBRACE expr_list RBRACE
      { $$ = newAST(IF_THEN_ELSE_EXPR, $3, 0, NULL, yylineno); appendToChildrenList($$, $6); appendToChildrenList($$, $10); }
    | WHILE LPAREN expr RPAREN LBRACE expr_list RBRACE
      { $$ = newAST(WHILE_EXPR, $3, 0, NULL, yylineno); appendToChildrenList($$, $6); }
    ;

%%

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: parsedj filename\n");
    exit(-1);
  }
  yyin = fopen(argv[1], "r");
  if (yyin == NULL) {
    printf("ERROR: could not open file %s\n", argv[1]);
    exit(-1);
  }
  /* parse the input program */
  return yyparse();
}
