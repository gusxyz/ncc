#ifndef IMPORT_H
#define IMPORT_H

#include <stdbool.h>

char *read_source_file(const char *path);
char *resolve_import_path(const char *source_path, const char *import_path);
bool mark_source_loaded(const char *path);

#endif