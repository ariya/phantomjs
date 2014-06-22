----------------------------------------------------------------------------
--
-- Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
-- Contact: http://www.qt-project.org/legal
--
-- This file is part of the QtCore module of the Qt Toolkit.
--
-- $QT_BEGIN_LICENSE:LGPL$
-- Commercial License Usage
-- Licensees holding valid commercial Qt licenses may use this file in
-- accordance with the commercial license agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and Digia.  For licensing terms and
-- conditions see http://qt.digia.com/licensing.  For further information
-- use the contact form at http://qt.digia.com/contact-us.
--
-- GNU Lesser General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- In addition, as a special exception, Digia gives you certain additional
-- rights.  These rights are described in the Digia Qt LGPL Exception
-- version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
--
-- GNU General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU
-- General Public License version 3.0 as published by the Free Software
-- Foundation and appearing in the file LICENSE.GPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU General Public License version 3.0 requirements will be
-- met: http://www.gnu.org/copyleft/gpl.html.
--
--
-- $QT_END_LICENSE$
--
----------------------------------------------------------------------------

%parser GLSLParserTable
%merged_output glsl.cpp

%token ADD_ASSIGN
%token AMPERSAND
%token AND_ASSIGN
%token AND_OP
%token ATTRIBUTE
%token BANG
%token BOOL
%token BOOLCONSTANT
%token BREAK
%token BVEC2
%token BVEC3
%token BVEC4
%token CARET
%token COLON
%token COMMA
%token CONST
%token CONTINUE
%token DASH
%token DEC_OP
%token DISCARD
%token DIV_ASSIGN
%token DO
%token DOT
%token ELSE
%token EQUAL
%token EQ_OP
%token FLOAT
%token FLOATCONSTANT
%token FOR
%token GE_OP
%token IDENTIFIER
%token IF
%token IN
%token INC_OP
%token INOUT
%token INT
%token INTCONSTANT
%token IVEC2
%token IVEC3
%token IVEC4
%token LEFT_ANGLE
%token LEFT_ASSIGN
%token LEFT_BRACE
%token LEFT_BRACKET
%token LEFT_OP
%token LEFT_PAREN
%token LE_OP
%token MAT2
%token MAT3
%token MAT4
%token MOD_ASSIGN
%token MUL_ASSIGN
%token NE_OP
%token OR_ASSIGN
%token OR_OP
%token OUT
%token PERCENT
%token PLUS
%token QUESTION
%token RETURN
%token RIGHT_ANGLE
%token RIGHT_ASSIGN
%token RIGHT_BRACE
%token RIGHT_BRACKET
%token RIGHT_OP
%token RIGHT_PAREN
%token SAMPLER1D
%token SAMPLER1DSHADOW
%token SAMPLER2D
%token SAMPLER2DSHADOW
%token SAMPLER3D
%token SAMPLERCUBE
%token SEMICOLON
%token SLASH
%token STAR
%token STRUCT
%token SUB_ASSIGN
%token TILDE
%token TYPE_NAME
%token UNIFORM
%token VARYING
%token VEC2
%token VEC3
%token VEC4
%token VERTICAL_BAR
%token VOID
%token WHILE
%token XOR_ASSIGN
%token XOR_OP
%token ERROR
%token HIGH_PRECISION
%token MEDIUM_PRECISION
%token LOW_PRECISION
%start translation_unit


/:

#include <QtCore>

class GLSLParser: protected $table
{
public:
    union Value {
      int i;
      unsigned u;
      unsigned long ul;
      unsigned long long ull;
      long l;
      double d;
      float f;
      const QString *s;
      // ### more...
    };

public:
    GLSLParser();
    ~GLSLParser();

    bool parse();

protected:
    inline void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    int nextToken();

    bool isTypename(const QString *s) const
    {
      return types.contains(s);
    }

    inline const QString *intern(const QString &s)
    { return &*string_repository.insert(s); }

protected:
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    Value yylval;
    QSet<QString> string_repository;
    QSet<const QString*> types;

