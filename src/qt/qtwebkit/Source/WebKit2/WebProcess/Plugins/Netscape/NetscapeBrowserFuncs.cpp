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
#include "NetscapeBrowserFuncs.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "NPRuntimeUtilities.h"
#include "NetscapePlugin.h"
#include "PluginController.h"
#include <WebCore/HTTPHeaderMap.h>
#include <WebCore/IdentifierRep.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/ProtectionSpace.h>
#include <WebCore/SharedBuffer.h>
#include <utility>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

// Helper class for delaying destruction of a plug-in.
class PluginDestructionProtector {
public:
    explicit PluginDestructionProtector(NetscapePlugin* plugin)
    {
        if (plugin)
            m_protector = adoptPtr(new PluginController::PluginDestructionProtector(static_cast<Plugin*>(plugin)->controller()));
    }
    
private:
    OwnPtr<PluginController::PluginDestructionProtector> m_protector;
};

static bool startsWithBlankLine(const char* bytes, unsigned length)
{
    return length > 0 && bytes[0] == '\n';
}

static int locationAfterFirstBlankLine(const char* bytes, unsigned length)
{
    for (unsigned i = 0; i < length - 4; i++) {
        // Support for Acrobat. It sends "\n\n".
        if (bytes[i] == '\n' && bytes[i + 1] == '\n')
            return i + 2;
        
        // Returns the position after 2 CRLF's or 1 CRLF if it is the first line.
        if (bytes[i] == '\r' && bytes[i + 1] == '\n') {
            i += 2;
            if (i == 2)
                return i;

            if (bytes[i] == '\n') {
                // Support for Director. It sends "\r\n\n" (3880387).
                return i + 1;
            }

            if (bytes[i] == '\r' && bytes[i + 1] == '\n') {
                // Support for Flash. It sends "\r\n\r\n" (3758113).
                return i + 2;
            }
        }
    }

    return -1;
}

static const char* findEndOfLine(const char* bytes, unsigned length)
{
    // According to the HTTP specification EOL is defined as
    // a CRLF pair. Unfortunately, some servers will use LF
    // instead. Worse yet, some servers will use a combination
    // of both (e.g. <header>CRLFLF<body>), so findEOL needs
    // to be more forgiving. It will now accept CRLF, LF or
    // CR.
    //
    // It returns 0 if EOLF is not found or it will return
    // a pointer to the first terminating character.
    for (unsigned i = 0; i < length; i++) {
        if (bytes[i] == '\n')
            return bytes + i;
        if (bytes[i] == '\r') {
            // Check to see if spanning buffer bounds
            // (CRLF is across reads). If so, wait for
            // next read.
            if (i + 1 == length)
                break;

            return bytes + i;
        }
    }

    return 0;
}

static String capitalizeRFC822HeaderFieldName(const String& name)
{
    bool capitalizeCharacter = true;
    StringBuilder result;

    for (unsigned i = 0; i < name.length(); i++) {
        UChar c;

        if (capitalizeCharacter && name[i] >= 'a' && name[i] <= 'z')
            c = toASCIIUpper(name[i]);
        else if (!capitalizeCharacter && name[i] >= 'A' && name[i] <= 'Z')
            c = toASCIILower(name[i]);
        else
            c = name[i];

        if (name[i] == '-')
            capitalizeCharacter = true;
        else
            capitalizeCharacter = false;

        result.append(c);
    }

    return result.toString();
}

static HTTPHeaderMap parseRFC822HeaderFields(const char* bytes, unsigned length)
{
    String lastHeaderKey;
    HTTPHeaderMap headerFields;

    // Loop over lines until we're past the header, or we can't find any more end-of-lines
    while (const char* endOfLine = findEndOfLine(bytes, length)) {
        const char* line = bytes;
        int lineLength = endOfLine - bytes;

        // Move bytes to the character after the terminator as returned by findEndOfLine.
        bytes = endOfLine + 1;
        if ((*endOfLine == '\r') && (*bytes == '\n'))
            bytes++; // Safe since findEndOfLine won't return a spanning CRLF.

        length -= (bytes - line);
        if (!lineLength) {
            // Blank line; we're at the end of the header
            break;
        }

        if (*line == ' ' || *line == '\t') {
            // Continuation of the previous header
            if (lastHeaderKey.isNull()) {
                // malformed header; ignore it and continue
                continue;
            } 
            
            // Merge the continuation of the previous header
            String currentValue = headerFields.get(lastHeaderKey);
            String newValue(line, lineLength);
            
            headerFields.set(lastHeaderKey, currentValue + newValue);
        } else {
            // Brand new header
            const char* colon = line;
            while (*colon != ':' && colon != endOfLine)
                colon++;

            if (colon == endOfLine) {
                // malformed header; ignore it and continue
                continue;
            }

            lastHeaderKey = capitalizeRFC822HeaderFieldName(String(line, colon - line));
            String value;
            
            for (colon++; colon != endOfLine; colon++) {
                if (*colon != ' ' && *colon != '\t')
                    break;
            }
            if (colon == endOfLine)
                value = "";
            else
                value = String(colon, endOfLine - colon);
            
            String oldValue = headerFields.get(lastHeaderKey);
            if (!oldValue.isNull())
                value = oldValue + ", " + value;
            
            headerFields.set(lastHeaderKey, value);
        }
    }

    return headerFields;
}
    
