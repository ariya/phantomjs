/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "AboutData.h"

#include "AboutTemplate.html.cpp"
#include "CacheHelper.h"
#include "CookieManager.h"
#include "JSDOMWindow.h"
#include "MemoryCache.h"
#include "MemoryStatistics.h"
#include "SurfacePool.h"
#include "WebKitVersion.h"

#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMemory.h>
#include <BlackBerryPlatformSettings.h>
#include <BuildInformation.h>
#include <heap/Heap.h>
#include <process.h>
#include <runtime/VM.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static String writeFeatures(const Vector<String>& trueList, const Vector<String>& falseList)
{
    String ret;
    for (unsigned int i = 0, j = 0; i < trueList.size() || j < falseList.size();) {
        bool pickFromFalse = ((i >= trueList.size()) || (j < falseList.size() && strcmp(falseList[j].ascii().data(), trueList[i].ascii().data()) < 0));
        const String& item = (pickFromFalse ? falseList : trueList)[ (pickFromFalse ? j : i)++ ];
        ret += String("<tr><td><div class='" + String(pickFromFalse ? "false" : "true") + "'" + (item.length() >= 30 ? " style='font-size:10px;' " : "") + ">" + item + "</div></td></tr>");
    }
    return ret;
}

template<class T> static String numberToHTMLTr(const String& description, T number)
{
    return String("<tr><td>") + description + "</td><td>" + String::number(number) + "</td></tr>";
}

template<> String numberToHTMLTr<bool>(const String& description, bool truth)
{
    return String("<tr><td>") + description + "</td><td>" + (truth?"true":"false") + "</td></tr>";
}

