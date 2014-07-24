/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
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

#include "config.h"
#include "PluginDatabase.h"

#include "Frame.h"
#include "KURL.h"
#include "PluginPackage.h"
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
#include "FileSystem.h"
#endif
#include <stdlib.h>
#include <wtf/text/CString.h>

#if PLATFORM(BLACKBERRY)
#include <BlackBerryPlatformSettings.h>
#endif

namespace WebCore {

typedef HashMap<String, RefPtr<PluginPackage> > PluginPackageByNameMap;

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
static const size_t maximumPersistentPluginMetadataCacheSize = 32768;

static bool gPersistentPluginMetadataCacheIsEnabled;

String& persistentPluginMetadataCachePath()
{
    DEFINE_STATIC_LOCAL(String, cachePath, ());
    return cachePath;
}
#endif

PluginDatabase::PluginDatabase()
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    : m_persistentMetadataCacheIsLoaded(false)
#endif
{
}

PluginDatabase* PluginDatabase::installedPlugins(bool populate)
{
    static PluginDatabase* plugins = 0;

    if (!plugins) {
        plugins = new PluginDatabase;

        if (populate) {
            plugins->setPluginDirectories(PluginDatabase::defaultPluginDirectories());
            plugins->refresh();
        }
    }

    return plugins;
}

bool PluginDatabase::isMIMETypeRegistered(const String& mimeType)
{
    if (mimeType.isNull())
        return false;
    if (m_registeredMIMETypes.contains(mimeType))
        return true;
    // No plugin was found, try refreshing the database and searching again
    return (refresh() && m_registeredMIMETypes.contains(mimeType));
}

void PluginDatabase::addExtraPluginDirectory(const String& directory)
{
    m_pluginDirectories.append(directory);
    refresh();
}

bool PluginDatabase::refresh()
{
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    if (!m_persistentMetadataCacheIsLoaded)
        loadPersistentMetadataCache();
#endif
    bool pluginSetChanged = false;

    if (!m_plugins.isEmpty()) {
        PluginSet pluginsToUnload;
        getDeletedPlugins(pluginsToUnload);

        // Unload plugins
        PluginSet::const_iterator end = pluginsToUnload.end();
        for (PluginSet::const_iterator it = pluginsToUnload.begin(); it != end; ++it)
            remove(it->get());

        pluginSetChanged = !pluginsToUnload.isEmpty();
    }

    HashSet<String> paths;
    getPluginPathsInDirectories(paths);

    HashMap<String, time_t> pathsWithTimes;

    // We should only skip unchanged files if we didn't remove any plugins above. If we did remove
    // any plugins, we need to look at every plugin file so that, e.g., if the user has two versions
    // of RealPlayer installed and just removed the newer one, we'll pick up the older one.
    bool shouldSkipUnchangedFiles = !pluginSetChanged;

    HashSet<String>::const_iterator pathsEnd = paths.end();
    for (HashSet<String>::const_iterator it = paths.begin(); it != pathsEnd; ++it) {
        time_t lastModified;
        if (!getFileModificationTime(*it, lastModified))
            continue;

        pathsWithTimes.add(*it, lastModified);

        // If the path's timestamp hasn't changed since the last time we ran refresh(), we don't have to do anything.
        if (shouldSkipUnchangedFiles && m_pluginPathsWithTimes.get(*it) == lastModified)
            continue;

        if (RefPtr<PluginPackage> oldPackage = m_pluginsByPath.get(*it)) {
            ASSERT(!shouldSkipUnchangedFiles || oldPackage->lastModified() != lastModified);
            remove(oldPackage.get());
        }

        RefPtr<PluginPackage> package = PluginPackage::createPackage(*it, lastModified);
        if (package && add(package.release()))
            pluginSetChanged = true;
    }

    // Cache all the paths we found with their timestamps for next time.
    pathsWithTimes.swap(m_pluginPathsWithTimes);

    if (!pluginSetChanged)
        return false;

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    updatePersistentMetadataCache();
#endif

    m_registeredMIMETypes.clear();

    // Register plug-in MIME types
    PluginSet::const_iterator end = m_plugins.end();
    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it) {
        // Get MIME types
        MIMEToDescriptionsMap::const_iterator map_it = (*it)->mimeToDescriptions().begin();
        MIMEToDescriptionsMap::const_iterator map_end = (*it)->mimeToDescriptions().end();
        for (; map_it != map_end; ++map_it)
            m_registeredMIMETypes.add(map_it->key);
    }

    return true;
}

