/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "cppgenerator.h"

#include "lalr.h"
#include "recognizer.h"

#include <QtCore/qbitarray.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qfile.h>
#include <QtCore/qmap.h>


QString CppGenerator::copyrightHeader() const
{
  return QLatin1String(
    "/****************************************************************************\n"
    "**\n"
    "** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).\n"
    "** Contact: http://www.qt-project.org/legal\n"
    "**\n"
    "** This file is part of the Qt Toolkit.\n"
    "**\n"
    "** $QT_BEGIN_LICENSE:LGPL$\n"
    "** Commercial License Usage\n"
    "** Licensees holding valid commercial Qt licenses may use this file in\n"
    "** accordance with the commercial license agreement provided with the\n"
    "** Software or, alternatively, in accordance with the terms contained in\n"
    "** a written agreement between you and Digia.  For licensing terms and\n"
    "** conditions see http://qt.digia.com/licensing.  For further information\n"
    "** use the contact form at http://qt.digia.com/contact-us.\n"
    "**\n"
    "** GNU Lesser General Public License Usage\n"
    "** Alternatively, this file may be used under the terms of the GNU Lesser\n"
    "** General Public License version 2.1 as published by the Free Software\n"
    "** Foundation and appearing in the file LICENSE.LGPL included in the\n"
    "** packaging of this file.  Please review the following information to\n"
    "** ensure the GNU Lesser General Public License version 2.1 requirements\n"
    "** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
    "**\n"
    "** In addition, as a special exception, Digia gives you certain additional\n"
    "** rights.  These rights are described in the Digia Qt LGPL Exception\n"
    "** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.\n"
    "**\n"
    "** GNU General Public License Usage\n"
    "** Alternatively, this file may be used under the terms of the GNU\n"
    "** General Public License version 3.0 as published by the Free Software\n"
    "** Foundation and appearing in the file LICENSE.GPL included in the\n"
    "** packaging of this file.  Please review the following information to\n"
    "** ensure the GNU General Public License version 3.0 requirements will be\n"
    "** met: http://www.gnu.org/copyleft/gpl.html.\n"
    "**\n"
    "**\n"
    "** $QT_END_LICENSE$\n"
    "**\n"
    "****************************************************************************/\n"
    "\n");
}

QString CppGenerator::privateCopyrightHeader() const
{
  return QLatin1String(
    "//\n"
    "//  W A R N I N G\n"
    "//  -------------\n"
    "//\n"
    "// This file is not part of the Qt API.  It exists for the convenience\n"
    "// of other Qt classes.  This header file may change from version to\n"
    "// version without notice, or even be removed.\n"
    "//\n"
    "// We mean it.\n"
    "//\n");
}

QString CppGenerator::startIncludeGuard(const QString &fileName)
{
    const QString normalized(QString(fileName).replace(QLatin1Char('.'), QLatin1Char('_')).toUpper());

    return QString::fromLatin1("#ifndef %1\n"
                               "#define %2\n").arg(normalized, normalized);
}

QString CppGenerator::endIncludeGuard(const QString &fileName)
{
    const QString normalized(QString(fileName).replace(QLatin1Char('.'), QLatin1Char('_')).toUpper());

    return QString::fromLatin1("#endif // %1\n").arg(normalized);
}

