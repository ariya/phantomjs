/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTEXTTABLE_P_H
#define QTEXTTABLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qtextobject_p.h"
#include "private/qtextdocument_p.h"

QT_BEGIN_NAMESPACE

class QTextTablePrivate : public QTextFramePrivate
{
    Q_DECLARE_PUBLIC(QTextTable)
public:
    QTextTablePrivate(QTextDocument *document) : QTextFramePrivate(document), grid(0), nRows(0), nCols(0), dirty(true), blockFragmentUpdates(false) {}
    ~QTextTablePrivate();

    static QTextTable *createTable(QTextDocumentPrivate *, int pos, int rows, int cols, const QTextTableFormat &tableFormat);
    void fragmentAdded(const QChar &type, uint fragment);
    void fragmentRemoved(const QChar &type, uint fragment);

    void update() const;

    int findCellIndex(int fragment) const;

    QList<int> cells;
    // symmetric to cells array and maps to indecs in grid,
    // used for fast-lookup for row/column by fragment
    mutable QVector<int> cellIndices;
    mutable int *grid;
    mutable int nRows;
    mutable int nCols;
    mutable bool dirty;
    bool blockFragmentUpdates;
};

QT_END_NAMESPACE

#endif // QTEXTTABLE_P_H