Vector<PluginPackage*> PluginDatabase::plugins() const
{
    Vector<PluginPackage*> result;

    PluginSet::const_iterator end = m_plugins.end();
    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it)
        result.append((*it).get());

    return result;
}

int PluginDatabase::preferredPluginCompare(const void* a, const void* b)
{
    PluginPackage* pluginA = *static_cast<PluginPackage* const*>(a);
    PluginPackage* pluginB = *static_cast<PluginPackage* const*>(b);

    return pluginA->compare(*pluginB);
}

PluginPackage* PluginDatabase::pluginForMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return 0;

    String key = mimeType.lower();
    PluginSet::const_iterator end = m_plugins.end();
    PluginPackage* preferredPlugin = m_preferredPlugins.get(key);
    if (preferredPlugin
        && preferredPlugin->isEnabled()
        && preferredPlugin->mimeToDescriptions().contains(key)) {
        return preferredPlugin;
    }

    Vector<PluginPackage*, 2> pluginChoices;

    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it) {
        PluginPackage* plugin = (*it).get();

        if (!plugin->isEnabled())
            continue;

        if (plugin->mimeToDescriptions().contains(key)) {
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
            if (!plugin->ensurePluginLoaded())
                continue;
#endif
            pluginChoices.append(plugin);
        }
    }

    if (pluginChoices.isEmpty())
        return 0;

    qsort(pluginChoices.data(), pluginChoices.size(), sizeof(PluginPackage*), PluginDatabase::preferredPluginCompare);

    return pluginChoices[0];
}

String PluginDatabase::MIMETypeForExtension(const String& extension) const
{
    if (extension.isEmpty())
        return String();

    PluginSet::const_iterator end = m_plugins.end();
    String mimeType;
    Vector<PluginPackage*, 2> pluginChoices;
    HashMap<PluginPackage*, String> mimeTypeForPlugin;

    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it) {
        if (!(*it)->isEnabled())
            continue;

        MIMEToExtensionsMap::const_iterator mime_end = (*it)->mimeToExtensions().end();

        for (MIMEToExtensionsMap::const_iterator mime_it = (*it)->mimeToExtensions().begin(); mime_it != mime_end; ++mime_it) {
            mimeType = mime_it->key;
            PluginPackage* preferredPlugin = m_preferredPlugins.get(mimeType);
            const Vector<String>& extensions = mime_it->value;
            bool foundMapping = false;
            for (unsigned i = 0; i < extensions.size(); i++) {
                if (equalIgnoringCase(extensions[i], extension)) {
                    PluginPackage* plugin = (*it).get();

                    if (preferredPlugin && PluginPackage::equal(*plugin, *preferredPlugin))
                        return mimeType;

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
                    if (!plugin->ensurePluginLoaded())
                        continue;
#endif
                    pluginChoices.append(plugin);
                    mimeTypeForPlugin.add(plugin, mimeType);
                    foundMapping = true;
                    break;
                }
            }
            if (foundMapping)
                break;
        }
    }

    if (pluginChoices.isEmpty())
        return String();

    qsort(pluginChoices.data(), pluginChoices.size(), sizeof(PluginPackage*), PluginDatabase::preferredPluginCompare);

    return mimeTypeForPlugin.get(pluginChoices[0]);
}

PluginPackage* PluginDatabase::findPlugin(const KURL& url, String& mimeType)
{
    if (!mimeType.isEmpty())
        return pluginForMIMEType(mimeType);
    
    String filename = url.lastPathComponent();
    if (filename.endsWith('/'))
        return 0;
    
    int extensionPos = filename.reverseFind('.');
    if (extensionPos == -1)
        return 0;
    
    String mimeTypeForExtension = MIMETypeForExtension(filename.substring(extensionPos + 1));
    PluginPackage* plugin = pluginForMIMEType(mimeTypeForExtension);
    if (!plugin) {
        // FIXME: if no plugin could be found, query Windows for the mime type
        // corresponding to the extension.
        return 0;
    }
    
    mimeType = mimeTypeForExtension;
    return plugin;
}

void PluginDatabase::setPreferredPluginForMIMEType(const String& mimeType, PluginPackage* plugin)
{
    if (!plugin || plugin->mimeToExtensions().contains(mimeType))
        m_preferredPlugins.set(mimeType.lower(), plugin);
}

bool PluginDatabase::fileExistsAndIsNotDisabled(const String& filePath) const
{
    // Skip plugin files that are disabled by filename.
    if (m_disabledPluginFiles.contains(pathGetFileName(filePath)))
        return false;

    return fileExists(filePath);
}

