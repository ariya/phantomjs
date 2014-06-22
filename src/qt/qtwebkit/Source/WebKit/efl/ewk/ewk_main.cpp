/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics

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
#include "ewk_main.h"

#include "FileSystem.h"
#include "InitializeLogging.h"
#include "PageCache.h"
#include "PageGroup.h"
#include "PlatformStrategiesEfl.h"
#include "ResourceHandle.h"
#include "ScriptController.h"
#include "Settings.h"
#include "StorageTracker.h"
#include "StorageTrackerClientEfl.h"
#include "ewk_auth_soup_private.h"
#include "ewk_network.h"
#include "ewk_private.h"
#include "ewk_settings.h"
#include "ewk_settings_private.h"
#include "runtime/InitializeThreading.h"
#include "runtime/Operations.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Efreet.h>
#include <Eina.h>
#include <Evas.h>
#include <glib-object.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <wtf/Threading.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

static int _ewkInitCount = 0;

/**
 * \var     _ewk_log_dom
 * @brief   the log domain identifier that is used with EINA's macros
 */
int _ewk_log_dom = -1;

static Eina_Bool _ewk_init_body(void);

int ewk_init(void)
{
    if (_ewkInitCount)
        return ++_ewkInitCount;

    if (!eina_init())
        goto error_eina;

    _ewk_log_dom = eina_log_domain_register("ewebkit", EINA_COLOR_ORANGE);
    if (_ewk_log_dom < 0) {
        EINA_LOG_CRIT("could not register log domain 'ewebkit'");
        goto error_log_domain;
    }

    if (!evas_init()) {
        CRITICAL("could not init evas.");
        goto error_evas;
    }

    if (!ecore_init()) {
        CRITICAL("could not init ecore.");
        goto error_ecore;
    }

    if (!ecore_evas_init()) {
        CRITICAL("could not init ecore_evas.");
        goto error_ecore_evas;
    }

    if (!edje_init()) {
        CRITICAL("could not init edje.");
        goto error_edje;
    }

#ifdef HAVE_ECORE_X
    if (!ecore_x_init(0)) {
        CRITICAL("could not init ecore_x.");
        goto error_ecore_x;
    }
#endif

    if (!_ewk_init_body()) {
        CRITICAL("could not init body");
        goto error_edje;
    }

    return ++_ewkInitCount;

#ifdef HAVE_ECORE_X
error_ecore_x:
    edje_shutdown();
#endif
error_edje:
    ecore_evas_shutdown();
error_ecore_evas:
    ecore_shutdown();
error_ecore:
    evas_shutdown();
error_evas:
    eina_log_domain_unregister(_ewk_log_dom);
    _ewk_log_dom = -1;
error_log_domain:
    eina_shutdown();
error_eina:
    return 0;
}

int ewk_shutdown(void)
{
    _ewkInitCount--;
    if (_ewkInitCount)
        return _ewkInitCount;

#ifdef HAVE_ECORE_X
    ecore_x_shutdown();
#endif
    edje_shutdown();
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
    eina_log_domain_unregister(_ewk_log_dom);
    _ewk_log_dom = -1;
    eina_shutdown();

    return 0;
}

static WebCore::StorageTrackerClientEfl* trackerClient()
{
    DEFINE_STATIC_LOCAL(WebCore::StorageTrackerClientEfl, trackerClient, ());
    return &trackerClient;
}

Eina_Bool _ewk_init_body(void)
{
#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif

    if (!ecore_main_loop_glib_integrate())
        WARN("Ecore was not compiled with GLib support, some plugins will not "
            "work (ie: Adobe Flash)");

    WebCore::ScriptController::initializeThreading();
#if !LOG_DISABLED
    WebCore::initializeLoggingChannelsIfNecessary();
#endif // !LOG_DISABLED
    WebCore::Settings::setDefaultMinDOMTimerInterval(0.004);

    PlatformStrategiesEfl::initialize();

    // Page cache capacity (in pages). Comment from Mac port:
    // (Research indicates that value / page drops substantially after 3 pages.)
    // FIXME: Calculate based on available resources
    ewk_settings_page_cache_capacity_set(3);
    WebCore::PageGroup::setShouldTrackVisitedLinks(true);

    String localStorageDirectory = String::fromUTF8(efreet_data_home_get()) + "/WebKitEfl/LocalStorage";
    String webDatabaseDirectory = String::fromUTF8(efreet_cache_home_get()) + "/WebKitEfl/Databases";
    String applicationCacheDirectory = String::fromUTF8(efreet_cache_home_get()) + "/WebKitEfl/Applications";
    String fileSystemDirectory = String::fromUTF8(efreet_data_home_get()) + "/WebKitEfl/FileSystem";

    ewk_settings_local_storage_path_set(localStorageDirectory.utf8().data());
    ewk_settings_web_database_path_set(webDatabaseDirectory.utf8().data());
    ewk_settings_application_cache_path_set(applicationCacheDirectory.utf8().data());
    ewk_settings_file_system_path_set(fileSystemDirectory.utf8().data());

    ewk_network_tls_certificate_check_set(false);

    WebCore::StorageTracker::initializeTracker(localStorageDirectory.utf8().data(), trackerClient());

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupSessionFeature* auth_dialog = static_cast<SoupSessionFeature*>(g_object_new(EWK_TYPE_SOUP_AUTH_DIALOG, 0));
    soup_session_add_feature(session, auth_dialog);

    WebCore::ResourceHandle::setIgnoreSSLErrors(true);

    return true;
}
