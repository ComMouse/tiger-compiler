#include <stdio.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/

/* Constructors*/
struct expty expTy(Tr_exp exp, Ty_ty ty) {
  struct expty e;
  e.exp = exp;
  e.ty = ty;
  return e;
}

/* Utilities */
static Ty_ty actual_ty(Ty_ty ty) {
  Ty_ty actual = ty;
  while (actual && actual->kind == Ty_name) {
    actual = actual->u.name.ty;
    /* Return if a cycle is detected */
    if (actual == ty)
      break;
  }
  return actual;
}

/* Compare types strictly */
static int compare_ty(Ty_ty ty1, Ty_ty ty2) {
  ty1 = actual_ty(ty1);
  ty2 = actual_ty(ty2);
  if (ty1->kind == Ty_int || ty1->kind == Ty_string)
    return ty1->kind == ty2->kind;
  
  if (ty1->kind == Ty_nil && ty2->kind == Ty_nil)
    return FALSE;
  
  if (ty1->kind == Ty_nil)
    return ty2->kind == Ty_record || ty2->kind == Ty_array;
  
  if (ty2->kind == Ty_nil)
    return ty1->kind == Ty_record || ty1->kind == Ty_array;
  
  return ty1 == ty2;
}

/* Protected symbol for loop variable checking */
S_symbol protect_sym(S_symbol s) {
  char protected_sym[256];
  sprintf(protected_sym, "<%s>", S_name(s));
  return S_Symbol(String(protected_sym));
}

/* Program entrance */
void SEM_transProg(A_exp exp) {
  S_table venv = E_base_venv();
  S_table tenv = E_base_tenv();
  SEM_transExp(venv, tenv, exp);
}

