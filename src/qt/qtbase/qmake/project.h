/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef PROJECT_H
#define PROJECT_H

#include <qmakeevaluator.h>

QT_BEGIN_NAMESPACE

class QMakeProject : private QMakeEvaluator
{
    QString m_projectFile;

public:
    QMakeProject();
    QMakeProject(QMakeProject *p);

    bool read(const QString &project, LoadFlags what = LoadAll);

    QString projectFile() const { return m_projectFile; }
    QString sourceRoot() const { return m_sourceRoot.isEmpty() ? m_buildRoot : m_sourceRoot; }
    QString buildRoot() const { return m_buildRoot; }
    QString confFile() const { return m_conffile; }
    QString cacheFile() const { return m_cachefile; }
    QString specDir() const { return m_qmakespec; }

    ProString expand(const QString &v, const QString &file, int line);
    QStringList expand(const ProKey &func, const QList<ProStringList> &args);
    bool test(const QString &v, const QString &file, int line)
        { m_current.clear(); return evaluateConditional(v, file, line); }
    bool test(const ProKey &func, const QList<ProStringList> &args);

    bool isSet(const ProKey &v) const { return m_valuemapStack.first().contains(v); }
    bool isEmpty(const ProKey &v) const;
    ProStringList &values(const ProKey &v) { return valuesRef(v); }
    int intValue(const ProKey &v, int defaultValue = 0) const;
    const ProValueMap &variables() const { return m_valuemapStack.first(); }
    ProValueMap &variables() { return m_valuemapStack.first(); }

    void dump() const;

    using QMakeEvaluator::LoadFlags;
    using QMakeEvaluator::VisitReturn;
    using QMakeEvaluator::setExtraVars;
    using QMakeEvaluator::setExtraConfigs;
    using QMakeEvaluator::loadSpec;
    using QMakeEvaluator::evaluateFeatureFile;
    using QMakeEvaluator::evaluateConfigFeatures;
    using QMakeEvaluator::evaluateExpression;
    using QMakeEvaluator::values;
    using QMakeEvaluator::first;
    using QMakeEvaluator::isActiveConfig;
    using QMakeEvaluator::isHostBuild;
    using QMakeEvaluator::dirSep;

private:
    static bool boolRet(VisitReturn vr);
};

/*!
 * For variables that are supposed to contain a single int,
 * this method returns the numeric value.
 * Only the first value of the variable is taken into account.
 * The string representation is assumed to look like a C int literal.
 */
inline int QMakeProject::intValue(const ProKey &v, int defaultValue) const
{
    const ProString &str = first(v);
    if (!str.isEmpty()) {
        bool ok;
        int i = str.toInt(&ok, 0);
        if (ok)
            return i;
    }
    return defaultValue;
}

QT_END_NAMESPACE

#endif // PROJECT_H