static String configPage()
{
    StringBuilder builder;
#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD
    builder.append(writeHeader("Configuration"));
    builder.appendLiteral("<div class=\"box\"><div class=\"box-title\">Compiler Information</div><table class='fixed-table'><col width=75%><col width=25%>");
#if COMPILER(MSVC)
    builder.appendLiteral("<tr><td>Microsoft Visual C++</td><td>MSVC</td></tr>");
    builder.append("<tr><td>_MSC_VER</td><td>" + String::number(_MSC_VER) + "</td></tr>");
    builder.append("<tr><td>_MSC_FULL_VER</td><td>" + String::number(_MSC_FULL_VER) + "</td></tr>");
    builder.append("<tr><td>_MSC_BUILD</td><td>" + String::number(_MSC_BUILD) + "</td></tr>");
#elif COMPILER(RVCT)
    builder.appendLiteral("<tr><td>ARM RealView Compiler Toolkit</td><td>RVCT</td></tr>");
    builder.append("<tr><td>__ARMCC_VERSION</td><td>" + String::number(__ARMCC_VERSION) + "</td></tr>");
#if COMPILER(RVCT4GNU)
    builder.appendLiteral("<tr><td>RVCT 4+ in --gnu mode</td><td>1</td></tr>");
#endif
#elif COMPILER(GCC)
    builder.append("<tr><td>GCC</td><td>" + String::number(__GNUC__) + "." + String::number(__GNUC_MINOR__) + "." + String::number(__GNUC_PATCHLEVEL__) + "</td></tr>");
#endif

    // Add "" to satisfy check-webkit-style.
    builder.appendLiteral("");

    builder.append(String("</table></div><br><div class='box'><div class='box-title'>CPU Information</div><table class='fixed-table'><col width=75%><col width=25%>"));
#if CPU(X86)
    builder.appendLiteral("<tr><td>X86</td><td></td></tr>");
#elif CPU(ARM)
    builder.appendLiteral("<tr><td>ARM</td><td></td></tr>");
    builder.append("<tr><td>ARM_ARCH_VERSION</td><td>" + String::number(WTF_ARM_ARCH_VERSION) + "</td></tr>");
    builder.append("<tr><td>THUMB_ARCH_VERSION</td><td>" + String::number(WTF_THUMB_ARCH_VERSION) + "</td></tr>");
    builder.append("<tr><td>THUMB2</td><td>" + String::number(WTF_CPU_ARM_THUMB2) + "</td></tr>");
#endif
    builder.appendLiteral("<tr><td>Endianness</td><td>");
#if CPU(BIG_ENDIAN)
    builder.appendLiteral("big");
#elif CPU(MIDDLE_ENDIAN)
    builder.appendLiteral("middle");
#else
    builder.appendLiteral("little");
#endif
    builder.appendLiteral("</td></tr>");

    builder.append(String("</table></div><br><div class='box'><div class='box-title'>Platform Information</div><table class='fixed-table'><col width=75%><col width=25%>"));
    builder.append("<tr><td>WebKit Version</td><td>" + String::number(WEBKIT_MAJOR_VERSION) + "." + String::number(WEBKIT_MINOR_VERSION) + "</td></tr>");
    builder.appendLiteral("<tr><td>BlackBerry</td><td>");
#if PLATFORM(BLACKBERRY)
    builder.appendLiteral("1");
#else
    builder.appendLiteral("0");
#endif
    builder.appendLiteral("</td></tr>");
    builder.appendLiteral("<tr><td>__STDC_ISO_10646__</td><td>");
#ifdef __STDC_ISO_10646__
    builder.appendLiteral("1");
#else
    builder.appendLiteral("0");
#endif
    builder.appendLiteral("</td></tr>");

    BlackBerry::Platform::Settings* settings = BlackBerry::Platform::Settings::instance();
    builder.append(String("</table></div><br><div class='box'><div class='box-title'>Platform Settings</div><table style='font-size:11px;' class='fixed-table'><col width=75%><col width=25%>"));
    builder.append(numberToHTMLTr("isRSSFilteringEnabled", settings->isRSSFilteringEnabled()));
    builder.append(numberToHTMLTr("maxPixelsPerDecodedImage", settings->maxPixelsPerDecodedImage()));
    builder.append(numberToHTMLTr("shouldReportLowMemoryToUser", settings->shouldReportLowMemoryToUser()));
    builder.append(numberToHTMLTr("numberOfBackingStoreFrontBuffers", settings->numberOfBackingStoreFrontBuffers()));
    builder.append(numberToHTMLTr("numberOfBackingStoreBackBuffers", settings->numberOfBackingStoreBackBuffers()));
    builder.append(numberToHTMLTr("tabsSupportedByClient", settings->tabsSupportedByClient()));
    builder.append(numberToHTMLTr("contextMenuEnabled", settings->contextMenuEnabled()));
    builder.append(numberToHTMLTr("selectionEnabled", settings->selectionEnabled()));
    builder.append(numberToHTMLTr("fineCursorControlEnabled", settings->fineCursorControlEnabled()));
    builder.append(numberToHTMLTr("alwaysShowKeyboardOnFocus", settings->alwaysShowKeyboardOnFocus()));
    builder.append(numberToHTMLTr("allowedScrollAdjustmentForInputFields", settings->allowedScrollAdjustmentForInputFields()));
    builder.append(numberToHTMLTr("unrestrictedResizeEvents", settings->unrestrictedResizeEvents()));
    builder.append(numberToHTMLTr("isBridgeBrowser", settings->isBridgeBrowser()));
    builder.append(numberToHTMLTr("showImageLocationOptionsInGCM", settings->showImageLocationOptionsInGCM()));
    builder.append(numberToHTMLTr("forceGLES2WindowUsage", settings->forceGLES2WindowUsage()));
    builder.append(numberToHTMLTr("maxClickableSpeed", settings->maxClickableSpeed()));
    builder.append(numberToHTMLTr("maxJitterRadiusClick", settings->maxJitterRadiusClick()));
    builder.append(numberToHTMLTr("maxJitterRadiusTap", settings->maxJitterRadiusTap()));
    builder.append(numberToHTMLTr("maxJitterRadiusSingleTouchMove", settings->maxJitterRadiusSingleTouchMove()));
    builder.append(numberToHTMLTr("maxJitterRadiusTouchHold", settings->maxJitterRadiusTouchHold()));
    builder.append(numberToHTMLTr("maxJitterRadiusHandleDrag", settings->maxJitterRadiusHandleDrag()));
    builder.append(numberToHTMLTr("maxJitterDistanceClick", settings->maxJitterDistanceClick()));
    builder.append(numberToHTMLTr("maxJitterDistanceTap", settings->maxJitterDistanceTap()));
    builder.append(numberToHTMLTr("maxJitterDistanceSingleTouchMove", settings->maxJitterDistanceSingleTouchMove()));
    builder.append(numberToHTMLTr("maxJitterDistanceTouchHold", settings->maxJitterDistanceTouchHold()));
    builder.append(numberToHTMLTr("maxJitterDistanceHandleDrag", settings->maxJitterDistanceHandleDrag()));
    builder.append(numberToHTMLTr("topFatFingerPadding", settings->topFatFingerPadding()));
    builder.append(numberToHTMLTr("rightFatFingerPadding", settings->rightFatFingerPadding()));
    builder.append(numberToHTMLTr("bottomFatFingerPadding", settings->bottomFatFingerPadding()));
    builder.append(numberToHTMLTr("maxSelectionNeckHeight", settings->maxSelectionNeckHeight()));
    builder.append(numberToHTMLTr("leftFatFingerPadding", settings->leftFatFingerPadding()));

    Vector<String> trueList, falseList;
#include "AboutDataEnableFeatures.cpp"
    builder.append(String("</table></div><br><div class='box'><div class='box-title'>WebKit Features (ENABLE_)</div><table class='fixed-table'>"));

    builder.append(writeFeatures(trueList, falseList));

    trueList.clear();
    falseList.clear();
#include "AboutDataHaveFeatures.cpp"
    builder.append(String("</table></div><br><div class='box'><div class='box-title'>WebKit Features (HAVE_)</div><table class='fixed-table'>"));

    builder.append(writeFeatures(trueList, falseList));

    trueList.clear();
    falseList.clear();
#include "AboutDataUseFeatures.cpp"
    builder.append(String("</table></div><br><div class='box'><div class='box-title'>WebKit Features (USE_)</div><table class='fixed-table'>"));
    builder.append(writeFeatures(trueList, falseList));
    builder.append(String("</table></div></body></html>"));
#endif

    return builder.toString();
}