static NPError parsePostBuffer(bool isFile, const char *buffer, uint32_t length, bool parseHeaders, HTTPHeaderMap& headerFields, Vector<uint8_t>& bodyData)
{
    RefPtr<SharedBuffer> fileContents;
    const char* postBuffer = 0;
    uint32_t postBufferSize = 0;

    if (isFile) {
        fileContents = SharedBuffer::createWithContentsOfFile(String::fromUTF8(buffer));
        if (!fileContents)
            return NPERR_FILE_NOT_FOUND;

        postBuffer = fileContents->data();
        postBufferSize = fileContents->size();

        // FIXME: The NPAPI spec states that the file should be deleted here.
    } else {
        postBuffer = buffer;
        postBufferSize = length;
    }

    if (parseHeaders) {
        if (startsWithBlankLine(postBuffer, postBufferSize)) {
            postBuffer++;
            postBufferSize--;
        } else {
            int location = locationAfterFirstBlankLine(postBuffer, postBufferSize);
            if (location != -1) {
                // If the blank line is somewhere in the middle of the buffer, everything before is the header
                headerFields = parseRFC822HeaderFields(postBuffer, location);
                unsigned dataLength = postBufferSize - location;
                
                // Sometimes plugins like to set Content-Length themselves when they post,
                // but WebFoundation does not like that. So we will remove the header
                // and instead truncate the data to the requested length.
                String contentLength = headerFields.get("Content-Length");
                
                if (!contentLength.isNull())
                    dataLength = std::min(contentLength.toInt(), (int)dataLength);
                headerFields.remove("Content-Length");
                
                postBuffer += location;
                postBufferSize = dataLength;
                
            }
        }
    }

    ASSERT(bodyData.isEmpty());
    bodyData.append(postBuffer, postBufferSize);

    return NPERR_NO_ERROR;
}

static String makeURLString(const char* url)
{
    String urlString(url);
    
    // Strip return characters.
    urlString.replaceWithLiteral('\r', "");
    urlString.replaceWithLiteral('\n', "");

    return urlString;
}

static NPError NPN_GetURL(NPP npp, const char* url, const char* target)
{
    if (!url)
        return NPERR_GENERIC_ERROR;
    
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->loadURL("GET", makeURLString(url), target, HTTPHeaderMap(), Vector<uint8_t>(), false, 0);
    
    return NPERR_GENERIC_ERROR;
}

static NPError NPN_PostURL(NPP npp, const char* url, const char* target, uint32_t len, const char* buf, NPBool file)
{
    HTTPHeaderMap headerFields;
    Vector<uint8_t> postData;
    
    // NPN_PostURL only allows headers if the post buffer points to a file.
    bool parseHeaders = file;

    NPError error = parsePostBuffer(file, buf, len, parseHeaders, headerFields, postData);
    if (error != NPERR_NO_ERROR)
        return error;

    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->loadURL("POST", makeURLString(url), target, headerFields, postData, false, 0);
    return NPERR_NO_ERROR;
}

static NPError NPN_RequestRead(NPStream*, NPByteRange*)
{
    notImplemented();
    return NPERR_GENERIC_ERROR;
}

static NPError NPN_NewStream(NPP, NPMIMEType, const char*, NPStream**)
{
    notImplemented();
    return NPERR_GENERIC_ERROR;
}
    
static int32_t NPN_Write(NPP, NPStream*, int32_t, void*)
{
    notImplemented();    
    return -1;
}
    
