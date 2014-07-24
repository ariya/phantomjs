/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "driver.h"
#include "uic.h"
#include "ui4.h"

#include <qfileinfo.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

Driver::Driver()
    : m_stdout(stdout, QFile::WriteOnly | QFile::Text)
{
    m_output = &m_stdout;
}

Driver::~Driver()
{
}

QString Driver::findOrInsertWidget(DomWidget *ui_widget)
{
    if (!m_widgets.contains(ui_widget))
        m_widgets.insert(ui_widget, unique(ui_widget->attributeName(), ui_widget->attributeClass()));

    return m_widgets.value(ui_widget);
}

QString Driver::findOrInsertSpacer(DomSpacer *ui_spacer)
{
    if (!m_spacers.contains(ui_spacer)) {
        QString name;
        if (ui_spacer->hasAttributeName())
            name = ui_spacer->attributeName();
        m_spacers.insert(ui_spacer, unique(name, QLatin1String("QSpacerItem")));
    }

    return m_spacers.value(ui_spacer);
}

QString Driver::findOrInsertLayout(DomLayout *ui_layout)
{
    if (!m_layouts.contains(ui_layout)) {
        QString name;
        if (ui_layout->hasAttributeName())
            name = ui_layout->attributeName();
        m_layouts.insert(ui_layout, unique(name, ui_layout->attributeClass()));
    }

    return m_layouts.value(ui_layout);
}

QString Driver::findOrInsertLayoutItem(DomLayoutItem *ui_layoutItem)
{
    switch (ui_layoutItem->kind()) {
        case DomLayoutItem::Widget:
            return findOrInsertWidget(ui_layoutItem->elementWidget());
        case DomLayoutItem::Spacer:
            return findOrInsertSpacer(ui_layoutItem->elementSpacer());
        case DomLayoutItem::Layout:
            return findOrInsertLayout(ui_layoutItem->elementLayout());
        case DomLayoutItem::Unknown:
            break;
    }

    Q_ASSERT( 0 );

    return QString();
}

QString Driver::findOrInsertActionGroup(DomActionGroup *ui_group)
{
    if (!m_actionGroups.contains(ui_group))
        m_actionGroups.insert(ui_group, unique(ui_group->attributeName(), QLatin1String("QActionGroup")));

    return m_actionGroups.value(ui_group);
}

QString Driver::findOrInsertAction(DomAction *ui_action)
{
    if (!m_actions.contains(ui_action))
        m_actions.insert(ui_action, unique(ui_action->attributeName(), QLatin1String("QAction")));

    return m_actions.value(ui_action);
}

QString Driver::findOrInsertButtonGroup(const DomButtonGroup *ui_group)
{
    ButtonGroupNameHash::iterator it = m_buttonGroups.find(ui_group);
    if (it == m_buttonGroups.end())
        it = m_buttonGroups.insert(ui_group, unique(ui_group->attributeName(), QLatin1String("QButtonGroup")));
    return it.value();
}

// Find a group by its non-uniqified name
const DomButtonGroup *Driver::findButtonGroup(const QString &attributeName) const
{
    const ButtonGroupNameHash::const_iterator cend = m_buttonGroups.constEnd();
    for (ButtonGroupNameHash::const_iterator it = m_buttonGroups.constBegin(); it != cend; ++it)
        if (it.key()->attributeName() == attributeName)
            return it.key();
    return 0;
}


QString Driver::findOrInsertName(const QString &name)
{
    return unique(name);
}

QString Driver::normalizedName(const QString &name)
{
    QString result = name;
    QChar *data = result.data();
    for (int i = name.size(); --i >= 0; ++data) {
        if (!data->isLetterOrNumber())
            *data = QLatin1Char('_');
    }
    return result;
}

QString Driver::unique(const QString &instanceName, const QString &className)
{
    QString name;
    bool alreadyUsed = false;

    if (instanceName.size()) {
        int id = 1;
        name = instanceName;
        name = normalizedName(name);
        QString base = name;

        while (m_nameRepository.contains(name)) {
            alreadyUsed = true;
            name = base + QString::number(id++);
        }
    } else if (className.size()) {
        name = unique(qtify(className));
    } else {
        name = unique(QLatin1String("var"));
    }

    if (alreadyUsed && className.size()) {
        fprintf(stderr, "%s: Warning: The name '%s' (%s) is already in use, defaulting to '%s'.\n",
                qPrintable(m_option.messagePrefix()),
                qPrintable(instanceName), qPrintable(className),
                qPrintable(name));
    }

    m_nameRepository.insert(name, true);
    return name;
}

