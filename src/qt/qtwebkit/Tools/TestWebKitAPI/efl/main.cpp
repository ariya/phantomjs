/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestsController.h"
#include <Ecore.h>
#include <Eina.h>
#include <getopt.h>
#include <wtf/Assertions.h>

bool useX11Window = false;

static bool checkForUseX11WindowArgument(int argc, char** argv)
{
    int hasUseX11Window = 0;

    static const option options[] = {
        {"useX11Window", no_argument, &hasUseX11Window, 1},
        {0, 0, 0, 0}
    };

    while (getopt_long(argc, argv, "", options, 0) != -1) { }

    return hasUseX11Window;
}

int main(int argc, char** argv)
{
    WTFInstallReportBacktraceOnCrashHook();

    if (!eina_init())
        return EXIT_FAILURE;

    if (!ecore_init())
        return EXIT_FAILURE;

    useX11Window = checkForUseX11WindowArgument(argc, argv);

    int returnCode = TestWebKitAPI::TestsController::shared().run(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE;

    ecore_shutdown();
    eina_shutdown();

    return returnCode;
}
