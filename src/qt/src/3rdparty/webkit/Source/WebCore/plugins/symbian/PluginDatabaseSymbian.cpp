/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "config.h"
#include "PluginDatabase.h"

#include <QFileInfo>
#include <f32file.h>

static const char QTPLUGIN_FILTER[] = "*.qtplugin";
static const char QT_PLUGIN_FOLDER[] = ":\\resource\\qt\\plugins\\npqtplugins\\";

namespace WebCore {

Vector<String> PluginDatabase::defaultPluginDirectories()
{
    Vector<String> directories;
    //find the installation drive
    TDriveList drivelist;
    TChar driveLetter;
    RFs fsSession;
    
    if (fsSession.Connect() == KErrNone && fsSession.DriveList(drivelist) == KErrNone) {
        for (TInt driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++) {
            if (drivelist[driveNumber] && fsSession.DriveToChar(driveNumber, driveLetter) == KErrNone) {
                QString driveStringValue(QChar((uint)driveLetter.GetUpperCase()));
                QString stubDirPath;
                stubDirPath.append(driveStringValue);
                stubDirPath.append(QT_PLUGIN_FOLDER);
                if (QFileInfo(stubDirPath).exists())
                    directories.append(stubDirPath);
            }
        }
    }

    fsSession.Close();
    return directories;
}

bool PluginDatabase::isPreferredPluginDirectory(const String& path)
{
    return true;
}

void PluginDatabase::getPluginPathsInDirectories(HashSet<String>& paths) const
{
    // FIXME: This should be a case insensitive set.
    HashSet<String> uniqueFilenames;

    String fileNameFilter(QTPLUGIN_FILTER);

    Vector<String>::const_iterator dirsEnd = m_pluginDirectories.end();
    for (Vector<String>::const_iterator dIt = m_pluginDirectories.begin(); dIt != dirsEnd; ++dIt) {
        Vector<String> pluginPaths = listDirectory(*dIt, fileNameFilter);
        Vector<String>::const_iterator pluginsEnd = pluginPaths.end();
        for (Vector<String>::const_iterator pIt = pluginPaths.begin(); pIt != pluginsEnd; ++pIt) {
            if (!fileExists(*pIt))
                continue;
            paths.add(*pIt);
        }
    }
}

}
