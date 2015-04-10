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

%parser XMLTable

%impl xmlreader.cpp

%token LEFT_ANGLE
%token RIGHT_ANGLE
%token ANY

%start XmlStream

/.
#ifndef XMLREADER_H
#define XMLREADER_H

#include <QtCore>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "$header"

class XMLReader: protected $table
{
public:
    XMLReader(const QByteArray &bytes);
    ~XMLReader();

    bool parse();

    inline int nextToken()
    {
        switch (*bufptr++) {
        case '\0':
            return EOF_SYMBOL;

        case '<':
            in_tag = true;
            return LEFT_ANGLE;

        case '>':
            if (! in_tag)
                break;
            in_tag = false;
            return RIGHT_ANGLE;
            break;

        } // switch

        return ANY;
    }

protected:
    inline void reallocateStack();

    inline int &sym(int index)
    { return stack [tos + index - 1].ival; }

protected:
    int tos;
    int stack_size;

    struct StackItem {
        int state;
        int ival;
    };

    QVarLengthArray<StackItem> stack;
    unsigned in_tag: 1;
    QByteArray bytes;
    const char *bufptr;
};

inline void XMLReader::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    stack.resize (stack_size);
}

#endif // XMLREADER_H

XMLReader::XMLReader(const QByteArray &bytes):
    tos(0),
    stack_size(0),
    bytes(bytes)
{
    bufptr = bytes.constData();
}

XMLReader::~XMLReader()
{
}

bool XMLReader::parse()
{
  const int INITIAL_STATE = 0;

  in_tag = 0;
  bufptr = bytes.constData();

  int yytoken = -1;
  reallocateStack();

  tos = 0;
  stack [++tos].state = INITIAL_STATE;

  while (true)
    {
      const int state = stack [tos].state;

      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state])
        yytoken = nextToken();

      int act = t_action (state, yytoken);

      if (act == ACCEPT_STATE)
        return true;

      else if (act > 0)
        {
          if (++tos == stack_size)
            reallocateStack();

          stack [tos].ival = *bufptr; // ### save the token value here
          stack [tos].state = act;
          yytoken = -1;
        }

      else if (act < 0)
        {
          int r = - act - 1;

          tos -= rhs [r];
          act = stack [tos++].state;

          switch (r) {
./




XmlStream: TagOrWord ;
XmlStream: XmlStream TagOrWord ;

TagOrWord: Tag ;
TagOrWord: ANY ;

Tag: LEFT_ANGLE TagName RIGHT_ANGLE ;
/.
    case $rule_number: {
        fprintf (stderr, "*** found a tag\n");
    } break;
./

TagName: ANY ;
TagName: TagName ANY ;


/.
          } // switch

          stack [tos].state = nt_action (act, lhs [r] - TERMINAL_COUNT);
        }

      else
        {
          // ### ERROR RECOVERY HERE
          break;
        }
    }

    return false;
}



/////////////////////////////
// entry point
/////////////////////////////
int main(int, char *argv[])
{
    QFile f (argv[1]);

    if (f.open(QFile::ReadOnly)) {
        QByteArray contents = f.readAll();
        XMLReader parser (contents);

        if (parser.parse())
            printf ("OK\n");
        else
            printf ("KO\n");
    }
}




./

