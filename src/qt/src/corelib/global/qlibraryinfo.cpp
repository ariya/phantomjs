/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qdir.h"
#include "qfile.h"
#include "qconfig.h"
#include "qsettings.h"
#include "qlibraryinfo.h"
#include "qscopedpointer.h"

#if defined(QT_BUILD_QMAKE) || defined(QT_BOOTSTRAPPED)
# define BOOTSTRAPPING
#endif

#ifdef BOOTSTRAPPING
QT_BEGIN_NAMESPACE
extern QString qmake_libraryInfoFile();
QT_END_NAMESPACE
#else
# include "qcoreapplication.h"
#endif

#ifdef Q_OS_MAC
#  include "private/qcore_mac_p.h"
#endif

#ifdef QLIBRARYINFO_EPOCROOT
# include "symbian/epocroot_p.h"
#endif

#include "qconfig.cpp"

QT_BEGIN_NAMESPACE

extern void qDumpCPUFeatures(); // in qsimd.cpp

#ifndef QT_NO_SETTINGS

struct QLibrarySettings
{
    QLibrarySettings();
    QScopedPointer<QSettings> settings;
};
Q_GLOBAL_STATIC(QLibrarySettings, qt_library_settings)

class QLibraryInfoPrivate
{
public:
    static QSettings *findConfiguration();
    static void cleanup()
    {
        QLibrarySettings *ls = qt_library_settings();
        if (ls)
            ls->settings.reset(0);
    }
    static QSettings *configuration()
    {
        QLibrarySettings *ls = qt_library_settings();
        return ls ? ls->settings.data() : 0;
    }
};

QLibrarySettings::QLibrarySettings()
    : settings(QLibraryInfoPrivate::findConfiguration())
{
#ifndef BOOTSTRAPPING
    qAddPostRoutine(QLibraryInfoPrivate::cleanup);
#endif
}

QSettings *QLibraryInfoPrivate::findConfiguration()
{
    QString qtconfig = QLatin1String(":/qt/etc/qt.conf");
#ifdef BOOTSTRAPPING
    if(!QFile::exists(qtconfig))
        qtconfig = qmake_libraryInfoFile();
#else
    if (!QFile::exists(qtconfig) && QCoreApplication::instance()) {
#ifdef Q_OS_MAC
	CFBundleRef bundleRef = CFBundleGetMainBundle();
        if (bundleRef) {
	    QCFType<CFURLRef> urlRef = CFBundleCopyResourceURL(bundleRef,
							       QCFString(QLatin1String("qt.conf")),
							       0,
							       0);
	    if (urlRef) {
	        QCFString path = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
		qtconfig = QDir::cleanPath(path);
	    }
	}
	if (qtconfig.isEmpty())
#endif
            {
                QDir pwd(QCoreApplication::applicationDirPath());
                qtconfig = pwd.filePath(QLatin1String("qt.conf"));
	    }
    }
#endif
    if (QFile::exists(qtconfig))
        return new QSettings(qtconfig, QSettings::IniFormat);
    return 0;     //no luck
}

/*!
    \class QLibraryInfo
    \brief The QLibraryInfo class provides information about the Qt library.

    Many pieces of information are established when Qt is configured.
    Installation paths, license information, and even a unique build
    key. This class provides an abstraction for accessing this
    information.

    \table
    \header \o Function           \o Return value
    \row    \o buildKey()         \o A string that identifies the Qt version and
                                     the configuration. This key is used to ensure
                                     that \l{plugins} link against the same version
                                     of Qt as the application.
    \row    \o location()         \o The path to a certain Qt
                                     component (e.g., documentation, header files).
    \row    \o licensee(),
               licensedProducts() \o Licensing information.
    \endtable

    You can also use a \c qt.conf file to override the hard-coded paths
    that are compiled into the Qt library. For more information, see
    the \l {Using qt.conf} documentation.

    \sa QSysInfo, {Using qt.conf}
*/

