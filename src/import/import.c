#include "token/token.h"
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error/error.h>
#include <import/import.h>
#include <errno.h>
#include <da.h>

char *read_source_file(const char* path) {
  FILE *file = fopen(path, "rb");
  if(!file) {
    perror(path);
    exit(1);
  }

  if(fseek(file, 0, SEEK_END) != 0) {
    perror(path);
    exit(1);
  }

  long length = ftell(file);
  if (length < 0) {
    perror(path);
    exit(1);
  }

  rewind(file);

  char *source = calloc((size_t) length + 1, 1);
  if (!source) {
    perror("calloc");
    exit(1);
  }

  if(fread(source, 1, (size_t) length, file) != (size_t) length) {
    perror(path);
    exit(1);
  }

  fclose(file);
  return source;
}

typedef struct LoadedSource LoadedSource;

struct LoadedSource {
  char *path;
  LoadedSource *next;
};

static LoadedSource *loaded_sources;

bool mark_source_loaded(const char *path) {
    char *canonical = realpath(path, NULL);

    if(!canonical)
        error("Cannot resolve '%s': %s", path, strerror(errno));

    for(LoadedSource *file = loaded_sources; file; file = file->next) {
        if (strcmp(file->path, canonical) == 0) {
            free(canonical);
            return false;
        }
    }

    LoadedSource *file = calloc(1, sizeof(*file));
    if (!file)
        error("Out of memory");

    file->path = canonical;
    file->next = loaded_sources;
    loaded_sources = file;

    return true;
}

char *resolve_import_path(const char *source_path, const char *import_path) {
    if(import_path[0] == '/' || source_path == NULL) {
        char *path = malloc(strlen(import_path) + 1);
        if (!path)
            error("Out of memory");
        strcpy(path, import_path);
        return path;
    }


    const char *slash = strrchr(source_path, '/');
    size_t directory_length = slash ? (size_t) (slash - source_path + 1) : 0;

    char *path = malloc(directory_length + strlen(import_path) + 1);
    if (!path)
        error("Out of memory");

    memcpy(path, source_path, directory_length);
    strcpy(path + directory_length, import_path);
    return path;
}