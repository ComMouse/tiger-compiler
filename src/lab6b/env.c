#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"
#include "translate.h"

/*Lab4: Your implementation of lab4*/
E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
  E_enventry p = checked_malloc(sizeof(*p));
  p->kind = E_varEntry;
  p->u.var.access = access;
  p->u.var.ty = ty;
  return p;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label,
                    Ty_tyList formals, Ty_ty result) {
  E_enventry p = checked_malloc(sizeof(*p));
  p->kind = E_funEntry;
  p->u.fun.level = level;
  p->u.fun.label = label;
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
      Tr_outermost(),
      Temp_namedlabel("print"),
      Ty_TyList(Ty_String(), NULL),
      Ty_Void()));
  S_enter(t, S_Symbol("printi"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("printi"),
      Ty_TyList(Ty_Int(), NULL),
      Ty_Void()));
  S_enter(t, S_Symbol("flush"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("flush"),
      NULL,
      Ty_Void()));
  S_enter(t, S_Symbol("getchar"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("getchar"),
      NULL,
      Ty_String()));
  S_enter(t, S_Symbol("ord"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("ord"),
      Ty_TyList(Ty_String(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("chr"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("chr"),
      Ty_TyList(Ty_Int(), NULL),
      Ty_String()));
  S_enter(t, S_Symbol("size"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("size"),
      Ty_TyList(Ty_String(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("substring"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("substring"),
      Ty_TyList(Ty_String(),
        Ty_TyList(Ty_Int(),
          Ty_TyList(Ty_Int(), NULL))),
      Ty_String()));
  S_enter(t, S_Symbol("concat"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("concat"),
      Ty_TyList(Ty_String(),
        Ty_TyList(Ty_String(), NULL)),
      Ty_String()));
  S_enter(t, S_Symbol("not"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("not"),
      Ty_TyList(Ty_Int(), NULL),
      Ty_Int()));
  S_enter(t, S_Symbol("exit"),
    E_FunEntry(
      Tr_outermost(),
      Temp_namedlabel("exit"),
      Ty_TyList(Ty_Int(), NULL),
      Ty_Void()));
  return t;
}
