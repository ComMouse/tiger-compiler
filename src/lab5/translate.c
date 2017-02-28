#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"
#include "types.h"

typedef struct patchList_ *patchList;
struct patchList_ {
  Temp_label *head; patchList tail;
};

struct Cx {
  patchList trues;
  patchList falses;
  T_stm stm;
};

struct Tr_access_ {
  //Lab5: your code here
  Tr_level level;
  F_access access;
};

struct Tr_accessList_ {
  Tr_access head;
  Tr_accessList tail;
};

struct Tr_level_ {
  //Lab5: your code here
  Tr_level parent;
  F_frame frame;
};

struct Tr_exp_ {
  //Lab5: your code here
  enum  {Tr_ex, Tr_nx, Tr_cx} kind;
  union {
    T_exp ex;
    T_stm nx;
    struct Cx cx;
  } u;
};

struct Tr_expList_ {
  Tr_exp head;
  Tr_expList tail;
};

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
  Tr_expList el = checked_malloc(sizeof(*el));
  el->head = head;
  el->tail = tail;
  return el;
}

Tr_exp Tr_expListHead(Tr_expList el) {
  return el->head;
}

Tr_expList Tr_expListTail(Tr_expList el) {
  return el->tail;
}

static Tr_access Tr_Access(Tr_level level, F_access access) {
  Tr_access a = checked_malloc(sizeof(*a));
  a->level = level;
  a->access = access;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList l = checked_malloc(sizeof(*l));
    l->head = head;
    l->tail = tail;
    return l;
}

Tr_access Tr_accessListHead(Tr_accessList al) {
  return al->head;
}

Tr_accessList Tr_accessListTail(Tr_accessList al) {
  return al->tail;
}

static Tr_level outermost_level = NULL;

Tr_level Tr_outermost(void) {
  if (outermost_level == NULL) {
    outermost_level = checked_malloc(sizeof(*outermost_level));
    outermost_level->parent = NULL;
    outermost_level->frame = F_newFrame(Temp_newlabel(), NULL);
  }
  return outermost_level;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, 
                    U_boolList formals) {
  Tr_level lv = checked_malloc(sizeof(*lv));
  lv->parent = parent;
  lv->frame = F_newFrame(name, formals);
  return lv;
}

Tr_accessList Tr_formals(Tr_level level) {
  F_accessList accessList = F_formals(level->frame);
  Tr_accessList a = NULL, last_a = NULL;
  for (; accessList; accessList = accessList->tail) {
    if (last_a == NULL) {
      a = Tr_AccessList(Tr_Access(level, accessList->head), NULL);
      last_a = a;
    } else {
      last_a->tail = Tr_AccessList(Tr_Access(level, accessList->head), NULL);
      last_a = last_a->tail;
    }
  }
  return a;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
  return Tr_Access(level, F_allocLocal(level->frame, escape));
}

/* Basic expressions */

static patchList PatchList(Temp_label *head, patchList tail) {
  patchList p = checked_malloc(sizeof(*p));
  p->head = head;
  p->tail = tail;
  return p;
}

void doPatch(patchList tList, Temp_label label) {
  for (; tList; tList = tList->tail)
    *(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second) {
  if (!first)
    return second;
  for (; first->tail; first = first->tail)  /* go to end of list */
    first->tail = second;
  return first;
}

static Tr_exp Tr_Ex(T_exp ex) {
  Tr_exp e = checked_malloc(sizeof(*e));
  e->kind = Tr_ex;
  e->u.ex = ex;
  return e;
}

static Tr_exp Tr_Nx(T_stm nx) {
  Tr_exp e = checked_malloc(sizeof(*e));
  e->kind = Tr_nx;
  e->u.nx = nx;
  return e;
}
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
  struct Cx cx;
  cx.trues = trues;
  cx.falses = falses;
  cx.stm = stm;

  Tr_exp e = checked_malloc(sizeof(*e));
  e->kind = Tr_cx;
  e->u.cx = cx;
  return e;
}

