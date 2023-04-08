/* vim: ft=c
 * 0wm runtime
 */

#pragma once

#include <stdint.h>
#include "0wm.h"

typedef uint64_t pubregs_t[8];
typedef uint64_t regs_t[40];

struct VMContext {
    pubregs_t pub;
    regs_t    *regs;
    uint32_t  stack[2048];
    uint64_t  position;
    uint32_t  call_pos;
    uint32_t  call_cap;
    struct Statement *stmts;
    uint64_t         stmts_count;
};

enum VMOpcodes {
    VMOP_DIE  = 0x0000,
    VMOP_RET,

    VMOP_CALL = 0x2000,
    VMOP_JMP,
    VMOP_JME,
    /*VMOP_JMNE,
    VMOP_JMLT,
    VMOP_JMGT,
    VMOP_JMLE,
    VMOP_JMGE,*/
    
    VMOP_PRNT = 0x2100,

    VMOP_MOV = 0x4000,
    VMOP_ADD,
    VMOP_SUB,
    VMOP_MUL,
    VMOP_DIV,
    VMOP_IMUL,
    VMOP_IDIV,
    VMOP_SHR,
    VMOP_SHL,
    VMOP_AND,
    VMOP_OR,
    VMOP_XOR,
};

struct VMContext vmctx_new();

void vmctx_push(struct VMContext *ctx);
void vmctx_pop(struct VMContext *ctx);

uint64_t *vmctx_getreg(struct VMContext *ctx, uint8_t no);
uint64_t *vmctx_stmt_reg(struct VMContext *ctx, const uint8_t *arg);
uint8_t  *vmctx_stmt_mem(struct VMContext *ctx, const uint8_t *arg);
uint64_t *vmctx_stmt_ptr(struct VMContext *ctx, const uint8_t *arg);
uint64_t vmctx_stmt_val(struct VMContext *ctx, const uint8_t *arg);

int vmctx_execline(struct VMContext *ctx);

