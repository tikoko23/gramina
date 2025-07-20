#define GRAMINA_NO_NAMESPACE

#include "parser/token.h"

GRAMINA_IMPLEMENT_ARRAY(GraminaToken)
GRAMINA_IMPLEMENT_ARRAY(GraminaTokenType)

void gramina_token_free(Token *this) {
    str_free(&this->contents);

    this->pos.line = 0;
    this->pos.column = 0;
    this->pos.depth = 0;
    this->type = GRAMINA_TOK_ILL_FORMED;
    this->flags = 0;
}

TokenType gramina_classify_wordlike(const StringView *w) {
    if (false) {
    } else if (sv_cmp_c(w, "true") == 0) {
        return GRAMINA_TOK_KW_TRUE;
    } else if (sv_cmp_c(w, "false") == 0) {
        return GRAMINA_TOK_KW_FALSE;
    } else if (sv_cmp_c(w, "const") == 0) {
        return GRAMINA_TOK_KW_CONST;
    } else if (sv_cmp_c(w, "import") == 0) {
        return GRAMINA_TOK_KW_IMPORT;
    } else if (sv_cmp_c(w, "fence") == 0) {
        return GRAMINA_TOK_KW_FENCE;
    } else if (sv_cmp_c(w, "fn") == 0) {
        return GRAMINA_TOK_KW_FN;
    } else if (sv_cmp_c(w, "struct") == 0) {
        return GRAMINA_TOK_KW_STRUCT;
    } else if (sv_cmp_c(w, "if") == 0) {
        return GRAMINA_TOK_KW_IF;
    } else if (sv_cmp_c(w, "else") == 0) {
        return GRAMINA_TOK_KW_ELSE;
    } else if (sv_cmp_c(w, "for") == 0) {
        return GRAMINA_TOK_KW_FOR;
    } else if (sv_cmp_c(w, "foreach") == 0) {
        return GRAMINA_TOK_KW_FOREACH;
    } else if (sv_cmp_c(w, "while") == 0) {
        return GRAMINA_TOK_KW_WHILE;
    } else if (sv_cmp_c(w, "return") == 0) {
        return GRAMINA_TOK_KW_RETURN;
    } else if (sv_cmp_c(w, "sizeof") == 0) {
        return GRAMINA_TOK_KW_SIZEOF;
    } else if (sv_cmp_c(w, "alignof") == 0) {
        return GRAMINA_TOK_KW_ALIGNOF;
    } else {
        return GRAMINA_TOK_IDENTIFIER;
    }
}


