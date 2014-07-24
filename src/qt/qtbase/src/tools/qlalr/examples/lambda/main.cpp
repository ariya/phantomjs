/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QLALR module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "lambda.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "parser_table_p.h"

class Parser: protected parser_table
{
public:
    union Value {
      int ival;
      // ### more...
    };

public:
    Parser();
    ~Parser();

    bool parse();

protected:
    inline void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    int nextToken();
    void consumeRule(int ruleno);

protected:
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    int current_char;
    unsigned in_tag: 1;
};

inline void Parser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (::realloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (::realloc(state_stack, stack_size * sizeof(int)));
}

Parser::Parser():
    tos(0),
    stack_size(0),
    sym_stack(0),
    state_stack(0)
{
}

Parser::~Parser()
{
    if (stack_size) {
        ::free(sym_stack);
        ::free(state_stack);
    }
}

bool Parser::parse()
{
  const int INITIAL_STATE = 0;

  current_char = 0;
  in_tag = 0;

  int yytoken = -1;
  reallocateStack();

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

          sym_stack [tos].ival = current_char; // ### save the token value here
          state_stack [tos] = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          tos -= rhs [r];
          act = state_stack [tos++];
          consumeRule (r);
          state_stack [tos] = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        break;
    }

    return false;
}


int Parser::nextToken()
{
    static int tokens[] = { ID, ID, ID, EOF_SYMBOL };
    static int *tk = tokens;

    return *tk++;
}

void Parser::consumeRule(int ruleno)
{
    switch (ruleno) {
    case Symbol:
        printf("symbol\n");
        break;
    case SubExpression:
        printf("sub-expr\n");
        break;
    case Appl:
        printf("appl\n");
        break;
    case Abstr:
        printf("abstr\n");
        break;
    }
}

/////////////////////////////
// entry point
/////////////////////////////
int main()
{
    Parser parser;

    if (parser.parse())
        printf ("OK\n");
    else
        printf ("KO\n");
}