    struct /*Context*/ {
      int line;
      const QString *function_name;
      QString fileName;

      void init()
      {
        line = 1;
        function_name = 0;
        fileName.clear();
      }
    } context;
};

inline void GLSLParser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (realloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (realloc(state_stack, stack_size * sizeof(int)));
}

:/


/.

GLSLParser::GLSLParser():
    tos(0),
    stack_size(0),
    sym_stack(0),
    state_stack(0)
{
}

GLSLParser::~GLSLParser()
{
    if (stack_size) {
        free(sym_stack);
        free(state_stack);
    }
}

bool GLSLParser::parse()
{
  const int INITIAL_STATE = 0;

  int yytoken = -1;

  reallocateStack();

  context.init();
  tos = 0;
  state_stack[++tos] = INITIAL_STATE;

  while (true)
    {
      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state_stack [tos]])
        yytoken = nextToken();

      int act = t_action (state_stack [tos], yytoken);

      if (act == ACCEPT_STATE) {
        return true;
      }

      else if (act > 0)
        {
          if (++tos == stack_size)
            reallocateStack();

          sym_stack [tos] = yylval;
          state_stack [tos] = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          int ridx = rule_index [r];
          printf ("*** reduce using rule %d %s ::=", r + 1, spell[rule_info [ridx]]);
          ++ridx;
          for (int i = ridx; i < ridx + rhs [r]; ++i)
            {
              int symbol = rule_info [i];
              if (const char *name = spell [symbol])
                printf (" %s", name);
              else
                printf (" #%d", symbol);
            }
          printf ("\n");

          tos -= rhs [r];
          act = state_stack [tos++];

          switch (r) {
./


translation_unit ::= external_declaration ;
translation_unit ::= translation_unit external_declaration ;

variable_identifier ::= IDENTIFIER ;

primary_expression ::= variable_identifier ;
primary_expression ::= INTCONSTANT ;
primary_expression ::= FLOATCONSTANT ;
primary_expression ::= BOOLCONSTANT ;
primary_expression ::= LEFT_PAREN expression RIGHT_PAREN ;


postfix_expression ::= primary_expression ;
postfix_expression ::= postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET ;
postfix_expression ::= function_call ;
postfix_expression ::= postfix_expression DOT IDENTIFIER ;
postfix_expression ::= postfix_expression DOT TYPE_NAME ;
postfix_expression ::= postfix_expression INC_OP ;
postfix_expression ::= postfix_expression DEC_OP ;


integer_expression ::= expression ;

function_call ::= function_call_generic ;

function_call_generic ::= function_call_header_with_parameters RIGHT_PAREN ;
function_call_generic ::= function_call_header_no_parameters RIGHT_PAREN ;

function_call_header_no_parameters ::= function_call_header VOID ;
function_call_header_no_parameters ::= function_call_header ;


function_call_header_with_parameters ::= function_call_header assignment_expression ;
function_call_header_with_parameters ::= function_call_header_with_parameters COMMA assignment_expression ;

function_call_header ::=  function_identifier LEFT_PAREN ;

function_identifier ::= constructor_identifier ;
function_identifier ::= IDENTIFIER ;


constructor_identifier ::= FLOAT ;
constructor_identifier ::= INT ;
constructor_identifier ::= BOOL ;
constructor_identifier ::= VEC2 ;
constructor_identifier ::= VEC3 ;
constructor_identifier ::= VEC4 ;
constructor_identifier ::= BVEC2 ;
constructor_identifier ::= BVEC3 ;
constructor_identifier ::= BVEC4 ;
constructor_identifier ::= IVEC2 ;
constructor_identifier ::= IVEC3 ;
constructor_identifier ::= IVEC4 ;
constructor_identifier ::= MAT2 ;
constructor_identifier ::= MAT3 ;
constructor_identifier ::= MAT4 ;
constructor_identifier ::= TYPE_NAME ;

unary_expression ::= postfix_expression ;
unary_expression ::= INC_OP unary_expression ;
unary_expression ::= DEC_OP unary_expression ;
unary_expression ::= unary_operator unary_expression ;

-- Grammar Note:  No traditional style type casts.

unary_operator ::= PLUS ;
unary_operator ::= DASH ;
unary_operator ::= BANG ;
unary_operator ::= TILDE  ;    -- reserved

-- Grammar Note:  No '*' or '&' unary ops. Pointers are not supported.

multiplicative_expression ::= unary_expression ;
multiplicative_expression ::= multiplicative_expression STAR unary_expression ;
multiplicative_expression ::= multiplicative_expression SLASH unary_expression ;
multiplicative_expression ::= multiplicative_expression PERCENT unary_expression ;          -- reserved


additive_expression ::= multiplicative_expression ;
additive_expression ::= additive_expression PLUS multiplicative_expression ;
additive_expression ::= additive_expression DASH multiplicative_expression ;

shift_expression ::= additive_expression ;
shift_expression ::= shift_expression LEFT_OP additive_expression ;  -- reserved
shift_expression ::= shift_expression RIGHT_OP additive_expression ;  -- reserved

relational_expression ::= shift_expression ;
relational_expression ::= relational_expression LEFT_ANGLE shift_expression ;
relational_expression ::= relational_expression RIGHT_ANGLE shift_expression ;
relational_expression ::= relational_expression LE_OP shift_expression ;
relational_expression ::= relational_expression GE_OP shift_expression ;

equality_expression ::= relational_expression ;
equality_expression ::= equality_expression EQ_OP relational_expression ;
equality_expression ::= equality_expression NE_OP relational_expression ;

and_expression ::= equality_expression ;
and_expression ::= and_expression AMPERSAND equality_expression ; -- reserved

exclusive_or_expression ::= and_expression ;
exclusive_or_expression ::= exclusive_or_expression CARET and_expression ; -- reserved

inclusive_or_expression ::= exclusive_or_expression ;
inclusive_or_expression ::= inclusive_or_expression VERTICAL_BAR exclusive_or_expression ; -- reserved

logical_and_expression ::= inclusive_or_expression ;
logical_and_expression ::= logical_and_expression AND_OP inclusive_or_expression ;

logical_xor_expression ::= logical_and_expression ;
logical_xor_expression ::= logical_xor_expression XOR_OP logical_and_expression ;

logical_or_expression ::= logical_xor_expression ;
logical_or_expression ::= logical_or_expression OR_OP logical_xor_expression ;

conditional_expression ::= logical_or_expression ;
conditional_expression ::= logical_or_expression QUESTION expression COLON conditional_expression ;

assignment_expression ::= conditional_expression ;
assignment_expression ::= unary_expression assignment_operator assignment_expression ;

assignment_operator ::= EQUAL ;
assignment_operator ::= MUL_ASSIGN ;
assignment_operator ::= DIV_ASSIGN ;
assignment_operator ::= MOD_ASSIGN ; -- reserved
assignment_operator ::= ADD_ASSIGN ;
assignment_operator ::= SUB_ASSIGN ;
assignment_operator ::= LEFT_ASSIGN ; -- reserved
assignment_operator ::= RIGHT_ASSIGN ; -- reserved
assignment_operator ::= AND_ASSIGN ; -- reserved
assignment_operator ::= XOR_ASSIGN ; -- reserved
assignment_operator ::= OR_ASSIGN ; -- reserved

expression ::= assignment_expression ;
expression ::= expression COMMA assignment_expression ;

constant_expression ::= conditional_expression ;

declaration ::= function_prototype SEMICOLON ;
declaration ::= init_declarator_list SEMICOLON ;

function_prototype ::= function_declarator RIGHT_PAREN ;

function_declarator ::= function_header ;
function_declarator ::= function_header_with_parameters ;

function_header_with_parameters ::= function_header parameter_declaration ;
function_header_with_parameters ::= function_header_with_parameters COMMA parameter_declaration ;

function_header ::= fully_specified_type IDENTIFIER LEFT_PAREN ;
/.
case $rule_number: {
  context.function_name = sym(2).s;
} break;
./

parameter_declarator ::= type_specifier IDENTIFIER ;
parameter_declarator ::= type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;

parameter_declaration ::= type_qualifier parameter_qualifier parameter_declarator ;
parameter_declaration ::= parameter_qualifier parameter_declarator ;
parameter_declaration ::= type_qualifier parameter_qualifier parameter_type_specifier ;
parameter_declaration ::= parameter_qualifier parameter_type_specifier ;

parameter_qualifier ::=  ;
parameter_qualifier ::= IN ;
parameter_qualifier ::= OUT ;
parameter_qualifier ::= INOUT ;

parameter_type_specifier ::= type_specifier ;
parameter_type_specifier ::= type_specifier LEFT_BRACKET constant_expression RIGHT_BRACKET ;

init_declarator_list ::= single_declaration ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression ;
init_declarator_list ::= RIGHT_BRACKET ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER EQUAL initializer ;

single_declaration ::= fully_specified_type ;
single_declaration ::= fully_specified_type IDENTIFIER ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
single_declaration ::= fully_specified_type IDENTIFIER EQUAL initializer ;

-- Grammar Note:  No 'enum', or 'typedef'.

--fully_specified_type ::= type_specifier ;
--fully_specified_type ::= type_qualifier type_specifier ;

fully_specified_type ::= type_specifier ;
fully_specified_type ::= type_qualifier ;
fully_specified_type ::= fully_specified_type type_specifier ;
fully_specified_type ::= fully_specified_type type_qualifier ;

type_qualifier ::= CONST ;
type_qualifier ::= ATTRIBUTE ;           -- Vertex only.
type_qualifier ::= VARYING ;
type_qualifier ::= UNIFORM ;

type_specifier ::= type_specifier_no_prec ;
type_specifier ::= precision_qualifier type_specifier_no_prec ;

type_specifier_no_prec ::= VOID ;
type_specifier_no_prec ::= FLOAT ;
type_specifier_no_prec ::= INT ;
type_specifier_no_prec ::= BOOL ;
type_specifier_no_prec ::= VEC2 ;
type_specifier_no_prec ::= VEC3 ;
type_specifier_no_prec ::= VEC4 ;
type_specifier_no_prec ::= BVEC2 ;
type_specifier_no_prec ::= BVEC3 ;
type_specifier_no_prec ::= BVEC4 ;
type_specifier_no_prec ::= IVEC2 ;
type_specifier_no_prec ::= IVEC3 ;
type_specifier_no_prec ::= IVEC4 ;
type_specifier_no_prec ::= MAT2 ;
type_specifier_no_prec ::= MAT3 ;
type_specifier_no_prec ::= MAT4 ;
type_specifier_no_prec ::= SAMPLER1D ;
type_specifier_no_prec ::= SAMPLER2D ;
type_specifier_no_prec ::= SAMPLER3D ;
type_specifier_no_prec ::= SAMPLERCUBE ;
type_specifier_no_prec ::= SAMPLER1DSHADOW ;
type_specifier_no_prec ::= SAMPLER2DSHADOW ;
type_specifier_no_prec ::= struct_specifier ;
type_specifier_no_prec ::= TYPE_NAME ;

precision_qualifier ::= HIGH_PRECISION ;
precision_qualifier ::= MEDIUM_PRECISION ;
precision_qualifier ::= LOW_PRECISION ;

struct_specifier ::= STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
/.
case $rule_number: {
  types.insert(sym(2).s);
} break;
./

struct_specifier ::= STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE ;

struct_declaration_list ::= struct_declaration ;
struct_declaration_list ::= struct_declaration_list struct_declaration ;

struct_declaration ::= type_specifier struct_declarator_list SEMICOLON ;

struct_declarator_list ::= struct_declarator ;
struct_declarator_list ::= struct_declarator_list COMMA struct_declarator ;

struct_declarator ::= IDENTIFIER ;
struct_declarator ::= IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;

initializer ::= assignment_expression ;

declaration_statement ::= declaration ;

statement ::= compound_statement ;
statement ::= simple_statement ;

-- Grammar Note:  No labeled statements; 'goto' is not supported.

simple_statement ::= declaration_statement ;
simple_statement ::= expression_statement ;
simple_statement ::= selection_statement ;
simple_statement ::= iteration_statement ;
simple_statement ::= jump_statement ;

compound_statement ::= LEFT_BRACE RIGHT_BRACE ;
compound_statement ::= LEFT_BRACE statement_list RIGHT_BRACE ;

statement_no_new_scope ::= compound_statement_no_new_scope ;
statement_no_new_scope ::= simple_statement ;

compound_statement_no_new_scope ::= LEFT_BRACE RIGHT_BRACE ;
compound_statement_no_new_scope ::= LEFT_BRACE statement_list RIGHT_BRACE ;

statement_list ::= statement ;
statement_list ::= statement_list statement ;

expression_statement ::= SEMICOLON ;
expression_statement ::= expression SEMICOLON ;

selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN statement ELSE statement ;
selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN statement ;

-- Grammar Note:  No 'switch'. Switch statements not supported.

condition ::= expression ;
condition ::= fully_specified_type IDENTIFIER EQUAL initializer ;

iteration_statement ::= WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope ;
iteration_statement ::= DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON ;
iteration_statement ::= FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope ;

for_init_statement ::= expression_statement ;
for_init_statement ::= declaration_statement ;

conditionopt ::=  ;
conditionopt ::=  condition ;

for_rest_statement ::= conditionopt SEMICOLON ;
for_rest_statement ::= conditionopt SEMICOLON expression ;

jump_statement ::= CONTINUE SEMICOLON ;
jump_statement ::= BREAK SEMICOLON ;
jump_statement ::= RETURN SEMICOLON ;
jump_statement ::= RETURN expression SEMICOLON ;
jump_statement ::= DISCARD SEMICOLON ;                -- Fragment shader only.

-- Grammar Note:  No 'goto'. Gotos are not supported.

external_declaration ::= function_definition ;
external_declaration ::= declaration ;

function_definition ::= function_prototype compound_statement_no_new_scope ;
/.
    case $rule_number: { // $rule_name
       qDebug() << "--> function" << *context.function_name;
    }  break;
./





/.
          } // switch

          state_stack [tos] = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        {
          // ### ERROR RECOVERY HERE
          break;
        }
    }

    fprintf (stderr, "%s:%d: Syntax Error\n", qPrintable(context.fileName), context.line);

    return false;
}

#include "glsl-lex.incl"


/////////////////////////////
// entry point
/////////////////////////////
int main()
{
#if 0 // dump the GLSL grammar
    for (int r = 0; r < GLSLParserTable::RULE_COUNT; ++r)
      {
        int ridx = GLSLParserTable::rule_index [r];
        int rhs = GLSLParserTable::rhs [r];
        printf ("%3d) %s ::=", r + 1, GLSLParserTable::spell[GLSLParserTable::rule_info [ridx]]);
        ++ridx;
        for (int i = ridx; i < ridx + rhs; ++i)
          {
            int symbol = GLSLParserTable::rule_info [i];
            if (const char *name = GLSLParserTable::spell [symbol])
              printf (" %s", name);
            else
              printf (" #%d", symbol);
          }
        printf ("\n");
      }
#endif

    GLSLParser parser;

    if (parser.parse())
        qDebug() << "OK";
    else
        qDebug() << "KO";
}

./
