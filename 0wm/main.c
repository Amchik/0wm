#include <stdio.h>
#include <stdlib.h>

#include "include/0wm.h"
#include "include/0wm.rt.h"

int main(int argc, char **argv) {
    FILE *fp;
    size_t len, i;
    uint8_t buff[1024];
    struct OpcodeStream stream;
    struct OpcodeRef op_ref;
    struct Statement stmt;
    struct VMContext ctx;

    if (argc != 2) {
        fputs("Usage: 0wm <file>\n", stderr);
        exit(1);
    }
    fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Failed to open file");
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(buff, 1, len, fp);
    fclose(fp);

    stream.position = buff;
    stream.end = buff + len;

    i = 0;
    while (opstream_read(&stream, &op_ref) > 0) {
        printf("\033[1;34m%4lu | \033[0m%.2X %.2X\e[2m", ++i, op_ref.position[0], op_ref.position[1]);
        for (len = 2; len < op_ref.length; ++len)
            printf(" %.2X", op_ref.position[len]);
        printf("\033[0m\n");
    }

    ctx = vmctx_new();
    stream.position = buff;
    len = 64;
    ctx.stmts = malloc(sizeof(stmt) * len);
    while (opstream_readstmt(&stream, &stmt) > 0) {
        if (ctx.stmts_count == len) {
            len += 64;
            ctx.stmts = realloc(ctx.stmts, sizeof(stmt) * len);
        }
        ctx.stmts[ctx.stmts_count++] = stmt;
    }

    while (vmctx_execline(&ctx) == 0);
    if (ctx.pub[0] != 0) {
        printf("\033[1;31merror\033[0m: recieved signal %ld at position %lu\n",
            ctx.pub[0], ctx.position);
    }

    return(0);
}