void PluginDatabase::getDeletedPlugins(PluginSet& plugins) const
{
    PluginSet::const_iterator end = m_plugins.end();
    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it) {
        if (!fileExistsAndIsNotDisabled((*it)->path()))
            plugins.add(*it);
    }
}

bool PluginDatabase::add(PassRefPtr<PluginPackage> prpPackage)
{
    ASSERT_ARG(prpPackage, prpPackage);

    RefPtr<PluginPackage> package = prpPackage;

    if (!m_plugins.add(package).isNewEntry)
        return false;

    m_pluginsByPath.add(package->path(), package);
    return true;
}

void PluginDatabase::remove(PluginPackage* package)
{
    MIMEToExtensionsMap::const_iterator it = package->mimeToExtensions().begin();
    MIMEToExtensionsMap::const_iterator end = package->mimeToExtensions().end();
    for ( ; it != end; ++it) {
        PluginPackageByNameMap::iterator packageInMap = m_preferredPlugins.find(it->key);
        if (packageInMap != m_preferredPlugins.end() && packageInMap->value == package)
            m_preferredPlugins.remove(packageInMap);
    }

    m_plugins.remove(package);
    m_pluginsByPath.remove(package->path());
}

void PluginDatabase::clear()
{
    m_plugins.clear();
    m_pluginsByPath.clear();
    m_pluginPathsWithTimes.clear();
    m_registeredMIMETypes.clear();
    m_preferredPlugins.clear();
#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    m_persistentMetadataCacheIsLoaded = false;
#endif
}

bool PluginDatabase::removeDisabledPluginFile(const String& fileName)
{
    if (!m_disabledPluginFiles.contains(fileName))
        return false;

    m_disabledPluginFiles.remove(fileName);
    return true;
}

bool PluginDatabase::addDisabledPluginFile(const String& fileName)
{
    return m_disabledPluginFiles.add(fileName).isNewEntry;
}

#if (!OS(WINCE)) && (!OS(WINDOWS) || !ENABLE(NETSCAPE_PLUGIN_API))
// For Safari/Win the following three methods are implemented
// in PluginDatabaseWin.cpp, but if we can use WebCore constructs
// for the logic we should perhaps move it here under XP_WIN?

Vector<String> PluginDatabase::defaultPluginDirectories()
{
    Vector<String> paths;

    // Add paths specific to each platform
#if defined(XP_UNIX) && !PLATFORM(BLACKBERRY)
    String userPluginPath = homeDirectoryPath();
    userPluginPath.append(String("/.mozilla/plugins"));
    paths.append(userPluginPath);

    userPluginPath = homeDirectoryPath();
    userPluginPath.append(String("/.netscape/plugins"));
    paths.append(userPluginPath);

    paths.append("/usr/lib/browser/plugins");
    paths.append("/usr/local/lib/mozilla/plugins");
    paths.append("/usr/lib/firefox/plugins");
    paths.append("/usr/lib64/browser-plugins");
    paths.append("/usr/lib/browser-plugins");
    paths.append("/usr/lib/mozilla/plugins");
    paths.append("/usr/local/netscape/plugins");
    paths.append("/opt/mozilla/plugins");
    paths.append("/opt/mozilla/lib/plugins");
    paths.append("/opt/netscape/plugins");
    paths.append("/opt/netscape/communicator/plugins");
    paths.append("/usr/lib/netscape/plugins");
    paths.append("/usr/lib/netscape/plugins-libc5");
    paths.append("/usr/lib/netscape/plugins-libc6");
    paths.append("/usr/lib64/netscape/plugins");
    paths.append("/usr/lib64/mozilla/plugins");
    paths.append("/usr/lib/nsbrowser/plugins");
    paths.append("/usr/lib64/nsbrowser/plugins");

    String mozHome(getenv("MOZILLA_HOME"));
    mozHome.append("/plugins");
    paths.append(mozHome);

    Vector<String> mozPaths;
    String mozPath(getenv("MOZ_PLUGIN_PATH"));
    mozPath.split(UChar(':'), /* allowEmptyEntries */ false, mozPaths);
    paths.appendVector(mozPaths);
#elif PLATFORM(BLACKBERRY)
    paths.append(BlackBerry::Platform::Settings::instance()->applicationPluginDirectory().c_str());
#elif defined(XP_MACOSX)
    String userPluginPath = homeDirectoryPath();
    userPluginPath.append(String("/Library/Internet Plug-Ins"));
    paths.append(userPluginPath);
    paths.append("/Library/Internet Plug-Ins");
#elif defined(XP_WIN)
    String userPluginPath = homeDirectoryPath();
    userPluginPath.append(String("\\Application Data\\Mozilla\\plugins"));
    paths.append(userPluginPath);
#endif

    // Add paths specific to each port
#if PLATFORM(QT)
    Vector<String> qtPaths;
    String qtPath(qgetenv("QTWEBKIT_PLUGIN_PATH").constData());
    qtPath.split(UChar(':'), /* allowEmptyEntries */ false, qtPaths);
    paths.appendVector(qtPaths);
#endif

    return paths;
}

