#include <stdio.h>
#include <string.h>
#include "as.h"

#define MAX_DEPTH 8

#define ERROR(msg) do {\
    fprintf(stderr, "\033[1;31merror\033[0m: %s\n\033[1;34m%4lu |\033[0m %s", msg, ln, line); \
    exit(3); } while (0)

#define SKIP_WHITESPACE() for (; line[tp] == ' ' || line[tp] == '\t'; ++tp);
#define READ_WORD(buff, cond) for (i = tp; line[tp] != ' ' && line[tp] != '\t' && line[tp] != ',' && line[tp] != '\n' cond; ++tp) buff[tp - i] = line[tp];

#define FIND_REGISTER(reg, buff, i) do { \
    for (i = 0; i < vec_len(*v_regs, defreg_t); i += 1) \
        if (!strcmp(vec_get(*v_regs, i, defreg_t)->name, buff)) \
            break; \
    if (i == vec_len(*v_regs, defreg_t)) ERROR("expected known register"); \
    reg = vec_get(*v_regs, i, defreg_t); } while (0)

#define h2i(w) ('0' <= w && w <= '9' ? w - '0' : \
        'a' <= w && w <= 'f' ? w - 'a' + 10 : \
        'A' <= w && w <= 'F' ? w - 'A' + 10 : 0)
#define d2i(w) ('0' <= w && w <= '9' ? w - '0' : 0)
static uint64_t get_int(char *buff, size_t sz) {
    uint64_t res;
    char c;

    res = 0;
    if (buff[sz - 1] == 'h') {
        while (--sz) {
            c = *(buff++);
            res = res * 16 + h2i(c);
        }
        return(res);
    }
    while (sz--) {
        c = *(buff++);
        res = res * 10 + d2i(c);
    }

    return(res);
}
static void fputbyte(uint8_t b, FILE *fp) {
    fwrite(&b, 1, 1, fp);
}

