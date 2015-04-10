
/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QLALR project on Qt Labs.
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

#include "recognizer.h"

#include <cstdlib>
#include <cstring>
#include <cctype>

Recognizer::Recognizer (Grammar *grammar, bool no_lines):
  tos(0),
  stack_size(0),
  state_stack(0),
  _M_line(1),
  _M_action_line(0),
  _M_grammar(grammar),
  _M_no_lines(no_lines)
{
}

Recognizer::~Recognizer()
{
  if (stack_size)
    ::free(state_stack);
}

inline void Recognizer::reallocateStack()
{
  if (! stack_size)
    stack_size = 128;
  else
    stack_size <<= 1;

  sym_stack.resize (stack_size);

  if (! state_stack)
    state_stack = reinterpret_cast<int*> (::malloc(stack_size * sizeof(int)));
  else
    state_stack = reinterpret_cast<int*> (::realloc(state_stack, stack_size * sizeof(int)));
}

int Recognizer::nextToken()
{
  QString text;

 Lagain:
  while (ch.isSpace ())
    inp ();

  if (ch.isNull ())
    return EOF_SYMBOL;

  int token = ch.unicode ();

  if (token == '"')
    {
      inp(); // skip "
      text.clear ();
      while (! ch.isNull () && ch != QLatin1Char ('"'))
        {
          if (ch == QLatin1Char ('\\'))
            {
              text += ch;
              inp();
            }
          text += ch;
          inp ();
        }

      if (ch == QLatin1Char ('"'))
        inp ();
      else
        qerr << _M_input_file << ":" << _M_line << ": Warning. Expected `\"'" << endl;

      _M_current_value = text;
      return (token = STRING_LITERAL);
    }

  else if (ch.isLetterOrNumber () || ch == QLatin1Char ('_'))
    {
      text.clear ();
      do { text += ch; inp (); }
      while (ch.isLetterOrNumber () || ch == QLatin1Char ('_') || ch == QLatin1Char ('.'));
      _M_current_value = text;
      return (token = ID);
    }

  else if (token == '%')
    {
      text.clear ();

      do { inp (); }
      while (ch.isSpace ());

      do { text += ch; inp (); }
      while (ch.isLetterOrNumber () || ch == QLatin1Char ('_') || ch == QLatin1Char ('-'));

      if (text == QLatin1String("token_prefix"))
        return (token = TOKEN_PREFIX);
      else if (text == QLatin1String("merged_output"))
        return (token = MERGED_OUTPUT);
      else if (text == QLatin1String("token"))
        return (token = TOKEN);
      else if (text == QLatin1String("start"))
        return (token = START);
      else if (text == QLatin1String("parser"))
        return (token = PARSER);
      else if (text == QLatin1String("decl"))
        return (token = DECL_FILE);
      else if (text == QLatin1String("impl"))
        return (token = IMPL_FILE);
      else if (text == QLatin1String("expect"))
        return (token = EXPECT);
      else if (text == QLatin1String("expect-rr"))
        return (token = EXPECT_RR);
      else if (text == QLatin1String("left"))
        return (token = LEFT);
      else if (text == QLatin1String("right"))
        return (token = RIGHT);
      else if (text == QLatin1String("nonassoc"))
        return (token = NONASSOC);
      else if (text == QLatin1String("prec"))
        return (token = PREC);
      else
        {
          qerr << _M_input_file << ":" << _M_line << ": Unknown keyword `" << text << "'" << endl;
          exit (EXIT_FAILURE);
          return (token = ERROR);
        }
    }

  inp ();

  if (token == '-' && ch == QLatin1Char ('-'))
    {
      do { inp (); }
      while (! ch.isNull () && ch != QLatin1Char ('\n'));
      goto Lagain;
    }

  else if (token == ':' && ch == QLatin1Char (':'))
    {
      inp ();
      if (ch != QLatin1Char ('='))
        return (token = ERROR);
      inp ();
      return (token = COLON);
    }

  else if (token == '/' && ch == QLatin1Char (':'))
    {
      _M_action_line = _M_line;

      text.clear ();
      if (! _M_no_lines)
        text += QLatin1String ("\n#line ") + QString::number (_M_action_line) + " \"" + _M_input_file + "\"\n";
      inp (); // skip ':'

      forever
        {
          while (! ch.isNull ())
            {
              token = ch.unicode ();
              inp ();

              if (token == ':' && ch == QLatin1Char ('/'))
                break;

              text += QLatin1Char (token);
            }

          if (ch != QLatin1Char ('/'))
            return (token = ERROR);

          inp ();

          if (ch.isNull () || ch.isSpace ())
            {
              _M_current_value = text;
              return (token = DECL);
            }
          else
            text += QLatin1String (":/");
        }
    }

  else if (token == '/' && ch == QLatin1Char ('.'))
    {
      _M_action_line = _M_line;

      text.clear ();
      if (! _M_no_lines)
        text += QLatin1String ("\n#line ") + QString::number (_M_action_line) + " \"" + _M_input_file + "\"\n";

      inp (); // skip ':'

      forever
        {
          while (! ch.isNull ())
            {
              token = ch.unicode ();
              inp ();

              if (token == '.' && ch == QLatin1Char ('/'))
                break;

              text += QLatin1Char (token);
            }

          if (ch != QLatin1Char ('/'))
            return (token = ERROR);

          inp ();

          if (ch.isNull () || ch.isSpace ())
            {
              _M_current_value = text;
              return (token = IMPL);
            }
          else
            text += QLatin1String ("");
        }
    }

  switch (token) {
  case ':':
    return (token = COLON);

  case ';':
    return (token = SEMICOLON);

  case '|':
    return (token = OR);

  default:
    break;
  }

  return token;
}

