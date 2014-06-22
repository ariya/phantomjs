/*
 * Copyright (C) 2006, 2008, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "MIMETypeRegistry.h"

#include "MediaPlayer.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

#if USE(CG)
#include "ImageSourceCG.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(QT)
#include <QImageReader>
#include <QImageWriter>
#endif

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
#include "ArchiveFactory.h"
#endif

namespace WebCore {

namespace {
struct TypeExtensionPair {
    const char* type;
    const char* extension;
};
}

// A table of common media MIME types and file extenstions used when a platform's
// specific MIME type lookup doesn't have a match for a media file extension.
static const TypeExtensionPair commonMediaTypes[] = {

    // Ogg
    { "application/ogg", "ogx" },
    { "audio/ogg", "ogg" },
    { "audio/ogg", "oga" },
    { "video/ogg", "ogv" },

    // Annodex
    { "application/annodex", "anx" },
    { "audio/annodex", "axa" },
    { "video/annodex", "axv" },
    { "audio/speex", "spx" },

    // WebM
    { "video/webm", "webm" },
    { "audio/webm", "webm" },

    // MPEG
    { "audio/mpeg", "m1a" },
    { "audio/mpeg", "m2a" },
    { "audio/mpeg", "m1s" },
    { "audio/mpeg", "mpa" },
    { "video/mpeg", "mpg" },
    { "video/mpeg", "m15" },
    { "video/mpeg", "m1s" },
    { "video/mpeg", "m1v" },
    { "video/mpeg", "m75" },
    { "video/mpeg", "mpa" },
    { "video/mpeg", "mpeg" },
    { "video/mpeg", "mpm" },
    { "video/mpeg", "mpv" },

    // MPEG playlist
    { "application/vnd.apple.mpegurl", "m3u8" },
    { "application/mpegurl", "m3u8" },
    { "application/x-mpegurl", "m3u8" },
    { "audio/mpegurl", "m3url" },
    { "audio/x-mpegurl", "m3url" },
    { "audio/mpegurl", "m3u" },
    { "audio/x-mpegurl", "m3u" },

    // MPEG-4
    { "video/x-m4v", "m4v" },
    { "audio/x-m4a", "m4a" },
    { "audio/x-m4b", "m4b" },
    { "audio/x-m4p", "m4p" },
    { "audio/mp4", "m4a" },

    // MP3
    { "audio/mp3", "mp3" },
    { "audio/x-mp3", "mp3" },
    { "audio/x-mpeg", "mp3" },

    // MPEG-2
    { "video/x-mpeg2", "mp2" },
    { "video/mpeg2", "vob" },
    { "video/mpeg2", "mod" },
    { "video/m2ts", "m2ts" },
    { "video/x-m2ts", "m2t" },
    { "video/x-m2ts", "ts" },

    // 3GP/3GP2
    { "audio/3gpp", "3gpp" }, 
    { "audio/3gpp2", "3g2" }, 
    { "application/x-mpeg", "amc" },

    // AAC
    { "audio/aac", "aac" },
    { "audio/aac", "adts" },
    { "audio/x-aac", "m4r" },

    // CoreAudio File
    { "audio/x-caf", "caf" },
    { "audio/x-gsm", "gsm" },

    // ADPCM
    { "audio/x-wav", "wav" }
};

static const char textPlain[] = "text/plain";
static const char textHtml[] = "text/html";
static const char imageJpeg[] = "image/jpeg";
static const char octetStream[] = "application/octet-stream";

// A table of well known MIME types used when we don't want to leak to the
// caller information about types known to underlying platform.
static const TypeExtensionPair wellKnownMimeTypes[] = {
    { textPlain, "txt" },
    { textPlain, "text" },
    { textHtml, "html" },
    { textHtml, "htm" },
    { "text/css", "css" },
    { "text/xml", "xml" },
    { "text/xsl", "xsl" },
    { "image/gif", "gif" },
    { "image/png", "png" },
    { imageJpeg, "jpeg" },
    { imageJpeg, "jpg" },
    { imageJpeg, "jfif" },
    { imageJpeg, "pjpeg" },
    { "image/webp", "webp" },
    { "image/bmp", "bmp" },
    { "application/xhtml+xml", "xhtml" },
    { "application/x-javascript", "js" },
    { "application/json", "json" },
    { octetStream, "exe" },
    { octetStream, "com" },
    { octetStream, "bin" },
    { "application/zip", "zip" },
    { "application/gzip", "gz" },
    { "application/pdf", "pdf" },
    { "application/postscript", "ps" },
    { "image/x-icon", "ico" },
    { "image/tiff", "tiff" },
    { "image/x-xbitmap", "xbm" },
    { "image/svg+xml", "svg" },
    { "application/rss+xml", "rss" },
    { "application/rdf+xml", "rdf" },
    { "application/x-shockwave-flash", "swf" },
};

static HashSet<String>* supportedImageResourceMIMETypes;
static HashSet<String>* supportedImageMIMETypes;
static HashSet<String>* supportedImageMIMETypesForEncoding;
static HashSet<String>* supportedJavaScriptMIMETypes;
static HashSet<String>* supportedNonImageMIMETypes;
static HashSet<String>* supportedMediaMIMETypes;
static HashSet<String>* pdfAndPostScriptMIMETypes;
static HashSet<String>* unsupportedTextMIMETypes;

typedef HashMap<String, Vector<String>*, CaseFoldingHash> MediaMIMETypeMap;
    
static void initializeSupportedImageMIMETypes()
{
#if USE(CG)
    RetainPtr<CFArrayRef> supportedTypes = adoptCF(CGImageSourceCopyTypeIdentifiers());
    CFIndex count = CFArrayGetCount(supportedTypes.get());
    for (CFIndex i = 0; i < count; i++) {
        CFStringRef supportedType = reinterpret_cast<CFStringRef>(CFArrayGetValueAtIndex(supportedTypes.get(), i));
        String mimeType = MIMETypeForImageSourceType(supportedType);
        if (!mimeType.isEmpty()) {
            supportedImageMIMETypes->add(mimeType);
            supportedImageResourceMIMETypes->add(mimeType);
        }
    }

    // On Tiger and Leopard, com.microsoft.bmp doesn't have a MIME type in the registry.
    supportedImageMIMETypes->add("image/bmp");
    supportedImageResourceMIMETypes->add("image/bmp");

    // Favicons don't have a MIME type in the registry either.
    supportedImageMIMETypes->add("image/vnd.microsoft.icon");
    supportedImageMIMETypes->add("image/x-icon");
    supportedImageResourceMIMETypes->add("image/vnd.microsoft.icon");
    supportedImageResourceMIMETypes->add("image/x-icon");

    //  We only get one MIME type per UTI, hence our need to add these manually
    supportedImageMIMETypes->add("image/pjpeg");
    supportedImageResourceMIMETypes->add("image/pjpeg");

    //  We don't want to try to treat all binary data as an image
    supportedImageMIMETypes->remove("application/octet-stream");
    supportedImageResourceMIMETypes->remove("application/octet-stream");

    //  Don't treat pdf/postscript as images directly
    supportedImageMIMETypes->remove("application/pdf");
    supportedImageMIMETypes->remove("application/postscript");

#else
    // assume that all implementations at least support the following standard
    // image types:
    static const char* types[] = {
        "image/jpeg",
        "image/png",
        "image/gif",
        "image/bmp",
        "image/vnd.microsoft.icon",    // ico
        "image/x-icon",    // ico
        "image/x-xbitmap"  // xbm
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(types); ++i) {
        supportedImageMIMETypes->add(types[i]);
        supportedImageResourceMIMETypes->add(types[i]);
    }

#if USE(WEBP)
    supportedImageMIMETypes->add("image/webp");
    supportedImageResourceMIMETypes->add("image/webp");
#endif

#if PLATFORM(QT)
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    QList<QByteArray> mimeTypes = QImageReader::supportedMimeTypes();
    Q_FOREACH(const QByteArray& mimeType, mimeTypes) {
        supportedImageMIMETypes->add(mimeType.constData());
        supportedImageResourceMIMETypes->add(mimeType.constData());
    }
#else
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
    for (int i = 0; i < formats.size(); ++i) {
        String mimeType = MIMETypeRegistry::getMIMETypeForExtension(formats.at(i).constData());
        if (!mimeType.isEmpty()) {
            supportedImageMIMETypes->add(mimeType);
            supportedImageResourceMIMETypes->add(mimeType);
        }
    }
#endif // QT_VERSION
#if ENABLE(SVG)
    // Do not treat SVG as images directly if WebKit can handle them.
    supportedImageMIMETypes->remove("image/svg+xml");
    supportedImageResourceMIMETypes->remove("image/svg+xml");
#endif
#endif // PLATFORM(QT)
#endif // USE(CG)
}

static void initializeSupportedImageMIMETypesForEncoding()
{
    supportedImageMIMETypesForEncoding = new HashSet<String>;

#if USE(CG)
#if PLATFORM(MAC)
    RetainPtr<CFArrayRef> supportedTypes = adoptCF(CGImageDestinationCopyTypeIdentifiers());
    CFIndex count = CFArrayGetCount(supportedTypes.get());
    for (CFIndex i = 0; i < count; i++) {
        CFStringRef supportedType = reinterpret_cast<CFStringRef>(CFArrayGetValueAtIndex(supportedTypes.get(), i));
        String mimeType = MIMETypeForImageSourceType(supportedType);
        if (!mimeType.isEmpty())
            supportedImageMIMETypesForEncoding->add(mimeType);
    }
#else
    // FIXME: Add Windows support for all the supported UTI's when a way to convert from MIMEType to UTI reliably is found.
    // For now, only support PNG, JPEG and GIF.  See <rdar://problem/6095286>.
    supportedImageMIMETypesForEncoding->add("image/png");
    supportedImageMIMETypesForEncoding->add("image/jpeg");
    supportedImageMIMETypesForEncoding->add("image/gif");
#endif
#elif PLATFORM(QT)
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    QList<QByteArray> mimeTypes = QImageWriter::supportedMimeTypes();
    Q_FOREACH(const QByteArray& mimeType, mimeTypes) {
        supportedImageMIMETypesForEncoding->add(mimeType.constData());
    }
#else
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    for (int i = 0; i < formats.size(); ++i) {
        String mimeType = MIMETypeRegistry::getMIMETypeForExtension(formats.at(i).constData());
        if (!mimeType.isEmpty())
            supportedImageMIMETypesForEncoding->add(mimeType);
    }
#endif // QT_VERSION
#elif PLATFORM(GTK)
    supportedImageMIMETypesForEncoding->add("image/png");
    supportedImageMIMETypesForEncoding->add("image/jpeg");
    supportedImageMIMETypesForEncoding->add("image/tiff");
    supportedImageMIMETypesForEncoding->add("image/bmp");
    supportedImageMIMETypesForEncoding->add("image/ico");
#elif USE(CAIRO)
    supportedImageMIMETypesForEncoding->add("image/png");
#elif PLATFORM(BLACKBERRY)
    supportedImageMIMETypesForEncoding->add("image/png");
    supportedImageMIMETypesForEncoding->add("image/jpeg");
#endif
}

static void initializeSupportedJavaScriptMIMETypes()
{
    /*
        Mozilla 1.8 and WinIE 7 both accept text/javascript and text/ecmascript.
        Mozilla 1.8 accepts application/javascript, application/ecmascript, and application/x-javascript, but WinIE 7 doesn't.
        WinIE 7 accepts text/javascript1.1 - text/javascript1.3, text/jscript, and text/livescript, but Mozilla 1.8 doesn't.
        Mozilla 1.8 allows leading and trailing whitespace, but WinIE 7 doesn't.
        Mozilla 1.8 and WinIE 7 both accept the empty string, but neither accept a whitespace-only string.
        We want to accept all the values that either of these browsers accept, but not other values.
     */
    static const char* types[] = {
        "text/javascript",
        "text/ecmascript",
        "application/javascript",
        "application/ecmascript",
        "application/x-javascript",
        "text/javascript1.1",
        "text/javascript1.2",
        "text/javascript1.3",
        "text/jscript",
        "text/livescript",
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(types); ++i)
      supportedJavaScriptMIMETypes->add(types[i]);
}

