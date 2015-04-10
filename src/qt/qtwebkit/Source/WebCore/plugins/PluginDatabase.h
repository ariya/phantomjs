/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PluginDatabase_h
#define PluginDatabase_h

#include "PluginPackage.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class Element;
    class Frame;
    class IntSize;
    class KURL;
    class PluginDatabaseClient;
    class PluginPackage;

    typedef HashSet<RefPtr<PluginPackage>, PluginPackageHash, PluginPackageHashTraits> PluginSet;

    class PluginDatabase {
        WTF_MAKE_NONCOPYABLE(PluginDatabase); WTF_MAKE_FAST_ALLOCATED;
    public:
        PluginDatabase();

        // The first call to installedPlugins creates the plugin database
        // and by default populates it with the plugins installed on the system.
        // For testing purposes, it is possible to not populate the database
        // automatically, as the plugins might affect the DRT results by
        // writing to a.o. stderr.
        static PluginDatabase* installedPlugins(bool populate = true);

        bool refresh();
        void clear();
        Vector<PluginPackage*> plugins() const;
        bool isMIMETypeRegistered(const String& mimeType);
        void addExtraPluginDirectory(const String&);

        static bool isPreferredPluginDirectory(const String& directory);
        static int preferredPluginCompare(const void*, const void*);

        PluginPackage* findPlugin(const KURL&, String& mimeType);
        PluginPackage* pluginForMIMEType(const String& mimeType);
        void setPreferredPluginForMIMEType(const String& mimeType, PluginPackage* plugin);

        void setPluginDirectories(const Vector<String>& directories)
        {
            clear();
            m_pluginDirectories = directories;
        }

        bool removeDisabledPluginFile(const String& fileName);
        bool addDisabledPluginFile(const String& fileName);
        static Vector<String> defaultPluginDirectories();
        Vector<String> pluginDirectories() const { return m_pluginDirectories; }

        String MIMETypeForExtension(const String& extension) const;
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
        static bool isPersistentMetadataCacheEnabled();
        static void setPersistentMetadataCacheEnabled(bool isEnabled);
        static String persistentMetadataCachePath();
        static void setPersistentMetadataCachePath(const String& persistentMetadataCachePath);
#endif

    private:
        void getPluginPathsInDirectories(HashSet<String>&) const;
        void getDeletedPlugins(PluginSet&) const;
        bool fileExistsAndIsNotDisabled(const String&) const;

        // Returns whether the plugin was actually added or not (it won't be added if it's a duplicate of an existing plugin).
        bool add(PassRefPtr<PluginPackage>);
        void remove(PluginPackage*);
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
        void loadPersistentMetadataCache();
        void updatePersistentMetadataCache();
#endif

        HashSet<String> m_disabledPluginFiles;
        Vector<String> m_pluginDirectories;
        HashSet<String> m_registeredMIMETypes;
        PluginSet m_plugins;
        HashMap<String, RefPtr<PluginPackage> > m_pluginsByPath;
        HashMap<String, time_t> m_pluginPathsWithTimes;
        HashMap<String, RefPtr<PluginPackage> > m_preferredPlugins;
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
        bool m_persistentMetadataCacheIsLoaded;
#endif
    };

} // namespace WebCore

#endif
