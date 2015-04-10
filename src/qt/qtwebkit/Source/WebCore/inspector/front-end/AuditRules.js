/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.AuditRules.IPAddressRegexp = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;

WebInspector.AuditRules.CacheableResponseCodes =
{
    200: true,
    203: true,
    206: true,
    300: true,
    301: true,
    410: true,

    304: true // Underlying request is cacheable
}

/**
 * @param {!Array.<!WebInspector.NetworkRequest>} requests
 * @param {Array.<!WebInspector.resourceTypes>} types
 * @param {boolean} needFullResources
 * @return {(Object.<string, !Array.<!WebInspector.NetworkRequest>>|Object.<string, !Array.<string>>)}
 */
WebInspector.AuditRules.getDomainToResourcesMap = function(requests, types, needFullResources)
{
    var domainToResourcesMap = {};
    for (var i = 0, size = requests.length; i < size; ++i) {
        var request = requests[i];
        if (types && types.indexOf(request.type) === -1)
            continue;
        var parsedURL = request.url.asParsedURL();
        if (!parsedURL)
            continue;
        var domain = parsedURL.host;
        var domainResources = domainToResourcesMap[domain];
        if (domainResources === undefined) {
          domainResources = [];
          domainToResourcesMap[domain] = domainResources;
        }
        domainResources.push(needFullResources ? request : request.url);
    }
    return domainToResourcesMap;
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.GzipRule = function()
{
    WebInspector.AuditRule.call(this, "network-gzip", "Enable gzip compression");
}

WebInspector.AuditRules.GzipRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var totalSavings = 0;
        var compressedSize = 0;
        var candidateSize = 0;
        var summary = result.addChild("", true);
        for (var i = 0, length = requests.length; i < length; ++i) {
            var request = requests[i];
            if (request.statusCode === 304)
                continue; // Do not test 304 Not Modified requests as their contents are always empty.
            if (this._shouldCompress(request)) {
                var size = request.resourceSize;
                candidateSize += size;
                if (this._isCompressed(request)) {
                    compressedSize += size;
                    continue;
                }
                var savings = 2 * size / 3;
                totalSavings += savings;
                summary.addFormatted("%r could save ~%s", request.url, Number.bytesToString(savings));
                result.violationCount++;
            }
        }
        if (!totalSavings)
            return callback(null);
        summary.value = String.sprintf("Compressing the following resources with gzip could reduce their transfer size by about two thirds (~%s):", Number.bytesToString(totalSavings));
        callback(result);
    },

    _isCompressed: function(request)
    {
        var encodingHeader = request.responseHeaderValue("Content-Encoding");
        if (!encodingHeader)
            return false;

        return /\b(?:gzip|deflate)\b/.test(encodingHeader);
    },

    _shouldCompress: function(request)
    {
        return request.type.isTextType() && request.parsedURL.host && request.resourceSize !== undefined && request.resourceSize > 150;
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.CombineExternalResourcesRule = function(id, name, type, resourceTypeName, allowedPerDomain)
{
    WebInspector.AuditRule.call(this, id, name);
    this._type = type;
    this._resourceTypeName = resourceTypeName;
    this._allowedPerDomain = allowedPerDomain;
}

WebInspector.AuditRules.CombineExternalResourcesRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var domainToResourcesMap = WebInspector.AuditRules.getDomainToResourcesMap(requests, [this._type], false);
        var penalizedResourceCount = 0;
        // TODO: refactor according to the chosen i18n approach
        var summary = result.addChild("", true);
        for (var domain in domainToResourcesMap) {
            var domainResources = domainToResourcesMap[domain];
            var extraResourceCount = domainResources.length - this._allowedPerDomain;
            if (extraResourceCount <= 0)
                continue;
            penalizedResourceCount += extraResourceCount - 1;
            summary.addChild(String.sprintf("%d %s resources served from %s.", domainResources.length, this._resourceTypeName, WebInspector.AuditRuleResult.resourceDomain(domain)));
            result.violationCount += domainResources.length;
        }
        if (!penalizedResourceCount)
            return callback(null);

        summary.value = "There are multiple resources served from same domain. Consider combining them into as few files as possible.";
        callback(result);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CombineExternalResourcesRule}
 */
WebInspector.AuditRules.CombineJsResourcesRule = function(allowedPerDomain) {
    WebInspector.AuditRules.CombineExternalResourcesRule.call(this, "page-externaljs", "Combine external JavaScript", WebInspector.resourceTypes.Script, "JavaScript", allowedPerDomain);
}

WebInspector.AuditRules.CombineJsResourcesRule.prototype = {
    __proto__: WebInspector.AuditRules.CombineExternalResourcesRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CombineExternalResourcesRule}
 */
WebInspector.AuditRules.CombineCssResourcesRule = function(allowedPerDomain) {
    WebInspector.AuditRules.CombineExternalResourcesRule.call(this, "page-externalcss", "Combine external CSS", WebInspector.resourceTypes.Stylesheet, "CSS", allowedPerDomain);
}

