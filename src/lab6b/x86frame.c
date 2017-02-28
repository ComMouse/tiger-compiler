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

  // %ebp: old %ebp
  // %ebp + 4: static link
  // Arguments start from %ebp + 12
  // Local variables start from %ebp - 4 - 12 (callee save)
  int offset = 8;
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
  // escape=false
  if (escape == FALSE) {
    F_access l = InReg(Temp_newtemp());
    f->locals = F_AccessList(l, f->locals);
    return l;
  }

  // Locals start from %ebp - 4 - 12 (callee save)
  int offset = -4 - 12;
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

static int frameSize(F_frame f) {
  int size = 12;
  F_accessList locals = f->locals;
  while (locals != NULL) {
    if (locals->head->kind == inFrame)
      size += 4;
    locals = locals->tail;
  }
  return size;
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

F_frag F_newProcFrag(T_stm body, F_frame frame) {
  return F_ProcFrag(body, frame);
}

string F_string(Temp_label lab, string str) {
  string buf = (string)checked_malloc(sizeof(char) * (strlen(str) + 100));
  sprintf(buf, "%s: .ascii \"%s\"\n", Temp_labelstring(lab), str);
  return buf;
}

static Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
  F_fragList l = checked_malloc(sizeof(*l));
  l->head = head;
  l->tail = tail;
  return l;
}

static AS_instrList appendCalleeSave(AS_instrList il) {
  Temp_tempList calleesaves = Temp_reverseList(F_calleesaves());
  AS_instrList ail = il;
  for (; calleesaves; calleesaves = calleesaves->tail) {
    ail = AS_InstrList(
            AS_Oper("pushl `s0\n", L(F_SP(), NULL), L(calleesaves->head, NULL), NULL), ail);
  }

  return ail;
}

static AS_instrList restoreCalleeSave(AS_instrList il) {
  Temp_tempList calleesaves = F_calleesaves();
  AS_instrList ail = NULL;
  for (; calleesaves; calleesaves = calleesaves->tail) {
    ail = AS_InstrList(
            AS_Oper("popl `s0\n", L(F_SP(), NULL), L(calleesaves->head, NULL), NULL), ail);
  }

  return AS_splice(ail, il);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
  frameStack = frameStack->tail;
  return stm;
}

static Temp_tempList returnSink = NULL;

AS_instrList F_procEntryExit2(AS_instrList body) {
  Temp_tempList calleeSaves = F_calleesaves();
  if (!returnSink)
    returnSink = Temp_TempList(F_RA(),
                    Temp_TempList(F_SP(), calleeSaves));

  char inst_add[128];
  int frame_size = 100;//frameSize(frame);
  sprintf(inst_add, "addl $%d, `s0\n", frame_size);

  return AS_splice(body, 
          AS_InstrList(AS_Oper(String(inst_add), L(F_SP(), NULL), L(F_SP(), NULL), NULL),
            restoreCalleeSave(
              AS_InstrList(AS_Oper("leave\n", L(F_SP(), L(F_FP(), NULL)), L(F_SP(), NULL), NULL),
                AS_InstrList(AS_Oper("ret\n", NULL, returnSink, NULL), NULL)))));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
  char buf[1024], inst_lbl[128], inst_sub[128];
  int frame_size = 100;//frameSize(frame);
  sprintf(buf, "# PROCEDURE %s\n", S_name(frame->name));
  sprintf(inst_lbl, "%s:\n", S_name(frame->name));
  // sprintf(buf, "%s    pushl %%ebp\n", buf);
  // sprintf(buf, "%s    movl %%esp, %%ebp\n", buf);
  sprintf(inst_sub, "subl $%d, `s0\n", frame_size);
  body = AS_InstrList(AS_Label(String(inst_lbl), frame->name),
            AS_InstrList(AS_Oper("pushl `s0\n", L(F_FP(), L(F_SP(), NULL)), L(F_FP(), NULL), NULL),
              AS_InstrList(AS_Move("movl `s0, `d0\n", L(F_FP(), NULL), L(F_SP(), NULL)),
                appendCalleeSave(
                    AS_InstrList(AS_Oper(String(inst_sub), L(F_SP(), NULL), L(F_SP(), NULL), NULL),
                      body)))));
  return AS_Proc(String(buf), body, "# END\n");
}

/* Machine-related Features */

Temp_map F_tempMap = NULL;
const int F_wordSize = 4; /* X86 */

static Temp_temp eax = NULL;
static Temp_temp ecx = NULL;
static Temp_temp edx = NULL;
static Temp_temp ebx = NULL;
static Temp_temp esi = NULL;
static Temp_temp edi = NULL;

static Temp_temp fp = NULL;
static Temp_temp sp = NULL;
static Temp_temp zero = NULL;
static Temp_temp ra = NULL;
static Temp_temp rv = NULL;

