#include <stdlib.h>
#include <type.h>

static Type integer_type = {
    .kind = TYPE_INT,
    .size = 8,
    .alignment = 8,
};

static Type u8integer_type = {
    .kind = TYPE_U8,
    .size = 1,
    .alignment = 1,
};

Type *type_int = &integer_type;
Type *type_u8 = &u8integer_type;

Type *pointer_to(Type *base) {
    Type *type = calloc(1,sizeof(Type));
    type->kind = TYPE_POINTER;
    type->size = 8;
    type->alignment = 8;
    type->base = base;
    return type;
}

bool types_equal(Type *left, Type *right) {
  if (left == right)
    return true;
  if (!left || !right || left->kind != right->kind)
    return false;

  if (left->kind == TYPE_POINTER)
    return types_equal(left->base, right->base);
  
  if (left->kind == TYPE_STRUCT)
    return false;
  return true;
}