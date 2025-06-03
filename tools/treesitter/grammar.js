function keyword(s) {
  return token(prec(10, s));
}

function higher(self, hi, ops) {
  return prec.left(1, seq(
    hi,
    repeat(seq(
      ops,
      self
    ))
  ));
}

// reserved.
function op(_, r) {
  return r;
}

function escape(quote_char) {
  return seq(
    "\\",
    choice(
      "\\",
      seq("x", /[0-9abcdefABCDEF]{2}/),
      /[01234567]{3}/,
      /0|n|t|f|v|e/,
      quote_char
    ),
  );
}

module.exports = grammar({
  name: "gramina",

  extras: $ => [
    $.comment,
    /\s/
  ],

  inline: $ => [
    $.comment,
  ],

  rules: {
    source_file: $ => repeat($.global_statement),

    comment: _ => choice(
      /\/\/[^\n]*/,
      seq(
        "/*",
        repeat(/.|\n/),
        "*/"
      )
    ),

    global_statement: $ => choice(
      $.function_def,
      $.struct_def,
    ),

    param_list: $ => prec.left(1, seq(
      $.typename,
      $.param,
      repeat(seq(
        ",",
        $.param_list
      ))
    )),

    declaration: $ => seq(
      $.typename,
      $.identifier,
      optional(seq("=", $.expression)),
    ),

    statement_block: $ => $.block_statement,

    block_statement: $ => seq(
      "{", repeat($.statement), "}"
    ),

    if_statement: $ => prec.left(1, seq(
      keyword("if"),
      $.expression,
      $.statement_block,
      optional(seq(keyword("else"), choice(
        $.if_statement,
        $.statement_block
      )))
    )),

    while_statement: $ => seq(
      keyword("while"),
      $.expression,
      $.statement_block
    ),

    for_statement: $ => seq(
      keyword("for"),
      $.declaration, ";",
      $.expression, ";",
      $.expression,
      $.statement_block
    ),

    return_statement: $ => seq(
      keyword("return"),
      $.expression,
      ";"
    ),

    statement: $ => choice(
      $.block_statement,
      $.if_statement,
      $.for_statement,
      $.while_statement,
      $.return_statement,
      seq(choice(
        $.declaration,
        $.expression
      ), ";")
    ),

    attribs: $ => repeat1(
      seq("#", $.identifier, optional(seq(
        "(", $.string, ")"
      )))
    ),

    struct_field: $ => seq($.typename, $.member, ";"),
    struct_def: $ => seq(
      optional($.attribs),
      keyword("struct"),
      $.identifier,
      "{",
      repeat($.struct_field),
      "}"
    ),

    function_def: $ => seq(
      optional($.attribs),
      keyword("fn"),
      $.identifier,
      "(",
      optional($.param_list),
      ")",
      optional(seq(
        "->",
        $.typename
      )),
      choice(
        ";",
        seq(
          "{",
          repeat($.statement),
          "}"
        )
      )
    ),

    typename: $ => seq(
      optional(keyword("const")),
      choice(
        $.identifier,
        "$"
      ),
      repeat(seq(
        optional(keyword("const")),
        choice(
          "&",
          "[]",
          seq("[", /[0-9]+/, "]")
        )
      )),
    ),

    param: $ => $.identifier,
    member: $ => $.identifier,
    identifier: _ => token(
      prec(-10, /[A-Za-z_][A-Za-z0-9_]*/),
    ),

    number: _ => seq(
      /[0-9]+/,
      choice(
        /lu|ul|u|l/i,
        seq(
          optional(/\.[0-9]+/),
          optional(/e-|e/i),
          optional(/lf|fl|l|f/i)
        )
      ),
    ),

    char_escape_seq: _ => escape("'"),
    string_escape_seq: _ => escape("\""),

    char: $ => seq(
      "'",
      repeat1(choice(
        $.char_escape_seq,
        /[^\\']/
      )),
      "'"
    ),

    string: $ => seq(
      "\"",
      repeat(choice(
        $.string_escape_seq,
        /[^\\"]/
      )),
      "\""
    ),

    bool: _ => choice(
      keyword("true"),
      keyword("false")
    ),

    value: $ => choice(
      $.identifier,
      $.number,
      $.bool,
      $.char,
      $.string,
    ),

    expression: $ => $.assignment_exp,
    expression_list: $ => seq(
      $.expression,
      repeat(seq(",", $.expression))
    ),

    factor: $ => choice(
      $.value,
      seq("\\", $.typename, "(", $.expression, ")"),
      seq("(", $.expression, ")")
    ),

    subscript_exp_list: $ => seq("[", $.expression_list, "]"),
    call_exp_list: $ => seq("(", $.expression_list, ")"),

    access_exp: $ => prec.left(1, seq(
      $.factor,
      repeat(choice(
        seq(".", $.identifier),
        seq(":", $.identifier),
        seq("::", $.identifier),
        $.call_exp_list,
        $.subscript_exp_list,
        seq("[", optional($.expression), "..", optional($.expression), "]"),
      ))
    )),

    evaluative_op: $ => op($, choice("?", "^")),
    evaluative_exp: $ => prec.left(1, seq(
      $.access_exp,
      repeat($.evaluative_op)
    )),

    unary_op: $ => op($, choice("+", "-", "!", "@", "&", "!`")),
    unary_exp: $ => prec.right(1, seq(
      repeat($.unary_op),
      $.evaluative_exp
    )),

    concat_op: $ => op($, "~"),
    concat_exp: $ => higher($.concat_exp, $.unary_exp, $.concat_op),

    multiplicative_op: $ => op($, choice("*", "/", "%")),
    multiplicative_exp: $ => higher($.multiplicative_exp, $.concat_exp, $.multiplicative_op),

    additive_op: $ => op($, choice("+", "-")),
    additive_exp: $ => higher($.additive_exp, $.multiplicative_exp, $.additive_op),

    shift_op: $ => op($, choice("<<", ">>")),
    shift_exp: $ => higher($.shift_exp, $.additive_exp, $.shift_op),

    stream_op: $ => op($, choice("<~", "~>")),
    stream_exp: $ => higher($.stream_exp, $.shift_exp, $.stream_op),

    comparison_op: $ => op($, choice("<", "<=", ">", ">=")),
    comparison_exp: $ => higher($.comparison_exp, $.stream_exp, $.comparison_op),

    equality_op: $ => op($, choice("==", "!=")),
    equality_exp: $ => higher($.equality_exp, $.comparison_exp, $.equality_op),

    alternate_and_op: $ => op($, "&&`"),
    alternate_and_exp: $ => higher($.alternate_and_exp, $.equality_exp, $.alternate_and_op),

    alternate_xor_op: $ => op($, "\\\\`"),
    alternate_xor_exp: $ => higher($.alternate_xor_exp, $.alternate_and_exp, $.alternate_xor_op),

    alternate_or_op: $ => op($, "||`"),
    alternate_or_exp: $ => higher($.alternate_or_exp, $.alternate_xor_exp, $.alternate_or_op),

    logical_and_op: $ => op($, "&&"),
    logical_and_exp: $ => higher($.logical_and_exp, $.alternate_or_exp, $.logical_and_op),

    logical_xor_op: $ => op($, "\\\\"),
    logical_xor_exp: $ => higher($.logical_xor_exp, $.logical_and_exp, $.logical_xor_op),

    logical_or_op: $ => op($, "||"),
    logical_or_exp: $ => higher($.logical_or_exp, $.logical_xor_exp, $.logical_or_op),

    fallback_op: $ => op($, "??"),
    fallback_exp: $ => higher($.fallback_exp, $.logical_or_exp, $.fallback_op),

    assignment_op: $ => op($, choice("=", "+=", "-=", "*=", "/=", "%=", "~=")),
    assignment_exp: $ => higher($.assignment_exp, $.fallback_exp, $.assignment_op),
  },
  conflicts: $ => [
    [ $.value, $.typename ]
  ]
});

