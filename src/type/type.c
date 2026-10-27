#include <stdlib.h>
#include <type/type.h>
#include <error/error.h>
#include <limits.h>

static Type u8integer_type = {
    .kind = TYPE_U8,
    .size = 1,
    .alignment = 1,
};

static Type u16integer_type = {
    .kind = TYPE_U16,
    .size = 2,
    .alignment = 2,
};

static Type u32integer_type = {
    .kind = TYPE_U32,
    .size = 4,
    .alignment = 4,
};

static Type u64integer_type = {
    .kind = TYPE_U64,
    .size = 8,
    .alignment = 8,
};

static Type i64integer_type = {
    .kind = TYPE_I64,
    .size = 8,
    .alignment = 8,
};


Type *type_int = &i64integer_type;
Type *type_u8 = &u8integer_type;

Type *array_of(Type *base, int length) { 
  if(length <= 0) 
    error("Invalid array length");

  if(base->size <= 0)
    error("Array element requires a complete type");

  if(length > INT_MAX / base->size)
    error("Array size is too large");

  Type *type = calloc(1, sizeof(Type));
  type->kind = TYPE_ARRAY;
  type->base = base;
  type->size = base->size * length;
  type->alignment = base->alignment;
  type->array_length = length;

  return type;
}

Type *pointer_to(Type *base) {
  Type *type = calloc(1, sizeof(Type));
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