void CppGenerator::operator () ()
{
  // action table...
  state_count = aut.states.size ();
  terminal_count = grammar.terminals.size ();
  non_terminal_count = grammar.non_terminals.size ();

#define ACTION(i, j) table [(i) * terminal_count + (j)]
#define GOTO(i, j) pgoto [(i) * non_terminal_count + (j)]

  int *table = new int [state_count * terminal_count];
  ::memset (table, 0, state_count * terminal_count * sizeof (int));

  int *pgoto = new int [state_count * non_terminal_count];
  ::memset (pgoto, 0, state_count * non_terminal_count * sizeof (int));

  accept_state = -1;
  int shift_reduce_conflict_count = 0;
  int reduce_reduce_conflict_count = 0;

  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state)
    {
      int q = aut.id (state);

      for (Bundle::iterator a = state->bundle.begin (); a != state->bundle.end (); ++a)
        {
          int symbol = aut.id (a.key ());
          int r = aut.id (a.value ());

          Q_ASSERT (r < state_count);

          if (grammar.isNonTerminal (a.key ()))
            {
              Q_ASSERT (symbol >= terminal_count && symbol < grammar.names.size ());
              GOTO (q, symbol - terminal_count) = r;
            }

          else
            ACTION (q, symbol) = r;
        }

      for (ItemPointer item = state->closure.begin (); item != state->closure.end (); ++item)
        {
          if (item->dot != item->end_rhs ())
            continue;

          int r = aut.id (item->rule);

          NameSet lookaheads = aut.lookaheads.value (item);

          if (item->rule == grammar.goal)
            accept_state = q;

          foreach (Name s, lookaheads)
            {
              int &u = ACTION (q, aut.id (s));

              if (u == 0)
                u = - r;

              else if (u < 0)
                {
                  if (verbose)
                    qout << "*** Warning. Found a reduce/reduce conflict in state " << q << " on token ``" << s << "'' between rule "
                         << r << " and " << -u << endl;

                  ++reduce_reduce_conflict_count;

                  u = qMax (u, -r);

                  if (verbose)
                    qout << "\tresolved using rule " << -u << endl;
                }

              else if (u > 0)
                {
                  if (item->rule->prec != grammar.names.end() && grammar.token_info.contains (s))
                    {
                      Grammar::TokenInfo info_r = grammar.token_info.value (item->rule->prec);
                      Grammar::TokenInfo info_s = grammar.token_info.value (s);

                      if (info_r.prec > info_s.prec)
                        u = -r;
                      else if (info_r.prec == info_s.prec)
                        {
                          switch (info_r.assoc) {
                          case Grammar::Left:
                            u = -r;
                            break;
                          case Grammar::Right:
                            // shift... nothing to do
                            break;
                          case Grammar::NonAssoc:
                            u = 0;
                            break;
                          } // switch
                        }
                    }

                  else
                    {
                      ++shift_reduce_conflict_count;

                      if (verbose)
                        qout << "*** Warning. Found a shift/reduce conflict in state " << q << " on token ``" << s << "'' with rule " << r << endl;
                    }
                }
            }
        }
    }

  if (shift_reduce_conflict_count || reduce_reduce_conflict_count)
    {
      if (shift_reduce_conflict_count != grammar.expected_shift_reduce
          || reduce_reduce_conflict_count != grammar.expected_reduce_reduce)
        qerr << "*** Conflicts: " << shift_reduce_conflict_count << " shift/reduce, " << reduce_reduce_conflict_count << " reduce/reduce" << endl;

      if (verbose)
        qout << endl << "*** Conflicts: " << shift_reduce_conflict_count << " shift/reduce, " << reduce_reduce_conflict_count << " reduce/reduce" << endl
             << endl;
    }

  QBitArray used_rules (grammar.rules.count ());

  int q = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++q)
    {
      for (int j = 0; j < terminal_count; ++j)
        {
          int &u = ACTION (q, j);

          if (u < 0)
            used_rules.setBit (-u - 1);
        }
    }

  for (int i = 0; i < used_rules.count (); ++i)
    {
      if (! used_rules.testBit (i))
        {
          RulePointer rule = grammar.rules.begin () + i;

          if (rule != grammar.goal)
            qerr << "*** Warning: Rule ``" << *rule << "'' is useless!" << endl;
        }
    }

  q = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++q)
    {
      for (int j = 0; j < terminal_count; ++j)
        {
          int &u = ACTION (q, j);

          if (u >= 0)
            continue;

          RulePointer rule = grammar.rules.begin () + (- u - 1);

          if (state->defaultReduce == rule)
            u = 0;
        }
    }

  // ... compress the goto table
  defgoto.resize (non_terminal_count);
  for (int j = 0; j < non_terminal_count; ++j)
    {
      count.fill (0, state_count);

      int &mx = defgoto [j];

      for (int i = 0; i < state_count; ++i)
        {
          int r = GOTO (i, j);

          if (! r)
            continue;

          ++count [r];

          if (count [r] > count [mx])
            mx = r;
        }
    }

  for (int i = 0; i < state_count; ++i)
    {
      for (int j = 0; j < non_terminal_count; ++j)
        {
          int &r = GOTO (i, j);

          if (r == defgoto [j])
            r = 0;
        }
    }

  compressed_action (table, state_count, terminal_count);
  compressed_goto (pgoto, state_count, non_terminal_count);

  delete[] table;
  table = 0;

  delete[] pgoto;
  pgoto = 0;