static NPError NPN_DestroyStream(NPP npp, NPStream* stream, NPReason reason)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    
    return plugin->destroyStream(stream, reason);
}

static void NPN_Status(NPP npp, const char* message)
{
    String statusbarText;
    if (!message)
        statusbarText = "";
    else
        statusbarText = String::fromUTF8WithLatin1Fallback(message, strlen(message));

    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->setStatusbarText(statusbarText);
}
    
static const char* NPN_UserAgent(NPP npp)
{
    return NetscapePlugin::userAgent(npp);
}

static void* NPN_MemAlloc(uint32_t size)
{
    return npnMemAlloc(size);
}

static void NPN_MemFree(void* ptr)
{
    npnMemFree(ptr);
}

static uint32_t NPN_MemFlush(uint32_t)
{
    return 0;
}

static void NPN_ReloadPlugins(NPBool)
{
    notImplemented();
}

static JRIEnv* NPN_GetJavaEnv(void)
{
    notImplemented();
    return 0;
}

static jref NPN_GetJavaPeer(NPP)
{
    notImplemented();
    return 0;
}

static NPError NPN_GetURLNotify(NPP npp, const char* url, const char* target, void* notifyData)
{
    if (!url)
        return NPERR_GENERIC_ERROR;

    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->loadURL("GET", makeURLString(url), target, HTTPHeaderMap(), Vector<uint8_t>(), true, notifyData);
    
    return NPERR_NO_ERROR;
}

static NPError NPN_PostURLNotify(NPP npp, const char* url, const char* target, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
    HTTPHeaderMap headerFields;
    Vector<uint8_t> postData;
    NPError error = parsePostBuffer(file, buf, len, true, headerFields, postData);
    if (error != NPERR_NO_ERROR)
        return error;

    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->loadURL("POST", makeURLString(url), target, headerFields, postData, true, notifyData);
    return NPERR_NO_ERROR;
}

#if PLATFORM(MAC)
// Whether the browser supports compositing of Core Animation plug-ins.
static const unsigned WKNVSupportsCompositingCoreAnimationPluginsBool = 74656;

// Whether the browser expects a non-retained Core Animation layer.
static const unsigned WKNVExpectsNonretainedLayer = 74657;

// 74658 and 74659 are no longer implemented.

static const unsigned WKNVPlugInContainer = 74660;

#endif

static NPError NPN_GetValue(NPP npp, NPNVariable variable, void *value)
{
    switch (static_cast<unsigned>(variable)) {
        case NPNVWindowNPObject: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            PluginDestructionProtector protector(plugin.get());

            NPObject* windowNPObject = plugin->windowScriptNPObject();
            if (!windowNPObject)
                return NPERR_GENERIC_ERROR;

            *(NPObject**)value = windowNPObject;
            break;
        }
        case NPNVPluginElementNPObject: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            PluginDestructionProtector protector(plugin.get());

            NPObject* pluginElementNPObject = plugin->pluginElementNPObject();
            *(NPObject**)value = pluginElementNPObject;
            break;
        }
        case NPNVprivateModeBool: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

            *(NPBool*)value = plugin->isPrivateBrowsingEnabled();
            break;
        }
#if PLATFORM(MAC)
        case NPNVsupportsCoreGraphicsBool:
            // Always claim to support the Core Graphics drawing model.
            *(NPBool*)value = true;
            break;

        case WKNVSupportsCompositingCoreAnimationPluginsBool:
        case NPNVsupportsCoreAnimationBool: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            
            *(NPBool*)value = plugin->isAcceleratedCompositingEnabled();
            break;
        }
        case NPNVcontentsScaleFactor: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

            *(double*)value = plugin->contentsScaleFactor();
            break;
        }
        case NPNVsupportsCocoaBool:
            // Always claim to support the Cocoa event model.
            *(NPBool*)value = true;
            break;

        case NPNVsupportsUpdatedCocoaTextInputBool: {
            // The plug-in is asking whether we support the updated Cocoa text input model.
            // If we haven't yet delivered a key down event to the plug-in, we can opt into the updated
            // model and say that we support it. Otherwise, we'll just fall back and say that we don't support it.
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

            bool supportsUpdatedTextInput = !plugin->hasHandledAKeyDownEvent();
            if (supportsUpdatedTextInput)
                plugin->setPluginWantsLegacyCocoaTextInput(false);

            *reinterpret_cast<NPBool*>(value) = supportsUpdatedTextInput;
            break;
        }

        case WKNVCALayerRenderServerPort: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

            *(mach_port_t*)value = plugin->compositingRenderServerPort();
            break;
        }

        case WKNVExpectsNonretainedLayer: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

            // Asking for this will make us expect a non-retained layer from the plug-in.
            plugin->setPluginReturnsNonretainedLayer(true);
            *(NPBool*)value = true;
            break;
        }

        case WKNVPlugInContainer: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            *reinterpret_cast<void**>(value) = plugin->plugInContainer();
            break;
        }

