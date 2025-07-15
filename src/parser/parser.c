#define GRAMINA_NO_NAMESPACE
#define PAUSE_ON_ERR 0

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/array.h"
#include "common/log.h"

#include "parser/ast.h"
#include "parser/attributes.h"
#include "parser/parser.h"

#define SV_NULL ((StringView) { .length = 0, .data = NULL })

#undef slice
#define sslice(T, this, start, end) gramina_ ## T ## _slice(this, start, end)
#define CURRENT(S) ((S)->tokens->items[(S)->index])
#define N_AFTER(S, n) ((S)->tokens->items[(S)->index + (n)])
#define CONSUME(S) ((S)->tokens->items[(S)->index++])
#define SET_ERR(S, str) ((S)->has_error = true, (S)->error = (str))
#define HAS_ERR(S) ((S)->has_error)
#define CLEAR_ERR(S) ((S)->has_error ? str_free(&(S)->error) : 0, (S)->has_error = false)

#define CHAINABLE_EXPRESSION_RULE(this, higher)                         \
    AstNode *value_node = higher(S);                                    \
    if (!value_node) {                                                  \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    TokenPosition pos = CURRENT(S).pos;                                 \
                                                                        \
    AstNodeType typ;                                                    \
    AstNode *right = this ## _pr(S, &typ);                              \
    if (!right) {                                                       \
        return value_node;                                              \
    }                                                                   \
                                                                        \
    AstNode *this = mk_ast_node_lr(NULL, value_node, right);            \
    this->type = typ;                                                   \
    this->pos = pos;                                                    \
                                                                        \
    return this                                                         \

#define CHAIN_SELF(this, higher)                                        \
    AstNode *value_node = higher(S);                                    \
    if (!value_node) {                                                  \
        return NULL;                                                    \
    }                                                                   \
                                                                        \
    AstNodeType subtyp;                                                 \
    AstNode *right = this(S, &subtyp);                                  \
    if (!right) {                                                       \
        return value_node;                                              \
    }                                                                   \
                                                                        \
    AstNode *this = mk_ast_node_lr(NULL, value_node, right);            \
    this->type = subtyp;                                                \
    this->pos = cur.pos;                                                \
                                                                        \
    return this                                                         \


struct tagged_parser_state;

typedef struct tagged_parser_state {
    size_t index;
    const Slice(GraminaToken) *tokens;
    struct gramina_string error;
    bool has_error;
} ParserState;

void gramina_parse_result_free(ParseResult *this) {
    if (!this->root) {
        str_free(&this->error.description);
    } else {
        ast_node_free(this->root);
    }
}

static void print_parser_state(const ParserState *S);
static AstNode *global_statement(ParserState *S);
ParseResult gramina_parse(const Slice(GraminaToken) *tokens) {
    ParserState S = {
        .index = 0,
        .tokens = tokens,
        .has_error = false,
    };

    AstNode *last = global_statement(&S);
    AstNode *root = last;
    size_t index = (size_t)-1;
    while (S.tokens->items[S.index].type != GRAMINA_TOK_EOF) {
        AstNode *st = global_statement(&S);
        if (!st) {
            break;
        }

        if (index == S.index) {
            print_parser_state(&S);
            SET_ERR(&S, mk_str_c("infinite loop!"));
            break;
        }

        index = S.index;

        ast_node_child_r(last, st);
        last = st;
    }

    if (S.has_error) {
        elog_fmt("Parser: {s}\n", &S.error);

        ast_node_free(root);
        root = NULL;

        const Token *last = &tokens->items[S.index];
        String contents = token_contents(last);
        str_cat_cfmt(&S.error, ", found '{s}'", &contents);
        str_free(&contents);
    }

    return (ParseResult) {
        .root = root,
        .error = {
            .description = S.error,
            .pos = tokens->items[S.index].pos,
        },
    };
}

static void print_parser_state(const ParserState *S) {
    slice_foreach_ref(GraminaToken, i, tok, *S->tokens) {
        StringView typ = token_type_to_str(tok->type);
        char pre = i == S->index
                 ? '>'
                 : ' ';

        String contents = token_contents(tok);
        printf("%c %.*s: %.*s\n", pre, (int)typ.length, typ.data, (int)contents.length, contents.data);
        str_free(&contents);
    }

    puts("");
}

static AstNode *typename(ParserState *S);
static AstNode *identifier(ParserState *S);
static AstNode *param_list_pr(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_COMMA) {
        return NULL;
    }

    CONSUME(S);

    AstNode *type = typename(S);
    if (!type) {
        return NULL;
    }

    AstNode *name = identifier(S);
    if (!name) {
        ast_node_free(type);
        return NULL;
    }

    ast_node_child_l(name, type);

    AstNode *right = param_list_pr(S);
    AstNode *this = mk_ast_node_lr(NULL, name, right);
    this->type = GRAMINA_AST_PARAM_LIST;
    this->pos = type->pos;

    return this;
}

static AstNode *param_list(ParserState *S) {
    if (CURRENT(S).type == GRAMINA_TOK_PAREN_RIGHT) {
        return NULL;
    }

    AstNode *type = typename(S);
    if (!type) {
        return NULL;
    }

    AstNode *name = identifier(S);
    if (!name) {
        ast_node_free(type);
        return NULL;
    }

    ast_node_child_l(name, type);

    if (CURRENT(S).type != GRAMINA_TOK_COMMA) {
        return name;
    }

    AstNode *right = param_list_pr(S);
    AstNode *this = mk_ast_node_lr(NULL, name, right);
    this->type = GRAMINA_AST_PARAM_LIST;
    this->pos = type->pos;

    return this;
}

