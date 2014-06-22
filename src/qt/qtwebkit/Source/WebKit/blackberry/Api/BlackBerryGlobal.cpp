/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "BlackBerryGlobal.h"

#include "ApplicationCacheStorage.h"
#include "CacheClientBlackBerry.h"
#include "CookieManager.h"
#include "CrossOriginPreflightResultCache.h"
#include "FontCache.h"
#include "ImageSource.h"
#include "InitializeLogging.h"
#include "InitializeThreading.h"
#include "JSDOMWindow.h"
#include "VM.h"
#include "MemoryCache.h"
#include "NetworkStateNotifier.h"
#include "PageCache.h"
#include "PageGroup.h"
#include "PlatformStrategiesBlackBerry.h"
#include "Settings.h"
#include "TextureCacheCompositingThread.h"
#include "bindings/js/GCController.h"
#include "runtime/JSLock.h"
#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformString.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static bool gIsGlobalInitialized = false;

// Global initialization of various WebKit singletons.
void globalInitialize()
{
    if (gIsGlobalInitialized)
        return;
    gIsGlobalInitialized = true;

    WebCore::Settings::setHiddenPageDOMTimerAlignmentInterval(BlackBerry::Platform::Settings::instance()->isChromeProcess() ? 0 : 1);
#if ENABLE(BLACKBERRY_DEBUG_MEMORY)
    blackberryDebugInitialize();
#endif

#if !LOG_DISABLED
    // Turn on logging.
    initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED

    // Initialize threading.
    JSC::initializeThreading();

    // Normally this is called from initializeThreading, but we're using ThreadingNone
    // we're grabbing callOnMainThread without using the rest of the threading support.
    WTF::initializeMainThread();

    // Initialize our platform strategies.
    PlatformStrategiesBlackBerry::initialize();

    // Set the minimal timer interval to 4 milliseconds.
    WebCore::Settings::setDefaultMinDOMTimerInterval(0.004);

    // Track visited links.
    PageGroup::setShouldTrackVisitedLinks(true);

    CacheClientBlackBerry::get()->initialize();

    BlackBerry::Platform::Settings* settings = BlackBerry::Platform::Settings::instance();

    ImageSource::setMaxPixelsPerDecodedImage(settings->maxPixelsPerDecodedImage());
    updateOnlineStatus(settings->isNetworkAvailable());
}

void collectJavascriptGarbageNow()
{
    if (gIsGlobalInitialized)
        gcController().garbageCollectNow();
}

void clearCookieCache()
{
    cookieManager().removeAllCookies(RemoveFromBackingStore);
}

#if USE(ACCELERATED_COMPOSITING)
static void clearMemoryCachesInCompositingThread()
{
    textureCacheCompositingThread()->prune(0);
}
#endif

void clearMemoryCaches()
{
#if USE(ACCELERATED_COMPOSITING)
    // Call textureCacheCompositingThread()->prune(0) in UI thread.
    BlackBerry::Platform::userInterfaceThreadMessageClient()->dispatchMessage(BlackBerry::Platform::createFunctionCallMessage(clearMemoryCachesInCompositingThread));
#endif

    {
        JSC::JSLockHolder lock(JSDOMWindow::commonVM());
        collectJavascriptGarbageNow();
    }

    // Clean caches after JS garbage collection because JS GC can
    // generate more dead resources.
    int capacity = pageCache()->capacity();
    pageCache()->setCapacity(0);
    pageCache()->setCapacity(capacity);

    CrossOriginPreflightResultCache::shared().empty();

    if (!memoryCache()->disabled()) {
        // Evict all dead resources and prune live resources.
        memoryCache()->setCapacities(0, 0, 0);

        // Update cache capacity based on current memory status.
        CacheClientBlackBerry::get()->updateCacheCapacity();
    }

    fontCache()->invalidate();
}

void clearAppCache(const BlackBerry::Platform::String&)
{
    cacheStorage().empty();
}

void clearDatabase(const BlackBerry::Platform::String&)
{
}

void updateOnlineStatus(bool online)
{
    networkStateNotifier().networkStateChange(online);
}

bool isRunningDrt()
{
#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD
    return getenv("drtRun");
#else
    return false;
#endif
}

}
}