#ifndef NP_NO_QUICKDRAW
        case NPNVsupportsQuickDrawBool:
            // We don't support the QuickDraw drawing model.
            *(NPBool*)value = false;
            break;
#endif
#ifndef NP_NO_CARBON
       case NPNVsupportsCarbonBool:
            *(NPBool*)value = true;
            break;
#endif
#elif PLUGIN_ARCHITECTURE(X11)
       case NPNVxDisplay: {
           if (!npp)
               return NPERR_GENERIC_ERROR;
           *reinterpret_cast<Display**>(value) = NetscapePlugin::x11HostDisplay();
           break;
       }
       case NPNVSupportsXEmbedBool:
           *static_cast<NPBool*>(value) = true;
           break;
       case NPNVSupportsWindowless:
           *static_cast<NPBool*>(value) = true;
           break;

       case NPNVToolkit: {
           // Gtk based plugins need to be assured about the toolkit version.
           const uint32_t expectedGtkToolKitVersion = 2;
           *reinterpret_cast<uint32_t*>(value) = expectedGtkToolKitVersion;
           break;
       }

       // TODO: implement NPNVnetscapeWindow once we want to support windowed plugins.
#endif
        default:
            notImplemented();
            return NPERR_GENERIC_ERROR;
    }

    return NPERR_NO_ERROR;
}

static NPError NPN_SetValue(NPP npp, NPPVariable variable, void *value)
{
    switch (variable) {
#if PLATFORM(MAC)
        case NPPVpluginDrawingModel: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            
            NPDrawingModel drawingModel = static_cast<NPDrawingModel>(reinterpret_cast<uintptr_t>(value));
            return plugin->setDrawingModel(drawingModel);
        }

        case NPPVpluginEventModel: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            
            NPEventModel eventModel = static_cast<NPEventModel>(reinterpret_cast<uintptr_t>(value));
            return plugin->setEventModel(eventModel);
        }
#endif

        case NPPVpluginWindowBool: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            plugin->setIsWindowed(value);
            return NPERR_NO_ERROR;
        }

        case NPPVpluginTransparentBool: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            plugin->setIsTransparent(value);
            return NPERR_NO_ERROR;
        }

        default:
            notImplemented();
            return NPERR_GENERIC_ERROR;
    }
}

static void NPN_InvalidateRect(NPP npp, NPRect* invalidRect)
{
#if PLUGIN_ARCHITECTURE(X11)
    // NSPluginWrapper, a plugin wrapper binary that allows running 32-bit plugins
    // on 64-bit architectures typically used in X11, will sometimes give us a null NPP here.
    if (!npp)
        return;
#endif
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->invalidate(invalidRect);
}

static void NPN_InvalidateRegion(NPP npp, NPRegion)
{
    // FIXME: We could at least figure out the bounding rectangle of the invalid region.
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->invalidate(0);
}

static void NPN_ForceRedraw(NPP)
{
    notImplemented();
}

static NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
    return static_cast<NPIdentifier>(IdentifierRep::get(name));
}
    
static void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount, NPIdentifier *identifiers)
{
    ASSERT(names);
    ASSERT(identifiers);

    if (!names || !identifiers)
        return;

    for (int32_t i = 0; i < nameCount; ++i)
        identifiers[i] = NPN_GetStringIdentifier(names[i]);
}

static NPIdentifier NPN_GetIntIdentifier(int32_t intid)
{
    return static_cast<NPIdentifier>(IdentifierRep::get(intid));
}

static bool NPN_IdentifierIsString(NPIdentifier identifier)
{
    return static_cast<IdentifierRep*>(identifier)->isString();
}

static NPUTF8 *NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    const char* string = static_cast<IdentifierRep*>(identifier)->string();
    if (!string)
        return 0;

    uint32_t stringLength = strlen(string);
    char* utf8String = npnMemNewArray<char>(stringLength + 1);
    memcpy(utf8String, string, stringLength);
    utf8String[stringLength] = '\0';
    
    return utf8String;
}

