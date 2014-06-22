/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

/**
 * @constructor
 */
WebInspector.UserAgentSupport = function()
{
    this._userAgentOverrideEnabled = false;
    this._deviceMetricsOverrideEnabled = false;
    this._geolocationPositionOverrideEnabled = false;
    this._deviceOrientationOverrideEnabled = false;

    WebInspector.settings.userAgent.addChangeListener(this._userAgentChanged, this);
    WebInspector.settings.deviceMetrics.addChangeListener(this._deviceMetricsChanged, this);
    WebInspector.settings.deviceFitWindow.addChangeListener(this._deviceMetricsChanged, this);
    WebInspector.settings.geolocationOverride.addChangeListener(this._geolocationPositionChanged, this);
    WebInspector.settings.deviceOrientationOverride.addChangeListener(this._deviceOrientationChanged, this);
}

/**
 * @constructor
 * @param {number} width
 * @param {number} height
 * @param {number} fontScaleFactor
 */
WebInspector.UserAgentSupport.DeviceMetrics = function(width, height, fontScaleFactor)
{
    this.width = width;
    this.height = height;
    this.fontScaleFactor = fontScaleFactor;
}

/**
 * @return {WebInspector.UserAgentSupport.DeviceMetrics}
 */
WebInspector.UserAgentSupport.DeviceMetrics.parseSetting = function(value)
{
    if (value) {
        var splitMetrics = value.split("x");
        if (splitMetrics.length === 3)
            return new WebInspector.UserAgentSupport.DeviceMetrics(parseInt(splitMetrics[0], 10), parseInt(splitMetrics[1], 10), parseFloat(splitMetrics[2]));
    }
    return new WebInspector.UserAgentSupport.DeviceMetrics(0, 0, 1);
}

/**
 * @return {?WebInspector.UserAgentSupport.DeviceMetrics}
 */
WebInspector.UserAgentSupport.DeviceMetrics.parseUserInput = function(widthString, heightString, fontScaleFactorString)
{
    function isUserInputValid(value, isInteger)
    {
        if (!value)
            return true;
        return isInteger ? /^[0]*[1-9][\d]*$/.test(value) : /^[0]*([1-9][\d]*(\.\d+)?|\.\d+)$/.test(value);
    }

    if (!widthString ^ !heightString)
        return null;

    var isWidthValid = isUserInputValid(widthString, true);
    var isHeightValid = isUserInputValid(heightString, true);
    var isFontScaleFactorValid = isUserInputValid(fontScaleFactorString, false);

    if (!isWidthValid && !isHeightValid && !isFontScaleFactorValid)
        return null;

    var width = isWidthValid ? parseInt(widthString || "0", 10) : -1;
    var height = isHeightValid ? parseInt(heightString || "0", 10) : -1;
    var fontScaleFactor = isFontScaleFactorValid ? parseFloat(fontScaleFactorString) : -1;

    return new WebInspector.UserAgentSupport.DeviceMetrics(width, height, fontScaleFactor);
}

WebInspector.UserAgentSupport.DeviceMetrics.prototype = {
    /**
     * @return {boolean}
     */
    isValid: function()
    {
        return this.isWidthValid() && this.isHeightValid() && this.isFontScaleFactorValid();
    },

    /**
     * @return {boolean}
     */
    isWidthValid: function()
    {
        return this.width >= 0;
    },

    /**
     * @return {boolean}
     */
    isHeightValid: function()
    {
        return this.height >= 0;
    },

    /**
     * @return {boolean}
     */
    isFontScaleFactorValid: function()
    {
        return this.fontScaleFactor > 0;
    },

    /**
     * @return {string}
     */
    toSetting: function()
    {
        if (!this.isValid())
            return "";

        return this.width && this.height ? this.width + "x" + this.height + "x" + this.fontScaleFactor : "";
    },

    /**
     * @return {string}
     */
    widthToInput: function()
    {
        return this.isWidthValid() && this.width ? String(this.width) : "";
    },

    /**
     * @return {string}
     */
    heightToInput: function()
    {
        return this.isHeightValid() && this.height ? String(this.height) : "";
    },

    /**
     * @return {string}
     */
    fontScaleFactorToInput: function()
    {
        return this.isFontScaleFactorValid() && this.fontScaleFactor ? String(this.fontScaleFactor) : "";
    }
}

