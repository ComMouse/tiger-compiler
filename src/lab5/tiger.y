%{
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h" 
#include "errormsg.h"
#include "absyn.h"

int yylex(void); /* function prototype */

A_exp absyn_root;

void yyerror(char *s)
{
 EM_error(EM_tokPos, "%s", s);
 exit(1);
}
%}


%union {
	int pos;
	int ival;
	string sval;
	A_var var;
	A_exp exp;
	/* et cetera */
    A_dec dec;
    A_ty ty;
    A_field field;
    A_fieldList fieldList;
    A_expList expList;
    A_fundec fundec;
    A_fundecList fundecList;
    A_decList decList;
    A_namety namety;
    A_nametyList nametyList;
    A_efield efield;
    A_efieldList efieldList;
	}

%token <sval> ID STRING
%token <ival> INT

%token 
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK 
  LBRACE RBRACE DOT 
  PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF 
  BREAK NIL
  FUNCTION VAR TYPE 

/* et cetera */
%type <exp> exp program valexp constexp arrayexp seqlist callexp opexp recexp assignexp ifexp whileexp forexp letexp
%type <var> lvalue
%type <dec> dec vardec
%type <ty> ty
%type <field> tyfield
%type <fieldList> tyfields
%type <expList> exps funcargs
%type <fundec> fundec
%type <fundecList> fundecs
%type <decList> decs
%type <namety> tydec
%type <nametyList> tydecs
%type <efield> recfield
%type <efieldList> recfields

%right EXP
%right FUNDEC
%left OR
%left AND
%nonassoc EQ NEQ LE GE LT GT
%left PLUS MINUS
%left TIMES DIVIDE
%left UMINUS

%nonassoc IFX
%nonassoc ELSE

%start program

%%

program:   exp    {absyn_root=$1;}

/* example:
 * exp:   ID         {$$=A_VarExp(EM_tokPos,A_SimpleVar(EM_tokPos,S_Symbol($1)));} 
 */

/* expressions */

exp:      valexp        %prec EXP   {$$ = $1;}
    |     assignexp                 {$$ = $1;}
    |     whileexp                  {$$ = $1;}
    |     forexp                    {$$ = $1;}
    |     BREAK                     {$$ = A_BreakExp(EM_tokPos);}

valexp:   lvalue                    {$$ = A_VarExp(EM_tokPos, $1);}
    |     constexp                  {$$ = $1;}
    |     seqlist                   {$$ = $1;}
    |     callexp                   {$$ = $1;}
    |     opexp                     {$$ = $1;}
    |     recexp                    {$$ = $1;}
    |     arrayexp                  {$$ = $1;}
    |     letexp                    {$$ = $1;}
    |     ifexp                     {$$ = $1;}

/* declarations */

decs:   dec                         {$$ = A_DecList($1, NULL);}
    |   dec decs                    {$$ = A_DecList($1, $2);}

dec:    tydecs                      {$$ = A_TypeDec(EM_tokPos, $1);}
    |   vardec                      {$$ = $1;}
    |   fundecs                     {$$ = A_FunctionDec(EM_tokPos, $1);}

tydecs: tydec                       {$$ = A_NametyList($1, NULL);}
    |   tydec tydecs                {$$ = A_NametyList($1, $2);}

tydec:  TYPE ID EQ ty               {$$ = A_Namety(S_Symbol($2), $4);}

ty:     ID                          {$$ = A_NameTy(EM_tokPos, S_Symbol($1));}
    |   LBRACE tyfields RBRACE      {$$ = A_RecordTy(EM_tokPos, $2);}
    |   ARRAY OF ID                 {$$ = A_ArrayTy(EM_tokPos, S_Symbol($3));}

tyfields:                           {$$ = NULL;}
    |     tyfield                   {$$ = A_FieldList($1, NULL); }
    |     tyfield COMMA tyfields    {$$ = A_FieldList($1, $3); }

tyfield:  ID COLON ID               {$$ = A_Field(EM_tokPos, S_Symbol($1), S_Symbol($3));}

vardec:   VAR ID ASSIGN exp             {$$ = A_VarDec(EM_tokPos, S_Symbol($2), NULL, $4);}
    |     VAR ID COLON ID ASSIGN exp    {$$ = A_VarDec(EM_tokPos, S_Symbol($2), S_Symbol($4), $6);}

fundecs:  fundec                    {$$ = A_FundecList($1, NULL);}
    |     fundec fundecs            {$$ = A_FundecList($1, $2);}

fundec:   FUNCTION ID LPAREN tyfields RPAREN EQ exp             {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, NULL, $7);}
    |     FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp    {$$ = A_Fundec(EM_tokPos, S_Symbol($2), $4, S_Symbol($7), $9);}

/* value expressions */

constexp: INT                       {$$ = A_IntExp(EM_tokPos, $1);}
    |     STRING                    {$$ = A_StringExp(EM_tokPos, $1);}
    |     NIL                       {$$ = A_NilExp(EM_tokPos);}