static void initializePDFAndPostScriptMIMETypes()
{
    const char* const types[] = {
        "application/pdf",
        "text/pdf",
        "application/postscript",
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(types); ++i)
        pdfAndPostScriptMIMETypes->add(types[i]);
}

static void initializeSupportedNonImageMimeTypes()
{
    static const char* types[] = {
        "text/html",
        "text/xml",
        "text/xsl",
        "text/plain",
        "text/",
        "application/xml",
        "application/xhtml+xml",
        "application/vnd.wap.xhtml+xml",
        "application/rss+xml",
        "application/atom+xml",
        "application/json",
#if ENABLE(SVG)
        "image/svg+xml",
#endif
#if ENABLE(FTPDIR)
        "application/x-ftp-directory",
#endif
        "multipart/x-mixed-replace"
        // Note: ADDING a new type here will probably render it as HTML. This can
        // result in cross-site scripting.
    };
    COMPILE_ASSERT(sizeof(types) / sizeof(types[0]) <= 16,
                   nonimage_mime_types_must_be_less_than_or_equal_to_16);

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(types); ++i)
        supportedNonImageMIMETypes->add(types[i]);

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
    ArchiveFactory::registerKnownArchiveMIMETypes();
#endif
}

static MediaMIMETypeMap& mediaMIMETypeMap()
{
    DEFINE_STATIC_LOCAL(MediaMIMETypeMap, mediaMIMETypeForExtensionMap, ());

    if (!mediaMIMETypeForExtensionMap.isEmpty())
        return mediaMIMETypeForExtensionMap;

    const unsigned numPairs = sizeof(commonMediaTypes) / sizeof(commonMediaTypes[0]);
    for (unsigned ndx = 0; ndx < numPairs; ++ndx) {

        if (mediaMIMETypeForExtensionMap.contains(commonMediaTypes[ndx].extension))
            mediaMIMETypeForExtensionMap.get(commonMediaTypes[ndx].extension)->append(commonMediaTypes[ndx].type);
        else {
            Vector<String>* synonyms = new Vector<String>;

            // If there is a system specific type for this extension, add it as the first type so
            // getMediaMIMETypeForExtension will always return it.
            String systemType = MIMETypeRegistry::getMIMETypeForExtension(commonMediaTypes[ndx].extension);
            if (!systemType.isEmpty() && commonMediaTypes[ndx].type != systemType)
                synonyms->append(systemType);
            synonyms->append(commonMediaTypes[ndx].type);
            mediaMIMETypeForExtensionMap.add(commonMediaTypes[ndx].extension, synonyms);
        }
    }

    return mediaMIMETypeForExtensionMap;
}

