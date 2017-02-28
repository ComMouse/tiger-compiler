#include "util.h"
#include "symbol.h" 
#include "absyn.h"  
#include <stdlib.h>
#include <stdio.h>
#include "table.h"

typedef struct escapeEntry_ *escapeEntry;

struct escapeEntry_ {
  int d;
  bool* escape;
};

escapeEntry EscapeEntry(int d, bool* escape) {
  escapeEntry e = (escapeEntry)checked_malloc(sizeof(*e));
  e->d = d;
  e->escape = escape;
  return e;
}

static void traverseExp(S_table env, int depth, A_exp e);
static void traverseDec(S_table env, int depth, A_dec d);
static void traverseVar(S_table env, int depth, A_var v);

void Esc_findEscape(A_exp exp) {
    S_table env = S_empty();
    traverseExp(env, 0, exp);
}

static void traverseExp(S_table env, int depth, A_exp a) {
  switch (a->kind) {
    case A_varExp: {
      traverseVar(env, depth, a->u.var);
      break;
    } 
    case A_nilExp:
      break;
    case A_intExp:
      break;
    case A_stringExp:
      break;
    case A_callExp: {
      /* Compare para type */
      A_expList el;
      for (el = a->u.call.args; el; el = el->tail) {
        traverseExp(env, depth, el->head);
      }
      break;
    }
    case A_opExp: {
      traverseExp(env, depth, a->u.op.left);
      traverseExp(env, depth, a->u.op.right);
      break;
    }
    case A_recordExp: {
      /* Compare fields */
      A_efieldList el;
      for (el = a->u.record.fields; el; el = el->tail) {
        traverseExp(env, depth, el->head->exp);
      }
      break;
    }
    case A_seqExp: {
      A_expList list;
      for (list = a->u.seq; list != NULL; list = list->tail) {
        traverseExp(env, depth, list->head);
      }
      break;
    }
    case A_assignExp: {
      traverseVar(env, depth, a->u.assign.var);
      traverseExp(env, depth, a->u.assign.exp);
      break;
    }
    case A_ifExp: {
      traverseExp(env, depth, a->u.iff.test);
      traverseExp(env, depth, a->u.iff.then);
      if (a->u.iff.elsee) {
        traverseExp(env, depth, a->u.iff.elsee);
      }
      break;
    }
    case A_whileExp: {
      traverseExp(env, depth, a->u.whilee.test);
      traverseExp(env, depth, a->u.whilee.body);
      break;
    }
    case A_forExp: {
      traverseExp(env, depth, a->u.forr.lo);
      traverseExp(env, depth, a->u.forr.hi);
      
      S_beginScope(env);
      S_enter(env, a->u.forr.var, EscapeEntry(depth, &a->u.forr.escape));
      traverseExp(env, depth, a->u.forr.body);
      S_endScope(env);
      break;
    }
    case A_breakExp:
      break;
    case A_letExp: {
      A_decList d;
      S_beginScope(env);
      for (d = a->u.let.decs; d; d = d->tail) {
        traverseDec(env, depth, d->head);
      }
      traverseExp(env, depth, a->u.let.body);
      S_endScope(env);
      break;
    }
    case A_arrayExp: {
      traverseExp(env, depth, a->u.array.size);
      traverseExp(env, depth, a->u.array.init);
      break;
    }
    default:
      assert(0);
  }
}

static void traverseDec(S_table env, int depth, A_dec d) {
    switch (d->kind) {
    case A_functionDec: {
      A_fundecList funList;
      /* Second Handle */
      for (funList = d->u.function; funList; funList = funList->tail) {
        A_fundec f = funList->head;

        S_beginScope(env);
        A_fieldList l;
        for (l = f->params; l; l = l->tail) {
          S_enter(env, l->head->name, EscapeEntry(depth + 1, &l->head->escape));
        }
        traverseExp(env, depth + 1, f->body);
        S_endScope(env);
      }
      break;
    }
    case A_varDec: {
      traverseExp(env, depth, d->u.var.init);
      S_enter(env, d->u.var.var, EscapeEntry(depth, &d->u.var.escape));
      break;
    }
    case A_typeDec: {
      break;
    }
    default:
      assert(0);
  }
}

static void traverseVar(S_table env, int depth, A_var v) {
  switch (v->kind) {
    case A_simpleVar: {
      escapeEntry entry = S_look(env, v->u.simple);
      if (entry) {
        if (entry->d != depth) {
          *(entry->escape) = TRUE;
        }
      } else {
        EM_error(v->pos, "escape: undefined variable %s", S_name(v->u.simple));
      }
      break;
    }
    case A_fieldVar: {
      traverseVar(env, depth, v->u.field.var);
      break;
    }
    case A_subscriptVar: {
      traverseExp(env, depth, v->u.subscript.exp);
      traverseVar(env, depth, v->u.subscript.var);
      break;
    }
    default:
      assert(0);
  }
}