static T_exp unEx(Tr_exp e) {
  switch (e->kind) {
    case Tr_ex:
      return e->u.ex;
    case Tr_cx: {
      Temp_temp r = Temp_newtemp();
      Temp_label t = Temp_newlabel(), f = Temp_newlabel();
      doPatch(e->u.cx.trues, t);
      doPatch(e->u.cx.falses, f);
      return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
              T_Eseq(e->u.cx.stm,
                T_Eseq(T_Label(f),
                  T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                    T_Eseq(T_Label(t),
                            T_Temp(r))))));
    }
    case Tr_nx:
      return T_Eseq(e->u.nx, T_Const(0));
  }
}

static T_stm unNx(Tr_exp e) {
  switch (e->kind) {
    case Tr_ex:
      return T_Exp(e->u.ex);
    case Tr_cx: {
      Temp_temp r = Temp_newtemp();
      Temp_label t = Temp_newlabel(), f = Temp_newlabel();
      doPatch(e->u.cx.trues, t);
      doPatch(e->u.cx.falses, f);
      return T_Seq(T_Move(T_Temp(r), T_Const(1)),
              T_Seq(e->u.cx.stm,
                T_Seq(T_Label(f),
                  T_Seq(T_Move(T_Temp(r), T_Const(0)),
                    T_Seq(T_Label(t),
                            T_Exp(T_Temp(r)))))));
    }
    case Tr_nx:
      return e->u.nx;
  }
}

static struct Cx unCx(Tr_exp e) {
  switch (e->kind) {
    case Tr_ex: {
      T_stm s = T_Cjump(T_ne, unEx(e), T_Const(0), NULL, NULL);
      patchList trues = PatchList(&s->u.CJUMP.true, NULL);
      patchList falses = PatchList(&s->u.CJUMP.false, NULL);
      Tr_exp cx = Tr_Cx(trues, falses, s);
      return cx->u.cx;
    }
    case Tr_cx:
      return e->u.cx;
    case Tr_nx: {
      EM_error(0, "Unable to unpack nx to cx");
      struct Cx cx;
      return cx;
    }
  }
}

/* Fragments */

static F_fragList fragList = NULL;

void Tr_procEntryExit(Tr_level level, Tr_exp body,
                    Tr_accessList formals) {
  T_stm stm = unNx(body);
  F_frag frag = F_ProcFrag(stm, level->frame);
  fragList = F_FragList(frag, fragList);
  F_procEntryExit1(level->frame, stm);
}

F_fragList Tr_getResult(void) {
  return fragList;
}

/* Tree Expressions */

Tr_exp Tr_nullEx() {
  return Tr_Ex(T_Const(0));
}

Tr_exp Tr_nullCx() {
  return Tr_Cx(NULL, NULL, T_Exp(T_Const(0)));
}

Tr_exp Tr_nullNx() {
  return Tr_Nx(T_Exp(T_Const(0)));
}

Tr_exp Tr_nilExp() {
  return Tr_Ex(T_Const(0));
}

Tr_exp Tr_intExp(A_exp e) {
  return Tr_Ex(T_Const(e->u.intt));
}

Tr_exp Tr_stringExp(string str) {
  Temp_label strpos = Temp_newlabel();
  F_frag frag = F_string(strpos, str);
  fragList = F_FragList(frag, fragList);

  return Tr_Ex(T_Name(strpos));
}

Tr_exp Tr_simpleVar(Tr_access a, Tr_level l) {
  T_exp staticLinkExp = T_Temp(F_FP());
  while (a->level != l) {
    staticLinkExp = F_FPExp(staticLinkExp);
    l = l->parent;
  }
  return Tr_Ex(F_Exp(a->access, staticLinkExp));
}

Tr_exp Tr_fieldVar(Tr_exp var, int fieldIndex, Tr_level l) {
  return Tr_Ex(T_Mem(
          T_Binop(T_plus, unEx(var),
            T_Binop(T_mul, T_Const(fieldIndex), T_Const(F_wordSize)))));
}

Tr_exp Tr_subscriptVar(Tr_exp var, Tr_exp sub, Tr_level l) {
  return Tr_Ex(T_Mem(
          T_Binop(T_plus, unEx(var),
            T_Binop(T_mul, unEx(sub), T_Const(F_wordSize)))));
}

