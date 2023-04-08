#include <stdio.h> /* for printf() */
#include <stdlib.h>
#include <string.h>

#include "include/0wm.h"
#include "include/0wm.rt.h"

struct VMContext vmctx_new() {
    struct VMContext res;

    memset(&res, 0, sizeof(res));
    res.call_cap = 32;
    res.regs = malloc(res.call_cap * sizeof(*res.regs));
    memset(*res.regs, 0, sizeof(*res.regs));

    return(res);
}

void vmctx_push(struct VMContext *ctx) {
    ctx->call_pos += 1;
    if (ctx->call_pos == ctx->call_cap) {
        ctx->call_cap += 32;
        ctx->regs = realloc(ctx->regs, ctx->call_cap * sizeof(*ctx->regs));
    }
    memset(ctx->regs[ctx->call_pos], 0, sizeof(*ctx->regs));
    ctx->regs[ctx->call_pos][0] = ctx->pub[7]; /* todo: macroses */
}
void vmctx_pop(struct VMContext *ctx) {
    ctx->pub[7] = ctx->regs[ctx->call_pos][0];
    ctx->call_pos -= 1;
}

uint64_t *vmctx_getreg(struct VMContext *ctx, uint8_t no) {
    return no < 8 ? ctx->pub + no : ctx->regs[ctx->call_pos] + no - 8;
}

__attribute__((deprecated("very shitty")))
uint64_t *vmctx_stmt_reg(struct VMContext *ctx, const uint8_t *arg) {
    if (stmt_ty(arg) == 0) return vmctx_getreg(ctx, stmt_reg(arg));
    else if (stmt_ty(arg) < 4) return (uint64_t*)*vmctx_getreg(ctx, stmt_reg(arg)) + stmt_off(arg);

    return(0);
}
uint8_t *vmctx_stmt_mem(struct VMContext *ctx, const uint8_t *arg) {
    int64_t off;

    if (stmt_ty(arg) == 0 || stmt_ty(arg) > 3) return(0);
    off = *vmctx_getreg(ctx, stmt_reg(arg)) + stmt_off(arg);
    if (off < 0 || off > sizeof(ctx->stack)) return(0);

    return (uint8_t*)ctx->stack + off;
}
uint64_t *vmctx_stmt_ptr(struct VMContext *ctx, const uint8_t *arg) {
    if (stmt_ty(arg) == 0) return vmctx_getreg(ctx, stmt_reg(arg));
    else if (stmt_ty(arg) < 4) return (uint64_t*)vmctx_stmt_mem(ctx, arg);

    return(0);
}
uint64_t vmctx_stmt_val(struct VMContext *ctx, const uint8_t *arg) {
    if (stmt_ty(arg) == 0)
        return *vmctx_getreg(ctx, stmt_reg(arg));
    else if (stmt_ty(arg) < 4)
        return *vmctx_stmt_mem(ctx, arg);
    else
        return stmt_val(arg);
}

#define CHECK_ARG_VALID(arg)   if (stmt_ty(arg) > 6) SIGNAL(5);
#define CHECK_REG_VALID(arg)   if (stmt_ty(arg) > 3) SIGNAL(6);
#define CHECK_MEM_CORRECT(ctx, arg) if (stmt_ty(arg) > 0 && stmt_ty(arg) < 4 && !vmctx_stmt_mem(ctx, arg)) SIGNAL(7);

