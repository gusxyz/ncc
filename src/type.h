#ifndef TYPE_H
#define TYPE_H

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

Type *pointer_to(Type *base);

#endif