void parse_file(FILE *fp, vec_t *v_opcodes, vec_t *v_regs, int layer) {
    char buff[256], *line;
    size_t off, ln, tp, i, j;
    ssize_t nread;
    defreg_t *reg;
    defstmt_t *stmt;
    FILE *fp2;

    line = 0;
    off = ln = 0;
    if (layer > MAX_DEPTH)
        ERROR("Max depth exceeded");

    while ((nread = getline(&line, &off, fp)) > 0) {
        ln++;
        tp = 0;
        SKIP_WHITESPACE();
        if (line[tp] == '\n' || line[tp] == ';')
            continue;
        if (line[tp] == '.') {
            if (!strncmp(line + tp + 1, "opcode ", 7)) {
                tp += 8;
                SKIP_WHITESPACE();
                stmt = vec_push(v_opcodes, defstmt_t);
                READ_WORD(stmt->name, && tp - i < sizeof(stmt->name));
                if (tp - i == sizeof(stmt->name))
                    ERROR("invalid statement length");
                stmt->name[tp - i] = 0;
                tp += 1;
                SKIP_WHITESPACE();
                READ_WORD(buff, );
                if (*buff == '-') ERROR("opcode id cannot be negative");
                if (tp == i) ERROR("expected opcode id");
                stmt->opcode = get_int(buff, tp - i);
            } else if (!strncmp(line + tp + 1, "reg ", 4)) {
                tp += 5;
                SKIP_WHITESPACE();
                reg = vec_push(v_regs, defreg_t);
                READ_WORD(reg->name, && tp - i < sizeof(reg->name));
                if (tp - i == sizeof(reg->name))
                    ERROR("invalid register length");
                reg->name[tp - i] = 0;
                tp += 1;
                SKIP_WHITESPACE();
                READ_WORD(buff, );
                if (*buff == '-') ERROR("register id cannot be negative");
                if (tp == i) ERROR("expected register id");
                reg->reg = get_int(buff, tp - i) % 0x30;
            } else if (!strncmp(line + tp + 1, "import ", 7)) {
                tp += 8;
                line[nread - 1] = 0;
                fp2 = fopen(line + tp, "r");
                if (!fp2) {
                    perror("\033[1;31merror\033[0m: unable to open file");
                    ERROR("invalid `import` break");
                }
                parse_file(fp2, v_opcodes, v_regs, layer + 1);
                fclose(fp2);
            } else {
                ERROR("invalid break");
            }
            continue;
        }
        READ_WORD(buff, );
        if (tp == i) ERROR("expeced opcode");
        buff[tp - i] = 0;
        for (i = 0; i < vec_len(*v_opcodes, defstmt_t); i += 1)
            if (!strcmp(vec_get(*v_opcodes, i, defstmt_t)->name, buff))
                break;
        if (i == vec_len(*v_opcodes, defstmt_t)) ERROR("expected known opcode name");
        j = vec_get(*v_opcodes, i, defstmt_t)->opcode;
        fputc((j >> 8) & 0xFF, stdout);
        fputc(j & 0xFF, stdout);
        for (;;) {
            tp += 1;
            SKIP_WHITESPACE();
            READ_WORD(buff, && line[tp] != 0);
            if (tp - i == 0) break;
            if (*buff == '$') {
                if (buff[1] == '$') {
                    fputbyte(0xC1, stdout);
                    j = get_int(buff + 2, tp - i - 2);
                    if (buff[2] == '-') j = -(int16_t)j;
                    fputc((j >> 8) & 0xFF, stdout);
                    fputc(j & 0xFF, stdout);
                } else {
                    fputbyte(0xC0, stdout);
                    j = get_int(buff + 1, tp - i - 1);
                    if (buff[1] == '-') j = -(int8_t)j;
                    fputc(j & 0xFF, stdout);
                }
            } else if (*buff == '&') {
                fputbyte(0xC2, stdout);
                j = get_int(buff + 1, tp - i - 1);
                if (buff[1] == '-') j = -(int32_t)j;
                fputc((j >> 24) & 0xFF, stdout);
                fputc((j >> 16) & 0xFF, stdout);
                fputc((j >> 8) & 0xFF, stdout);
                fputc(j & 0xFF, stdout);
            } else if (buff[tp - i - 1] == ')') {
                buff[tp - i - 1] = 0;
                if (*buff == '(') {
                    FIND_REGISTER(reg, buff + 1, j);
                    fputbyte(reg->reg + 0x30, stdout);
                } else if (*buff == '-') {
                    FIND_REGISTER(reg, buff, j);
                    for (j = 1; buff[j] != '(' && buff[j] != 0; ++j);
                    if (buff[j] == 0) ERROR("unexpected ')' (invalid dereference syntax)");
                    j = get_int(buff + 1, j - 1);
                    fputbyte(reg->reg + 0x90, stdout);
                    fputbyte(j & 0xFF, stdout);
                } else {
                    FIND_REGISTER(reg, buff, j);
                    for (j = 0; buff[j] != '(' && buff[j] != 0; ++j);
                    if (buff[i] == 0) ERROR("unexpected ')' (invalid dereference syntax)");
                    j = get_int(buff, j);
                    fputbyte(reg->reg + 0x60, stdout);
                    fputbyte(j & 0xFF, stdout);
                }
            } else {
                buff[tp - i] = 0;
                FIND_REGISTER(reg, buff, j);
                fputbyte(reg->reg, stdout);
            }
        }
    }
}

int main(int argc, char **argv) {
    vec_t v_opcodes, v_regs;
    FILE *fp;

    if (argc != 2) {
        fputs("Usage: 0wm-as <filename>\n", stderr);
        return(1);
    }

    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Failed to open file");
        return(2);
    }

    v_opcodes = vec_alloc(64, defstmt_t);
    v_regs = vec_alloc(64, defreg_t);

    parse_file(fp, &v_opcodes, &v_regs, 0);

    return(0);
}
