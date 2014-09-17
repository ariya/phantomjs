/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "IconDatabase.h"

#if ENABLE(ICONDATABASE)

#include "AutodrainedPool.h"
#include "DocumentLoader.h"
#include "FileSystem.h"
#include "IconDatabaseClient.h"
#include "IconRecord.h"
#include "IntSize.h"
#include "Logging.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include "SuddenTermination.h"
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

// For methods that are meant to support API from the main thread - should not be called internally
#define ASSERT_NOT_SYNC_THREAD() ASSERT(!m_syncThreadRunning || !IS_ICON_SYNC_THREAD())

// For methods that are meant to support the sync thread ONLY
#define IS_ICON_SYNC_THREAD() (m_syncThread == currentThread())
#define ASSERT_ICON_SYNC_THREAD() ASSERT(IS_ICON_SYNC_THREAD())

#if PLATFORM(QT) || PLATFORM(GTK)
#define CAN_THEME_URL_ICON
#endif

namespace WebCore {

static int databaseCleanupCounter = 0;

// This version number is in the DB and marks the current generation of the schema
// Currently, a mismatched schema causes the DB to be wiped and reset.  This isn't 
// so bad during development but in the future, we would need to write a conversion
// function to advance older released schemas to "current"
static const int currentDatabaseVersion = 6;

// Icons expire once every 4 days
static const int iconExpirationTime = 60*60*24*4; 

static const int updateTimerDelay = 5; 

static bool checkIntegrityOnOpen = false;

#ifndef NDEBUG
static String urlForLogging(const String& url)
{
    static unsigned urlTruncationLength = 120;

    if (url.length() < urlTruncationLength)
        return url;
    return url.substring(0, urlTruncationLength) + "...";
}
#endif

class DefaultIconDatabaseClient : public IconDatabaseClient {
public:
    virtual bool performImport() { return true; }
    virtual void didImportIconURLForPageURL(const String&) { } 
    virtual void didImportIconDataForPageURL(const String&) { }
    virtual void didChangeIconForPageURL(const String&) { }
    virtual void didRemoveAllIcons() { }
    virtual void didFinishURLImport() { }
};

static IconDatabaseClient* defaultClient() 
{
    static IconDatabaseClient* defaultClient = new DefaultIconDatabaseClient();
    return defaultClient;
}

static inline bool pageCanHaveIcon(const String& pageURL)
{
    return protocolIsInHTTPFamily(pageURL);
}

// ************************
// *** Main Thread Only ***
// ************************

void IconDatabase::setClient(IconDatabaseClient* client)
{
    // We don't allow a null client, because we never null check it anywhere in this code
    // Also don't allow a client change after the thread has already began 
    // (setting the client should occur before the database is opened)
    ASSERT(client);
    ASSERT(!m_syncThreadRunning);
    if (!client || m_syncThreadRunning)
        return;
        
    m_client = client;
}

bool IconDatabase::open(const String& directory, const String& filename)
{
    ASSERT_NOT_SYNC_THREAD();

    if (!m_isEnabled)
        return false;

    if (isOpen()) {
        LOG_ERROR("Attempt to reopen the IconDatabase which is already open.  Must close it first.");
        return false;
    }

    m_databaseDirectory = directory.crossThreadString();

    // Formulate the full path for the database file
    m_completeDatabasePath = pathByAppendingComponent(m_databaseDirectory, filename);

    // Lock here as well as first thing in the thread so the thread doesn't actually commence until the createThread() call 
    // completes and m_syncThreadRunning is properly set
    m_syncLock.lock();
    m_syncThread = createThread(IconDatabase::iconDatabaseSyncThreadStart, this, "WebCore: IconDatabase");
    m_syncThreadRunning = m_syncThread;
    m_syncLock.unlock();
    if (!m_syncThread)
        return false;
    return true;
}

void IconDatabase::close()
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (m_syncThreadRunning) {
        // Set the flag to tell the sync thread to wrap it up
        m_threadTerminationRequested = true;

        // Wake up the sync thread if it's waiting
        wakeSyncThread();
        
        // Wait for the sync thread to terminate
        waitForThreadCompletion(m_syncThread, 0);
    }

    m_syncThreadRunning = false;    
    m_threadTerminationRequested = false;
    m_removeIconsRequested = false;

    m_syncDB.close();
    ASSERT(!isOpen());
}

void IconDatabase::removeAllIcons()
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (!isOpen())
        return;

    LOG(IconDatabase, "Requesting background thread to remove all icons");
    
    // Clear the in-memory record of every IconRecord, anything waiting to be read from disk, and anything waiting to be written to disk
    {
        MutexLocker locker(m_urlAndIconLock);
        
        // Clear the IconRecords for every page URL - RefCounting will cause the IconRecords themselves to be deleted
        // We don't delete the actual PageRecords because we have the "retain icon for url" count to keep track of
        HashMap<String, PageURLRecord*>::iterator iter = m_pageURLToRecordMap.begin();
        HashMap<String, PageURLRecord*>::iterator end = m_pageURLToRecordMap.end();
        for (; iter != end; ++iter)
            (*iter).second->setIconRecord(0);
            
        // Clear the iconURL -> IconRecord map
        m_iconURLToRecordMap.clear();
                    
        // Clear all in-memory records of things that need to be synced out to disk
        {
            MutexLocker locker(m_pendingSyncLock);
            m_pageURLsPendingSync.clear();
            m_iconsPendingSync.clear();
        }
        
        // Clear all in-memory records of things that need to be read in from disk
        {
            MutexLocker locker(m_pendingReadingLock);
            m_pageURLsPendingImport.clear();
            m_pageURLsInterestedInIcons.clear();
            m_iconsPendingReading.clear();
            m_loadersPendingDecision.clear();
        }
    }
    
    m_removeIconsRequested = true;
    wakeSyncThread();
}

Image* IconDatabase::synchronousIconForPageURL(const String& pageURLOriginal, const IntSize& size)
{   
    ASSERT_NOT_SYNC_THREAD();

    // pageURLOriginal cannot be stored without being deep copied first.  
    // We should go our of our way to only copy it if we have to store it
    
    if (!isOpen() || !pageCanHaveIcon(pageURLOriginal))
        return defaultIcon(size);

    MutexLocker locker(m_urlAndIconLock);
    
    String pageURLCopy; // Creates a null string for easy testing
    
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    if (!pageRecord) {
        pageURLCopy = pageURLOriginal.crossThreadString();
        pageRecord = getOrCreatePageURLRecord(pageURLCopy);
    }
    
    // If pageRecord is NULL, one of two things is true -
    // 1 - The initial url import is incomplete and this pageURL was marked to be notified once it is complete if an iconURL exists
    // 2 - The initial url import IS complete and this pageURL has no icon
    if (!pageRecord) {
        MutexLocker locker(m_pendingReadingLock);
        
        // Import is ongoing, there might be an icon.  In this case, register to be notified when the icon comes in
        // If we ever reach this condition, we know we've already made the pageURL copy
        if (!m_iconURLImportComplete)
            m_pageURLsInterestedInIcons.add(pageURLCopy);
        
        return 0;
    }

    IconRecord* iconRecord = pageRecord->iconRecord();
    
    // If the initial URL import isn't complete, it's possible to have a PageURL record without an associated icon
    // In this case, the pageURL is already in the set to alert the client when the iconURL mapping is complete so
    // we can just bail now
    if (!m_iconURLImportComplete && !iconRecord)
        return 0;
    
    // The only way we should *not* have an icon record is if this pageURL is retained but has no icon yet - make sure of that
    ASSERT(iconRecord || m_retainedPageURLs.contains(pageURLOriginal));
    
    if (!iconRecord)
        return 0;
        
    // If it's a new IconRecord object that doesn't have its imageData set yet,
    // mark it to be read by the background thread
    if (iconRecord->imageDataStatus() == ImageDataStatusUnknown) {
        if (pageURLCopy.isNull())
            pageURLCopy = pageURLOriginal.crossThreadString();
    
        MutexLocker locker(m_pendingReadingLock);
        m_pageURLsInterestedInIcons.add(pageURLCopy);
        m_iconsPendingReading.add(iconRecord);
        wakeSyncThread();
        return 0;
    }
    
    // If the size parameter was (0, 0) that means the caller of this method just wanted the read from disk to be kicked off
    // and isn't actually interested in the image return value
    if (size == IntSize(0, 0))
        return 0;
        
    // PARANOID DISCUSSION: This method makes some assumptions.  It returns a WebCore::image which the icon database might dispose of at anytime in the future,
    // and Images aren't ref counted.  So there is no way for the client to guarantee continued existence of the image.
    // This has *always* been the case, but in practice clients would always create some other platform specific representation of the image
    // and drop the raw Image*.  On Mac an NSImage, and on windows drawing into an HBITMAP.
    // The async aspect adds a huge question - what if the image is deleted before the platform specific API has a chance to create its own
    // representation out of it?
    // If an image is read in from the icondatabase, we do *not* overwrite any image data that exists in the in-memory cache.  
    // This is because we make the assumption that anything in memory is newer than whatever is in the database.
    // So the only time the data will be set from the second thread is when it is INITIALLY being read in from the database, but we would never 
    // delete the image on the secondary thread if the image already exists.
    return iconRecord->image(size);
}

void IconDatabase::readIconForPageURLFromDisk(const String& pageURL)
{
    // The effect of asking for an Icon for a pageURL automatically queues it to be read from disk
    // if it hasn't already been set in memory.  The special IntSize (0, 0) is a special way of telling 
    // that method "I don't care about the actual Image, i just want you to make sure you're getting it from disk.
    synchronousIconForPageURL(pageURL, IntSize(0, 0));
}

String IconDatabase::synchronousIconURLForPageURL(const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD(); 
        
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    // Also, in the case we have a real answer for the caller, we must deep copy that as well
    
    if (!isOpen() || !pageCanHaveIcon(pageURLOriginal))
        return String();
        
    MutexLocker locker(m_urlAndIconLock);
    
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    if (!pageRecord)
        pageRecord = getOrCreatePageURLRecord(pageURLOriginal.crossThreadString());
    
    // If pageRecord is NULL, one of two things is true -
    // 1 - The initial url import is incomplete and this pageURL has already been marked to be notified once it is complete if an iconURL exists
    // 2 - The initial url import IS complete and this pageURL has no icon
    if (!pageRecord)
        return String();
    
    // Possible the pageRecord is around because it's a retained pageURL with no iconURL, so we have to check
    return pageRecord->iconRecord() ? pageRecord->iconRecord()->iconURL().threadsafeCopy() : String();
}

