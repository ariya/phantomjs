/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

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
#include "ewk_settings.h"

#include "ApplicationCacheStorage.h"
#include "CairoUtilitiesEfl.h"
#include "CrossOriginPreflightResultCache.h"
#include "DatabaseManager.h"
#include "FontCache.h"
#include "FrameView.h"
#include "GCController.h"
#include "IconDatabase.h"
#include "Image.h"
#include "IntSize.h"
#include "KURL.h"
#include "LocalFileSystem.h"
#include "MemoryCache.h"
#include "PageCache.h"
#include "RuntimeEnabledFeatures.h"
#include "Settings.h"
#include "StorageThread.h"
#include "StorageTracker.h"
#include "WebKitVersion.h"
#include "WorkerThread.h"
#include "ewk_private.h"
#include <Eina.h>
#include <eina_safety_checks.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <wtf/FastMalloc.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

#if OS(UNIX)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#elif OS(WINDOWS)
#include "SystemInfo.h"
#endif

static const char* s_offlineAppCachePath = 0;

static const char* _ewk_icon_database_path = 0;

static const char* s_webDatabasePath = 0;
static const char* s_localStoragePath = 0;
static const char* s_cssMediaType = 0;
static uint64_t s_webDatabaseQuota = 1 * 1024 * 1024; // 1MB.

static WTF::String _ewk_settings_webkit_platform_get()
{
    WTF::String uaPlatform;
#if PLATFORM(X11)
    uaPlatform = "X11";
#else
    uaPlatform = "Unknown";
#endif
    return uaPlatform;
}

static WTF::String _ewk_settings_webkit_os_version_get()
{
    WTF::String uaOsVersion;
#if OS(DARWIN)
#if CPU(X86) || CPU(X86_64)
    uaOsVersion = "Intel Mac OS X";
#else
    uaOsVersion = "PPC Mac OS X";
#endif
#elif OS(UNIX)
    struct utsname name;

    if (uname(&name) != -1)
        uaOsVersion = makeString(name.sysname, ' ', name.machine);
    else
        uaOsVersion = "Unknown";
#elif OS(WINDOWS)
    uaOsVersion = windowsVersionFroUAString();
#else
    uaOsVersion = "Unknown";
#endif
    return uaOsVersion;
}

uint64_t ewk_settings_web_database_default_quota_get()
{
    return s_webDatabaseQuota;
}

void ewk_settings_web_database_default_quota_set(uint64_t maximumSize)
{
    s_webDatabaseQuota = maximumSize;
}

void ewk_settings_local_storage_path_set(const char* path)
{
    WebCore::StorageTracker::tracker().setDatabaseDirectoryPath(WTF::String::fromUTF8(path));
    eina_stringshare_replace(&s_localStoragePath, path);
}

const char* ewk_settings_local_storage_path_get(void)
{
    return s_localStoragePath;
}

void ewk_settings_local_storage_database_clear()
{
    WebCore::StorageTracker::tracker().deleteAllOrigins();
}

void ewk_settings_local_storage_database_origin_clear(const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN(url);

    const WebCore::KURL kurl(WebCore::KURL(), WTF::String::fromUTF8(url));
    WebCore::StorageTracker::tracker().deleteOrigin(WebCore::SecurityOrigin::create(kurl).get());
}

void ewk_settings_web_database_path_set(const char* path)
{
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().setDatabaseDirectoryPath(WTF::String::fromUTF8(path));
    eina_stringshare_replace(&s_webDatabasePath, path);
#endif
}

const char* ewk_settings_web_database_path_get(void)
{
    return s_webDatabasePath;
}

