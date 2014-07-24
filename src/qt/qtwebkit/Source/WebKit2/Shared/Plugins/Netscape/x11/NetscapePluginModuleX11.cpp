/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#if PLUGIN_ARCHITECTURE(X11) && ENABLE(NETSCAPE_PLUGIN_API)

#include "NetscapePluginModule.h"

#include "PluginProcessProxy.h"
#include "NetscapeBrowserFuncs.h"
#include <WebCore/FileSystem.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace WebCore;

namespace WebKit {

class StdoutDevNullRedirector {
public:
    StdoutDevNullRedirector();
    ~StdoutDevNullRedirector();

private:
    int m_savedStdout;
};

StdoutDevNullRedirector::StdoutDevNullRedirector()
    : m_savedStdout(-1)
{
    int newStdout = open("/dev/null", O_WRONLY);
    if (newStdout == -1)
        return;
    m_savedStdout = dup(STDOUT_FILENO);
    dup2(newStdout, STDOUT_FILENO);
}

StdoutDevNullRedirector::~StdoutDevNullRedirector()
{
    if (m_savedStdout != -1)
        dup2(m_savedStdout, STDOUT_FILENO);
}


static void parseMIMEDescription(const String& mimeDescription, Vector<MimeClassInfo>& result)
{
    ASSERT_ARG(result, result.isEmpty());

    Vector<String> types;
    mimeDescription.lower().split(UChar(';'), false, types);
    result.reserveInitialCapacity(types.size());

    size_t mimeInfoCount = 0;
    for (size_t i = 0; i < types.size(); ++i) {
        Vector<String> mimeTypeParts;
        types[i].split(UChar(':'), true, mimeTypeParts);
        if (mimeTypeParts.size() <= 0)
            continue;

        result.uncheckedAppend(MimeClassInfo());
        MimeClassInfo& mimeInfo = result[mimeInfoCount++];
        mimeInfo.type = mimeTypeParts[0];

        if (mimeTypeParts.size() > 1)
            mimeTypeParts[1].split(UChar(','), false, mimeInfo.extensions);

        if (mimeTypeParts.size() > 2)
            mimeInfo.desc = mimeTypeParts[2];
    }
}

bool NetscapePluginModule::getPluginInfoForLoadedPlugin(RawPluginMetaData& metaData)
{
    ASSERT(m_isInitialized);

    Module* module = m_module.get();
    NPP_GetValueProcPtr NPP_GetValue = module->functionPointer<NPP_GetValueProcPtr>("NP_GetValue");
    if (!NPP_GetValue)
        return false;

    NP_GetMIMEDescriptionFuncPtr NP_GetMIMEDescription = module->functionPointer<NP_GetMIMEDescriptionFuncPtr>("NP_GetMIMEDescription");
    if (!NP_GetMIMEDescription)
        return false;

    char* buffer;
    NPError error = NPP_GetValue(0, NPPVpluginNameString, &buffer);
    if (error == NPERR_NO_ERROR)
        metaData.name = String::fromUTF8(buffer);

    error = NPP_GetValue(0, NPPVpluginDescriptionString, &buffer);
    if (error == NPERR_NO_ERROR)
        metaData.description = String::fromUTF8(buffer);

    String mimeDescription = String::fromUTF8(NP_GetMIMEDescription());
    if (mimeDescription.isNull())
        return false;

    metaData.mimeDescription = mimeDescription;

    return true;
}

bool NetscapePluginModule::getPluginInfo(const String& pluginPath, PluginModuleInfo& plugin)
{
    RawPluginMetaData metaData;
    if (!PluginProcessProxy::scanPlugin(pluginPath, metaData))
        return false;

    plugin.path = pluginPath;
    plugin.info.file = pathGetFileName(pluginPath);
    plugin.info.name = metaData.name;
    plugin.info.desc = metaData.description;
    parseMIMEDescription(metaData.mimeDescription, plugin.info.mimes);

    return true;
}

void NetscapePluginModule::determineQuirks()
{
#if CPU(X86_64)
    RawPluginMetaData metaData;
    if (!getPluginInfoForLoadedPlugin(metaData))
        return;

    Vector<MimeClassInfo> mimeTypes;
    parseMIMEDescription(metaData.mimeDescription, mimeTypes);
    for (size_t i = 0; i < mimeTypes.size(); ++i) {
        if (mimeTypes[i].type == "application/x-shockwave-flash") {
            m_pluginQuirks.add(PluginQuirks::IgnoreRightClickInWindowlessMode);
            break;
        }
    }
#endif
}

static String truncateToSingleLine(const String& string)
{
    unsigned oldLength = string.length();
    UChar* buffer;
    String stringBuffer(StringImpl::createUninitialized(oldLength + 1, buffer));

    unsigned newLength = 0;
    const UChar* start = string.characters();
    for (const UChar* c = start; c < start + oldLength; ++c) {
        if (*c != UChar('\n'))
            buffer[newLength++] = *c;
    }
    buffer[newLength++] = UChar('\n');

    String result = (newLength == oldLength + 1) ? stringBuffer : String(stringBuffer.characters16(), newLength);
    ASSERT(result.endsWith(UChar('\n')));
    return result;
}

bool NetscapePluginModule::scanPlugin(const String& pluginPath)
{
    RawPluginMetaData metaData;

    {
        // Don't allow the plugin to pollute the standard output.
        StdoutDevNullRedirector stdOutRedirector;

        // We are loading the plugin here since it does not seem to be a standardized way to
        // get the needed informations from a UNIX plugin without loading it.
        RefPtr<NetscapePluginModule> pluginModule = NetscapePluginModule::getOrCreate(pluginPath);
        if (!pluginModule)
            return false;

        pluginModule->incrementLoadCount();
        bool success = pluginModule->getPluginInfoForLoadedPlugin(metaData);
        pluginModule->decrementLoadCount();

        if (!success)
            return false;
    }

    // Write data to standard output for the UI process.
    String output[3] = {
        truncateToSingleLine(metaData.name),
        truncateToSingleLine(metaData.description),
        truncateToSingleLine(metaData.mimeDescription)
    };
    for (unsigned i = 0; i < 3; ++i) {
        const String& line = output[i];
        const char* current = reinterpret_cast<const char*>(line.characters16());
        const char* end = reinterpret_cast<const char*>(line.characters16()) + (line.length() * sizeof(UChar));
        while (current < end) {
            int result;
            while ((result = fputc(*current, stdout)) == EOF && errno == EINTR) { }
            ASSERT(result != EOF);
            ++current;
        }
    }

    fflush(stdout);

    return true;
}

} // namespace WebKit

#endif // PLUGIN_ARCHITECTURE(X11) && ENABLE(NETSCAPE_PLUGIN_API)
