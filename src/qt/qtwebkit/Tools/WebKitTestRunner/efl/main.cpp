/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "EWebKit2.h"
#include "TestController.h"
#include <wtf/Assertions.h>
#include <stdlib.h>

#ifdef HAVE_ECORE_X
#include <X11/Xlib.h>
#include <X11/extensions/Xext.h>

static int dummyExtensionErrorHandler(Display*, _Xconst char*, _Xconst char*)
{
    return 0;
}
#endif

int main(int argc, char** argv)
{
    WTFInstallReportBacktraceOnCrashHook();

#ifdef HAVE_ECORE_X
    XSetExtensionErrorHandler(dummyExtensionErrorHandler);
#endif

    if (!ewk_init())
        return 1;

    {
        // Test controller has own ptr containing WebView and WebView must be deleted
        // before its evas object is deleted. Call of ewk_shutdown() leads to evas objects deletion.

        WTR::TestController controller(argc, const_cast<const char**>(argv));
    }

    ewk_shutdown();

    return 0;
}

