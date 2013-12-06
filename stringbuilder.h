#ifndef STRING_BUILDER_H_INCLUDED
#define STRING_BUILDER_H_INCLUDED

#include <stdlib.h>
#include "pstring.h"

typedef struct _StringBuilder StringBuilder;

struct _StringBuilder
{
  char *str;
  size_t len;
  size_t allocated_len;
};

StringBuilder *string_builder_new(void);
void string_builder_append_length(StringBuilder *sb, const char *str, size_t len);
void string_builder_append(StringBuilder *sb, const char *str);
void string_builder_append_pstr(StringBuilder *sb, const PString *pstr);
char *string_builder_detach(StringBuilder *sb);
void string_builder_reserve(StringBuilder *sb, size_t len);
void string_builder_free(StringBuilder *sb);

#endif

