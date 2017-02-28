#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: your code here
static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
  if (last != NULL) {
    last = last->tail = AS_InstrList(inst, NULL);
  } else {
    last = iList = AS_InstrList(inst, NULL);
  }
}

Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);
static Temp_tempList munchArgs(int i, T_expList args);

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
  // Temp_temp temp = Temp_newtemp();
  // Temp_enter(F_tempMap, temp, "tmp");
  // return AS_InstrList(AS_Move("movl", Temp_TempList(temp, NULL), Temp_TempList(temp, NULL)), NULL);

  AS_instrList list;
  T_stmList sl;

  /* miscellaneous initializations as necessary */

  for (sl = stmList; sl; sl = sl->tail) {
    munchStm(sl->head);
  }
  list = iList;
  iList = last = NULL;
  return list;
}

static Temp_temp munchExp(T_exp e) {
  char *inst = checked_malloc(sizeof(char) * 120);
  char *inst2 = checked_malloc(sizeof(char) * 120);
  switch (e->kind) {
    case T_MEM: {
      T_exp mem = e->u.MEM;
      if (mem->kind == T_BINOP) {
        if (mem->u.BINOP.op == T_plus
            && mem->u.BINOP.right->kind == T_CONST) {
          /* MEM(BINOP(PLUS,e1,CONST(i))) */
          T_exp e1 = mem->u.BINOP.left;
          int i = mem->u.BINOP.right->u.CONST;
          Temp_temp r = Temp_newtemp();
          sprintf(inst, "movl %d(`s0), `d0\n", i);
          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
          return r;
        } else if (mem->u.BINOP.op == T_plus
            && mem->u.BINOP.left->kind == T_CONST) {
          /* MEM(BINOP(PLUS,CONST(i),e1)) */
          T_exp e1 = mem->u.BINOP.right;
          int i = mem->u.BINOP.left->u.CONST;
          Temp_temp r = Temp_newtemp();
          sprintf(inst, "movl %d(`s0), `d0\n", i);
          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
          return r;
        } else {
          /* MEM(e1) */
          T_exp e1 = mem;
          Temp_temp r = Temp_newtemp();
          sprintf(inst, "movl (`s0), `d0\n");
          emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
          return r;
        }
      } else if (mem->kind == T_CONST) {
        /* MEM(CONST(i)) */
        int i = mem->u.CONST;
        Temp_temp r = Temp_newtemp();
        sprintf(inst, "movl %d, `d0\n", i);
        emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
        return r;
      } else {
        /* MEM(e1) */
        T_exp e1 = mem;
        Temp_temp r = Temp_newtemp();
        sprintf(inst, "movl (`s0), `d0\n");
        emit(AS_Oper(inst, L(r, NULL), L(munchExp(e1), NULL), NULL));
        return r;
      }
    }
    case T_BINOP: {
      if (e->kind == T_plus
         && e->u.BINOP.right->kind == T_CONST) {
        /* BINOP(PLUS,e1,CONST(i)) */
        T_exp e1 = e->u.BINOP.left;
        int i = e->u.BINOP.right->u.CONST;
        Temp_temp r = Temp_newtemp();
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
        sprintf(inst2, "addl $%d, `d0\n", i);
        emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
        return r;
      } else if (e->kind == T_plus
         && e->u.BINOP.left->kind == T_CONST) {
        /* BINOP(PLUS,CONST(i),e1) */
        T_exp e1 = e->u.BINOP.right;
        int i = e->u.BINOP.left->u.CONST;
        Temp_temp r = Temp_newtemp();
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
        sprintf(inst2, "addl $%d, `d0\n", i);
        emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
        return r;
      } else if (e->kind == T_minus
         && e->u.BINOP.right->kind == T_CONST) {
        /* BINOP(MINUS,e1,CONST(i)) */
        T_exp e1 = e->u.BINOP.left;
        int i = e->u.BINOP.right->u.CONST;
        Temp_temp r = Temp_newtemp();
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(munchExp(e1), NULL)));
        sprintf(inst2, "subl $%d, `d0\n", i);
        emit(AS_Oper(inst2, L(r, NULL), L(r, NULL), NULL));
        return r;
      } else if (e->kind == T_plus) {
        /* BINOP(PLUS,e1,e2) */
        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
        Temp_temp r = Temp_newtemp();
        Temp_temp r1 = munchExp(e1);
        Temp_temp r2 = munchExp(e2);
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
        sprintf(inst, "addl `s0, `d0\n");
        emit(AS_Oper(inst, L(r, NULL), L(r2, L(r, NULL)), NULL));
        return r;
      } else if (e->kind == T_minus) {
        /* BINOP(MINUS,e1,e2) */
        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
        Temp_temp r = Temp_newtemp();
        Temp_temp r1 = munchExp(e1);
        Temp_temp r2 = munchExp(e2);
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
        sprintf(inst, "subl `s0, `d0\n");
        emit(AS_Oper(inst, L(r, NULL), L(r2, L(r, NULL)), NULL));
        return r;
      } else if (e->kind == T_mul) {
        /* BINOP(MUL,e1,e2) */
        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
        Temp_temp r = Temp_newtemp();
        Temp_temp r1 = munchExp(e1);
        Temp_temp r2 = munchExp(e2);
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
        sprintf(inst, "imul `s0, `d0\n");
        emit(AS_Oper(inst, L(r, NULL), L(r2, L(r, NULL)), NULL));
        return r;
      } else if (e->kind == T_div) {
        /* BINOP(DIV,e1,e2) */
        T_exp e1 = e->u.BINOP.left, e2 = e->u.BINOP.right;
        Temp_temp r = Temp_newtemp();
        Temp_temp r1 = munchExp(e1);
        Temp_temp r2 = munchExp(e2);
        sprintf(inst, "movl `s0, `d0\n");
        emit(AS_Move(inst, L(r, NULL), L(r1, NULL)));
        sprintf(inst, "divl `s0, `d0\n"); // TODO: edx eax
        emit(AS_Oper(inst, L(r, NULL), L(r2, L(r, NULL)), NULL));
        return r;
      } else {
        assert(0);
        break;
      }
    }
    case T_CONST: {
      /* CONST(i) */
      int i = e->u.CONST;
      Temp_temp r = Temp_newtemp();
      sprintf(inst, "movl $%d, `d0\n", i);
      emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
      return r;
    }
    case T_TEMP: {
      /* TEMP(t) */
      return e->u.TEMP;
    }
    case T_NAME: {
      /* NAME(lab) */
      Temp_label lab = e->u.NAME;
      Temp_temp r = Temp_newtemp();
      sprintf(inst, "movl %s, `d0\n", Temp_labelstring(lab));
      emit(AS_Oper(inst, L(r, NULL), NULL, NULL));
      return r;
    }
    default:
      assert(0);
  }
}