#undef ACTION
#undef GOTO

  if (! grammar.merged_output.isEmpty())
    {
      QFile f(grammar.merged_output);
      if (! f.open (QFile::WriteOnly))
        {
          fprintf (stderr, "*** cannot create %s\n", qPrintable(grammar.merged_output));
          return;
        }

      QTextStream out (&f);

      // copyright headers must come first, otherwise the headers tests will fail
      if (copyright)
        {
          out << copyrightHeader()
              << privateCopyrightHeader()
              << endl;
        }

      out << "// This file was generated by qlalr - DO NOT EDIT!\n";

      out << startIncludeGuard(grammar.merged_output) << endl;

      if (copyright) {
          out << "#if defined(ERROR)" << endl
              << "#  undef ERROR" << endl
              << "#endif" << endl << endl;
      }

      generateDecl (out);
      generateImpl (out);
      out << p.decls();
      out << p.impls();
      out << endl;

      out << endIncludeGuard(grammar.merged_output) << endl;

      return;
    }

  // default behaviour
  QString declFileName = grammar.table_name.toLower () + QLatin1String("_p.h");
  QString bitsFileName = grammar.table_name.toLower () + QLatin1String(".cpp");

  { // decls...
    QFile f (declFileName);
    f.open (QFile::WriteOnly);
    QTextStream out (&f);

    QString prot = declFileName.toUpper ().replace (QLatin1Char ('.'), QLatin1Char ('_'));

    // copyright headers must come first, otherwise the headers tests will fail
    if (copyright)
      {
        out << copyrightHeader()
            << privateCopyrightHeader()
            << endl;
      }

    out << "// This file was generated by qlalr - DO NOT EDIT!\n";

    out << "#ifndef " << prot << endl
        << "#define " << prot << endl
        << endl;

    if (copyright) {
        out << "#include <QtCore/qglobal.h>" << endl << endl;
        out << "QT_BEGIN_NAMESPACE" << endl << endl;
    }
    generateDecl (out);
    if (copyright)
        out << "QT_END_NAMESPACE" << endl;

    out << "#endif // " << prot << endl << endl;
  } // end decls

  { // bits...
    QFile f (bitsFileName);
    f.open (QFile::WriteOnly);
    QTextStream out (&f);

    // copyright headers must come first, otherwise the headers tests will fail
    if (copyright)
      out << copyrightHeader();

    out << "// This file was generated by qlalr - DO NOT EDIT!\n";

    out << "#include \"" << declFileName << "\"" << endl << endl;
    if (copyright)
        out << "QT_BEGIN_NAMESPACE" << endl << endl;
    generateImpl(out);
    if (copyright)
        out << "QT_END_NAMESPACE" << endl;

  } // end bits

  if (! grammar.decl_file_name.isEmpty ())
    {
      QFile f (grammar.decl_file_name);
      f.open (QFile::WriteOnly);
      QTextStream out (&f);
      out << p.decls();
    }

  if (! grammar.impl_file_name.isEmpty ())
    {
      QFile f (grammar.impl_file_name);
      f.open (QFile::WriteOnly);
      QTextStream out (&f);
      out << p.impls();
    }
}