#ifdef CAN_THEME_URL_ICON
static inline void loadDefaultIconRecord(IconRecord* defaultIconRecord)
{
     defaultIconRecord->loadImageFromResource("urlIcon");
}
#else
static inline void loadDefaultIconRecord(IconRecord* defaultIconRecord)
{
    static const unsigned char defaultIconData[] = { 0x4D, 0x4D, 0x00, 0x2A, 0x00, 0x00, 0x03, 0x32, 0x80, 0x00, 0x20, 0x50, 0x38, 0x24, 0x16, 0x0D, 0x07, 0x84, 0x42, 0x61, 0x50, 0xB8, 
        0x64, 0x08, 0x18, 0x0D, 0x0A, 0x0B, 0x84, 0xA2, 0xA1, 0xE2, 0x08, 0x5E, 0x39, 0x28, 0xAF, 0x48, 0x24, 0xD3, 0x53, 0x9A, 0x37, 0x1D, 0x18, 0x0E, 0x8A, 0x4B, 0xD1, 0x38, 
        0xB0, 0x7C, 0x82, 0x07, 0x03, 0x82, 0xA2, 0xE8, 0x6C, 0x2C, 0x03, 0x2F, 0x02, 0x82, 0x41, 0xA1, 0xE2, 0xF8, 0xC8, 0x84, 0x68, 0x6D, 0x1C, 0x11, 0x0A, 0xB7, 0xFA, 0x91, 
        0x6E, 0xD1, 0x7F, 0xAF, 0x9A, 0x4E, 0x87, 0xFB, 0x19, 0xB0, 0xEA, 0x7F, 0xA4, 0x95, 0x8C, 0xB7, 0xF9, 0xA9, 0x0A, 0xA9, 0x7F, 0x8C, 0x88, 0x66, 0x96, 0xD4, 0xCA, 0x69, 
        0x2F, 0x00, 0x81, 0x65, 0xB0, 0x29, 0x90, 0x7C, 0xBA, 0x2B, 0x21, 0x1E, 0x5C, 0xE6, 0xB4, 0xBD, 0x31, 0xB6, 0xE7, 0x7A, 0xBF, 0xDD, 0x6F, 0x37, 0xD3, 0xFD, 0xD8, 0xF2, 
        0xB6, 0xDB, 0xED, 0xAC, 0xF7, 0x03, 0xC5, 0xFE, 0x77, 0x53, 0xB6, 0x1F, 0xE6, 0x24, 0x8B, 0x1D, 0xFE, 0x26, 0x20, 0x9E, 0x1C, 0xE0, 0x80, 0x65, 0x7A, 0x18, 0x02, 0x01, 
        0x82, 0xC5, 0xA0, 0xC0, 0xF1, 0x89, 0xBA, 0x23, 0x30, 0xAD, 0x1F, 0xE7, 0xE5, 0x5B, 0x6D, 0xFE, 0xE7, 0x78, 0x3E, 0x1F, 0xEE, 0x97, 0x8B, 0xE7, 0x37, 0x9D, 0xCF, 0xE7, 
        0x92, 0x8B, 0x87, 0x0B, 0xFC, 0xA0, 0x8E, 0x68, 0x3F, 0xC6, 0x27, 0xA6, 0x33, 0xFC, 0x36, 0x5B, 0x59, 0x3F, 0xC1, 0x02, 0x63, 0x3B, 0x74, 0x00, 0x03, 0x07, 0x0B, 0x61, 
        0x00, 0x20, 0x60, 0xC9, 0x08, 0x00, 0x1C, 0x25, 0x9F, 0xE0, 0x12, 0x8A, 0xD5, 0xFE, 0x6B, 0x4F, 0x35, 0x9F, 0xED, 0xD7, 0x4B, 0xD9, 0xFE, 0x8A, 0x59, 0xB8, 0x1F, 0xEC, 
        0x56, 0xD3, 0xC1, 0xFE, 0x63, 0x4D, 0xF2, 0x83, 0xC6, 0xB6, 0x1B, 0xFC, 0x34, 0x68, 0x61, 0x3F, 0xC1, 0xA6, 0x25, 0xEB, 0xFC, 0x06, 0x58, 0x5C, 0x3F, 0xC0, 0x03, 0xE4, 
        0xC3, 0xFC, 0x04, 0x0F, 0x1A, 0x6F, 0xE0, 0xE0, 0x20, 0xF9, 0x61, 0x7A, 0x02, 0x28, 0x2B, 0xBC, 0x46, 0x25, 0xF3, 0xFC, 0x66, 0x3D, 0x99, 0x27, 0xF9, 0x7E, 0x6B, 0x1D, 
        0xC7, 0xF9, 0x2C, 0x5E, 0x1C, 0x87, 0xF8, 0xC0, 0x4D, 0x9A, 0xE7, 0xF8, 0xDA, 0x51, 0xB2, 0xC1, 0x68, 0xF2, 0x64, 0x1F, 0xE1, 0x50, 0xED, 0x0A, 0x04, 0x23, 0x79, 0x8A, 
        0x7F, 0x82, 0xA3, 0x39, 0x80, 0x7F, 0x80, 0xC2, 0xB1, 0x5E, 0xF7, 0x04, 0x2F, 0xB2, 0x10, 0x02, 0x86, 0x63, 0xC9, 0xCC, 0x07, 0xBF, 0x87, 0xF8, 0x4A, 0x38, 0xAF, 0xC1, 
        0x88, 0xF8, 0x66, 0x1F, 0xE1, 0xD9, 0x08, 0xD4, 0x8F, 0x25, 0x5B, 0x4A, 0x49, 0x97, 0x87, 0x39, 0xFE, 0x25, 0x12, 0x10, 0x68, 0xAA, 0x4A, 0x2F, 0x42, 0x29, 0x12, 0x69, 
        0x9F, 0xE1, 0xC1, 0x00, 0x67, 0x1F, 0xE1, 0x58, 0xED, 0x00, 0x83, 0x23, 0x49, 0x82, 0x7F, 0x81, 0x21, 0xE0, 0xFC, 0x73, 0x21, 0x00, 0x50, 0x7D, 0x2B, 0x84, 0x03, 0x83, 
        0xC2, 0x1B, 0x90, 0x06, 0x69, 0xFE, 0x23, 0x91, 0xAE, 0x50, 0x9A, 0x49, 0x32, 0xC2, 0x89, 0x30, 0xE9, 0x0A, 0xC4, 0xD9, 0xC4, 0x7F, 0x94, 0xA6, 0x51, 0xDE, 0x7F, 0x9D, 
        0x07, 0x89, 0xF6, 0x7F, 0x91, 0x85, 0xCA, 0x88, 0x25, 0x11, 0xEE, 0x50, 0x7C, 0x43, 0x35, 0x21, 0x60, 0xF1, 0x0D, 0x82, 0x62, 0x39, 0x07, 0x2C, 0x20, 0xE0, 0x80, 0x72, 
        0x34, 0x17, 0xA1, 0x80, 0xEE, 0xF0, 0x89, 0x24, 0x74, 0x1A, 0x2C, 0x93, 0xB3, 0x78, 0xCC, 0x52, 0x9D, 0x6A, 0x69, 0x56, 0xBB, 0x0D, 0x85, 0x69, 0xE6, 0x7F, 0x9E, 0x27, 
        0xB9, 0xFD, 0x50, 0x54, 0x47, 0xF9, 0xCC, 0x78, 0x9F, 0x87, 0xF9, 0x98, 0x70, 0xB9, 0xC2, 0x91, 0x2C, 0x6D, 0x1F, 0xE1, 0xE1, 0x00, 0xBF, 0x02, 0xC1, 0xF5, 0x18, 0x84,
        0x01, 0xE1, 0x48, 0x8C, 0x42, 0x07, 0x43, 0xC9, 0x76, 0x7F, 0x8B, 0x04, 0xE4, 0xDE, 0x35, 0x95, 0xAB, 0xB0, 0xF0, 0x5C, 0x55, 0x23, 0xF9, 0x7E, 0x7E, 0x9F, 0xE4, 0x0C, 
        0xA7, 0x55, 0x47, 0xC7, 0xF9, 0xE6, 0xCF, 0x1F, 0xE7, 0x93, 0x35, 0x52, 0x54, 0x63, 0x19, 0x46, 0x73, 0x1F, 0xE2, 0x61, 0x08, 0xF0, 0x82, 0xE1, 0x80, 0x92, 0xF9, 0x20, 
        0xC0, 0x28, 0x18, 0x0A, 0x05, 0xA1, 0xA2, 0xF8, 0x6E, 0xDB, 0x47, 0x49, 0xFE, 0x3E, 0x17, 0xB6, 0x61, 0x13, 0x1A, 0x29, 0x26, 0xA9, 0xFE, 0x7F, 0x92, 0x70, 0x69, 0xFE, 
        0x4C, 0x2F, 0x55, 0x01, 0xF1, 0x54, 0xD4, 0x35, 0x49, 0x4A, 0x69, 0x59, 0x83, 0x81, 0x58, 0x76, 0x9F, 0xE2, 0x20, 0xD6, 0x4C, 0x9B, 0xA0, 0x48, 0x1E, 0x0B, 0xB7, 0x48, 
        0x58, 0x26, 0x11, 0x06, 0x42, 0xE8, 0xA4, 0x40, 0x17, 0x27, 0x39, 0x00, 0x60, 0x2D, 0xA4, 0xC3, 0x2C, 0x7F, 0x94, 0x56, 0xE4, 0xE1, 0x77, 0x1F, 0xE5, 0xB9, 0xD7, 0x66, 
        0x1E, 0x07, 0xB3, 0x3C, 0x63, 0x1D, 0x35, 0x49, 0x0E, 0x63, 0x2D, 0xA2, 0xF1, 0x12, 0x60, 0x1C, 0xE0, 0xE0, 0x52, 0x1B, 0x8B, 0xAC, 0x38, 0x0E, 0x07, 0x03, 0x60, 0x28, 
        0x1C, 0x0E, 0x87, 0x00, 0xF0, 0x66, 0x27, 0x11, 0xA2, 0xC1, 0x02, 0x5A, 0x1C, 0xE4, 0x21, 0x83, 0x1F, 0x13, 0x86, 0xFA, 0xD2, 0x55, 0x1D, 0xD6, 0x61, 0xBC, 0x77, 0xD3, 
        0xE6, 0x91, 0xCB, 0x4C, 0x90, 0xA6, 0x25, 0xB8, 0x2F, 0x90, 0xC5, 0xA9, 0xCE, 0x12, 0x07, 0x02, 0x91, 0x1B, 0x9F, 0x68, 0x00, 0x16, 0x76, 0x0D, 0xA1, 0x00, 0x08, 0x06, 
        0x03, 0x81, 0xA0, 0x20, 0x1A, 0x0D, 0x06, 0x80, 0x30, 0x24, 0x12, 0x89, 0x20, 0x98, 0x4A, 0x1F, 0x0F, 0x21, 0xA0, 0x9E, 0x36, 0x16, 0xC2, 0x88, 0xE6, 0x48, 0x9B, 0x83, 
        0x31, 0x1C, 0x55, 0x1E, 0x43, 0x59, 0x1A, 0x56, 0x1E, 0x42, 0xF0, 0xFA, 0x4D, 0x1B, 0x9B, 0x08, 0xDC, 0x5B, 0x02, 0xA1, 0x30, 0x7E, 0x3C, 0xEE, 0x5B, 0xA6, 0xDD, 0xB8, 
        0x6D, 0x5B, 0x62, 0xB7, 0xCD, 0xF3, 0x9C, 0xEA, 0x04, 0x80, 0x80, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x01, 0x01, 
        0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x03, 0xE0, 0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 
        0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x01, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x01, 0x11, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
        0x00, 0x08, 0x01, 0x15, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x01, 0x16, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x01, 0x17, 
        0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x29, 0x01, 0x1A, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0xE8, 0x01, 0x1B, 0x00, 0x05, 0x00, 0x00, 
        0x00, 0x01, 0x00, 0x00, 0x03, 0xF0, 0x01, 0x1C, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x28, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 
        0x00, 0x00, 0x01, 0x52, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x0A, 
        0xFC, 0x80, 0x00, 0x00, 0x27, 0x10, 0x00, 0x0A, 0xFC, 0x80, 0x00, 0x00, 0x27, 0x10 };
        
    DEFINE_STATIC_LOCAL(RefPtr<SharedBuffer>, defaultIconBuffer, (SharedBuffer::create(defaultIconData, sizeof(defaultIconData))));
    defaultIconRecord->setImageData(defaultIconBuffer);
}
#endif

