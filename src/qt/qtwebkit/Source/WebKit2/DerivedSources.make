# Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(WebKit2) \
    $(WebKit2)/NetworkProcess \
    $(WebKit2)/NetworkProcess/mac \
    $(WebKit2)/PluginProcess \
    $(WebKit2)/PluginProcess/mac \
    $(WebKit2)/Shared/Plugins \
    $(WebKit2)/Shared \
    $(WebKit2)/Shared/mac \
    $(WebKit2)/Shared/Authentication \
    $(WebKit2)/Shared/Network/CustomProtocols \
    $(WebKit2)/SharedWorkerProcess \
    $(WebKit2)/OfflineStorageProcess \
    $(WebKit2)/WebProcess/ApplicationCache \
    $(WebKit2)/WebProcess/Cookies \
    $(WebKit2)/WebProcess/FullScreen \
    $(WebKit2)/WebProcess/Geolocation \
    $(WebKit2)/WebProcess/IconDatabase \
    $(WebKit2)/WebProcess/MediaCache \
    $(WebKit2)/WebProcess/Network \
    $(WebKit2)/WebProcess/Notifications \
    $(WebKit2)/WebProcess/Plugins \
    $(WebKit2)/WebProcess/ResourceCache \
    $(WebKit2)/WebProcess/Storage \
    $(WebKit2)/WebProcess/WebCoreSupport \
    $(WebKit2)/WebProcess/WebPage \
    $(WebKit2)/WebProcess \
    $(WebKit2)/UIProcess \
    $(WebKit2)/UIProcess/Downloads \
    $(WebKit2)/UIProcess/Network \
    $(WebKit2)/UIProcess/Network/CustomProtocols \
    $(WebKit2)/UIProcess/Notifications \
    $(WebKit2)/UIProcess/Plugins \
    $(WebKit2)/UIProcess/SharedWorkers \
    $(WebKit2)/UIProcess/Storage \
    $(WebKit2)/UIProcess/mac \
#

MESSAGE_RECEIVERS = \
    AuthenticationManager \
    CustomProtocolManager \
    CustomProtocolManagerProxy \
    DrawingArea \
    DrawingAreaProxy \
    DownloadProxy \
    EventDispatcher \
    NetworkProcess \
    NetworkProcessConnection \
    NetworkProcessProxy \
    NetworkResourceLoader \
    NPObjectMessageReceiver \
    OfflineStorageProcess \
    PluginControllerProxy \
    PluginProcess \
    PluginProcessConnection \
    PluginProcessConnectionManager \
    PluginProcessProxy \
    PluginProxy \
    SharedWorkerProcess \
    SharedWorkerProcessProxy \
    StorageManager \
    WebApplicationCacheManager \
    WebApplicationCacheManagerProxy \
    WebCookieManager \
    WebCookieManagerProxy \
    WebConnection \
    NetworkConnectionToWebProcess \
    RemoteLayerTreeHost \
    SecItemShim \
    SecItemShimProxy \
    StorageAreaMap \
    WebContext \
    WebDatabaseManager \
    WebDatabaseManagerProxy \
    WebFullScreenManager \
    WebFullScreenManagerProxy \
    WebGeolocationManager \
    WebGeolocationManagerProxy \
    WebIconDatabase \
    WebIconDatabaseProxy \
    WebInspector \
    WebInspectorProxy \
    WebMediaCacheManager \
    WebMediaCacheManagerProxy \
    WebNotificationManager \
    WebPage \
    WebPageGroupProxy \
    WebPageProxy \
    WebProcess \
    WebProcessConnection \
    WebProcessProxy \
    WebResourceCacheManager \
    WebResourceCacheManagerProxy \
    WebResourceLoader \
#

SCRIPTS = \
    $(WebKit2)/Scripts/generate-message-receiver.py \
    $(WebKit2)/Scripts/generate-messages-header.py \
    $(WebKit2)/Scripts/webkit2/__init__.py \
    $(WebKit2)/Scripts/webkit2/messages.py \
    $(WebKit2)/Scripts/webkit2/model.py \
    $(WebKit2)/Scripts/webkit2/parser.py \
#

.PHONY : all

all : \
    $(MESSAGE_RECEIVERS:%=%MessageReceiver.cpp) \
    $(MESSAGE_RECEIVERS:%=%Messages.h) \
#

%MessageReceiver.cpp : %.messages.in $(SCRIPTS)
	@echo Generating messages header for $*...
	@python $(WebKit2)/Scripts/generate-message-receiver.py $< > $@

%Messages.h : %.messages.in $(SCRIPTS)
	@echo Generating message receiver for $*...
	@python $(WebKit2)/Scripts/generate-messages-header.py $< > $@

# Mac-specific rules

ifeq ($(PLATFORM_NAME),macosx)

FRAMEWORK_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(FRAMEWORK_SEARCH_PATHS) | perl -e 'print "-F " . join(" -F ", split(" ", <>));')
HEADER_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(HEADER_SEARCH_PATHS) | perl -e 'print "-I" . join(" -I", split(" ", <>));')

# Some versions of clang incorrectly strip out // comments in c89 code.
# Use -traditional as a workaround, but only when needed since that causes
# other problems with later versions of clang.
ifeq ($(shell echo '//x' | $(CC) -E -P -x c -std=c89 - | grep x),)
TEXT_PREPROCESSOR_FLAGS=-E -P -x c -traditional -w
else
TEXT_PREPROCESSOR_FLAGS=-E -P -x c -std=c89 -w
endif

ifneq ($(SDKROOT),)
	SDK_FLAGS=-isysroot $(SDKROOT)
endif

SANDBOX_PROFILES = \
	com.apple.WebProcess.sb \
	com.apple.WebKit.NetworkProcess.sb

all: $(SANDBOX_PROFILES)

%.sb : %.sb.in
	@echo Pre-processing $* sandbox profile...
	$(CC) $(SDK_FLAGS) $(TEXT_PREPROCESSOR_FLAGS) $(FRAMEWORK_FLAGS) $(HEADER_FLAGS) -include "wtf/Platform.h" $< > $@

endif # macosx

# ------------------------

# Windows-specific rules

ifeq ($(OS),Windows_NT)

all : HeaderDetection.h

HeaderDetection.h : DerivedSources.make
	if [ -f "$(WEBKITLIBRARIESDIR)/include/WebKitQuartzCoreAdditions/WebKitQuartzCoreAdditionsBase.h" ] && [ ! -f "$(WEBKITLIBRARIESDIR)/include/cairo/cairo.h" ]; then echo "#define HAVE_WKQCA 1" > $@; else echo > $@; fi
	if [ -f "$(WEBKITLIBRARIESDIR)/include/AVFoundationCF/AVCFBase.h" ]; then echo "#define HAVE_AVCF 1" >> $@; else echo >> $@; fi

endif # Windows_NT
