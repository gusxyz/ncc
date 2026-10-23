#include <stdlib.h>
#include <type.h>

static Type integer_type = {
    .kind = TYPE_INT,
    .size = 8,
    .alignment = 8,
};

Type *type_int = &integer_type;

Type *pointer_to(Type *base) {
    Type *type = calloc(1,sizeof(Type));
    type->kind = TYPE_POINTER;
    type->size = 8;
    type->alignment = 8;
    type->base = base;
    return type;
}