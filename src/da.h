#ifndef DA_H
#define DA_H

#define da_append(da, item)                                      \
  do {                                                           \
    if ((da)->count >= (da)->capacity) {                          \
      (da)->capacity = (da)->capacity ? (da)->capacity * 2 : 16; \
      (da)->items = realloc(                                     \
          (da)->items,                                           \
          (da)->capacity * sizeof(*(da)->items));                 \
    }                                                            \
    (da)->items[(da)->count++] = (item);                          \
  } while (0)

#endif
