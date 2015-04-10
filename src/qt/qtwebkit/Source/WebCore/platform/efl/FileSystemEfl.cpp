/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd. All rights reserved.
 * Copyright (C) 2008 Kenneth Rohde Christiansen.
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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
#include "FileSystem.h"

#include <Ecore.h>
#include <Ecore_File.h>
#include <Eina.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fnmatch.h>
#include <glib.h> // TODO: remove me after following TODO is solved.
#include <limits.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

CString fileSystemRepresentation(const String& path)
{
// WARNING: this is just used by platform/network/soup, thus must be GLIB!!!
// TODO: move this to CString and use it instead in both, being more standard
#if !PLATFORM(WIN_OS) && defined(WTF_USE_SOUP)
    char* filename = g_uri_unescape_string(path.utf8().data(), 0);
    CString cfilename(filename);
    g_free(filename);
    return cfilename;
#else
    return path.utf8();
#endif
}

bool unloadModule(PlatformModule module)
{
    // caution, closing handle will make memory vanish and any remaining
    // timer, idler, threads or any other left-over will crash,
    // maybe just ignore this is a safer solution?
    return eina_module_free(module);
}

String homeDirectoryPath()
{
    const char *home = getenv("HOME");
    if (!home) {
        home = getenv("TMPDIR");
        if (!home)
            home = "/tmp";
    }
    return String::fromUTF8(home);
}

Vector<String> listDirectory(const String& path, const String& filter)
{
    Vector<String> matchingEntries;
    CString cfilter = filter.utf8();
    const char *f_name;

    Eina_Iterator* it = eina_file_ls(path.utf8().data());
    // FIXME: Early return if the iterator is null to avoid error messages from eina_iterator_free().
    // This check can be removed once the magic check on _free() removed in Eina.
    // http://www.mail-archive.com/enlightenment-devel@lists.sourceforge.net/msg42944.html
    if (!it)
        return matchingEntries;

    EINA_ITERATOR_FOREACH(it, f_name) {
        if (!fnmatch(cfilter.data(), f_name, 0))
            matchingEntries.append(String::fromUTF8(f_name));
        eina_stringshare_del(f_name);
    }
    eina_iterator_free(it);

    return matchingEntries;
}

uint64_t getVolumeFreeSizeForPath(const char* path)
{
    struct statvfs buf;
    if (statvfs(path, &buf) < 0)
        return 0;

    return buf.f_bavail * buf.f_bsize;
}

}
