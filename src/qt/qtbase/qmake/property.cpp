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

#include "property.h"
#include "option.h"

#include <qdir.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

static const struct {
    const char *name;
    QLibraryInfo::LibraryLocation loc;
    bool raw;
} propList[] = {
    { "QT_SYSROOT", QLibraryInfo::SysrootPath, true },
    { "QT_INSTALL_PREFIX", QLibraryInfo::PrefixPath, false },
    { "QT_INSTALL_ARCHDATA", QLibraryInfo::ArchDataPath, false },
    { "QT_INSTALL_DATA", QLibraryInfo::DataPath, false },
    { "QT_INSTALL_DOCS", QLibraryInfo::DocumentationPath, false },
    { "QT_INSTALL_HEADERS", QLibraryInfo::HeadersPath, false },
    { "QT_INSTALL_LIBS", QLibraryInfo::LibrariesPath, false },
    { "QT_INSTALL_LIBEXECS", QLibraryInfo::LibraryExecutablesPath, false },
    { "QT_INSTALL_BINS", QLibraryInfo::BinariesPath, false },
    { "QT_INSTALL_TESTS", QLibraryInfo::TestsPath, false },
    { "QT_INSTALL_PLUGINS", QLibraryInfo::PluginsPath, false },
    { "QT_INSTALL_IMPORTS", QLibraryInfo::ImportsPath, false },
    { "QT_INSTALL_QML", QLibraryInfo::Qml2ImportsPath, false },
    { "QT_INSTALL_TRANSLATIONS", QLibraryInfo::TranslationsPath, false },
    { "QT_INSTALL_CONFIGURATION", QLibraryInfo::SettingsPath, false },
    { "QT_INSTALL_EXAMPLES", QLibraryInfo::ExamplesPath, false },
    { "QT_INSTALL_DEMOS", QLibraryInfo::ExamplesPath, false }, // Just backwards compat
    { "QT_HOST_PREFIX", QLibraryInfo::HostPrefixPath, true },
    { "QT_HOST_DATA", QLibraryInfo::HostDataPath, true },
    { "QT_HOST_BINS", QLibraryInfo::HostBinariesPath, true },
    { "QT_HOST_LIBS", QLibraryInfo::HostLibrariesPath, true },
    { "QMAKE_SPEC", QLibraryInfo::HostSpecPath, true },
    { "QMAKE_XSPEC", QLibraryInfo::TargetSpecPath, true },
};

QMakeProperty::QMakeProperty() : settings(0)
{
    for (unsigned i = 0; i < sizeof(propList)/sizeof(propList[0]); i++) {
        QString name = QString::fromLatin1(propList[i].name);
        m_values[ProKey(name + "/src")] = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::EffectiveSourcePaths);
        m_values[ProKey(name + "/get")] = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::EffectivePaths);
        QString val = QLibraryInfo::rawLocation(propList[i].loc, QLibraryInfo::FinalPaths);
        if (!propList[i].raw) {
            m_values[ProKey(name)] = QLibraryInfo::location(propList[i].loc);
            name += "/raw";
        }
        m_values[ProKey(name)] = val;
    }
    m_values["QMAKE_VERSION"] = ProString(QMAKE_VERSION_STR);
#ifdef QT_VERSION_STR
    m_values["QT_VERSION"] = ProString(QT_VERSION_STR);
#endif
}

QMakeProperty::~QMakeProperty()
{
    delete settings;
    settings = 0;
}

void QMakeProperty::initSettings()
{
    if(!settings) {
        settings = new QSettings(QSettings::UserScope, "QtProject", "QMake");
        settings->setFallbacksEnabled(false);
    }
}

ProString
QMakeProperty::value(const ProKey &vk)
{
    ProString val = m_values.value(vk);
    if (!val.isNull())
        return val;

    initSettings();
    return settings->value(vk.toQString()).toString();
}

bool
QMakeProperty::hasValue(const ProKey &v)
{
    return !value(v).isNull();
}

void
QMakeProperty::setValue(QString var, const QString &val)
{
    initSettings();
    settings->setValue(var, val);
}

void
QMakeProperty::remove(const QString &var)
{
    initSettings();
    settings->remove(var);
}

bool
QMakeProperty::exec()
{
    bool ret = true;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY) {
        if(Option::prop::properties.isEmpty()) {
            initSettings();
            foreach (const QString &key, settings->childKeys()) {
                QString val = settings->value(key).toString();
                fprintf(stdout, "%s:%s\n", qPrintable(key), qPrintable(val));
            }
            QStringList specialProps;
            for (unsigned i = 0; i < sizeof(propList)/sizeof(propList[0]); i++)
                specialProps.append(QString::fromLatin1(propList[i].name));
            specialProps.append("QMAKE_VERSION");
#ifdef QT_VERSION_STR
            specialProps.append("QT_VERSION");
#endif
            foreach (QString prop, specialProps) {
                ProString val = value(ProKey(prop));
                ProString pval = value(ProKey(prop + "/raw"));
                ProString gval = value(ProKey(prop + "/get"));
                ProString sval = value(ProKey(prop + "/src"));
                fprintf(stdout, "%s:%s\n", prop.toLatin1().constData(), val.toLatin1().constData());
                if (!pval.isEmpty() && pval != val)
                    fprintf(stdout, "%s/raw:%s\n", prop.toLatin1().constData(), pval.toLatin1().constData());
                if (!gval.isEmpty() && gval != (pval.isEmpty() ? val : pval))
                    fprintf(stdout, "%s/get:%s\n", prop.toLatin1().constData(), gval.toLatin1().constData());
                if (!sval.isEmpty() && sval != gval)
                    fprintf(stdout, "%s/src:%s\n", prop.toLatin1().constData(), sval.toLatin1().constData());
            }
            return true;
        }
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            if(Option::prop::properties.count() > 1)
                fprintf(stdout, "%s:", (*it).toLatin1().constData());
            const ProKey pkey(*it);
            if (!hasValue(pkey)) {
                ret = false;
                fprintf(stdout, "**Unknown**\n");
            } else {
                fprintf(stdout, "%s\n", value(pkey).toLatin1().constData());
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