static int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
    return static_cast<IdentifierRep*>(identifier)->number();
}

static NPObject* NPN_CreateObject(NPP npp, NPClass *npClass)
{
    return createNPObject(npp, npClass);
}

static NPObject *NPN_RetainObject(NPObject *npObject)
{
    retainNPObject(npObject);
    return npObject;
}

static void NPN_ReleaseObject(NPObject *npObject)
{
    releaseNPObject(npObject);
}

static bool NPN_Invoke(NPP npp, NPObject *npObject, NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());

    if (npObject->_class->invoke)
        return npObject->_class->invoke(npObject, methodName, arguments, argumentCount, result);

    return false;
}

static bool NPN_InvokeDefault(NPP npp, NPObject *npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());

    if (npObject->_class->invokeDefault)
        return npObject->_class->invokeDefault(npObject, arguments, argumentCount, result);

    return false;
}

static bool NPN_Evaluate(NPP npp, NPObject *npObject, NPString *script, NPVariant* result)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    String scriptString = String::fromUTF8WithLatin1Fallback(script->UTF8Characters, script->UTF8Length);
    
    return plugin->evaluate(npObject, scriptString, result);
}

static bool NPN_GetProperty(NPP npp, NPObject* npObject, NPIdentifier propertyName, NPVariant* result)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (npObject->_class->getProperty)
        return npObject->_class->getProperty(npObject, propertyName, result);
    
    return false;
}

static bool NPN_SetProperty(NPP npp, NPObject* npObject, NPIdentifier propertyName, const NPVariant* value)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (npObject->_class->setProperty)
        return npObject->_class->setProperty(npObject, propertyName, value);

    return false;
}

static bool NPN_RemoveProperty(NPP npp, NPObject* npObject, NPIdentifier propertyName)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (npObject->_class->removeProperty)
        return npObject->_class->removeProperty(npObject, propertyName);

    return false;
}

static bool NPN_HasProperty(NPP npp, NPObject* npObject, NPIdentifier propertyName)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (npObject->_class->hasProperty)
        return npObject->_class->hasProperty(npObject, propertyName);

    return false;
}

static bool NPN_HasMethod(NPP npp, NPObject* npObject, NPIdentifier methodName)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (npObject->_class->hasMethod)
        return npObject->_class->hasMethod(npObject, methodName);

    return false;
}

static void NPN_ReleaseVariantValue(NPVariant* variant)
{
    releaseNPVariantValue(variant);
}

static void NPN_SetException(NPObject*, const NPUTF8* message)
{
    NetscapePlugin::setException(message);
}

static void NPN_PushPopupsEnabledState(NPP npp, NPBool enabled)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->pushPopupsEnabledState(enabled);
}
    
static void NPN_PopPopupsEnabledState(NPP npp)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    plugin->popPopupsEnabledState();
}
    
static bool NPN_Enumerate(NPP npp, NPObject* npObject, NPIdentifier** identifiers, uint32_t* identifierCount)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (NP_CLASS_STRUCT_VERSION_HAS_ENUM(npObject->_class) && npObject->_class->enumerate)
        return npObject->_class->enumerate(npObject, identifiers, identifierCount);

    return false;
}

static void NPN_PluginThreadAsyncCall(NPP npp, void (*function)(void*), void* userData)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

    plugin->pluginThreadAsyncCall(function, userData);
}

static bool NPN_Construct(NPP npp, NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    PluginDestructionProtector protector(plugin.get());
    
    if (NP_CLASS_STRUCT_VERSION_HAS_CTOR(npObject->_class) && npObject->_class->construct)
        return npObject->_class->construct(npObject, arguments, argumentCount, result);

    return false;
}

static NPError copyCString(const CString& string, char** value, uint32_t* len)
{
    ASSERT(!string.isNull());
    ASSERT(value);
    ASSERT(len);

    *value = npnMemNewArray<char>(string.length());
    if (!*value)
        return NPERR_GENERIC_ERROR;

    memcpy(*value, string.data(), string.length());
    *len = string.length();
    return NPERR_NO_ERROR;
}