static String cacheTypeStatisticToHTMLTr(const String& description, const MemoryCache::TypeStatistic& statistic)
{
    const int s_kiloByte = 1024;
    return String("<tr><td>") + description + "</td>"
        + "<td>" + String::number(statistic.count) + "</td>"
        + "<td>" + String::number(statistic.size / s_kiloByte) + "</td>"
        + "<td>" + String::number(statistic.liveSize / s_kiloByte) + "</td>"
        + "<td>" + String::number(statistic.decodedSize / s_kiloByte) + "</td>"
        + "</tr>";
}

static void dumpJSCTypeCountSetToTableHTML(StringBuilder& tableHTML, JSC::TypeCountSet* typeCountSet)
{
    if (!typeCountSet)
        return;

    for (JSC::TypeCountSet::const_iterator iter = typeCountSet->begin(); iter != typeCountSet->end(); ++iter)
        tableHTML.append(numberToHTMLTr(iter->key, iter->value));
}

#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD
static inline struct malloc_stats mallocStats()
{
    struct malloc_stats ms;
    mallopt(MALLOC_STATS, reinterpret_cast<intptr_t>(&ms));
    return ms;
}
#endif

static String memoryPage()
{
    StringBuilder builder;

    builder.append(writeHeader("Memory"));
    builder.appendLiteral("<div class=\"box\"><div class=\"box-title\">Cache Information<br><div style='font-size:11px;color:#A8A8A8'>Size, Living, and Decoded are expressed in KB.</div><br></div><table class='fixed-table'><col width=75%><col width=25%>");

    // generate cache information
    MemoryCache* cacheInc = memoryCache();
    MemoryCache::Statistics cacheStat = cacheInc->getStatistics();

    builder.appendLiteral("<tr> <th align=left>Item</th> <th align=left>Count</th> <th align=left>Size</th> <th align=left>Living</th> <th align=left>Decoded</th></tr>");

    MemoryCache::TypeStatistic total;
    total.count = cacheStat.images.count + cacheStat.cssStyleSheets.count
            + cacheStat.scripts.count + cacheStat.xslStyleSheets.count + cacheStat.fonts.count;
    total.size = cacheInc->totalSize();
    total.liveSize = cacheStat.images.liveSize + cacheStat.cssStyleSheets.liveSize
            + cacheStat.scripts.liveSize + cacheStat.xslStyleSheets.liveSize + cacheStat.fonts.liveSize;
    total.decodedSize = cacheStat.images.decodedSize
            + cacheStat.cssStyleSheets.decodedSize + cacheStat.scripts.decodedSize
            + cacheStat.xslStyleSheets.decodedSize + cacheStat.fonts.decodedSize;

    builder.append(cacheTypeStatisticToHTMLTr("Total", total));
    builder.append(cacheTypeStatisticToHTMLTr("Images", cacheStat.images));
    builder.append(cacheTypeStatisticToHTMLTr("CSS Style Sheets", cacheStat.cssStyleSheets));
    builder.append(cacheTypeStatisticToHTMLTr("Scripts", cacheStat.scripts));
#if ENABLE(XSLT)
    builder.append(cacheTypeStatisticToHTMLTr("XSL Style Sheets", cacheStat.xslStyleSheets));
#endif
    builder.append(cacheTypeStatisticToHTMLTr("Fonts", cacheStat.fonts));

    builder.appendLiteral("</table></div><br>");

#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD

    // JS engine memory usage.
    JSC::GlobalMemoryStatistics jscMemoryStat = JSC::globalMemoryStatistics();
    JSC::Heap& mainHeap = JSDOMWindow::commonVM()->heap;
    OwnPtr<JSC::TypeCountSet> objectTypeCounts = mainHeap.objectTypeCounts();
    OwnPtr<JSC::TypeCountSet> protectedObjectTypeCounts = mainHeap.protectedObjectTypeCounts();

    // Malloc info.
    struct malloc_stats mallocInfo = mallocStats();

    builder.appendLiteral("<div class='box'><div class='box-title'>Process memory usage summary</div><table class='fixed-table'><col width=75%><col width=25%>");

    builder.append(numberToHTMLTr("Total used memory (malloc + JSC)", mallocInfo.m_small_allocmem + mallocInfo.m_allocmem + jscMemoryStat.stackBytes + jscMemoryStat.JITBytes + mainHeap.capacity()));

    if (unsigned totalCommittedMemoryOfChromeProcess = BlackBerry::Platform::totalCommittedMemoryOfChromeProcess()) {
        builder.append(numberToHTMLTr("Total committed memory of tab process", BlackBerry::Platform::totalCommittedMemoryOfCurrentProcess()));
        builder.append(numberToHTMLTr("Total committed memory of chrome process", totalCommittedMemoryOfChromeProcess));
    } else
        builder.append(numberToHTMLTr("Total committed memory", BlackBerry::Platform::totalCommittedMemoryOfCurrentProcess()));

    struct stat processInfo;
    if (!stat(String::format("/proc/%u/as", getpid()).latin1().data(), &processInfo))
        builder.append(numberToHTMLTr("Total mapped memory", processInfo.st_size));

    builder.append(numberToHTMLTr("System free memory", BlackBerry::Platform::systemFreeMemory()));

    builder.appendLiteral("</table></div><br>");

    builder.appendLiteral("<div class='box'><div class='box-title'>JS engine memory usage</div><table class='fixed-table'><col width=75%><col width=25%>");

    builder.append(numberToHTMLTr("Stack size", jscMemoryStat.stackBytes));
    builder.append(numberToHTMLTr("JIT memory usage", jscMemoryStat.JITBytes));
    builder.append(numberToHTMLTr("Main heap capacity", mainHeap.capacity()));
    builder.append(numberToHTMLTr("Main heap size", mainHeap.size()));
    builder.append(numberToHTMLTr("Object count", mainHeap.objectCount()));
    builder.append(numberToHTMLTr("Global object count", mainHeap.globalObjectCount()));
    builder.append(numberToHTMLTr("Protected object count", mainHeap.protectedObjectCount()));
    builder.append(numberToHTMLTr("Protected global object count", mainHeap.protectedGlobalObjectCount()));

    builder.appendLiteral("</table></div><br>");

    builder.appendLiteral("<div class='box'><div class='box-title'>JS object type counts</div><table class='fixed-table'><col width=75%><col width=25%>");
    dumpJSCTypeCountSetToTableHTML(builder, objectTypeCounts.get());
    builder.appendLiteral("</table></div><br>");

    builder.appendLiteral("<div class='box'><div class='box-title'>JS protected object type counts</div><table class='fixed-table'><col width=75%><col width=25%>");
    dumpJSCTypeCountSetToTableHTML(builder, protectedObjectTypeCounts.get());
    builder.appendLiteral("</table></div><br>");

    builder.appendLiteral("<div class='box'><div class='box-title'>Malloc Information</div><table class='fixed-table'><col width=75%><col width=25%>");

    builder.append(numberToHTMLTr("Total space in use", mallocInfo.m_small_allocmem + mallocInfo.m_allocmem));
    builder.append(numberToHTMLTr("Total space in free blocks", mallocInfo.m_small_freemem + mallocInfo.m_freemem));
    builder.append(numberToHTMLTr("Memory in free small blocks", mallocInfo.m_small_freemem));
    builder.append(numberToHTMLTr("Memory in free big blocks", mallocInfo.m_freemem));
    builder.append(numberToHTMLTr("Space in header block headers", mallocInfo.m_small_overhead));
    builder.append(numberToHTMLTr("Space used by block headers", mallocInfo.m_overhead));
    builder.append(numberToHTMLTr("Space in small blocks in use", mallocInfo.m_small_allocmem));
    builder.append(numberToHTMLTr("Space in big blocks in use", mallocInfo.m_allocmem));
    builder.append(numberToHTMLTr("Number of core allocations", mallocInfo.m_coreallocs));
    builder.append(numberToHTMLTr("Number of core de-allocations performed", mallocInfo.m_corefrees));
    builder.append(numberToHTMLTr("Size of the arena", mallocInfo.m_heapsize));
    builder.append(numberToHTMLTr("Number of frees performed", mallocInfo.m_frees));
    builder.append(numberToHTMLTr("Number of allocations performed", mallocInfo.m_allocs));
    builder.append(numberToHTMLTr("Number of realloc functions performed", mallocInfo.m_reallocs));
    builder.append(numberToHTMLTr("Number of small blocks", mallocInfo.m_small_blocks));
    builder.append(numberToHTMLTr("Number of big blocks", mallocInfo.m_blocks));
    builder.append(numberToHTMLTr("Number of header blocks", mallocInfo.m_hblocks));

    builder.append("</table></div>");
#endif

    builder.append("</body></html>");
    return builder.toString();
}

