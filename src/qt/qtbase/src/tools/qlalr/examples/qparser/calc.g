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

%parser calc_grammar
%decl calc_parser.h
%impl calc_parser.cpp

%token_prefix Token_
%token number
%token lparen
%token rparen
%token plus
%token minus

%start Goal

/:
#ifndef CALC_PARSER_H
#define CALC_PARSER_H

#include "qparser.h"
#include "calc_grammar_p.h"

class CalcParser: public QParser<CalcParser, $table>
{
public:
  int nextToken();
  void consumeRule(int ruleno);
};

#endif // CALC_PARSER_H
:/





/.
#include "calc_parser.h"

#include <QtDebug>
#include <cstdlib>

void CalcParser::consumeRule(int ruleno)
  {
    switch (ruleno) {
./

Goal: Expression ;
/.
case $rule_number:
  qDebug() << "value:" << sym(1);
  break;
./

PrimaryExpression: number ;
PrimaryExpression: lparen Expression rparen ;
/.
case $rule_number:
  sym(1) = sym (2);
  break;
./

Expression: PrimaryExpression ;

Expression: Expression plus PrimaryExpression;
/.
case $rule_number:
  sym(1) += sym (3);
  break;
./

Expression: Expression minus PrimaryExpression;
/.
case $rule_number:
  sym(1) -= sym (3);
  break;
./



/.
    } // switch
}

#include <cstdio>

int main()
{
  CalcParser p;

  if (p.parse())
    printf("ok\n");
}
./