/**
 * @constructor
 * @param {number} latitude
 * @param {number} longitude
 */
WebInspector.UserAgentSupport.GeolocationPosition = function(latitude, longitude, error)
{
    this.latitude = latitude;
    this.longitude = longitude;
    this.error = error;
}

WebInspector.UserAgentSupport.GeolocationPosition.prototype = {
    /**
     * @return {string}
     */
    toSetting: function()
    {
        return (typeof this.latitude === "number" && typeof this.longitude === "number" && typeof this.error === "string") ? this.latitude + "@" + this.longitude + ":" + this.error : "";
    }
}

/**
 * @return {WebInspector.UserAgentSupport.GeolocationPosition}
 */
WebInspector.UserAgentSupport.GeolocationPosition.parseSetting = function(value)
{
    if (value) {
        var splitError = value.split(":");
        if (splitError.length === 2) {
            var splitPosition = splitError[0].split("@")
            if (splitPosition.length === 2)
                return new WebInspector.UserAgentSupport.GeolocationPosition(parseFloat(splitPosition[0]), parseFloat(splitPosition[1]), splitError[1]);
        }
    }
    return new WebInspector.UserAgentSupport.GeolocationPosition(0, 0, "");
}

/**
 * @return {?WebInspector.UserAgentSupport.GeolocationPosition}
 */
WebInspector.UserAgentSupport.GeolocationPosition.parseUserInput = function(latitudeString, longitudeString, errorStatus)
{
    function isUserInputValid(value)
    {
        if (!value)
            return true;
        return /^[-]?[0-9]*[.]?[0-9]*$/.test(value);
    }

    if (!latitudeString ^ !latitudeString)
        return null;

    var isLatitudeValid = isUserInputValid(latitudeString);
    var isLongitudeValid = isUserInputValid(longitudeString);

    if (!isLatitudeValid && !isLongitudeValid)
        return null;

    var latitude = isLatitudeValid ? parseFloat(latitudeString) : -1;
    var longitude = isLongitudeValid ? parseFloat(longitudeString) : -1;

    return new WebInspector.UserAgentSupport.GeolocationPosition(latitude, longitude, errorStatus ? "PositionUnavailable" : "");
}

WebInspector.UserAgentSupport.GeolocationPosition.clearGeolocationOverride = function()
{
    PageAgent.clearGeolocationOverride();
}

/**
 * @constructor
 * @param {number} alpha
 * @param {number} beta
 * @param {number} gamma
 */
WebInspector.UserAgentSupport.DeviceOrientation = function(alpha, beta, gamma)
{
    this.alpha = alpha;
    this.beta = beta;
    this.gamma = gamma;
}

WebInspector.UserAgentSupport.DeviceOrientation.prototype = {
    /**
     * @return {string}
     */
    toSetting: function()
    {
        return JSON.stringify(this);
    }
}

/**
 * @return {WebInspector.UserAgentSupport.DeviceOrientation}
 */
WebInspector.UserAgentSupport.DeviceOrientation.parseSetting = function(value)
{
    if (value) {
        var jsonObject = JSON.parse(value);
        return new WebInspector.UserAgentSupport.DeviceOrientation(jsonObject.alpha, jsonObject.beta, jsonObject.gamma);
    }
    return new WebInspector.UserAgentSupport.DeviceOrientation(0, 0, 0);
}

/**
 * @return {?WebInspector.UserAgentSupport.DeviceOrientation}
 */
