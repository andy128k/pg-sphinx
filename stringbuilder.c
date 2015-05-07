#include <string.h>
#include "stringbuilder.h"

static inline size_t align_size(size_t num)
{
  return (num + 1023) / 1024 * 1024;
}

StringBuilder *string_builder_new(void)
{
  StringBuilder *sb;

  sb = malloc(sizeof(struct _StringBuilder));
  sb->allocated_len = 0;
  sb->len = 0;
  sb->str = NULL;
  return sb;
}

void string_builder_reserve(StringBuilder *sb, size_t len)
{
  if (!sb || !len)
    return;
  if (sb->len + len >= sb->allocated_len)
    {
      sb->allocated_len = align_size(sb->len + len + 1);
      sb->str = realloc(sb->str, sb->allocated_len);
    }
}

void string_builder_append_length(StringBuilder *sb, const char *str, size_t len)
{
  if (!sb || !str || !len || !*str)
    return;
  string_builder_reserve(sb, len);
  memcpy(sb->str + sb->len, str, len);
  sb->len += len;
  sb->str[sb->len] = 0;
}

void string_builder_append(StringBuilder *sb, const char *str)
{
  if (!sb || !str || !*str)
    return;
  string_builder_append_length(sb, str, strlen(str));
}

void string_builder_append_pstr(StringBuilder *sb, const PString *pstr)
{
  if (!sb || !pstr)
    return;
  string_builder_append_length(sb, pstr->str, pstr->len);
}

char *string_builder_detach(StringBuilder *sb)
{
  char *str;

  if (!sb)
    return NULL;
  str = sb->str;
  sb->allocated_len = 0;
  sb->len = 0;
  sb->str = NULL;
  return str;
}

void string_builder_free(StringBuilder *sb)
{
  if (sb)
    {
      if (sb->str)
        free(sb->str);
      free(sb);
    }
}

