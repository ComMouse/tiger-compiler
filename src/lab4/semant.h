#ifndef SEMANT_H
#define SEMANT_H

typedef void *Tr_exp;

struct expty {Tr_exp exp; Ty_ty ty;};

struct expty expTy(Tr_exp exp, Ty_ty ty);

void SEM_transProg(A_exp exp);
struct expty SEM_transExp(S_table venv, S_table tenv, A_exp a);
struct expty SEM_transVar(S_table venv, S_table tenv, A_var v);
void SEM_transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty SEM_transTy(S_table tenv, A_ty a);

#endif