#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD
class MemoryTracker {
public:
    static MemoryTracker& instance();
    void start();
    void stop();
    bool isActive() const { return m_memoryTrackingTimer.isActive(); }
    void clear()
    {
        m_peakTotalUsedMemory = 0;
        m_peakTotalCommittedMemoryOfCurrentProcess = 0;
        m_peakTotalCommittedMemoryOfChromeProcess = 0;
        m_peakTotalMappedMemory = 0;
    }

    void updateMemoryPeaks(Timer<MemoryTracker>*);
    unsigned peakTotalUsedMemory() const { return m_peakTotalUsedMemory; }
    unsigned peakTotalCommittedMemoryOfCurrentProcess() const { return m_peakTotalCommittedMemoryOfCurrentProcess; }
    unsigned peakTotalCommittedMemoryOfChromeProcess() const { return m_peakTotalCommittedMemoryOfChromeProcess; }
    unsigned peakTotalMappedMemory() const { return m_peakTotalMappedMemory; }

private:
    MemoryTracker();
    Timer<MemoryTracker> m_memoryTrackingTimer;
    unsigned m_peakTotalUsedMemory;
    unsigned m_peakTotalCommittedMemoryOfCurrentProcess;
    unsigned m_peakTotalCommittedMemoryOfChromeProcess;
    unsigned m_peakTotalMappedMemory;
};