static NPError NPN_GetValueForURL(NPP npp, NPNURLVariable variable, const char* url, char** value, uint32_t* len)
{
    if (!value || !len)
        return NPERR_GENERIC_ERROR;
    
    switch (variable) {
        case NPNURLVCookie: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            PluginDestructionProtector protector(plugin.get());
            
            String cookies = plugin->cookiesForURL(makeURLString(url));
            if (cookies.isNull())
                return NPERR_GENERIC_ERROR;

            return copyCString(cookies.utf8(), value, len);
        }

        case NPNURLVProxy: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            PluginDestructionProtector protector(plugin.get());
            
            String proxies = plugin->proxiesForURL(makeURLString(url));
            if (proxies.isNull())
                return NPERR_GENERIC_ERROR;

            return copyCString(proxies.utf8(), value, len);
        }
        default:
            notImplemented();
            return NPERR_GENERIC_ERROR;
    }
}

static NPError NPN_SetValueForURL(NPP npp, NPNURLVariable variable, const char* url, const char* value, uint32_t len)
{
    switch (variable) {
        case NPNURLVCookie: {
            RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
            PluginDestructionProtector protector(plugin.get());
            
            plugin->setCookiesForURL(makeURLString(url), String(value, len));
            return NPERR_NO_ERROR;
        }

        case NPNURLVProxy:
            // Can't set the proxy for a URL.
            return NPERR_GENERIC_ERROR;

        default:
            notImplemented();
            return NPERR_GENERIC_ERROR;
    }
}

static bool initializeProtectionSpace(const char* protocol, const char* host, int port, const char* scheme, const char* realm, ProtectionSpace& protectionSpace)
{
    ProtectionSpaceServerType serverType;
    if (!strcasecmp(protocol, "http"))
        serverType = ProtectionSpaceServerHTTP;
    else if (!strcasecmp(protocol, "https"))
        serverType = ProtectionSpaceServerHTTPS;
    else {
        // We only care about http and https.
        return false;
    }

    ProtectionSpaceAuthenticationScheme authenticationScheme = ProtectionSpaceAuthenticationSchemeDefault;
    if (serverType == ProtectionSpaceServerHTTP) {
        if (!strcasecmp(scheme, "basic"))
            authenticationScheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
        else if (!strcmp(scheme, "digest"))
            authenticationScheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
    }

    protectionSpace = ProtectionSpace(host, port, serverType, realm, authenticationScheme);
    return true;
}

static NPError NPN_GetAuthenticationInfo(NPP npp, const char* protocol, const char* host, int32_t port, const char* scheme, 
                                         const char* realm, char** username, uint32_t* usernameLength, char** password, uint32_t* passwordLength)
{
    if (!protocol || !host || !scheme || !realm || !username || !usernameLength || !password || !passwordLength)
        return NPERR_GENERIC_ERROR;

    ProtectionSpace protectionSpace;
    if (!initializeProtectionSpace(protocol, host, port, scheme, realm, protectionSpace))
        return NPERR_GENERIC_ERROR;

    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);
    String usernameString;
    String passwordString;
    if (!plugin->getAuthenticationInfo(protectionSpace, usernameString, passwordString))
        return NPERR_GENERIC_ERROR;

    NPError result = copyCString(usernameString.utf8(), username, usernameLength);
    if (result != NPERR_NO_ERROR)
        return result;

    result = copyCString(passwordString.utf8(), password, passwordLength);
    if (result != NPERR_NO_ERROR) {
        npnMemFree(*username);
        return result;
    }

    return NPERR_NO_ERROR;
}

static uint32_t NPN_ScheduleTimer(NPP npp, uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID))
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

    return plugin->scheduleTimer(interval, repeat, timerFunc);
}

static void NPN_UnscheduleTimer(NPP npp, uint32_t timerID)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

    plugin->unscheduleTimer(timerID);
}

#if PLATFORM(MAC)
static NPError NPN_PopUpContextMenu(NPP npp, NPMenu* menu)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

    return plugin->popUpContextMenu(menu);
}

static NPBool NPN_ConvertPoint(NPP npp, double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double* destX, double* destY, NPCoordinateSpace destSpace)
{
    RefPtr<NetscapePlugin> plugin = NetscapePlugin::fromNPP(npp);

    double destinationX;
    double destinationY;

    bool returnValue = plugin->convertPoint(sourceX, sourceY, sourceSpace, destinationX, destinationY, destSpace);

    if (destX)
        *destX = destinationX;
    if (destY)
        *destY = destinationY;

    return returnValue;
}
#endif