Image* IconDatabase::defaultIcon(const IntSize& size)
{
    ASSERT_NOT_SYNC_THREAD();

    
    if (!m_defaultIconRecord) {
        m_defaultIconRecord = IconRecord::create("urlIcon");
        loadDefaultIconRecord(m_defaultIconRecord.get());
    }
    
    return m_defaultIconRecord->image(size);
}


void IconDatabase::retainIconForPageURL(const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();
    
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    
    if (!isEnabled() || !pageCanHaveIcon(pageURLOriginal))
        return;
       
    MutexLocker locker(m_urlAndIconLock);

    PageURLRecord* record = m_pageURLToRecordMap.get(pageURLOriginal);
    
    String pageURL;
    
    if (!record) {
        pageURL = pageURLOriginal.crossThreadString();

        record = new PageURLRecord(pageURL);
        m_pageURLToRecordMap.set(pageURL, record);
    }
    
    if (!record->retain()) {
        if (pageURL.isNull())
            pageURL = pageURLOriginal.crossThreadString();

        // This page just had its retain count bumped from 0 to 1 - Record that fact
        m_retainedPageURLs.add(pageURL);

        // If we read the iconURLs yet, we want to avoid any pageURL->iconURL lookups and the pageURLsPendingDeletion is moot, 
        // so we bail here and skip those steps
        if (!m_iconURLImportComplete)
            return;

        MutexLocker locker(m_pendingSyncLock);
        // If this pageURL waiting to be sync'ed, update the sync record
        // This saves us in the case where a page was ready to be deleted from the database but was just retained - so theres no need to delete it!
        if (!m_privateBrowsingEnabled && m_pageURLsPendingSync.contains(pageURL)) {
            LOG(IconDatabase, "Bringing %s back from the brink", pageURL.ascii().data());
            m_pageURLsPendingSync.set(pageURL, record->snapshot());
        }
    }
}

void IconDatabase::releaseIconForPageURL(const String& pageURLOriginal)
{
    ASSERT_NOT_SYNC_THREAD();
        
    // Cannot do anything with pageURLOriginal that would end up storing it without deep copying first
    
    if (!isEnabled() || !pageCanHaveIcon(pageURLOriginal))
        return;
    
    MutexLocker locker(m_urlAndIconLock);

    // Check if this pageURL is actually retained
    if (!m_retainedPageURLs.contains(pageURLOriginal)) {
        LOG_ERROR("Attempting to release icon for URL %s which is not retained", urlForLogging(pageURLOriginal).ascii().data());
        return;
    }
    
    // Get its retain count - if it's retained, we'd better have a PageURLRecord for it
    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
    ASSERT(pageRecord);
    LOG(IconDatabase, "Releasing pageURL %s to a retain count of %i", urlForLogging(pageURLOriginal).ascii().data(), pageRecord->retainCount() - 1);
    ASSERT(pageRecord->retainCount() > 0);
        
    // If it still has a positive retain count, store the new count and bail
    if (pageRecord->release())
        return;
        
    // This pageRecord has now been fully released.  Do the appropriate cleanup
    LOG(IconDatabase, "No more retainers for PageURL %s", urlForLogging(pageURLOriginal).ascii().data());
    m_pageURLToRecordMap.remove(pageURLOriginal);
    m_retainedPageURLs.remove(pageURLOriginal);       
    
    // Grab the iconRecord for later use (and do a sanity check on it for kicks)
    IconRecord* iconRecord = pageRecord->iconRecord();
    
    ASSERT(!iconRecord || (iconRecord && m_iconURLToRecordMap.get(iconRecord->iconURL()) == iconRecord));

    {
        MutexLocker locker(m_pendingReadingLock);
        
        // Since this pageURL is going away, there's no reason anyone would ever be interested in its read results    
        if (!m_iconURLImportComplete)
            m_pageURLsPendingImport.remove(pageURLOriginal);
        m_pageURLsInterestedInIcons.remove(pageURLOriginal);
        
        // If this icon is down to it's last retainer, we don't care about reading it in from disk anymore
        if (iconRecord && iconRecord->hasOneRef()) {
            m_iconURLToRecordMap.remove(iconRecord->iconURL());
            m_iconsPendingReading.remove(iconRecord);
        }
    }
    
    // Mark stuff for deletion from the database only if we're not in private browsing
    if (!m_privateBrowsingEnabled) {
        MutexLocker locker(m_pendingSyncLock);
        m_pageURLsPendingSync.set(pageURLOriginal.crossThreadString(), pageRecord->snapshot(true));
    
        // If this page is the last page to refer to a particular IconRecord, that IconRecord needs to
        // be marked for deletion
        if (iconRecord && iconRecord->hasOneRef())
            m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));
    }
    
    delete pageRecord;

    if (isOpen())
        scheduleOrDeferSyncTimer();
}

void IconDatabase::setIconDataForIconURL(PassRefPtr<SharedBuffer> dataOriginal, const String& iconURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();
    
    // Cannot do anything with dataOriginal or iconURLOriginal that would end up storing them without deep copying first
    
    if (!isOpen() || iconURLOriginal.isEmpty())
        return;
    
    RefPtr<SharedBuffer> data = dataOriginal ? dataOriginal->copy() : PassRefPtr<SharedBuffer>(0);
    String iconURL = iconURLOriginal.crossThreadString();
    
    Vector<String> pageURLs;
    {
        MutexLocker locker(m_urlAndIconLock);
    
        // If this icon was pending a read, remove it from that set because this new data should override what is on disk
        RefPtr<IconRecord> icon = m_iconURLToRecordMap.get(iconURL);
        if (icon) {
            MutexLocker locker(m_pendingReadingLock);
            m_iconsPendingReading.remove(icon.get());
        } else
            icon = getOrCreateIconRecord(iconURL);
    
        // Update the data and set the time stamp
        icon->setImageData(data.release());
        icon->setTimestamp((int)currentTime());
        
        // Copy the current retaining pageURLs - if any - to notify them of the change
        pageURLs.appendRange(icon->retainingPageURLs().begin(), icon->retainingPageURLs().end());
        
        // Mark the IconRecord as requiring an update to the database only if private browsing is disabled
        if (!m_privateBrowsingEnabled) {
            MutexLocker locker(m_pendingSyncLock);
            m_iconsPendingSync.set(iconURL, icon->snapshot());
        }

        if (icon->hasOneRef()) {
            ASSERT(icon->retainingPageURLs().isEmpty());
            LOG(IconDatabase, "Icon for icon url %s is about to be destroyed - removing mapping for it", urlForLogging(icon->iconURL()).ascii().data());
            m_iconURLToRecordMap.remove(icon->iconURL());
        }
    }

    // Send notification out regarding all PageURLs that retain this icon
    // But not if we're on the sync thread because that implies this mapping
    // comes from the initial import which we don't want notifications for
    if (!IS_ICON_SYNC_THREAD()) {
        // Start the timer to commit this change - or further delay the timer if it was already started
        scheduleOrDeferSyncTimer();

        // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
        // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up        
        AutodrainedPool pool(25);

        for (unsigned i = 0; i < pageURLs.size(); ++i) {
            LOG(IconDatabase, "Dispatching notification that retaining pageURL %s has a new icon", urlForLogging(pageURLs[i]).ascii().data());
            m_client->didChangeIconForPageURL(pageURLs[i]);

            pool.cycle();
        }
    }
}

void IconDatabase::setIconURLForPageURL(const String& iconURLOriginal, const String& pageURLOriginal)
{    
    ASSERT_NOT_SYNC_THREAD();

    // Cannot do anything with iconURLOriginal or pageURLOriginal that would end up storing them without deep copying first
    
    ASSERT(!iconURLOriginal.isEmpty());
        
    if (!isOpen() || !pageCanHaveIcon(pageURLOriginal))
        return;
    
    String iconURL, pageURL;
    
    {
        MutexLocker locker(m_urlAndIconLock);

        PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURLOriginal);
        
        // If the urls already map to each other, bail.
        // This happens surprisingly often, and seems to cream iBench performance
        if (pageRecord && pageRecord->iconRecord() && pageRecord->iconRecord()->iconURL() == iconURLOriginal)
            return;
            
        pageURL = pageURLOriginal.crossThreadString();
        iconURL = iconURLOriginal.crossThreadString();

        if (!pageRecord) {
            pageRecord = new PageURLRecord(pageURL);
            m_pageURLToRecordMap.set(pageURL, pageRecord);
        }

        RefPtr<IconRecord> iconRecord = pageRecord->iconRecord();

        // Otherwise, set the new icon record for this page
        pageRecord->setIconRecord(getOrCreateIconRecord(iconURL));

        // If the current icon has only a single ref left, it is about to get wiped out. 
        // Remove it from the in-memory records and don't bother reading it in from disk anymore
        if (iconRecord && iconRecord->hasOneRef()) {
            ASSERT(iconRecord->retainingPageURLs().size() == 0);
            LOG(IconDatabase, "Icon for icon url %s is about to be destroyed - removing mapping for it", urlForLogging(iconRecord->iconURL()).ascii().data());
            m_iconURLToRecordMap.remove(iconRecord->iconURL());
            MutexLocker locker(m_pendingReadingLock);
            m_iconsPendingReading.remove(iconRecord.get());
        }
        
        // And mark this mapping to be added to the database
        if (!m_privateBrowsingEnabled) {
            MutexLocker locker(m_pendingSyncLock);
            m_pageURLsPendingSync.set(pageURL, pageRecord->snapshot());
            
            // If the icon is on its last ref, mark it for deletion
            if (iconRecord && iconRecord->hasOneRef())
                m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));
        }
    }

    // Since this mapping is new, send the notification out - but not if we're on the sync thread because that implies this mapping
    // comes from the initial import which we don't want notifications for
    if (!IS_ICON_SYNC_THREAD()) {
        // Start the timer to commit this change - or further delay the timer if it was already started
        scheduleOrDeferSyncTimer();
        
        LOG(IconDatabase, "Dispatching notification that we changed an icon mapping for url %s", urlForLogging(pageURL).ascii().data());
        AutodrainedPool pool;
        m_client->didChangeIconForPageURL(pageURL);
    }
}

IconLoadDecision IconDatabase::synchronousLoadDecisionForIconURL(const String& iconURL, DocumentLoader* notificationDocumentLoader)
{
    ASSERT_NOT_SYNC_THREAD();

    if (!isOpen() || iconURL.isEmpty())
        return IconLoadNo;
    
    // If we have a IconRecord, it should also have its timeStamp marked because there is only two times when we create the IconRecord:
    // 1 - When we read the icon urls from disk, getting the timeStamp at the same time
    // 2 - When we get a new icon from the loader, in which case the timestamp is set at that time
    {
        MutexLocker locker(m_urlAndIconLock);
        if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL)) {
            LOG(IconDatabase, "Found expiration time on a present icon based on existing IconRecord");
            return (int)currentTime() - icon->getTimestamp() > iconExpirationTime ? IconLoadYes : IconLoadNo;
        }
    }
    
    // If we don't have a record for it, but we *have* imported all iconURLs from disk, then we should load it now
    MutexLocker readingLocker(m_pendingReadingLock);
    if (m_iconURLImportComplete)
        return IconLoadYes;
        
    // Otherwise - since we refuse to perform I/O on the main thread to find out for sure - we return the answer that says
    // "You might be asked to load this later, so flag that"
    LOG(IconDatabase, "Don't know if we should load %s or not - adding %p to the set of document loaders waiting on a decision", iconURL.ascii().data(), notificationDocumentLoader);
    if (notificationDocumentLoader)
        m_loadersPendingDecision.add(notificationDocumentLoader);    

    return IconLoadUnknown;
}

