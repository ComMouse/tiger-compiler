/* This file is not complete.  You should fill it in with your
   solution to the programming exercise. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prog1.h"
#include "slp.h"

// Macros
#define MAX(a, b) (a > b ? a : b)

// Struct definitions
typedef struct table_ *table;
typedef struct intAndtable_ *intAndTable;
struct table_ {string id; int value; table tail;};
struct intAndtable_ {int i; table t;};

// Function definitions
int maxargs(A_stm stm);
int maxargsExp(A_exp exp);

int interpOp(int a, int b, A_binop op);
table interpStm(A_stm s, table t);
intAndTable interpExp(A_exp e, table t);
int lookup(table t, string key);
void interp(A_stm stm);

// Constructors
table Table(string id, int value, table tail)
{
	table t = checked_malloc(sizeof(*t));
	t->id=id; t->value=value; t->tail=tail;
	return t;
}

intAndTable IntAndTable(int i, table t)
{
	intAndTable it = checked_malloc(sizeof(*it));
	it->i=i; it->t=t;
	return it;
}

// Maxargs
int maxargsExp(A_exp exp)
{
	switch (exp->kind) {
		case A_idExp:
		case A_numExp:
			return 0;
		case A_opExp:
			return MAX(maxargsExp(exp->u.op.left),
						maxargsExp(exp->u.op.right));
		case A_eseqExp:
			return MAX(maxargs(exp->u.eseq.stm),
						maxargsExp(exp->u.eseq.exp));
		default:
			fprintf(stderr, "Unknown expression kind\n");
			exit(1);
	}
}

int maxargs(A_stm stm)
{
	// Assign Stm
	if (stm->kind == A_assignStm)
		return maxargsExp(stm->u.assign.exp);

	// Compound Stm
	if (stm->kind == A_compoundStm)
		return MAX(maxargs(stm->u.compound.stm1),
					maxargs(stm->u.compound.stm2));

	// Print Stm
	int list_argc 	= 0;
	int exp_argc 	= 0;
	A_expList list 	= stm->u.print.exps;

	// Iterate pair list
	while (list->kind == A_pairExpList) {
		++list_argc;
		exp_argc 	= MAX(exp_argc, maxargsExp(list->u.pair.head));
		list 		= list->u.pair.tail;
	}

	// Handle last list
	++list_argc;
	exp_argc = MAX(exp_argc, maxargsExp(list->u.last));

	return MAX(list_argc, exp_argc);
}

// Interp
int interpOp(int a, int b, A_binop op)
{
	switch (op) {
		case A_plus:	return a + b;
		case A_minus:	return a - b;
		case A_times:	return a * b;
		case A_div:
			// Check zero division
			if (b == 0) {
				fprintf(stderr, "Cannot divide by zero\n");
				exit(1);
			}
			return a / b;
		default:
			fprintf(stderr, "Unknown operator\n");
			exit(1);
	}
}

table interpPrintExpList(A_expList l, table t)
{
	if (l->kind == A_lastExpList) {
		intAndTable it = interpExp(l->u.last, t);
		printf("%d\n", it->i);
		return it->t;
	}

	// If PairExpList
	intAndTable it = interpExp(l->u.pair.head, t);
	printf("%d ", it->i);

	// Print recursively
	return interpPrintExpList(l->u.pair.tail, t);
}

table interpStm(A_stm s, table t)
{
	switch (s->kind) {
		case A_compoundStm:
			return interpStm(s->u.compound.stm2,
						interpStm(s->u.compound.stm1, t));
		case A_assignStm:
		{
			intAndTable it = interpExp(s->u.assign.exp, t);
			return Table(s->u.assign.id, it->i, it->t);
		}
		case A_printStm:
			return interpPrintExpList(s->u.print.exps, t);
		default:
			fprintf(stderr, "Unknown statement kind\n");
			exit(1);
	}
}

intAndTable interpExp(A_exp e, table t)
{
	switch (e->kind) {
		case A_idExp:
			return IntAndTable(lookup(t, e->u.id), t);
		case A_numExp:
			return IntAndTable(e->u.num, t);
		case A_opExp:
		{
			intAndTable it1 = interpExp(e->u.op.left, t);
			intAndTable it2 = interpExp(e->u.op.right, it1->t);
			return IntAndTable(interpOp(it1->i, it2->i, e->u.op.oper), it2->t);
		}
		case A_eseqExp:
			return interpExp(e->u.eseq.exp,
						interpStm(e->u.eseq.stm, t));
		default:
			fprintf(stderr, "Unknown expression kind\n");
			exit(1);
	}
}

int lookup(table t, string key)
{
	// If found
	if (strcmp(key, t->id) == 0) return t->value;

	// If no tail exists
	if (!t->tail) {
		fprintf(stderr, "Key not found: %s\n", key);
		exit(1);
	}

	// Find element recursively
	return lookup(t->tail, key);
}

void interp(A_stm stm)
{
	// Begin with an empty table
	interpStm(stm, NULL);
}

/*
 *Please don't modify the main() function
 */
int main()
{
	int args;

	printf("prog\n");
	args = maxargs(prog());
	printf("args: %d\n",args);
	interp(prog());

	printf("prog_prog\n");
	args = maxargs(prog_prog());
	printf("args: %d\n",args);
	interp(prog_prog());

	printf("right_prog\n");
	args = maxargs(right_prog());
	printf("args: %d\n",args);
	interp(right_prog());

	return 0;
}
