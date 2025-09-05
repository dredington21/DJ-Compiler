/* DJ PARSER */
%code provides {
  #include "lex.yy.c"
 
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

%right ASSIGN
%right NOT ASSERT
%left OR
%nonassoc EQUALITY LESS
%left PLUS MINUS
%left TIMES
%right DOT

%%

pgm:
    class_declarations main_block ENDOFFILE
    ;

class_declarations:
    /* empty */
    | class_declarations class_declaration
    ;

class_declaration:
    opt_final CLASS ID EXTENDS ID LBRACE var_declarations method_declarations RBRACE
    ;

opt_final:
    /* empty */
    | FINAL
    ;

var_declarations:
    /* empty */
    | var_declarations var_declaration
    ;

var_declaration:
    type ID SEMICOLON
    ;

type:
    NATTYPE
    | ID
    ;

method_declarations:
    /* empty */
    | method_declarations method_declaration
    ;

method_declaration:
    opt_final type ID LPAREN param_list RPAREN var_expr_block
    ;

param_list:
    /* empty */
    | type ID
    ;

var_expr_block:
    LBRACE var_declarations expr_list RBRACE
    ;

main_block:
    MAIN var_expr_block
    ;

expr_list:
    expr SEMICOLON
    | expr_list expr SEMICOLON
    ;

expr:
    expr PLUS expr
    | expr MINUS expr
    | expr TIMES expr
    | expr EQUALITY expr
    | expr LESS expr
    | ASSERT expr
    | NOT expr
    | expr OR expr
    | NATLITERAL
    | NUL
    | IF LPAREN expr RPAREN LBRACE expr_list RBRACE ELSE LBRACE expr_list RBRACE
    | WHILE LPAREN expr RPAREN LBRACE expr_list RBRACE
    | NEW ID LPAREN RPAREN
    | THIS
    | PRINTNAT LPAREN expr RPAREN
    | READNAT LPAREN RPAREN
    | ID
    | expr DOT ID
    | ID ASSIGN expr
    | expr DOT ID ASSIGN expr
    | ID LPAREN expr RPAREN
    | expr DOT ID LPAREN expr RPAREN
    | LPAREN expr RPAREN
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