MemoryTracker::MemoryTracker()
    : m_memoryTrackingTimer(this, &MemoryTracker::updateMemoryPeaks)
    , m_peakTotalUsedMemory(0)
    , m_peakTotalCommittedMemoryOfCurrentProcess(0)
    , m_peakTotalCommittedMemoryOfChromeProcess(0)
    , m_peakTotalMappedMemory(0)
{
}

MemoryTracker& MemoryTracker::instance()
{
    DEFINE_STATIC_LOCAL(MemoryTracker, s_memoryTracker, ());
    return s_memoryTracker;
}

void MemoryTracker::start()
{
    clear();
    if (!m_memoryTrackingTimer.isActive())
        m_memoryTrackingTimer.start(0, 0.01);
}

void MemoryTracker::stop()
{
    m_memoryTrackingTimer.stop();
}

void MemoryTracker::updateMemoryPeaks(Timer<MemoryTracker>*)
{
    // JS engine memory usage.
    JSC::GlobalMemoryStatistics jscMemoryStat = JSC::globalMemoryStatistics();
    JSC::Heap& mainHeap = JSDOMWindow::commonVM()->heap;

    // Malloc info.
    struct malloc_stats mallocInfo = mallocStats();

    // Malloc and JSC memory.
    unsigned totalUsedMemory = static_cast<unsigned>(mallocInfo.m_small_allocmem + mallocInfo.m_allocmem + jscMemoryStat.stackBytes + jscMemoryStat.JITBytes + mainHeap.capacity());
    if (totalUsedMemory > m_peakTotalUsedMemory)
        m_peakTotalUsedMemory = totalUsedMemory;

    unsigned totalCommittedMemoryOfCurrentProcess = BlackBerry::Platform::totalCommittedMemoryOfCurrentProcess();
    if (totalCommittedMemoryOfCurrentProcess > m_peakTotalCommittedMemoryOfCurrentProcess)
        m_peakTotalCommittedMemoryOfCurrentProcess = totalCommittedMemoryOfCurrentProcess;

    unsigned totalCommittedMemoryOfChromeProcess = BlackBerry::Platform::totalCommittedMemoryOfChromeProcess();
    if (totalCommittedMemoryOfChromeProcess > m_peakTotalCommittedMemoryOfChromeProcess)
        m_peakTotalCommittedMemoryOfChromeProcess = totalCommittedMemoryOfChromeProcess;

    struct stat processInfo;
    if (!stat(String::format("/proc/%u/as", getpid()).latin1().data(), &processInfo)) {
        unsigned totalMappedMemory = static_cast<unsigned>(processInfo.st_size);
        if (totalMappedMemory > m_peakTotalMappedMemory)
            m_peakTotalMappedMemory = totalMappedMemory;
    }
}

