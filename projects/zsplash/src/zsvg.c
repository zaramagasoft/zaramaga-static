#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Uso: %s logo.svg\n", argv[0]);
    return 1;
  }

  FILE *fp = fopen(argv[1], "rb");

  if (!fp) {
    perror("fopen");
    return 1;
  }

  /* Obtener tamaño del archivo */
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  rewind(fp);

  /* Reservar memoria */
  char *svg = malloc(size + 1);

  if (!svg) {
    perror("malloc");
    fclose(fp);
    return 1;
  }

  /* Leer archivo completo */
  if (fread(svg, 1, size, fp) != (size_t)size) {
    perror("fread");
    free(svg);
    fclose(fp);
    return 1;
  }

  svg[size] = '\0';

  fclose(fp);

  /* Buscar todos los atributos d="..." */
  char *p = svg;

  while ((p = strstr(p, " d=\"")) != NULL) {
    p += 4; /* saltar ' d="' */

    char *end = strchr(p, '"');

    if (!end)
      break;

    *end = '\0';

    char *cursor = p;

    Token t;

    while ((t = next_token(&cursor)).type != TOK_END) {
      if (t.type == TOK_CMD)
        printf("CMD  %c\n", t.cmd);

      else if (t.type == TOK_NUM)
        printf("NUM  %.4f\n", t.number);
    }

    printf("\n");

    p = end + 1;
  }

  free(svg);

  return 0;
}