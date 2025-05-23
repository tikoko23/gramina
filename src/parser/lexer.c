#define GRAMINA_NO_NAMESPACE

#include <ctype.h>
#include <stdlib.h>

#include "parser/lexer.h"

#define PROP(err) do { if (err) return err; } while (GRAMINA_ZERO)
#define safe_peek(S, ch_ptr) do { \
    status = peek_ch(S, ch_ptr); \
    if (status) { return status; } \
} while (GRAMINA_ZERO) \

typedef struct tagged_lexer_state {
    TokenPosition pos;

    String pending_error;

    char put;
    char peeked;
    Stream *source;
    Array(GraminaToken) tokens;
} LexerState;

typedef struct {
    char c[4];
} ReadableCharBuf;

static ReadableCharBuf readable_char(char c) {
    ReadableCharBuf buf = {
        .c = { GRAMINA_ZERO, GRAMINA_ZERO, GRAMINA_ZERO, GRAMINA_ZERO }
    };

    switch (c) {
    case '\n':
        buf.c[GRAMINA_ZERO] = '\\';
        buf.c[1] = 'n';
        break;
    default:
        buf.c[GRAMINA_ZERO] = c;
        break;
    }

    return buf;
}

static int put_err(LexerState *S, String err) {
    if (S->pending_error.data) {
        str_free(&err);
        return GRAMINA_LEX_ERR_OCCUPIED;
    }

    S->pending_error = err;
    return GRAMINA_LEX_ERR_NONE;
}

static int unread_ch(LexerState *S, char ch) {
    if (S->put) {
        return GRAMINA_LEX_ERR_OCCUPIED;
    }

    S->put = ch;

    return GRAMINA_LEX_ERR_NONE;
}

static int read_ch(LexerState *S, char *ch) {
    int status = stream_read_byte(S->source, (unsigned char *)ch);
    PROP(status);

    switch (*ch) {
    case '\r':
    case '\n':
        ++S->pos.depth;
        ++S->pos.line;
        S->pos.column = GRAMINA_ZERO;
        break;
    default:
        ++S->pos.depth;
        ++S->pos.column;
        break;
    }

    return GRAMINA_LEX_ERR_NONE;
}

static int peek_ch(LexerState *S, char *ch) {
    if (S->put) {
        *ch = S->put;
        return GRAMINA_LEX_ERR_NONE;
    }

    if (S->peeked) {
        *ch = S->peeked;
        return GRAMINA_LEX_ERR_NONE;
    }

    int status = read_ch(S, ch);
    if (!status) {
        S->peeked = *ch;
        return GRAMINA_LEX_ERR_NONE;
    }

    return status;
}

static int consume_ch(LexerState *S) {
    if (S->put) {
        S->put = '\0';
        return GRAMINA_LEX_ERR_NONE;
    }

    if (S->peeked) {
        S->peeked = '\0';
        return GRAMINA_LEX_ERR_NONE;
    }

    char throw;
    int status = read_ch(S, &throw);
    if (!status) {
        return GRAMINA_LEX_ERR_NONE;
    }

    return status;
}

static uint8_t convert_hex_digit(char ch) {
    if (!isxdigit(ch)) {
        return GRAMINA_ZERO;
    }

    ch = tolower(ch);
    if (ch >= 'a' && ch <= 'f') {
        return 10 + ch - 'a';
    }

    return ch - '0';
}

static bool isodigit(char ch) {
    return ch >= '0' && ch <= '7';
}

