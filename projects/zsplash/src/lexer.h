#ifndef LEXER_H
#define LEXER_H

typedef enum
{
    TOK_END,
    TOK_CMD,
    TOK_NUM
} TokenType;

typedef struct
{
    TokenType type;
    char cmd;
    double number;
} Token;

Token next_token(char **p);

#endif