/*! \internal

   You cannot create a QLibraryInfo, instead only the static functions are available to query
   information.
*/

QLibraryInfo::QLibraryInfo()
{ }

/*!
  Returns the person to whom this build of Qt is licensed.

  \sa licensedProducts()
*/

QString
QLibraryInfo::licensee()
{
    const char *str = QT_CONFIGURE_LICENSEE;
    return QString::fromLocal8Bit(str);
}

/*!
  Returns the products that the license for this build of Qt has access to.

  \sa licensee()
*/

QString
QLibraryInfo::licensedProducts()
{
    const char *str = QT_CONFIGURE_LICENSED_PRODUCTS;
    return QString::fromLatin1(str);
}

/*!
    Returns a unique key identifying this build of Qt and its
    configurations. This key is not globally unique, rather only useful
    for establishing of two configurations are compatible. This can be
    used to compare with the \c QT_BUILD_KEY preprocessor symbol.

    \sa location()
*/

QString
QLibraryInfo::buildKey()
{
    return QString::fromLatin1(QT_BUILD_KEY);
}

/*!
    \since 4.6
    Returns the installation date for this build of Qt. The install date will
    usually be the last time that Qt sources were configured.
*/
#ifndef QT_NO_DATESTRING
QDate
QLibraryInfo::buildDate()
{
    return QDate::fromString(QString::fromLatin1(qt_configure_installation + 12), Qt::ISODate);
}
#endif //QT_NO_DATESTRING

/*!
  Returns the location specified by \a loc.

*/