WebInspector.AuditRules.CombineCssResourcesRule.prototype = {
    __proto__: WebInspector.AuditRules.CombineExternalResourcesRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.MinimizeDnsLookupsRule = function(hostCountThreshold) {
    WebInspector.AuditRule.call(this, "network-minimizelookups", "Minimize DNS lookups");
    this._hostCountThreshold = hostCountThreshold;
}

WebInspector.AuditRules.MinimizeDnsLookupsRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var summary = result.addChild("");
        var domainToResourcesMap = WebInspector.AuditRules.getDomainToResourcesMap(requests, null, false);
        for (var domain in domainToResourcesMap) {
            if (domainToResourcesMap[domain].length > 1)
                continue;
            var parsedURL = domain.asParsedURL();
            if (!parsedURL)
                continue;
            if (!parsedURL.host.search(WebInspector.AuditRules.IPAddressRegexp))
                continue; // an IP address
            summary.addSnippet(domain);
            result.violationCount++;
        }
        if (!summary.children || summary.children.length <= this._hostCountThreshold)
            return callback(null);

        summary.value = "The following domains only serve one resource each. If possible, avoid the extra DNS lookups by serving these resources from existing domains.";
        callback(result);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.ParallelizeDownloadRule = function(optimalHostnameCount, minRequestThreshold, minBalanceThreshold)
{
    WebInspector.AuditRule.call(this, "network-parallelizehosts", "Parallelize downloads across hostnames");
    this._optimalHostnameCount = optimalHostnameCount;
    this._minRequestThreshold = minRequestThreshold;
    this._minBalanceThreshold = minBalanceThreshold;
}

WebInspector.AuditRules.ParallelizeDownloadRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        function hostSorter(a, b)
        {
            var aCount = domainToResourcesMap[a].length;
            var bCount = domainToResourcesMap[b].length;
            return (aCount < bCount) ? 1 : (aCount == bCount) ? 0 : -1;
        }

        var domainToResourcesMap = WebInspector.AuditRules.getDomainToResourcesMap(
            requests,
            [WebInspector.resourceTypes.Stylesheet, WebInspector.resourceTypes.Image],
            true);

        var hosts = [];
        for (var url in domainToResourcesMap)
            hosts.push(url);

        if (!hosts.length)
            return callback(null); // no hosts (local file or something)

        hosts.sort(hostSorter);

        var optimalHostnameCount = this._optimalHostnameCount;
        if (hosts.length > optimalHostnameCount)
            hosts.splice(optimalHostnameCount);

        var busiestHostResourceCount = domainToResourcesMap[hosts[0]].length;
        var requestCountAboveThreshold = busiestHostResourceCount - this._minRequestThreshold;
        if (requestCountAboveThreshold <= 0)
            return callback(null);

        var avgResourcesPerHost = 0;
        for (var i = 0, size = hosts.length; i < size; ++i)
            avgResourcesPerHost += domainToResourcesMap[hosts[i]].length;

        // Assume optimal parallelization.
        avgResourcesPerHost /= optimalHostnameCount;
        avgResourcesPerHost = Math.max(avgResourcesPerHost, 1);

        var pctAboveAvg = (requestCountAboveThreshold / avgResourcesPerHost) - 1.0;
        var minBalanceThreshold = this._minBalanceThreshold;
        if (pctAboveAvg < minBalanceThreshold)
            return callback(null);

        var requestsOnBusiestHost = domainToResourcesMap[hosts[0]];
        var entry = result.addChild(String.sprintf("This page makes %d parallelizable requests to %s. Increase download parallelization by distributing the following requests across multiple hostnames.", busiestHostResourceCount, hosts[0]), true);
        for (var i = 0; i < requestsOnBusiestHost.length; ++i)
            entry.addURL(requestsOnBusiestHost[i].url);

        result.violationCount = requestsOnBusiestHost.length;
        callback(result);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * The reported CSS rule size is incorrect (parsed != original in WebKit),
 * so use percentages instead, which gives a better approximation.
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.UnusedCssRule = function()
{
    WebInspector.AuditRule.call(this, "page-unusedcss", "Remove unused CSS rules");
}

WebInspector.AuditRules.UnusedCssRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var self = this;

        function evalCallback(styleSheets) {
            if (progress.isCanceled())
                return;

            if (!styleSheets.length)
                return callback(null);

            var pseudoSelectorRegexp = /:hover|:link|:active|:visited|:focus|:before|:after/;
            var selectors = [];
            var testedSelectors = {};
            for (var i = 0; i < styleSheets.length; ++i) {
                var styleSheet = styleSheets[i];
                for (var curRule = 0; curRule < styleSheet.rules.length; ++curRule) {
                    var selectorText = styleSheet.rules[curRule].selectorText;
                    if (selectorText.match(pseudoSelectorRegexp) || testedSelectors[selectorText])
                        continue;
                    selectors.push(selectorText);
                    testedSelectors[selectorText] = 1;
                }
            }

            function selectorsCallback(callback, styleSheets, testedSelectors, foundSelectors)
            {
                if (progress.isCanceled())
                    return;

                var inlineBlockOrdinal = 0;
                var totalStylesheetSize = 0;
                var totalUnusedStylesheetSize = 0;
                var summary;

                for (var i = 0; i < styleSheets.length; ++i) {
                    var styleSheet = styleSheets[i];
                    var unusedRules = [];
                    for (var curRule = 0; curRule < styleSheet.rules.length; ++curRule) {
                        var rule = styleSheet.rules[curRule];
                        if (!testedSelectors[rule.selectorText] || foundSelectors[rule.selectorText])
                            continue;
                        unusedRules.push(rule.selectorText);
                    }
                    totalStylesheetSize += styleSheet.rules.length;
                    totalUnusedStylesheetSize += unusedRules.length;

                    if (!unusedRules.length)
                        continue;

                    var resource = WebInspector.resourceForURL(styleSheet.sourceURL);
                    var isInlineBlock = resource && resource.request && resource.request.type == WebInspector.resourceTypes.Document;
                    var url = !isInlineBlock ? WebInspector.AuditRuleResult.linkifyDisplayName(styleSheet.sourceURL) : String.sprintf("Inline block #%d", ++inlineBlockOrdinal);
                    var pctUnused = Math.round(100 * unusedRules.length / styleSheet.rules.length);
                    if (!summary)
                        summary = result.addChild("", true);
                    var entry = summary.addFormatted("%s: %d% is not used by the current page.", url, pctUnused);

                    for (var j = 0; j < unusedRules.length; ++j)
                        entry.addSnippet(unusedRules[j]);

                    result.violationCount += unusedRules.length;
                }

                if (!totalUnusedStylesheetSize)
                    return callback(null);

                var totalUnusedPercent = Math.round(100 * totalUnusedStylesheetSize / totalStylesheetSize);
                summary.value = String.sprintf("%s rules (%d%) of CSS not used by the current page.", totalUnusedStylesheetSize, totalUnusedPercent);

                callback(result);
            }

            var foundSelectors = {};
            function queryCallback(boundSelectorsCallback, selector, styleSheets, testedSelectors, nodeId)
            {
                if (nodeId)
                    foundSelectors[selector] = true;
                if (boundSelectorsCallback)
                    boundSelectorsCallback(foundSelectors);
            }

            function documentLoaded(selectors, document) {
                for (var i = 0; i < selectors.length; ++i) {
                    if (progress.isCanceled())
                        return;
                    WebInspector.domAgent.querySelector(document.id, selectors[i], queryCallback.bind(null, i === selectors.length - 1 ? selectorsCallback.bind(null, callback, styleSheets, testedSelectors) : null, selectors[i], styleSheets, testedSelectors));
                }
            }

            WebInspector.domAgent.requestDocument(documentLoaded.bind(null, selectors));
        }

        function styleSheetCallback(styleSheets, sourceURL, continuation, styleSheet)
        {
            if (progress.isCanceled())
                return;

            if (styleSheet) {
                styleSheet.sourceURL = sourceURL;
                styleSheets.push(styleSheet);
            }
            if (continuation)
                continuation(styleSheets);
        }

        function allStylesCallback(error, styleSheetInfos)
        {
            if (progress.isCanceled())
                return;

            if (error || !styleSheetInfos || !styleSheetInfos.length)
                return evalCallback([]);
            var styleSheets = [];
            for (var i = 0; i < styleSheetInfos.length; ++i) {
                var info = styleSheetInfos[i];
                WebInspector.CSSStyleSheet.createForId(info.styleSheetId, styleSheetCallback.bind(null, styleSheets, info.sourceURL, i == styleSheetInfos.length - 1 ? evalCallback : null));
            }
        }

        CSSAgent.getAllStyleSheets(allStylesCallback);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.CacheControlRule = function(id, name)
{
    WebInspector.AuditRule.call(this, id, name);
}

WebInspector.AuditRules.CacheControlRule.MillisPerMonth = 1000 * 60 * 60 * 24 * 30;

WebInspector.AuditRules.CacheControlRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var cacheableAndNonCacheableResources = this._cacheableAndNonCacheableResources(requests);
        if (cacheableAndNonCacheableResources[0].length)
            this.runChecks(cacheableAndNonCacheableResources[0], result);
        this.handleNonCacheableResources(cacheableAndNonCacheableResources[1], result);

        callback(result);
    },

    handleNonCacheableResources: function(requests, result)
    {
    },

    _cacheableAndNonCacheableResources: function(requests)
    {
        var processedResources = [[], []];
        for (var i = 0; i < requests.length; ++i) {
            var request = requests[i];
            if (!this.isCacheableResource(request))
                continue;
            if (this._isExplicitlyNonCacheable(request))
                processedResources[1].push(request);
            else
                processedResources[0].push(request);
        }
        return processedResources;
    },

    execCheck: function(messageText, requestCheckFunction, requests, result)
    {
        var requestCount = requests.length;
        var urls = [];
        for (var i = 0; i < requestCount; ++i) {
            if (requestCheckFunction.call(this, requests[i]))
                urls.push(requests[i].url);
        }
        if (urls.length) {
            var entry = result.addChild(messageText, true);
            entry.addURLs(urls);
            result.violationCount += urls.length;
        }
    },

    freshnessLifetimeGreaterThan: function(request, timeMs)
    {
        var dateHeader = this.responseHeader(request, "Date");
        if (!dateHeader)
            return false;

        var dateHeaderMs = Date.parse(dateHeader);
        if (isNaN(dateHeaderMs))
            return false;

        var freshnessLifetimeMs;
        var maxAgeMatch = this.responseHeaderMatch(request, "Cache-Control", "max-age=(\\d+)");

        if (maxAgeMatch)
            freshnessLifetimeMs = (maxAgeMatch[1]) ? 1000 * maxAgeMatch[1] : 0;
        else {
            var expiresHeader = this.responseHeader(request, "Expires");
            if (expiresHeader) {
                var expDate = Date.parse(expiresHeader);
                if (!isNaN(expDate))
                    freshnessLifetimeMs = expDate - dateHeaderMs;
            }
        }

        return (isNaN(freshnessLifetimeMs)) ? false : freshnessLifetimeMs > timeMs;
    },

    responseHeader: function(request, header)
    {
        return request.responseHeaderValue(header);
    },

    hasResponseHeader: function(request, header)
    {
        return request.responseHeaderValue(header) !== undefined;
    },

    isCompressible: function(request)
    {
        return request.type.isTextType();
    },

    isPubliclyCacheable: function(request)
    {
        if (this._isExplicitlyNonCacheable(request))
            return false;

        if (this.responseHeaderMatch(request, "Cache-Control", "public"))
            return true;

        return request.url.indexOf("?") == -1 && !this.responseHeaderMatch(request, "Cache-Control", "private");
    },

    responseHeaderMatch: function(request, header, regexp)
    {
        return request.responseHeaderValue(header)
            ? request.responseHeaderValue(header).match(new RegExp(regexp, "im"))
            : undefined;
    },

    hasExplicitExpiration: function(request)
    {
        return this.hasResponseHeader(request, "Date") &&
            (this.hasResponseHeader(request, "Expires") || this.responseHeaderMatch(request, "Cache-Control", "max-age"));
    },

    _isExplicitlyNonCacheable: function(request)
    {
        var hasExplicitExp = this.hasExplicitExpiration(request);
        return this.responseHeaderMatch(request, "Cache-Control", "(no-cache|no-store|must-revalidate)") ||
            this.responseHeaderMatch(request, "Pragma", "no-cache") ||
            (hasExplicitExp && !this.freshnessLifetimeGreaterThan(request, 0)) ||
            (!hasExplicitExp && request.url && request.url.indexOf("?") >= 0) ||
            (!hasExplicitExp && !this.isCacheableResource(request));
    },

    isCacheableResource: function(request)
    {
        return request.statusCode !== undefined && WebInspector.AuditRules.CacheableResponseCodes[request.statusCode];
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CacheControlRule}
 */