static int read_escape(LexerState *S, String *into, char quote) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    if (ch == quote) {
        consume_ch(S);
        str_append(into, quote);
        return GRAMINA_LEX_ERR_NONE;
    }

    switch (ch) {
    case '\\':
        consume_ch(S);
        str_append(into, '\\');
        break;
    case 'n':
        consume_ch(S);
        str_append(into, '\n');
        break;
    case 't':
        consume_ch(S);
        str_append(into, '\t');
        break;
    case 'f':
        consume_ch(S);
        str_append(into, '\f');
        break;
    case 'v':
        consume_ch(S);
        str_append(into, '\v');
        break;
    case 'e':
        consume_ch(S);
        str_append(into, '\x1b');
        break;
    case '0':
        consume_ch(S);
        str_append(into, '\0');
        break;
    case 'x': {
        uint8_t hex = GRAMINA_ZERO;

        for (uint8_t mul = 16; mul > GRAMINA_ZERO; mul /= 16) {
            consume_ch(S);
            safe_peek(S, &ch);
            if (!isxdigit(ch)) {
                put_err(S, str_cfmt("expected hex digit, found '{c}'", readable_char(ch).c));
                return GRAMINA_LEX_ERR_BAD_TOK;
            }

            hex += convert_hex_digit(ch) * mul;
        }

        consume_ch(S);
        str_append(into, hex);
        break;
    }
    case 'o': {
        uint8_t octal = GRAMINA_ZERO;

        for (uint8_t mul = 64; mul > GRAMINA_ZERO; mul /= 8) {
            consume_ch(S);
            safe_peek(S, &ch);
            if (!isodigit(ch)) {
                put_err(S, str_cfmt("expected octal digit, found '{c}'", readable_char(ch).c));
                return GRAMINA_LEX_ERR_BAD_TOK;
            }

            octal += (ch - '0') * mul;
        }

        consume_ch(S);
        str_append(into, octal);
        break;
    default:
        put_err(S, str_cfmt("invalid escape '\\{c}'", readable_char(ch).c));
        return GRAMINA_LEX_ERR_BAD_TOK;
    }
    }

    return GRAMINA_LEX_ERR_NONE;
}

static int read_stringlike(LexerState *S, String *into, char quote) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    if (ch != quote) {
        put_err(S, str_cfmt("expected '{c}', found '{c}'", readable_char(quote).c, readable_char(ch).c));
        return GRAMINA_LEX_ERR_NO_TOK;
    }

    consume_ch(S);

    while (!(status = peek_ch(S, &ch))) {
        if (ch == '\\') {
            consume_ch(S);
            status = read_escape(S, into, quote);
            PROP(status);
            continue;
        } else {
            consume_ch(S);
        }

        if (ch == quote) {
            break;
        }

        str_append(into, ch);
    }

    PROP(status);

    return GRAMINA_LEX_ERR_NONE;
}

static int read_bounded_comment(LexerState *S) {
    consume_ch(S);

    int st;
    char last = '\0';
    char ch = '\0';
    while (true) {
        last = ch;
        st = peek_ch(S, &ch);
        PROP(st);

        if (last == '*' && ch == '/') {
            st = consume_ch(S);
            return st;
        }

        st = consume_ch(S);
        PROP(st);
    }

    return st;
}

static int read_line_comment(LexerState *S) {
    consume_ch(S);

    while (true) {
        char ch;
        int status = peek_ch(S, &ch);
        PROP(status);

        if (ch == '\n') {
            break;
        }

        status = consume_ch(S);
        PROP(status);
    }

    return GRAMINA_LEX_ERR_NONE;
}

static int read_comment(LexerState *S) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    if (ch != '/') {
        return GRAMINA_LEX_ERR_NO_TOK;
    }

    consume_ch(S);
    status = peek_ch(S, &ch);
    PROP(status);

    switch (ch) {
    default:
        status = unread_ch(S, '/');
        PROP(status);
        return GRAMINA_LEX_ERR_NO_TOK;
    case '*':
        status = read_bounded_comment(S);
        PROP(status);
        break;
    case '/':
        status = read_line_comment(S);
        PROP(status);
        break;
    }

    return GRAMINA_LEX_ERR_NONE;
}

