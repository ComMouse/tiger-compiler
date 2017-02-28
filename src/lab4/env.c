#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/
E_enventry E_VarEntry(Ty_ty ty) {
  E_enventry p = checked_malloc(sizeof(*p));
  p->kind = E_varEntry;
  p->u.var.ty = ty;
  return p;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result) {
  E_enventry p = checked_malloc(sizeof(*p));
  p->kind = E_funEntry;
  p->u.fun.formals = formals;
  p->u.fun.result = result;
  return p;
}

S_table E_base_tenv(void) {
  S_table t = S_empty();
  S_enter(t, S_Symbol("int"), Ty_Int());
  S_enter(t, S_Symbol("string"), Ty_String());
  return t;
}

S_table E_base_venv(void) {
  S_table t = S_empty();
  S_enter(t, S_Symbol("print"),
    E_FunEntry(
      Ty_TyList(Ty_String(), NULL),
      Ty_Void()));
  S_enter(t, S_Symbol("flush"),
    E_FunEntry(
      NULL,
      Ty_Void()));
  S_enter(t, S_Symbol("getchar"),
    E_FunEntry(
      NULL,
      Ty_String()));
  S_enter(t, S_Symbol("ord"),
    E_FunEntry(
      Ty_TyList(Ty_String(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("chr"),
    E_FunEntry(
      Ty_TyList(Ty_Int(), NULL),
      Ty_String()));
  S_enter(t, S_Symbol("size"),
    E_FunEntry(
      Ty_TyList(Ty_String(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("substring"),
    E_FunEntry(
      Ty_TyList(Ty_String(),
        Ty_TyList(Ty_Int(),
          Ty_TyList(Ty_Int(), NULL))),
      Ty_String()));
  S_enter(t, S_Symbol("concat"),
    E_FunEntry(
      Ty_TyList(Ty_String(),
        Ty_TyList(Ty_String(), NULL)),
      Ty_String()));
  S_enter(t, S_Symbol("not"),
    E_FunEntry(
      Ty_TyList(Ty_Int(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("exit"),
    E_FunEntry(
      Ty_TyList(Ty_Int(), NULL),
      Ty_Void()));
  return t;
}