bool IconDatabase::synchronousIconDataKnownForIconURL(const String& iconURL)
{
    ASSERT_NOT_SYNC_THREAD();
    
    MutexLocker locker(m_urlAndIconLock);
    if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL))
        return icon->imageDataStatus() != ImageDataStatusUnknown;

    return false;
}

void IconDatabase::setEnabled(bool enabled)
{
    ASSERT_NOT_SYNC_THREAD();
    
    if (!enabled && isOpen())
        close();
    m_isEnabled = enabled;
}

bool IconDatabase::isEnabled() const
{
    ASSERT_NOT_SYNC_THREAD();
    
     return m_isEnabled;
}

void IconDatabase::setPrivateBrowsingEnabled(bool flag)
{
    m_privateBrowsingEnabled = flag;
}

bool IconDatabase::isPrivateBrowsingEnabled() const
{
    return m_privateBrowsingEnabled;
}

void IconDatabase::delayDatabaseCleanup()
{
    ++databaseCleanupCounter;
    if (databaseCleanupCounter == 1)
        LOG(IconDatabase, "Database cleanup is now DISABLED");
}

void IconDatabase::allowDatabaseCleanup()
{
    if (--databaseCleanupCounter < 0)
        databaseCleanupCounter = 0;
    if (databaseCleanupCounter == 0)
        LOG(IconDatabase, "Database cleanup is now ENABLED");
}

void IconDatabase::checkIntegrityBeforeOpening()
{
    checkIntegrityOnOpen = true;
}

size_t IconDatabase::pageURLMappingCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_pageURLToRecordMap.size();
}

size_t IconDatabase::retainedPageURLCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_retainedPageURLs.size();
}

size_t IconDatabase::iconRecordCount()
{
    MutexLocker locker(m_urlAndIconLock);
    return m_iconURLToRecordMap.size();
}

size_t IconDatabase::iconRecordCountWithData()
{
    MutexLocker locker(m_urlAndIconLock);
    size_t result = 0;
    
    HashMap<String, IconRecord*>::iterator i = m_iconURLToRecordMap.begin();
    HashMap<String, IconRecord*>::iterator end = m_iconURLToRecordMap.end();
    
    for (; i != end; ++i)
        result += ((*i).second->imageDataStatus() == ImageDataStatusPresent);
            
    return result;
}

IconDatabase::IconDatabase()
    : m_syncTimer(this, &IconDatabase::syncTimerFired)
    , m_syncThreadRunning(false)
    , m_isEnabled(false)
    , m_privateBrowsingEnabled(false)
    , m_threadTerminationRequested(false)
    , m_removeIconsRequested(false)
    , m_iconURLImportComplete(false)
    , m_disabledSuddenTerminationForSyncThread(false)
    , m_initialPruningComplete(false)
    , m_client(defaultClient())
    , m_imported(false)
    , m_isImportedSet(false)
{
    LOG(IconDatabase, "Creating IconDatabase %p", this);
    ASSERT(isMainThread());
}

IconDatabase::~IconDatabase()
{
    ASSERT_NOT_REACHED();
}

void IconDatabase::notifyPendingLoadDecisionsOnMainThread(void* context)
{
    static_cast<IconDatabase*>(context)->notifyPendingLoadDecisions();
}

void IconDatabase::notifyPendingLoadDecisions()
{
    ASSERT_NOT_SYNC_THREAD();
    
    // This method should only be called upon completion of the initial url import from the database
    ASSERT(m_iconURLImportComplete);
    LOG(IconDatabase, "Notifying all DocumentLoaders that were waiting on a load decision for thier icons");
    
    HashSet<RefPtr<DocumentLoader> >::iterator i = m_loadersPendingDecision.begin();
    HashSet<RefPtr<DocumentLoader> >::iterator end = m_loadersPendingDecision.end();
    
    for (; i != end; ++i)
        if ((*i)->refCount() > 1)
            (*i)->iconLoadDecisionAvailable();
            
    m_loadersPendingDecision.clear();
}

void IconDatabase::wakeSyncThread()
{
    MutexLocker locker(m_syncLock);

    if (!m_disabledSuddenTerminationForSyncThread) {
        m_disabledSuddenTerminationForSyncThread = true;
        // The following is balanced by the call to enableSuddenTermination in the
        // syncThreadMainLoop function.
        // FIXME: It would be better to only disable sudden termination if we have
        // something to write, not just if we have something to read.
        disableSuddenTermination();
    }

    m_syncCondition.signal();
}

void IconDatabase::scheduleOrDeferSyncTimer()
{
    ASSERT_NOT_SYNC_THREAD();

    if (!m_syncTimer.isActive()) {
        // The following is balanced by the call to enableSuddenTermination in the
        // syncTimerFired function.
        disableSuddenTermination();
    }

    m_syncTimer.startOneShot(updateTimerDelay);
}

void IconDatabase::syncTimerFired(Timer<IconDatabase>*)
{
    ASSERT_NOT_SYNC_THREAD();
    wakeSyncThread();

    // The following is balanced by the call to disableSuddenTermination in the
    // scheduleOrDeferSyncTimer function.
    enableSuddenTermination();
}

// ******************
// *** Any Thread ***
// ******************

bool IconDatabase::isOpen() const
{
    MutexLocker locker(m_syncLock);
    return m_syncDB.isOpen();
}

String IconDatabase::databasePath() const
{
    MutexLocker locker(m_syncLock);
    return m_completeDatabasePath.threadsafeCopy();
}

String IconDatabase::defaultDatabaseFilename()
{
    DEFINE_STATIC_LOCAL(String, defaultDatabaseFilename, ("WebpageIcons.db"));
    return defaultDatabaseFilename.threadsafeCopy();
}

// Unlike getOrCreatePageURLRecord(), getOrCreateIconRecord() does not mark the icon as "interested in import"
PassRefPtr<IconRecord> IconDatabase::getOrCreateIconRecord(const String& iconURL)
{
    // Clients of getOrCreateIconRecord() are required to acquire the m_urlAndIconLock before calling this method
    ASSERT(!m_urlAndIconLock.tryLock());

    if (IconRecord* icon = m_iconURLToRecordMap.get(iconURL))
        return icon;

    RefPtr<IconRecord> newIcon = IconRecord::create(iconURL);
    m_iconURLToRecordMap.set(iconURL, newIcon.get());

    return newIcon.release();
}

// This method retrieves the existing PageURLRecord, or creates a new one and marks it as "interested in the import" for later notification
PageURLRecord* IconDatabase::getOrCreatePageURLRecord(const String& pageURL)
{
    // Clients of getOrCreatePageURLRecord() are required to acquire the m_urlAndIconLock before calling this method
    ASSERT(!m_urlAndIconLock.tryLock());

    if (!pageCanHaveIcon(pageURL))
        return 0;

    PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURL);
    
    MutexLocker locker(m_pendingReadingLock);
    if (!m_iconURLImportComplete) {
        // If the initial import of all URLs hasn't completed and we have no page record, we assume we *might* know about this later and create a record for it
        if (!pageRecord) {
            LOG(IconDatabase, "Creating new PageURLRecord for pageURL %s", urlForLogging(pageURL).ascii().data());
            pageRecord = new PageURLRecord(pageURL);
            m_pageURLToRecordMap.set(pageURL, pageRecord);
        }

        // If the pageRecord for this page does not have an iconRecord attached to it, then it is a new pageRecord still awaiting the initial import
        // Mark the URL as "interested in the result of the import" then bail
        if (!pageRecord->iconRecord()) {
            m_pageURLsPendingImport.add(pageURL);
            return 0;
        }
    }

    // We've done the initial import of all URLs known in the database.  If this record doesn't exist now, it never will    
     return pageRecord;
}


// ************************
// *** Sync Thread Only ***
// ************************

void IconDatabase::importIconURLForPageURL(const String& iconURL, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    // This function is only for setting actual existing url mappings so assert that neither of these URLs are empty
    ASSERT(!iconURL.isEmpty());
    ASSERT(!pageURL.isEmpty());
    ASSERT(pageCanHaveIcon(pageURL));
    
    setIconURLForPageURLInSQLDatabase(iconURL, pageURL);    
}

void IconDatabase::importIconDataForIconURL(PassRefPtr<SharedBuffer> data, const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    ASSERT(!iconURL.isEmpty());

    writeIconSnapshotToSQLDatabase(IconSnapshot(iconURL, (int)currentTime(), data.get()));
}

bool IconDatabase::shouldStopThreadActivity() const
{
    ASSERT_ICON_SYNC_THREAD();
    
    return m_threadTerminationRequested || m_removeIconsRequested;
}

void* IconDatabase::iconDatabaseSyncThreadStart(void* vIconDatabase)
{    
    IconDatabase* iconDB = static_cast<IconDatabase*>(vIconDatabase);
    
    return iconDB->iconDatabaseSyncThread();
}