static AstNode *function_body(ParserState *S);
static AstNode *function_def(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_KW_FN) {
        return NULL;
    }

    CONSUME(S);

    AstNode *name = identifier(S);
    if (!name) {
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_PAREN_LEFT) {
        SET_ERR(S, mk_str_c("expected '('"));

        ast_node_free(name);
        return NULL;
    }

    CONSUME(S);

    AstNode *params = param_list(S);
    if (HAS_ERR(S)) {
        ast_node_free(name);
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_PAREN_RIGHT) {
        if (params) {
            SET_ERR(S, mk_str_c("expected ')'"));
        } else {
            SET_ERR(S, mk_str_c("expected parameter list or ')'"));
        }

        ast_node_free(name);
        ast_node_free(params);
        return NULL;
    }

    CONSUME(S);

    AstNode *return_type = NULL;
    if (CURRENT(S).type == GRAMINA_TOK_MINUS) {
        CONSUME(S);

        if (CURRENT(S).type != GRAMINA_TOK_GREATER_THAN) {
            SET_ERR(S, mk_str_c("expected '->'"));

            ast_node_free(name);
            ast_node_free(params);
            return NULL;
        }

        CONSUME(S);

        return_type = typename(S);
        if (HAS_ERR(S)) {
            ast_node_free(name);
            ast_node_free(params);
            return NULL;
        }
    }

    if (CURRENT(S).type != GRAMINA_TOK_BRACE_LEFT && CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        SET_ERR(S, mk_str_c("expected '{' or ';'"));

        ast_node_free(name);
        ast_node_free(params);
        ast_node_free(return_type);
        return NULL;
    }

    if (CURRENT(S).type == GRAMINA_TOK_SEMICOLON) {
        AstNode *this = mk_ast_node_lr(NULL, NULL, NULL);
        this->type = GRAMINA_AST_FUNCTION_DECLARATION;
        this->pos = name->pos;
        this->value.identifier = str_dup(&name->value.identifier);

        AstNode *typ = mk_ast_node_lr(this, params, return_type);
        typ->type = GRAMINA_AST_FUNCTION_TYPE;
        typ->pos = name->pos;

        ast_node_free(name);

        CONSUME(S);

        return this;
    }

    CONSUME(S);

    AstNode *contents = function_body(S);
    if (HAS_ERR(S)) {
        ast_node_free(name);
        ast_node_free(params);
        ast_node_free(return_type);
        return NULL;
    }

    AstNode *this = mk_ast_node_lr(NULL, NULL, contents);
    this->type = GRAMINA_AST_FUNCTION_DEF;
    this->pos = name->pos;
    this->value.identifier = str_dup(&name->value.identifier);

    AstNode *typ = mk_ast_node_lr(this, params, return_type);
    typ->type = GRAMINA_AST_FUNCTION_TYPE;
    typ->pos = name->pos;

    ast_node_free(name);

    if (CURRENT(S).type != GRAMINA_TOK_BRACE_RIGHT) {
        if (contents) {
            SET_ERR(S, mk_str_c("expected '}'"));
        } else {
            SET_ERR(S, mk_str_c("expected function body or '}'"));
        }

        ast_node_free(contents);
        return NULL;
    }

    CONSUME(S);

    return this;
}

static AstNode *statement(ParserState *S);
static AstNode *function_body(ParserState *S) {
    AstNode *root = NULL;
    AstNode *current = root;

    while (CURRENT(S).type != GRAMINA_TOK_BRACE_RIGHT) {
        AstNode *this = statement(S);
        if (!this) {
            if (!HAS_ERR(S)) {
                SET_ERR(S, mk_str_c("expected statement"));
            }

            break;
        }

        if (current == NULL) {
            root = this;
            current = this;
        } else {
            ast_node_child_r(current, this);
        }

        current = this;
    }

    if (HAS_ERR(S)) {
        ast_node_free(root);
        return NULL;
    }

    return root;
}

static AstNode *struct_field(ParserState *S) {
    AstNode *type = typename(S);
    if (!type) {
        return NULL;
    }

    AstNode *field = identifier(S);
    if (!field) {
        ast_node_free(type);
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        SET_ERR(S, mk_str_c("expected ';'"));

        ast_node_free(field);
        ast_node_free(type);
        return NULL;
    }

    CONSUME(S);

    ast_node_child_l(field, type);

    AstNode *this = mk_ast_node_lr(NULL, field, NULL);
    this->type = GRAMINA_AST_STRUCT_FIELD;
    this->pos = type->pos;

    return this;
}

static AstNode *struct_def(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_KW_STRUCT) {
        SET_ERR(S, mk_str_c("expected 'struct'"));
        return NULL;
    }

    TokenPosition def_pos = CURRENT(S).pos;

    CONSUME(S);

    AstNode *name = identifier(S);
    if (!name) {
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_BRACE_LEFT) {
        SET_ERR(S, mk_str_c("expected '{'"));

        ast_node_free(name);
        return NULL;
    }

    CONSUME(S);

    AstNode *this = mk_ast_node_lr(NULL, name, NULL);
    this->type = GRAMINA_AST_STRUCT_DEF;
    this->pos = def_pos;

    AstNode *cur = this;
    while (CURRENT(S).type != GRAMINA_TOK_BRACE_RIGHT) {
        AstNode *field = struct_field(S);
        if (HAS_ERR(S)) {
            ast_node_free(this);
            return NULL;
        }

        ast_node_child_r(cur, field);
        cur = field;
    }

    CONSUME(S);

    return this;
}