String gramina_token_contents(const Token *this) {
    switch (this->type) {
    case GRAMINA_TOK_LIT_I32:
        return i32_to_str(this->data.i32);
    case GRAMINA_TOK_LIT_I64:
        return i64_to_str(this->data.i64);
    case GRAMINA_TOK_LIT_U32:
        return u32_to_str(this->data.u32);
    case GRAMINA_TOK_LIT_U64:
        return u64_to_str(this->data.u64);
    case GRAMINA_TOK_LIT_F32:
        return f32_to_str(this->data.f32);
    case GRAMINA_TOK_LIT_F64:
        return f64_to_str(this->data.f64);

    case GRAMINA_TOK_SEMICOLON:
        return mk_str_c(";");
    case GRAMINA_TOK_PIPE:
        return mk_str_c("|");
    case GRAMINA_TOK_DOLLAR:
        return mk_str_c("$");
    case GRAMINA_TOK_HASH:
        return mk_str_c("#");
    case GRAMINA_TOK_BACKSLASH:
        return mk_str_c("\\");
    case GRAMINA_TOK_COMMA:
        return mk_str_c(",");
    case GRAMINA_TOK_DOUBLE_DOT:
        return mk_str_c("..");

    // Operators
    case GRAMINA_TOK_STATIC_MEMBER:
        return mk_str_c("::");
    case GRAMINA_TOK_COLON:
        return mk_str_c(":");
    case GRAMINA_TOK_DOT:
        return mk_str_c(".");
    case GRAMINA_TOK_PLUS:
        return mk_str_c("+");
    case GRAMINA_TOK_MINUS:
        return mk_str_c("-");
    case GRAMINA_TOK_ASTERISK:
        return mk_str_c("*");
    case GRAMINA_TOK_FORWARDSLASH:
        return mk_str_c("/");
    case GRAMINA_TOK_PERCENT:
        return mk_str_c("%");
    case GRAMINA_TOK_TILDE:
        return mk_str_c("~");
    case GRAMINA_TOK_EXTRACT:
        return mk_str_c("~>");
    case GRAMINA_TOK_INSERT:
        return mk_str_c("<~");
    case GRAMINA_TOK_AT:
        return mk_str_c("@");
    case GRAMINA_TOK_AMPERSAND:
        return mk_str_c("&");
    case GRAMINA_TOK_AND:
        return mk_str_c("&&");
    case GRAMINA_TOK_OR:
        return mk_str_c("||");
    case GRAMINA_TOK_XOR:
        return mk_str_c("\\\\");
    case GRAMINA_TOK_EXCLAMATION:
        return mk_str_c("!");
    case GRAMINA_TOK_LESS_THAN:
        return mk_str_c("<");
    case GRAMINA_TOK_GREATER_THAN:
        return mk_str_c(">");
    case GRAMINA_TOK_LESS_THAN_EQ:
        return mk_str_c("<=");
    case GRAMINA_TOK_GREATER_THAN_EQ:
        return mk_str_c(">=");
    case GRAMINA_TOK_EQUALITY:
        return mk_str_c("==");
    case GRAMINA_TOK_INEQUALITY:
        return mk_str_c("!=");
    case GRAMINA_TOK_LSHIFT:
        return mk_str_c("<<");
    case GRAMINA_TOK_RSHIFT:
        return mk_str_c(">>");
    case GRAMINA_TOK_CARET:
        return mk_str_c("^");
    case GRAMINA_TOK_QUESTION:
        return mk_str_c("?");
    case GRAMINA_TOK_FALLBACK:
        return mk_str_c("??");

    case GRAMINA_TOK_ASSIGN:
        return mk_str_c("=");
    case GRAMINA_TOK_ASSIGN_ADD:
        return mk_str_c("+=");
    case GRAMINA_TOK_ASSIGN_SUB:
        return mk_str_c("-=");
    case GRAMINA_TOK_ASSIGN_MUL:
        return mk_str_c("*=");
    case GRAMINA_TOK_ASSIGN_DIV:
        return mk_str_c("/=");
    case GRAMINA_TOK_ASSIGN_REM:
        return mk_str_c("%=");
    case GRAMINA_TOK_ASSIGN_CAT:
        return mk_str_c("~=");

    case GRAMINA_TOK_ALTERNATE_OR:
        return mk_str_c("||`");
    case GRAMINA_TOK_ALTERNATE_XOR:
        return mk_str_c("\\\\`");
    case GRAMINA_TOK_ALTERNATE_AND:
        return mk_str_c("&&`");
    case GRAMINA_TOK_ALTERNATE_NOT:
        return mk_str_c("!`");

    case GRAMINA_TOK_BRACE_LEFT:
        return mk_str_c("{");
    case GRAMINA_TOK_BRACE_RIGHT:
        return mk_str_c("}");
    case GRAMINA_TOK_PAREN_LEFT:
        return mk_str_c("(");
    case GRAMINA_TOK_PAREN_RIGHT:
        return mk_str_c(")");
    case GRAMINA_TOK_SUBSCRIPT_LEFT:
        return mk_str_c("[");
    case GRAMINA_TOK_SUBSCRIPT_RIGHT:
        return mk_str_c("]");

    case GRAMINA_TOK_EOF:
        return mk_str_c("<eof>");
    default:
        return str_dup(&this->contents);
    }
}