String MIMETypeRegistry::getMediaMIMETypeForExtension(const String& ext)
{
    // Look in the system-specific registry first.
    String type = getMIMETypeForExtension(ext);
    if (!type.isEmpty())
        return type;

    Vector<String>* typeList = mediaMIMETypeMap().get(ext);
    if (typeList)
        return (*typeList)[0];
    
    return String();
}
    
Vector<String> MIMETypeRegistry::getMediaMIMETypesForExtension(const String& ext)
{
    Vector<String>* typeList = mediaMIMETypeMap().get(ext);
    if (typeList)
        return *typeList;

    // Only need to look in the system-specific registry if mediaMIMETypeMap() doesn't contain
    // the extension at all, because it always contains the system-specific type if the
    // extension is in the static mapping table.
    String type = getMIMETypeForExtension(ext);
    if (!type.isEmpty()) {
        Vector<String> typeList;
        typeList.append(type);
        return typeList;
    }
    
    return Vector<String>();
}

static void initializeSupportedMediaMIMETypes()
{
    supportedMediaMIMETypes = new HashSet<String>;
#if ENABLE(VIDEO)
    MediaPlayer::getSupportedTypes(*supportedMediaMIMETypes);
#endif
}

static void initializeUnsupportedTextMIMETypes()
{
    static const char* types[] = {
        "text/calendar",
        "text/x-calendar",
        "text/x-vcalendar",
        "text/vcalendar",
        "text/vcard",
        "text/x-vcard",
        "text/directory",
        "text/ldif",
        "text/qif",
        "text/x-qif",
        "text/x-csv",
        "text/x-vcf",
        "text/rtf",
    };
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(types); ++i)
      unsupportedTextMIMETypes->add(types[i]);
}