bool PluginDatabase::isPreferredPluginDirectory(const String& path)
{
    String preferredPath = homeDirectoryPath();

#if defined(XP_UNIX) && !PLATFORM(BLACKBERRY)
    preferredPath.append(String("/.mozilla/plugins"));
#elif PLATFORM(BLACKBERRY)
    preferredPath = BlackBerry::Platform::Settings::instance()->applicationPluginDirectory().c_str();
#elif defined(XP_MACOSX)
    preferredPath.append(String("/Library/Internet Plug-Ins"));
#elif defined(XP_WIN)
    preferredPath.append(String("\\Application Data\\Mozilla\\plugins"));
#endif

    // TODO: We should normalize the path before doing a comparison.
    return path == preferredPath;
}

void PluginDatabase::getPluginPathsInDirectories(HashSet<String>& paths) const
{
    // FIXME: This should be a case insensitive set.
    HashSet<String> uniqueFilenames;

#if defined(XP_UNIX)
    String fileNameFilter("*.so");
#else
    String fileNameFilter("");
#endif

    Vector<String>::const_iterator dirsEnd = m_pluginDirectories.end();
    for (Vector<String>::const_iterator dIt = m_pluginDirectories.begin(); dIt != dirsEnd; ++dIt) {
        Vector<String> pluginPaths = listDirectory(*dIt, fileNameFilter);
        Vector<String>::const_iterator pluginsEnd = pluginPaths.end();
        for (Vector<String>::const_iterator pIt = pluginPaths.begin(); pIt != pluginsEnd; ++pIt) {
            if (!fileExistsAndIsNotDisabled(*pIt))
                continue;

            paths.add(*pIt);
        }
    }
}

#endif // !OS(WINDOWS)

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)

static void fillBufferWithContentsOfFile(PlatformFileHandle file, Vector<char>& buffer)
{
    size_t bufferSize = 0;
    size_t bufferCapacity = 1024;
    buffer.resize(bufferCapacity);

    do {
        bufferSize += readFromFile(file, buffer.data() + bufferSize, bufferCapacity - bufferSize);
        if (bufferSize == bufferCapacity) {
            if (bufferCapacity < maximumPersistentPluginMetadataCacheSize) {
                bufferCapacity *= 2;
                buffer.resize(bufferCapacity);
            } else {
                buffer.clear();
                return;
            }
        } else
            break;
    } while (true);

    buffer.shrink(bufferSize);
}

static bool readUTF8String(String& resultString, char*& start, const char* end)
{
    if (start >= end)
        return false;

    int len = strlen(start);
    resultString = String::fromUTF8(start, len);
    start += len + 1;

    return true;
}

static bool readTime(time_t& resultTime, char*& start, const char* end)
{
    if (start + sizeof(time_t) >= end)
        return false;

    // The stream is not necessary aligned.
    memcpy(&resultTime, start, sizeof(time_t));
    start += sizeof(time_t);

    return true;
}

static const char schemaVersion = '1';
static const char persistentPluginMetadataCacheFilename[] = "PluginMetadataCache.bin";