WebInspector.AuditRules.BrowserCacheControlRule = function()
{
    WebInspector.AuditRules.CacheControlRule.call(this, "http-browsercache", "Leverage browser caching");
}

WebInspector.AuditRules.BrowserCacheControlRule.prototype = {
    handleNonCacheableResources: function(requests, result)
    {
        if (requests.length) {
            var entry = result.addChild("The following resources are explicitly non-cacheable. Consider making them cacheable if possible:", true);
            result.violationCount += requests.length;
            for (var i = 0; i < requests.length; ++i)
                entry.addURL(requests[i].url);
        }
    },

    runChecks: function(requests, result, callback)
    {
        this.execCheck("The following resources are missing a cache expiration. Resources that do not specify an expiration may not be cached by browsers:",
            this._missingExpirationCheck, requests, result);
        this.execCheck("The following resources specify a \"Vary\" header that disables caching in most versions of Internet Explorer:",
            this._varyCheck, requests, result);
        this.execCheck("The following cacheable resources have a short freshness lifetime:",
            this._oneMonthExpirationCheck, requests, result);

        // Unable to implement the favicon check due to the WebKit limitations.
        this.execCheck("To further improve cache hit rate, specify an expiration one year in the future for the following cacheable resources:",
            this._oneYearExpirationCheck, requests, result);
    },

    _missingExpirationCheck: function(request)
    {
        return this.isCacheableResource(request) && !this.hasResponseHeader(request, "Set-Cookie") && !this.hasExplicitExpiration(request);
    },

    _varyCheck: function(request)
    {
        var varyHeader = this.responseHeader(request, "Vary");
        if (varyHeader) {
            varyHeader = varyHeader.replace(/User-Agent/gi, "");
            varyHeader = varyHeader.replace(/Accept-Encoding/gi, "");
            varyHeader = varyHeader.replace(/[, ]*/g, "");
        }
        return varyHeader && varyHeader.length && this.isCacheableResource(request) && this.freshnessLifetimeGreaterThan(request, 0);
    },

    _oneMonthExpirationCheck: function(request)
    {
        return this.isCacheableResource(request) &&
            !this.hasResponseHeader(request, "Set-Cookie") &&
            !this.freshnessLifetimeGreaterThan(request, WebInspector.AuditRules.CacheControlRule.MillisPerMonth) &&
            this.freshnessLifetimeGreaterThan(request, 0);
    },

    _oneYearExpirationCheck: function(request)
    {
        return this.isCacheableResource(request) &&
            !this.hasResponseHeader(request, "Set-Cookie") &&
            !this.freshnessLifetimeGreaterThan(request, 11 * WebInspector.AuditRules.CacheControlRule.MillisPerMonth) &&
            this.freshnessLifetimeGreaterThan(request, WebInspector.AuditRules.CacheControlRule.MillisPerMonth);
    },

    __proto__: WebInspector.AuditRules.CacheControlRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CacheControlRule}
 */