void* IconDatabase::iconDatabaseSyncThread()
{
    // The call to create this thread might not complete before the thread actually starts, so we might fail this ASSERT_ICON_SYNC_THREAD() because the pointer 
    // to our thread structure hasn't been filled in yet.
    // To fix this, the main thread acquires this lock before creating us, then releases the lock after creation is complete.  A quick lock/unlock cycle here will 
    // prevent us from running before that call completes
    m_syncLock.lock();
    m_syncLock.unlock();

    ASSERT_ICON_SYNC_THREAD();
    
    LOG(IconDatabase, "(THREAD) IconDatabase sync thread started");

#ifndef NDEBUG
    double startTime = currentTime();
#endif

    // Need to create the database path if it doesn't already exist
    makeAllDirectories(m_databaseDirectory);

    // Existence of a journal file is evidence of a previous crash/force quit and automatically qualifies
    // us to do an integrity check
    String journalFilename = m_completeDatabasePath + "-journal";
    if (!checkIntegrityOnOpen) {
        AutodrainedPool pool;
        checkIntegrityOnOpen = fileExists(journalFilename);
    }
    
    {
        MutexLocker locker(m_syncLock);
        if (!m_syncDB.open(m_completeDatabasePath)) {
            LOG_ERROR("Unable to open icon database at path %s - %s", m_completeDatabasePath.ascii().data(), m_syncDB.lastErrorMsg());
            return 0;
        }
    }
    
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();
        
#ifndef NDEBUG
    double timeStamp = currentTime();
    LOG(IconDatabase, "(THREAD) Open took %.4f seconds", timeStamp - startTime);
#endif    

    performOpenInitialization();
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();
        
#ifndef NDEBUG
    double newStamp = currentTime();
    LOG(IconDatabase, "(THREAD) performOpenInitialization() took %.4f seconds, now %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
    timeStamp = newStamp;
#endif 

    if (!imported()) {
        LOG(IconDatabase, "(THREAD) Performing Safari2 import procedure");
        SQLiteTransaction importTransaction(m_syncDB);
        importTransaction.begin();
        
        // Commit the transaction only if the import completes (the import should be atomic)
        if (m_client->performImport()) {
            setImported(true);
            importTransaction.commit();
        } else {
            LOG(IconDatabase, "(THREAD) Safari 2 import was cancelled");
            importTransaction.rollback();
        }
        
        if (shouldStopThreadActivity())
            return syncThreadMainLoop();
            
#ifndef NDEBUG
        newStamp = currentTime();
        LOG(IconDatabase, "(THREAD) performImport() took %.4f seconds, now %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
        timeStamp = newStamp;
#endif 
    }
        
    // Uncomment the following line to simulate a long lasting URL import (*HUGE* icon databases, or network home directories)
    // while (currentTime() - timeStamp < 10);

    // Read in URL mappings from the database          
    LOG(IconDatabase, "(THREAD) Starting iconURL import");
    performURLImport();
    
    if (shouldStopThreadActivity())
        return syncThreadMainLoop();

#ifndef NDEBUG
    newStamp = currentTime();
    LOG(IconDatabase, "(THREAD) performURLImport() took %.4f seconds.  Entering main loop %.4f seconds from thread start", newStamp - timeStamp, newStamp - startTime);
#endif 

    LOG(IconDatabase, "(THREAD) Beginning sync");
    return syncThreadMainLoop();
}

static int databaseVersionNumber(SQLiteDatabase& db)
{
    return SQLiteStatement(db, "SELECT value FROM IconDatabaseInfo WHERE key = 'Version';").getColumnInt(0);
}

static bool isValidDatabase(SQLiteDatabase& db)
{
    // These four tables should always exist in a valid db
    if (!db.tableExists("IconInfo") || !db.tableExists("IconData") || !db.tableExists("PageURL") || !db.tableExists("IconDatabaseInfo"))
        return false;
    
    if (databaseVersionNumber(db) < currentDatabaseVersion) {
        LOG(IconDatabase, "DB version is not found or below expected valid version");
        return false;
    }
    
    return true;
}

static void createDatabaseTables(SQLiteDatabase& db)
{
    if (!db.executeCommand("CREATE TABLE PageURL (url TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT REPLACE,iconID INTEGER NOT NULL ON CONFLICT FAIL);")) {
        LOG_ERROR("Could not create PageURL table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX PageURLIndex ON PageURL (url);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconInfo (iconID INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE ON CONFLICT REPLACE, url TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT FAIL, stamp INTEGER);")) {
        LOG_ERROR("Could not create IconInfo table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX IconInfoIndex ON IconInfo (url, iconID);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconData (iconID INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE ON CONFLICT REPLACE, data BLOB);")) {
        LOG_ERROR("Could not create IconData table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE INDEX IconDataIndex ON IconData (iconID);")) {
        LOG_ERROR("Could not create PageURL index in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand("CREATE TABLE IconDatabaseInfo (key TEXT NOT NULL ON CONFLICT FAIL UNIQUE ON CONFLICT REPLACE,value TEXT NOT NULL ON CONFLICT FAIL);")) {
        LOG_ERROR("Could not create IconDatabaseInfo table in database (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
    if (!db.executeCommand(String("INSERT INTO IconDatabaseInfo VALUES ('Version', ") + String::number(currentDatabaseVersion) + ");")) {
        LOG_ERROR("Could not insert icon database version into IconDatabaseInfo table (%i) - %s", db.lastError(), db.lastErrorMsg());
        db.close();
        return;
    }
}    

void IconDatabase::performOpenInitialization()
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (!isOpen())
        return;
    
    if (checkIntegrityOnOpen) {
        checkIntegrityOnOpen = false;
        if (!checkIntegrity()) {
            LOG(IconDatabase, "Integrity check was bad - dumping IconDatabase");

            m_syncDB.close();
            
            {
                MutexLocker locker(m_syncLock);
                // Should've been consumed by SQLite, delete just to make sure we don't see it again in the future;
                deleteFile(m_completeDatabasePath + "-journal");
                deleteFile(m_completeDatabasePath);
            }
            
            // Reopen the write database, creating it from scratch
            if (!m_syncDB.open(m_completeDatabasePath)) {
                LOG_ERROR("Unable to open icon database at path %s - %s", m_completeDatabasePath.ascii().data(), m_syncDB.lastErrorMsg());
                return;
            }          
        }
    }
    
    int version = databaseVersionNumber(m_syncDB);
    
    if (version > currentDatabaseVersion) {
        LOG(IconDatabase, "Database version number %i is greater than our current version number %i - closing the database to prevent overwriting newer versions", version, currentDatabaseVersion);
        m_syncDB.close();
        m_threadTerminationRequested = true;
        return;
    }
    
    if (!isValidDatabase(m_syncDB)) {
        LOG(IconDatabase, "%s is missing or in an invalid state - reconstructing", m_completeDatabasePath.ascii().data());
        m_syncDB.clearAllTables();
        createDatabaseTables(m_syncDB);
    }

    // Reduce sqlite RAM cache size from default 2000 pages (~1.5kB per page). 3MB of cache for icon database is overkill
    if (!SQLiteStatement(m_syncDB, "PRAGMA cache_size = 200;").executeCommand())         
        LOG_ERROR("SQLite database could not set cache_size");

    // Tell backup software (i.e., Time Machine) to never back up the icon database, because  
    // it's a large file that changes frequently, thus using a lot of backup disk space, and 
    // it's unlikely that many users would be upset about it not being backed up. We could 
    // make this configurable on a per-client basis some day if some clients don't want this.
    if (canExcludeFromBackup() && !wasExcludedFromBackup() && excludeFromBackup(m_completeDatabasePath))
        setWasExcludedFromBackup();
}

bool IconDatabase::checkIntegrity()
{
    ASSERT_ICON_SYNC_THREAD();
    
    SQLiteStatement integrity(m_syncDB, "PRAGMA integrity_check;");
    if (integrity.prepare() != SQLResultOk) {
        LOG_ERROR("checkIntegrity failed to execute");
        return false;
    }
    
    int resultCode = integrity.step();
    if (resultCode == SQLResultOk)
        return true;
        
    if (resultCode != SQLResultRow)
        return false;

    int columns = integrity.columnCount();
    if (columns != 1) {
        LOG_ERROR("Received %i columns performing integrity check, should be 1", columns);
        return false;
    }
        
    String resultText = integrity.getColumnText(0);
        
    // A successful, no-error integrity check will be "ok" - all other strings imply failure
    if (resultText == "ok")
        return true;
    
    LOG_ERROR("Icon database integrity check failed - \n%s", resultText.ascii().data());
    return false;
}

void IconDatabase::performURLImport()
{
    ASSERT_ICON_SYNC_THREAD();

    SQLiteStatement query(m_syncDB, "SELECT PageURL.url, IconInfo.url, IconInfo.stamp FROM PageURL INNER JOIN IconInfo ON PageURL.iconID=IconInfo.iconID;");
    
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare icon url import query");
        return;
    }
    
    // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
    // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up
    AutodrainedPool pool(25);
        
    int result = query.step();
    while (result == SQLResultRow) {
        String pageURL = query.getColumnText(0);
        String iconURL = query.getColumnText(1);

        {
            MutexLocker locker(m_urlAndIconLock);
            
            PageURLRecord* pageRecord = m_pageURLToRecordMap.get(pageURL);
            
            // If the pageRecord doesn't exist in this map, then no one has retained this pageURL
            // If the s_databaseCleanupCounter count is non-zero, then we're not supposed to be pruning the database in any manner,
            // so go ahead and actually create a pageURLRecord for this url even though it's not retained.
            // If database cleanup *is* allowed, we don't want to bother pulling in a page url from disk that noone is actually interested
            // in - we'll prune it later instead!
            if (!pageRecord && databaseCleanupCounter && pageCanHaveIcon(pageURL)) {
                pageRecord = new PageURLRecord(pageURL);
                m_pageURLToRecordMap.set(pageURL, pageRecord);
            }
            
            if (pageRecord) {
                IconRecord* currentIcon = pageRecord->iconRecord();

                if (!currentIcon || currentIcon->iconURL() != iconURL) {
                    pageRecord->setIconRecord(getOrCreateIconRecord(iconURL));
                    currentIcon = pageRecord->iconRecord();
                }
            
                // Regardless, the time stamp from disk still takes precedence.  Until we read this icon from disk, we didn't think we'd seen it before
                // so we marked the timestamp as "now", but it's really much older
                currentIcon->setTimestamp(query.getColumnInt(2));
            }            
        }
        
        // FIXME: Currently the WebKit API supports 1 type of notification that is sent whenever we get an Icon URL for a Page URL.  We might want to re-purpose it to work for 
        // getting the actually icon itself also (so each pageurl would get this notification twice) or we might want to add a second type of notification -
        // one for the URL and one for the Image itself
        // Note that WebIconDatabase is not neccessarily API so we might be able to make this change
        {
            MutexLocker locker(m_pendingReadingLock);
            if (m_pageURLsPendingImport.contains(pageURL)) {
                dispatchDidImportIconURLForPageURLOnMainThread(pageURL);
                m_pageURLsPendingImport.remove(pageURL);
            
                pool.cycle();
            }
        }
        
        // Stop the import at any time of the thread has been asked to shutdown
        if (shouldStopThreadActivity()) {
            LOG(IconDatabase, "IconDatabase asked to terminate during performURLImport()");
            return;
        }
        
        result = query.step();
    }
    
    if (result != SQLResultDone)
        LOG(IconDatabase, "Error reading page->icon url mappings from database");

    // Clear the m_pageURLsPendingImport set - either the page URLs ended up with an iconURL (that we'll notify about) or not, 
    // but after m_iconURLImportComplete is set to true, we don't care about this set anymore
    Vector<String> urls;
    {
        MutexLocker locker(m_pendingReadingLock);

        urls.appendRange(m_pageURLsPendingImport.begin(), m_pageURLsPendingImport.end());
        m_pageURLsPendingImport.clear();        
        m_iconURLImportComplete = true;
    }
    
    Vector<String> urlsToNotify;
    
    // Loop through the urls pending import
    // Remove unretained ones if database cleanup is allowed
    // Keep a set of ones that are retained and pending notification
    {
        MutexLocker locker(m_urlAndIconLock);
        
        for (unsigned i = 0; i < urls.size(); ++i) {
            if (!m_retainedPageURLs.contains(urls[i])) {
                PageURLRecord* record = m_pageURLToRecordMap.get(urls[i]);
                if (record && !databaseCleanupCounter) {
                    m_pageURLToRecordMap.remove(urls[i]);
                    IconRecord* iconRecord = record->iconRecord();
                    
                    // If this page is the only remaining retainer of its icon, mark that icon for deletion and don't bother
                    // reading anything related to it 
                    if (iconRecord && iconRecord->hasOneRef()) {
                        m_iconURLToRecordMap.remove(iconRecord->iconURL());
                        
                        {
                            MutexLocker locker(m_pendingReadingLock);
                            m_pageURLsInterestedInIcons.remove(urls[i]);
                            m_iconsPendingReading.remove(iconRecord);
                        }
                        {
                            MutexLocker locker(m_pendingSyncLock);
                            m_iconsPendingSync.set(iconRecord->iconURL(), iconRecord->snapshot(true));                    
                        }
                    }
                    
                    delete record;
                }
            } else {
                urlsToNotify.append(urls[i]);
            }
        }
    }

    LOG(IconDatabase, "Notifying %lu interested page URLs that their icon URL is known due to the import", static_cast<unsigned long>(urlsToNotify.size()));
    // Now that we don't hold any locks, perform the actual notifications
    for (unsigned i = 0; i < urlsToNotify.size(); ++i) {
        LOG(IconDatabase, "Notifying icon info known for pageURL %s", urlsToNotify[i].ascii().data());
        dispatchDidImportIconURLForPageURLOnMainThread(urlsToNotify[i]);
        if (shouldStopThreadActivity())
            return;

        pool.cycle();
    }
    
    // Notify the client that the URL import is complete in case it's managing its own pending notifications.
    dispatchDidFinishURLImportOnMainThread();
    
    // Notify all DocumentLoaders that were waiting for an icon load decision on the main thread
    callOnMainThread(notifyPendingLoadDecisionsOnMainThread, this);
}

void* IconDatabase::syncThreadMainLoop()
{
    ASSERT_ICON_SYNC_THREAD();

    bool shouldReenableSuddenTermination = false;

    m_syncLock.lock();

    // It's possible thread termination is requested before the main loop even starts - in that case, just skip straight to cleanup
    while (!m_threadTerminationRequested) {
        m_syncLock.unlock();

#ifndef NDEBUG
        double timeStamp = currentTime();
#endif
        LOG(IconDatabase, "(THREAD) Main work loop starting");

        // If we should remove all icons, do it now.  This is an uninteruptible procedure that we will always do before quitting if it is requested
        if (m_removeIconsRequested) {
            removeAllIconsOnThread();
            m_removeIconsRequested = false;
        }
        
        // Then, if the thread should be quitting, quit now!
        if (m_threadTerminationRequested)
            break;
        
        bool didAnyWork = true;
        while (didAnyWork) {
            bool didWrite = writeToDatabase();
            if (shouldStopThreadActivity())
                break;
                
            didAnyWork = readFromDatabase();
            if (shouldStopThreadActivity())
                break;
                
            // Prune unretained icons after the first time we sync anything out to the database
            // This way, pruning won't be the only operation we perform to the database by itself
            // We also don't want to bother doing this if the thread should be terminating (the user is quitting)
            // or if private browsing is enabled
            // We also don't want to prune if the m_databaseCleanupCounter count is non-zero - that means someone
            // has asked to delay pruning
            static bool prunedUnretainedIcons = false;
            if (didWrite && !m_privateBrowsingEnabled && !prunedUnretainedIcons && !databaseCleanupCounter) {
#ifndef NDEBUG
                double time = currentTime();
#endif
                LOG(IconDatabase, "(THREAD) Starting pruneUnretainedIcons()");
                
                pruneUnretainedIcons();
                
                LOG(IconDatabase, "(THREAD) pruneUnretainedIcons() took %.4f seconds", currentTime() - time);
                
                // If pruneUnretainedIcons() returned early due to requested thread termination, its still okay
                // to mark prunedUnretainedIcons true because we're about to terminate anyway
                prunedUnretainedIcons = true;
            }
            
            didAnyWork = didAnyWork || didWrite;
            if (shouldStopThreadActivity())
                break;
        }
        
#ifndef NDEBUG
        double newstamp = currentTime();
        LOG(IconDatabase, "(THREAD) Main work loop ran for %.4f seconds, %s requested to terminate", newstamp - timeStamp, shouldStopThreadActivity() ? "was" : "was not");
#endif
                    
        m_syncLock.lock();
        
        // There is some condition that is asking us to stop what we're doing now and handle a special case
        // This is either removing all icons, or shutting down the thread to quit the app
        // We handle those at the top of this main loop so continue to jump back up there
        if (shouldStopThreadActivity())
            continue;

        if (shouldReenableSuddenTermination) {
            // The following is balanced by the call to disableSuddenTermination in the
            // wakeSyncThread function. Any time we wait on the condition, we also have
            // to enableSuddenTermation, after doing the next batch of work.
            ASSERT(m_disabledSuddenTerminationForSyncThread);
            enableSuddenTermination();
            m_disabledSuddenTerminationForSyncThread = false;
        }

        m_syncCondition.wait(m_syncLock);

        shouldReenableSuddenTermination = true;
    }

    m_syncLock.unlock();
    
    // Thread is terminating at this point
    cleanupSyncThread();

    if (shouldReenableSuddenTermination) {
        // The following is balanced by the call to disableSuddenTermination in the
        // wakeSyncThread function. Any time we wait on the condition, we also have
        // to enableSuddenTermation, after doing the next batch of work.
        ASSERT(m_disabledSuddenTerminationForSyncThread);
        enableSuddenTermination();
        m_disabledSuddenTerminationForSyncThread = false;
    }

    return 0;
}

bool IconDatabase::readFromDatabase()
{
    ASSERT_ICON_SYNC_THREAD();
    
#ifndef NDEBUG
    double timeStamp = currentTime();
#endif

    bool didAnyWork = false;

    // We'll make a copy of the sets of things that need to be read.  Then we'll verify at the time of updating the record that it still wants to be updated
    // This way we won't hold the lock for a long period of time
    Vector<IconRecord*> icons;
    {
        MutexLocker locker(m_pendingReadingLock);
        icons.appendRange(m_iconsPendingReading.begin(), m_iconsPendingReading.end());
    }
    
    // Keep track of icons we actually read to notify them of the new icon    
    HashSet<String> urlsToNotify;
    
    for (unsigned i = 0; i < icons.size(); ++i) {
        didAnyWork = true;
        RefPtr<SharedBuffer> imageData = getImageDataForIconURLFromSQLDatabase(icons[i]->iconURL());

        // Verify this icon still wants to be read from disk
        {
            MutexLocker urlLocker(m_urlAndIconLock);
            {
                MutexLocker readLocker(m_pendingReadingLock);
                
                if (m_iconsPendingReading.contains(icons[i])) {
                    // Set the new data
                    icons[i]->setImageData(imageData.release());
                    
                    // Remove this icon from the set that needs to be read
                    m_iconsPendingReading.remove(icons[i]);
                    
                    // We have a set of all Page URLs that retain this icon as well as all PageURLs waiting for an icon
                    // We want to find the intersection of these two sets to notify them
                    // Check the sizes of these two sets to minimize the number of iterations
                    const HashSet<String>* outerHash;
                    const HashSet<String>* innerHash;
                    
                    if (icons[i]->retainingPageURLs().size() > m_pageURLsInterestedInIcons.size()) {
                        outerHash = &m_pageURLsInterestedInIcons;
                        innerHash = &(icons[i]->retainingPageURLs());
                    } else {
                        innerHash = &m_pageURLsInterestedInIcons;
                        outerHash = &(icons[i]->retainingPageURLs());
                    }
                    
                    HashSet<String>::const_iterator iter = outerHash->begin();
                    HashSet<String>::const_iterator end = outerHash->end();
                    for (; iter != end; ++iter) {
                        if (innerHash->contains(*iter)) {
                            LOG(IconDatabase, "%s is interesting in the icon we just read.  Adding it to the list and removing it from the interested set", urlForLogging(*iter).ascii().data());
                            urlsToNotify.add(*iter);
                        }
                        
                        // If we ever get to the point were we've seen every url interested in this icon, break early
                        if (urlsToNotify.size() == m_pageURLsInterestedInIcons.size())
                            break;
                    }
                    
                    // We don't need to notify a PageURL twice, so all the ones we're about to notify can be removed from the interested set
                    if (urlsToNotify.size() == m_pageURLsInterestedInIcons.size())
                        m_pageURLsInterestedInIcons.clear();
                    else {
                        iter = urlsToNotify.begin();
                        end = urlsToNotify.end();
                        for (; iter != end; ++iter)
                            m_pageURLsInterestedInIcons.remove(*iter);
                    }
                }
            }
        }
    
        if (shouldStopThreadActivity())
            return didAnyWork;
        
        // Informal testing shows that draining the autorelease pool every 25 iterations is about as low as we can go
        // before performance starts to drop off, but we don't want to increase this number because then accumulated memory usage will go up
        AutodrainedPool pool(25);

        // Now that we don't hold any locks, perform the actual notifications
        HashSet<String>::iterator iter = urlsToNotify.begin();
        HashSet<String>::iterator end = urlsToNotify.end();
        for (unsigned iteration = 0; iter != end; ++iter, ++iteration) {
            LOG(IconDatabase, "Notifying icon received for pageURL %s", urlForLogging(*iter).ascii().data());
            dispatchDidImportIconDataForPageURLOnMainThread(*iter);
            if (shouldStopThreadActivity())
                return didAnyWork;
            
            pool.cycle();
        }

        LOG(IconDatabase, "Done notifying %i pageURLs who just received their icons", urlsToNotify.size());
        urlsToNotify.clear();
        
        if (shouldStopThreadActivity())
            return didAnyWork;
    }

    LOG(IconDatabase, "Reading from database took %.4f seconds", currentTime() - timeStamp);

    return didAnyWork;
}

bool IconDatabase::writeToDatabase()
{
    ASSERT_ICON_SYNC_THREAD();

#ifndef NDEBUG
    double timeStamp = currentTime();
#endif

    bool didAnyWork = false;
    
    // We can copy the current work queue then clear it out - If any new work comes in while we're writing out,
    // we'll pick it up on the next pass.  This greatly simplifies the locking strategy for this method and remains cohesive with changes
    // asked for by the database on the main thread
    {
        MutexLocker locker(m_urlAndIconLock);
        Vector<IconSnapshot> iconSnapshots;
        Vector<PageURLSnapshot> pageSnapshots;
        {
            MutexLocker locker(m_pendingSyncLock);

            iconSnapshots.appendRange(m_iconsPendingSync.begin().values(), m_iconsPendingSync.end().values());
            m_iconsPendingSync.clear();

            pageSnapshots.appendRange(m_pageURLsPendingSync.begin().values(), m_pageURLsPendingSync.end().values());
            m_pageURLsPendingSync.clear();
        }

        if (iconSnapshots.size() || pageSnapshots.size())
            didAnyWork = true;

        SQLiteTransaction syncTransaction(m_syncDB);
        syncTransaction.begin();

        for (unsigned i = 0; i < iconSnapshots.size(); ++i) {
            writeIconSnapshotToSQLDatabase(iconSnapshots[i]);
            LOG(IconDatabase, "Wrote IconRecord for IconURL %s with timeStamp of %i to the DB", urlForLogging(iconSnapshots[i].iconURL()).ascii().data(), iconSnapshots[i].timestamp());
        }

        for (unsigned i = 0; i < pageSnapshots.size(); ++i) {
            // If the icon URL is empty, this page is meant to be deleted
            // ASSERTs are sanity checks to make sure the mappings exist if they should and don't if they shouldn't
            if (pageSnapshots[i].iconURL().isEmpty())
                removePageURLFromSQLDatabase(pageSnapshots[i].pageURL());
            else
                setIconURLForPageURLInSQLDatabase(pageSnapshots[i].iconURL(), pageSnapshots[i].pageURL());
            LOG(IconDatabase, "Committed IconURL for PageURL %s to database", urlForLogging(pageSnapshots[i].pageURL()).ascii().data());
        }

        syncTransaction.commit();
    }

    // Check to make sure there are no dangling PageURLs - If there are, we want to output one log message but not spam the console potentially every few seconds
    if (didAnyWork)
        checkForDanglingPageURLs(false);

    LOG(IconDatabase, "Updating the database took %.4f seconds", currentTime() - timeStamp);

    return didAnyWork;
}

void IconDatabase::pruneUnretainedIcons()
{
    ASSERT_ICON_SYNC_THREAD();

    if (!isOpen())
        return;        
    
    // This method should only be called once per run
    ASSERT(!m_initialPruningComplete);

    // This method relies on having read in all page URLs from the database earlier.
    ASSERT(m_iconURLImportComplete);

    // Get the known PageURLs from the db, and record the ID of any that are not in the retain count set.
    Vector<int64_t> pageIDsToDelete; 

    SQLiteStatement pageSQL(m_syncDB, "SELECT rowid, url FROM PageURL;");
    pageSQL.prepare();
    
    int result;
    while ((result = pageSQL.step()) == SQLResultRow) {
        MutexLocker locker(m_urlAndIconLock);
        if (!m_pageURLToRecordMap.contains(pageSQL.getColumnText(1)))
            pageIDsToDelete.append(pageSQL.getColumnInt64(0));
    }
    
    if (result != SQLResultDone)
        LOG_ERROR("Error reading PageURL table from on-disk DB");
    pageSQL.finalize();
    
    // Delete page URLs that were in the table, but not in our retain count set.
    size_t numToDelete = pageIDsToDelete.size();
    if (numToDelete) {
        SQLiteTransaction pruningTransaction(m_syncDB);
        pruningTransaction.begin();
        
        SQLiteStatement pageDeleteSQL(m_syncDB, "DELETE FROM PageURL WHERE rowid = (?);");
        pageDeleteSQL.prepare();
        for (size_t i = 0; i < numToDelete; ++i) {
#if OS(WINDOWS)
            LOG(IconDatabase, "Pruning page with rowid %I64i from disk", static_cast<long long>(pageIDsToDelete[i]));
#else
            LOG(IconDatabase, "Pruning page with rowid %lli from disk", static_cast<long long>(pageIDsToDelete[i]));
#endif
            pageDeleteSQL.bindInt64(1, pageIDsToDelete[i]);
            int result = pageDeleteSQL.step();
            if (result != SQLResultDone)
#if OS(WINDOWS)
                LOG_ERROR("Unabled to delete page with id %I64i from disk", static_cast<long long>(pageIDsToDelete[i]));
#else
                LOG_ERROR("Unabled to delete page with id %lli from disk", static_cast<long long>(pageIDsToDelete[i]));
#endif
            pageDeleteSQL.reset();
            
            // If the thread was asked to terminate, we should commit what pruning we've done so far, figuring we can
            // finish the rest later (hopefully)
            if (shouldStopThreadActivity()) {
                pruningTransaction.commit();
                return;
            }
        }
        pruningTransaction.commit();
        pageDeleteSQL.finalize();
    }
    
    // Deleting unreferenced icons from the Icon tables has to be atomic - 
    // If the user quits while these are taking place, they might have to wait.  Thankfully this will rarely be an issue
    // A user on a network home directory with a wildly inconsistent database might see quite a pause...

    SQLiteTransaction pruningTransaction(m_syncDB);
    pruningTransaction.begin();
    
    // Wipe Icons that aren't retained
    if (!m_syncDB.executeCommand("DELETE FROM IconData WHERE iconID NOT IN (SELECT iconID FROM PageURL);"))
        LOG_ERROR("Failed to execute SQL to prune unretained icons from the on-disk IconData table");    
    if (!m_syncDB.executeCommand("DELETE FROM IconInfo WHERE iconID NOT IN (SELECT iconID FROM PageURL);"))
        LOG_ERROR("Failed to execute SQL to prune unretained icons from the on-disk IconInfo table");    
    
    pruningTransaction.commit();
        
    checkForDanglingPageURLs(true);

    m_initialPruningComplete = true;
}

void IconDatabase::checkForDanglingPageURLs(bool pruneIfFound)
{
    ASSERT_ICON_SYNC_THREAD();

    // This check can be relatively expensive so we don't do it in a release build unless the caller has asked us to prune any dangling
    // entries.  We also don't want to keep performing this check and reporting this error if it has already found danglers before so we
    // keep track of whether we've found any.  We skip the check in the release build pretending to have already found danglers already.
#ifndef NDEBUG
    static bool danglersFound = true;
#else
    static bool danglersFound = false;
#endif

    if ((pruneIfFound || !danglersFound) && SQLiteStatement(m_syncDB, "SELECT url FROM PageURL WHERE PageURL.iconID NOT IN (SELECT iconID FROM IconInfo) LIMIT 1;").returnsAtLeastOneResult()) {
        danglersFound = true;
        LOG(IconDatabase, "Dangling PageURL entries found");
        if (pruneIfFound && !m_syncDB.executeCommand("DELETE FROM PageURL WHERE iconID NOT IN (SELECT iconID FROM IconInfo);"))
            LOG(IconDatabase, "Unable to prune dangling PageURLs");
    }
}

void IconDatabase::removeAllIconsOnThread()
{
    ASSERT_ICON_SYNC_THREAD();

    LOG(IconDatabase, "Removing all icons on the sync thread");
        
    // Delete all the prepared statements so they can start over
    deleteAllPreparedStatements();    
    
    // To reset the on-disk database, we'll wipe all its tables then vacuum it
    // This is easier and safer than closing it, deleting the file, and recreating from scratch
    m_syncDB.clearAllTables();
    m_syncDB.runVacuumCommand();
    createDatabaseTables(m_syncDB);
    
    LOG(IconDatabase, "Dispatching notification that we removed all icons");
    dispatchDidRemoveAllIconsOnMainThread();    
}

void IconDatabase::deleteAllPreparedStatements()
{
    ASSERT_ICON_SYNC_THREAD();
    
    m_setIconIDForPageURLStatement.clear();
    m_removePageURLStatement.clear();
    m_getIconIDForIconURLStatement.clear();
    m_getImageDataForIconURLStatement.clear();
    m_addIconToIconInfoStatement.clear();
    m_addIconToIconDataStatement.clear();
    m_getImageDataStatement.clear();
    m_deletePageURLsForIconURLStatement.clear();
    m_deleteIconFromIconInfoStatement.clear();
    m_deleteIconFromIconDataStatement.clear();
    m_updateIconInfoStatement.clear();
    m_updateIconDataStatement.clear();
    m_setIconInfoStatement.clear();
    m_setIconDataStatement.clear();
}

void* IconDatabase::cleanupSyncThread()
{
    ASSERT_ICON_SYNC_THREAD();
    
#ifndef NDEBUG
    double timeStamp = currentTime();
#endif 

    // If the removeIcons flag is set, remove all icons from the db.
    if (m_removeIconsRequested)
        removeAllIconsOnThread();

    // Sync remaining icons out
    LOG(IconDatabase, "(THREAD) Doing final writeout and closure of sync thread");
    writeToDatabase();
    
    // Close the database
    MutexLocker locker(m_syncLock);
    
    m_databaseDirectory = String();
    m_completeDatabasePath = String();
    deleteAllPreparedStatements();    
    m_syncDB.close();
    
#ifndef NDEBUG
    LOG(IconDatabase, "(THREAD) Final closure took %.4f seconds", currentTime() - timeStamp);
#endif
    
    m_syncThreadRunning = false;
    return 0;
}

bool IconDatabase::imported()
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (m_isImportedSet)
        return m_imported;
        
    SQLiteStatement query(m_syncDB, "SELECT IconDatabaseInfo.value FROM IconDatabaseInfo WHERE IconDatabaseInfo.key = \"ImportedSafari2Icons\";");
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare imported statement");
        return false;
    }
    
    int result = query.step();
    if (result == SQLResultRow)
        result = query.getColumnInt(0);
    else {
        if (result != SQLResultDone)
            LOG_ERROR("imported statement failed");
        result = 0;
    }
    
    m_isImportedSet = true;
    return m_imported = result;
}

void IconDatabase::setImported(bool import)
{
    ASSERT_ICON_SYNC_THREAD();

    m_imported = import;
    m_isImportedSet = true;
    
    String queryString = import ?
        "INSERT INTO IconDatabaseInfo (key, value) VALUES (\"ImportedSafari2Icons\", 1);" :
        "INSERT INTO IconDatabaseInfo (key, value) VALUES (\"ImportedSafari2Icons\", 0);";
        
    SQLiteStatement query(m_syncDB, queryString);
    
    if (query.prepare() != SQLResultOk) {
        LOG_ERROR("Unable to prepare set imported statement");
        return;
    }    
    
    if (query.step() != SQLResultDone)
        LOG_ERROR("set imported statement failed");
}

// readySQLiteStatement() handles two things
// 1 - If the SQLDatabase& argument is different, the statement must be destroyed and remade.  This happens when the user
//     switches to and from private browsing
// 2 - Lazy construction of the Statement in the first place, in case we've never made this query before
inline void readySQLiteStatement(OwnPtr<SQLiteStatement>& statement, SQLiteDatabase& db, const String& str)
{
    if (statement && (statement->database() != &db || statement->isExpired())) {
        if (statement->isExpired())
            LOG(IconDatabase, "SQLiteStatement associated with %s is expired", str.ascii().data());
        statement.clear();
    }
    if (!statement) {
        statement = adoptPtr(new SQLiteStatement(db, str));
        if (statement->prepare() != SQLResultOk)
            LOG_ERROR("Preparing statement %s failed", str.ascii().data());
    }
}

void IconDatabase::setIconURLForPageURLInSQLDatabase(const String& iconURL, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(iconURL);

    if (!iconID)
        iconID = addIconURLToSQLDatabase(iconURL);
    
    if (!iconID) {
        LOG_ERROR("Failed to establish an ID for iconURL %s", urlForLogging(iconURL).ascii().data());
        ASSERT(false);
        return;
    }
    
    setIconIDForPageURLInSQLDatabase(iconID, pageURL);
}

void IconDatabase::setIconIDForPageURLInSQLDatabase(int64_t iconID, const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLiteStatement(m_setIconIDForPageURLStatement, m_syncDB, "INSERT INTO PageURL (url, iconID) VALUES ((?), ?);");
    m_setIconIDForPageURLStatement->bindText(1, pageURL);
    m_setIconIDForPageURLStatement->bindInt64(2, iconID);

    int result = m_setIconIDForPageURLStatement->step();
    if (result != SQLResultDone) {
        ASSERT(false);
        LOG_ERROR("setIconIDForPageURLQuery failed for url %s", urlForLogging(pageURL).ascii().data());
    }

    m_setIconIDForPageURLStatement->reset();
}

void IconDatabase::removePageURLFromSQLDatabase(const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLiteStatement(m_removePageURLStatement, m_syncDB, "DELETE FROM PageURL WHERE url = (?);");
    m_removePageURLStatement->bindText(1, pageURL);

    if (m_removePageURLStatement->step() != SQLResultDone)
        LOG_ERROR("removePageURLFromSQLDatabase failed for url %s", urlForLogging(pageURL).ascii().data());
    
    m_removePageURLStatement->reset();
}


int64_t IconDatabase::getIconIDForIconURLFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    readySQLiteStatement(m_getIconIDForIconURLStatement, m_syncDB, "SELECT IconInfo.iconID FROM IconInfo WHERE IconInfo.url = (?);");
    m_getIconIDForIconURLStatement->bindText(1, iconURL);
    
    int64_t result = m_getIconIDForIconURLStatement->step();
    if (result == SQLResultRow)
        result = m_getIconIDForIconURLStatement->getColumnInt64(0);
    else {
        if (result != SQLResultDone)
            LOG_ERROR("getIconIDForIconURLFromSQLDatabase failed for url %s", urlForLogging(iconURL).ascii().data());
        result = 0;
    }

    m_getIconIDForIconURLStatement->reset();
    return result;
}