static AstNode *global_statement(ParserState *S) {
    Array(_GraminaSymAttr) attribs = mk_array(_GraminaSymAttr);
    while (CURRENT(S).type == GRAMINA_TOK_HASH) {
        CONSUME(S);

        if (CURRENT(S).type != GRAMINA_TOK_IDENTIFIER) {
            SET_ERR(S, mk_str_c("expected attribute name"));

            array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                symattr_free(attr);
            }

            array_free(_GraminaSymAttr, &attribs);
            return NULL;
        }

        const Token *attrib_tok = &CURRENT(S);
        CONSUME(S);

        StringView contents = mk_sv_c("");

        if (CURRENT(S).type == GRAMINA_TOK_PAREN_LEFT) {
            CONSUME(S);

            if (CURRENT(S).type != GRAMINA_TOK_LIT_STR_DOUBLE) {
                SET_ERR(S, mk_str_c("expected string literal"));

                array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                    symattr_free(attr);
                }

                array_free(_GraminaSymAttr, &attribs);
                return NULL;
            }

            contents = str_as_view(&CURRENT(S).contents);
            CONSUME(S);

            if (CURRENT(S).type != GRAMINA_TOK_PAREN_RIGHT) {
                SET_ERR(S, mk_str_c("expected ')'"));

                array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                    symattr_free(attr);
                }

                array_free(_GraminaSymAttr, &attribs);
                return NULL;
            }

            CONSUME(S);
        }

        StringView attrib_name = str_as_view(&attrib_tok->contents);
        SymbolAttributeKind kind = get_attrib_kind(&attrib_name);
        if (kind == GRAMINA_ATTRIBUTE_NONE) {
            SET_ERR(S, str_cfmt("unknown attribute '{sv}'", &attrib_name));

            array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                symattr_free(attr);
            }

            array_free(_GraminaSymAttr, &attribs);
            return NULL;
        }

        SymbolAttribute attrib = {
            .kind = kind,
            .string = {
                .length = 0,
                .data = 0,
            },
        };

        if (contents.data) {
            attrib.string = sv_dup(&contents);
        }

        array_append(_GraminaSymAttr, &attribs, attrib);
    }

    switch (CURRENT(S).type) {
    case GRAMINA_TOK_KW_FN: {
        AstNode *def = function_def(S);
        if (!def) {
            if (!HAS_ERR(S)) {
                SET_ERR(S, mk_str_c("expected function definition"));
            }

            array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                symattr_free(attr);
            }

            array_free(_GraminaSymAttr, &attribs);
            return NULL;
        }

        AstNode *this = mk_ast_node_lr(NULL, def, NULL);
        this->type = GRAMINA_AST_GLOBAL_STATEMENT;
        this->pos = def->pos;

        def->value.attributes = attribs;

        return this;
    }
    case GRAMINA_TOK_KW_STRUCT: {
        AstNode *def = struct_def(S);
        if (!def) {
            if (!HAS_ERR(S)) {
                SET_ERR(S, mk_str_c("expected struct definition"));
            }

            array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
                symattr_free(attr);
            }

            array_free(_GraminaSymAttr, &attribs);
            return NULL;
        }

        AstNode *this = mk_ast_node_lr(NULL, def, NULL);
        this->type = GRAMINA_AST_STRUCT_DEF;
        this->pos = def->pos;

        def->value.attributes = attribs;

        return this;
    }
    default:
        break;
    }

    array_foreach_ref(_GraminaSymAttr, _, attr, attribs) {
        symattr_free(attr);
    }

    array_free(_GraminaSymAttr, &attribs);

    if (!HAS_ERR(S)) {
        SET_ERR(S, mk_str_c("expected 'fn' or 'struct'"));
    }

    return NULL;
}

static AstNode *assignment_exp(ParserState *S);
static AstNode *expression(ParserState *S) {
    return assignment_exp(S);
}

static AstNode *expression_statement(ParserState *S) {
    if (CURRENT(S).type == GRAMINA_TOK_SEMICOLON) {
        CONSUME(S);
        return NULL;
    }

    AstNode *expr = expression(S);
    if (HAS_ERR(S)) {
        ast_node_free(expr);
        return NULL;
    }

    TokenPosition semicolon_pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        SET_ERR(S, mk_str_c("expected ';'"));

        ast_node_free(expr);
        return NULL;
    }

    CONSUME(S);

    AstNode *this = mk_ast_node_lr(NULL, expr, NULL);
    this->type = GRAMINA_AST_EXPRESSION_STATEMENT;
    this->pos = expr
              ? expr->pos
              : semicolon_pos;

    return this;
}

static AstNode *typename(ParserState *S);
static AstNode *identifier(ParserState *S);
static AstNode *declaration_statement(ParserState *S) {
    AstNode *type = typename(S);
    if (!type) {
        if (!HAS_ERR(S)) {
            SET_ERR(S, mk_str_c("expected typename"));
        }

        return NULL;
    }

    AstNode *ident = identifier(S);
    if (!ident) {
        SET_ERR(S, mk_str_c("expected identifier"));

        ast_node_free(type);
        return NULL;
    }

    AstNode *this = mk_ast_node_lr(NULL, ident, NULL);
    this->type = GRAMINA_AST_DECLARATION_STATEMENT;
    this->pos = type->pos;

    ast_node_child_l(ident, type);

    if (CURRENT(S).type == GRAMINA_TOK_ASSIGN) {
        CONSUME(S);

        AstNode *expr = expression(S);
        if (!expr) {
            if (!HAS_ERR(S)) {
                SET_ERR(S, mk_str_c("expected expression"));
            }

            ast_node_free(this);
            return NULL;
        }

        ast_node_child_r(ident, expr);
    }

    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        SET_ERR(S, mk_str_c("expected ';'"));
        ast_node_free(this);
        return NULL;
    }

    CONSUME(S);

    return this;
}