WebInspector.AuditRules.ProxyCacheControlRule = function() {
    WebInspector.AuditRules.CacheControlRule.call(this, "http-proxycache", "Leverage proxy caching");
}

WebInspector.AuditRules.ProxyCacheControlRule.prototype = {
    runChecks: function(requests, result, callback)
    {
        this.execCheck("Resources with a \"?\" in the URL are not cached by most proxy caching servers:",
            this._questionMarkCheck, requests, result);
        this.execCheck("Consider adding a \"Cache-Control: public\" header to the following resources:",
            this._publicCachingCheck, requests, result);
        this.execCheck("The following publicly cacheable resources contain a Set-Cookie header. This security vulnerability can cause cookies to be shared by multiple users.",
            this._setCookieCacheableCheck, requests, result);
    },

    _questionMarkCheck: function(request)
    {
        return request.url.indexOf("?") >= 0 && !this.hasResponseHeader(request, "Set-Cookie") && this.isPubliclyCacheable(request);
    },

    _publicCachingCheck: function(request)
    {
        return this.isCacheableResource(request) &&
            !this.isCompressible(request) &&
            !this.responseHeaderMatch(request, "Cache-Control", "public") &&
            !this.hasResponseHeader(request, "Set-Cookie");
    },

    _setCookieCacheableCheck: function(request)
    {
        return this.hasResponseHeader(request, "Set-Cookie") && this.isPubliclyCacheable(request);
    },

    __proto__: WebInspector.AuditRules.CacheControlRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.ImageDimensionsRule = function()
{
    WebInspector.AuditRule.call(this, "page-imagedims", "Specify image dimensions");
}

WebInspector.AuditRules.ImageDimensionsRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var urlToNoDimensionCount = {};

        function doneCallback()
        {
            for (var url in urlToNoDimensionCount) {
                var entry = entry || result.addChild("A width and height should be specified for all images in order to speed up page display. The following image(s) are missing a width and/or height:", true);
                var format = "%r";
                if (urlToNoDimensionCount[url] > 1)
                    format += " (%d uses)";
                entry.addFormatted(format, url, urlToNoDimensionCount[url]);
                result.violationCount++;
            }
            callback(entry ? result : null);
        }

        function imageStylesReady(imageId, styles, isLastStyle, computedStyle)
        {
            if (progress.isCanceled())
                return;

            const node = WebInspector.domAgent.nodeForId(imageId);
            var src = node.getAttribute("src");
            if (!src.asParsedURL()) {
                for (var frameOwnerCandidate = node; frameOwnerCandidate; frameOwnerCandidate = frameOwnerCandidate.parentNode) {
                    if (frameOwnerCandidate.baseURL) {
                        var completeSrc = WebInspector.ParsedURL.completeURL(frameOwnerCandidate.baseURL, src);
                        break;
                    }
                }
            }
            if (completeSrc)
                src = completeSrc;

            if (computedStyle.getPropertyValue("position") === "absolute") {
                if (isLastStyle)
                    doneCallback();
                return;
            }

            if (styles.attributesStyle) {
                var widthFound = !!styles.attributesStyle.getLiveProperty("width");
                var heightFound = !!styles.attributesStyle.getLiveProperty("height");
            }

            var inlineStyle = styles.inlineStyle;
            if (inlineStyle) {
                if (inlineStyle.getPropertyValue("width") !== "")
                    widthFound = true;
                if (inlineStyle.getPropertyValue("height") !== "")
                    heightFound = true;
            }

            for (var i = styles.matchedCSSRules.length - 1; i >= 0 && !(widthFound && heightFound); --i) {
                var style = styles.matchedCSSRules[i].style;
                if (style.getPropertyValue("width") !== "")
                    widthFound = true;
                if (style.getPropertyValue("height") !== "")
                    heightFound = true;
            }

            if (!widthFound || !heightFound) {
                if (src in urlToNoDimensionCount)
                    ++urlToNoDimensionCount[src];
                else
                    urlToNoDimensionCount[src] = 1;
            }

            if (isLastStyle)
                doneCallback();
        }

        function getStyles(nodeIds)
        {
            if (progress.isCanceled())
                return;
            var targetResult = {};

            function inlineCallback(inlineStyle, attributesStyle)
            {
                targetResult.inlineStyle = inlineStyle;
                targetResult.attributesStyle = attributesStyle;
            }

            function matchedCallback(result)
            {
                if (result)
                    targetResult.matchedCSSRules = result.matchedCSSRules;
            }

            if (!nodeIds || !nodeIds.length)
                doneCallback();

            for (var i = 0; nodeIds && i < nodeIds.length; ++i) {
                WebInspector.cssModel.getMatchedStylesAsync(nodeIds[i], false, false, matchedCallback);
                WebInspector.cssModel.getInlineStylesAsync(nodeIds[i], inlineCallback);
                WebInspector.cssModel.getComputedStyleAsync(nodeIds[i], imageStylesReady.bind(null, nodeIds[i], targetResult, i === nodeIds.length - 1));
            }
        }

        function onDocumentAvailable(root)
        {
            if (progress.isCanceled())
                return;
            WebInspector.domAgent.querySelectorAll(root.id, "img[src]", getStyles);
        }

        if (progress.isCanceled())
            return;
        WebInspector.domAgent.requestDocument(onDocumentAvailable);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.CssInHeadRule = function()
{
    WebInspector.AuditRule.call(this, "page-cssinhead", "Put CSS in the document head");
}

WebInspector.AuditRules.CssInHeadRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        function evalCallback(evalResult)
        {
            if (progress.isCanceled())
                return;

            if (!evalResult)
                return callback(null);

            var summary = result.addChild("");

            var outputMessages = [];
            for (var url in evalResult) {
                var urlViolations = evalResult[url];
                if (urlViolations[0]) {
                    result.addFormatted("%s style block(s) in the %r body should be moved to the document head.", urlViolations[0], url);
                    result.violationCount += urlViolations[0];
                }
                for (var i = 0; i < urlViolations[1].length; ++i)
                    result.addFormatted("Link node %r should be moved to the document head in %r", urlViolations[1][i], url);
                result.violationCount += urlViolations[1].length;
            }
            summary.value = String.sprintf("CSS in the document body adversely impacts rendering performance.");
            callback(result);
        }

        function externalStylesheetsReceived(root, inlineStyleNodeIds, nodeIds)
        {
            if (progress.isCanceled())
                return;

            if (!nodeIds)
                return;
            var externalStylesheetNodeIds = nodeIds;
            var result = null;
            if (inlineStyleNodeIds.length || externalStylesheetNodeIds.length) {
                var urlToViolationsArray = {};
                var externalStylesheetHrefs = [];
                for (var j = 0; j < externalStylesheetNodeIds.length; ++j) {
                    var linkNode = WebInspector.domAgent.nodeForId(externalStylesheetNodeIds[j]);
                    var completeHref = WebInspector.ParsedURL.completeURL(linkNode.ownerDocument.baseURL, linkNode.getAttribute("href"));
                    externalStylesheetHrefs.push(completeHref || "<empty>");
                }
                urlToViolationsArray[root.documentURL] = [inlineStyleNodeIds.length, externalStylesheetHrefs];
                result = urlToViolationsArray;
            }
            evalCallback(result);
        }

        function inlineStylesReceived(root, nodeIds)
        {
            if (progress.isCanceled())
                return;

            if (!nodeIds)
                return;
            WebInspector.domAgent.querySelectorAll(root.id, "body link[rel~='stylesheet'][href]", externalStylesheetsReceived.bind(null, root, nodeIds));
        }

        function onDocumentAvailable(root)
        {
            if (progress.isCanceled())
                return;

            WebInspector.domAgent.querySelectorAll(root.id, "body style", inlineStylesReceived.bind(null, root));
        }

        WebInspector.domAgent.requestDocument(onDocumentAvailable);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.StylesScriptsOrderRule = function()
{
    WebInspector.AuditRule.call(this, "page-stylescriptorder", "Optimize the order of styles and scripts");
}

WebInspector.AuditRules.StylesScriptsOrderRule.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        function evalCallback(resultValue)
        {
            if (progress.isCanceled())
                return;

            if (!resultValue)
                return callback(null);

            var lateCssUrls = resultValue[0];
            var cssBeforeInlineCount = resultValue[1];

            var entry = result.addChild("The following external CSS files were included after an external JavaScript file in the document head. To ensure CSS files are downloaded in parallel, always include external CSS before external JavaScript.", true);
            entry.addURLs(lateCssUrls);
            result.violationCount += lateCssUrls.length;

            if (cssBeforeInlineCount) {
                result.addChild(String.sprintf(" %d inline script block%s found in the head between an external CSS file and another resource. To allow parallel downloading, move the inline script before the external CSS file, or after the next resource.", cssBeforeInlineCount, cssBeforeInlineCount > 1 ? "s were" : " was"));
                result.violationCount += cssBeforeInlineCount;
            }
            callback(result);
        }

        function cssBeforeInlineReceived(lateStyleIds, nodeIds)
        {
            if (progress.isCanceled())
                return;

            if (!nodeIds)
                return;

            var cssBeforeInlineCount = nodeIds.length;
            var result = null;
            if (lateStyleIds.length || cssBeforeInlineCount) {
                var lateStyleUrls = [];
                for (var i = 0; i < lateStyleIds.length; ++i) {
                    var lateStyleNode = WebInspector.domAgent.nodeForId(lateStyleIds[i]);
                    var completeHref = WebInspector.ParsedURL.completeURL(lateStyleNode.ownerDocument.baseURL, lateStyleNode.getAttribute("href"));
                    lateStyleUrls.push(completeHref || "<empty>");
                }
                result = [ lateStyleUrls, cssBeforeInlineCount ];
            }

            evalCallback(result);
        }

        function lateStylesReceived(root, nodeIds)
        {
            if (progress.isCanceled())
                return;

            if (!nodeIds)
                return;

            WebInspector.domAgent.querySelectorAll(root.id, "head link[rel~='stylesheet'][href] ~ script:not([src])", cssBeforeInlineReceived.bind(null, nodeIds));
        }

        function onDocumentAvailable(root)
        {
            if (progress.isCanceled())
                return;

            WebInspector.domAgent.querySelectorAll(root.id, "head script[src] ~ link[rel~='stylesheet'][href]", lateStylesReceived.bind(null, root));
        }

        WebInspector.domAgent.requestDocument(onDocumentAvailable);
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.CSSRuleBase = function(id, name)
{
    WebInspector.AuditRule.call(this, id, name);
}

WebInspector.AuditRules.CSSRuleBase.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        CSSAgent.getAllStyleSheets(sheetsCallback.bind(this));

        function sheetsCallback(error, headers)
        {
            if (error)
                return callback(null);

            if (!headers.length)
                return callback(null);
            for (var i = 0; i < headers.length; ++i) {
                var header = headers[i];
                if (header.disabled)
                    continue; // Do not check disabled stylesheets.

                this._visitStyleSheet(header.styleSheetId, i === headers.length - 1 ? finishedCallback : null, result, progress);
            }
        }

        function finishedCallback()
        {
            callback(result);
        }
    },

    _visitStyleSheet: function(styleSheetId, callback, result, progress)
    {
        WebInspector.CSSStyleSheet.createForId(styleSheetId, sheetCallback.bind(this));

        function sheetCallback(styleSheet)
        {
            if (progress.isCanceled())
                return;

            if (!styleSheet) {
                if (callback)
                    callback();
                return;
            }

            this.visitStyleSheet(styleSheet, result);

            for (var i = 0; i < styleSheet.rules.length; ++i)
                this._visitRule(styleSheet, styleSheet.rules[i], result);

            this.didVisitStyleSheet(styleSheet, result);

            if (callback)
                callback();
        }
    },

    _visitRule: function(styleSheet, rule, result)
    {
        this.visitRule(styleSheet, rule, result);
        var allProperties = rule.style.allProperties;
        for (var i = 0; i < allProperties.length; ++i)
            this.visitProperty(styleSheet, allProperties[i], result);
        this.didVisitRule(styleSheet, rule, result);
    },

    visitStyleSheet: function(styleSheet, result)
    {
        // Subclasses can implement.
    },

    didVisitStyleSheet: function(styleSheet, result)
    {
        // Subclasses can implement.
    },
    
    visitRule: function(styleSheet, rule, result)
    {
        // Subclasses can implement.
    },

    didVisitRule: function(styleSheet, rule, result)
    {
        // Subclasses can implement.
    },
    
    visitProperty: function(styleSheet, property, result)
    {
        // Subclasses can implement.
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CSSRuleBase}
 */
