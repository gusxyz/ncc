#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

typedef enum {
    TYPE_INT,
    TYPE_U8,
    TYPE_POINTER,
} TypeKind;

typedef struct Type Type;

struct Type {
    TypeKind kind;
    int size;
    int alignment;
    Type *base;
};

extern Type *type_int;
extern Type *type_u8;

Type *pointer_to(Type *base);
bool types_equal(Type *left, Type *right);

#endif
