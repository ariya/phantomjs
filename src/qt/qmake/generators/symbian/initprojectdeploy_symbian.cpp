/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "initprojectdeploy_symbian.h"
#include <QDirIterator>
#include <project.h>
#include <qxmlstream.h>
#include <qsettings.h>
#include <qdebug.h>

// Included from tools/shared
#include <symbian/epocroot_p.h>

#define SYSBIN_DIR "/sys/bin"
#define HW_Z_DIR "epoc32/data/z"

#define SUFFIX_DLL "dll"
#define SUFFIX_EXE "exe"
#define SUFFIX_QTPLUGIN "qtplugin"

static QString fixPathToEpocOS(const QString &src)
{
    QString ret = Option::fixPathToTargetOS(src);

    bool pathHasDriveLetter = false;
    if (ret.size() > 1)
        pathHasDriveLetter = (ret.at(1) == QLatin1Char(':'));

    return pathHasDriveLetter ? ret.replace('/', '\\') : QDir::toNativeSeparators(ret);
}

static bool isPlugin(const QFileInfo& info, const QString& devicePath)
{
    // Libraries are plugins if deployment path is something else than
    // SYSBIN_DIR with or without drive letter
    if (0 == info.suffix().compare(QLatin1String(SUFFIX_DLL), Qt::CaseInsensitive)
            && (devicePath.size() < 8
             || (0 != devicePath.compare(QLatin1String(SYSBIN_DIR), Qt::CaseInsensitive)
                && 0 != devicePath.mid(1).compare(QLatin1String(":" SYSBIN_DIR), Qt::CaseInsensitive)
                && 0 != devicePath.compare(qt_epocRoot() + QLatin1String(HW_Z_DIR SYSBIN_DIR))))) {
        return true;
    } else {
        return false;
    }
}

static bool isBinary(const QFileInfo& info)
{
    if (0 == info.suffix().compare(QLatin1String(SUFFIX_DLL), Qt::CaseInsensitive) ||
            0 == info.suffix().compare(QLatin1String(SUFFIX_EXE), Qt::CaseInsensitive)) {
        return true;
    } else {
        return false;
    }
}

static void createPluginStub(const QFileInfo& info,
                             const QString& devicePath,
                             DeploymentList &deploymentList,
                             QStringList& generatedDirs,
                             QStringList& generatedFiles)
{
    QString pluginStubDir = Option::output_dir + QLatin1Char('/') + QLatin1String(PLUGIN_STUB_DIR);
    QDir().mkpath(pluginStubDir);
    if (!generatedDirs.contains(pluginStubDir))
        generatedDirs << pluginStubDir;
    // Plugin stubs must have different name from the actual plugins, because
    // the toolchain for creating ROM images cannot handle non-binary .dll files properly.
    QFile stubFile(pluginStubDir + QLatin1Char('/') + info.completeBaseName() + QLatin1Char('.') + QLatin1String(SUFFIX_QTPLUGIN));
    if (stubFile.open(QIODevice::WriteOnly)) {
        if (!generatedFiles.contains(stubFile.fileName()))
            generatedFiles << stubFile.fileName();
        QTextStream t(&stubFile);
        // Add note to stub so that people will not wonder what it is.
        // Creation date is added to make new stub to deploy always to
        // force plugin cache miss when loading plugins.
        t << "This file is a Qt plugin stub file. The real Qt plugin is located in " SYSBIN_DIR ". Created:" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    } else {
        fprintf(stderr, "cannot deploy \"%s\" because of plugin stub file creation failed\n", info.fileName().toLatin1().constData());
    }
    QFileInfo stubInfo(stubFile);
    deploymentList.append(CopyItem(Option::fixPathToLocalOS(stubInfo.absoluteFilePath()),
                                   fixPathToEpocOS(devicePath + "/" + stubInfo.fileName())));
}

QString generate_uid(const QString& target)
{
    static QMap<QString, QString> targetToUid;

    QString tmp = targetToUid[target];

    if (!tmp.isEmpty()) {
        return tmp;
    }

    quint32 hash = 5381;
    int c;

    for (int i = 0; i < target.size(); ++i) {
        c = target.at(i).toAscii();
        hash ^= c + ((c - i) << i % 20) + ((c + i) << (i + 5) % 20) + ((c - 2 * i) << (i + 10) % 20) + ((c + 2 * i) << (i + 15) % 20);
    }

    tmp.setNum(hash, 16);
    for (int i = tmp.size(); i < 8; ++i)
        tmp.prepend("0");

    targetToUid[target] = tmp;

    return tmp;
}

// UIDs starting with 0xE are test UIDs in symbian
QString generate_test_uid(const QString& target)
{
    QString tmp = generate_uid(target);
    tmp.replace(0, 1, "E");
    tmp.prepend("0x");

    return tmp;
}


