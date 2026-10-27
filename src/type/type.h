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
    TYPE_ENUM,
} TypeKind;

typedef struct Type Type;
typedef struct StructMember StructMember;
typedef struct EnumMember EnumMember;
struct Type {
    TypeKind kind;
    int size;
    int alignment;
    Type *base;
    union {
        StructMember *struct_members;
        EnumMember *enum_members;
    };
    int array_length;
};

Type *array_of(Type *base, int length);

struct StructMember {
    int offset;
    size_t length;
    char *name;
    Type *type;
    StructMember *next;
};

struct EnumMember {
    int value;
    char *name;
    size_t length;
    EnumMember *next;
};

extern Type *type_int;
extern Type *type_u8;

Type *pointer_to(Type *base);
bool types_equal(Type *left, Type *right);

#endif