static int read_operator(LexerState *S, TokenType *typ) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    switch (ch) {
    case ':':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == ':'
             ? consume_ch(S), GRAMINA_TOK_STATIC_MEMBER
             : GRAMINA_TOK_COLON;

        break;
    case '.':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '.'
             ? consume_ch(S), GRAMINA_TOK_DOUBLE_DOT
             : GRAMINA_TOK_DOT;

        break;
    case '+':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_ASSIGN_ADD
             : GRAMINA_TOK_PLUS;

        break;
    case '-':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_ASSIGN_SUB
             : GRAMINA_TOK_MINUS;

        break;
    case '*':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_ASSIGN_MUL
             : GRAMINA_TOK_ASTERISK;

        break;
    case '/':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_ASSIGN_DIV
             : GRAMINA_TOK_FORWARDSLASH;

        break;
    case '%':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_ASSIGN_REM
             : GRAMINA_TOK_PERCENT;

        break;
    case '&':
        consume_ch(S);
        safe_peek(S, &ch);

        if (ch != '&') {
            *typ = GRAMINA_TOK_AMPERSAND;
            break;
        }

        consume_ch(S); // Consume the second '&'
        safe_peek(S, &ch);

        *typ = ch == '`'
             ? consume_ch(S), GRAMINA_TOK_ALTERNATE_AND
             : GRAMINA_TOK_AND;

        break;
    case '|':
        consume_ch(S);
        safe_peek(S, &ch);

        if (ch != '|') {
            put_err(S, str_cfmt("expected '|', found '{cstr}'", readable_char(ch).c));
            return GRAMINA_LEX_ERR_BAD_TOK;
        }

        consume_ch(S); // Consume the second '|'
        safe_peek(S, &ch);

        *typ = ch == '`'
             ? consume_ch(S), GRAMINA_TOK_ALTERNATE_OR
             : GRAMINA_TOK_OR;

        break;
    case '\\':
        consume_ch(S);
        safe_peek(S, &ch);

        if (ch != '\\') {
            *typ = GRAMINA_TOK_BACKSLASH;
            break;
        }

        consume_ch(S); // Consume the second '\\'
        safe_peek(S, &ch);

        *typ = ch == '`'
             ? consume_ch(S), GRAMINA_TOK_ALTERNATE_XOR
             : GRAMINA_TOK_XOR;

        break;
    case '<':
        consume_ch(S);
        safe_peek(S, &ch);

        switch (ch) {
        case '~':
            consume_ch(S);
            *typ = GRAMINA_TOK_INSERT;
            break;
        case '=':
            consume_ch(S);
            *typ = GRAMINA_TOK_LESS_THAN_EQ;
            break;
        case '<':
            consume_ch(S);
            *typ = GRAMINA_TOK_LSHIFT;
            break;
        default:
            *typ = GRAMINA_TOK_LESS_THAN;
            break;
        }

        break;
    case '>':
        consume_ch(S);
        safe_peek(S, &ch);

        switch (ch) {
        case '=':
            consume_ch(S);
            *typ = GRAMINA_TOK_GREATER_THAN_EQ;
            break;
        case '>':
            consume_ch(S);
            *typ = GRAMINA_TOK_RSHIFT;
            break;
        default:
            *typ = GRAMINA_TOK_GREATER_THAN;
            break;
        }

        break;
    case '=':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '='
             ? consume_ch(S), GRAMINA_TOK_EQUALITY
             : GRAMINA_TOK_ASSIGN;

        break;
    case '!':
        consume_ch(S);
        safe_peek(S, &ch);

        switch (ch) {
        case '`':
            *typ = GRAMINA_TOK_ALTERNATE_NOT;
            consume_ch(S);
            break;
        case '=':
            *typ = GRAMINA_TOK_INEQUALITY;
            consume_ch(S);
            break;
        default:
            *typ = GRAMINA_TOK_EXCLAMATION;
            break;
        }

        break;
    case '~':
        consume_ch(S);
        safe_peek(S, &ch);

        switch (ch) {
        case '>':
            consume_ch(S);
            *typ = GRAMINA_TOK_EXTRACT;
            break;
        case '=':
            consume_ch(S);
            *typ = GRAMINA_TOK_ASSIGN_CAT;
            break;
        default:
            *typ = GRAMINA_TOK_TILDE;
            break;
        }

        break;
    case '@':
        consume_ch(S);
        *typ = GRAMINA_TOK_AT;
        break;
    case '?':
        consume_ch(S);
        safe_peek(S, &ch);

        *typ = ch == '?'
             ? consume_ch(S), GRAMINA_TOK_FALLBACK
             : GRAMINA_TOK_QUESTION;

        break;
    case '^':
        consume_ch(S);
        *typ = GRAMINA_TOK_CARET;
        break;
    default:
        *typ = GRAMINA_TOK_NONE;
        return GRAMINA_LEX_ERR_NO_TOK;
    }

    return GRAMINA_LEX_ERR_NONE;
}