static String memoryPeaksToHtmlTable(MemoryTracker& memoryTracker)
{
    String htmlTable = String("<table class='fixed-table'><col width=75%><col width=25%>")
        + numberToHTMLTr("Total used memory(malloc + JSC):", memoryTracker.peakTotalUsedMemory());

    if (unsigned peakTotalCommittedMemoryOfChromeProcess = memoryTracker.peakTotalCommittedMemoryOfChromeProcess()) {
        htmlTable += numberToHTMLTr("Total committed memory of tab process:", memoryTracker.peakTotalCommittedMemoryOfCurrentProcess());
        htmlTable += numberToHTMLTr("Total committed memory of chrome process:", peakTotalCommittedMemoryOfChromeProcess);
    } else
        htmlTable += numberToHTMLTr("Total committed memory:", memoryTracker.peakTotalCommittedMemoryOfCurrentProcess());

    htmlTable += numberToHTMLTr("Total mapped memory:", memoryTracker.peakTotalMappedMemory()) + "</table>";
    return htmlTable;
}

static String memoryLivePage(String memoryLiveCommand)
{
    StringBuilder builder;

    builder.append(writeHeader("Memory Live Page"));
    builder.appendLiteral("<div class='box'><div class='box-title'>Memory Peaks</div>");
    builder.appendLiteral("<div style='font-size:12px;color:#1BE0C9'>\"about:memory-live/start\": start tracking memory peaks.</div>");
    builder.appendLiteral("<div style='font-size:12px;color:#1BE0C9'>\"about:memory-live\": show memory peaks every 30ms.</div>");
    builder.appendLiteral("<div style='font-size:12px;color:#1BE0C9'>\"about:memory-live/stop\": stop tracking and show memory peaks.</div><br>");

    MemoryTracker& memoryTracker = MemoryTracker::instance();
    if (memoryLiveCommand.isEmpty()) {
        if (!memoryTracker.isActive())
            builder.appendLiteral("<div style='font-size:15px;color:#E6F032'>Memory tracker isn't running, please use \"about:memory-live/start\" to start the tracker.</div>");
        else {
            builder.append(memoryPeaksToHtmlTable(memoryTracker));
            builder.appendLiteral("<script type=\"text/javascript\">setInterval(function(){window.location.reload()},30);</script>");
        }
    } else if (equalIgnoringCase(memoryLiveCommand, "/start")) {
        memoryTracker.start();
        builder.appendLiteral("<div style='font-size:15px;color:#E6F032'>Memory tracker is running.</div>");
    } else if (equalIgnoringCase(memoryLiveCommand, "/stop")) {
        if (!memoryTracker.isActive())
            builder.appendLiteral("<div style='font-size:15px;color:#E6F032'>Memory tracker isn't running.</div>");
        else {
            memoryTracker.stop();
            builder.append(memoryPeaksToHtmlTable(memoryTracker));
            builder.appendLiteral("<div style='font-size:15px;color:#E6F032'>Memory tracker is stopped.</div>");
        }
    } else
        builder.appendLiteral("<div style='font-size:15spx;color:#E6F032'>Unknown command! Please input a correct command!</div>");

    builder.appendLiteral("</div><br></div></body></html>");
    return builder.toString();
}
#endif