WebInspector.AuditRules.VendorPrefixedCSSProperties = function()
{
    WebInspector.AuditRules.CSSRuleBase.call(this, "page-vendorprefixedcss", "Use normal CSS property names instead of vendor-prefixed ones");
    this._webkitPrefix = "-webkit-";
}

WebInspector.AuditRules.VendorPrefixedCSSProperties.supportedProperties = [
    "background-clip", "background-origin", "background-size",
    "border-radius", "border-bottom-left-radius", "border-bottom-right-radius", "border-top-left-radius", "border-top-right-radius",
    "box-shadow", "box-sizing", "opacity", "text-shadow"
].keySet();

WebInspector.AuditRules.VendorPrefixedCSSProperties.prototype = {
    didVisitStyleSheet: function(styleSheet)
    {
        delete this._styleSheetResult;
    },

    visitRule: function(rule)
    {
        this._mentionedProperties = {};
    },

    didVisitRule: function()
    {
        delete this._ruleResult;
        delete this._mentionedProperties;
    },

    visitProperty: function(styleSheet, property, result)
    {
        if (!property.name.startsWith(this._webkitPrefix))
            return;

        var normalPropertyName = property.name.substring(this._webkitPrefix.length).toLowerCase(); // Start just after the "-webkit-" prefix.
        if (WebInspector.AuditRules.VendorPrefixedCSSProperties.supportedProperties[normalPropertyName] && !this._mentionedProperties[normalPropertyName]) {
            var style = property.ownerStyle;
            var liveProperty = style.getLiveProperty(normalPropertyName);
            if (liveProperty && !liveProperty.styleBased)
                return; // WebCore can provide normal versions of prefixed properties automatically, so be careful to skip only normal source-based properties.

            var rule = style.parentRule;
            this._mentionedProperties[normalPropertyName] = true;
            if (!this._styleSheetResult)
                this._styleSheetResult = result.addChild(rule.sourceURL ? WebInspector.linkifyResourceAsNode(rule.sourceURL) : "<unknown>");
            if (!this._ruleResult) {
                var anchor = WebInspector.linkifyURLAsNode(rule.sourceURL, rule.selectorText);
                anchor.preferredPanel = "resources";
                anchor.lineNumber = rule.sourceLine;
                this._ruleResult = this._styleSheetResult.addChild(anchor);
            }
            ++result.violationCount;
            this._ruleResult.addSnippet(String.sprintf("\"" + this._webkitPrefix + "%s\" is used, but \"%s\" is supported.", normalPropertyName, normalPropertyName));
        }
    },

    __proto__: WebInspector.AuditRules.CSSRuleBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRule}
 */