WebInspector.UserAgentSupport.DeviceOrientation.parseUserInput = function(alphaString, betaString, gammaString)
{
    function isUserInputValid(value)
    {
        if (!value)
            return true;
        return /^[-]?[0-9]*[.]?[0-9]*$/.test(value);
    }

    if (!alphaString ^ !betaString ^ !gammaString)
        return null;

    var isAlphaValid = isUserInputValid(alphaString);
    var isBetaValid = isUserInputValid(betaString);
    var isGammaValid = isUserInputValid(gammaString);

    if (!isAlphaValid && !isBetaValid && !isGammaValid)
        return null;

    var alpha = isAlphaValid ? parseFloat(alphaString) : -1;
    var beta = isBetaValid ? parseFloat(betaString) : -1;
    var gamma = isGammaValid ? parseFloat(gammaString) : -1;

    return new WebInspector.UserAgentSupport.DeviceOrientation(alpha, beta, gamma);
}

WebInspector.UserAgentSupport.DeviceOrientation.clearDeviceOrientationOverride = function()
{
    PageAgent.clearDeviceOrientationOverride();
}

WebInspector.UserAgentSupport.prototype = {
    toggleUserAgentOverride: function(enabled)
    {
        if (enabled === this._userAgentOverrideEnabled)
            return;
        this._userAgentOverrideEnabled = enabled;
        this._userAgentChanged();
    },

    toggleDeviceMetricsOverride: function(enabled)
    {
        if (enabled === this._deviceMetricsOverrideEnabled)
            return;
        this._deviceMetricsOverrideEnabled = enabled;
        this._deviceMetricsChanged();
    },

    toggleGeolocationPositionOverride: function(enabled)
    {
        if (enabled === this._geolocationPositionOverrideEnabled)
            return;
        this._geolocationPositionOverrideEnabled = enabled;
        this._geolocationPositionChanged();
    },

    toggleDeviceOrientationOverride: function(enabled)
    {
        if (enabled === this._deviceOrientationOverrideEnabled)
            return;
        this._deviceOrientationOverrideEnabled = enabled;
        this._deviceOrientationChanged();
    },

    _userAgentChanged: function()
    {
        NetworkAgent.setUserAgentOverride(this._userAgentOverrideEnabled ? WebInspector.settings.userAgent.get() : "");
    },

    _deviceMetricsChanged: function()
    {
        var metrics = WebInspector.UserAgentSupport.DeviceMetrics.parseSetting(this._deviceMetricsOverrideEnabled ? WebInspector.settings.deviceMetrics.get() : "");
        if (metrics.isValid())
            PageAgent.setDeviceMetricsOverride(metrics.width, metrics.height, metrics.fontScaleFactor, WebInspector.settings.deviceFitWindow.get());
    },

    _geolocationPositionChanged: function()
    {
        if (!this._geolocationPositionOverrideEnabled) {
            PageAgent.clearGeolocationOverride();
            return;
        }
        var geolocation = WebInspector.UserAgentSupport.GeolocationPosition.parseSetting(WebInspector.settings.geolocationOverride.get());
        if (geolocation.error)
            PageAgent.setGeolocationOverride();
        else
            PageAgent.setGeolocationOverride(geolocation.latitude, geolocation.longitude, 150);
    },

    _deviceOrientationChanged: function()
    {
        if (!this._deviceOrientationOverrideEnabled) {
            PageAgent.clearDeviceOrientationOverride();
            return;
        }
        var deviceOrientation = WebInspector.UserAgentSupport.DeviceOrientation.parseSetting(WebInspector.settings.deviceOrientationOverride.get());
        PageAgent.setDeviceOrientationOverride(deviceOrientation.alpha, deviceOrientation.beta, deviceOrientation.gamma);
    }
}


/**
 * @type {WebInspector.UserAgentSupport} 
 */
WebInspector.userAgentSupport;