static void initializeMIMETypeRegistry()
{
    supportedJavaScriptMIMETypes = new HashSet<String>;
    initializeSupportedJavaScriptMIMETypes();

    supportedNonImageMIMETypes = new HashSet<String>(*supportedJavaScriptMIMETypes);
    initializeSupportedNonImageMimeTypes();

    supportedImageResourceMIMETypes = new HashSet<String>;
    supportedImageMIMETypes = new HashSet<String>;
    initializeSupportedImageMIMETypes();

    pdfAndPostScriptMIMETypes = new HashSet<String>;
    initializePDFAndPostScriptMIMETypes();

    unsupportedTextMIMETypes = new HashSet<String>;
    initializeUnsupportedTextMIMETypes();
}

static String findMimeType(const TypeExtensionPair* pairs, unsigned numPairs, const String& extension)
{
    if (!extension.isEmpty()) {
      for (unsigned i = 0; i < numPairs; ++i, ++pairs) {
          if (equalIgnoringCase(extension, pairs->extension))
              return String(pairs->type);
      }
    }
    return String();
}

String MIMETypeRegistry::getWellKnownMIMETypeForExtension(const String& extension)
{
    // This method must be thread safe and should not consult the OS/registry.
    String found = findMimeType(wellKnownMimeTypes, sizeof(wellKnownMimeTypes) / sizeof(wellKnownMimeTypes[0]), extension);
    if (!found.isEmpty())
        return found;
    return findMimeType(commonMediaTypes, sizeof(commonMediaTypes) / sizeof(commonMediaTypes[0]), extension);
}

