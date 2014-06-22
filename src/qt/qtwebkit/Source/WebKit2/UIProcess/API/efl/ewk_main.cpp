/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics
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
#include "ewk_main.h"

#include "ewk_private.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <Edje.h>
#include <Eina.h>
#include <Evas.h>
#include <glib-object.h>
#include <glib.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

static int _ewkInitCount = 0;

/**
 * \var     _ewk_log_dom
 * @brief   the log domain identifier that is used with EINA's macros
 */
int _ewk_log_dom = -1;

int ewk_init(void)
{
    if (_ewkInitCount)
        return ++_ewkInitCount;

    if (!eina_init())
        goto error_eina;

    _ewk_log_dom = eina_log_domain_register("ewebkit2", EINA_COLOR_ORANGE);
    if (_ewk_log_dom < 0) {
        EINA_LOG_CRIT("could not register log domain 'ewebkit2'");
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

    if (!ecore_imf_init()) {
        CRITICAL("could not init ecore_imf.");
        goto error_ecore_imf;
    }

#ifdef HAVE_ECORE_X
    if (!ecore_x_init(0)) {
        CRITICAL("could not init ecore_x.");
        goto error_ecore_x;
    }
#endif

    if (!edje_init()) {
        CRITICAL("Could not init edje.");
        goto error_edje;
    }

#if !GLIB_CHECK_VERSION(2, 35, 0)
    g_type_init();
#endif

    if (!ecore_main_loop_glib_integrate()) {
        WARN("Ecore was not compiled with GLib support, some plugins will not "
            "work (ie: Adobe Flash)");
    }

    return ++_ewkInitCount;

error_edje:
#ifdef HAVE_ECORE_X
    ecore_x_shutdown();
error_ecore_x:
#else
    ecore_imf_shutdown();
#endif
error_ecore_imf:
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
    if (--_ewkInitCount)
        return _ewkInitCount;

    edje_shutdown();
#ifdef HAVE_ECORE_X
    ecore_x_shutdown();
#endif
    ecore_imf_shutdown();
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
    eina_log_domain_unregister(_ewk_log_dom);
    _ewk_log_dom = -1;
    eina_shutdown();

    return 0;
}
