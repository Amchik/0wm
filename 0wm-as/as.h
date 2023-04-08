/* vim: ft=c
 * 0wm portable assembler
 */

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    char name[8];
    uint16_t opcode;
} defstmt_t;
typedef struct {
    char name[4];
    uint8_t reg;
} defreg_t;

typedef struct {
    void *ptr;
    uint64_t len;
    uint64_t cap;
} vec_t;

#define vec_get(v, id, ty) ((ty*)((v).ptr) + id)
#define vec_push(v, ty) (ty*)vec_void_push(v, sizeof(ty))
#define vec_alloc(cap, ty) vec_void_alloc(cap * sizeof(ty))
#define vec_cap(v, ty) ((v).cap / sizeof(ty))
#define vec_len(v, ty) ((v).len / sizeof(ty))

static vec_t vec_void_alloc(uint64_t cap) {
    vec_t v;
    v.cap = cap;
    v.len = 0;
    v.ptr = malloc(cap);
    return(v);
}
static void* vec_void_push(vec_t *v, uint64_t s) {
    if (v->cap < v->len + s)
        v->ptr = realloc(v->ptr, v->cap *= 2);
    v->len += s;
    return((char*)v->ptr + v->len - s);
}

