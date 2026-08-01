#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>

static void skip_spaces(char **p) {
  while (**p) {
    if (isspace((unsigned char)**p) || **p == ',') {
      (*p)++;
      continue;
    }

    break;
  }
}

Token next_token(char **p) {
  Token tok;

  skip_spaces(p);

  tok.type = TOK_END;
  tok.cmd = 0;
  tok.number = 0.0;

  if (**p == '\0')
    return tok;

  if (isalpha((unsigned char)**p)) {
    tok.type = TOK_CMD;
    tok.cmd = **p;
    (*p)++;
    return tok;
  }

  char *end;

  tok.number = strtod(*p, &end);

  if (end != *p) {
    tok.type = TOK_NUM;
    *p = end;
    return tok;
  }

  (*p)++;

  return tok;
}