# Copyright (C) 2012 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# A module to contain all the enable/disable feature option code.

use strict;
use warnings;

use FindBin;
use lib $FindBin::Bin;
use webkitdirs;

BEGIN {
   use Exporter   ();
   our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
   $VERSION     = 1.00;
   @ISA         = qw(Exporter);
   @EXPORT      = qw(&getFeatureOptionList);
   %EXPORT_TAGS = ( );
   @EXPORT_OK   = ();
}

my (
    $threeDRenderingSupport,
    $accelerated2DCanvasSupport,
    $batteryStatusSupport,
    $blobSupport,
    $canvasPathSupport,
    $canvasProxySupport,
    $channelMessagingSupport,
    $cspNextSupport,
    $css3ConditionalRulesSupport,
    $css3TextSupport,
    $css3TextLineBreakSupport,
    $css4ImagesSupport,
    $cssBoxDecorationBreakSupport,
    $cssDeviceAdaptation,
    $cssExclusionsSupport,
    $cssFiltersSupport,
    $cssImageOrientationSupport,
    $cssImageResolutionSupport,
    $cssImageSetSupport,
    $cssRegionsSupport,
    $cssShadersSupport,
    $cssShapesSupport,
    $cssStickyPositionSupport,
    $cssCompositingSupport,
    $cssAnimationsTransformsUnprefixedSupport,
    $cssVariablesSupport,
    $customSchemeHandlerSupport,
    $dataTransferItemsSupport,
    $datalistElementSupport,
    $detailsElementSupport,
    $deviceOrientationSupport,
    $dialogElementSupport,
    $directoryUploadSupport,
    $dom4EventsConstructor,
    $downloadAttributeSupport,
    $fileSystemSupport,
    $filtersSupport,
    $fontLoadEventsSupport,
    $ftpDirSupport,
    $fullscreenAPISupport,
    $gamepadSupport,
    $geolocationSupport,
    $highDPICanvasSupport,
    $icondatabaseSupport,
    $iframeSeamlessSupport,
    $indexedDatabaseSupport,
    $inputSpeechSupport,
    $inputTypeColorSupport,
    $inputTypeDateSupport,
    $inputTypeDatetimeSupport,
    $inputTypeDatetimelocalSupport,
    $inputTypeMonthSupport,
    $inputTypeTimeSupport,
    $inputTypeWeekSupport,
    $inspectorSupport,
    $javascriptDebuggerSupport,
    $legacyNotificationsSupport,
    $legacyVendorPrefixSupport,
    $legacyWebAudioSupport,
    $linkPrefetchSupport,
    $linkPrerenderSupport,
    $mathmlSupport,
    $mediaCaptureSupport,
    $mediaSourceSupport,
    $mediaStatisticsSupport,
    $mediaStreamSupport,
    $meterElementSupport,
    $mhtmlSupport,
    $microdataSupport,
    $mouseCursorScaleSupport,
    $netscapePluginAPISupport,
    $networkInfoSupport,
    $nosniffSupport,
    $notificationsSupport,
    $orientationEventsSupport,
    $pageVisibilityAPISupport,
    $performanceTimelineSupport,
    $progressElementSupport,
    $proximityEventsSupport,
    $quotaSupport,
    $resolutionMediaQuerySupport,
    $registerProtocolHandlerSupport,
    $requestAnimationFrameSupport,
    $resourceTimingSupport,
    $scriptedSpeechSupport,
    $seccompFiltersSupport,
    $shadowDOMSupport,
    $sharedWorkersSupport,
    $sqlDatabaseSupport,
    $styleScopedSupport,
    $suidLinuxSandbox,
    $svgDOMObjCBindingsSupport,
    $svgFontsSupport,
    $svgSupport,
    $systemMallocSupport,
    $templateElementSupport,
    $textAutosizingSupport,
    $tiledBackingStoreSupport,
    $threadedHTMLParserSupport,
    $touchEventsSupport,
    $touchSliderSupport,
    $touchIconLoadingSupport,
    $userTimingSupport,
    $vibrationSupport,
    $videoSupport,
    $videoTrackSupport,
    $webglSupport,
    $webAudioSupport,
    $webSocketsSupport,
    $webTimingSupport,
    $workersSupport,
    $xhrTimeoutSupport,
    $xsltSupport,
);