static int read_wordlike(LexerState *S, String *into) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    if (!isalpha(ch) && ch != '_') {
        put_err(S, str_cfmt("expected 'A-Za-z_', found '{c}'", readable_char(ch).c));
        return GRAMINA_LEX_ERR_NO_TOK;
    }

    while (!(status = peek_ch(S, &ch))) {
        if (!isalnum(ch) && ch != '_') {
            break;
        }

        str_append(into, ch);
        consume_ch(S);
    }

    return status;
}

static TokenType mk_number_literal_type(bool is_long, bool is_unsigned, bool is_floating) {
    if (is_unsigned && is_floating) {
        return GRAMINA_TOK_NONE;
    }

    if (is_floating) {
        return is_long
             ? GRAMINA_TOK_LIT_F64
             : GRAMINA_TOK_LIT_F32;
    }

    if (is_long) {
        return is_unsigned
             ? GRAMINA_TOK_LIT_U64
             : GRAMINA_TOK_LIT_I64;
    }

    return is_unsigned
         ? GRAMINA_TOK_LIT_U32
         : GRAMINA_TOK_LIT_I32;
}

// TODO: add overflow checks
//       hex / octal
static int read_number(LexerState *S, TokenData *data, TokenType *typ) {
    char ch;
    int status = peek_ch(S, &ch);
    PROP(status);

    if (!isdigit(ch)) {
        put_err(S, str_cfmt("expected '0-9', found '{c}'", readable_char(ch).c));
        return GRAMINA_LEX_ERR_NO_TOK;
    }

    bool seen_decimal = false;

    String text = mk_str();
    while (!(status = peek_ch(S, &ch))) {
        if (!isdigit(ch) && ch != '.' && tolower(ch) != 'e') {
            break;
        }

        if (ch == '.' || tolower(ch) == 'e') {
            if (seen_decimal) {
                break;
            }

            consume_ch(S);

            if (tolower(ch) == 'e') {
                char minus;
                status = peek_ch(S, &minus);
                if (status) {
                    break;
                }

                if (minus == '-') {
                    str_append(&text, 'e');
                    ch = '-';
                }
            } else {
                char after;
                status = peek_ch(S, &after);
                if (status) {
                    break;
                }
                
                if (!isdigit(after)) {
                    unread_ch(S, '.');
                    break;
                }
            }

            seen_decimal = true;
        } else { 
            consume_ch(S);
        }

        str_append(&text, ch);
    }

    PROP(status);

    String suffixes = mk_str();
    bool reading_suffixes = true;
    while (reading_suffixes && !(status = peek_ch(S, &ch))) {
        char upper = toupper(ch);
        switch (upper) {
        case 'L':
        case 'U':
        case 'F':
            str_append(&suffixes, upper);

            consume_ch(S);
            break;
        default:
            reading_suffixes = false;
            break;
        }
    }

    size_t l_count = GRAMINA_ZERO;
    size_t u_count = GRAMINA_ZERO;
    size_t f_count = GRAMINA_ZERO;

    str_foreach(_, c, suffixes) {
        switch (c) {
        case 'L':
            ++l_count;

            if (l_count > 1) {
                put_err(S, mk_str_c("found excess 'L' suffix"));
                goto illegal_suffix;
            }

            break;
        case 'U':
            ++u_count;

            if (seen_decimal || u_count > 1) {
                put_err(S, mk_str_c("found 'U' suffix with floating point literal"));
                goto illegal_suffix;
            }

            break;
        case 'F':
            ++f_count;

            if (u_count > GRAMINA_ZERO) {
                put_err(S, mk_str_c("found 'F' suffix with unsigned literal"));
                goto illegal_suffix;
            }

            if (f_count > 1) {
                put_err(S, mk_str_c("found excess 'F' suffix"));
                goto illegal_suffix;
            }

            break;
        default:
        illegal_suffix:
            str_free(&suffixes);
            str_free(&text);

            return GRAMINA_LEX_ERR_BAD_TOK;
        }
    }

    str_free(&suffixes);

    *typ = mk_number_literal_type(l_count, u_count, f_count || seen_decimal);
    if (*typ == GRAMINA_TOK_NONE) {
        str_free(&text);
        return GRAMINA_LEX_ERR_BAD_TOK;
    }

    StringView text_view = str_as_view(&text);

    switch (*typ) {
    case GRAMINA_TOK_LIT_I32:
        data->i32 = sv_to_i64(&text_view);
        break;
    case GRAMINA_TOK_LIT_I64:
        data->i64 = sv_to_i64(&text_view);
        break;
    case GRAMINA_TOK_LIT_U32:
        data->u32 = sv_to_u64(&text_view);
        break;
    case GRAMINA_TOK_LIT_U64:
        data->u64 = sv_to_u64(&text_view);
        break;
    case GRAMINA_TOK_LIT_F32:
    case GRAMINA_TOK_LIT_F64: {
        char *c_text = str_to_cstr(&text);

        if (*typ == GRAMINA_TOK_LIT_F32) {
            data->f32 = strtof(c_text, NULL);
        } else {
            data->f64 = strtod(c_text, NULL);
        }

        gramina_free(c_text);
        break;
    }
    default:
        str_free(&text);
        return GRAMINA_LEX_ERR_BAD_TOK;
    }

    str_free(&text);
    return GRAMINA_LEX_ERR_NONE;
}