QString Driver::qtify(const QString &name)
{
    QString qname = name;

    if (qname.at(0) == QLatin1Char('Q') || qname.at(0) == QLatin1Char('K'))
        qname = qname.mid(1);

    int i=0;
    while (i < qname.length()) {
        if (qname.at(i).toLower() != qname.at(i))
            qname[i] = qname.at(i).toLower();
        else
            break;

        ++i;
    }

    return qname;
}

static bool isAnsiCCharacter(QChar c)
{
    return (c.toUpper() >= QLatin1Char('A') && c.toUpper() <= QLatin1Char('Z'))
           || c.isDigit() || c == QLatin1Char('_');
}

QString Driver::headerFileName() const
{
    QString name = m_option.outputFile;

    if (name.isEmpty()) {
        name = QLatin1String("ui_"); // ### use ui_ as prefix.
        name.append(m_option.inputFile);
    }

    return headerFileName(name);
}

QString Driver::headerFileName(const QString &fileName)
{
    if (fileName.isEmpty())
        return headerFileName(QLatin1String("noname"));

    QFileInfo info(fileName);
    QString baseName = info.baseName();
    // Transform into a valid C++ identifier
    if (!baseName.isEmpty() && baseName.at(0).isDigit())
        baseName.prepend(QLatin1Char('_'));
    for (int i = 0; i < baseName.size(); ++i) {
        QChar c = baseName.at(i);
        if (!isAnsiCCharacter(c)) {
            // Replace character by its unicode value
            QString hex = QString::number(c.unicode(), 16);
            baseName.replace(i, 1, QLatin1Char('_') + hex + QLatin1Char('_'));
            i += hex.size() + 1;
        }
    }
    return baseName.toUpper() + QLatin1String("_H");
}

bool Driver::printDependencies(const QString &fileName)
{
    Q_ASSERT(m_option.dependencies == true);

    m_option.inputFile = fileName;

    Uic tool(this);
    return tool.printDependencies();
}

bool Driver::uic(const QString &fileName, DomUI *ui, QTextStream *out)
{
    m_option.inputFile = fileName;

    QTextStream *oldOutput = m_output;

    m_output = out != 0 ? out : &m_stdout;

    Uic tool(this);
    bool rtn = false;
#ifdef QT_UIC_CPP_GENERATOR
    rtn = tool.write(ui);
#else
    Q_UNUSED(ui);
    fprintf(stderr, "uic: option to generate cpp code not compiled in [%s:%d]\n",
            __FILE__, __LINE__);
#endif

    m_output = oldOutput;

    return rtn;
}

bool Driver::uic(const QString &fileName, QTextStream *out)
{
    QFile f;
    if (fileName.isEmpty())
        f.open(stdin, QIODevice::ReadOnly);
    else {
        f.setFileName(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return false;
    }

    m_option.inputFile = fileName;

    QTextStream *oldOutput = m_output;
    bool deleteOutput = false;

    if (out) {
        m_output = out;
    } else {
#ifdef Q_OS_WIN
        // As one might also redirect the output to a file on win,
        // we should not create the textstream with QFile::Text flag.
        // The redirected file is opened in TextMode and this will
        // result in broken line endings as writing will replace \n again.
        m_output = new QTextStream(stdout, QIODevice::WriteOnly);
#else
        m_output = new QTextStream(stdout, QIODevice::WriteOnly | QFile::Text);
#endif
        deleteOutput = true;
    }

    Uic tool(this);
    bool rtn = tool.write(&f);
    f.close();

    if (deleteOutput)
        delete m_output;

    m_output = oldOutput;

    return rtn;
}

void Driver::reset()
{
    Q_ASSERT( m_output == 0 );

    m_option = Option();
    m_output = 0;
    m_problems.clear();

    QStringList m_problems;

    m_widgets.clear();
    m_spacers.clear();
    m_layouts.clear();
    m_actionGroups.clear();
    m_actions.clear();
    m_nameRepository.clear();
    m_pixmaps.clear();
}

void Driver::insertPixmap(const QString &pixmap)
{
    m_pixmaps.insert(pixmap, true);
}

bool Driver::containsPixmap(const QString &pixmap) const
{
    return m_pixmaps.contains(pixmap);
}

DomWidget *Driver::widgetByName(const QString &name) const
{
    return m_widgets.key(name);
}

DomSpacer *Driver::spacerByName(const QString &name) const
{
    return m_spacers.key(name);
}

DomLayout *Driver::layoutByName(const QString &name) const
{
    return m_layouts.key(name);
}

DomActionGroup *Driver::actionGroupByName(const QString &name) const
{
    return m_actionGroups.key(name);
}

DomAction *Driver::actionByName(const QString &name) const
{
    return m_actions.key(name);
}

QT_END_NAMESPACE