int64_t IconDatabase::addIconURLToSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    // There would be a transaction here to make sure these two inserts are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another
    // here is unnecessary
    
    readySQLiteStatement(m_addIconToIconInfoStatement, m_syncDB, "INSERT INTO IconInfo (url, stamp) VALUES (?, 0);");
    m_addIconToIconInfoStatement->bindText(1, iconURL);
    
    int result = m_addIconToIconInfoStatement->step();
    m_addIconToIconInfoStatement->reset();
    if (result != SQLResultDone) {
        LOG_ERROR("addIconURLToSQLDatabase failed to insert %s into IconInfo", urlForLogging(iconURL).ascii().data());
        return 0;
    }
    int64_t iconID = m_syncDB.lastInsertRowID();
    
    readySQLiteStatement(m_addIconToIconDataStatement, m_syncDB, "INSERT INTO IconData (iconID, data) VALUES (?, ?);");
    m_addIconToIconDataStatement->bindInt64(1, iconID);
    
    result = m_addIconToIconDataStatement->step();
    m_addIconToIconDataStatement->reset();
    if (result != SQLResultDone) {
        LOG_ERROR("addIconURLToSQLDatabase failed to insert %s into IconData", urlForLogging(iconURL).ascii().data());
        return 0;
    }
    
    return iconID;
}

