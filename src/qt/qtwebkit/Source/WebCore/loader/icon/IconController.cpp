/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
 * Copyright (C) 2011 Kris Jordan <krisjordan@gmail.com>
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IconController.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "IconDatabase.h"
#include "IconDatabaseBase.h"
#include "IconLoader.h"
#include "IconURL.h"
#include "Logging.h"
#include "Page.h"
#include "Settings.h"

namespace WebCore {

IconController::IconController(Frame* frame)
    : m_frame(frame)
    , m_waitingForLoadDecision(false)
{
}

IconController::~IconController()
{
}

KURL IconController::url()
{
    IconURLs iconURLs = urlsForTypes(Favicon);
    return iconURLs.isEmpty() ? KURL() : iconURLs[0].m_iconURL;
}

IconURL IconController::iconURL(IconType iconType) const
{
    IconURL result;
    const Vector<IconURL>& iconURLs = m_frame->document()->iconURLs(iconType);
    Vector<IconURL>::const_iterator iter(iconURLs.begin());
    for (; iter != iconURLs.end(); ++iter) {
        if (result.m_iconURL.isEmpty() || !iter->m_mimeType.isEmpty())
            result = *iter;
    }

    return result;
}

IconURLs IconController::urlsForTypes(int iconTypesMask)
{
    IconURLs iconURLs;
    if (m_frame->tree() && m_frame->tree()->parent())
        return iconURLs;
        
    if (iconTypesMask & Favicon && !appendToIconURLs(Favicon, &iconURLs))
        iconURLs.append(defaultURL(Favicon));

#if ENABLE(TOUCH_ICON_LOADING)
    int missedIcons = 0;
    if (iconTypesMask & TouchPrecomposedIcon)
        missedIcons += appendToIconURLs(TouchPrecomposedIcon, &iconURLs) ? 0:1;

    if (iconTypesMask & TouchIcon)
      missedIcons += appendToIconURLs(TouchIcon, &iconURLs) ? 0:1;

    // Only return the default touch icons when the both were required and neither was gotten.
    if (missedIcons == 2) {
        iconURLs.append(defaultURL(TouchPrecomposedIcon));
        iconURLs.append(defaultURL(TouchIcon));
    }
#endif

    // Finally, append all remaining icons of this type.
    const Vector<IconURL>& allIconURLs = m_frame->document()->iconURLs(iconTypesMask);
    for (Vector<IconURL>::const_iterator iter = allIconURLs.begin(); iter != allIconURLs.end(); ++iter) {
        int i;
        int iconCount = iconURLs.size();
        for (i = 0; i < iconCount; ++i) {
            if (*iter == iconURLs.at(i))
                break;
        }
        if (i == iconCount)
            iconURLs.append(*iter);
    }

    return iconURLs;
}

void IconController::commitToDatabase(const KURL& icon)
{
    LOG(IconDatabase, "Committing iconURL %s to database for pageURLs %s and %s", icon.string().ascii().data(), m_frame->document()->url().string().ascii().data(), m_frame->loader()->initialRequest().url().string().ascii().data());
    iconDatabase().setIconURLForPageURL(icon.string(), m_frame->document()->url().string());
    iconDatabase().setIconURLForPageURL(icon.string(), m_frame->loader()->initialRequest().url().string());
}

void IconController::startLoader()
{
    // FIXME: We kick off the icon loader when the frame is done receiving its main resource.
    // But we should instead do it when we're done parsing the head element.
    if (!m_frame->loader()->isLoadingMainFrame())
        return;

    if (!iconDatabase().isEnabled())
        return;

    ASSERT(!m_frame->tree()->parent());
    if (!documentCanHaveIcon(m_frame->document()->url()))
        return;

    KURL iconURL(url());
    String urlString(iconURL.string());
    if (urlString.isEmpty())
        return;

    // People who want to avoid loading images generally want to avoid loading all images, unless an exception has been made for site icons.
    // Now that we've accounted for URL mapping, avoid starting the network load if images aren't set to display automatically.
    Settings* settings = m_frame->settings();
    if (settings && !settings->loadsImagesAutomatically() && !settings->loadsSiteIconsIgnoringImageLoadingSetting())
        return;

    // If we're reloading the page, always start the icon load now.
    // FIXME: How can this condition ever be true?
    if (m_frame->loader()->loadType() == FrameLoadTypeReload && m_frame->loader()->loadType() == FrameLoadTypeReloadFromOrigin) {
        continueLoadWithDecision(IconLoadYes);
        return;
    }

    if (iconDatabase().supportsAsynchronousMode()) {
        m_frame->loader()->documentLoader()->getIconLoadDecisionForIconURL(urlString);
        // Commit the icon url mapping to the database just in case we don't end up loading later.
        commitToDatabase(iconURL);
        return;
    }

    IconLoadDecision decision = iconDatabase().synchronousLoadDecisionForIconURL(urlString, m_frame->loader()->documentLoader());

    if (decision == IconLoadUnknown) {
        // In this case, we may end up loading the icon later, but we still want to commit the icon url mapping to the database
        // just in case we don't end up loading later - if we commit the mapping a second time after the load, that's no big deal
        // We also tell the client to register for the notification that the icon is received now so it isn't missed in case the 
        // icon is later read in from disk
        LOG(IconDatabase, "IconController %p might load icon %s later", this, urlString.ascii().data());
        m_waitingForLoadDecision = true;    
        m_frame->loader()->client()->registerForIconNotification();
        commitToDatabase(iconURL);
        return;
    }

    continueLoadWithDecision(decision);
}

void IconController::stopLoader()
{
    if (m_iconLoader)
        m_iconLoader->stopLoading();
}

// Callback for the old-style synchronous IconDatabase interface.
void IconController::loadDecisionReceived(IconLoadDecision iconLoadDecision)
{
    if (!m_waitingForLoadDecision)
        return;
    LOG(IconDatabase, "IconController %p was told a load decision is available for its icon", this);
    continueLoadWithDecision(iconLoadDecision);
    m_waitingForLoadDecision = false;
}

void IconController::continueLoadWithDecision(IconLoadDecision iconLoadDecision)
{
    ASSERT(iconLoadDecision != IconLoadUnknown);

    //  FIXME (<rdar://problem/9168605>) - We should support in-memory-only private browsing icons in asynchronous icon database mode.
    if (iconDatabase().supportsAsynchronousMode() && m_frame->page()->settings()->privateBrowsingEnabled())
        return;

    if (iconLoadDecision == IconLoadNo) {
        KURL iconURL(url());
        String urlString(iconURL.string());
        if (urlString.isEmpty())
            return;

        LOG(IconDatabase, "IconController::startLoader() - Told not to load this icon, committing iconURL %s to database for pageURL mapping", urlString.ascii().data());
        commitToDatabase(iconURL);

        if (iconDatabase().supportsAsynchronousMode()) {
            m_frame->loader()->documentLoader()->getIconDataForIconURL(urlString);
            return;
        }

        // We were told not to load this icon - that means this icon is already known by the database
        // If the icon data hasn't been read in from disk yet, kick off the read of the icon from the database to make sure someone
        // has done it. This is after registering for the notification so the WebView can call the appropriate delegate method.
        // Otherwise if the icon data *is* available, notify the delegate
        if (!iconDatabase().synchronousIconDataKnownForIconURL(urlString)) {
            LOG(IconDatabase, "Told not to load icon %s but icon data is not yet available - registering for notification and requesting load from disk", urlString.ascii().data());
            m_frame->loader()->client()->registerForIconNotification();
            iconDatabase().synchronousIconForPageURL(m_frame->document()->url().string(), IntSize(0, 0));
            iconDatabase().synchronousIconForPageURL(m_frame->loader()->initialRequest().url().string(), IntSize(0, 0));
        } else
            m_frame->loader()->client()->dispatchDidReceiveIcon();

        return;
    } 

    if (!m_iconLoader)
        m_iconLoader = IconLoader::create(m_frame);

    m_iconLoader->startLoading();
}

bool IconController::appendToIconURLs(IconType iconType, IconURLs* iconURLs)
{
    IconURL faviconURL = iconURL(iconType);
    if (faviconURL.m_iconURL.isEmpty())
        return false;

    iconURLs->append(faviconURL);
    return true;
}

IconURL IconController::defaultURL(IconType iconType)
{
    // Don't return a favicon iconURL unless we're http or https
    KURL documentURL = m_frame->document()->url();
    if (!documentURL.protocolIsInHTTPFamily())
        return IconURL();

    KURL url;
    bool couldSetProtocol = url.setProtocol(documentURL.protocol());
    ASSERT_UNUSED(couldSetProtocol, couldSetProtocol);
    url.setHost(documentURL.host());
    if (documentURL.hasPort())
        url.setPort(documentURL.port());

    if (iconType == Favicon) {
        url.setPath("/favicon.ico");
        return IconURL::defaultIconURL(url, Favicon);
    }
#if ENABLE(TOUCH_ICON_LOADING)
    if (iconType == TouchPrecomposedIcon) {
        url.setPath("/apple-touch-icon-precomposed.png");
        return IconURL::defaultIconURL(url, TouchPrecomposedIcon);
    }
    if (iconType == TouchIcon) {
        url.setPath("/apple-touch-icon.png");
        return IconURL::defaultIconURL(url, TouchIcon);
    }
#endif
    return IconURL();
}

}