static AstNode *return_statement(ParserState *S) {
    TokenPosition pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_KW_RETURN) {
        SET_ERR(S, mk_str_c("expected 'return'"));
        return NULL;
    }

    CONSUME(S);

    if (CURRENT(S).type == GRAMINA_TOK_SEMICOLON) {
        CONSUME(S);
        AstNode *this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_RETURN_STATEMENT;
        this->pos = pos;

        return this;
    }

    AstNode *expr = expression(S);
    if (!expr) {
        if (!HAS_ERR(S)) {
            SET_ERR(S, mk_str_c("expected expression"));
        }

        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        SET_ERR(S, mk_str_c("expected ';'"));

        ast_node_free(expr);
        return NULL;
    }

    CONSUME(S);

    AstNode *this = mk_ast_node_lr(NULL, expr, NULL);
    this->type = GRAMINA_AST_RETURN_STATEMENT;
    this->pos = pos;

    return this;
}

static AstNode *statement_block(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_BRACE_LEFT) {
        SET_ERR(S, mk_str_c("expected '{'"));
        return NULL;
    }

    CONSUME(S);

    AstNode *body = function_body(S);
    if (HAS_ERR(S)) {
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_BRACE_RIGHT) {
        SET_ERR(S, mk_str_c("expected '}'"));

        return NULL;
    }

    CONSUME(S);

    return body;
}

static AstNode *if_statement(ParserState *S) {
    TokenPosition pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_KW_IF) {
        SET_ERR(S, mk_str_c("expected 'if'"));
        return NULL;
    }

    CONSUME(S);

    AstNode *cond = expression(S);
    if (HAS_ERR(S)) {
        return NULL;
    }

    AstNode *body = statement_block(S);
    if (HAS_ERR(S)) {
        ast_node_free(cond);
        return NULL;
    }

    AstNode *else_clause = NULL;
    if (CURRENT(S).type == GRAMINA_TOK_KW_ELSE) {
        TokenPosition else_pos = CURRENT(S).pos;
        CONSUME(S);

        if (CURRENT(S).type == GRAMINA_TOK_KW_IF) {
            AstNode *else_if = if_statement(S);
            else_clause = mk_ast_node_lr(NULL, else_if, NULL);
        } else {
            AstNode *else_body = statement_block(S);
            else_clause = mk_ast_node_lr(NULL, else_body, NULL);
        }

        else_clause->type = GRAMINA_AST_ELSE_CLAUSE;
        else_clause->pos = else_pos;
    }

    AstNode *linker = mk_ast_node_lr(NULL, body, else_clause);
    linker->type = GRAMINA_AST_CONTROL_FLOW;
    linker->pos = pos;

    AstNode *this = mk_ast_node_lr(NULL, cond, linker);
    this->type = GRAMINA_AST_IF_STATEMENT;
    this->pos = pos;

    AstNode *wrapper = mk_ast_node_lr(NULL, this, NULL);
    wrapper->type = GRAMINA_AST_CONTROL_FLOW;
    wrapper->pos = pos;

    return wrapper;
}

static AstNode *for_statement(ParserState *S) {
    TokenPosition pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_KW_FOR) {
        SET_ERR(S, mk_str_c("expected 'for'"));
        return NULL;
    }

    CONSUME(S);

    AstNode *decl = NULL;
    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        decl = declaration_statement(S);
        if (HAS_ERR(S)) {
            return NULL;
        }
    } else {
        CONSUME(S);
    }

    AstNode *cond = NULL;
    if (CURRENT(S).type != GRAMINA_TOK_SEMICOLON) {
        cond = expression(S);
        if (HAS_ERR(S)) {
            ast_node_free(decl);
            return NULL;
        }
    }

    CONSUME(S);

    AstNode *incr = NULL;
    if (CURRENT(S).type != GRAMINA_TOK_BRACE_LEFT) {
        incr = expression(S);
        if (HAS_ERR(S)) {
            ast_node_free(decl);
            ast_node_free(cond);
            return NULL;
        }
    }

    AstNode *body = statement_block(S);
    if (HAS_ERR(S)) {
        ast_node_free(decl);
        ast_node_free(cond);
        ast_node_free(incr);

        return NULL;
    }

    AstNode *st_condition = mk_ast_node_lr(NULL, cond, incr);
    AstNode *st_declaration = mk_ast_node_lr(NULL, decl, st_condition);
    AstNode *this = mk_ast_node_lr(NULL, st_declaration, body);
    AstNode *wrapper = mk_ast_node_lr(NULL, this, NULL);

    st_condition->type = GRAMINA_AST_EXPRESSION_STATEMENT;
    st_declaration->type = GRAMINA_AST_DECLARATION_STATEMENT;
    this->type = GRAMINA_AST_FOR_STATEMENT;
    wrapper->type = GRAMINA_AST_CONTROL_FLOW;

    st_condition->pos = cond->pos;
    st_declaration->pos = decl->pos;
    this->pos = pos;
    wrapper->pos = pos;

    return wrapper;
}

static AstNode *while_statement(ParserState *S) {
    TokenPosition pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_KW_WHILE) {
        SET_ERR(S, mk_str_c("expected 'while'"));
        return NULL;
    }

    CONSUME(S);

    AstNode *exp = expression(S);
    if (HAS_ERR(S)) {
        return NULL;
    }

    AstNode *body = statement_block(S);
    if (HAS_ERR(S)) {
        ast_node_free(exp);
        return NULL;
    }

    AstNode *this = mk_ast_node_lr(NULL, exp, body);
    this->type = GRAMINA_AST_WHILE_STATEMENT;
    this->pos = pos;

    AstNode *wrapper = mk_ast_node_lr(NULL, this, NULL);
    wrapper->type = GRAMINA_AST_CONTROL_FLOW;
    wrapper->pos = pos;

    return wrapper;
}