Eina_Bool ewk_settings_icon_database_path_set(const char* directory)
{
    WebCore::IconDatabase::delayDatabaseCleanup();

    if (directory) {
        if (WebCore::iconDatabase().isEnabled()) {
            ERR("IconDatabase is already open: %s", _ewk_icon_database_path);
            return false;
        }

        struct stat st;

        if (stat(directory, &st)) {
            ERR("could not stat(%s): %s", directory, strerror(errno));
            return false;
        }

        if (!S_ISDIR(st.st_mode)) {
            ERR("not a directory: %s", directory);
            return false;
        }

        if (access(directory, R_OK | W_OK)) {
            ERR("could not access directory '%s' for read and write: %s",
                directory, strerror(errno));
            return false;
        }

        WebCore::iconDatabase().setEnabled(true);
        WebCore::iconDatabase().open(WTF::String::fromUTF8(directory), WebCore::IconDatabase::defaultDatabaseFilename());

        eina_stringshare_replace(&_ewk_icon_database_path, directory);
    } else {
        WebCore::iconDatabase().setEnabled(false);
        WebCore::iconDatabase().close();

        eina_stringshare_del(_ewk_icon_database_path);
        _ewk_icon_database_path = 0;
    }
    return true;
}

const char* ewk_settings_icon_database_path_get(void)
{
    return _ewk_icon_database_path;
}

Eina_Bool ewk_settings_icon_database_clear(void)
{
    if (!WebCore::iconDatabase().isEnabled())
        return false;
    if (!WebCore::iconDatabase().isOpen())
        return false;

    WebCore::iconDatabase().removeAllIcons();
    return true;
}

cairo_surface_t* ewk_settings_icon_database_icon_surface_get(const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, 0);

    WebCore::KURL kurl(WebCore::KURL(), WTF::String::fromUTF8(url));
    RefPtr<cairo_surface_t> icon = WebCore::iconDatabase().synchronousNativeIconForPageURL(kurl.string(), WebCore::IntSize(16, 16));
    if (!icon)
        ERR("no icon for url %s", url);

    return icon.get();
}

Evas_Object* ewk_settings_icon_database_icon_object_get(const char* url, Evas* canvas)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    WebCore::KURL kurl(WebCore::KURL(), WTF::String::fromUTF8(url));
    RefPtr<cairo_surface_t> surface = WebCore::iconDatabase().synchronousNativeIconForPageURL(kurl.string(), WebCore::IntSize(16, 16));

    if (!surface) {
        ERR("no icon for url %s", url);
        return 0;
    }

    return surface ? WebCore::evasObjectFromCairoImageSurface(canvas, surface.get()).leakRef() : 0;
}

void ewk_settings_object_cache_capacity_set(unsigned minDeadCapacity, unsigned maxDeadCapacity, unsigned totalCapacity)
{
    WebCore::memoryCache()->setCapacities(minDeadCapacity, maxDeadCapacity, totalCapacity);
}

Eina_Bool ewk_settings_object_cache_enable_get()
{
    return !WebCore::memoryCache()->disabled();
}

void ewk_settings_object_cache_enable_set(Eina_Bool enable)
{
    WebCore::memoryCache()->setDisabled(!enable);
}