static void initializeBrowserFuncs(NPNetscapeFuncs &netscapeFuncs)
{
    netscapeFuncs.size = sizeof(NPNetscapeFuncs);
    netscapeFuncs.version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    
    netscapeFuncs.geturl = NPN_GetURL;
    netscapeFuncs.posturl = NPN_PostURL;
    netscapeFuncs.requestread = NPN_RequestRead;
    netscapeFuncs.newstream = NPN_NewStream;
    netscapeFuncs.write = NPN_Write;
    netscapeFuncs.destroystream = NPN_DestroyStream;
    netscapeFuncs.status = NPN_Status;
    netscapeFuncs.uagent = NPN_UserAgent;
    netscapeFuncs.memalloc = NPN_MemAlloc;
    netscapeFuncs.memfree = NPN_MemFree;
    netscapeFuncs.memflush = NPN_MemFlush;
    netscapeFuncs.reloadplugins = NPN_ReloadPlugins;
    netscapeFuncs.getJavaEnv = NPN_GetJavaEnv;
    netscapeFuncs.getJavaPeer = NPN_GetJavaPeer;
    netscapeFuncs.geturlnotify = NPN_GetURLNotify;
    netscapeFuncs.posturlnotify = NPN_PostURLNotify;
    netscapeFuncs.getvalue = NPN_GetValue;
    netscapeFuncs.setvalue = NPN_SetValue;
    netscapeFuncs.invalidaterect = NPN_InvalidateRect;
    netscapeFuncs.invalidateregion = NPN_InvalidateRegion;
    netscapeFuncs.forceredraw = NPN_ForceRedraw;
    
    netscapeFuncs.getstringidentifier = NPN_GetStringIdentifier;
    netscapeFuncs.getstringidentifiers = NPN_GetStringIdentifiers;
    netscapeFuncs.getintidentifier = NPN_GetIntIdentifier;
    netscapeFuncs.identifierisstring = NPN_IdentifierIsString;
    netscapeFuncs.utf8fromidentifier = NPN_UTF8FromIdentifier;
    netscapeFuncs.intfromidentifier = NPN_IntFromIdentifier;
    netscapeFuncs.createobject = NPN_CreateObject;
    netscapeFuncs.retainobject = NPN_RetainObject;
    netscapeFuncs.releaseobject = NPN_ReleaseObject;
    netscapeFuncs.invoke = NPN_Invoke;
    netscapeFuncs.invokeDefault = NPN_InvokeDefault;
    netscapeFuncs.evaluate = NPN_Evaluate;
    netscapeFuncs.getproperty = NPN_GetProperty;
    netscapeFuncs.setproperty = NPN_SetProperty;
    netscapeFuncs.removeproperty = NPN_RemoveProperty;
    netscapeFuncs.hasproperty = NPN_HasProperty;
    netscapeFuncs.hasmethod = NPN_HasMethod;
    netscapeFuncs.releasevariantvalue = NPN_ReleaseVariantValue;
    netscapeFuncs.setexception = NPN_SetException;
    netscapeFuncs.pushpopupsenabledstate = NPN_PushPopupsEnabledState;
    netscapeFuncs.poppopupsenabledstate = NPN_PopPopupsEnabledState;
    netscapeFuncs.enumerate = NPN_Enumerate;
    netscapeFuncs.pluginthreadasynccall = NPN_PluginThreadAsyncCall;
    netscapeFuncs.construct = NPN_Construct;
    netscapeFuncs.getvalueforurl = NPN_GetValueForURL;
    netscapeFuncs.setvalueforurl = NPN_SetValueForURL;
    netscapeFuncs.getauthenticationinfo = NPN_GetAuthenticationInfo;
    netscapeFuncs.scheduletimer = NPN_ScheduleTimer;
    netscapeFuncs.unscheduletimer = NPN_UnscheduleTimer;
#if PLATFORM(MAC)
    netscapeFuncs.popupcontextmenu = NPN_PopUpContextMenu;
    netscapeFuncs.convertpoint = NPN_ConvertPoint;
#else
    netscapeFuncs.popupcontextmenu = 0;
    netscapeFuncs.convertpoint = 0;
#endif
}
    
NPNetscapeFuncs* netscapeBrowserFuncs()
{
    static NPNetscapeFuncs netscapeFuncs;
    static bool initialized = false;
    
    if (!initialized) {
        initializeBrowserFuncs(netscapeFuncs);
        initialized = true;
    }

    return &netscapeFuncs;
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
