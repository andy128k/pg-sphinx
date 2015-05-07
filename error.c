#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"
#include "stringbuilder.h"

char *error_concat_strings(const char *first, ...)
{
  StringBuilder *sb;
  char *p;
  va_list argp;

  if (first == NULL)
    return NULL;

  sb = string_builder_new();
  string_builder_append(sb, first);
  va_start(argp, first);
  while ((p = va_arg(argp, char *)) != NULL)
    string_builder_append(sb, p);
  va_end(argp);

  p = string_builder_detach(sb);
  string_builder_free(sb);

  return p;
}