QString
QLibraryInfo::location(LibraryLocation loc)
{
    QString ret;
    if(!QLibraryInfoPrivate::configuration()) {
        const char *path = 0;
        switch (loc) {
#ifdef QT_CONFIGURE_PREFIX_PATH
        case PrefixPath:
            path = QT_CONFIGURE_PREFIX_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_DOCUMENTATION_PATH
        case DocumentationPath:
            path = QT_CONFIGURE_DOCUMENTATION_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_HEADERS_PATH
        case HeadersPath:
            path = QT_CONFIGURE_HEADERS_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_LIBRARIES_PATH
        case LibrariesPath:
            path = QT_CONFIGURE_LIBRARIES_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_BINARIES_PATH
        case BinariesPath:
            path = QT_CONFIGURE_BINARIES_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_PLUGINS_PATH
        case PluginsPath:
            path = QT_CONFIGURE_PLUGINS_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_IMPORTS_PATH
        case ImportsPath:
            path = QT_CONFIGURE_IMPORTS_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_DATA_PATH
        case DataPath:
            path = QT_CONFIGURE_DATA_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_TRANSLATIONS_PATH
        case TranslationsPath:
            path = QT_CONFIGURE_TRANSLATIONS_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_SETTINGS_PATH
        case SettingsPath:
            path = QT_CONFIGURE_SETTINGS_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_EXAMPLES_PATH
        case ExamplesPath:
            path = QT_CONFIGURE_EXAMPLES_PATH;
            break;
#endif
#ifdef QT_CONFIGURE_DEMOS_PATH
        case DemosPath:
            path = QT_CONFIGURE_DEMOS_PATH;
            break;
#endif
        default:
            break;
        }

        if (path)
            ret = QString::fromLocal8Bit(path);
    } else {
        QString key;
        QString defaultValue;
        switch(loc) {
        case PrefixPath:
            key = QLatin1String("Prefix");
            break;
        case DocumentationPath:
            key = QLatin1String("Documentation");
            defaultValue = QLatin1String("doc");
            break;
        case HeadersPath:
            key = QLatin1String("Headers");
            defaultValue = QLatin1String("include");
            break;
        case LibrariesPath:
            key = QLatin1String("Libraries");
            defaultValue = QLatin1String("lib");
            break;
        case BinariesPath:
            key = QLatin1String("Binaries");
            defaultValue = QLatin1String("bin");
            break;
        case PluginsPath:
            key = QLatin1String("Plugins");
            defaultValue = QLatin1String("plugins");
            break;
        case ImportsPath:
            key = QLatin1String("Imports");
            defaultValue = QLatin1String("imports");
            break;
        case DataPath:
            key = QLatin1String("Data");
            break;
        case TranslationsPath:
            key = QLatin1String("Translations");
            defaultValue = QLatin1String("translations");
            break;
        case SettingsPath:
            key = QLatin1String("Settings");
            break;
        case ExamplesPath:
            key = QLatin1String("Examples");
            break;
        case DemosPath:
            key = QLatin1String("Demos");
            break;
        default:
            break;
        }

        if(!key.isNull()) {
            QSettings *config = QLibraryInfoPrivate::configuration();
            config->beginGroup(QLatin1String("Paths"));

            QString subKey;
            {
                /*
                  find the child group whose version number is closest
                  to the library version.  for example and we have the
                  following groups:

                  Paths
                  Paths/4.0
                  Paths/4.1.2
                  Paths/4.2.5
                  Paths/5

                  if QT_VERSION is 4.0.1, then we use 'Paths/4.0'
                  if QT_VERSION is 4.1.5, then we use 'Paths/4.1.2'
                  if QT_VERSION is 4.6.3, then we use 'Paths/4.2.5'
                  if QT_VERSION is 6.0.2, then we use 'Paths/5'

                  note: any of the trailing version numbers may be
                  omitted (in which case, they default to zero),
                  i.e. 4 == 4.0.0, 4.1 == 4.1.0, and so on
                */
                enum {
                    QT_MAJOR = ((QT_VERSION >> 16) & 0xFF),
                    QT_MINOR = ((QT_VERSION >> 8) & 0xFF),
                    QT_PATCH = (QT_VERSION & 0xFF)
                };
                int maj = 0, min = 0, pat = 0;
                QStringList children = config->childGroups();
                for(int child = 0; child < children.size(); ++child) {
                    QString cver = children.at(child);
                    QStringList cver_list = cver.split(QLatin1Char('.'));
                    if(cver_list.size() > 0 && cver_list.size() < 4) {
                        bool ok;
                        int cmaj = -1, cmin = -1, cpat = -1;
                        cmaj = cver_list[0].toInt(&ok);
                        if(!ok || cmaj < 0)
                            continue;
                        if(cver_list.size() >= 2) {
                            cmin = cver_list[1].toInt(&ok);
                            if(!ok)
                                continue;
                            if(cmin < 0)
                                cmin = -1;
                        }
                        if(cver_list.size() >= 3) {
                            cpat = cver_list[2].toInt(&ok);
                            if(!ok)
                                continue;
                            if(cpat < 0)
                                cpat = -1;
                        }
                        if((cmaj >= maj && cmaj <= QT_MAJOR) &&
                           (cmin == -1 || (cmin >= min && cmin <= QT_MINOR)) &&
                           (cpat == -1 || (cpat >= pat && cpat <= QT_PATCH)) &&
                           config->contains(cver + QLatin1Char('/') + key)) {
                            subKey = cver + QLatin1Char('/');
                            maj = cmaj;
                            min = cmin;
                            pat = cpat;
                        }
                    }
                }
            }
            ret = config->value(subKey + key, defaultValue).toString();
            // expand environment variables in the form $(ENVVAR)
            int rep;
            QRegExp reg_var(QLatin1String("\\$\\(.*\\)"));
            reg_var.setMinimal(true);
            while((rep = reg_var.indexIn(ret)) != -1) {
                ret.replace(rep, reg_var.matchedLength(),
                            QString::fromLocal8Bit(qgetenv(ret.mid(rep + 2,
                                reg_var.matchedLength() - 3).toLatin1().constData()).constData()));
            }

#ifdef QLIBRARYINFO_EPOCROOT
            // $${EPOCROOT} is a special case, resolve it similarly to qmake.
            QRegExp epocrootMatcher(QLatin1String("\\$\\$\\{EPOCROOT\\}"));
            if ((rep = epocrootMatcher.indexIn(ret)) != -1)
                ret.replace(rep, epocrootMatcher.matchedLength(), qt_epocRoot());
#endif

            config->endGroup();
        }
    }

    if (QDir::isRelativePath(ret)) {
        QString baseDir;
        if (loc == PrefixPath) {
            // we make the prefix path absolute to the executable's directory
#ifdef BOOTSTRAPPING
            baseDir = QFileInfo(qmake_libraryInfoFile()).absolutePath();
#else
            if (QCoreApplication::instance()) {
#ifdef Q_OS_MAC
                CFBundleRef bundleRef = CFBundleGetMainBundle();
                if (bundleRef) {
                    QCFType<CFURLRef> urlRef = CFBundleCopyBundleURL(bundleRef);
                    if (urlRef) {
                        QCFString path = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
                        return QDir::cleanPath(QString(path) + QLatin1String("/Contents/") + ret);
                    }
                }
#endif
                baseDir = QCoreApplication::applicationDirPath();
            } else {
                baseDir = QDir::currentPath();
            }
#endif
        } else {
            // we make any other path absolute to the prefix directory
            baseDir = location(PrefixPath);
        }
        ret = QDir::cleanPath(baseDir + QLatin1Char('/') + ret);
    }
    return ret;
}