Tr_exp Tr_arOpExp(A_oper o, Tr_exp left, Tr_exp right) {
  T_binOp op = T_plus;
  switch (o) {
    case A_plusOp:    op = T_plus;    break;
    case A_minusOp:   op = T_minus;   break;
    case A_timesOp:   op = T_mul;     break;
    case A_divideOp:  op = T_div;     break;
  }
  return Tr_Ex(T_Binop(op, unEx(left), unEx(right)));
}

Tr_exp Tr_condOpExp(A_oper o, Tr_exp left, Tr_exp right) {
  T_binOp op = T_plus;
  switch (o) {
    case A_eqOp:    op = T_eq;     break;
    case A_neqOp:   op = T_ne;     break;
    case A_ltOp:    op = T_lt;     break;
    case A_leOp:    op = T_le;     break;
    case A_gtOp:    op = T_gt;     break;
    case A_geOp:    op = T_ge;     break;
  }

  T_stm s = T_Cjump(o, unEx(left), unEx(right), NULL, NULL);
  patchList trues = PatchList(&s->u.CJUMP.true, NULL);
  patchList falses = PatchList(&s->u.CJUMP.false, NULL);
  return Tr_Cx(trues, falses, s);
}

Tr_exp Tr_strOpExp(A_oper o, Tr_exp left, Tr_exp right) {
  T_binOp op = T_plus;
  T_stm s;
  switch (o) {
    case A_eqOp:    op = T_eq;     break;
    case A_neqOp:   op = T_ne;     break;
    case A_ltOp:    op = T_lt;     break;
    case A_leOp:    op = T_le;     break;
    case A_gtOp:    op = T_gt;     break;
    case A_geOp:    op = T_ge;     break;
  }

  /* String equal */
  if (op == T_eq || op == T_ne) {

    T_exp e = F_externalCall("stringEqual",
                T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));
    s = T_Cjump(op, e, T_Const(1), NULL, NULL);
  /* String compare */
  } else {
    assert(1);  // Not implemented
    Temp_temp t = Temp_newtemp();
    Temp_label fin = Temp_newlabel();
    T_exp e = F_externalCall("stringCompare",
                  T_ExpList(unEx(left), T_ExpList(unEx(right), NULL)));

    s = T_Cjump(op, e, T_Const(0), NULL, NULL);
  }
  
  patchList trues = PatchList(&s->u.CJUMP.true, NULL);
  patchList falses = PatchList(&s->u.CJUMP.false, NULL);
  return Tr_Cx(trues, falses, s);
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp exp) {
  return Tr_Nx(T_Move(unEx(var), unEx(exp)));
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee, Ty_ty ifty) {
  Temp_label t = Temp_newlabel();
  Temp_label f = Temp_newlabel();
  Temp_label m = Temp_newlabel();

  /* Convert to test to cx */
  if (test->kind == Tr_ex) {
    struct Cx testcx = unCx(test);
    test = Tr_Cx(testcx.trues, testcx.falses, testcx.stm);
  } else if (test->kind == Tr_nx) {
    EM_error(0, "if test exp cannot be nx");
  }

  doPatch(test->u.cx.trues, t);
  doPatch(test->u.cx.falses, f);

  /* Value expression */
  if (ifty->kind != Ty_void) {
    Temp_temp r = Temp_newtemp();

    T_stm s = T_Seq(unCx(test).stm,
                T_Seq(T_Label(t),
                  T_Seq(T_Move(T_Temp(r), unEx(then)),
                    T_Seq(T_Jump(T_Name(m), Temp_LabelList(m, NULL)),
                      T_Seq(T_Label(f),
                        T_Seq(T_Move(T_Temp(r), unEx(elsee)),
                          T_Label(m)))))));
    
    /* Type checking */
    if (then->kind == Tr_nx) {
      EM_error(0, "if then exp cannot be nx when if is as an exp");
    }
    if (elsee->kind == Tr_nx) {
      EM_error(0, "if then exp cannot be nx when if is as an exp");
    }

    return Tr_Ex(T_Eseq(s, T_Temp(r)));
  /* Statement */
  } else {
    T_stm s = T_Seq(unCx(test).stm,
                T_Seq(T_Label(t),
                  T_Seq(unNx(then),
                    T_Seq(T_Jump(T_Name(m), Temp_LabelList(m, NULL)),
                      T_Seq(T_Label(f),
                        T_Seq(unNx(elsee),
                          T_Label(m)))))));

    return Tr_Nx(s);
  }
}