/* Expression entrance */
struct expty SEM_transExp(S_table venv, S_table tenv, A_exp a) {
  switch (a->kind) {
    case A_varExp: {
      struct expty exp = SEM_transVar(venv, tenv, a->u.var);
      return expTy(NULL, exp.ty);
    }
    case A_nilExp:
      return expTy(NULL, Ty_Nil());
    case A_intExp:
      return expTy(NULL, Ty_Int());
    case A_stringExp:
      if (a->u.stringg == NULL)
        EM_error(a->pos, "string required");
      return expTy(NULL, Ty_String());
    case A_callExp: {
      E_enventry func = S_look(venv, a->u.call.func);
      if (func == NULL) {
        EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
        return expTy(NULL, Ty_Void());
      }

      if (func->kind != E_funEntry) {
        EM_error(a->pos, "%s is not a function", S_name(a->u.call.func));
        return expTy(NULL, Ty_Void());
      }

      /* Compare para type */
      Ty_tyList tl;
      A_expList el;
      struct expty exp;
      for (tl = func->u.fun.formals, el = a->u.call.args;
        tl && el; tl = tl->tail, el = el->tail) {
        exp = SEM_transExp(venv, tenv, el->head);
        if (!compare_ty(tl->head, exp.ty)) {
          EM_error(el->head->pos, "para type mismatch");
          break;
        }
      }

      if (el)
        EM_error(a->pos, "too many params in function %s", S_name(a->u.call.func));
      if (tl)
        EM_error(a->pos, "too few params in function %s", S_name(a->u.call.func));

      return expTy(NULL, func->u.fun.result);
    }
    case A_opExp: {
      A_oper oper = a->u.op.oper;
      struct expty left = SEM_transExp(venv, tenv, a->u.op.left);
      struct expty right = SEM_transExp(venv, tenv, a->u.op.right);
      switch (oper) {
        case A_plusOp:
        case A_minusOp:
        case A_timesOp:
        case A_divideOp: {
          if (left.ty->kind != Ty_int)
            EM_error(a->u.op.left->pos, "integer required");
          if (right.ty->kind != Ty_int)
            EM_error(a->u.op.right->pos, "integer required");
          return expTy(NULL, Ty_Int());
        }
        case A_eqOp:
        case A_neqOp: {
          if (!compare_ty(left.ty, right.ty)) {
            EM_error(a->u.op.right->pos, "same type required");
            return expTy(NULL, Ty_Int());
          }
          return expTy(NULL, Ty_Int());
        }
        case A_ltOp:
        case A_leOp:
        case A_gtOp:
        case A_geOp: {
          Ty_ty leftTy = actual_ty(left.ty);
          Ty_ty rightTy = actual_ty(right.ty);
          if (leftTy->kind != Ty_int && leftTy->kind != Ty_string)
            EM_error(a->u.op.left->pos, "string or integer required");
          if (leftTy->kind == Ty_int) {
            if (rightTy->kind != Ty_int)
              EM_error(a->u.op.right->pos, "same type required");
            return expTy(NULL, Ty_Int());
          }
          if (leftTy->kind == Ty_string) {
            if (rightTy->kind != Ty_string)
              EM_error(a->u.op.right->pos, "same type required");
            return expTy(NULL, Ty_Int());
          }
          /* Error recovery */
          return expTy(NULL, Ty_Int());
        }
      }
    }
    case A_recordExp: {
      /* Get the record type */
      Ty_ty t = actual_ty(SEM_transTy(tenv, A_NameTy(a->pos, a->u.record.typ)));
      if (t->kind != Ty_record) {
        EM_error(a->pos, "type %s is not record", S_name(a->u.record.typ));
        return expTy(NULL, Ty_Record(NULL));
      }

      /* Compare fields */
      A_efieldList el;
      Ty_fieldList fl;
      struct expty exp;
      for (el = a->u.record.fields, fl = t->u.record;
        el && fl; el = el->tail, fl = fl->tail) {
        if (strcmp(S_name(el->head->name), S_name(fl->head->name)) != 0) {
          EM_error(a->pos, "field name should be %s but not %s",
            S_name(fl->head->name), S_name(el->head->name));
          continue;
        }

        exp = SEM_transExp(venv, tenv, el->head->exp);
        if (!compare_ty(fl->head->ty, exp.ty))
          EM_error(el->head->exp->pos, "field type of %s mismatch",
            S_name(fl->head->name));
      }
      if (el || fl) {
        EM_error(a->pos, "fields of type %s mismatch", S_name(a->u.record.typ));
      }

      return expTy(NULL, t);
    }
    case A_seqExp: {
      struct expty exp = expTy(NULL, Ty_Void());
      A_expList list;
      for (list = a->u.seq; list != NULL; list = list->tail)
        exp = SEM_transExp(venv, tenv, list->head);
      return exp;
    }
    case A_assignExp: {
      struct expty var = SEM_transVar(venv, tenv, a->u.assign.var);
      struct expty exp = SEM_transExp(venv, tenv, a->u.assign.exp);
      if (!compare_ty(var.ty, exp.ty))
        EM_error(a->pos, "unmatched assign exp");

      /* Loop variable detection */
      if (a->u.assign.var->kind == A_simpleVar &&
        S_look(venv, protect_sym(a->u.assign.var->u.simple)) != NULL) {
        EM_error(a->u.assign.var->pos, "loop variable can't be assigned");
      }
      return expTy(NULL, Ty_Void());
    }
    case A_ifExp: {
      struct expty test, then, elsee;
      test = SEM_transExp(venv, tenv, a->u.iff.test);
      if (test.ty->kind != Ty_int)
        EM_error(a->u.iff.test->pos, "expression must be integer");

      then = SEM_transExp(venv, tenv, a->u.iff.then);
      if (a->u.iff.elsee != NULL) {           /* if then else */
        elsee = SEM_transExp(venv, tenv, a->u.iff.elsee);
        if (!compare_ty(then.ty, elsee.ty)) {
          EM_error(a->u.iff.elsee->pos,
            "then exp and else exp type mismatch");
        }
      } else if (then.ty->kind != Ty_void) {  /* if then */
        EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
      }

      return expTy(NULL, then.ty);
    }
    case A_whileExp: {
      struct expty exp, body;
      exp = SEM_transExp(venv, tenv, a->u.whilee.test);
      if (exp.ty->kind != Ty_int)
        EM_error(a->u.whilee.test->pos, "expression must be integer");

      S_beginScope(venv);
      /* Symbol for break checking */
      S_enter(venv, S_Symbol("<loop>"), E_VarEntry(Ty_Int()));
      body = SEM_transExp(venv, tenv, a->u.whilee.body);
      S_endScope(venv);

      if (body.ty->kind != Ty_void)
        EM_error(a->u.whilee.test->pos, "while body must produce no value");
      return expTy(NULL, Ty_Void());
    }
    case A_forExp: {
      struct expty explo, exphi, body;
      S_beginScope(venv);

      explo = SEM_transExp(venv, tenv, a->u.forr.lo);
      if (explo.ty->kind != Ty_int)
        EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
      exphi = SEM_transExp(venv, tenv, a->u.forr.hi);
      if (exphi.ty->kind != Ty_int)
        EM_error(a->u.forr.hi->pos, "for exp's range type is not integer");

      S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Int()));
      /* Symbol for loop variable checking */
      S_enter(venv, protect_sym(a->u.forr.var), E_VarEntry(Ty_Int()));
      /* Symbol for break checking */
      S_enter(venv, S_Symbol("<loop>"), E_VarEntry(Ty_Int()));