static AstNode *statement(ParserState *S) {
    switch (CURRENT(S).type) {
    case GRAMINA_TOK_KW_RETURN:
        return return_statement(S);
    case GRAMINA_TOK_KW_FOR:
        return for_statement(S);
    case GRAMINA_TOK_KW_WHILE:
        return while_statement(S);
    case GRAMINA_TOK_KW_IF:
        return if_statement(S);
    default:
        break;
    }

    ParserState Sp = *S;
    AstNode *exp_node = expression_statement(&Sp);
    if (exp_node) {
        *S = Sp;
        return exp_node;
    }

    CLEAR_ERR(&Sp);

    return declaration_statement(S);
}

static AstNode *lit_bool(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_KW_TRUE
     && CURRENT(S).type != GRAMINA_TOK_KW_FALSE) {
        return NULL;
    }

    AstNode *this = mk_ast_node(NULL);
    this->type = GRAMINA_AST_VAL_BOOL;
    this->pos = CURRENT(S).pos;
    this->value.logical = CURRENT(S).type == GRAMINA_TOK_KW_TRUE;

    CONSUME(S);

    return this;
}

static AstNode *lit_number(ParserState *S) {
    AstNode *this = NULL;
    Token cur = CURRENT(S);

    switch (cur.type) {
    case GRAMINA_TOK_LIT_F32:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_F32;
        this->value.f32 = cur.data.f32;
        break;
    case GRAMINA_TOK_LIT_F64:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_F64;
        this->value.f64 = cur.data.f64;
        break;
    case GRAMINA_TOK_LIT_I32:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_I32;
        this->value.i32 = cur.data.i32;
        break;
    case GRAMINA_TOK_LIT_U32:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_U32;
        this->value.u32 = cur.data.u32;
        break;
    case GRAMINA_TOK_LIT_I64:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_I64;
        this->value.i64 = cur.data.i64;
        break;
    case GRAMINA_TOK_LIT_U64:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_U64;
        this->value.u64 = cur.data.u64;
        break;
    default:
        return NULL;
    }

    this->pos = cur.pos;

    CONSUME(S);
    return this;
}

static AstNode *lit_quoted(ParserState *S) {
    AstNode *this = NULL;
    Token cur = CURRENT(S);

    switch (cur.type) {
    case GRAMINA_TOK_LIT_STR_SINGLE:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_CHAR;
        this->value._char = cur.data._char;
        break;
    case GRAMINA_TOK_LIT_STR_DOUBLE:
        this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_VAL_STRING;
        this->value.string = str_dup(&cur.contents);
        break;
    default:
        return NULL;
    }

    this->pos = cur.pos;

    CONSUME(S);
    return this;
}

static AstNode *identifier(ParserState *S) {
    Token cur = CURRENT(S);
    if (cur.type != GRAMINA_TOK_IDENTIFIER) {
        SET_ERR(S, mk_str_c("expected identifier"));

        return NULL;
    }

    AstNode *node = mk_ast_node(NULL);
    node->type = GRAMINA_AST_IDENTIFIER;
    node->value.identifier = str_dup(&cur.contents);
    node->pos = cur.pos;

    CONSUME(S);
    return node;
}

static bool handle_array_type(ParserState *S, AstNode **cur, bool *const_next) {
    *cur = mk_ast_node_lr(NULL, *cur, NULL);
    (*cur)->type = GRAMINA_AST_TYPE_ARRAY;
    (*cur)->pos = N_AFTER(S, -1).pos;

    if (*const_next) {
        (*cur)->flags |= GRAMINA_AST_CONST_TYPE;
        *const_next = false;
    }

    const Token *tok = &CURRENT(S);

    switch (tok->type) {
    case GRAMINA_TOK_LIT_U32:
    case GRAMINA_TOK_LIT_U64: {
        uint64_t n = tok->type == GRAMINA_TOK_LIT_U32
                   ? (size_t)tok->data.u32
                   : (size_t)tok->data.u64;

        // This check is necessary since length could be `0`, which passes all other checks
        if (n <= 0) {
            goto err;
        }

        (*cur)->value.array_length = n;
        break;
    }
    case GRAMINA_TOK_LIT_I32:
    case GRAMINA_TOK_LIT_I64: {
        int64_t n = tok->type == GRAMINA_TOK_LIT_I32
                  ? (int64_t)tok->data.i32
                  : (int64_t)tok->data.i64;

        // This check is necessary since length could be `0`, which passes all other checks
        if (n <= 0) {
            goto err;
        }

        (*cur)->value.array_length = n;
        break;
    }

    err:
    default:
        SET_ERR(S, mk_str_c("array length must be a positive integer literal"));
        return true;
    }

    CONSUME(S);

    if (CURRENT(S).type != GRAMINA_TOK_SUBSCRIPT_RIGHT) {
        SET_ERR(S, mk_str_c("unterminated '['"));
        return true;
    }

    CONSUME(S);

    return false;
}