WebInspector.AuditRules.CookieRuleBase = function(id, name)
{
    WebInspector.AuditRule.call(this, id, name);
}

WebInspector.AuditRules.CookieRuleBase.prototype = {
    /**
     * @param {!Array.<!WebInspector.NetworkRequest>} requests
     * @param {!WebInspector.AuditRuleResult} result
     * @param {function(WebInspector.AuditRuleResult)} callback
     * @param {!WebInspector.Progress} progress
     */
    doRun: function(requests, result, callback, progress)
    {
        var self = this;
        function resultCallback(receivedCookies, isAdvanced) {
            if (progress.isCanceled())
                return;

            self.processCookies(isAdvanced ? receivedCookies : [], requests, result);
            callback(result);
        }

        WebInspector.Cookies.getCookiesAsync(resultCallback);
    },

    mapResourceCookies: function(requestsByDomain, allCookies, callback)
    {
        for (var i = 0; i < allCookies.length; ++i) {
            for (var requestDomain in requestsByDomain) {
                if (WebInspector.Cookies.cookieDomainMatchesResourceDomain(allCookies[i].domain(), requestDomain))
                    this._callbackForResourceCookiePairs(requestsByDomain[requestDomain], allCookies[i], callback);
            }
        }
    },

    _callbackForResourceCookiePairs: function(requests, cookie, callback)
    {
        if (!requests)
            return;
        for (var i = 0; i < requests.length; ++i) {
            if (WebInspector.Cookies.cookieMatchesResourceURL(cookie, requests[i].url))
                callback(requests[i], cookie);
        }
    },

    __proto__: WebInspector.AuditRule.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CookieRuleBase}
 */
