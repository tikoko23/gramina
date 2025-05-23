#ifndef __GRAMINA_PARSER_H

#include "ast.h"
#include "parser/lexer.h"


#include "common/str.h"
#include "common/array.h"

struct gramina_parse_error {
    struct gramina_string description;
    struct gramina_token_position pos;
};
#endif

#define GRAMINA_WANT_TAGLESS
#include "gen/parser/parser.h"
#undef GRAMINA_WANT_TAGLESS

#ifndef __GRAMINA_PARSER_H
#define __GRAMINA_PARSER_H

GRAMINA_DECLARE_ARRAY(GraminaParseError);

struct gramina_parse_result {
    struct gramina_ast_node *root;
    struct gramina_parse_error error;
};

void gramina_parse_result_free(struct gramina_parse_result *this);

struct gramina_parse_result gramina_parse(const GraminaSlice(GraminaToken) *tokens); /* symname: "gramina_parse" */

#endif