static AstNode *typename(ParserState *S) {
    bool const_next = false;

    if (CURRENT(S).type == GRAMINA_TOK_KW_CONST) {
        const_next = true;
        CONSUME(S);
    }

    if (CURRENT(S).type == GRAMINA_TOK_DOLLAR) {
        AstNode *this = mk_ast_node(NULL);
        this->type = GRAMINA_AST_REFLECT;
        this->pos = CURRENT(S).pos;
        if (const_next) {
            this->flags |= GRAMINA_AST_CONST_TYPE;
        }

        CONSUME(S);
        return this;
    }

    AstNode *cur = identifier(S);
    if (!cur) {
        SET_ERR(S, mk_str_c("expected identifier or '$'"));
        return NULL;
    }

    if (const_next) {
        cur->flags |= GRAMINA_AST_CONST_TYPE;
    }

    const_next = false;

    bool loop = true;
    while (loop) {
        switch (CURRENT(S).type) {
        case GRAMINA_TOK_KW_CONST:
            const_next = true;
            CONSUME(S);
            break;
        case GRAMINA_TOK_AMPERSAND:
        case GRAMINA_TOK_AND:
            for (size_t i = 0; i < (CURRENT(S).type == GRAMINA_TOK_AND ? 2 : 1); ++i) {
                cur = mk_ast_node_lr(NULL, cur, NULL);
                cur->type = GRAMINA_AST_TYPE_POINTER;
                cur->pos = CURRENT(S).pos;

                if (const_next) {
                    cur->flags |= GRAMINA_AST_CONST_TYPE;
                    const_next = false;
                }
            }

            CONSUME(S);
            break;
        case GRAMINA_TOK_SUBSCRIPT_LEFT:
            CONSUME(S);
            switch (CURRENT(S).type) {
            case GRAMINA_TOK_SUBSCRIPT_RIGHT:
                cur = mk_ast_node_lr(NULL, cur, NULL);
                cur->type = GRAMINA_AST_TYPE_SLICE;
                cur->pos = N_AFTER(S, -1).pos;

                if (const_next) {
                    cur->flags |= GRAMINA_AST_CONST_TYPE;
                    const_next = false;
                }

                CONSUME(S);
                break;
            case GRAMINA_TOK_LIT_I32:
            case GRAMINA_TOK_LIT_U32:
            case GRAMINA_TOK_LIT_I64:
            case GRAMINA_TOK_LIT_U64:
                if (handle_array_type(S, &cur, &const_next)) {
                    ast_node_free(cur);
                    return NULL;
                }

                break;
            default:
                SET_ERR(S, mk_str_c("expected ']' or integer literal"));

                ast_node_free(cur);
                return NULL;
            }

            break;
        default:
            loop = false;
            break;
        }
    }

    return cur;
}

static AstNode *value(ParserState *S) {
    switch (CURRENT(S).type) {
    case GRAMINA_TOK_LIT_F32:
    case GRAMINA_TOK_LIT_F64:
    case GRAMINA_TOK_LIT_I32:
    case GRAMINA_TOK_LIT_U32:
    case GRAMINA_TOK_LIT_I64:
    case GRAMINA_TOK_LIT_U64:
        return lit_number(S);
    case GRAMINA_TOK_KW_TRUE:
    case GRAMINA_TOK_KW_FALSE:
        return lit_bool(S);
    case GRAMINA_TOK_LIT_STR_SINGLE:
    case GRAMINA_TOK_LIT_STR_DOUBLE:
        return lit_quoted(S);
    case GRAMINA_TOK_IDENTIFIER:
        return identifier(S);
    default:
        SET_ERR(S, mk_str_c("expected number literal, string literal or identifier"));
        return NULL;
    }
}

static AstNode *cast(ParserState *S) {
    TokenPosition pos = CURRENT(S).pos;
    if (CURRENT(S).type != GRAMINA_TOK_BACKSLASH) {
        SET_ERR(S, mk_str_c("expected '\\'"));
        return NULL;
    }

    CONSUME(S);

    AstNode *into = typename(S);
    if (HAS_ERR(S)) {
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_PAREN_LEFT) {
        SET_ERR(S, mk_str_c("expected '('"));

        ast_node_free(into);
        return NULL;
    }

    CONSUME(S);

    AstNode *expr = expression(S);
    if (!expr) {
        ast_node_free(into);
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_PAREN_RIGHT) {
        SET_ERR(S, mk_str_c("expected ')'"));

        ast_node_free(into);
        return NULL;
    }

    CONSUME(S);

    AstNode *this = mk_ast_node_lr(NULL, into, expr);
    this->type = GRAMINA_AST_OP_CAST;
    this->pos = pos;

    return this;
}

static AstNode *factor(ParserState *S) {
    AstNode *value_node = value(S);
    if (value_node) {
        return value_node;
    }

    CLEAR_ERR(S);

    switch (CURRENT(S).type) {
    case GRAMINA_TOK_BACKSLASH:
        return cast(S);
    case GRAMINA_TOK_PAREN_LEFT:
        CONSUME(S);
        break;
    default:
        SET_ERR(S, mk_str_c("expected expression"));
        return NULL;
    }

    AstNode *expr_node = expression(S);
    if (expr_node == NULL) {
        SET_ERR(S, mk_str_c("expected expression"));

        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_PAREN_RIGHT) {
        SET_ERR(S, mk_str_c("expected ')'"));

        ast_node_free(expr_node);
        return NULL;
    }

    CONSUME(S);

    return expr_node;
}

static AstNode *expression_list_pr(ParserState *S) {
    if (CURRENT(S).type != GRAMINA_TOK_COMMA) {
        return NULL;
    }

    CONSUME(S);

    AstNode *expr = expression(S);
    if (!expr) {
        SET_ERR(S, mk_str_c("expected expression"));

        return NULL;
    }

    AstNode *after = expression_list_pr(S);
    if (!after) {
        if (HAS_ERR(S)) {
            ast_node_free(expr);
            return NULL;
        }

        return expr;
    }

    AstNode *this = mk_ast_node_lr(NULL, expr, after);
    this->type = GRAMINA_AST_EXPRESSION_LIST;
    this->pos = expr->pos;

    return this;
}

static AstNode *expression_list(ParserState *S) {
    AstNode *expr = expression(S);
    if (!expr) {
        return NULL;
    }

    if (CURRENT(S).type != GRAMINA_TOK_COMMA) {
        return expr;
    }

    AstNode *list = expression_list_pr(S);
    if (!list && HAS_ERR(S)) {
        ast_node_free(expr);

        return NULL;
    }

    AstNode *this = mk_ast_node_lr(NULL, expr, list);
    this->type = GRAMINA_AST_EXPRESSION_LIST;
    this->pos = expr->pos;

    return this;
}

