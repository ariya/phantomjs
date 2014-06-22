/*
 *  Copyright (C) 2007-2009 Torch Mobile Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "IconDatabase.h"

#include "DocumentLoader.h"
#include "FileSystem.h"
#include "IconDatabaseClient.h"
#include "IconRecord.h"
#include "Image.h"
#include <wtf/AutodrainedPool.h>
#include <wtf/text/CString.h>

namespace WebCore {

static IconDatabase* sharedIconDatabase = 0;

// Function to obtain the global icon database.
IconDatabase& iconDatabase()
{
    if (!sharedIconDatabase)
        sharedIconDatabase = new IconDatabase;
    return *sharedIconDatabase;
}

IconDatabase::IconDatabase() {}
IconDatabase::~IconDatabase() {}

void IconDatabase::setClient(IconDatabaseClient*) {}

bool IconDatabase::open(const String& path) { return false; }
void IconDatabase::close() {}

void IconDatabase::removeAllIcons() {}

Image* IconDatabase::iconForPageURL(const String&, const IntSize&) { return 0; }
void IconDatabase::readIconForPageURLFromDisk(const String&) {}
String IconDatabase::iconURLForPageURL(const String&) { return String(); }
Image* IconDatabase::defaultIcon(const IntSize&) { return 0;}

void IconDatabase::retainIconForPageURL(const String&) {}
void IconDatabase::releaseIconForPageURL(const String&) {}

void IconDatabase::setIconDataForIconURL(PassRefPtr<SharedBuffer> data, const String&) {}
void IconDatabase::setIconURLForPageURL(const String& iconURL, const String& pageURL) {}

IconLoadDecision IconDatabase::loadDecisionForIconURL(const String&, DocumentLoader*) { return IconLoadNo; }
bool IconDatabase::iconDataKnownForIconURL(const String&) { return false; }

void IconDatabase::setEnabled(bool enabled) {}
bool IconDatabase::isEnabled() const { return false; }

void IconDatabase::setPrivateBrowsingEnabled(bool flag) {}
bool IconDatabase::isPrivateBrowsingEnabled() const { return false; }

void IconDatabase::delayDatabaseCleanup() {}
void IconDatabase::allowDatabaseCleanup() {}
void IconDatabase::checkIntegrityBeforeOpening() {}

// Support for WebCoreStatistics in WebKit
size_t IconDatabase::pageURLMappingCount() { return 0; }
size_t IconDatabase::retainedPageURLCount() {return 0; }
size_t IconDatabase::iconRecordCount() { return 0; }
size_t IconDatabase::iconRecordCountWithData() { return 0; }

bool IconDatabase::isOpen() const { return false; }
String IconDatabase::databasePath() const { return String(); }
String IconDatabase::defaultDatabaseFilename() { return String(); }

} // namespace WebCore