static void munchStm(T_stm s) {
  char *inst = checked_malloc(sizeof(char) * 120);
  char *inst2 = checked_malloc(sizeof(char) * 120);
  char *inst3 = checked_malloc(sizeof(char) * 120);
  switch (s->kind) {
    case T_MOVE: {
      T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
      if (dst->kind == T_MEM) {
        if (dst->u.MEM->kind == T_BINOP
            && dst->u.MEM->u.BINOP.op == T_plus
            && dst->u.MEM->u.BINOP.right->kind == T_CONST) {
          /* MOVE(MEM(BINOP(PLUS,e1,CONST(i))),e2) */
          T_exp e1 = dst->u.MEM->u.BINOP.left, e2 = src;
          int i = dst->u.MEM->u.BINOP.right->u.CONST;
          sprintf(inst, "movl `s1, %d(`s0)\n", i);
          emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
        } else if (dst->u.MEM->kind == T_BINOP
            && dst->u.MEM->u.BINOP.op == T_plus
            && dst->u.MEM->u.BINOP.left->kind == T_CONST) {
          /* MOVE(MEM(BINOP(PLUS,CONST(i),e1)),e2) */
          T_exp e1 = dst->u.MEM->u.BINOP.right, e2 = src;
          int i = dst->u.MEM->u.BINOP.left->u.CONST;
          sprintf(inst, "movl `s1, %d(`s0)\n", i);
          emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
        } else if (src->kind == T_MEM) {
          /* MOVE(MEM(e1), MEM(e2)) */
          T_exp e1 = dst->u.MEM, e2 = src->u.MEM;
          Temp_temp r = Temp_newtemp();
          sprintf(inst, "movl (`s0), `d0\n");
          emit(AS_Move(inst, L(r, NULL), L(munchExp(e2), NULL)));
          sprintf(inst2, "movl `s0, (`s1)\n");
          emit(AS_Oper(inst2, NULL, L(r, L(munchExp(e1), NULL)), NULL));
        } else if (dst->u.MEM->kind == T_CONST) {
          /* MOVE(MEM(CONST(i)), e2) */
          T_exp e2 = src;
          int i = dst->u.MEM->u.CONST;
          sprintf(inst, "movl `s0, %d\n", i);
          emit(AS_Oper(inst, NULL, L(munchExp(e2), NULL), NULL));
        } else {
          /* MOVE(MEM(e1), e2) */
          T_exp e1 = dst->u.MEM, e2 = src;
          sprintf(inst, "movl `s1, (`s0)\n");
          emit(AS_Oper(inst, NULL, L(munchExp(e1), L(munchExp(e2), NULL)), NULL));
        }
      } else if (dst->kind == T_TEMP) {
        if (src->kind == T_CALL) {
          if (src->u.CALL.fun->kind == T_NAME) {
            /* MOVE(TEMP(t),CALL(NAME(lab),args)) */
            Temp_label lab = src->u.CALL.fun->u.NAME;
            T_expList args = src->u.CALL.args;
            Temp_temp t = dst->u.TEMP;
            Temp_tempList l = munchArgs(0, args);
            Temp_tempList calldefs = NULL; // TODO
            sprintf(inst, "call %s\n", Temp_labelstring(lab));
            emit(AS_Oper(inst, L(F_RA(), calldefs), l, AS_Targets(Temp_LabelList(lab, NULL))));
            sprintf(inst2, "movl `s0, `d0\n");
            emit(AS_Move(inst2, L(t, NULL), L(F_RA(), NULL)));
            // TODO: Move %eax to r to save call result
          } else {
            /* MOVE(TEMP(t),CALL(e,args)) */
            T_exp e = src->u.CALL.fun;
            T_expList args = src->u.CALL.args;
            Temp_temp t = dst->u.TEMP;
            Temp_temp r = munchExp(e);
            Temp_tempList l = munchArgs(0, args);
            Temp_tempList calldefs = NULL; // TODO
            sprintf(inst, "call *`s0\n");
            emit(AS_Oper(inst, L(F_RA(), calldefs), L(r, l), NULL));
            sprintf(inst2, "movl `s0, `d0\n");
            emit(AS_Move(inst2, L(t, NULL), L(F_RA(), NULL)));
          }
        } else {
          /* MOVE(TEMP(i),e2) */
          T_exp e2 = src;
          Temp_temp i = dst->u.TEMP;
          sprintf(inst, "movl `s0, `d0\n");
          emit(AS_Move(inst, L(i, NULL), L(munchExp(e2), NULL)));
        }
      } else {
        assert(0);
      }
      break;
    }
    case T_LABEL: {
      /* LABEL(lab) */
      Temp_label lab = s->u.LABEL;
      sprintf(inst, "%s:\n", Temp_labelstring(lab));
      emit(AS_Label(inst, lab));
      break;
    }
    case T_EXP: {
      if (s->u.EXP->kind == T_CALL) {
        T_exp call = s->u.EXP;
        if (call->u.CALL.fun->kind == T_NAME) {
          /* EXP(CALL(eNAME(lab),args)) */
          Temp_label lab = call->u.CALL.fun->u.NAME;
          T_expList args = call->u.CALL.args;
          Temp_tempList l = munchArgs(0, args);
          Temp_tempList calldefs = NULL; // TODO
          sprintf(inst, "call %s\n", Temp_labelstring(lab));
          emit(AS_Oper(inst, calldefs, l, AS_Targets(Temp_LabelList(lab, NULL))));
        } else {
          /* EXP(CALL(e,args)) */
          T_exp e = call->u.CALL.fun;
          T_expList args = call->u.CALL.args;
          Temp_temp r = munchExp(e);
          Temp_tempList l = munchArgs(0, args);
          Temp_tempList calldefs = NULL; // TODO
          sprintf(inst, "call *`s0\n");
          emit(AS_Oper(inst, calldefs, L(r, l), NULL));
        }
      } else {
        /* EXP(e) */
        munchExp(s->u.EXP);
      }
      break;
    }
    case T_JUMP: {
      if (s->u.JUMP.exp->kind == T_NAME) {
        /* JUMP(NAME(lab)) */
        Temp_label lab = s->u.JUMP.exp->u.NAME;
        Temp_labelList jumps = s->u.JUMP.jumps;
        sprintf(inst, "jmp %s\n", Temp_labelstring(lab));
        emit(AS_Oper(inst, NULL, NULL, AS_Targets(jumps)));
      } else {
        /* JUMP(e) */
        T_exp e = s->u.JUMP.exp;
        Temp_labelList jumps = s->u.JUMP.jumps;
        sprintf(inst, "jmp *`s0\n");
        emit(AS_Oper(inst, NULL, L(munchExp(e), NULL), AS_Targets(jumps)));
      }
      break;
    }
    case T_CJUMP: {
      /* CJUMP(op,e1,e2,jt,jf) */
      T_relOp op = s->u.CJUMP.op;
      T_exp e1 = s->u.CJUMP.left;
      T_exp e2 = s->u.CJUMP.right;
      Temp_temp r1 = munchExp(e1);
      Temp_temp r2 = munchExp(e2);
      Temp_label jt = s->u.CJUMP.true;
      Temp_label jf = s->u.CJUMP.false;
      sprintf(inst, "cmp `s1, `s0\n");
      emit(AS_Oper(inst, NULL, L(r1, L(r2, NULL)), NULL));

      char* opcode = "";
      switch (op) {
        case T_eq:  opcode = "je";  break;
        case T_ne:  opcode = "jne"; break;
        case T_lt:  opcode = "jl";  break;
        case T_gt:  opcode = "jg";  break;
        case T_le:  opcode = "jle"; break;
        case T_ge:  opcode = "jge"; break;
        case T_ult: opcode = "jb";  break;
        case T_ule: opcode = "jbe"; break;
        case T_ugt: opcode = "ja";  break;
        case T_uge: opcode = "jae"; break;
      }
      sprintf(inst2, "%s %s\n", opcode, Temp_labelstring(jt));
      emit(AS_Oper(inst2, NULL, NULL, AS_Targets(Temp_LabelList(jt, NULL))));

      sprintf(inst3, "jmp %s\n", Temp_labelstring(jf));
      emit(AS_Oper(inst3, NULL, NULL, AS_Targets(Temp_LabelList(jf, NULL))));
      break;
    }
    default: {
      assert(0);
    }
  }
}

static Temp_tempList munchArgs(int i, T_expList args) {
  // TODO
  return NULL;
}


