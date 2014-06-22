/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QLOGGINGREGISTRY_P_H
#define QLOGGINGREGISTRY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qvector.h>

class tst_QLoggingRegistry;

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QLoggingRule
{
public:
    QLoggingRule();
    QLoggingRule(const QStringRef &pattern, bool enabled);
    int pass(const QString &categoryName, QtMsgType type) const;

    enum PatternFlag {
        FullText = 0x1,
        LeftFilter = 0x2,
        RightFilter = 0x4,
        MidFilter = LeftFilter |  RightFilter
    };
    Q_DECLARE_FLAGS(PatternFlags, PatternFlag)

    QString category;
    int messageType;
    PatternFlags flags;
    bool enabled;

private:
    void parse(const QStringRef &pattern);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLoggingRule::PatternFlags)
Q_DECLARE_TYPEINFO(QLoggingRule, Q_MOVABLE_TYPE);

class Q_AUTOTEST_EXPORT QLoggingSettingsParser
{
public:
    void setSection(const QString &section) { _section = section; }

    void setContent(const QString &content);
    void setContent(QTextStream &stream);

    QVector<QLoggingRule> rules() const { return _rules; }

private:
    QString _section;
    QVector<QLoggingRule> _rules;
};

class Q_AUTOTEST_EXPORT QLoggingRegistry
{
public:
    QLoggingRegistry();

    void init();

    void registerCategory(QLoggingCategory *category);
    void unregisterCategory(QLoggingCategory *category);

    void setApiRules(const QString &content);

    QLoggingCategory::CategoryFilter
    installFilter(QLoggingCategory::CategoryFilter filter);

    static QLoggingRegistry *instance();

private:
    void updateRules();

    static void defaultCategoryFilter(QLoggingCategory *category);

    QMutex registryMutex;

    QVector<QLoggingRule> configRules;
    QVector<QLoggingRule> envRules;
    QVector<QLoggingRule> apiRules;
    QVector<QLoggingRule> rules;
    QList<QLoggingCategory*> categories;
    QLoggingCategory::CategoryFilter categoryFilter;

    friend class ::tst_QLoggingRegistry;
};

QT_END_NAMESPACE

#endif // QLOGGINGREGISTRY_P_H
