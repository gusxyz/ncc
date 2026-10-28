#ifndef ERROR_H
#define ERROR_H

void set_user_input(char*);
char *get_user_input();
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void set_source_path(const char *path);
void expect(char op);
int expect_number();

#endif