static AstNode *access_exp_pr(ParserState *S) {
    AstNodeType typ;

    switch (CURRENT(S).type) {
    case GRAMINA_TOK_STATIC_MEMBER:
        typ = GRAMINA_AST_OP_STATIC_MEMBER;
        break;
    case GRAMINA_TOK_COLON:
        typ = GRAMINA_AST_OP_PROPERTY;
        break;
    case GRAMINA_TOK_DOT:
        typ = GRAMINA_AST_OP_MEMBER;
        break;
    case GRAMINA_TOK_PAREN_LEFT:
        typ = GRAMINA_AST_OP_CALL;
        break;
    case GRAMINA_TOK_SUBSCRIPT_LEFT:
        typ = GRAMINA_AST_OP_SUBSCRIPT;
        break;
    default:
        return NULL;
    }

    TokenPosition pos = CURRENT(S).pos;

    CONSUME(S);

    AstNode *right = NULL;

    switch (typ) {
    case GRAMINA_AST_OP_STATIC_MEMBER:
    case GRAMINA_AST_OP_PROPERTY:
    case GRAMINA_AST_OP_MEMBER: {
        AstNode *ident = identifier(S);
        if (!ident) {
            return NULL;
        }

        right = ident;
        break;
    }
    case GRAMINA_AST_OP_CALL: {
        if (CURRENT(S).type == GRAMINA_TOK_PAREN_RIGHT) {
            CONSUME(S);

            right = NULL;
            break;
        }

        AstNode *params = expression_list(S);
        if (!params) {
            return NULL;
        }

        if (CURRENT(S).type != GRAMINA_TOK_PAREN_RIGHT) {
            ast_node_free(params);

            SET_ERR(S, mk_str_c("expected ')'"));

            return NULL;
        }

        CONSUME(S);

        right = params;
        break;
    }
    case GRAMINA_AST_OP_SUBSCRIPT: {
        AstNode *params = expression_list(S);
        if (!params) {
            return NULL;
        }

        if (CURRENT(S).type != GRAMINA_TOK_SUBSCRIPT_RIGHT) {
            SET_ERR(S, mk_str_c("expected ']'"));

            ast_node_free(params);
            return NULL;
        }

        CONSUME(S);

        right = params;
        break;
    }
    default:
        return NULL;
    }

    AstNode *parent = access_exp_pr(S);
    AstNode *this = mk_ast_node_lr(parent, NULL, right);
    this->type = typ;
    this->pos = pos;

    return this;
}

static AstNode *access_exp(ParserState *S) {
    AstNode *from = factor(S);
    if (!from) {
        return NULL;
    }

    AstNode *lowest = access_exp_pr(S);
    if (!lowest) {
        if (HAS_ERR(S)) {
            ast_node_free(from);
            return NULL;
        }

        return from;
    }

    ast_node_child_l(lowest, from);

    AstNode *top = lowest;
    while (top->parent) {
        top = top->parent;
    }

    return top;
}

static AstNode *evaluative_exp_pr(ParserState *S, AstNode *higher_tree, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_CARET:
        *typ = GRAMINA_AST_OP_EVAL;
        break;
    case GRAMINA_TOK_QUESTION:
        *typ = GRAMINA_AST_OP_RETHROW;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    AstNodeType subtyp;
    AstNode *subtree = evaluative_exp_pr(S, higher_tree, &subtyp);
    if (!subtree) {
        if (HAS_ERR(S)) {
            return NULL;
        }

        return higher_tree;
    }

    AstNode *this = mk_ast_node_lr(NULL, subtree, NULL);
    this->type = subtyp;
    this->pos = cur.pos;

    return this;
}

static AstNode *evaluative_exp(ParserState *S) {
    AstNode *higher_tree = access_exp(S);
    if (!higher_tree) {
        return NULL;
    }

    AstNodeType typ;
    AstNode *subtree = evaluative_exp_pr(S, higher_tree, &typ);
    if (!subtree) {
        return higher_tree;
    }

    AstNode *this = mk_ast_node_lr(NULL, subtree, NULL);
    this->type = typ;

    return this;
}

static AstNode *unary_exp(ParserState *S) {
    AstNodeType typ = GRAMINA_AST_INVALID;

    switch (CURRENT(S).type) {
    case GRAMINA_TOK_PLUS:
        typ = GRAMINA_AST_OP_UNARY_PLUS;
        break;
    case GRAMINA_TOK_MINUS:
        typ = GRAMINA_AST_OP_UNARY_MINUS;
        break;
    case GRAMINA_TOK_EXCLAMATION:
        typ = GRAMINA_AST_OP_LOGICAL_NOT;
        break;
    case GRAMINA_TOK_AT:
        typ = GRAMINA_AST_OP_DEREF;
        break;
    case GRAMINA_TOK_AMPERSAND:
        typ = GRAMINA_AST_OP_ADDRESS_OF;
        break;
    case GRAMINA_TOK_ALTERNATE_NOT:
        typ = GRAMINA_AST_OP_ALTERNATE_NOT;
        break;
    default:
        break;
    }

    if (typ == GRAMINA_AST_INVALID) {
        return evaluative_exp(S);
    }

    TokenPosition pos = CURRENT(S).pos;
    CONSUME(S);

    AstNode *next_op = unary_exp(S);
    AstNode *this = mk_ast_node_lr(NULL, next_op, NULL);
    this->type = typ;
    this->pos = pos;

    return this;
}

static AstNode *concat_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_TILDE:
        *typ = GRAMINA_AST_OP_CONCAT;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(concat_exp_pr, unary_exp);
}

static AstNode *concat_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(concat_exp, unary_exp);
}

