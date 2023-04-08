#include "include/0wm.h"

int8_t opstream_read(struct OpcodeStream *stream, struct OpcodeRef *op) {
    uint8_t len, arg, acnt;

    if (stream->position == stream->end)
        return(0);
    op->position = stream->position;
    if ((stream->position += 2) > (stream->end + 1))
        return(-1);
    len = 2;
    acnt = *op->position % 0x80 / 0x20;
    while (acnt--) {
        arg = *(stream->position++);
        if (arg < 0x60)                { len += 1; }
        else if (arg <= 0xc0)          { len += 2; stream->position += 1; }
        else if (arg == 0xc1)          { len += 3; stream->position += 2; }
        else if (arg == 0xc2)          { len += 5; stream->position += 4; }
        else                           { return(-1); }
    }
    op->length = len;

    return(len);
}
int8_t opstream_readstmt(struct OpcodeStream *stream, struct Statement *stmt) {
    uint8_t len, arg, acnt, acnt_b;

    if (stream->position == stream->end)
        return(0);
    if ((stream->position += 2) > (stream->end + 1))
        return(-1);
    stmt->opcode = opcode_id(stream->position - 2);
    len = 2;
    acnt = acnt_b = (stmt->opcode >> 8) % 0x80 / 0x20;
    while (acnt) {
        arg = *stream->position;
        if (arg < 0x60)                { stmt->args[acnt_b - acnt--] = stream->position; len += 1; stream->position += 1; }
        else if (arg <= 0xc0)          { stmt->args[acnt_b - acnt--] = stream->position; len += 2; stream->position += 2; }
        else if (arg == 0xc1)          { stmt->args[acnt_b - acnt--] = stream->position; len += 3; stream->position += 3; }
        else if (arg == 0xc2)          { stmt->args[acnt_b - acnt--] = stream->position; len += 5; stream->position += 5; }
        else                           { return(-1); }
    }

    return(len);
}
uint16_t opcode_id(const uint8_t *opcode) {
    return((*opcode << 8) | *(opcode + 1));
}

char stmt_ty(const uint8_t *arg) {
    uint8_t p;
    p = *arg;
    if      (p < 0x30) return 0;
    else if (p < 0x60) return 1;
    else if (p < 0x90) return 2;
    else if (p < 0xc0) return 3;
    else               return p - 0xc0 + 4;
}
uint8_t stmt_reg(const uint8_t *arg) {
    return(*arg % 0x30);
}
uint8_t stmt_off(const uint8_t *arg) {
    return(*arg < 0x60 ? 0 : *arg < 0x90 ? arg[1] : -arg[1]);
}
uint64_t stmt_val(const uint8_t *arg) {
    switch (*arg) {
        case 0xc0: return arg[1];
        case 0xc1: return (arg[1] << 8) | arg[2];
        case 0xc2: return (arg[1] << 24)
                           | (arg[2] << 16)
                           | (arg[3] << 8)
                           | arg[4];
    }
    __builtin_unreachable();
}