lvalue:   ID                        {$$ = A_SimpleVar(EM_tokPos, S_Symbol($1));}
          /* corner case for lvalue & arrayexp */
    |     ID LBRACK exp RBRACK      {$$ = A_SubscriptVar(EM_tokPos, A_SimpleVar(EM_tokPos, S_Symbol($1)), $3);}
    |     lvalue DOT ID             {$$ = A_FieldVar(EM_tokPos, $1, S_Symbol($3));}
    |     lvalue LBRACK exp RBRACK  {$$ = A_SubscriptVar(EM_tokPos, $1, $3);}

arrayexp: ID LBRACK exp RBRACK OF exp           {$$ = A_ArrayExp(EM_tokPos, S_Symbol($1), $3, $6);}

seqlist:  LPAREN exps RPAREN                    {$$ = A_SeqExp(EM_tokPos, $2);}
    |     LPAREN RPAREN                         {$$ = A_SeqExp(EM_tokPos, NULL);}

exps:     exp                                   {$$ = A_ExpList($1, NULL);}
    |     exp SEMICOLON exps                    {$$ = A_ExpList($1, $3);}

callexp:  ID LPAREN RPAREN                      {$$ = A_CallExp(EM_tokPos, S_Symbol($1), NULL);}
    |     ID LPAREN funcargs RPAREN             {$$ = A_CallExp(EM_tokPos, S_Symbol($1), $3);}

funcargs: exp                                   {$$ = A_ExpList($1, NULL);}
    |     exp COMMA funcargs                    {$$ = A_ExpList($1, $3);}

opexp:    valexp PLUS valexp  %prec PLUS        {$$ = A_OpExp(EM_tokPos, A_plusOp, $1, $3);}
    |     valexp MINUS valexp  %prec MINUS      {$$ = A_OpExp(EM_tokPos, A_minusOp, $1, $3);}
    |     valexp TIMES valexp  %prec TIMES      {$$ = A_OpExp(EM_tokPos, A_timesOp, $1, $3);}
    |     valexp DIVIDE valexp  %prec DIVIDE    {$$ = A_OpExp(EM_tokPos, A_divideOp, $1, $3);}
    |     MINUS valexp   %prec UMINUS           {$$ = A_OpExp(EM_tokPos, A_minusOp, A_IntExp(EM_tokPos, 0), $2);}
    |     valexp EQ valexp %prec EQ             {$$ = A_OpExp(EM_tokPos, A_eqOp, $1, $3);}
    |     valexp NEQ valexp  %prec NEQ          {$$ = A_OpExp(EM_tokPos, A_neqOp, $1, $3);}
    |     valexp GT valexp %prec GT             {$$ = A_OpExp(EM_tokPos, A_gtOp, $1, $3);}
    |     valexp LT valexp %prec LT             {$$ = A_OpExp(EM_tokPos, A_ltOp, $1, $3);}
    |     valexp GE valexp %prec GE             {$$ = A_OpExp(EM_tokPos, A_geOp, $1, $3);}
    |     valexp LE valexp %prec LE             {$$ = A_OpExp(EM_tokPos, A_leOp, $1, $3);}
    |     valexp AND valexp %prec AND           {
                                                    $$ = A_IfExp(EM_tokPos,
                                                        $1,
                                                        $3,
                                                        A_IntExp(EM_tokPos, 0));
                                                }
    |     valexp OR valexp %prec OR             {
                                                    $$ = A_IfExp(EM_tokPos,
                                                        $1,
                                                        A_IntExp(EM_tokPos, 1),
                                                        $3);
                                                }

recexp:   ID LBRACE RBRACE                      {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), NULL);}
    |     ID LBRACE recfields RBRACE            {$$ = A_RecordExp(EM_tokPos, S_Symbol($1), $3);}

recfields:  recfield                            {$$ = A_EfieldList($1, NULL);}
    |       recfield COMMA recfields            {$$ = A_EfieldList($1, $3);}

recfield: ID EQ exp                             {$$ = A_Efield(S_Symbol($1), $3);}

/* control flow expressions */

assignexp:  lvalue ASSIGN exp                       {$$ = A_AssignExp(EM_tokPos, $1, $3);}

ifexp:    IF valexp THEN exp           %prec IFX    {$$ = A_IfExp(EM_tokPos, $2, $4, NULL);}
    |     IF valexp THEN exp ELSE exp               {$$ = A_IfExp(EM_tokPos, $2, $4, $6);}

whileexp: WHILE exp DO exp                          {$$ = A_WhileExp(EM_tokPos, $2, $4);}

forexp:   FOR ID ASSIGN exp TO exp DO exp           {$$ = A_ForExp(EM_tokPos, S_Symbol($2), $4, $6, $8);}

letexp:   LET decs IN exps END                      {$$ = A_LetExp(EM_tokPos, $2, A_SeqExp(EM_tokPos, $4));}