#if !PLATFORM(QT)
String MIMETypeRegistry::getMIMETypeForPath(const String& path)
{
    size_t pos = path.reverseFind('.');
    if (pos != notFound) {
        String extension = path.substring(pos + 1);
        String result = getMIMETypeForExtension(extension);
        if (result.length())
            return result;
    }
    return defaultMIMEType();
}
#endif

bool MIMETypeRegistry::isSupportedImageMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!supportedImageMIMETypes)
        initializeMIMETypeRegistry();
    return supportedImageMIMETypes->contains(getNormalizedMIMEType(mimeType));
}

bool MIMETypeRegistry::isSupportedImageResourceMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!supportedImageResourceMIMETypes)
        initializeMIMETypeRegistry();
    return supportedImageResourceMIMETypes->contains(getNormalizedMIMEType(mimeType));
}

bool MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(const String& mimeType)
{
    ASSERT(isMainThread());

    if (mimeType.isEmpty())
        return false;
    if (!supportedImageMIMETypesForEncoding)
        initializeSupportedImageMIMETypesForEncoding();
    return supportedImageMIMETypesForEncoding->contains(mimeType);
}

bool MIMETypeRegistry::isSupportedJavaScriptMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!supportedJavaScriptMIMETypes)
        initializeMIMETypeRegistry();
    return supportedJavaScriptMIMETypes->contains(mimeType);
}

bool MIMETypeRegistry::isSupportedNonImageMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!supportedNonImageMIMETypes)
        initializeMIMETypeRegistry();
    return supportedNonImageMIMETypes->contains(mimeType);
}

bool MIMETypeRegistry::isSupportedMediaMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!supportedMediaMIMETypes)
        initializeSupportedMediaMIMETypes();
    return supportedMediaMIMETypes->contains(mimeType);
}

bool MIMETypeRegistry::isUnsupportedTextMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!unsupportedTextMIMETypes)
        initializeMIMETypeRegistry();
    return unsupportedTextMIMETypes->contains(mimeType);
}

bool MIMETypeRegistry::isJavaAppletMIMEType(const String& mimeType)
{
    // Since this set is very limited and is likely to remain so we won't bother with the overhead
    // of using a hash set.
    // Any of the MIME types below may be followed by any number of specific versions of the JVM,
    // which is why we use startsWith()
    return mimeType.startsWith("application/x-java-applet", false)
        || mimeType.startsWith("application/x-java-bean", false)
        || mimeType.startsWith("application/x-java-vm", false);
}