      body = SEM_transExp(venv, tenv, a->u.forr.body);
      if (body.ty->kind != Ty_void)
        EM_error(a->u.forr.body->pos, "for body must produce no value");

      S_endScope(venv);
      return expTy(NULL, Ty_Void());
    }
    case A_breakExp: {
      if (!S_look(venv, S_Symbol("<loop>"))) {
        EM_error(a->pos, "break must be in a loop");
      }
      return expTy(NULL, Ty_Void());
    }
    case A_letExp: {
      struct expty exp;
      A_decList d;
      S_beginScope(venv);
      S_beginScope(tenv);

      for (d = a->u.let.decs; d; d = d->tail)
        SEM_transDec(venv, tenv, d->head);
      exp = SEM_transExp(venv, tenv, a->u.let.body);

      S_endScope(tenv);
      S_endScope(venv);
      return exp;
    }
    case A_arrayExp: {
      Ty_ty t = S_look(tenv, a->u.array.typ);
      struct expty size, init;
      size = SEM_transExp(venv, tenv, a->u.array.size);
      init = SEM_transExp(venv, tenv, a->u.array.init);

      t = actual_ty(t);
      if (t->kind != Ty_array) {
        EM_error(a->pos, "type %s must be array",  S_name(a->u.array.typ));
        return expTy(NULL, t);
      }

      if (!compare_ty(t->u.array, init.ty))
        EM_error(a->pos, "type mismatch");

      if (t)
        return expTy(NULL, t);
      else {
        EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
        return expTy(NULL, Ty_Int());
      }
    }
  }
  assert(0);
}

/* Variable entry */
struct expty SEM_transVar(S_table venv, S_table tenv, A_var v) {
  switch (v->kind) {
    case A_simpleVar: {
      E_enventry x = S_look(venv, v->u.simple);
      if (x && x->kind == E_varEntry) {
        return expTy(NULL, actual_ty(x->u.var.ty));
      }
      else {
        EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
        return expTy(NULL, Ty_Int());
      }
    }
    case A_fieldVar: {
      struct expty var = SEM_transVar(venv, tenv, v->u.field.var);
      if (var.ty->kind != Ty_record) {
        EM_error(v->pos, "not a record type");
        return expTy(NULL, Ty_Int());
      }

      Ty_fieldList list;
      for (list = var.ty->u.record; list != NULL; list = list->tail)
        if (!strcmp(S_name(list->head->name), S_name(v->u.field.sym)))
          return expTy(NULL, actual_ty(list->head->ty));

      EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
      return expTy(NULL, Ty_Int());
    }
    case A_subscriptVar: {
      struct expty var, sub;
      sub = SEM_transExp(venv, tenv, v->u.subscript.exp);
      if (sub.ty->kind != Ty_int)
        EM_error(v->u.subscript.exp->pos, "subscript must be int");

      var = SEM_transVar(venv, tenv, v->u.subscript.var);
      if (var.ty->kind != Ty_array) {
        EM_error(v->pos, "array type required");
        return expTy(NULL, Ty_Int());
      }

      return expTy(NULL, actual_ty(var.ty->u.array));
    }
  }
  assert(0);
}

/* Type list generation of a function */
static Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params) {
  A_fieldList l;
  Ty_tyList tys = NULL, last_tys = NULL;
  for (l = params; l; l = l->tail) {
    /* Insert at tail to avoid order reversed */
    if (tys == NULL) {
      tys = Ty_TyList(Ty_Name(l->head->name,
        actual_ty(SEM_transTy(tenv, A_NameTy(l->head->pos, l->head->typ)))), NULL);
      last_tys = tys;
    } else {
      last_tys->tail = Ty_TyList(Ty_Name(l->head->name,
        actual_ty(SEM_transTy(tenv, A_NameTy(l->head->pos, l->head->typ)))), NULL);
      last_tys = last_tys->tail;
    }
  }
  return tys;
}