QString CppGenerator::debugInfoProt() const
{
    QString prot = QLatin1String("QLALR_NO_");
    prot += grammar.table_name.toUpper();
    prot += QLatin1String("_DEBUG_INFO");
    return prot;
}

void CppGenerator::generateDecl (QTextStream &out)
{
  out << "class " << grammar.table_name << endl
      << "{" << endl
      << "public:" << endl
      << "  enum VariousConstants {" << endl;

  foreach (Name t, grammar.terminals)
    {
      QString name = *t;
      int value = std::distance (grammar.names.begin (), t);

      if (name == QLatin1String ("$end"))
        name = QLatin1String ("EOF_SYMBOL");

      else if (name == QLatin1String ("$accept"))
        name = QLatin1String ("ACCEPT_SYMBOL");

      else
        name.prepend (grammar.token_prefix);

      out << "    " << name << " = " << value << "," << endl;
    }

  out << endl
      << "    ACCEPT_STATE = " << accept_state << "," << endl
      << "    RULE_COUNT = " << grammar.rules.size () << "," << endl
      << "    STATE_COUNT = " << state_count << "," << endl
      << "    TERMINAL_COUNT = " << terminal_count << "," << endl
      << "    NON_TERMINAL_COUNT = " << non_terminal_count << "," << endl
      << endl
      << "    GOTO_INDEX_OFFSET = " << compressed_action.index.size () << "," << endl
      << "    GOTO_INFO_OFFSET = " << compressed_action.info.size () << "," << endl
      << "    GOTO_CHECK_OFFSET = " << compressed_action.check.size () << endl
      << "  };" << endl
      << endl
      << "  static const char  *const    spell [];" << endl
      << "  static const short             lhs [];" << endl
      << "  static const short             rhs [];" << endl;

  if (debug_info)
    {
      QString prot = debugInfoProt();

      out << endl << "#ifndef " << prot << endl
          << "  static const int     rule_index [];" << endl
          << "  static const int      rule_info [];" << endl
          << "#endif // " << prot << endl << endl;
    }

  out << "  static const short    goto_default [];" << endl
      << "  static const short  action_default [];" << endl
      << "  static const short    action_index [];" << endl
      << "  static const short     action_info [];" << endl
      << "  static const short    action_check [];" << endl
      << endl
      << "  static inline int nt_action (int state, int nt)" << endl
      << "  {" << endl
      << "    const int yyn = action_index [GOTO_INDEX_OFFSET + state] + nt;" << endl
      << "    if (yyn < 0 || action_check [GOTO_CHECK_OFFSET + yyn] != nt)" << endl
      << "      return goto_default [nt];" << endl
      << endl
      << "    return action_info [GOTO_INFO_OFFSET + yyn];" << endl
      << "  }" << endl
      << endl
      << "  static inline int t_action (int state, int token)" << endl
      << "  {" << endl
      << "    const int yyn = action_index [state] + token;" << endl
      << endl
      << "    if (yyn < 0 || action_check [yyn] != token)" << endl
      << "      return - action_default [state];" << endl
      << endl
      << "    return action_info [yyn];" << endl
      << "  }" << endl
      << "};" << endl
      << endl
      << endl;
}

