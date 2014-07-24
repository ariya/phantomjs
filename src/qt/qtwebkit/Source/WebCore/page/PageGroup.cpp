/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PageGroup.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "DOMWrapperWorld.h"
#include "Document.h"
#include "DocumentStyleSheetCollection.h"
#include "Frame.h"
#include "GroupSettings.h"
#include "Page.h"
#include "PageCache.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "StorageNamespace.h"

#if ENABLE(VIDEO_TRACK)
#if (PLATFORM(MAC) && !PLATFORM(IOS)) || HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
#include "CaptionUserPreferencesMediaAF.h"
#else
#include "CaptionUserPreferences.h"
#endif
#endif

namespace WebCore {

static unsigned getUniqueIdentifier()
{
    static unsigned currentIdentifier = 0;
    return ++currentIdentifier;
}

// --------

static bool shouldTrackVisitedLinks = false;

PageGroup::PageGroup(const String& name)
    : m_name(name)
    , m_visitedLinksPopulated(false)
    , m_identifier(getUniqueIdentifier())
    , m_groupSettings(GroupSettings::create())
{
}

PageGroup::PageGroup(Page* page)
    : m_visitedLinksPopulated(false)
    , m_identifier(getUniqueIdentifier())
    , m_groupSettings(GroupSettings::create())
{
    ASSERT(page);
    addPage(page);
}

PageGroup::~PageGroup()
{
    removeAllUserContent();
}

PassOwnPtr<PageGroup> PageGroup::create(Page* page)
{
    return adoptPtr(new PageGroup(page));
}

typedef HashMap<String, PageGroup*> PageGroupMap;
static PageGroupMap* pageGroups = 0;

PageGroup* PageGroup::pageGroup(const String& groupName)
{
    ASSERT(!groupName.isEmpty());
    
    if (!pageGroups)
        pageGroups = new PageGroupMap;

    PageGroupMap::AddResult result = pageGroups->add(groupName, 0);

    if (result.isNewEntry) {
        ASSERT(!result.iterator->value);
        result.iterator->value = new PageGroup(groupName);
    }

    ASSERT(result.iterator->value);
    return result.iterator->value;
}

void PageGroup::closeLocalStorage()
{
    if (!pageGroups)
        return;

    PageGroupMap::iterator end = pageGroups->end();

    for (PageGroupMap::iterator it = pageGroups->begin(); it != end; ++it) {
        if (it->value->hasLocalStorage())
            it->value->localStorage()->close();
    }
}

void PageGroup::clearLocalStorageForAllOrigins()
{
    if (!pageGroups)
        return;

    PageGroupMap::iterator end = pageGroups->end();
    for (PageGroupMap::iterator it = pageGroups->begin(); it != end; ++it) {
        if (it->value->hasLocalStorage())
            it->value->localStorage()->clearAllOriginsForDeletion();
    }
}

void PageGroup::clearLocalStorageForOrigin(SecurityOrigin* origin)
{
    if (!pageGroups)
        return;

    PageGroupMap::iterator end = pageGroups->end();
    for (PageGroupMap::iterator it = pageGroups->begin(); it != end; ++it) {
        if (it->value->hasLocalStorage())
            it->value->localStorage()->clearOriginForDeletion(origin);
    }
}

void PageGroup::closeIdleLocalStorageDatabases()
{
    if (!pageGroups)
        return;

    PageGroupMap::iterator end = pageGroups->end();
    for (PageGroupMap::iterator it = pageGroups->begin(); it != end; ++it) {
        if (it->value->hasLocalStorage())
            it->value->localStorage()->closeIdleLocalStorageDatabases();
    }
}

void PageGroup::syncLocalStorage()
{
    if (!pageGroups)
        return;

    PageGroupMap::iterator end = pageGroups->end();
    for (PageGroupMap::iterator it = pageGroups->begin(); it != end; ++it) {
        if (it->value->hasLocalStorage())
            it->value->localStorage()->sync();
    }
}

unsigned PageGroup::numberOfPageGroups()
{
    if (!pageGroups)
        return 0;

    return pageGroups->size();
}

void PageGroup::addPage(Page* page)
{
    ASSERT(page);
    ASSERT(!m_pages.contains(page));
    m_pages.add(page);
}

void PageGroup::removePage(Page* page)
{
    ASSERT(page);
    ASSERT(m_pages.contains(page));
    m_pages.remove(page);
}

bool PageGroup::isLinkVisited(LinkHash visitedLinkHash)
{
    if (!m_visitedLinksPopulated) {
        m_visitedLinksPopulated = true;
        ASSERT(!m_pages.isEmpty());
        (*m_pages.begin())->chrome().client()->populateVisitedLinks();
    }
    return m_visitedLinkHashes.contains(visitedLinkHash);
}

void PageGroup::addVisitedLinkHash(LinkHash hash)
{
    if (shouldTrackVisitedLinks)
        addVisitedLink(hash);
}

inline void PageGroup::addVisitedLink(LinkHash hash)
{
    ASSERT(shouldTrackVisitedLinks);
    if (!m_visitedLinkHashes.add(hash).isNewEntry)
        return;
    Page::visitedStateChanged(this, hash);
    pageCache()->markPagesForVistedLinkStyleRecalc();
}

void PageGroup::addVisitedLink(const KURL& url)
{
    if (!shouldTrackVisitedLinks)
        return;
    ASSERT(!url.isEmpty());
    addVisitedLink(visitedLinkHash(url.string()));
}

void PageGroup::addVisitedLink(const UChar* characters, size_t length)
{
    if (!shouldTrackVisitedLinks)
        return;
    addVisitedLink(visitedLinkHash(characters, length));
}

void PageGroup::removeVisitedLinks()
{
    m_visitedLinksPopulated = false;
    if (m_visitedLinkHashes.isEmpty())
        return;
    m_visitedLinkHashes.clear();
    Page::allVisitedStateChanged(this);
    pageCache()->markPagesForVistedLinkStyleRecalc();
}

void PageGroup::removeAllVisitedLinks()
{
    Page::removeAllVisitedLinks();
    pageCache()->markPagesForVistedLinkStyleRecalc();
}

void PageGroup::setShouldTrackVisitedLinks(bool shouldTrack)
{
    if (shouldTrackVisitedLinks == shouldTrack)
        return;
    shouldTrackVisitedLinks = shouldTrack;
    if (!shouldTrackVisitedLinks)
        removeAllVisitedLinks();
}

StorageNamespace* PageGroup::localStorage()
{
    if (!m_localStorage)
        m_localStorage = StorageNamespace::localStorageNamespace(this);

    return m_localStorage.get();
}

StorageNamespace* PageGroup::transientLocalStorage(SecurityOrigin* topOrigin)
{
    HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageNamespace> >::AddResult result = m_transientLocalStorageMap.add(topOrigin, 0);

    if (result.isNewEntry)
        result.iterator->value = StorageNamespace::transientLocalStorageNamespace(this, topOrigin);

    return result.iterator->value.get();
}

void PageGroup::addUserScriptToWorld(DOMWrapperWorld* world, const String& source, const KURL& url,
                                     const Vector<String>& whitelist, const Vector<String>& blacklist,
                                     UserScriptInjectionTime injectionTime, UserContentInjectedFrames injectedFrames)
{
    ASSERT_ARG(world, world);

    OwnPtr<UserScript> userScript = adoptPtr(new UserScript(source, url, whitelist, blacklist, injectionTime, injectedFrames));
    if (!m_userScripts)
        m_userScripts = adoptPtr(new UserScriptMap);
    OwnPtr<UserScriptVector>& scriptsInWorld = m_userScripts->add(world, nullptr).iterator->value;
    if (!scriptsInWorld)
        scriptsInWorld = adoptPtr(new UserScriptVector);
    scriptsInWorld->append(userScript.release());
}

void PageGroup::addUserStyleSheetToWorld(DOMWrapperWorld* world, const String& source, const KURL& url,
                                         const Vector<String>& whitelist, const Vector<String>& blacklist,
                                         UserContentInjectedFrames injectedFrames,
                                         UserStyleLevel level,
                                         UserStyleInjectionTime injectionTime)
{
    ASSERT_ARG(world, world);

    OwnPtr<UserStyleSheet> userStyleSheet = adoptPtr(new UserStyleSheet(source, url, whitelist, blacklist, injectedFrames, level));
    if (!m_userStyleSheets)
        m_userStyleSheets = adoptPtr(new UserStyleSheetMap);
    OwnPtr<UserStyleSheetVector>& styleSheetsInWorld = m_userStyleSheets->add(world, nullptr).iterator->value;
    if (!styleSheetsInWorld)
        styleSheetsInWorld = adoptPtr(new UserStyleSheetVector);
    styleSheetsInWorld->append(userStyleSheet.release());

    if (injectionTime == InjectInExistingDocuments)
        invalidatedInjectedStyleSheetCacheInAllFrames();
}

void PageGroup::removeUserScriptFromWorld(DOMWrapperWorld* world, const KURL& url)
{
    ASSERT_ARG(world, world);

    if (!m_userScripts)
        return;

    UserScriptMap::iterator it = m_userScripts->find(world);
    if (it == m_userScripts->end())
        return;
    
    UserScriptVector* scripts = it->value.get();
    for (int i = scripts->size() - 1; i >= 0; --i) {
        if (scripts->at(i)->url() == url)
            scripts->remove(i);
    }
    
    if (scripts->isEmpty())
        m_userScripts->remove(it);
}

void PageGroup::removeUserStyleSheetFromWorld(DOMWrapperWorld* world, const KURL& url)
{
    ASSERT_ARG(world, world);

    if (!m_userStyleSheets)
        return;

    UserStyleSheetMap::iterator it = m_userStyleSheets->find(world);
    bool sheetsChanged = false;
    if (it == m_userStyleSheets->end())
        return;
    
    UserStyleSheetVector* stylesheets = it->value.get();
    for (int i = stylesheets->size() - 1; i >= 0; --i) {
        if (stylesheets->at(i)->url() == url) {
            stylesheets->remove(i);
            sheetsChanged = true;
        }
    }
        
    if (!sheetsChanged)
        return;

    if (stylesheets->isEmpty())
        m_userStyleSheets->remove(it);

    invalidatedInjectedStyleSheetCacheInAllFrames();
}

void PageGroup::removeUserScriptsFromWorld(DOMWrapperWorld* world)
{
    ASSERT_ARG(world, world);

    if (!m_userScripts)
        return;

    UserScriptMap::iterator it = m_userScripts->find(world);
    if (it == m_userScripts->end())
        return;
       
    m_userScripts->remove(it);
}

void PageGroup::removeUserStyleSheetsFromWorld(DOMWrapperWorld* world)
{
    ASSERT_ARG(world, world);

    if (!m_userStyleSheets)
        return;
    
    UserStyleSheetMap::iterator it = m_userStyleSheets->find(world);
    if (it == m_userStyleSheets->end())
        return;
    
    m_userStyleSheets->remove(it);

    invalidatedInjectedStyleSheetCacheInAllFrames();
}

void PageGroup::removeAllUserContent()
{
    m_userScripts.clear();

    if (m_userStyleSheets) {
        m_userStyleSheets.clear();
        invalidatedInjectedStyleSheetCacheInAllFrames();
    }
}

void PageGroup::invalidatedInjectedStyleSheetCacheInAllFrames()
{
    // Clear our cached sheets and have them just reparse.
    HashSet<Page*>::const_iterator end = m_pages.end();
    for (HashSet<Page*>::const_iterator it = m_pages.begin(); it != end; ++it) {
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext())
            frame->document()->styleSheetCollection()->invalidateInjectedStyleSheetCache();
    }
}

#if ENABLE(VIDEO_TRACK)
void PageGroup::captionPreferencesChanged()
{
    for (HashSet<Page*>::iterator i = m_pages.begin(); i != m_pages.end(); ++i)
        (*i)->captionPreferencesChanged();
    pageCache()->markPagesForCaptionPreferencesChanged();
}

CaptionUserPreferences* PageGroup::captionPreferences()
{
    if (!m_captionPreferences)
#if (PLATFORM(MAC) && !PLATFORM(IOS)) || HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
        m_captionPreferences = CaptionUserPreferencesMediaAF::create(this);
#else
        m_captionPreferences = CaptionUserPreferences::create(this);
#endif

    return m_captionPreferences.get();
}

#endif

} // namespace WebCore