Tr_exp Tr_whileExp(Tr_exp exp, Tr_exp body, Temp_label breaklbl) {
  Temp_label test = Temp_newlabel();
  Temp_label done = breaklbl;
  Temp_label loopstart = Temp_newlabel();

  T_stm s = T_Seq(T_Label(test),
              T_Seq(T_Cjump(T_ne, unEx(exp), T_Const(0), loopstart, done),
                T_Seq(T_Label(loopstart),
                  T_Seq(unNx(body),
                    T_Seq(T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                      T_Label(done))))));

  return Tr_Nx(s);
}

Tr_exp Tr_forExp(Tr_exp explo, Tr_exp exphi, Tr_exp body, Temp_label breaklbl) {
  Temp_label test = Temp_newlabel();
  Temp_label done = breaklbl;
  Temp_label loopstart = Temp_newlabel();

  Temp_temp i = Temp_newtemp();
  Temp_temp limit = Temp_newtemp();

  T_stm s = T_Seq(T_Move(T_Temp(i), unEx(explo)),
              T_Seq(T_Move(T_Temp(limit), unEx(exphi)),
                T_Seq(T_Label(test),
                  T_Seq(T_Cjump(T_le, T_Temp(i), T_Temp(limit), loopstart, done),
                    T_Seq(T_Label(loopstart),
                      T_Seq(unNx(body),
                        T_Seq(T_Move(T_Temp(i), T_Binop(T_plus, T_Temp(i), T_Const(1))),
                          T_Seq(T_Jump(T_Name(test), Temp_LabelList(test, NULL)),
                            T_Label(done)))))))));
  return Tr_Nx(s);
}

Tr_exp Tr_breakExp(Temp_label breaklbl) {
  return Tr_Nx(T_Jump(T_Name(breaklbl), Temp_LabelList(breaklbl, NULL)));
}

Tr_exp Tr_arrayExp(Tr_exp init, Tr_exp size) {
  return Tr_Ex(F_externalCall("initArray",
                T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

Tr_exp Tr_recordExp(Tr_expList el, int fieldCount) {
  /* Allocation */
  Temp_temp r = Temp_newtemp();
  T_stm alloc = T_Move(T_Temp(r),
                  F_externalCall("malloc",
                    T_ExpList(T_Const(fieldCount * F_wordSize), NULL)));

  /* Init fields */
  T_stm init = NULL, current = NULL;
  int fieldIndex = 0;
  for (; el; el = el->tail, ++fieldIndex) {
    if (init == NULL) {
      init = current = T_Seq(T_Move(T_Mem(T_Binop(T_plus,
                              T_Temp(r),
                              T_Const((fieldCount - 1 - fieldIndex) * F_wordSize))),
                                unEx(el->head)),
                        T_Exp(T_Const(0)));         /* stamemnts in seq cannot be null */
    } else {
      current->u.SEQ.right = T_Seq(T_Move(T_Mem(T_Binop(T_plus,
                                    T_Temp(r),
                                    T_Const((fieldCount - 1 - fieldIndex) * F_wordSize))),
                                      unEx(el->head)),
                              T_Exp(T_Const(0)));   /* stamemnts in seq cannot be null */
      current = current->u.SEQ.right;
    }
  }

  return Tr_Ex(T_Eseq(
            T_Seq(alloc, init),
              T_Temp(r)));
}

Tr_exp Tr_callExp(Temp_label name, Tr_expList rawel) {
  T_expList el = NULL;
  for (; rawel; rawel = rawel->tail) {
    el = T_ExpList(unEx(rawel->head), el);
  }

  /* TODO: Fetch static link */
  T_exp staticlink = T_Const(0);
  el = T_ExpList(staticlink, el);

  return Tr_Ex(T_Call(T_Name(name), el));
}
