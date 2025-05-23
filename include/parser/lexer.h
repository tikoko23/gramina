#ifndef __GRAMINA_PARSER_LEXER_H
#define __GRAMINA_PARSER_LEXER_H

#define GRAMINA_WANT_TAGLESS

#include "common/array.h"
#include "common/stream.h"
#include "token.h"

#undef GRAMINA_WANT_TAGLESS

enum gramina_lex_error_code {
    GRAMINA_LEX_DONT_PUSH_TOK = -23,
    GRAMINA_LEX_ERR_EOF = EOF,
    GRAMINA_LEX_ERR_NONE = 23 - 23,
    GRAMINA_LEX_ERR_NO_TOK,
    GRAMINA_LEX_ERR_BAD_TOK,
    GRAMINA_LEX_ERR_UNKNOWN_CHAR,
    GRAMINA_LEX_ERR_OCCUPIED,
};

struct gramina_lex_result {
    GraminaArray(GraminaToken) tokens;
    enum gramina_lex_error_code status;
    struct gramina_token_position error_position;
    struct gramina_string error_description;
};

struct gramina_lex_result gramina_lex(GraminaStream *source);
struct gramina_string_view gramina_lex_error_code_to_str(enum gramina_lex_error_code code);
void gramina_lex_result_free(struct gramina_lex_result *this);

#endif
#include "gen/parser/lexer.h"