bool MIMETypeRegistry::isPDFOrPostScriptMIMEType(const String& mimeType)
{
    if (mimeType.isEmpty())
        return false;
    if (!pdfAndPostScriptMIMETypes)
        initializeMIMETypeRegistry();
    return pdfAndPostScriptMIMETypes->contains(mimeType);
}

bool MIMETypeRegistry::canShowMIMEType(const String& mimeType)
{
    if (isSupportedImageMIMEType(mimeType) || isSupportedNonImageMIMEType(mimeType) || isSupportedMediaMIMEType(mimeType))
        return true;

    if (mimeType.startsWith("text/", false))
        return !MIMETypeRegistry::isUnsupportedTextMIMEType(mimeType);

    return false;
}

HashSet<String>& MIMETypeRegistry::getSupportedImageMIMETypes()
{
    if (!supportedImageMIMETypes)
        initializeMIMETypeRegistry();
    return *supportedImageMIMETypes;
}

HashSet<String>& MIMETypeRegistry::getSupportedImageResourceMIMETypes()
{
    if (!supportedImageResourceMIMETypes)
        initializeMIMETypeRegistry();
    return *supportedImageResourceMIMETypes;
}

HashSet<String>& MIMETypeRegistry::getSupportedImageMIMETypesForEncoding()
{
    if (!supportedImageMIMETypesForEncoding)
        initializeSupportedImageMIMETypesForEncoding();
    return *supportedImageMIMETypesForEncoding;
}

HashSet<String>& MIMETypeRegistry::getSupportedNonImageMIMETypes()
{
    if (!supportedNonImageMIMETypes)
        initializeMIMETypeRegistry();
    return *supportedNonImageMIMETypes;
}

HashSet<String>& MIMETypeRegistry::getSupportedMediaMIMETypes()
{
    if (!supportedMediaMIMETypes)
        initializeSupportedMediaMIMETypes();
    return *supportedMediaMIMETypes;
}

HashSet<String>& MIMETypeRegistry::getPDFAndPostScriptMIMETypes()
{
    if (!pdfAndPostScriptMIMETypes)
        initializeMIMETypeRegistry();
    return *pdfAndPostScriptMIMETypes;
}

HashSet<String>& MIMETypeRegistry::getUnsupportedTextMIMETypes()
{
    if (!unsupportedTextMIMETypes)
        initializeMIMETypeRegistry();
    return *unsupportedTextMIMETypes;
}

const String& defaultMIMEType()
{
    DEFINE_STATIC_LOCAL(const String, defaultMIMEType, (ASCIILiteral("application/octet-stream")));
    return defaultMIMEType;
}

#if !PLATFORM(QT) && !PLATFORM(BLACKBERRY) && !USE(CURL)
String MIMETypeRegistry::getNormalizedMIMEType(const String& mimeType)
{
    return mimeType;
}
#endif

#if PLATFORM(BLACKBERRY) || USE(CURL)
typedef HashMap<String, String> MIMETypeAssociationMap;

