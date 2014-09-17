/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "property.h"
#include "option.h"

#include <qdir.h>
#include <qmap.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

QStringList qmake_mkspec_paths(); //project.cpp

QMakeProperty::QMakeProperty() : settings(0)
{
}

QMakeProperty::~QMakeProperty()
{
    delete settings;
    settings = 0;
}

void QMakeProperty::initSettings()
{
    if(!settings) {
        settings = new QSettings(QSettings::UserScope, "Trolltech", "QMake");
        settings->setFallbacksEnabled(false);
    }
}

QString
QMakeProperty::keyBase(bool version) const
{
    if (version)
        return QString(qmake_version()) + "/";
    return QString();
}

QString
QMakeProperty::value(QString v, bool just_check)
{
    if(v == "QT_INSTALL_PREFIX")
        return QLibraryInfo::location(QLibraryInfo::PrefixPath);
    else if(v == "QT_INSTALL_DATA")
        return QLibraryInfo::location(QLibraryInfo::DataPath);
    else if(v == "QT_INSTALL_DOCS")
        return QLibraryInfo::location(QLibraryInfo::DocumentationPath);
    else if(v == "QT_INSTALL_HEADERS")
        return QLibraryInfo::location(QLibraryInfo::HeadersPath);
    else if(v == "QT_INSTALL_LIBS")
        return QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    else if(v == "QT_INSTALL_BINS")
        return QLibraryInfo::location(QLibraryInfo::BinariesPath);
    else if(v == "QT_INSTALL_PLUGINS")
        return QLibraryInfo::location(QLibraryInfo::PluginsPath);
    else if(v == "QT_INSTALL_IMPORTS")
        return QLibraryInfo::location(QLibraryInfo::ImportsPath);
    else if(v == "QT_INSTALL_TRANSLATIONS")
        return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    else if(v == "QT_INSTALL_CONFIGURATION")
        return QLibraryInfo::location(QLibraryInfo::SettingsPath);
    else if(v == "QT_INSTALL_EXAMPLES")
        return QLibraryInfo::location(QLibraryInfo::ExamplesPath);
    else if(v == "QT_INSTALL_DEMOS")
        return QLibraryInfo::location(QLibraryInfo::DemosPath);
    else if(v == "QMAKE_MKSPECS")
        return qmake_mkspec_paths().join(Option::dirlist_sep);
    else if(v == "QMAKE_VERSION")
        return qmake_version();
#ifdef QT_VERSION_STR
    else if(v == "QT_VERSION")
        return QT_VERSION_STR;
#endif

    initSettings();
    int slash = v.lastIndexOf('/');
    QVariant var = settings->value(keyBase(slash == -1) + v);
    bool ok = var.isValid();
    QString ret = var.toString();
    if(!ok) {
        QString version = qmake_version();
        if(slash != -1) {
            version = v.left(slash-1);
            v = v.mid(slash+1);
        }
        settings->beginGroup(keyBase(false));
        QStringList subs = settings->childGroups();
        settings->endGroup();
        subs.sort();
        for (int x = subs.count() - 1; x >= 0; x--) {
            QString s = subs[x];
            if(s.isEmpty() || s > version)
                continue;
            var = settings->value(keyBase(false) + s + "/" + v);
            ok = var.isValid();
            ret = var.toString();
            if (ok) {
                if(!just_check)
                    debug_msg(1, "Fell back from %s -> %s for '%s'.", version.toLatin1().constData(),
                              s.toLatin1().constData(), v.toLatin1().constData());
                return ret;
            }
        }
    }
    return ok ? ret : QString();
}

bool
QMakeProperty::hasValue(QString v)
{
    return !value(v, true).isNull();
}

void
QMakeProperty::setValue(QString var, const QString &val)
{
    initSettings();
    settings->setValue(keyBase() + var, val);
}

void
QMakeProperty::remove(const QString &var)
{
    initSettings();
    settings->remove(keyBase() + var);
}

bool
QMakeProperty::exec()
{
    bool ret = true;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY) {
        if(Option::prop::properties.isEmpty()) {
            initSettings();
            settings->beginGroup(keyBase(false));
            QStringList subs = settings->childGroups();
            settings->endGroup();
            subs.sort();
            for(int x = subs.count() - 1; x >= 0; x--) {
                QString s = subs[x];
                if(s.isEmpty())
                    continue;
                settings->beginGroup(keyBase(false) + s);
                QStringList keys = settings->childKeys();
                settings->endGroup();
                for(QStringList::ConstIterator it2 = keys.begin(); it2 != keys.end(); it2++) {
                    QString ret = settings->value(keyBase(false) + s + "/" + (*it2)).toString();
                    if(s != qmake_version())
                        fprintf(stdout, "%s/", s.toLatin1().constData());
                    fprintf(stdout, "%s:%s\n", (*it2).toLatin1().constData(), ret.toLatin1().constData());
                }
            }
            QStringList specialProps;
            specialProps.append("QT_INSTALL_PREFIX");
            specialProps.append("QT_INSTALL_DATA");
            specialProps.append("QT_INSTALL_DOCS");
            specialProps.append("QT_INSTALL_HEADERS");
            specialProps.append("QT_INSTALL_LIBS");
            specialProps.append("QT_INSTALL_BINS");
            specialProps.append("QT_INSTALL_PLUGINS");
            specialProps.append("QT_INSTALL_IMPORTS");
            specialProps.append("QT_INSTALL_TRANSLATIONS");
            specialProps.append("QT_INSTALL_CONFIGURATION");
            specialProps.append("QT_INSTALL_EXAMPLES");
            specialProps.append("QT_INSTALL_DEMOS");
            specialProps.append("QMAKE_MKSPECS");
            specialProps.append("QMAKE_VERSION");
#ifdef QT_VERSION_STR
            specialProps.append("QT_VERSION");
#endif
            foreach (QString prop, specialProps)
                fprintf(stdout, "%s:%s\n", prop.toLatin1().constData(), value(prop).toLatin1().constData());
            return true;
        }
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            if(Option::prop::properties.count() > 1)
                fprintf(stdout, "%s:", (*it).toLatin1().constData());
            if(!hasValue((*it))) {
                ret = false;
                fprintf(stdout, "**Unknown**\n");
            } else {
                fprintf(stdout, "%s\n", value((*it)).toLatin1().constData());
            }
        }
    } else if(Option::qmake_mode == Option::QMAKE_SET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            it++;
            if(it == Option::prop::properties.end()) {
                ret = false;
                break;
            }
            if(!var.startsWith("."))
                setValue(var, (*it));
        }
    } else if(Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            if(!var.startsWith("."))
                remove(var);
        }
    }
    return ret;
}

QT_END_NAMESPACE
