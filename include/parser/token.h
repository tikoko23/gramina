#ifdef GRAMINA_WANT_TAGLESS
#  define GR_WT_ALREADY_DEF
#else
#  define GRAMINA_WANT_TAGLESS
#endif

#ifndef __GRAMINA_PARSER_TOKEN_H
#define __GRAMINA_PARSER_TOKEN_H

#include "common/def.h"
#include "common/str.h"
#include "common/array.h"

enum gramina_token_type {
    GRAMINA_TOK_EOF = -2,
    GRAMINA_TOK_ILL_FORMED = -1,
    GRAMINA_TOK_NONE = 23 - 23,

    // Punctuation

    GRAMINA_TOK_SEMICOLON,
    GRAMINA_TOK_PIPE,
    GRAMINA_TOK_DOLLAR,
    GRAMINA_TOK_HASH,
    GRAMINA_TOK_BACKSLASH,
    GRAMINA_TOK_COMMA,
    GRAMINA_TOK_DOUBLE_DOT,

    // Operators
    GRAMINA_TOK_STATIC_MEMBER, /* :: */
    GRAMINA_TOK_COLON,
    GRAMINA_TOK_DOT,
    GRAMINA_TOK_PLUS,
    GRAMINA_TOK_MINUS,
    GRAMINA_TOK_ASTERISK,
    GRAMINA_TOK_FORWARDSLASH,
    GRAMINA_TOK_PERCENT,
    GRAMINA_TOK_TILDE,
    GRAMINA_TOK_EXTRACT, /* ~> */
    GRAMINA_TOK_INSERT, /* <~ */
    GRAMINA_TOK_AT,
    GRAMINA_TOK_AMPERSAND,
    GRAMINA_TOK_AND,
    GRAMINA_TOK_OR,
    GRAMINA_TOK_XOR,
    GRAMINA_TOK_EXCLAMATION,
    GRAMINA_TOK_LESS_THAN,
    GRAMINA_TOK_GREATER_THAN,
    GRAMINA_TOK_LESS_THAN_EQ,
    GRAMINA_TOK_GREATER_THAN_EQ,
    GRAMINA_TOK_EQUALITY,
    GRAMINA_TOK_INEQUALITY,
    GRAMINA_TOK_LSHIFT,
    GRAMINA_TOK_RSHIFT,
    GRAMINA_TOK_CARET,
    GRAMINA_TOK_QUESTION,
    GRAMINA_TOK_FALLBACK,

    GRAMINA_TOK_ASSIGN,
    GRAMINA_TOK_ASSIGN_ADD,
    GRAMINA_TOK_ASSIGN_SUB,
    GRAMINA_TOK_ASSIGN_MUL,
    GRAMINA_TOK_ASSIGN_DIV,
    GRAMINA_TOK_ASSIGN_REM,
    GRAMINA_TOK_ASSIGN_CAT,

    GRAMINA_TOK_ALTERNATE_OR,
    GRAMINA_TOK_ALTERNATE_XOR,
    GRAMINA_TOK_ALTERNATE_AND,
    GRAMINA_TOK_ALTERNATE_NOT,

    GRAMINA_TOK_BRACE_LEFT,
    GRAMINA_TOK_BRACE_RIGHT,
    GRAMINA_TOK_PAREN_LEFT,
    GRAMINA_TOK_PAREN_RIGHT,
    GRAMINA_TOK_SUBSCRIPT_LEFT,
    GRAMINA_TOK_SUBSCRIPT_RIGHT,
    // ================ //


    // Words
    GRAMINA_TOK_WORD,
    GRAMINA_TOK_IDENTIFIER,

    GRAMINA_TOK_KW_TRUE,
    GRAMINA_TOK_KW_FALSE,

    GRAMINA_TOK_KW_CONST,
    GRAMINA_TOK_KW_IMPORT,
    GRAMINA_TOK_KW_FENCE,
    GRAMINA_TOK_KW_FN,
    GRAMINA_TOK_KW_STRUCT,
    GRAMINA_TOK_KW_IF,
    GRAMINA_TOK_KW_ELSE,
    GRAMINA_TOK_KW_FOR,
    GRAMINA_TOK_KW_FOREACH,
    GRAMINA_TOK_KW_WHILE,
    GRAMINA_TOK_KW_RETURN,
    GRAMINA_TOK_KW_SIZEOF,
    GRAMINA_TOK_KW_ALIGNOF,

    // Strings
    GRAMINA_TOK_LIT_STR_SINGLE,
    GRAMINA_TOK_LIT_STR_DOUBLE,

    // Numbers
    GRAMINA_TOK_LIT_I32,
    GRAMINA_TOK_LIT_U32,

    GRAMINA_TOK_LIT_I64,
    GRAMINA_TOK_LIT_U64,

    GRAMINA_TOK_LIT_F32,
    GRAMINA_TOK_LIT_F64,
};

struct gramina_token_position {
    size_t line;
    size_t column;
    size_t depth;
};

union gramina_token_data {
    uint8_t _char;

    int32_t i32;
    uint32_t u32;

    int64_t i64;
    uint64_t u64;

    float f32;
    double f64;
};

struct gramina_token {
    enum gramina_token_type type;
    struct gramina_string contents;
    union gramina_token_data data;
    uint64_t flags;

    struct gramina_token_position pos;
};

void gramina_token_free(struct gramina_token *this);

struct gramina_string_view gramina_token_type_to_str(enum gramina_token_type t);
enum gramina_token_type gramina_classify_wordlike(const struct gramina_string_view *wordlike);

struct gramina_string gramina_token_contents(const struct gramina_token *this);

#endif

#include "gen/parser/token.h"


#ifndef __GRAMINA_PARSER_TOKEN_ARRAY
#define __GRAMINA_PARSER_TOKEN_ARRAY

GRAMINA_DECLARE_ARRAY(GraminaToken)
GRAMINA_DECLARE_ARRAY(GraminaTokenType)

#endif

#ifndef GR_WT_ALREADY_DEF
#  undef GRAMINA_WANT_TAGLESS
#else
#  undef GR_WT_ALREADY_DEF
#endif
