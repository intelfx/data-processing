`calculator`
==========

SYNOPSIS
--------

    calculator [-S|--simplify[=VARIABLE]] [options...] EXPRESSION
    calculator [-D|--differentiate VARIABLE] [options...] EXPRESSION
    calculator [-E|--error] [options...] EXPRESSION
    calculator [-T|--taylor-series VARIABLE] [options...] EXPRESSION

DESCRIPTION
-----------

`calculator` processes an algebraic expression in various ways. Infinite
precision rational arithmetic is used in calculations, although variables can
be floating-point (this is specified when defining a variable).

The mode of operation is specified by adding one of options `--simplify`,
`--differentiate`, `--error` and `--taylor-series` on the command line.

*   The `--simplify` option allows to specify a variable to simplify for (in
    this mode, all other variables will be substituted). If a variable is not
    specified, no variable is substituted while simplifying.

    *Note that all other modes imply simplification of the input expression.*

*   The `--differentiate` option allows to take N-th derivative of the
    expression. This is done by repeated differentiation.

*   The `--error` option calculates an estimate of error for the expression
    with multiple variables, given errors of participating **independent**
    variables. This is done by taking partial derivatives of the expression
    for each variable.

*   The `--taylor-series` option calculates the Taylor series for the
    expression in given point up to N-th term. *(Only 0 is supported for now.)*

EXPRESSION SYNTAX
-----------------

The expression syntax understood by the parser resembles C expressions, but
with some additions. Associativity of all operations is left, but they can
be reordered assuming infinite precision.

Integers are specified in a format that C++ iostream understands. That is,
sequences of digit characters. Variables can have any names, but to use them
in an expression, they need to be parseble (i. e., follow C identifier rules).

Table: *Supported syntax, in order of ascending priority:*

Operator           Description
------------------ ------------------------------------------------
`+`, `-`, `*`, `/` Usual arithmetics (unary variants are supported)
`^`, `**`          Exponent (`base ** exponent`)
`foo`              Variables
`123`              Numbers
`(` and `)`        Parentheses
------------------ ------------------------------------------------

Table: *Elision of multiplication sign is supported between:*

Description                         Example
----------------------------------- ----------------------------
A number and a variable             `2x` into `2 * x`
A number and an opening parenthesis `2(a - 1)` into `2 * (a - 1)`

GENERAL OPTIONS
---------------

A number of options influence the program's behavior and output. Note that
the human-readable output is printed to stderr, while machine-readable output
is printed to stdout.

Table: Options influencing output

-------------------------------------------------------------------------------
Option                      Description
--------------------------- ---------------------------------------------------
`-m`, `--machine`           Enable machine-readable output. This depends on
                            mode of operation.

`-l FILE`, `--latex FILE`   Enable PDF output using `pdflatex`. The `FILE` will
                            be used as a temporary; its name will be passed to
                            `pdflatex`.

`-q`, `--terse`             Enable terse (quiet) output mode. This reduces the
                            amount of noise written to the terminal alongside
                            the actual results. E. g. input data and variable
                            list is not printed, and both expression and its
                            value are output in a single line instead of three
                            lines (original, simplified, substituted) plus a
                            fourth one for the value.

`-Q`, `--really-quiet`      Enables literally quiet output mode. Nothing is
                            written to stderr. Machine-readable output is
                            printed to stdout if enabled.
-------------------------------------------------------------------------------

Another batch of options is used to specify the expression's "name" which will
be used as the left-hand side of the printed equations.

Table: Options specifying expression name

-------------------------------------------------------------------------------
Option                      Description
--------------------------- ---------------------------------------------------
`-n NAME`, `--name NAME`    Use `NAME` by default in stderr output,
                            machine-readable output and LaTeX output. \
                            *The default for this option is `F`.*

`--name-machine NAME`       Use `NAME` in machine-readable output.

`--name-latex NAME`         Use `NAME` in LaTeX output. May include native code
                            such as `\\frac { \\partial F } { \\partial x }`.
-------------------------------------------------------------------------------

Finally, there are options defining new variables. Some universally-recognized
constants are already defined; for their list take a look in code.

The variable format is `NAME VALUE ERROR` or just `NAME` for "bare" variables
(without defined value).

The floating-point values are specified in a format that C++ iostream
understands. The fractions are specified in a format `P/Q`, where `P` and `Q`
are integers with the same format. There shall be no spaces between numerator
and denominator. Elision of denominator is currently not possible due to bug
in STL and/or boost (the author believes it to be so).

The error (measurement error) currently must be specified even if not used by
the selected mode of operation. This will be improved later.

Note that calculation is performed in rational arithmetic while possible.
If a floating-point value is encountered while calculating, the output becomes
floating-point. If a bare variable is encountered while calculating, there is
no output.

Table: Options defining variables

-------------------------------------------------------------------------------
Option                       Description
---------------------------- --------------------------------------------------
`-v VAR`, `--var VAR`        Define a new floating-point variable.

`-r VAR`, `--var-frac VAR`   Define a new rational variable.

`-b NAME`, `--var-bare NAME` Define a new "bare" variable (without any value).
-------------------------------------------------------------------------------

SIMPLIFICATION OPTIONS (APPLY TO `-S`)
--------------------------------------

The simplification variable may be specified. This will cause all other
variables to be substituted.

*(By default, no variable is substituted during simplification.)*

Table: Simplification options

-------------------------------------------------------------------------------
Option                      Description
--------------------------- ---------------------------------------------------
`--simplify`                Enable general simplification mode.

`-SNAME`, `--simplify=NAME` Enable simplification for a specific variable.
-------------------------------------------------------------------------------

DIFFERENTIATION OPTIONS (APPLY TO `-D`)
---------------------------------------

The order of derivative may be specified, as well as the differentiation
variable (the latter is actually *required*).

Table: Differentiation options

-------------------------------------------------------------------------------
Option                            Description
--------------------------------- ---------------------------------------------
`-D NAME`, `--differentiate NAME` Enable differentiation mode and set
                                  differentiation variable to `NAME`.
                                  Inexistent variables are allowed; the
                                  differential will become 0 in this case.

`-o ORDER`, `--deriv-order ORDER` Set differentiation order (count of
                                  iterations) to `ORDER`.
-------------------------------------------------------------------------------

The differentiation process (with high order values) may produce very large
expressions and currently takes very much time/memory to compute. For example,
the nineth derivative of the expression `1 / (1 + x + x^2)` is computed for 17
seconds on a modern i7-4700M Haswell CPU.

TAYLOR SERIES OPTIONS (APPLY TO `-T`)
-------------------------------------

The point to compute the series for may be specified (as a rational value),
as well as the length of the series (maximal degree of the variable) and
the variable itself (again, the latter is *required*).

Table: Taylor series options

-------------------------------------------------------------------------------
Option                                Description
------------------------------------- -----------------------------------------
`-T NAME`, `--taylor-series NAME`     Enable Taylor series calculation mode and
                                      set decomposition variable to `NAME`.
                                      Inexistent variables will cause a
                                      computation error.

`-s LENGTH`, `--series-length LENGTH` Set Taylor series length (maximal degree
                                      of a term) to `LENGTH`.
-------------------------------------------------------------------------------

The Taylor series is computed "by definition", i. e. by taking first `LENGTH`
derivatives of the input expression. Therefore, the performance of computing
Taylor series is also poor.

BUGS
----

It is **slow**.