my @features = (
    { option => "3d-rendering", desc => "Toggle 3D Rendering support",
      define => "ENABLE_3D_RENDERING", default => (isAppleMacWebKit() || isGtk() || isEfl()), value => \$threeDRenderingSupport },

    { option => "accelerated-2d-canvas", desc => "Toggle Accelerated 2D Canvas support",
      define => "ENABLE_ACCELERATED_2D_CANVAS", default => 0, value => \$accelerated2DCanvasSupport },

    { option => "battery-status", desc => "Toggle Battery Status support",
      define => "ENABLE_BATTERY_STATUS", default => (isEfl() || isBlackBerry()), value => \$batteryStatusSupport },

    { option => "blob", desc => "Toggle Blob support",
      define => "ENABLE_BLOB", default => (isAppleMacWebKit() || isGtk() || isBlackBerry() || isEfl()), value => \$blobSupport },

    { option => "canvas-path", desc => "Toggle Canvas Path support",
      define => "ENABLE_CANVAS_PATH", default => 1, value => \$canvasPathSupport },

    { option => "canvas-proxy", desc => "Toggle CanvasProxy support",
      define => "ENABLE_CANVAS_PROXY", default => 0, value => \$canvasProxySupport },

    { option => "channel-messaging", desc => "Toggle Channel Messaging support",
      define => "ENABLE_CHANNEL_MESSAGING", default => 1, value => \$channelMessagingSupport },

    { option => "csp-next", desc => "Toggle Content Security Policy 1.1 support",
      define => "ENABLE_CSP_NEXT", default => isGtk(), value => \$cspNextSupport },

    { option => "css-device-adaptation", desc => "Toggle CSS Device Adaptation support",
      define => "ENABLE_CSS_DEVICE_ADAPTATION", default => isEfl(), value => \$cssDeviceAdaptation },

    { option => "css-exclusions", desc => "Toggle CSS Exclusions support",
      define => "ENABLE_CSS_EXCLUSIONS", default => 1, value => \$cssExclusionsSupport },

    { option => "css-shapes", desc => "Toggle CSS Shapes support",
      define => "ENABLE_CSS_SHAPES", default => 1, value => \$cssShapesSupport },

    { option => "css-filters", desc => "Toggle CSS Filters support",
      define => "ENABLE_CSS_FILTERS", default => isAppleWebKit() || isBlackBerry(), value => \$cssFiltersSupport },

    { option => "css3-conditional-rules", desc => "Toggle CSS3 Conditional Rules support (i.e. \@supports)",
      define => "ENABLE_CSS3_CONDITIONAL_RULES", default => 0, value => \$css3ConditionalRulesSupport },

    { option => "css3-text", desc => "Toggle CSS3 Text support",
      define => "ENABLE_CSS3_TEXT", default => (isEfl() || isGtk()), value => \$css3TextSupport },

    { option => "css3-text-line-break", desc => "Toggle CSS3 Text Line Break support",
      define => "ENABLE_CSS3_TEXT_LINE_BREAK", default => 0, value => \$css3TextLineBreakSupport },

    { option => "css-box-decoration-break", desc => "Toggle CSS box-decoration-break support",
      define => "ENABLE_CSS_BOX_DECORATION_BREAK", default => 1, value => \$cssBoxDecorationBreakSupport },

    { option => "css-image-orientation", desc => "Toggle CSS image-orientation support",
      define => "ENABLE_CSS_IMAGE_ORIENTATION", default => isGtk(), value => \$cssImageOrientationSupport },

    { option => "css-image-resolution", desc => "Toggle CSS image-resolution support",
      define => "ENABLE_CSS_IMAGE_RESOLUTION", default => (isBlackBerry() || isGtk()), value => \$cssImageResolutionSupport },

    { option => "css-image-set", desc => "Toggle CSS image-set support",
      define => "ENABLE_CSS_IMAGE_SET", default => (isEfl() || isGtk()), value => \$cssImageSetSupport },

    { option => "css-regions", desc => "Toggle CSS Regions support",
      define => "ENABLE_CSS_REGIONS", default => 1, value => \$cssRegionsSupport },

    { option => "css-shaders", desc => "Toggle CSS Shaders support",
      define => "ENABLE_CSS_SHADERS", default => isAppleMacWebKit(), value => \$cssShadersSupport },

    { option => "css-sticky-position", desc => "Toggle CSS sticky position support",
      define => "ENABLE_CSS_STICKY_POSITION", default => (isGtk() || isEfl()), value => \$cssStickyPositionSupport },

    { option => "css-compositing", desc => "Toggle CSS Compositing support",
      define => "ENABLE_CSS_COMPOSITING", default => isAppleWebKit(), value => \$cssCompositingSupport },

    { option => "css-transforms-animations-unprefixed", desc => "Toggle support for unprefixed CSS animations and transforms",
      define => "ENABLE_CSS_TRANSFORMS_ANIMATIONS_UNPREFIXED", default => 1, value => \$cssAnimationsTransformsUnprefixedSupport },

    { option => "css-variables", desc => "Toggle CSS Variable support",
      define => "ENABLE_CSS_VARIABLES", default => (isBlackBerry() || isEfl() || isGtk()), value => \$cssVariablesSupport },

    { option => "custom-scheme-handler", desc => "Toggle Custom Scheme Handler support",
      define => "ENABLE_CUSTOM_SCHEME_HANDLER", default => (isBlackBerry() || isEfl()), value => \$customSchemeHandlerSupport },

    { option => "datalist-element", desc => "Toggle Datalist Element support",
      define => "ENABLE_DATALIST_ELEMENT", default => isEfl(), value => \$datalistElementSupport },

    { option => "data-transfer-items", desc => "Toggle Data Transfer Items support",
      define => "ENABLE_DATA_TRANSFER_ITEMS", default => 0, value => \$dataTransferItemsSupport },

    { option => "details-element", desc => "Toggle Details Element support",
      define => "ENABLE_DETAILS_ELEMENT", default => 1, value => \$detailsElementSupport },

    { option => "device-orientation", desc => "Toggle Device Orientation support",
      define => "ENABLE_DEVICE_ORIENTATION", default => isBlackBerry(), value => \$deviceOrientationSupport },

    { option => "dialog-element", desc => "Toggle Dialog Element support",
      define => "ENABLE_DIALOG_ELEMENT", default => 0, value => \$dialogElementSupport },

    { option => "directory-upload", desc => "Toggle Directory Upload support",
      define => "ENABLE_DIRECTORY_UPLOAD", default => 0, value => \$directoryUploadSupport },

    { option => "dom4-events-constructor", desc => "Expose DOM4 Events constructors",
      define => "ENABLE_DOM4_EVENTS_CONSTRUCTOR", default => (isAppleWebKit() || isGtk() || isEfl()), value => \$dom4EventsConstructor },

    { option => "download-attribute", desc => "Toggle Download Attribute support",
      define => "ENABLE_DOWNLOAD_ATTRIBUTE", default => (isBlackBerry() || isEfl()), value => \$downloadAttributeSupport },

    { option => "file-system", desc => "Toggle File System support",
      define => "ENABLE_FILE_SYSTEM", default => isBlackBerry(), value => \$fileSystemSupport },

    { option => "filters", desc => "Toggle Filters support",
      define => "ENABLE_FILTERS", default => (isAppleWebKit() || isGtk() || isEfl() || isBlackBerry()), value => \$filtersSupport },

    { option => "font-load-events", desc => "Toggle Font Load Events support",
      define => "ENABLE_FONT_LOAD_EVENTS", default => 0, value => \$fontLoadEventsSupport },

    { option => "ftpdir", desc => "Toggle FTP Directory support",
      define => "ENABLE_FTPDIR", default => !isWinCE(), value => \$ftpDirSupport },

    { option => "fullscreen-api", desc => "Toggle Fullscreen API support",
      define => "ENABLE_FULLSCREEN_API", default => (isAppleMacWebKit() || isEfl() || isGtk() || isBlackBerry()), value => \$fullscreenAPISupport },

    { option => "gamepad", desc => "Toggle Gamepad support",
      define => "ENABLE_GAMEPAD", default => (isEfl() || isGtk()), value => \$gamepadSupport },

    { option => "geolocation", desc => "Toggle Geolocation support",
      define => "ENABLE_GEOLOCATION", default => (isAppleWebKit() || isGtk() || isBlackBerry()), value => \$geolocationSupport },

    { option => "high-dpi-canvas", desc => "Toggle High DPI Canvas support",
      define => "ENABLE_HIGH_DPI_CANVAS", default => (isAppleWebKit()), value => \$highDPICanvasSupport },

    { option => "icon-database", desc => "Toggle Icondatabase support",
      define => "ENABLE_ICONDATABASE", default => 1, value => \$icondatabaseSupport },

    { option => "iframe-seamless", desc => "Toggle iframe seamless attribute support",
      define => "ENABLE_IFRAME_SEAMLESS", default => 1, value => \$iframeSeamlessSupport },

    { option => "indexed-database", desc => "Toggle Indexed Database support",
      define => "ENABLE_INDEXED_DATABASE", default => (isGtk() || isEfl()), value => \$indexedDatabaseSupport },

    { option => "input-speech", desc => "Toggle Input Speech support",
      define => "ENABLE_INPUT_SPEECH", default => 0, value => \$inputSpeechSupport },

    { option => "input-type-color", desc => "Toggle Input Type Color support",
      define => "ENABLE_INPUT_TYPE_COLOR", default => (isBlackBerry() || isEfl()), value => \$inputTypeColorSupport },

    { option => "input-type-date", desc => "Toggle Input Type Date support",
      define => "ENABLE_INPUT_TYPE_DATE", default => 0, value => \$inputTypeDateSupport },

    { option => "input-type-datetime", desc => "Toggle broken Input Type Datetime support",
      define => "ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE", default => 0, value => \$inputTypeDatetimeSupport },

    { option => "input-type-datetimelocal", desc => "Toggle Input Type Datetimelocal support",
      define => "ENABLE_INPUT_TYPE_DATETIMELOCAL", default => 0, value => \$inputTypeDatetimelocalSupport },

    { option => "input-type-month", desc => "Toggle Input Type Month support",
      define => "ENABLE_INPUT_TYPE_MONTH", default => 0, value => \$inputTypeMonthSupport },

    { option => "input-type-time", desc => "Toggle Input Type Time support",
      define => "ENABLE_INPUT_TYPE_TIME", default => 0, value => \$inputTypeTimeSupport },

    { option => "input-type-week", desc => "Toggle Input Type Week support",
      define => "ENABLE_INPUT_TYPE_WEEK", default => 0, value => \$inputTypeWeekSupport },

    { option => "inspector", desc => "Toggle Inspector support",
      define => "ENABLE_INSPECTOR", default => !isWinCE(), value => \$inspectorSupport },

    { option => "javascript-debugger", desc => "Toggle JavaScript Debugger support",
      define => "ENABLE_JAVASCRIPT_DEBUGGER", default => 1, value => \$javascriptDebuggerSupport },

    { option => "legacy-notifications", desc => "Toggle Legacy Notifications support",
      define => "ENABLE_LEGACY_NOTIFICATIONS", default => isBlackBerry(), value => \$legacyNotificationsSupport },

    { option => "legacy-vendor-prefixes", desc => "Toggle Legacy Vendor Prefix support",
      define => "ENABLE_LEGACY_VENDOR_PREFIXES", default => 1, value => \$legacyVendorPrefixSupport },

    { option => "legacy-web-audio", desc => "Toggle Legacy Web Audio support",
      define => "ENABLE_LEGACY_WEB_AUDIO", default => 1, value => \$legacyWebAudioSupport },

    { option => "link-prefetch", desc => "Toggle Link Prefetch support",
      define => "ENABLE_LINK_PREFETCH", default => (isGtk() || isEfl()), value => \$linkPrefetchSupport },

    { option => "mathml", desc => "Toggle MathML support",
      define => "ENABLE_MATHML", default => !isBlackBerry(), value => \$mathmlSupport },

    { option => "media-capture", desc => "Toggle Media Capture support",
      define => "ENABLE_MEDIA_CAPTURE", default => isEfl(), value => \$mediaCaptureSupport },

    { option => "media-source", desc => "Toggle Media Source support",
      define => "ENABLE_MEDIA_SOURCE", default => 0, value => \$mediaSourceSupport },

    { option => "media-statistics", desc => "Toggle Media Statistics support",
      define => "ENABLE_MEDIA_STATISTICS", default => 0, value => \$mediaStatisticsSupport },

    { option => "media-stream", desc => "Toggle Media Stream support",
      define => "ENABLE_MEDIA_STREAM", default => isBlackBerry(), value => \$mediaStreamSupport },

    { option => "meter-element", desc => "Toggle Meter Element support",
      define => "ENABLE_METER_ELEMENT", default => !isAppleWinWebKit(), value => \$meterElementSupport },

    { option => "mhtml", desc => "Toggle MHTML support",
      define => "ENABLE_MHTML", default => (isGtk() || isEfl()), value => \$mhtmlSupport },

    { option => "microdata", desc => "Toggle Microdata support",
      define => "ENABLE_MICRODATA", default => (isEfl() || isBlackBerry() || isGtk()), value => \$microdataSupport },

    { option => "mouse-cursor-scale", desc => "Toggle Scaled mouse cursor support",
      define => "ENABLE_MOUSE_CURSOR_SCALE", default => isEfl(), value => \$mouseCursorScaleSupport },

    { option => "navigator-content-utils", desc => "Toggle Navigator Content Utils support",
      define => "ENABLE_NAVIGATOR_CONTENT_UTILS", default => (isBlackBerry() || isEfl()), value => \$registerProtocolHandlerSupport },

    { option => "netscape-plugin-api", desc => "Toggle Netscape Plugin API support",
      define => "ENABLE_NETSCAPE_PLUGIN_API", default => 1, value => \$netscapePluginAPISupport },

    { option => "network-info", desc => "Toggle Network Info support",
      define => "ENABLE_NETWORK_INFO", default => (isEfl() || isBlackBerry()), value => \$networkInfoSupport },

    { option => "nosniff", desc => "Toggle support for 'X-Content-Type-Options: nosniff'",
      define => "ENABLE_NOSNIFF", default => 0, value => \$nosniffSupport },

    { option => "notifications", desc => "Toggle Notifications support",
      define => "ENABLE_NOTIFICATIONS", default => isBlackBerry(), value => \$notificationsSupport },

    { option => "orientation-events", desc => "Toggle Orientation Events support",
      define => "ENABLE_ORIENTATION_EVENTS", default => isBlackBerry(), value => \$orientationEventsSupport },

    { option => "page-visibility-api", desc => "Toggle Page Visibility API support",
      define => "ENABLE_PAGE_VISIBILITY_API", default => (isBlackBerry() || isEfl() || isGtk()), value => \$pageVisibilityAPISupport },

    { option => "performance-timeline", desc => "Toggle Performance Timeline support",
      define => "ENABLE_PERFORMANCE_TIMELINE", default => isGtk(), value => \$performanceTimelineSupport },

    { option => "progress-element", desc => "Toggle Progress Element support",
      define => "ENABLE_PROGRESS_ELEMENT", default => 1, value => \$progressElementSupport },

    { option => "proximity-events", desc => "Toggle Proximity Events support",
      define => "ENABLE_PROXIMITY_EVENTS", default => 0, value => \$proximityEventsSupport },

    { option => "quota", desc => "Toggle Quota support",
      define => "ENABLE_QUOTA", default => 0, value => \$quotaSupport },

    { option => "resolution-media-query", desc => "Toggle resolution media query support",
      define => "ENABLE_RESOLUTION_MEDIA_QUERY", default => isEfl(), value => \$resolutionMediaQuerySupport },

    { option => "resource-timing", desc => "Toggle Resource Timing support",
      define => "ENABLE_RESOURCE_TIMING", default => isGtk(), value => \$resourceTimingSupport },

    { option => "request-animation-frame", desc => "Toggle Request Animation Frame support",
      define => "ENABLE_REQUEST_ANIMATION_FRAME", default => (isAppleMacWebKit() || isGtk() || isEfl() || isBlackBerry()), value => \$requestAnimationFrameSupport },

    { option => "seccomp-filters", desc => "Toggle Seccomp Filter sandbox",
      define => "ENABLE_SECCOMP_FILTERS", default => 0, value => \$seccompFiltersSupport },

    { option => "scripted-speech", desc => "Toggle Scripted Speech support",
      define => "ENABLE_SCRIPTED_SPEECH", default => 0, value => \$scriptedSpeechSupport },

    { option => "shadow-dom", desc => "Toggle Shadow DOM support",
      define => "ENABLE_SHADOW_DOM", default => 0, value => \$shadowDOMSupport },

    { option => "shared-workers", desc => "Toggle Shared Workers support",
      define => "ENABLE_SHARED_WORKERS", default => (isAppleWebKit() || isGtk() || isBlackBerry() || isEfl()), value => \$sharedWorkersSupport },

    { option => "sql-database", desc => "Toggle SQL Database support",
      define => "ENABLE_SQL_DATABASE", default => 1, value => \$sqlDatabaseSupport },

    { option => "style-scoped", desc => "Toggle Style Scoped support",
      define => "ENABLE_STYLE_SCOPED", default => (isBlackBerry() || isGtk()), value => \$styleScopedSupport },

    { option => "suid-linux-sandbox", desc => "Toggle suid sandbox for linux",
      define => "ENABLE_SUID_SANDBOX_LINUX", default => 0, value => \$suidLinuxSandbox },

    { option => "svg", desc => "Toggle SVG support",
      define => "ENABLE_SVG", default => 1, value => \$svgSupport },

    { option => "svg-fonts", desc => "Toggle SVG Fonts support",
      define => "ENABLE_SVG_FONTS", default => 1, value => \$svgFontsSupport },

    { option => "system-malloc", desc => "Toggle system allocator instead of TCmalloc",
      define => "USE_SYSTEM_MALLOC", default => (isBlackBerry() || isWinCE()), value => \$systemMallocSupport },

    { option => "template-element", desc => "Toggle HTMLTemplateElement support",
      define => "ENABLE_TEMPLATE_ELEMENT", default => (isEfl() || isGtk()), value => \$templateElementSupport },

    { option => "text-autosizing", desc => "Toggle Text Autosizing support",
      define => "ENABLE_TEXT_AUTOSIZING", default => isBlackBerry(), value => \$textAutosizingSupport },

    { option => "tiled-backing-store", desc => "Toggle Tiled Backing Store support",
      define => "WTF_USE_TILED_BACKING_STORE", default => isEfl(), value => \$tiledBackingStoreSupport },

    { option => "threaded-html-parser", desc => "Toggle threaded HTML parser support",
      define => "ENABLE_THREADED_HTML_PARSER", default => 0, value => \$threadedHTMLParserSupport },

    { option => "touch-events", desc => "Toggle Touch Events support",
      define => "ENABLE_TOUCH_EVENTS", default => (isBlackBerry() || isEfl()), value => \$touchEventsSupport },

    { option => "touch-slider", desc => "Toggle Touch Slider support",
      define => "ENABLE_TOUCH_SLIDER", default => isBlackBerry(), value => \$touchSliderSupport },

    { option => "touch-icon-loading", desc => "Toggle Touch Icon Loading Support",
      define => "ENABLE_TOUCH_ICON_LOADING", default => 0, value => \$touchIconLoadingSupport },

    { option => "user-timing", desc => "Toggle User Timing support",
      define => "ENABLE_USER_TIMING", default => isGtk(), value => \$userTimingSupport },

    { option => "vibration", desc => "Toggle Vibration support",
      define => "ENABLE_VIBRATION", default => (isEfl() || isBlackBerry()), value => \$vibrationSupport },

    { option => "video", desc => "Toggle Video support",
      define => "ENABLE_VIDEO", default => (isAppleWebKit() || isGtk() || isBlackBerry() || isEfl()), value => \$videoSupport },

    { option => "video-track", desc => "Toggle Video Track support",
      define => "ENABLE_VIDEO_TRACK", default => (isAppleWebKit() || isGtk() || isEfl() || isBlackBerry()), value => \$videoTrackSupport },

    { option => "webgl", desc => "Toggle WebGL support",
      define => "ENABLE_WEBGL", default => (isAppleMacWebKit() || isGtk() || isEfl()), value => \$webglSupport },

    { option => "web-audio", desc => "Toggle Web Audio support",
      define => "ENABLE_WEB_AUDIO", default => (isEfl() || isGtk()), value => \$webAudioSupport },

    { option => "web-sockets", desc => "Toggle Web Sockets support",
      define => "ENABLE_WEB_SOCKETS", default => 1, value => \$webSocketsSupport },

    { option => "web-timing", desc => "Toggle Web Timing support",
      define => "ENABLE_WEB_TIMING", default => (isBlackBerry() || isGtk() || isEfl()), value => \$webTimingSupport },

    { option => "workers", desc => "Toggle Workers support",
      define => "ENABLE_WORKERS", default => (isAppleWebKit() || isGtk() || isBlackBerry() || isEfl()), value => \$workersSupport },

    { option => "xhr-timeout", desc => "Toggle XHR Timeout support",
      define => "ENABLE_XHR_TIMEOUT", default => (isEfl() || isGtk() || isAppleMacWebKit()), value => \$xhrTimeoutSupport },

    { option => "xslt", desc => "Toggle XSLT support",
      define => "ENABLE_XSLT", default => 1, value => \$xsltSupport },
);

sub getFeatureOptionList()
{
    return @features;
}

1;