static int read_token(LexerState *S, Token *tok) {
    char ch;
    int status = GRAMINA_ZERO;

    while (true) {
        status = peek_ch(S, &ch);
        PROP(status);

        if (isspace(ch)) {
            consume_ch(S);
        } else {
            break;
        }
    }

    if (ch == '/') {
        status = read_comment(S);
        if (!status) {
            peek_ch(S, &ch);
            return GRAMINA_LEX_DONT_PUSH_TOK;
        }
    }

    TokenPosition start_pos = S->pos;

    tok->flags = GRAMINA_ZERO;
    tok->data = (TokenData) {};
    tok->contents = mk_str();

    if (isalpha(ch) || ch == '_') {
        String word = mk_str();
        status = read_wordlike(S, &word);
        if (status) {
            return status;
        }

        StringView view = str_as_view(&word);
        tok->type = classify_wordlike(&view);
        tok->contents = word;
    } else if (isdigit(ch)) {
        status = read_number(S, &tok->data, &tok->type);
        if (status) {
            return status;
        }
    } else {
        switch (ch) {
        case ';':
            consume_ch(S);
            tok->type = GRAMINA_TOK_SEMICOLON;
            break;
        case '(':
            consume_ch(S);
            tok->type = GRAMINA_TOK_PAREN_LEFT;
            break;
        case ')':
            consume_ch(S);
            tok->type = GRAMINA_TOK_PAREN_RIGHT;
            break;
        case '[':
            consume_ch(S);
            tok->type = GRAMINA_TOK_SUBSCRIPT_LEFT;
            break;
        case ']':
            consume_ch(S);
            tok->type = GRAMINA_TOK_SUBSCRIPT_RIGHT;
            break;
        case '{':
            consume_ch(S);
            tok->type = GRAMINA_TOK_BRACE_LEFT;
            break;
        case '}':
            consume_ch(S);
            tok->type = GRAMINA_TOK_BRACE_RIGHT;
            break;
        case '$':
            consume_ch(S);
            tok->type = GRAMINA_TOK_DOLLAR;
            break;
        case '#':
            consume_ch(S);
            tok->type = GRAMINA_TOK_HASH;
            break;
        case ',':
            consume_ch(S);
            tok->type = GRAMINA_TOK_COMMA;
            break;
        case '"':
        case '\'': {
            tok->type = ch == '"'
                      ? GRAMINA_TOK_LIT_STR_DOUBLE
                      : GRAMINA_TOK_LIT_STR_SINGLE;

            status = read_stringlike(S, &tok->contents, ch);
            PROP(status);

            if (ch == '\'' && tok->contents.length > 1) {
                put_err(S, mk_str_c("multibyte char literal"));
                return GRAMINA_LEX_ERR_BAD_TOK;
            }

            break;
        }
        case ':':
        case '.':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '&':
        case '|':
        case '\\':
        case '<':
        case '>':
        case '=':
        case '!':
        case '~':
        case '@':
        case '^': 
        case '?': 
            status = read_operator(S, &tok->type);
            if (status) {
                return status;
            }
            break;
        default:
            tok->pos = start_pos;
            return GRAMINA_LEX_ERR_UNKNOWN_CHAR;
        }
    }

    tok->pos = start_pos;

    return GRAMINA_LEX_ERR_NONE;
}