static const MIMETypeAssociationMap& mimeTypeAssociationMap()
{
    static MIMETypeAssociationMap* mimeTypeMap = 0;
    if (mimeTypeMap)
        return *mimeTypeMap;

    mimeTypeMap = new MIMETypeAssociationMap;

    mimeTypeMap->add(ASCIILiteral("image/x-ms-bmp"), ASCIILiteral("image/bmp"));
    mimeTypeMap->add(ASCIILiteral("image/x-windows-bmp"), ASCIILiteral("image/bmp"));
    mimeTypeMap->add(ASCIILiteral("image/x-bmp"), ASCIILiteral("image/bmp"));
    mimeTypeMap->add(ASCIILiteral("image/x-bitmap"), ASCIILiteral("image/bmp"));
    mimeTypeMap->add(ASCIILiteral("image/x-ms-bitmap"), ASCIILiteral("image/bmp"));
    mimeTypeMap->add(ASCIILiteral("image/jpg"), ASCIILiteral("image/jpeg"));
    mimeTypeMap->add(ASCIILiteral("image/pjpeg"), ASCIILiteral("image/jpeg"));
    mimeTypeMap->add(ASCIILiteral("image/x-png"), ASCIILiteral("image/png"));
    mimeTypeMap->add(ASCIILiteral("image/vnd.rim.png"), ASCIILiteral("image/png"));
    mimeTypeMap->add(ASCIILiteral("image/ico"), ASCIILiteral("image/vnd.microsoft.icon"));
    mimeTypeMap->add(ASCIILiteral("image/icon"), ASCIILiteral("image/vnd.microsoft.icon"));
    mimeTypeMap->add(ASCIILiteral("text/ico"), ASCIILiteral("image/vnd.microsoft.icon"));
    mimeTypeMap->add(ASCIILiteral("application/ico"), ASCIILiteral("image/vnd.microsoft.icon"));
    mimeTypeMap->add(ASCIILiteral("image/x-icon"), ASCIILiteral("image/vnd.microsoft.icon"));
    mimeTypeMap->add(ASCIILiteral("audio/vnd.qcelp"), ASCIILiteral("audio/qcelp"));
    mimeTypeMap->add(ASCIILiteral("audio/qcp"), ASCIILiteral("audio/qcelp"));
    mimeTypeMap->add(ASCIILiteral("audio/vnd.qcp"), ASCIILiteral("audio/qcelp"));
    mimeTypeMap->add(ASCIILiteral("audio/wav"), ASCIILiteral("audio/x-wav"));
    mimeTypeMap->add(ASCIILiteral("audio/mid"), ASCIILiteral("audio/midi"));
    mimeTypeMap->add(ASCIILiteral("audio/sp-midi"), ASCIILiteral("audio/midi"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mid"), ASCIILiteral("audio/midi"));
    mimeTypeMap->add(ASCIILiteral("audio/x-midi"), ASCIILiteral("audio/midi"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mpeg"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/mp3"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mp3"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/mpeg3"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mpeg3"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/mpg3"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/mpg"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mpg"), ASCIILiteral("audio/mpeg"));
    mimeTypeMap->add(ASCIILiteral("audio/m4a"), ASCIILiteral("audio/mp4"));
    mimeTypeMap->add(ASCIILiteral("audio/x-m4a"), ASCIILiteral("audio/mp4"));
    mimeTypeMap->add(ASCIILiteral("audio/x-mp4"), ASCIILiteral("audio/mp4"));
    mimeTypeMap->add(ASCIILiteral("audio/x-aac"), ASCIILiteral("audio/aac"));
    mimeTypeMap->add(ASCIILiteral("audio/x-amr"), ASCIILiteral("audio/amr"));
    mimeTypeMap->add(ASCIILiteral("audio/mpegurl"), ASCIILiteral("audio/x-mpegurl"));
    mimeTypeMap->add(ASCIILiteral("audio/flac"), ASCIILiteral("audio/x-flac"));
    mimeTypeMap->add(ASCIILiteral("video/3gp"), ASCIILiteral("video/3gpp"));
    mimeTypeMap->add(ASCIILiteral("video/avi"), ASCIILiteral("video/x-msvideo"));
    mimeTypeMap->add(ASCIILiteral("video/x-m4v"), ASCIILiteral("video/mp4"));
    mimeTypeMap->add(ASCIILiteral("video/x-quicktime"), ASCIILiteral("video/quicktime"));
    mimeTypeMap->add(ASCIILiteral("application/java"), ASCIILiteral("application/java-archive"));
    mimeTypeMap->add(ASCIILiteral("application/x-java-archive"), ASCIILiteral("application/java-archive"));
    mimeTypeMap->add(ASCIILiteral("application/x-zip-compressed"), ASCIILiteral("application/zip"));
    mimeTypeMap->add(ASCIILiteral("text/cache-manifest"), ASCIILiteral("text/plain"));

    return *mimeTypeMap;
}

String MIMETypeRegistry::getNormalizedMIMEType(const String& mimeType)
{
    MIMETypeAssociationMap::const_iterator it = mimeTypeAssociationMap().find(mimeType);

    if (it != mimeTypeAssociationMap().end())
        return it->value;

    return mimeType;
}
#endif

} // namespace WebCore
