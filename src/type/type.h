#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    TYPE_INT,
    TYPE_U8,
    TYPE_POINTER,
    TYPE_STRUCT,
} TypeKind;

typedef struct Type Type;
typedef struct StructMember StructMember;

struct Type {
    TypeKind kind;
    int size;
    int alignment;
    Type *base;
    StructMember *members;
};

struct StructMember {
    StructMember *next;
    char *name;
    size_t length;
    Type *type;
    int offset;
};

extern Type *type_int;
extern Type *type_u8;

Type *pointer_to(Type *base);
bool types_equal(Type *left, Type *right);

#endif