// s/GRAMINA_TOK_\(\w\+\)\(.*\),\(.*\)/case GRAMINA_TOK_\1:\r        return mk_sv_c("\1");
StringView gramina_token_type_to_str(TokenType t) {
    switch (t) {
    case GRAMINA_TOK_EOF:
        return mk_sv_c("EOF");
    case GRAMINA_TOK_ILL_FORMED:
        return mk_sv_c("ILL_FORMED");
    case GRAMINA_TOK_NONE:
        return mk_sv_c("NONE");

    // Punctuation

    case GRAMINA_TOK_SEMICOLON:
        return mk_sv_c("SEMICOLON");
    case GRAMINA_TOK_PIPE:
        return mk_sv_c("PIPE");
    case GRAMINA_TOK_DOLLAR:
        return mk_sv_c("DOLLAR");
    case GRAMINA_TOK_HASH:
        return mk_sv_c("HASH");
    case GRAMINA_TOK_BACKSLASH:
        return mk_sv_c("BACKSLASH");
    case GRAMINA_TOK_COMMA:
        return mk_sv_c("COMMA");
    case GRAMINA_TOK_DOUBLE_DOT:
        return mk_sv_c("DOUBLE_DOT");

    // Operators
    case GRAMINA_TOK_STATIC_MEMBER:
        return mk_sv_c("STATIC_MEMBER");
    case GRAMINA_TOK_COLON:
        return mk_sv_c("COLON");
    case GRAMINA_TOK_DOT:
        return mk_sv_c("DOT");
    case GRAMINA_TOK_PLUS:
        return mk_sv_c("PLUS");
    case GRAMINA_TOK_MINUS:
        return mk_sv_c("MINUS");
    case GRAMINA_TOK_ASTERISK:
        return mk_sv_c("ASTERISK");
    case GRAMINA_TOK_FORWARDSLASH:
        return mk_sv_c("FORWARDSLASH");
    case GRAMINA_TOK_PERCENT:
        return mk_sv_c("PERCENT");
    case GRAMINA_TOK_TILDE:
        return mk_sv_c("TILDE");
    case GRAMINA_TOK_EXTRACT:
        return mk_sv_c("EXTRACT");
    case GRAMINA_TOK_INSERT:
        return mk_sv_c("INSERT");
    case GRAMINA_TOK_AT:
        return mk_sv_c("AT");
    case GRAMINA_TOK_AMPERSAND:
        return mk_sv_c("AMPERSAND");
    case GRAMINA_TOK_AND:
        return mk_sv_c("AND");
    case GRAMINA_TOK_OR:
        return mk_sv_c("OR");
    case GRAMINA_TOK_XOR:
        return mk_sv_c("XOR");
    case GRAMINA_TOK_EXCLAMATION:
        return mk_sv_c("EXCLAMATION");
    case GRAMINA_TOK_LESS_THAN:
        return mk_sv_c("LESS_THAN");
    case GRAMINA_TOK_GREATER_THAN:
        return mk_sv_c("GREATER_THAN");
    case GRAMINA_TOK_LESS_THAN_EQ:
        return mk_sv_c("LESS_THAN_EQ");
    case GRAMINA_TOK_GREATER_THAN_EQ:
        return mk_sv_c("GREATER_THAN_EQ");
    case GRAMINA_TOK_EQUALITY:
        return mk_sv_c("EQUALITY");
    case GRAMINA_TOK_INEQUALITY:
        return mk_sv_c("INEQUALITY");
    case GRAMINA_TOK_LSHIFT:
        return mk_sv_c("LSHIFT");
    case GRAMINA_TOK_RSHIFT:
        return mk_sv_c("RSHIFT");
    case GRAMINA_TOK_CARET:
        return mk_sv_c("CARET");
    case GRAMINA_TOK_QUESTION:
        return mk_sv_c("QUESTION");
    case GRAMINA_TOK_FALLBACK:
        return mk_sv_c("FALLBACK");

    case GRAMINA_TOK_ASSIGN:
        return mk_sv_c("ASSIGN");
    case GRAMINA_TOK_ASSIGN_ADD:
        return mk_sv_c("ASSIGN_ADD");
    case GRAMINA_TOK_ASSIGN_SUB:
        return mk_sv_c("ASSIGN_SUB");
    case GRAMINA_TOK_ASSIGN_MUL:
        return mk_sv_c("ASSIGN_MUL");
    case GRAMINA_TOK_ASSIGN_DIV:
        return mk_sv_c("ASSIGN_DIV");
    case GRAMINA_TOK_ASSIGN_REM:
        return mk_sv_c("ASSIGN_REM");
    case GRAMINA_TOK_ASSIGN_CAT:
        return mk_sv_c("ASSIGN_CAT");

    case GRAMINA_TOK_ALTERNATE_OR:
        return mk_sv_c("ALTERNATE_OR");
    case GRAMINA_TOK_ALTERNATE_XOR:
        return mk_sv_c("ALTERNATE_XOR");
    case GRAMINA_TOK_ALTERNATE_AND:
        return mk_sv_c("ALTERNATE_AND");
    case GRAMINA_TOK_ALTERNATE_NOT:
        return mk_sv_c("ALTERNATE_NOT");

    case GRAMINA_TOK_BRACE_LEFT:
        return mk_sv_c("BRACE_LEFT");
    case GRAMINA_TOK_BRACE_RIGHT:
        return mk_sv_c("BRACE_RIGHT");
    case GRAMINA_TOK_PAREN_LEFT:
        return mk_sv_c("PAREN_LEFT");
    case GRAMINA_TOK_PAREN_RIGHT:
        return mk_sv_c("PAREN_RIGHT");
    case GRAMINA_TOK_SUBSCRIPT_LEFT:
        return mk_sv_c("SUBSCRIPT_LEFT");
    case GRAMINA_TOK_SUBSCRIPT_RIGHT:
        return mk_sv_c("SUBSCRIPT_RIGHT");
    // ================ //


    // Words
    case GRAMINA_TOK_WORD:
        return mk_sv_c("WORD");
    case GRAMINA_TOK_IDENTIFIER:
        return mk_sv_c("IDENTIFIER");

    case GRAMINA_TOK_KW_TRUE:
        return mk_sv_c("KW_TRUE");
    case GRAMINA_TOK_KW_FALSE:
        return mk_sv_c("KW_FALSE");

    case GRAMINA_TOK_KW_CONST:
        return mk_sv_c("KW_CONST");
    case GRAMINA_TOK_KW_IMPORT:
        return mk_sv_c("KW_IMPORT");
    case GRAMINA_TOK_KW_FENCE:
        return mk_sv_c("KW_FENCE");
    case GRAMINA_TOK_KW_FN:
        return mk_sv_c("KW_FN");
    case GRAMINA_TOK_KW_STRUCT:
        return mk_sv_c("KW_STRUCT");
    case GRAMINA_TOK_KW_IF:
        return mk_sv_c("KW_IF");
    case GRAMINA_TOK_KW_ELSE:
        return mk_sv_c("KW_ELSE");
    case GRAMINA_TOK_KW_FOR:
        return mk_sv_c("KW_FOR");
    case GRAMINA_TOK_KW_FOREACH:
        return mk_sv_c("KW_FOREACH");
    case GRAMINA_TOK_KW_WHILE:
        return mk_sv_c("KW_WHILE");
    case GRAMINA_TOK_KW_RETURN:
        return mk_sv_c("KW_RETURN");

    // Strings
    case GRAMINA_TOK_LIT_STR_SINGLE:
        return mk_sv_c("LIT_STR_SINGLE");
    case GRAMINA_TOK_LIT_STR_DOUBLE:
        return mk_sv_c("LIT_STR_DOUBLE");

    // Numbers
    case GRAMINA_TOK_LIT_I32:
        return mk_sv_c("LIT_I32");
    case GRAMINA_TOK_LIT_U32:
        return mk_sv_c("LIT_U32");

    case GRAMINA_TOK_LIT_I64:
        return mk_sv_c("LIT_I64");
    case GRAMINA_TOK_LIT_U64:
        return mk_sv_c("LIT_U64");

    case GRAMINA_TOK_LIT_F32:
        return mk_sv_c("LIT_F32");
    case GRAMINA_TOK_LIT_F64:
        return mk_sv_c("LIT_F64");

    default:
        return mk_sv_c("UNKNOWN");
    }
}