#define SIGNAL(sig) do { ctx->pub[0] = sig; goto signaled; } while (0)
int vmctx_execline(struct VMContext *ctx) {
    struct Statement *stmt;
    uint64_t *ptr, rhs;

    if (ctx->position == ctx->stmts_count)
        return(1);
    else if (ctx->position > ctx->stmts_count)
        SIGNAL(1);
    stmt = ctx->stmts + ctx->position;
    ctx->position += 1;

    switch ((enum VMOpcodes)stmt->opcode) {
    case VMOP_DIE:
        SIGNAL(0);
    case VMOP_RET:
        if (ctx->call_pos == 0) SIGNAL(3);
        ctx->position = ctx->stack[*vmctx_getreg(ctx, 0x08)];
        *vmctx_getreg(ctx, 0x08) -= 8;
        vmctx_pop(ctx);
        break;

    case VMOP_CALL:
        CHECK_ARG_VALID(stmt->args[0]);
        CHECK_MEM_CORRECT(ctx, stmt->args[0]);
        vmctx_push(ctx);
        *vmctx_getreg(ctx, 0x08) += 8;
        ctx->stack[*vmctx_getreg(ctx, 0x08)] = ctx->position;
        ctx->position = vmctx_stmt_val(ctx, stmt->args[0]);
        goto end;
    case VMOP_JME:
        if (!*vmctx_getreg(ctx, 0x1a)) break;
        goto case_VMOP_JMP;
    case_VMOP_JMP:
    case VMOP_JMP:
        CHECK_ARG_VALID(stmt->args[0]);
        CHECK_MEM_CORRECT(ctx, stmt->args[0]);
        ctx->position = vmctx_stmt_val(ctx, stmt->args[0]);
        goto end;

    case VMOP_PRNT:
        CHECK_ARG_VALID(stmt->args[0]);
        CHECK_MEM_CORRECT(ctx, stmt->args[0]);
        printf("[%lu] 0x%lX\n", ctx->position, vmctx_stmt_val(ctx, stmt->args[0]));
        break;

    case VMOP_MOV:
        CHECK_REG_VALID(stmt->args[0]);
        CHECK_ARG_VALID(stmt->args[1]);
        CHECK_MEM_CORRECT(ctx, stmt->args[0]);
        CHECK_MEM_CORRECT(ctx, stmt->args[1]);
        *vmctx_stmt_ptr(ctx, stmt->args[0]) = vmctx_stmt_val(ctx, stmt->args[1]);
        break;
    case VMOP_ADD:
    case VMOP_SUB:
    case VMOP_MUL:
    case VMOP_DIV:
    case VMOP_IMUL:
    case VMOP_IDIV:
    case VMOP_SHR:
    case VMOP_SHL:
    case VMOP_AND:
    case VMOP_OR:
    case VMOP_XOR:
        CHECK_REG_VALID(stmt->args[0]);
        CHECK_ARG_VALID(stmt->args[1]);
        CHECK_MEM_CORRECT(ctx, stmt->args[0]);
        CHECK_MEM_CORRECT(ctx, stmt->args[1]);
        ptr = vmctx_stmt_ptr(ctx, stmt->args[0]);
        rhs = vmctx_stmt_val(ctx, stmt->args[1]);
        if (stmt->opcode == VMOP_ADD)
            *ptr += rhs;
        else if (stmt->opcode == VMOP_SUB)
            *ptr -= rhs;
        else if (stmt->opcode == VMOP_MUL)
            *ptr *= rhs;
        else if (stmt->opcode == VMOP_IMUL)
            *(int64_t*)ptr *= (int64_t)rhs;
        else if (stmt->opcode == VMOP_SHR)
            *ptr >>= rhs;
        else if (stmt->opcode == VMOP_SHL)
            *ptr <<= rhs;
        else if (stmt->opcode == VMOP_AND)
            *ptr &= rhs;
        else if (stmt->opcode == VMOP_OR)
            *ptr |= rhs;
        else if (stmt->opcode == VMOP_XOR)
            *ptr ^= rhs;
        else if (rhs == 0)
            SIGNAL(8);
        else if (stmt->opcode == VMOP_DIV)
            *ptr /= rhs;
        else if (stmt->opcode == VMOP_IDIV)
            *(int64_t*)ptr /= (int64_t)rhs;
        break;

    default:
        SIGNAL(2);
    }

end:
    return(0);

signaled:
    return(-1);
}