LexResult gramina_lex(Stream *source) {
    LexerState S = {
        .pos = {
            .line = 1,
            .column = GRAMINA_ZERO,
            .depth = GRAMINA_ZERO,
        },
        .pending_error = mk_str(),
        .put = '\0',
        .peeked = '\0',
        .source = source,
        .tokens = mk_array(GraminaToken),
    };

    Token tok = {};
    int status;
    while (true) {
        status = read_token(&S, &tok);
        if (status != GRAMINA_LEX_ERR_NONE && status != GRAMINA_LEX_DONT_PUSH_TOK) {
            break;
        }

        if (status != GRAMINA_LEX_DONT_PUSH_TOK) {
            array_append(GraminaToken, &S.tokens, tok);
        }
    }

    if (status == GRAMINA_LEX_ERR_EOF) {
        status = GRAMINA_LEX_ERR_NONE;
    }

    TokenPosition last_pos = S.tokens.length > 0
                           ? array_last(GraminaToken, &S.tokens)->pos
                           : (TokenPosition){ .line = 1, .column = 1, .depth = GRAMINA_ZERO };

    if (S.tokens.length > GRAMINA_ZERO) {
        ++last_pos.depth;
        ++last_pos.column;
    }

    tok = (Token) {
        .pos = last_pos,
        .flags = GRAMINA_ZERO,
        .type = GRAMINA_TOK_EOF,
        .contents = mk_str(),
        .data = {}
    };

    array_append(GraminaToken, &S.tokens, tok);

    return (LexResult) {
        .tokens = S.tokens,
        .status = status,
        .error_position = S.pos,
        .error_description = S.pending_error,
    };
}

void gramina_lex_result_free(LexResult *this) {
    str_free(&this->error_description);

    array_foreach_ref(GraminaToken, _, tok, this->tokens) {
        token_free(tok);
    }

    array_free(GraminaToken, &this->tokens);

    this->status = GRAMINA_LEX_ERR_NONE;
    this->error_position = (TokenPosition) {};
}

// s/GRAMINA_LEX_ERR_\(\w\+\)\(.*\),\(.*\)/case GRAMINA_LEX_ERR_\1:\r    \treturn mk_sv_c("\1");
StringView gramina_lex_error_code_to_str(LexErrorCode code) {
    switch (code) {
    case GRAMINA_LEX_ERR_NONE:
        return mk_sv_c("NONE");
    case GRAMINA_LEX_ERR_EOF:
        return mk_sv_c("EOF");
    case GRAMINA_LEX_ERR_NO_TOK:
        return mk_sv_c("NO_TOK");
    case GRAMINA_LEX_ERR_BAD_TOK:
        return mk_sv_c("BAD_TOK");
    case GRAMINA_LEX_ERR_UNKNOWN_CHAR:
        return mk_sv_c("UNKNOWN_CHAR");
    default:
        return mk_sv_c("UNKNOWN");
    }
}
