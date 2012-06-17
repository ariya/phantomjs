/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifdef USE_LEXEM_STORE
Symbol::LexemStore Symbol::lexemStore;
#endif

static const char *error_msg = 0;

#ifdef Q_CC_MSVC
#define ErrorFormatString "%s(%d): "
#else
#define ErrorFormatString "%s:%d: "
#endif

void Parser::error(int rollback) {
    index -= rollback;
    error();
}
void Parser::error(const char *msg) {
    if (msg || error_msg)
        qWarning(ErrorFormatString "Error: %s",
                 currentFilenames.top().constData(), symbol().lineNum, msg?msg:error_msg);
    else
        qWarning(ErrorFormatString "Parse error at \"%s\"",
                 currentFilenames.top().constData(), symbol().lineNum, symbol().lexem().data());
    exit(EXIT_FAILURE);
}

void Parser::warning(const char *msg) {
    if (displayWarnings && msg)
        fprintf(stderr, ErrorFormatString "Warning: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), msg);
}

void Parser::note(const char *msg) {
    if (displayNotes && msg)
        fprintf(stderr, ErrorFormatString "Note: %s\n",
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), msg);
}

QT_END_NAMESPACE
