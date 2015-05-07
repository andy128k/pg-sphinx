#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

#define REPORT(error, ...) do {                         \
    if (error)                                          \
      *error = error_concat_strings(__VA_ARGS__, NULL); \
  } while(0)

char *error_concat_strings(const char *first, ...);

#endif