WebInspector.AuditRules.CookieSizeRule = function(avgBytesThreshold)
{
    WebInspector.AuditRules.CookieRuleBase.call(this, "http-cookiesize", "Minimize cookie size");
    this._avgBytesThreshold = avgBytesThreshold;
    this._maxBytesThreshold = 1000;
}

WebInspector.AuditRules.CookieSizeRule.prototype = {
    _average: function(cookieArray)
    {
        var total = 0;
        for (var i = 0; i < cookieArray.length; ++i)
            total += cookieArray[i].size();
        return cookieArray.length ? Math.round(total / cookieArray.length) : 0;
    },

    _max: function(cookieArray)
    {
        var result = 0;
        for (var i = 0; i < cookieArray.length; ++i)
            result = Math.max(cookieArray[i].size(), result);
        return result;
    },

    processCookies: function(allCookies, requests, result)
    {
        function maxSizeSorter(a, b)
        {
            return b.maxCookieSize - a.maxCookieSize;
        }

        function avgSizeSorter(a, b)
        {
            return b.avgCookieSize - a.avgCookieSize;
        }

        var cookiesPerResourceDomain = {};

        function collectorCallback(request, cookie)
        {
            var cookies = cookiesPerResourceDomain[request.parsedURL.host];
            if (!cookies) {
                cookies = [];
                cookiesPerResourceDomain[request.parsedURL.host] = cookies;
            }
            cookies.push(cookie);
        }

        if (!allCookies.length)
            return;

        var sortedCookieSizes = [];

        var domainToResourcesMap = WebInspector.AuditRules.getDomainToResourcesMap(requests,
                null,
                true);
        var matchingResourceData = {};
        this.mapResourceCookies(domainToResourcesMap, allCookies, collectorCallback.bind(this));

        for (var requestDomain in cookiesPerResourceDomain) {
            var cookies = cookiesPerResourceDomain[requestDomain];
            sortedCookieSizes.push({
                domain: requestDomain,
                avgCookieSize: this._average(cookies),
                maxCookieSize: this._max(cookies)
            });
        }
        var avgAllCookiesSize = this._average(allCookies);

        var hugeCookieDomains = [];
        sortedCookieSizes.sort(maxSizeSorter);

        for (var i = 0, len = sortedCookieSizes.length; i < len; ++i) {
            var maxCookieSize = sortedCookieSizes[i].maxCookieSize;
            if (maxCookieSize > this._maxBytesThreshold)
                hugeCookieDomains.push(WebInspector.AuditRuleResult.resourceDomain(sortedCookieSizes[i].domain) + ": " + Number.bytesToString(maxCookieSize));
        }

        var bigAvgCookieDomains = [];
        sortedCookieSizes.sort(avgSizeSorter);
        for (var i = 0, len = sortedCookieSizes.length; i < len; ++i) {
            var domain = sortedCookieSizes[i].domain;
            var avgCookieSize = sortedCookieSizes[i].avgCookieSize;
            if (avgCookieSize > this._avgBytesThreshold && avgCookieSize < this._maxBytesThreshold)
                bigAvgCookieDomains.push(WebInspector.AuditRuleResult.resourceDomain(domain) + ": " + Number.bytesToString(avgCookieSize));
        }
        result.addChild(String.sprintf("The average cookie size for all requests on this page is %s", Number.bytesToString(avgAllCookiesSize)));

        var message;
        if (hugeCookieDomains.length) {
            var entry = result.addChild("The following domains have a cookie size in excess of 1KB. This is harmful because requests with cookies larger than 1KB typically cannot fit into a single network packet.", true);
            entry.addURLs(hugeCookieDomains);
            result.violationCount += hugeCookieDomains.length;
        }

        if (bigAvgCookieDomains.length) {
            var entry = result.addChild(String.sprintf("The following domains have an average cookie size in excess of %d bytes. Reducing the size of cookies for these domains can reduce the time it takes to send requests.", this._avgBytesThreshold), true);
            entry.addURLs(bigAvgCookieDomains);
            result.violationCount += bigAvgCookieDomains.length;
        }
    },

    __proto__: WebInspector.AuditRules.CookieRuleBase.prototype
}

