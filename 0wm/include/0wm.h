/* vim: ft=c
 * 0wm core features
 */

#pragma once

#include <stdint.h>

struct OpcodeStream {
    const uint8_t *position;
    const uint8_t *end;
};
struct OpcodeRef {
    const uint8_t *position;
    uint8_t length;
};

struct Statement {
    uint16_t opcode;
    const uint8_t *args[3];
};

int8_t opstream_read(struct OpcodeStream *stream, struct OpcodeRef *op);
int8_t opstream_readstmt(struct OpcodeStream *stream, struct Statement *stmt);
uint16_t opcode_id(const uint8_t *opcode);

char stmt_ty(const uint8_t *arg);
uint8_t stmt_reg(const uint8_t *arg);
uint8_t stmt_off(const uint8_t *arg);
uint64_t stmt_val(const uint8_t *arg);