/*!
    \enum QLibraryInfo::LibraryLocation

    \keyword library location

    This enum type is used to specify a specific location
    specifier:

    \value PrefixPath The default prefix for all paths.
    \value DocumentationPath The location for documentation upon install.
    \value HeadersPath The location for all headers.
    \value LibrariesPath The location of installed libraries.
    \value BinariesPath The location of installed Qt binaries (tools and applications).
    \value PluginsPath The location of installed Qt plugins.
    \value ImportsPath The location of installed QML extensions to import.
    \value DataPath The location of general Qt data.
    \value TranslationsPath The location of translation information for Qt strings.
    \value SettingsPath The location for Qt settings.
    \value ExamplesPath The location for examples upon install.
    \value DemosPath The location for demos upon install.

    \sa location()
*/

#endif // QT_NO_SETTINGS

QT_END_NAMESPACE

#if defined(Q_CC_GNU) && defined(ELF_INTERPRETER)
#  include <stdio.h>
#  include <stdlib.h>

extern const char qt_core_interpreter[] __attribute__((section(".interp")))
    = ELF_INTERPRETER;

extern "C" void qt_core_boilerplate();
void qt_core_boilerplate()
{
    printf("This is the QtCore library version " QT_VERSION_STR "\n"
           "Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).\n"
           "Contact: Nokia Corporation (qt-info@nokia.com)\n"
           "\n"
           "Build key:           " QT_BUILD_KEY "\n"
           "Compat build key:    "
#ifdef QT_BUILD_KEY_COMPAT
           "| " QT_BUILD_KEY_COMPAT " "
#endif
#ifdef QT_BUILD_KEY_COMPAT2
           "| " QT_BUILD_KEY_COMPAT2 " "
#endif
#ifdef QT_BUILD_KEY_COMPAT3
           "| " QT_BUILD_KEY_COMPAT3 " "
#endif
           "|\n"
           "Build date:          %s\n"
           "Installation prefix: %s\n"
           "Library path:        %s\n"
           "Include path:        %s\n",
           qt_configure_installation + 12,
           qt_configure_prefix_path_str + 12,
           qt_configure_libraries_path_str + 12,
           qt_configure_headers_path_str + 12);

    QT_PREPEND_NAMESPACE(qDumpCPUFeatures)();

#ifdef QT_EVAL
    extern void qt_core_eval_init(uint);
    qt_core_eval_init(1);
#endif

    exit(0);
}

#endif