bool Recognizer::parse (const QString &input_file)
{
  _M_input_file = input_file;

  QFile file(_M_input_file);
  if (! file.open(QFile::ReadOnly))
    {
      qerr << "qlalr: no input file\n";
      return false;
    }

  QString _M_contents = QTextStream(&file).readAll();
  _M_firstChar = _M_contents.constBegin();
  _M_lastChar = _M_contents.constEnd();
  _M_currentChar = _M_firstChar;
  _M_line = 1;

  int yytoken = -1;
  inp ();

  reallocateStack();

  _M_current_rule = _M_grammar->rules.end ();
  _M_decls.clear ();
  _M_impls.clear ();

  tos = 0;
  state_stack[++tos] = 0;

  while (true)
    {
      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state_stack [tos]])
        yytoken = nextToken();

      int act = t_action (state_stack [tos], yytoken);

      if (act == ACCEPT_STATE)
        return true;

      else if (act > 0)
        {
          if (++tos == stack_size)
            reallocateStack();

          sym_stack [tos] = _M_current_value;
          state_stack [tos] = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          tos -= rhs [r];
          act = state_stack [tos++];

          switch (r) {

case 3: {
  Name name = _M_grammar->intern (sym(2));
  _M_grammar->start = name;
  _M_grammar->non_terminals.insert (name);
} break;

case 5: {
  _M_grammar->table_name = sym(2);
} break;

case 6: {
  _M_grammar->merged_output = sym(2);
} break;

case 7: {
   _M_grammar->decl_file_name = sym(2);
} break;

case 8: {
   _M_grammar->impl_file_name = sym(2);
} break;

case 9: {
   _M_grammar->expected_shift_reduce = sym(2).toInt();
} break;

case 10: {
   _M_grammar->expected_reduce_reduce = sym(2).toInt();
} break;

case 11: {
  _M_grammar->token_prefix = sym(2);
} break;
case 17:case 18: {
  Name name = _M_grammar->intern (sym(1));
  _M_grammar->terminals.insert (name);
  _M_grammar->spells.insert (name, sym(2));
} break;

case 19: {
  _M_grammar->current_assoc = Grammar::Left;
  ++_M_grammar->current_prec;
} break;

case 20: {
  _M_grammar->current_assoc = Grammar::Right;
  ++_M_grammar->current_prec;
} break;

case 21: {
  _M_grammar->current_assoc = Grammar::NonAssoc;
  ++_M_grammar->current_prec;
} break;

case 25: {
  Name name = _M_grammar->intern (sym(1));
  _M_grammar->terminals.insert (name);

  Grammar::TokenInfo info;
  info.prec = _M_grammar->current_prec;
  info.assoc = _M_grammar->current_assoc;
  _M_grammar->token_info.insert (name, info);
} break;

case 26: {
  _M_decls += expand (sym(1));
} break;

case 27: {
  _M_impls += expand (sym(1));
} break;

case 34: {
  _M_current_rule = _M_grammar->rules.insert (_M_grammar->rules.end (), Rule ());
  _M_current_rule->lhs = _M_grammar->intern (sym(1));
  _M_grammar->declared_lhs.insert (_M_current_rule->lhs);

  if (_M_grammar->terminals.find (_M_current_rule->lhs) != _M_grammar->terminals.end ())
    {
      qerr << _M_input_file << ":" << _M_line << ": Invalid non terminal `" << *_M_current_rule->lhs << "'" << endl;
      return false;
    }

  _M_grammar->non_terminals.insert (_M_current_rule->lhs);
} break;

case 38: {
  Name lhs = _M_current_rule->lhs;
  _M_current_rule = _M_grammar->rules.insert (_M_grammar->rules.end (), Rule ());
  _M_current_rule->lhs = lhs;
  _M_grammar->declared_lhs.insert (_M_current_rule->lhs);

  if (_M_grammar->terminals.find (_M_current_rule->lhs) != _M_grammar->terminals.end ())
    {
      qerr << _M_input_file << ":" << _M_line << ": Invalid non terminal `" << *_M_current_rule->lhs << "'" << endl;
      return false;
    }

  _M_grammar->non_terminals.insert (_M_current_rule->lhs);
} break;

case 39: {
  _M_current_rule->prec = _M_grammar->names.end ();

  for (NameList::iterator it = _M_current_rule->rhs.begin (); it != _M_current_rule->rhs.end (); ++it)
    {
      if (! _M_grammar->isTerminal (*it))
        continue;

      _M_current_rule->prec = *it;
    }
} break;

case 40: {
  Name tok = _M_grammar->intern (sym(2));
  if (! _M_grammar->isTerminal (tok))
    {
      qerr << _M_input_file << ":" << _M_line << ": `" << *tok << " is not a terminal symbol" << endl;
      _M_current_rule->prec = _M_grammar->names.end ();
    }
  else
    _M_current_rule->prec = tok;
} break;

case 42: {
  Name name = _M_grammar->intern (sym(2));

  if (_M_grammar->terminals.find (name) == _M_grammar->terminals.end ())
    _M_grammar->non_terminals.insert (name);

  _M_current_rule->rhs.push_back (name);
} break;

case 43: {
  sym(1) = QString();
} break;

          } // switch

          state_stack [tos] = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        {
          break;
        }
    }

  qerr << _M_input_file << ":" << _M_line << ": Syntax error" << endl;
  return false;
}