PassRefPtr<SharedBuffer> IconDatabase::getImageDataForIconURLFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    RefPtr<SharedBuffer> imageData;
    
    readySQLiteStatement(m_getImageDataForIconURLStatement, m_syncDB, "SELECT IconData.data FROM IconData WHERE IconData.iconID IN (SELECT iconID FROM IconInfo WHERE IconInfo.url = (?));");
    m_getImageDataForIconURLStatement->bindText(1, iconURL);
    
    int result = m_getImageDataForIconURLStatement->step();
    if (result == SQLResultRow) {
        Vector<char> data;
        m_getImageDataForIconURLStatement->getColumnBlobAsVector(0, data);
        imageData = SharedBuffer::create(data.data(), data.size());
    } else if (result != SQLResultDone)
        LOG_ERROR("getImageDataForIconURLFromSQLDatabase failed for url %s", urlForLogging(iconURL).ascii().data());

    m_getImageDataForIconURLStatement->reset();
    
    return imageData.release();
}

void IconDatabase::removeIconFromSQLDatabase(const String& iconURL)
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (iconURL.isEmpty())
        return;

    // There would be a transaction here to make sure these removals are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another here is unnecessary
    
    // It's possible this icon is not in the database because of certain rapid browsing patterns (such as a stress test) where the
    // icon is marked to be added then marked for removal before it is ever written to disk.  No big deal, early return
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(iconURL);
    if (!iconID)
        return;
    
    readySQLiteStatement(m_deletePageURLsForIconURLStatement, m_syncDB, "DELETE FROM PageURL WHERE PageURL.iconID = (?);");
    m_deletePageURLsForIconURLStatement->bindInt64(1, iconID);
    
    if (m_deletePageURLsForIconURLStatement->step() != SQLResultDone)
        LOG_ERROR("m_deletePageURLsForIconURLStatement failed for url %s", urlForLogging(iconURL).ascii().data());
    
    readySQLiteStatement(m_deleteIconFromIconInfoStatement, m_syncDB, "DELETE FROM IconInfo WHERE IconInfo.iconID = (?);");
    m_deleteIconFromIconInfoStatement->bindInt64(1, iconID);
    
    if (m_deleteIconFromIconInfoStatement->step() != SQLResultDone)
        LOG_ERROR("m_deleteIconFromIconInfoStatement failed for url %s", urlForLogging(iconURL).ascii().data());
        
    readySQLiteStatement(m_deleteIconFromIconDataStatement, m_syncDB, "DELETE FROM IconData WHERE IconData.iconID = (?);");
    m_deleteIconFromIconDataStatement->bindInt64(1, iconID);
    
    if (m_deleteIconFromIconDataStatement->step() != SQLResultDone)
        LOG_ERROR("m_deleteIconFromIconDataStatement failed for url %s", urlForLogging(iconURL).ascii().data());
        
    m_deletePageURLsForIconURLStatement->reset();
    m_deleteIconFromIconInfoStatement->reset();
    m_deleteIconFromIconDataStatement->reset();
}