/* Declaration Entry */
void SEM_transDec(S_table venv, S_table tenv, A_dec d) {
  switch (d->kind) {
    case A_functionDec: {
      A_fundecList funList;

      /* First Handle */
      for (funList = d->u.function; funList; funList = funList->tail) {
        A_fundec f = funList->head;
        Ty_ty resultTy = (f->result) ? S_look(tenv, f->result) : Ty_Void();
        Ty_tyList formalTys = makeFormalTyList(tenv, f->params);

        // if (S_look(venv, f->name))
        //   EM_error(f->pos, "two functions have the same name");

        S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
      }

      /* Second Handle */
      for (funList = d->u.function; funList; funList = funList->tail) {
        A_fundec f = funList->head;
        E_enventry e = S_look(venv, f->name);
        if (e == NULL || e->kind != E_funEntry) {
          EM_error(f->pos, "cannot find function %s", S_name(f->name));
          continue;
        }

        S_beginScope(venv);
        {
          A_fieldList l; Ty_tyList t;
          for (l = f->params, t = e->u.fun.formals; l; l = l->tail, t = t->tail)
            S_enter(venv, l->head->name, E_VarEntry(t->head));
        }

        struct expty body = SEM_transExp(venv, tenv, f->body);
        if (!compare_ty(body.ty, e->u.fun.result))
          if (e->u.fun.result->kind == Ty_void)
            EM_error(f->pos, "procedure returns value");
          else 
            EM_error(f->pos, "function body return type mismatch: %s", S_name(f->name));
        S_endScope(venv);
      }
      break;
    }
    case A_varDec: {
      Ty_ty t = NULL;
      struct expty e = SEM_transExp(venv, tenv, d->u.var.init);

      if (d->u.var.typ)
        t = actual_ty(SEM_transTy(tenv, A_NameTy(d->pos, d->u.var.typ)));
      
      /* Nil assigning checking */
      if (!t && e.ty->kind == Ty_nil)
        EM_error(d->pos, "init should not be nil without type specified");

      if (!t)
        t = e.ty;

      if (!compare_ty(t, e.ty))
        EM_error(d->pos, "type mismatch");
        
      S_enter(venv, d->u.var.var, E_VarEntry(t));
      break;
    }
    case A_typeDec: {
      A_nametyList l;
      /* First Handle */
      for (l = d->u.type; l; l = l->tail) {
        // if (S_look(tenv, l->head->name))
        //   EM_error(d->pos, "two types have the same name");

        S_enter(tenv, l->head->name, Ty_Name(l->head->name, NULL));
      }

      /* Second Handle */
      for (l = d->u.type; l; l = l->tail) {
        Ty_ty t = S_look(tenv, l->head->name);
        if (!t || t->kind != Ty_name)
          EM_error(d->pos, "type %s not found", S_name(l->head->name));
        
        t->u.name.ty = SEM_transTy(tenv, l->head->ty);
      }

      /* Cycle Check */
      for (l = d->u.type; l; l = l->tail) {
        Ty_ty t = S_look(tenv, l->head->name);
        if (!t)
          EM_error(d->pos, "type %s not found", S_name(l->head->name));
        
        if (t->kind == Ty_name && actual_ty(t) == t) {
          EM_error(d->pos, "illegal type cycle");
          break;
        }
      }
      break;
    }
    default:
      assert(0);
  }
}

/* Type Entry (without actual_ty) */
Ty_ty SEM_transTy(S_table tenv, A_ty a) {
  switch (a->kind) {
    case A_nameTy: {
      Ty_ty t = S_look(tenv, a->u.name);
      if (t)
        return t;
      else {
        EM_error(a->pos, "undefined type %s", S_name(a->u.name));
        return Ty_Int();
      }
    }
    case A_recordTy: {
      A_fieldList l;
      Ty_fieldList tl = NULL, last_tl = NULL;
      Ty_ty ty;
      for (l = a->u.record; l; l = l->tail) {
        /* Find the type */
        ty = S_look(tenv, l->head->typ);
        if (!ty) {
          EM_error(l->head->pos, "undefined type %s", S_name(l->head->typ));
          ty = Ty_Int();
        }

        /* Ensure correct order of field list (not the reversed order) */
        if (tl == NULL) {
          tl = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
          last_tl = tl;
        } else {
          last_tl->tail = Ty_FieldList(Ty_Field(l->head->name, ty), NULL);
          last_tl = last_tl->tail;
        }
      }
      return Ty_Record(tl);
    }
    case A_arrayTy: {
      Ty_ty t = S_look(tenv, a->u.array);
      if (t)
        return Ty_Array(t);
      else {
        EM_error(a->pos, "undefined type %s", S_name(a->u.name));
        return Ty_Array(Ty_Int());
      }
    }
  }
}
