#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/

typedef struct F_frameList_ *F_frameList;

struct F_access_ {
  enum {inFrame, inReg} kind;
  union {
    int offset;		/* InFrame */
    Temp_temp reg;	/* InReg */
  } u;
};

struct F_frame_ {
  Temp_label name;
  Temp_map temp;
  F_accessList formals;
  F_accessList locals;
};

struct F_frameList_ {
  F_frame head;
  F_frameList tail;
};

static F_frameList F_FrameList(F_frame head, F_frameList tail) {
  F_frameList l = checked_malloc(sizeof(*l));
  l->head = head;
  l->tail = tail;
  return l;
}

static F_access InFrame(int offset) {
  F_access a = checked_malloc(sizeof(*a));
  a->kind = inFrame;
  a->u.offset = offset;
  return a;
}

static F_access InReg(Temp_temp reg) {
  F_access a = checked_malloc(sizeof(*a));
  a->kind = inReg;
  a->u.reg = reg;
  return a;
}

static F_frameList frameStack = NULL;

F_frame F_newFrame(Temp_label name, U_boolList formals) {
  F_frame f = checked_malloc(sizeof(*f));
  f->name = name;

  // Arguments start from %ebp + 4
  int offset = 4;
  U_boolList formalEscape = formals;
  F_accessList formal = NULL;
  while (formalEscape) {
    offset += 4;
    formal = F_AccessList(InFrame(offset), formal);
    formalEscape = formalEscape->tail;
  }

  f->formals = formal;
  f->locals = NULL;
  f->temp = Temp_empty();

  frameStack = F_FrameList(f, frameStack);
  return f;
}

Temp_label F_name(F_frame f) {
  return f->name;
}

F_accessList F_formals(F_frame f) {
  return f->formals;
}

F_access F_allocLocal(F_frame f, bool escape) {
  // Handle escape=true only
  // Locals start from %ebp - 4
  int offset = 0;
  F_accessList locals = f->locals;
  while (locals != NULL) {
    if (locals->head->kind == inFrame)
      offset -= 4;
    locals = locals->tail;
  }

  F_access l = InFrame(offset);
  f->locals = F_AccessList(l, f->locals);
  return l;
}

int F_accessOffset(F_access a) {
  if (a->kind != inFrame) {
    EM_error(0, "Offset of a reg access is invalid");
  }

  return a->u.offset;
}

Temp_temp F_accessReg(F_access a) {
  if (a->kind != inReg) {
    EM_error(0, "Reg of a frame access is invalid");
  }

  return a->u.reg;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
  F_accessList l = checked_malloc(sizeof(*l));
  l->head = head;
  l->tail = tail;
  return l;
}

F_frag F_StringFrag(Temp_label label, string str) {
  F_frag f = checked_malloc(sizeof(*f));
  f->kind = F_stringFrag;
  f->u.stringg.label = label;
  f->u.stringg.str = String(str);
  return f;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
  F_frag f = checked_malloc(sizeof(*f));
  f->kind = F_procFrag;
  f->u.proc.body = body;
  f->u.proc.frame = frame;
  return f;
}

F_frag F_string(Temp_label lab, string str) {
  F_frag frag = F_StringFrag(lab, str);
  return frag;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
  F_fragList l = checked_malloc(sizeof(*l));
  l->head = head;
  l->tail = tail;
  return l;
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
  frameStack = frameStack->tail;
  return stm;
}

static Temp_tempList returnSink = NULL;

AS_instrList F_procEntryExit2(AS_instrList body) {
  Temp_tempList calleeSaves = NULL;
  if (!returnSink)
    returnSink = Temp_TempList(F_ZERO(),
                    Temp_TempList(F_RA(),
                      Temp_TempList(F_SP(), calleeSaves)));
  return AS_splice(body, AS_InstrList(
            AS_Oper("", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
  char buf[100];
  sprintf(buf, "PROCEDURE %s\n", S_name(frame->name));
  return AS_Proc(String(buf), body, "END\n");
}

/* Machine-related Features */

static Temp_temp fp = NULL;
static Temp_temp sp = NULL;
static Temp_temp zero = NULL;
static Temp_temp ra = NULL;
static Temp_temp rv = NULL;

Temp_temp F_FP(void) {
  if (fp == NULL) {
    fp = Temp_newtemp();
  }
  return fp;
}

Temp_temp F_SP(void) {
  if (sp == NULL) {
    sp = Temp_newtemp();
  }
  return sp;
}

Temp_temp F_ZERO(void) {
  if (zero == NULL) {
    zero = Temp_newtemp();
  }
  return zero;
}

Temp_temp F_RA(void) {
  if (ra == NULL) {
    ra = Temp_newtemp();
  }
  return ra;
}

Temp_temp F_RV(void) {
  if (rv == NULL) {
    rv = Temp_newtemp();
  }
  return rv;
}

const int F_wordSize = 4; /* X86 */

T_exp F_Exp(F_access acc, T_exp framePtr) {
  return T_Mem(T_Binop(T_plus, framePtr, T_Const(F_accessOffset(acc))));
}

T_exp F_FPExp(T_exp framePtr) {
  return T_Mem(framePtr);
}

T_exp F_externalCall(string s, T_expList args) {
  return T_Call(T_Name(Temp_namedlabel(s)), args);
}

Temp_map F_tempMap = NULL;
