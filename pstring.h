#ifndef PSTRING_H_INCLUDED
#define PSTRING_H_INCLUDED

#include <memory.h>

typedef struct _PString PString;

struct _PString
{
  char *str;
  size_t len;
};

#define PSTRING_CONST(str0) {str0, sizeof(str0) - 1}

#define PSTR_EMPTY(pstr) (!(pstr) || !(pstr)->len)
#define PSTR_NOT_EMPTY(pstr) ((pstr) && (pstr)->len)

#define PSRT_IS_EQUAL_TO(pstr, str0)                                     \
  ((pstr)->len == strlen(str0) &&                                        \
   0 == memcmp((pstr)->str, str0, (pstr)->len))

int pstring_is_one_of(const PString *pstr, ...);

int pstring_to_integer(const PString *pstr);

#endif

