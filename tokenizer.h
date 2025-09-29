#ifndef TOKENIZER_H
#define TOKENIZER_H

enum tok_t { WORD, LT, GT, BAR, NL, EOS };

enum tok_t next_token(char **token, int reset);

#endif