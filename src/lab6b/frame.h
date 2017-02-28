
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "temp.h"
#include "assem.h"

/* declaration for frames */
typedef struct F_frame_ *F_frame;
typedef struct F_access_ *F_access;

typedef struct F_accessList_ *F_accessList;
struct F_accessList_ {F_access head; F_accessList tail;};

int F_accessOffset(F_access a);
Temp_temp F_accessReg(F_access a);

F_accessList F_AccessList(F_access head, F_accessList tail);

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
      union {
        struct {Temp_label label; string str;} stringg;
        struct {T_stm body; F_frame frame;} proc;
      } u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {F_frag head; F_fragList tail;};
F_fragList F_FragList(F_frag head, F_fragList tail);

/* machine-related features */

extern Temp_map F_tempMap;
Temp_tempList F_registers(void);
string F_getlabel(F_frame frame);
T_exp F_Exp(F_access acc, T_exp framePtr);
T_exp F_ExpWithStaticLink(F_access acc, T_exp staticLink);
T_exp F_FPExp(T_exp framePtr);
T_exp F_upperStaticLinkExp(T_exp staticLink);
T_exp F_staticLinkExp(T_exp framePtr);
T_exp F_staticLink2FP(T_exp staticLink);

F_access F_allocLocal(F_frame f, bool escape);
F_accessList F_formals(F_frame f);
Temp_label F_name(F_frame f);

extern const int F_wordSize;
Temp_temp F_FP(void);
Temp_temp F_SP(void);
Temp_temp F_ZERO(void);
Temp_temp F_RA(void);
Temp_temp F_RV(void);
Temp_temp F_EAX(void);
Temp_temp F_EDX(void);
void F_initRegisters(void);
Temp_map F_initialRegisters(F_frame f);
Temp_tempList F_callersaves(void);
Temp_tempList F_calleesaves(void);

F_frame F_newFrame(Temp_label name, U_boolList formals);
T_exp F_externalCall(string s, T_expList args);
string F_string(Temp_label lab, string str);
F_frag F_newProcFrag(T_stm body, F_frame frame);

T_stm F_procEntryExit1(F_frame frame, T_stm stm);
AS_instrList F_procEntryExit2(AS_instrList body);
AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);
void F_procEntryExit(void);

#endif