void IconDatabase::writeIconSnapshotToSQLDatabase(const IconSnapshot& snapshot)
{
    ASSERT_ICON_SYNC_THREAD();
    
    if (snapshot.iconURL().isEmpty())
        return;
        
    // A nulled out timestamp and data means this icon is destined to be deleted - do that instead of writing it out
    if (!snapshot.timestamp() && !snapshot.data()) {
        LOG(IconDatabase, "Removing %s from on-disk database", urlForLogging(snapshot.iconURL()).ascii().data());
        removeIconFromSQLDatabase(snapshot.iconURL());
        return;
    }

    // There would be a transaction here to make sure these removals are atomic
    // In practice the only caller of this method is always wrapped in a transaction itself so placing another here is unnecessary
        
    // Get the iconID for this url
    int64_t iconID = getIconIDForIconURLFromSQLDatabase(snapshot.iconURL());
    
    // If there is already an iconID in place, update the database.  
    // Otherwise, insert new records
    if (iconID) {    
        readySQLiteStatement(m_updateIconInfoStatement, m_syncDB, "UPDATE IconInfo SET stamp = ?, url = ? WHERE iconID = ?;");
        m_updateIconInfoStatement->bindInt64(1, snapshot.timestamp());
        m_updateIconInfoStatement->bindText(2, snapshot.iconURL());
        m_updateIconInfoStatement->bindInt64(3, iconID);

        if (m_updateIconInfoStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to update icon info for url %s", urlForLogging(snapshot.iconURL()).ascii().data());
        
        m_updateIconInfoStatement->reset();
        
        readySQLiteStatement(m_updateIconDataStatement, m_syncDB, "UPDATE IconData SET data = ? WHERE iconID = ?;");
        m_updateIconDataStatement->bindInt64(2, iconID);
                
        // If we *have* image data, bind it to this statement - Otherwise bind "null" for the blob data, 
        // signifying that this icon doesn't have any data    
        if (snapshot.data() && snapshot.data()->size())
            m_updateIconDataStatement->bindBlob(1, snapshot.data()->data(), snapshot.data()->size());
        else
            m_updateIconDataStatement->bindNull(1);
        
        if (m_updateIconDataStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to update icon data for url %s", urlForLogging(snapshot.iconURL()).ascii().data());

        m_updateIconDataStatement->reset();
    } else {    
        readySQLiteStatement(m_setIconInfoStatement, m_syncDB, "INSERT INTO IconInfo (url,stamp) VALUES (?, ?);");
        m_setIconInfoStatement->bindText(1, snapshot.iconURL());
        m_setIconInfoStatement->bindInt64(2, snapshot.timestamp());

        if (m_setIconInfoStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to set icon info for url %s", urlForLogging(snapshot.iconURL()).ascii().data());
        
        m_setIconInfoStatement->reset();
        
        int64_t iconID = m_syncDB.lastInsertRowID();

        readySQLiteStatement(m_setIconDataStatement, m_syncDB, "INSERT INTO IconData (iconID, data) VALUES (?, ?);");
        m_setIconDataStatement->bindInt64(1, iconID);

        // If we *have* image data, bind it to this statement - Otherwise bind "null" for the blob data, 
        // signifying that this icon doesn't have any data    
        if (snapshot.data() && snapshot.data()->size())
            m_setIconDataStatement->bindBlob(2, snapshot.data()->data(), snapshot.data()->size());
        else
            m_setIconDataStatement->bindNull(2);
        
        if (m_setIconDataStatement->step() != SQLResultDone)
            LOG_ERROR("Failed to set icon data for url %s", urlForLogging(snapshot.iconURL()).ascii().data());

        m_setIconDataStatement->reset();
    }
}

bool IconDatabase::wasExcludedFromBackup()
{
    ASSERT_ICON_SYNC_THREAD();

    return SQLiteStatement(m_syncDB, "SELECT value FROM IconDatabaseInfo WHERE key = 'ExcludedFromBackup';").getColumnInt(0);
}

void IconDatabase::setWasExcludedFromBackup()
{
    ASSERT_ICON_SYNC_THREAD();

    SQLiteStatement(m_syncDB, "INSERT INTO IconDatabaseInfo (key, value) VALUES ('ExcludedFromBackup', 1)").executeCommand();
}

class ClientWorkItem {
public:
    ClientWorkItem(IconDatabaseClient* client)
        : m_client(client)
    { }
    virtual void performWork() = 0;
    virtual ~ClientWorkItem() { }

protected:
    IconDatabaseClient* m_client;
};

class ImportedIconURLForPageURLWorkItem : public ClientWorkItem {
public:
    ImportedIconURLForPageURLWorkItem(IconDatabaseClient* client, const String& pageURL)
        : ClientWorkItem(client)
        , m_pageURL(new String(pageURL.threadsafeCopy()))
    { }
    
    virtual ~ImportedIconURLForPageURLWorkItem()
    {
        delete m_pageURL;
    }

    virtual void performWork()
    {
        ASSERT(m_client);
        m_client->didImportIconURLForPageURL(*m_pageURL);
        m_client = 0;
    }
    
private:
    String* m_pageURL;
};

class ImportedIconDataForPageURLWorkItem : public ClientWorkItem {
public:
    ImportedIconDataForPageURLWorkItem(IconDatabaseClient* client, const String& pageURL)
        : ClientWorkItem(client)
        , m_pageURL(new String(pageURL.threadsafeCopy()))
    { }
    
    virtual ~ImportedIconDataForPageURLWorkItem()
    {
        delete m_pageURL;
    }

    virtual void performWork()
    {
        ASSERT(m_client);
        m_client->didImportIconDataForPageURL(*m_pageURL);
        m_client = 0;
    }
    
private:
    String* m_pageURL;
};

class RemovedAllIconsWorkItem : public ClientWorkItem {
public:
    RemovedAllIconsWorkItem(IconDatabaseClient* client)
        : ClientWorkItem(client)
    { }

    virtual void performWork()
    {
        ASSERT(m_client);
        m_client->didRemoveAllIcons();
        m_client = 0;
    }
};

class FinishedURLImport : public ClientWorkItem {
public:
    FinishedURLImport(IconDatabaseClient* client)
        : ClientWorkItem(client)
    { }

    virtual void performWork()
    {
        ASSERT(m_client);
        m_client->didFinishURLImport();
        m_client = 0;
    }
};

static void performWorkItem(void* context)
{
    ClientWorkItem* item = static_cast<ClientWorkItem*>(context);
    item->performWork();
    delete item;
}

void IconDatabase::dispatchDidImportIconURLForPageURLOnMainThread(const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();

    ImportedIconURLForPageURLWorkItem* work = new ImportedIconURLForPageURLWorkItem(m_client, pageURL);
    callOnMainThread(performWorkItem, work);
}

void IconDatabase::dispatchDidImportIconDataForPageURLOnMainThread(const String& pageURL)
{
    ASSERT_ICON_SYNC_THREAD();

    ImportedIconDataForPageURLWorkItem* work = new ImportedIconDataForPageURLWorkItem(m_client, pageURL);
    callOnMainThread(performWorkItem, work);
}

void IconDatabase::dispatchDidRemoveAllIconsOnMainThread()
{
    ASSERT_ICON_SYNC_THREAD();

    RemovedAllIconsWorkItem* work = new RemovedAllIconsWorkItem(m_client);
    callOnMainThread(performWorkItem, work);
}

void IconDatabase::dispatchDidFinishURLImportOnMainThread()
{
    ASSERT_ICON_SYNC_THREAD();

    FinishedURLImport* work = new FinishedURLImport(m_client);
    callOnMainThread(performWorkItem, work);
}


} // namespace WebCore

#endif // ENABLE(ICONDATABASE)
