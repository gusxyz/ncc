#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    TYPE_INT,
    TYPE_POINTER,
    TYPE_STRUCT,
    TYPE_ARRAY,
    TYPE_I8,
    TYPE_U8,
    TYPE_I16,
    TYPE_U16,
    TYPE_I32,
    TYPE_U32,
    TYPE_I64,
    TYPE_U64,
} TypeKind;

typedef struct Type Type;
typedef struct StructMember StructMember;

struct Type {
    TypeKind kind;
    int size;
    int alignment;
    Type *base;
    StructMember *members;
    int array_length;
};

Type *array_of(Type *base, int length);

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
