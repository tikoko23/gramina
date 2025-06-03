["\\"] @punctuation

(comment) @comment

(number) @number
(bool) @boolean
(char) @character
(string) @string

[
  (char_escape_seq)
  (string_escape_seq)
] @string.escape

[
  (evaluative_op)
  (unary_op)
  (concat_op)
  (multiplicative_op)
  (additive_op)
  (shift_op)
  (stream_op)
  (comparison_op)
  (equality_op)
  (logical_or_op)
  (logical_xor_op)
  (logical_and_op)
  (alternate_or_op)
  (alternate_xor_op)
  (alternate_and_op)
  (fallback_op)
  (assignment_op)
  "&"
  "="
  "->"
] @operator

(access_exp
  (factor) @function
  (call_exp_list))

(access_exp
  ["." ":" "::"]
  (identifier) @variable.member)

((identifier) @variable.builtin
  (#any-of? @variable.builtin "self"
                              "this"))

[
  "for"
  "while"
] @keyword.repeat

[
  "if"
  "else"
] @keyword.conditional

"return" @keyword.return

"const" @keyword.type
"struct" @keyword.type
"fn" @keyword.function

(typename) @type
(param) @variable.parameter
(member) @variable.member

(attribs) @attribute.builtin

(struct_def
  (identifier) @type)

(function_def
  (identifier) @function)

["$"] @punctuation.special
["(" ")" "{" "}" "[" "]"] @punctuation.bracket
[";" "," "." ":" "::" ".."] @punctuation.delimiter