void PluginDatabase::loadPersistentMetadataCache()
{
    if (!isPersistentMetadataCacheEnabled() || persistentMetadataCachePath().isEmpty())
        return;

    PlatformFileHandle file;
    String absoluteCachePath = pathByAppendingComponent(persistentMetadataCachePath(), persistentPluginMetadataCacheFilename);
    file = openFile(absoluteCachePath, OpenForRead);

    if (!isHandleValid(file))
        return;

    // Mark cache as loaded regardless of success or failure. If
    // there's error in the cache, we won't try to load it anymore.
    m_persistentMetadataCacheIsLoaded = true;

    Vector<char> fileContents;
    fillBufferWithContentsOfFile(file, fileContents);
    closeFile(file);

    if (fileContents.size() < 2 || fileContents.first() != schemaVersion || fileContents.last() != '\0') {
        LOG_ERROR("Unable to read plugin metadata cache: corrupt schema");
        deleteFile(absoluteCachePath);
        return;
    }

    char* bufferPos = fileContents.data() + 1;
    char* end = fileContents.data() + fileContents.size();

    PluginSet cachedPlugins;
    HashMap<String, time_t> cachedPluginPathsWithTimes;
    HashMap<String, RefPtr<PluginPackage> > cachedPluginsByPath;

    while (bufferPos < end) {
        String path;
        time_t lastModified;
        String name;
        String desc;
        String mimeDesc;
        if (!(readUTF8String(path, bufferPos, end)
              && readTime(lastModified, bufferPos, end)
              && readUTF8String(name, bufferPos, end)
              && readUTF8String(desc, bufferPos, end)
              && readUTF8String(mimeDesc, bufferPos, end))) {
            LOG_ERROR("Unable to read plugin metadata cache: corrupt data");
            deleteFile(absoluteCachePath);
            return;
        }

        // Skip metadata that points to plugins from directories that
        // are not part of plugin directory list anymore.
        String pluginDirectoryName = directoryName(path);
        if (m_pluginDirectories.find(pluginDirectoryName) == WTF::notFound)
            continue;

        RefPtr<PluginPackage> package = PluginPackage::createPackageFromCache(path, lastModified, name, desc, mimeDesc);

        if (package && cachedPlugins.add(package).isNewEntry) {
            cachedPluginPathsWithTimes.add(package->path(), package->lastModified());
            cachedPluginsByPath.add(package->path(), package);
        }
    }

    m_plugins.swap(cachedPlugins);
    m_pluginsByPath.swap(cachedPluginsByPath);
    m_pluginPathsWithTimes.swap(cachedPluginPathsWithTimes);
}

static bool writeUTF8String(PlatformFileHandle file, const String& string)
{
    CString utf8String = string.utf8();
    int length = utf8String.length() + 1;
    return writeToFile(file, utf8String.data(), length) == length;
}

static bool writeTime(PlatformFileHandle file, const time_t& time)
{
    return writeToFile(file, reinterpret_cast<const char*>(&time), sizeof(time_t)) == sizeof(time_t);
}

void PluginDatabase::updatePersistentMetadataCache()
{
    if (!isPersistentMetadataCacheEnabled() || persistentMetadataCachePath().isEmpty())
        return;

    makeAllDirectories(persistentMetadataCachePath());
    String absoluteCachePath = pathByAppendingComponent(persistentMetadataCachePath(), persistentPluginMetadataCacheFilename);
    deleteFile(absoluteCachePath);

    if (m_plugins.isEmpty())
        return;

    PlatformFileHandle file;
    file = openFile(absoluteCachePath, OpenForWrite);

    if (!isHandleValid(file)) {
        LOG_ERROR("Unable to open plugin metadata cache for saving");
        return;
    }

    char localSchemaVersion = schemaVersion;
    if (writeToFile(file, &localSchemaVersion, 1) != 1) {
        LOG_ERROR("Unable to write plugin metadata cache schema");
        closeFile(file);
        deleteFile(absoluteCachePath);
        return;
    }

    PluginSet::const_iterator end = m_plugins.end();
    for (PluginSet::const_iterator it = m_plugins.begin(); it != end; ++it) {
        if (!(writeUTF8String(file, (*it)->path())
              && writeTime(file, (*it)->lastModified())
              && writeUTF8String(file, (*it)->name())
              && writeUTF8String(file, (*it)->description())
              && writeUTF8String(file, (*it)->fullMIMEDescription()))) {
            LOG_ERROR("Unable to write plugin metadata to cache");
            closeFile(file);
            deleteFile(absoluteCachePath);
            return;
        }
    }

    closeFile(file);
}

bool PluginDatabase::isPersistentMetadataCacheEnabled()
{
    return gPersistentPluginMetadataCacheIsEnabled;
}

void PluginDatabase::setPersistentMetadataCacheEnabled(bool isEnabled)
{
    gPersistentPluginMetadataCacheIsEnabled = isEnabled;
}

String PluginDatabase::persistentMetadataCachePath()
{
    return WebCore::persistentPluginMetadataCachePath();
}

void PluginDatabase::setPersistentMetadataCachePath(const String& persistentMetadataCachePath)
{
    WebCore::persistentPluginMetadataCachePath() = persistentMetadataCachePath;
}
#endif
}