static Temp_tempList registers = NULL;
static Temp_tempList specialregs = NULL;

Temp_temp F_FP(void) {
  if (fp == NULL) {
    F_initRegisters();
  }
  return fp;
}

Temp_temp F_SP(void) {
  if (sp == NULL) {
    F_initRegisters();
  }
  return sp;
}

// Zero register (not available in x86)
Temp_temp F_ZERO(void) {
  if (zero == NULL) {
    F_initRegisters();
  }
  return zero;
}

// Return address (not available in x86)
Temp_temp F_RA(void) {
  if (ra == NULL) {
    F_initRegisters();
  }
  return ra;
}

// Return value
Temp_temp F_RV(void) {
  if (rv == NULL) {
    F_initRegisters();
  }
  return rv;
}

Temp_temp F_EAX(void) {
  if (eax == NULL) {
    F_initRegisters();
  }
  return eax;
}

Temp_temp F_EDX(void) {
  if (edx == NULL) {
    F_initRegisters();
  }
  return edx;
}

void F_initRegisters(void)
{
  fp = Temp_newtemp();
  sp = Temp_newtemp();
  zero = Temp_newtemp();
  ra = Temp_newtemp();
  rv = Temp_newtemp();  
  eax = Temp_newtemp();
  ecx = Temp_newtemp();
  edx = Temp_newtemp();
  ebx = Temp_newtemp();
  esi = Temp_newtemp();
  edi = Temp_newtemp();

  Temp_enter(Temp_name(), eax, "%eax");
  Temp_enter(Temp_name(), ecx, "%ecx");
  Temp_enter(Temp_name(), edx, "%edx");
  Temp_enter(Temp_name(), ebx, "%ebx");
  Temp_enter(Temp_name(), esi, "%esi");
  Temp_enter(Temp_name(), edi, "%edi");
  
  specialregs = Temp_TempList(rv,
                  Temp_TempList(fp,
                    Temp_TempList(ra, NULL)));
}

Temp_map F_initialRegisters(F_frame f) {
  Temp_map m = Temp_empty();
  Temp_enter(m, fp, "%ebp");
  Temp_enter(m, sp, "%esp");
  Temp_enter(m, rv, "%eax");

  Temp_enter(m, eax, "%eax");
  Temp_enter(m, ecx, "%ecx");
  Temp_enter(m, edx, "%edx");
  Temp_enter(m, ebx, "%ebx");
  Temp_enter(m, esi, "%esi");
  Temp_enter(m, edi, "%edi");
  return m;
}

Temp_tempList F_registers(void) {
  if (fp == NULL) {
    F_initRegisters();
  }
  return //Temp_TempList(eax,
            Temp_TempList(ecx,
              Temp_TempList(edx,
                Temp_TempList(ebx,
                  Temp_TempList(esi,
                    Temp_TempList(edi, NULL)))));//);
}

Temp_tempList F_callersaves(void) {
  if (fp == NULL) {
    F_initRegisters();
  }
  return //Temp_TempList(eax,
            Temp_TempList(edx,
              Temp_TempList(ecx, NULL));//);
}

Temp_tempList F_calleesaves(void) {
  if (fp == NULL) {
    F_initRegisters();
  }
  return Temp_TempList(ebx,
            Temp_TempList(esi,
              Temp_TempList(edi, NULL)));
}

string F_getlabel(F_frame frame) {
  return Temp_labelstring(frame->name);
}

T_exp F_Exp(F_access acc, T_exp framePtr) {
  if (acc->kind == inReg) {
    return T_Temp(F_accessReg(acc));
  }
  return T_Mem(T_Binop(T_plus, framePtr, T_Const(F_accessOffset(acc))));
}

T_exp F_ExpWithStaticLink(F_access acc, T_exp staticLink) {
  if (acc->kind == inReg) {
    return T_Temp(F_accessReg(acc));
  }
  return T_Mem(T_Binop(T_plus, staticLink, T_Const(F_accessOffset(acc) - 8)));
}

T_exp F_FPExp(T_exp framePtr) {
  return T_Mem(framePtr);
}

T_exp F_staticLinkExp(T_exp framePtr) {
  // static link at fp + 8
  return T_Binop(T_plus, framePtr, T_Const(2 * F_wordSize));
}

T_exp F_upperStaticLinkExp(T_exp staticLink) {
  return T_Mem(staticLink);
}

T_exp F_staticLink2FP(T_exp staticLink) {
  return T_Binop(T_minus, T_Mem(staticLink), T_Const(2 * F_wordSize));
}

T_exp F_externalCall(string s, T_expList args) {
  return T_Call(T_Name(Temp_namedlabel(s)), args);
}