void CppGenerator::generateImpl (QTextStream &out)
{
  int idx = 0;

  out << "const char *const " << grammar.table_name << "::spell [] = {";
  idx = 0;

  QMap<Name, int> name_ids;
  bool first_nt = true;

  for (Name t = grammar.names.begin (); t != grammar.names.end (); ++t, ++idx)
    {
      bool terminal = grammar.isTerminal (t);

      if (! (debug_info || terminal))
        break;

      name_ids.insert (t, idx);

      if (idx)
        out << ", ";

      if (! (idx % 10))
        out << endl << "  ";

      if (terminal)
        {
          QString spell = grammar.spells.value (t);

          if (spell.isEmpty ())
            out << "0";
          else
            out << "\"" << spell << "\"";
        }
      else
        {
          if (first_nt)
            {
              first_nt = false;
              QString prot = debugInfoProt();
              out << endl << "#ifndef " << prot << endl;
            }
          out << "\"" << *t << "\"";
        }
    }

  if (debug_info)
    out << endl << "#endif // " << debugInfoProt() << endl;

  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::lhs [] = {";
  idx = 0;
  for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
    {
      if (idx)
        out << ", ";

      if (! (idx % 10))
        out << endl << "  ";

      out << aut.id (rule->lhs);
    }
  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::rhs [] = {";
  idx = 0;
  for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
    {
      if (idx)
        out << ", ";

      if (! (idx % 10))
        out << endl << "  ";

      out << rule->rhs.size ();
    }
  out << "};" << endl << endl;

  if (debug_info)
    {
      QString prot = debugInfoProt();

      out << endl << "#ifndef " << prot << endl;
      out << "const int " << grammar.table_name << "::rule_info [] = {";
      idx = 0;
      for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
        {
          out << endl << "  ";

          if (idx)
            out << ", ";
          else
            out << "  ";

          out << name_ids.value(rule->lhs);

          foreach (Name n, rule->rhs)
            out << ", " << name_ids.value (n);
        }
      out << "};" << endl << endl;

      out << "const int " << grammar.table_name << "::rule_index [] = {";
      idx = 0;
      int offset = 0;
      for (RulePointer rule = grammar.rules.begin (); rule != grammar.rules.end (); ++rule, ++idx)
        {
          if (idx)
            out << ", ";

          if (! (idx % 10))
            out << endl << "  ";

          out << offset;
          offset += rule->rhs.size () + 1;
        }
      out << "};" << endl
          << "#endif // " << prot << endl << endl;
    }

  out << "const short " << grammar.table_name << "::action_default [] = {";
  idx = 0;
  for (StatePointer state = aut.states.begin (); state != aut.states.end (); ++state, ++idx)
    {
      if (state != aut.states.begin ())
        out << ", ";

      if (! (idx % 10))
        out << endl << "  ";

      if (state->defaultReduce != grammar.rules.end ())
        out << aut.id (state->defaultReduce);
      else
        out << "0";
    }
  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::goto_default [] = {";
  for (int i = 0; i < defgoto.size (); ++i)
    {
      if (i)
        out << ", ";

      if (! (i % 10))
        out << endl << "  ";

      out << defgoto [i];
    }
  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::action_index [] = {";
  for (int i = 0; i < compressed_action.index.size (); ++i)
    {
      if (! (i % 10))
        out << endl << "  ";

      out << compressed_action.index [i] << ", ";
    }
  out << endl;
  for (int i = 0; i < compressed_goto.index.size (); ++i)
    {
      if (i)
        out << ", ";

      if (! (i % 10))
        out << endl << "  ";

      out << compressed_goto.index [i];
    }
  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::action_info [] = {";
  for (int i = 0; i < compressed_action.info.size (); ++i)
    {
      if (! (i % 10))
        out << endl << "  ";

      out << compressed_action.info [i] << ", ";
    }
  out << endl;
  for (int i = 0; i < compressed_goto.info.size (); ++i)
    {
      if (i)
        out << ", ";

      if (! (i % 10))
        out << endl << "  ";

      out << compressed_goto.info [i];
    }
  out << "};" << endl << endl;

  out << "const short " << grammar.table_name << "::action_check [] = {";
  for (int i = 0; i < compressed_action.check.size (); ++i)
    {
      if (! (i % 10))
        out << endl << "  ";

      out << compressed_action.check [i] << ", ";
    }
  out << endl;
  for (int i = 0; i < compressed_goto.check.size (); ++i)
    {
      if (i)
        out << ", ";

      if (! (i % 10))
        out << endl << "  ";

      out << compressed_goto.check [i];
    }
  out << "};" << endl << endl;
}