static AstNode *multiplicative_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_ASTERISK:
        *typ = GRAMINA_AST_OP_MUL;
        break;
    case GRAMINA_TOK_FORWARDSLASH:
        *typ = GRAMINA_AST_OP_DIV;
        break;
    case GRAMINA_TOK_PERCENT:
        *typ = GRAMINA_AST_OP_REM;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(multiplicative_exp_pr, concat_exp);
}

static AstNode *multiplicative_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(multiplicative_exp, concat_exp);
}

static AstNode *additive_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_PLUS:
        *typ = GRAMINA_AST_OP_ADD;
        break;
    case GRAMINA_TOK_MINUS:
        *typ = GRAMINA_AST_OP_SUB;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(additive_exp_pr, multiplicative_exp);
}

static AstNode *additive_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(additive_exp, multiplicative_exp);
}

static AstNode *shift_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_LSHIFT:
        *typ = GRAMINA_AST_OP_LSHIFT;
        break;
    case GRAMINA_TOK_RSHIFT:
        *typ = GRAMINA_AST_OP_RSHIFT;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(shift_exp_pr, additive_exp);
}

static AstNode *shift_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(shift_exp, additive_exp);
}

static AstNode *stream_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_INSERT:
        *typ = GRAMINA_AST_OP_INSERT;
        break;
    case GRAMINA_TOK_EXTRACT:
        *typ = GRAMINA_AST_OP_EXTRACT;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(stream_exp_pr, shift_exp);
}

static AstNode *stream_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(stream_exp, shift_exp);
}

static AstNode *comparison_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_LESS_THAN:
        *typ = GRAMINA_AST_OP_LT;
        break;
    case GRAMINA_TOK_LESS_THAN_EQ:
        *typ = GRAMINA_AST_OP_LTE;
        break;
    case GRAMINA_TOK_GREATER_THAN:
        *typ = GRAMINA_AST_OP_GT;
        break;
    case GRAMINA_TOK_GREATER_THAN_EQ:
        *typ = GRAMINA_AST_OP_GTE;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(comparison_exp_pr, stream_exp);
}

static AstNode *comparison_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(comparison_exp, stream_exp);
}

static AstNode *equality_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_EQUALITY:
        *typ = GRAMINA_AST_OP_EQUAL;
        break;
    case GRAMINA_TOK_INEQUALITY:
        *typ = GRAMINA_AST_OP_INEQUAL;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(equality_exp_pr, comparison_exp);
}

static AstNode *equality_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(equality_exp, comparison_exp);
}

static AstNode *alternate_and_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_ALTERNATE_AND:
        *typ = GRAMINA_AST_OP_ALTERNATE_AND;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(alternate_and_exp_pr, equality_exp);
}

static AstNode *alternate_and_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(alternate_and_exp, equality_exp);
}

static AstNode *alternate_xor_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_ALTERNATE_XOR:
        *typ = GRAMINA_AST_OP_ALTERNATE_XOR;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(alternate_xor_exp_pr, alternate_and_exp);
}

static AstNode *alternate_xor_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(alternate_xor_exp, alternate_and_exp);
}

static AstNode *alternate_or_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_ALTERNATE_OR:
        *typ = GRAMINA_AST_OP_ALTERNATE_OR;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(alternate_or_exp_pr, alternate_xor_exp);
}

static AstNode *alternate_or_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(alternate_or_exp, alternate_xor_exp);
}

static AstNode *logical_and_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_AND:
        *typ = GRAMINA_AST_OP_LOGICAL_AND;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(logical_and_exp_pr, alternate_or_exp);
}

static AstNode *logical_and_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(logical_and_exp, alternate_or_exp);
}

static AstNode *logical_xor_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_XOR:
        *typ = GRAMINA_AST_OP_LOGICAL_XOR;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(logical_xor_exp_pr, logical_and_exp);
}

static AstNode *logical_xor_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(logical_xor_exp, logical_and_exp);
}

static AstNode *logical_or_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_OR:
        *typ = GRAMINA_AST_OP_LOGICAL_OR;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(logical_or_exp_pr, logical_xor_exp);
}

static AstNode *logical_or_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(logical_or_exp, logical_xor_exp);
}

static AstNode *fallback_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_FALLBACK:
        *typ = GRAMINA_AST_OP_FALLBACK;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(fallback_exp_pr, logical_or_exp);
}

static AstNode *fallback_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(fallback_exp, logical_or_exp);
}

static AstNode *assignment_exp_pr(ParserState *S, AstNodeType *typ) {
    Token cur = CURRENT(S);
    switch (cur.type) {
    case GRAMINA_TOK_ASSIGN:
        *typ = GRAMINA_AST_OP_ASSIGN;
        break;
    case GRAMINA_TOK_ASSIGN_ADD:
        *typ = GRAMINA_AST_OP_ASSIGN_ADD;
        break;
    case GRAMINA_TOK_ASSIGN_SUB:
        *typ = GRAMINA_AST_OP_ASSIGN_SUB;
        break;
    case GRAMINA_TOK_ASSIGN_MUL:
        *typ = GRAMINA_AST_OP_ASSIGN_MUL;
        break;
    case GRAMINA_TOK_ASSIGN_DIV:
        *typ = GRAMINA_AST_OP_ASSIGN_DIV;
        break;
    case GRAMINA_TOK_ASSIGN_REM:
        *typ = GRAMINA_AST_OP_ASSIGN_REM;
        break;
    case GRAMINA_TOK_ASSIGN_CAT:
        *typ = GRAMINA_AST_OP_ASSIGN_CAT;
        break;
    default:
        return NULL;
    }

    CONSUME(S);

    CHAIN_SELF(assignment_exp_pr, fallback_exp);
}

static AstNode *assignment_exp(ParserState *S) {
    CHAINABLE_EXPRESSION_RULE(assignment_exp, fallback_exp);
}
