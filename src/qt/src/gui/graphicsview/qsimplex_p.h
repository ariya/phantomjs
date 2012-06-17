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

#ifndef QSIMPLEX_P_H
#define QSIMPLEX_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

struct QSimplexVariable
{
    QSimplexVariable() : result(0), index(0) {}

    qreal result;
    int index;
};


/*!
  \internal

  Representation of a LP constraint like:

    (c1 * X1) + (c2 * X2) + ...  =  K
                             or <=  K
                             or >=  K

    Where (ci, Xi) are the pairs in "variables" and K the real "constant".
*/
struct QSimplexConstraint
{
    QSimplexConstraint() : constant(0), ratio(Equal), artificial(0) {}

    enum Ratio {
        LessOrEqual = 0,
        Equal,
        MoreOrEqual
    };

    QHash<QSimplexVariable *, qreal> variables;
    qreal constant;
    Ratio ratio;

    QPair<QSimplexVariable *, qreal> helper;
    QSimplexVariable * artificial;

    void invert();

    bool isSatisfied() {
        qreal leftHandSide(0);

        QHash<QSimplexVariable *, qreal>::const_iterator iter;
        for (iter = variables.constBegin(); iter != variables.constEnd(); ++iter) {
            leftHandSide += iter.value() * iter.key()->result;
        }

        Q_ASSERT(constant > 0 || qFuzzyCompare(1, 1 + constant));

        if ((leftHandSide == constant) || qAbs(leftHandSide - constant) < 0.0000001)
            return true;

        switch (ratio) {
        case LessOrEqual:
            return leftHandSide < constant;
        case MoreOrEqual:
            return leftHandSide > constant;
        default:
            return false;
        }
    }

#ifdef QT_DEBUG
    QString toString() {
        QString result;
        result += QString::fromAscii("-- QSimplexConstraint %1 --").arg(quintptr(this), 0, 16);

        QHash<QSimplexVariable *, qreal>::const_iterator iter;
        for (iter = variables.constBegin(); iter != variables.constEnd(); ++iter) {
            result += QString::fromAscii("  %1 x %2").arg(iter.value()).arg(quintptr(iter.key()), 0, 16);
        }

        switch (ratio) {
        case LessOrEqual:
            result += QString::fromAscii("  (less) <= %1").arg(constant);
            break;
        case MoreOrEqual:
            result += QString::fromAscii("  (more) >= %1").arg(constant);
            break;
        default:
            result += QString::fromAscii("  (eqal) == %1").arg(constant);
        }

        return result;
    }
#endif
};

class QSimplex
{
public:
    QSimplex();
    virtual ~QSimplex();

    qreal solveMin();
    qreal solveMax();

    bool setConstraints(const QList<QSimplexConstraint *> constraints);
    void setObjective(QSimplexConstraint *objective);

    void dumpMatrix();

private:
    // Matrix handling
    qreal valueAt(int row, int column);
    void setValueAt(int row, int column, qreal value);
    void clearRow(int rowIndex);
    void clearColumns(int first, int last);
    void combineRows(int toIndex, int fromIndex, qreal factor);

    // Simplex
    bool simplifyConstraints(QList<QSimplexConstraint *> *constraints);
    int findPivotColumn();
    int pivotRowForColumn(int column);
    void reducedRowEchelon();
    bool iterate();

    // Helpers
    void clearDataStructures();
    void solveMaxHelper();
    enum solverFactor { Minimum = -1, Maximum = 1 };
    qreal solver(solverFactor factor);
    void collectResults();

    QList<QSimplexConstraint *> constraints;
    QList<QSimplexVariable *> variables;
    QSimplexConstraint *objective;

    int rows;
    int columns;
    int firstArtificial;

    qreal *matrix;
};

inline qreal QSimplex::valueAt(int rowIndex, int columnIndex)
{
    return matrix[rowIndex * columns + columnIndex];
}

inline void QSimplex::setValueAt(int rowIndex, int columnIndex, qreal value)
{
    matrix[rowIndex * columns + columnIndex] = value;
}

QT_END_NAMESPACE

#endif // QSIMPLEX_P_H