/**
 * @constructor
 * @extends {WebInspector.AuditRules.CookieRuleBase}
 */
WebInspector.AuditRules.StaticCookielessRule = function(minResources)
{
    WebInspector.AuditRules.CookieRuleBase.call(this, "http-staticcookieless", "Serve static content from a cookieless domain");
    this._minResources = minResources;
}

WebInspector.AuditRules.StaticCookielessRule.prototype = {
    processCookies: function(allCookies, requests, result)
    {
        var domainToResourcesMap = WebInspector.AuditRules.getDomainToResourcesMap(requests,
                [WebInspector.resourceTypes.Stylesheet,
                 WebInspector.resourceTypes.Image],
                true);
        var totalStaticResources = 0;
        for (var domain in domainToResourcesMap)
            totalStaticResources += domainToResourcesMap[domain].length;
        if (totalStaticResources < this._minResources)
            return;
        var matchingResourceData = {};
        this.mapResourceCookies(domainToResourcesMap, allCookies, this._collectorCallback.bind(this, matchingResourceData));

        var badUrls = [];
        var cookieBytes = 0;
        for (var url in matchingResourceData) {
            badUrls.push(url);
            cookieBytes += matchingResourceData[url]
        }
        if (badUrls.length < this._minResources)
            return;

        var entry = result.addChild(String.sprintf("%s of cookies were sent with the following static resources. Serve these static resources from a domain that does not set cookies:", Number.bytesToString(cookieBytes)), true);
        entry.addURLs(badUrls);
        result.violationCount = badUrls.length;
    },

    _collectorCallback: function(matchingResourceData, request, cookie)
    {
        matchingResourceData[request.url] = (matchingResourceData[request.url] || 0) + cookie.size();
    },

    __proto__: WebInspector.AuditRules.CookieRuleBase.prototype
}