static String cachePage(String cacheCommand)
{
    String result;

    result.append(String("<html><head><title>BlackBerry Browser Disk Cache</title></head><body>"));

    if (cacheCommand.isEmpty())
        result.append(String(BlackBerry::Platform::generateHtmlFragmentForCacheKeys().data()));
    else if (cacheCommand.startsWith("?query=", false)) {
        String key(cacheCommand.substring(7).utf8().data()); // 7 is length of "query=".
        result.append(key);
        result.append(String("<hr>"));
        result.append(String(BlackBerry::Platform::generateHtmlFragmentForCacheHeaders(key).data()));
    } else {
        // Unknown cache command.
        return String();
    }

    result.append(String("</body></html>"));

    return result;
}

static String buildPage()
{
    String result;

    result.append(writeHeader("Build"));
    result.append(String("<div class='box'><div class='box-title'>Basic</div><table>"));
    result.append(String("<tr><td>Built On:  </td><td>"));
    result.append(String(BlackBerry::Platform::BUILDCOMPUTER));
    result.append(String("</td></tr>"));
    result.append(String("<tr><td>Build User:  </td><td>"));
    result.append(String(BlackBerry::Platform::BUILDUSER));
    result.append(String("</td></tr>"));
    result.append(String("<tr><td>Build Time:  </td><td>"));
    result.append(String(BlackBerry::Platform::BUILDTIME));
    result.append(String("</table></div><br>"));
    result.append(String(BlackBerry::Platform::BUILDINFO_WEBKIT));
    result.append(String(BlackBerry::Platform::BUILDINFO_PLATFORM));
    result.append(String(BlackBerry::Platform::BUILDINFO_LIBWEBVIEW));
    result.append(String(BlackBerry::Platform::BUILDINFO_WEBPLATFORM));
    result.append(String("</body></html>"));

    return result;
}

static String cookiePage()
{
    String result;

    result.append(String("<html><head><title>BlackBerry Browser cookie information</title></head><body>"));
    result.append(cookieManager().generateHtmlFragmentForCookies());
    result.append(String("</body></html>"));

    return result;
}

String aboutData(String aboutWhat)
{
    if (aboutWhat.startsWith("cache"))
        return cachePage(aboutWhat.substring(5));

    if (equalIgnoringCase(aboutWhat, "memory"))
        return memoryPage();

#if !defined(PUBLIC_BUILD) || !PUBLIC_BUILD
    if (equalIgnoringCase(aboutWhat, "cookie"))
        return cookiePage();

    if (BlackBerry::Platform::debugSetting() > 0 && equalIgnoringCase(aboutWhat, "build"))
        return buildPage();

    if (BlackBerry::Platform::debugSetting() > 0 && equalIgnoringCase(aboutWhat, "config"))
        return configPage();

    if (aboutWhat.startsWith("memory-live"))
        return memoryLivePage(aboutWhat.substring(11));
#endif

    return String();
}

} // namespace WebKit
} // namespace BlackBerry