Eina_Bool ewk_settings_shadow_dom_enable_get()
{
#if ENABLE(SHADOW_DOM)
    return WebCore::RuntimeEnabledFeatures::shadowDOMEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_shadow_dom_enable_set(Eina_Bool enable)
{
#if ENABLE(SHADOW_DOM)
    enable = !!enable;
    WebCore::RuntimeEnabledFeatures::setShadowDOMEnabled(enable);
    return true;
#else
    UNUSED_PARAM(enable);
    return false;
#endif
}

unsigned ewk_settings_page_cache_capacity_get()
{
    return WebCore::pageCache()->capacity();
}

void ewk_settings_page_cache_capacity_set(unsigned pages)
{
    WebCore::pageCache()->setCapacity(pages);
}

void ewk_settings_memory_cache_clear()
{
    // Turn the cache on and off. Disabling the object cache will remove all
    // resources from the cache. They may still live on if they are referenced
    // by some Web page though.
    if (!WebCore::memoryCache()->disabled()) {
        WebCore::memoryCache()->setDisabled(true);
        WebCore::memoryCache()->setDisabled(false);
    }

    int pageCapacity = WebCore::pageCache()->capacity();
    // Setting size to 0, makes all pages be released.
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->setCapacity(pageCapacity);

    // Invalidating the font cache and freeing all inactive font data.
    WebCore::fontCache()->invalidate();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::shared().empty();

    // Drop JIT compiled code from ExecutableAllocator.
    WebCore::gcController().discardAllCompiledCode();
    // Garbage Collect to release the references of CachedResource from dead objects.
    WebCore::gcController().garbageCollectNow();

    // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
    WebCore::StorageThread::releaseFastMallocFreeMemoryInAllThreads();
#if ENABLE(WORKERS)
    WebCore::WorkerThread::releaseFastMallocFreeMemoryInAllThreads();
#endif
    WTF::releaseFastMallocFreeMemory();
}

void ewk_settings_repaint_throttling_set(double deferredRepaintDelay, double initialDeferredRepaintDelayDuringLoading, double maxDeferredRepaintDelayDuringLoading, double deferredRepaintDelayIncrementDuringLoading)
{
    WebCore::FrameView::setRepaintThrottlingDeferredRepaintDelay(deferredRepaintDelay);
    WebCore::FrameView::setRepaintThrottlingnInitialDeferredRepaintDelayDuringLoading(initialDeferredRepaintDelayDuringLoading);
    WebCore::FrameView::setRepaintThrottlingMaxDeferredRepaintDelayDuringLoading(maxDeferredRepaintDelayDuringLoading);
    WebCore::FrameView::setRepaintThrottlingDeferredRepaintDelayIncrementDuringLoading(deferredRepaintDelayIncrementDuringLoading);
}

/**
 * @internal
 *
 * Gets the default user agent string.
 *
 * @return a pointer to an eina_stringshare containing the user agent string
 */
const char* ewk_settings_default_user_agent_get()
{
    WTF::String uaVersion = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION) + '+';
    WTF::String staticUa = "Mozilla/5.0 (" + _ewk_settings_webkit_platform_get() + "; " + _ewk_settings_webkit_os_version_get() + ") AppleWebKit/" + uaVersion + " (KHTML, like Gecko) Version/5.0 Safari/" + uaVersion;

    return eina_stringshare_add(staticUa.utf8().data());
}

/**
 * @internal
 *
 * Sets the given path to the directory where WebKit will write for
 * the HTML5 file system API.
 *
 * @param path the new file system directory path
 */
void ewk_settings_file_system_path_set(const char* path)
{
#if ENABLE(FILE_SYSTEM)
    WebCore::LocalFileSystem::initializeLocalFileSystem(String::fromUTF8(path));
#else
    UNUSED_PARAM(path);
#endif
}

void ewk_settings_application_cache_path_set(const char* path)
{
    WebCore::cacheStorage().setCacheDirectory(WTF::String::fromUTF8(path));
    eina_stringshare_replace(&s_offlineAppCachePath, path);
}

const char* ewk_settings_application_cache_path_get()
{
    return s_offlineAppCachePath;
}

int64_t ewk_settings_application_cache_max_quota_get()
{
    return WebCore::cacheStorage().maximumSize();
}

void ewk_settings_application_cache_max_quota_set(int64_t maximumSize)
{
    ewk_settings_application_cache_clear();

    WebCore::cacheStorage().setMaximumSize(maximumSize);
}

void ewk_settings_application_cache_clear()
{
    WebCore::cacheStorage().deleteAllEntries();
}

double ewk_settings_default_timer_interval_get(void)
{
    return WebCore::Settings::defaultMinDOMTimerInterval();
}

void ewk_settings_css_media_type_set(const char* type)
{
    eina_stringshare_replace(&s_cssMediaType, type);
}

const char* ewk_settings_css_media_type_get()
{
    return s_cssMediaType;
}