void initProjectDeploySymbian(QMakeProject* project,
                              DeploymentList &deploymentList,
                              const QString &testPath,
                              bool deployBinaries,
                              bool epocBuild,
                              const QString &platform,
                              const QString &build,
                              QStringList& generatedDirs,
                              QStringList& generatedFiles)
{
    QString targetPath = testPath;
    if (targetPath.endsWith("/") || targetPath.endsWith("\\"))
        targetPath = targetPath.mid(0, targetPath.size() - 1);

    bool targetPathHasDriveLetter = false;
    if (targetPath.size() > 1) {
        targetPathHasDriveLetter = targetPath.at(1) == QLatin1Char(':');
    }

    QString deploymentDrive;
    if (0 == platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))) {
        deploymentDrive = qt_epocRoot() + HW_Z_DIR;
    } else {
        deploymentDrive = targetPathHasDriveLetter ? targetPath.left(2) : QLatin1String("c:");
    }

    foreach(QString item, project->values("DEPLOYMENT")) {
        QString devicePath = project->first(item + ".path");
        QString devicePathWithoutDrive = devicePath;

        bool devicePathHasDriveLetter = false;
        if (devicePath.size() > 1) {
            devicePathHasDriveLetter = devicePath.at(1) == QLatin1Char(':');
        }

        // Sometimes devicePath can contain disk but APP_RESOURCE_DIR does not,
        // so remove the drive letter for comparison purposes.
        if (devicePathHasDriveLetter)
        {
            devicePathWithoutDrive.remove(0,2);
        }
        if (!deployBinaries
                && 0 != platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))
                && !devicePathWithoutDrive.isEmpty()
                && (0 == devicePathWithoutDrive.compare(project->values("APP_RESOURCE_DIR").join(""), Qt::CaseInsensitive)
                    || 0 == devicePathWithoutDrive.compare(project->values("REG_RESOURCE_IMPORT_DIR").join(""), Qt::CaseInsensitive))) {
            // Do not deploy resources in emulator builds, as that seems to cause conflicts
            // If there is ever a real need to deploy pre-built resources for emulator,
            // BLD_INF_RULES.prj_exports can be used as a workaround.
            continue;
        }

        if (devicePath.isEmpty() || devicePath == QLatin1String(".")) {
            devicePath = targetPath;
        }
        // check if item.path is relative (! either / or \)
        else if (!(devicePath.at(0) == QLatin1Char('/')
                   || devicePath.at(0) == QLatin1Char('\\')
                   || devicePathHasDriveLetter)) {
            // Create output path
            devicePath = Option::fixPathToLocalOS(QDir::cleanPath(targetPath + QLatin1Char('/') + devicePath));
        } else {
            if (0 == platform.compare(QLatin1String(EMULATOR_DEPLOYMENT_PLATFORM))) {
                if (devicePathHasDriveLetter) {
                    if (devicePath.startsWith("z", Qt::CaseInsensitive)) {
                        devicePath = qt_epocRoot() + "epoc32/release/winscw/" + build + "/z" + devicePath.remove(0, 2);
                    } else {
                        // Do not create deployment for urel target. Otherwise we would be generating two
                        // identical deployments for paths with drive letter other than Z.
                        if (0 == build.compare(QLatin1String("urel"), Qt::CaseInsensitive))
                            continue;
                        if (devicePath.startsWith("!"))
                            devicePath = qt_epocRoot() + "epoc32/winscw/c" + devicePath.remove(0, 2);
                        else
                            devicePath = qt_epocRoot() + "epoc32/winscw/" + devicePath.remove(1, 1);
                    }
                } else {
                    // If no device path drive letter, deploy to "emulator ROM drive"
                    devicePath = qt_epocRoot() + "epoc32/release/winscw/" + build + "/z" + devicePath;
                }
            } else {
                if (devicePathHasDriveLetter
                    && 0 == platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))) {
                    devicePath.remove(0,2);
                }
                if (0 == platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))
                    || (!devicePathHasDriveLetter && targetPathHasDriveLetter)) {
                    devicePath = deploymentDrive + devicePath;
                }
            }
        }

        devicePath.replace(QLatin1String("\\"), QLatin1String("/"));

        if (!deployBinaries
                && 0 == devicePath.right(8).compare(QLatin1String(SYSBIN_DIR), Qt::CaseInsensitive)
                && 0 != platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))) {
            // Skip deploying to SYSBIN_DIR for anything but binary deployments
            // Note: Deploying pre-built binaries also follow this rule, so emulator builds
            // will not get those deployed. Since there is no way to differentiate currently
            // between pre-built binaries for emulator and HW anyway, this is not a major issue.
            continue;
        }

        QStringList flags = project->values(item + ".flags");

        // ### Qt 5: remove .sources, inconsistent with INSTALLS
        foreach(QString source, project->values(item + ".sources") + project->values(item + ".files")) {
            source = Option::fixPathToLocalOS(source);
            QString nameFilter;
            QFileInfo info(source);
            QString searchPath;
            bool dirSearch = false;

            if (info.isDir()) {
                nameFilter = QLatin1String("*");
                searchPath = info.absoluteFilePath();
                dirSearch = true;
            } else {
                if (info.exists() || source.indexOf('*') != -1) {
                    nameFilter = source.split(QDir::separator()).last();
                    searchPath = info.absolutePath();
                } else {
                    // Entry was not found. That is ok if it is a binary, since those do not necessarily yet exist.
                    // Dlls need to be processed even when not deploying binaries for the stubs
                    if (isBinary(info)) {
                        if (deployBinaries) {
                            // Executables and libraries are deployed to \sys\bin
                            QFileInfo targetPath;
                            if (epocBuild)
                                targetPath.setFile(qt_epocRoot() + "epoc32/release/" + platform + "/" + build + "/");
                            else
                                targetPath.setFile(info.path() + QDir::separator());
                            if(devicePathHasDriveLetter) {
                                deploymentList.append(CopyItem(
                                    Option::fixPathToLocalOS(targetPath.absolutePath() + "/" + info.fileName(),
                                    false, true),
                                    fixPathToEpocOS(devicePath.left(2) + QLatin1String(SYSBIN_DIR "/")
                                    + info.fileName()),
                                    flags));
                            } else {
                                deploymentList.append(CopyItem(
                                    Option::fixPathToLocalOS(targetPath.absolutePath() + "/" + info.fileName(),
                                    false, true),
                                    fixPathToEpocOS(deploymentDrive + QLatin1String("/" SYSBIN_DIR "/")
                                    + info.fileName()),
                                    flags));
                            }
                        }
                        if (isPlugin(info, devicePath)) {
                            createPluginStub(info, devicePath, deploymentList, generatedDirs, generatedFiles);
                            continue;
                        }
                    } else {
                        // Generate deployment even if file doesn't exist, as this may be the case
                        // when generating .pkg files.
                        deploymentList.append(CopyItem(Option::fixPathToLocalOS(info.absoluteFilePath()),
                                                       fixPathToEpocOS(devicePath + "/" + info.fileName()),
                                                       flags));
                        continue;
                    }
                }
            }

            int pathSize = info.absolutePath().size();
            QDirIterator iterator(searchPath, QStringList() << nameFilter
                                  , QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks
                                  , dirSearch ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

            while (iterator.hasNext()) {
                iterator.next();
                QFileInfo iteratorInfo(iterator.filePath());
                QString absoluteItemPath = Option::fixPathToLocalOS(iteratorInfo.absolutePath());
                int diffSize = absoluteItemPath.size() - pathSize;

                if (!iteratorInfo.isDir()) {
                    if (isPlugin(iterator.fileInfo(), devicePath)) {
                        // This deploys pre-built plugins. Other pre-built binaries will deploy normally,
                        // as they have SYSBIN_DIR target path.
                        if (deployBinaries
                            || (0 == platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM)))) {
                            if (devicePathHasDriveLetter) {
                                deploymentList.append(CopyItem(
                                    Option::fixPathToLocalOS(absoluteItemPath + "/" + iterator.fileName()),
                                    fixPathToEpocOS(devicePath.left(2) + QLatin1String(SYSBIN_DIR "/")
                                    + iterator.fileName()),
                                    flags));
                            } else {
                                deploymentList.append(CopyItem(
                                    Option::fixPathToLocalOS(absoluteItemPath + "/" + iterator.fileName()),
                                    fixPathToEpocOS(deploymentDrive + QLatin1String("/" SYSBIN_DIR "/")
                                    + iterator.fileName()),
                                    flags));
                            }
                        }
                        createPluginStub(info, devicePath + "/" + absoluteItemPath.right(diffSize),
                            deploymentList, generatedDirs, generatedFiles);
                        continue;
                    } else {
                        deploymentList.append(CopyItem(
                            Option::fixPathToLocalOS(absoluteItemPath + "/" + iterator.fileName()),
                            fixPathToEpocOS(devicePath + "/" + absoluteItemPath.right(diffSize)
                            + "/" + iterator.fileName()),
                            flags));
                    }
                }
            }
        }
    }

    // Remove deployments that do not actually do anything
    if (0 == platform.compare(QLatin1String(EMULATOR_DEPLOYMENT_PLATFORM))
        || 0 == platform.compare(QLatin1String(ROM_DEPLOYMENT_PLATFORM))) {
        QMutableListIterator<CopyItem> i(deploymentList);
        while(i.hasNext()) {
            CopyItem &item = i.next();
            QFileInfo fromItem(item.from);
            QFileInfo toItem(item.to);
#if defined(Q_OS_WIN)
            if (0 == fromItem.absoluteFilePath().compare(toItem.absoluteFilePath(), Qt::CaseInsensitive))
#else
            if (0 == fromItem.absoluteFilePath().compare(toItem.absoluteFilePath()))
#endif
                i.remove();
        }
    }
}
