#ifndef PSTRING_H_INCLUDED
#define PSTRING_H_INCLUDED

typedef struct _PString PString;

struct _PString
{
  char *str;
  size_t len;
};

#define PSTR_NOT_EMPTY(pstr) ((pstr) && (pstr)->len)

#endif

