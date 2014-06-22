/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Apple Inc.
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginProcessMainUnix.h"

#if ENABLE(PLUGIN_PROCESS)

#include "Logging.h"
#include "NetscapePlugin.h"
#include "PluginProcess.h"
#include "WebKit2Initialize.h"
#include <WebCore/RunLoop.h>
#if PLATFORM(GTK)
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#elif PLATFORM(EFL) && HAVE_ECORE_X
#include <Ecore_X.h>
#endif

using namespace WebCore;

namespace WebKit {

#ifdef XP_UNIX

#if !LOG_DISABLED
static const char xErrorString[] = "The program '%s' received an X Window System error.\n"
    "This probably reflects a bug in a browser plugin.\n"
    "The error was '%s'.\n"
    "  (Details: serial %ld error_code %d request_code %d minor_code %d)\n";
#endif /* !LOG_DISABLED */

static char* programName = 0;

static int webkitXError(Display* xdisplay, XErrorEvent* error)
{
    char errorMessage[64];
    XGetErrorText(xdisplay, error->error_code, errorMessage, 63);

    LOG(Plugins, xErrorString,
        programName, errorMessage,
        error->serial, error->error_code,
        error->request_code, error->minor_code);

    return 0;
}
#endif

WK_EXPORT int PluginProcessMainUnix(int argc, char* argv[])
{
    bool scanPlugin = !strcmp(argv[1], "-scanPlugin");
    ASSERT_UNUSED(argc, argc == 3);

#if PLATFORM(GTK)
    gtk_init(&argc, &argv);
#elif PLATFORM(EFL)
#ifdef HAVE_ECORE_X
    if (!ecore_x_init(0))
#endif
        return 1;
#endif

    InitializeWebKit2();

    if (scanPlugin) {
        String pluginPath(argv[2]);
        if (!NetscapePluginModule::scanPlugin(pluginPath))
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    // Plugins can produce X errors that are handled by the GDK X error handler, which
    // exits the process. Since we don't want to crash due to plugin bugs, we install a
    // custom error handler to show a warning when a X error happens without aborting.
#if defined(XP_UNIX)
    programName = basename(argv[0]);
    XSetErrorHandler(webkitXError);
#endif

    int socket = atoi(argv[1]);

    WebKit::ChildProcessInitializationParameters parameters;
    parameters.connectionIdentifier = socket;
    parameters.extraInitializationData.add("plugin-path", argv[2]);

    WebKit::PluginProcess::shared().initialize(parameters);

    RunLoop::run();

    return 0;
}

} // namespace WebKit

#endif
