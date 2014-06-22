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
 * @extends {WebInspector.View}
 */
WebInspector.OverridesView = function()
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("helpScreen.css");
    this.element.addStyleClass("fill");
    this.element.addStyleClass("help-window-main");
    this.element.addStyleClass("settings-tab-container");

    var paneContent = this.element.createChild("div", "tabbed-pane-content");

    function appendBlockTo(targetElement, contentElement)
    {
        var blockElement = targetElement.createChild("div", "help-block");
        blockElement.appendChild(contentElement);
    }

    var headerTitle = paneContent.createChild("header").createChild("h3");
    headerTitle.appendChild(document.createTextNode(WebInspector.UIString("Overrides")));

    var container = paneContent.createChild("div", "help-container-wrapper").createChild("div", "settings-tab help-content help-container");
    this.containerElement = container;
    appendBlockTo(container, this._createUserAgentControl());
    if (Capabilities.canOverrideDeviceMetrics)
        appendBlockTo(container, this._createDeviceMetricsControl());
    if (Capabilities.canOverrideGeolocation)
        appendBlockTo(container, this._createGeolocationOverrideControl());
    if (Capabilities.canOverrideDeviceOrientation)
        appendBlockTo(container, this._createDeviceOrientationOverrideControl());
    appendBlockTo(container, this._createCheckboxSetting(WebInspector.UIString("Emulate touch events"), WebInspector.settings.emulateTouchEvents));
    appendBlockTo(container, this._createMediaEmulationElement());

    this._statusElement = document.createElement("span");
    this._statusElement.textContent = WebInspector.UIString("Overrides");
}

WebInspector.OverridesView.showInDrawer = function()
{
    if (!WebInspector.OverridesView._view)
        WebInspector.OverridesView._view = new WebInspector.OverridesView();
    var view = WebInspector.OverridesView._view;
    WebInspector.showViewInDrawer(view._statusElement, view);
}

WebInspector.OverridesView.prototype = {
    /**
     * @param {boolean=} omitParagraphElement
     * @param {Element=} inputElement
     */
    _createCheckboxSetting: function(name, setting, omitParagraphElement, inputElement)
    {
        var input = inputElement || document.createElement("input");
        input.type = "checkbox";
        input.name = name;
        input.checked = setting.get();

        function listener()
        {
            setting.set(input.checked);
        }
        input.addEventListener("click", listener, false);

        var label = document.createElement("label");
        label.appendChild(input);
        label.appendChild(document.createTextNode(name));
        if (omitParagraphElement)
            return label;

        var p = document.createElement("p");
        p.appendChild(label);
        return p;
    },

    _createUserAgentControl: function()
    {
        var userAgent = WebInspector.settings.userAgent.get();

        var p = document.createElement("p");
        var labelElement = p.createChild("label");
        var checkboxElement = labelElement.createChild("input");
        checkboxElement.type = "checkbox";
        checkboxElement.checked = false;
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("User Agent")));
        p.appendChild(this._createUserAgentSelectRowElement(checkboxElement));
        return p;
    },

    _createUserAgentSelectRowElement: function(checkboxElement)
    {
        var userAgent = WebInspector.settings.userAgent.get();

        // When present, the third element lists device metrics separated by 'x':
        // - screen width,
        // - screen height,
        // - font scale factor.
        const userAgents = [
            ["Internet Explorer 9", "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)"],
            ["Internet Explorer 8", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.0; Trident/4.0)"],
            ["Internet Explorer 7", "Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.0)"],

            ["Firefox 7 \u2014 Windows", "Mozilla/5.0 (Windows NT 6.1; Intel Mac OS X 10.6; rv:7.0.1) Gecko/20100101 Firefox/7.0.1"],
            ["Firefox 7 \u2014 Mac", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv:7.0.1) Gecko/20100101 Firefox/7.0.1"],
            ["Firefox 4 \u2014 Windows", "Mozilla/5.0 (Windows NT 6.1; rv:2.0.1) Gecko/20100101 Firefox/4.0.1"],
            ["Firefox 4 \u2014 Mac", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.6; rv:2.0.1) Gecko/20100101 Firefox/4.0.1"],
            ["Firefox 14 \u2014 Android Mobile", "Mozilla/5.0 (Android; Mobile; rv:14.0) Gecko/14.0 Firefox/14.0"],
            ["Firefox 14 \u2014 Android Tablet", "Mozilla/5.0 (Android; Tablet; rv:14.0) Gecko/14.0 Firefox/14.0"],

            ["Chrome \u2014 Android Mobile", "Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 Mobile Safari/535.19"],
            ["Chrome \u2014 Android Tablet", "Mozilla/5.0 (Linux; Android 4.1.2; Nexus 7 Build/JZ054K) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166 Safari/535.19"],

            ["iPhone \u2014 iOS 5", "Mozilla/5.0 (iPhone; CPU iPhone OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3", "640x960x1"],
            ["iPhone \u2014 iOS 4", "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_2 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8H7 Safari/6533.18.5", "640x960x1"],
            ["iPad \u2014 iOS 5", "Mozilla/5.0 (iPad; CPU OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3", "1024x768x1"],
            ["iPad \u2014 iOS 4", "Mozilla/5.0 (iPad; CPU OS 4_3_2 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8H7 Safari/6533.18.5", "1024x768x1"],

            ["Android 2.3 \u2014 Nexus S", "Mozilla/5.0 (Linux; U; Android 2.3.6; en-us; Nexus S Build/GRK39F) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1", "480x800x1.1"],
            ["Android 4.0.2 \u2014 Galaxy Nexus", "Mozilla/5.0 (Linux; U; Android 4.0.2; en-us; Galaxy Nexus Build/ICL53F) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30", "720x1280x1.1"],

            ["BlackBerry \u2014 PlayBook 2.1", "Mozilla/5.0 (PlayBook; U; RIM Tablet OS 2.1.0; en-US) AppleWebKit/536.2+ (KHTML, like Gecko) Version/7.2.1.0 Safari/536.2+", "1024x600x1"],
            ["BlackBerry \u2014 9900", "Mozilla/5.0 (BlackBerry; U; BlackBerry 9900; en-US) AppleWebKit/534.11+ (KHTML, like Gecko) Version/7.0.0.187 Mobile Safari/534.11+", "640x480x1"],
            ["BlackBerry \u2014 BB10", "Mozilla/5.0 (BB10; Touch) AppleWebKit/537.1+ (KHTML, like Gecko) Version/10.0.0.1337 Mobile Safari/537.1+", "768x1280x1"],

            ["MeeGo \u2014 Nokia N9", "Mozilla/5.0 (MeeGo; NokiaN9) AppleWebKit/534.13 (KHTML, like Gecko) NokiaBrowser/8.5.0 Mobile Safari/534.13", "480x854x1"],

            [WebInspector.UIString("Other..."), "Other"]
        ];

        var fieldsetElement = document.createElement("fieldset");
        this._selectElement = fieldsetElement.createChild("select");
        this._otherUserAgentElement = fieldsetElement.createChild("input");
        this._otherUserAgentElement.type = "text";
        this._otherUserAgentElement.value = userAgent;
        this._otherUserAgentElement.title = userAgent;
        this._userAgentFieldsetElement = fieldsetElement;

        var selectionRestored = false;
        for (var i = 0; i < userAgents.length; ++i) {
            var agent = userAgents[i];
            var option = new Option(agent[0], agent[1]);
            option._metrics = agent[2] ? agent[2] : "";
            this._selectElement.add(option);
            if (userAgent === agent[1]) {
                this._selectElement.selectedIndex = i;
                selectionRestored = true;
            }
        }

        if (!selectionRestored) {
            if (!userAgent)
                this._selectElement.selectedIndex = 0;
            else
                this._selectElement.selectedIndex = userAgents.length - 1;
        }

        this._selectElement.addEventListener("change", this._selectionChanged.bind(this, true), false);

        fieldsetElement.addEventListener("dblclick", textDoubleClicked.bind(this), false);
        this._otherUserAgentElement.addEventListener("blur", textChanged.bind(this), false);

        function textDoubleClicked()
        {
            this._selectElement.selectedIndex = userAgents.length - 1;
            this._selectionChanged();
        }

        function textChanged()
        {
            WebInspector.settings.userAgent.set(this._otherUserAgentElement.value);
        }

        function checkboxClicked()
        {
            if (checkboxElement.checked) {
                this._userAgentFieldsetElement.disabled = false;
                this._selectionChanged();
            } else {
                this._userAgentFieldsetElement.disabled = true;
                this._otherUserAgentElement.disabled = true;
            }
            WebInspector.userAgentSupport.toggleUserAgentOverride(checkboxElement.checked);
        }
        checkboxElement.addEventListener("click", checkboxClicked.bind(this), false);

        checkboxClicked.call(this);
        return fieldsetElement;
    },

    /**
     * @param {boolean=} isUserGesture
     */
    _selectionChanged: function(isUserGesture)
    {
        var value = this._selectElement.options[this._selectElement.selectedIndex].value;
        if (value !== "Other") {
            WebInspector.settings.userAgent.set(value);
            this._otherUserAgentElement.value = value;
            this._otherUserAgentElement.title = value;
            this._otherUserAgentElement.disabled = true;
        } else {
            this._otherUserAgentElement.disabled = false;
            this._otherUserAgentElement.focus();
        }

        if (isUserGesture && Capabilities.canOverrideDeviceMetrics) {
            var metrics = this._selectElement.options[this._selectElement.selectedIndex]._metrics;
            this._setDeviceMetricsOverride(WebInspector.UserAgentSupport.DeviceMetrics.parseSetting(metrics), false, true);
        }
    },

    /**
     * Creates an input element under the parentElement with the given id and defaultText.
     * It also sets an onblur event listener.
     * @param {Element} parentElement
     * @param {string} id
     * @param {string} defaultText
     * @param {function(*)} eventListener
     * @param {boolean=} numeric
     * @return {Element} element
     */
    _createInput: function(parentElement, id, defaultText, eventListener, numeric)
    {
        var element = parentElement.createChild("input");
        element.id = id;
        element.type = "text";
        element.maxLength = 12;
        element.style.width = "80px";
        element.value = defaultText;
        element.align = "right";
        if (numeric)
            element.className = "numeric";
        element.addEventListener("blur", eventListener, false);
        return element;
    },

    _createDeviceMetricsControl: function()
    {
        const metricsSetting = WebInspector.settings.deviceMetrics.get();
        var metrics = WebInspector.UserAgentSupport.DeviceMetrics.parseSetting(metricsSetting);

        const p = document.createElement("p");
        const labelElement = p.createChild("label");
        const checkboxElement = labelElement.createChild("input");
        checkboxElement.id = "metrics-override-checkbox";
        checkboxElement.type = "checkbox";
        checkboxElement.checked = false;
        checkboxElement.addEventListener("click", this._onMetricsCheckboxClicked.bind(this), false);
        this._metricsCheckboxElement = checkboxElement;
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("Device metrics")));

        const metricsSectionElement = this._createDeviceMetricsElement(metrics);
        p.appendChild(metricsSectionElement);
        this._metricsSectionElement = metricsSectionElement;
        this._onMetricsCheckboxClicked();

        return p;
    },

    _onMetricsCheckboxClicked: function()
    {
        var controlsDisabled = !this._metricsCheckboxElement.checked;
        this._deviceMetricsFieldsetElement.disabled = controlsDisabled;

        if (controlsDisabled) {
            WebInspector.userAgentSupport.toggleDeviceMetricsOverride(false);
            return;
        }

        var metrics = WebInspector.UserAgentSupport.DeviceMetrics.parseUserInput(this._widthOverrideElement.value, this._heightOverrideElement.value, this._fontScaleFactorOverrideElement.value);
        if (metrics && metrics.isValid() && metrics.width && metrics.height) {
            this._setDeviceMetricsOverride(metrics, false, false);
            WebInspector.userAgentSupport.toggleDeviceMetricsOverride(true);
        }
        if (!this._widthOverrideElement.value)
            this._widthOverrideElement.focus();
    },

    _applyDeviceMetricsUserInput: function()
    {
        this._setDeviceMetricsOverride(WebInspector.UserAgentSupport.DeviceMetrics.parseUserInput(this._widthOverrideElement.value.trim(), this._heightOverrideElement.value.trim(), this._fontScaleFactorOverrideElement.value.trim()), true, false);
    },

    /**
     * @param {?WebInspector.UserAgentSupport.DeviceMetrics} metrics
     * @param {boolean} userInputModified
     */
    _setDeviceMetricsOverride: function(metrics, userInputModified, updateCheckbox)
    {
        function setValid(condition, element)
        {
            if (condition)
                element.removeStyleClass("error-input");
            else
                element.addStyleClass("error-input");
        }

        setValid(metrics && metrics.isWidthValid(), this._widthOverrideElement);
        setValid(metrics && metrics.isHeightValid(), this._heightOverrideElement);
        setValid(metrics && metrics.isFontScaleFactorValid(), this._fontScaleFactorOverrideElement);

        if (!metrics)
            return;

        if (!userInputModified) {
            this._widthOverrideElement.value = metrics.widthToInput();
            this._heightOverrideElement.value = metrics.heightToInput();
            this._fontScaleFactorOverrideElement.value = metrics.fontScaleFactorToInput();
        }

        if (metrics.isValid()) {
            var value = metrics.toSetting();
            if (value !== WebInspector.settings.deviceMetrics.get())
                WebInspector.settings.deviceMetrics.set(value);
        }

        if (this._metricsCheckboxElement && updateCheckbox) {
            this._metricsCheckboxElement.checked = !!metrics.toSetting();
            this._onMetricsCheckboxClicked();
        }
    },

    /**
     * @param {WebInspector.UserAgentSupport.DeviceMetrics} metrics
     */
    _createDeviceMetricsElement: function(metrics)
    {
        var fieldsetElement = document.createElement("fieldset");
        fieldsetElement.id = "metrics-override-section";
        this._deviceMetricsFieldsetElement = fieldsetElement;

        function swapDimensionsClicked(event)
        {
            var widthValue = this._widthOverrideElement.value;
            this._widthOverrideElement.value = this._heightOverrideElement.value;
            this._heightOverrideElement.value = widthValue;
            this._applyDeviceMetricsUserInput();
        }

        var tableElement = fieldsetElement.createChild("table", "nowrap");

        var rowElement = tableElement.createChild("tr");
        var cellElement = rowElement.createChild("td");
        cellElement.appendChild(document.createTextNode(WebInspector.UIString("Screen resolution:")));
        cellElement = rowElement.createChild("td");
        this._widthOverrideElement = this._createInput(cellElement, "metrics-override-width", String(metrics.width || screen.width), this._applyDeviceMetricsUserInput.bind(this), true);
        cellElement.appendChild(document.createTextNode(" \u00D7 ")); // MULTIPLICATION SIGN.
        this._heightOverrideElement = this._createInput(cellElement, "metrics-override-height", String(metrics.height || screen.height), this._applyDeviceMetricsUserInput.bind(this), true);
        cellElement.appendChild(document.createTextNode(" \u2014 ")); // EM DASH.
        this._swapDimensionsElement = cellElement.createChild("button");
        this._swapDimensionsElement.appendChild(document.createTextNode(" \u21C4 ")); // RIGHTWARDS ARROW OVER LEFTWARDS ARROW.
        this._swapDimensionsElement.title = WebInspector.UIString("Swap dimensions");
        this._swapDimensionsElement.addEventListener("click", swapDimensionsClicked.bind(this), false);

        rowElement = tableElement.createChild("tr");
        cellElement = rowElement.createChild("td");
        cellElement.appendChild(document.createTextNode(WebInspector.UIString("Font scale factor:")));
        cellElement = rowElement.createChild("td");
        this._fontScaleFactorOverrideElement = this._createInput(cellElement, "metrics-override-font-scale", String(metrics.fontScaleFactor || 1), this._applyDeviceMetricsUserInput.bind(this), true);

        rowElement = tableElement.createChild("tr");
        cellElement = rowElement.createChild("td");
        cellElement.colSpan = 2;
        this._fitWindowCheckboxElement = document.createElement("input");
        cellElement.appendChild(this._createCheckboxSetting(WebInspector.UIString("Fit in window"), WebInspector.settings.deviceFitWindow, true, this._fitWindowCheckboxElement));

        return fieldsetElement;
    },

    _createGeolocationOverrideControl: function()
    {
        const geolocationSetting = WebInspector.settings.geolocationOverride.get();
        var geolocation = WebInspector.UserAgentSupport.GeolocationPosition.parseSetting(geolocationSetting);
        var p = document.createElement("p");
        var labelElement = p.createChild("label");
        var checkboxElement = labelElement.createChild("input");
        checkboxElement.id = "geolocation-override-checkbox";
        checkboxElement.type = "checkbox";
        checkboxElement.checked = false;
        checkboxElement.addEventListener("click", this._onGeolocationOverrideCheckboxClicked.bind(this), false);
        this._geolocationOverrideCheckboxElement = checkboxElement;
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("Override Geolocation")));

        var geolocationSectionElement = this._createGeolocationOverrideElement(geolocation);
        p.appendChild(geolocationSectionElement);
        this._geolocationSectionElement = geolocationSectionElement;
        this._onGeolocationOverrideCheckboxClicked();
        return p;
    },

    _onGeolocationOverrideCheckboxClicked: function()
    {
        var controlsDisabled = !this._geolocationOverrideCheckboxElement.checked;
        this._geolocationFieldsetElement.disabled = controlsDisabled;

        if (controlsDisabled) {
            WebInspector.userAgentSupport.toggleGeolocationPositionOverride(false);
            return;
        }

        var geolocation = WebInspector.UserAgentSupport.GeolocationPosition.parseUserInput(this._latitudeElement.value, this._longitudeElement.value, this._geolocationErrorElement.checked);
        if (geolocation) {
            this._setGeolocationPosition(geolocation, false, false);
            WebInspector.userAgentSupport.toggleGeolocationPositionOverride(true);
        }
        if (!this._latitudeElement.value)
            this._latitudeElement.focus();
    },

    _applyGeolocationUserInput: function()
    {
        this._setGeolocationPosition(WebInspector.UserAgentSupport.GeolocationPosition.parseUserInput(this._latitudeElement.value.trim(), this._longitudeElement.value.trim(), this._geolocationErrorElement.checked), true, false);
    },

    /**
     * @param {?WebInspector.UserAgentSupport.GeolocationPosition} geolocation
     * @param {boolean} userInputModified
     * @param {boolean} updateCheckbox
     */
    _setGeolocationPosition: function(geolocation, userInputModified, updateCheckbox)
    {
        if (!geolocation)
            return;

        if (!userInputModified) {
            this._latitudeElement.value = geolocation.latitude;
            this._longitudeElement.value = geolocation.longitude;
        }

        var value = geolocation.toSetting();
        WebInspector.settings.geolocationOverride.set(value);

        if (this._geolocationOverrideCheckboxElement && updateCheckbox) {
            this._geolocationOverrideCheckboxElement.checked = !!geolocation.toSetting();
            this._onGeolocationOverrideCheckboxClicked();
        }
    },

    /**
     * @param {WebInspector.UserAgentSupport.GeolocationPosition} geolocation
     */
    _createGeolocationOverrideElement: function(geolocation)
    {
        var fieldsetElement = document.createElement("fieldset");
        fieldsetElement.id = "geolocation-override-section";
        this._geolocationFieldsetElement = fieldsetElement;

        var tableElement = fieldsetElement.createChild("table");
        var rowElement = tableElement.createChild("tr");
        var cellElement = rowElement.createChild("td");
        cellElement.appendChild(document.createTextNode(WebInspector.UIString("Geolocation Position") + ":"));
        cellElement = rowElement.createChild("td");
        cellElement.appendChild(document.createTextNode(WebInspector.UIString("Lat = ")));
        this._latitudeElement = this._createInput(cellElement, "geolocation-override-latitude", String(geolocation.latitude), this._applyGeolocationUserInput.bind(this), true);
        cellElement.appendChild(document.createTextNode(" , "));
        cellElement.appendChild(document.createTextNode(WebInspector.UIString("Lon = ")));
        this._longitudeElement = this._createInput(cellElement, "geolocation-override-longitude", String(geolocation.longitude), this._applyGeolocationUserInput.bind(this), true);
        rowElement = tableElement.createChild("tr");
        cellElement = rowElement.createChild("td");
        cellElement.colSpan = 2;
        var geolocationErrorLabelElement = document.createElement("label");
        var geolocationErrorCheckboxElement = geolocationErrorLabelElement.createChild("input");
        geolocationErrorCheckboxElement.id = "geolocation-error";
        geolocationErrorCheckboxElement.type = "checkbox";
        geolocationErrorCheckboxElement.checked = !geolocation || geolocation.error;
        geolocationErrorCheckboxElement.addEventListener("click", this._applyGeolocationUserInput.bind(this), false);
        geolocationErrorLabelElement.appendChild(document.createTextNode(WebInspector.UIString("Emulate position unavailable")));
        this._geolocationErrorElement = geolocationErrorCheckboxElement;
        cellElement.appendChild(geolocationErrorLabelElement);

        return fieldsetElement;
    },

    _createDeviceOrientationOverrideControl: function()
    {
        const deviceOrientationSetting = WebInspector.settings.deviceOrientationOverride.get();
        var deviceOrientation = WebInspector.UserAgentSupport.DeviceOrientation.parseSetting(deviceOrientationSetting);

        var p = document.createElement("p");
        var labelElement = p.createChild("label");
        var checkboxElement = labelElement.createChild("input");
        checkboxElement.id = "device-orientation-override-checkbox";
        checkboxElement.type = "checkbox";
        checkboxElement.checked = false;
        checkboxElement.addEventListener("click", this._onDeviceOrientationOverrideCheckboxClicked.bind(this), false);
        this._deviceOrientationOverrideCheckboxElement = checkboxElement;
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("Override Device Orientation")));

        var deviceOrientationSectionElement = this._createDeviceOrientationOverrideElement(deviceOrientation);
        p.appendChild(deviceOrientationSectionElement);
        this._deviceOrientationSectionElement = deviceOrientationSectionElement;
        this._onDeviceOrientationOverrideCheckboxClicked();
        return p;
    },

    _onDeviceOrientationOverrideCheckboxClicked: function()
    {
        var controlsDisabled = !this._deviceOrientationOverrideCheckboxElement.checked;
        this._deviceOrientationFieldsetElement.disabled = controlsDisabled;

        if (controlsDisabled) {
            WebInspector.userAgentSupport.toggleDeviceOrientationOverride(false);
            return;
        }

        var deviceOrientation = WebInspector.UserAgentSupport.DeviceOrientation.parseUserInput(this._alphaElement.value, this._betaElement.value, this._gammaElement.value);
        if (deviceOrientation) {
            this._setDeviceOrientation(deviceOrientation, false, false);
            WebInspector.userAgentSupport.toggleDeviceOrientationOverride(true);
        }
        if (!this._alphaElement.value)
            this._alphaElement.focus();
    },

    _applyDeviceOrientationUserInput: function()
    {
        this._setDeviceOrientation(WebInspector.UserAgentSupport.DeviceOrientation.parseUserInput(this._alphaElement.value.trim(), this._betaElement.value.trim(), this._gammaElement.value.trim()), true, false);
    },

    /**
     * @param {?WebInspector.UserAgentSupport.DeviceOrientation} deviceOrientation
     * @param {boolean} userInputModified
     * @param {boolean} updateCheckbox
     */
    _setDeviceOrientation: function(deviceOrientation, userInputModified, updateCheckbox)
    {
        if (!deviceOrientation)
            return;

        if (!userInputModified) {
            this._alphaElement.value = deviceOrientation.alpha;
            this._betaElement.value = deviceOrientation.beta;
            this._gammaElement.value = deviceOrientation.gamma;
        }

        var value = deviceOrientation.toSetting();
        WebInspector.settings.deviceOrientationOverride.set(value);

        if (this._deviceOrientationOverrideCheckboxElement && updateCheckbox) {
            this._deviceOrientationOverrideCheckboxElement.checked = !!deviceOrientation.toSetting();
            this._onDeviceOrientationOverrideCheckboxClicked();
        }
    },

    /**
     * @param {WebInspector.UserAgentSupport.DeviceOrientation} deviceOrientation
     */
    _createDeviceOrientationOverrideElement: function(deviceOrientation)
    {
        var fieldsetElement = document.createElement("fieldset");
        fieldsetElement.id = "device-orientation-override-section";
        this._deviceOrientationFieldsetElement = fieldsetElement;

        var tableElement = fieldsetElement.createChild("table");

        var rowElement = tableElement.createChild("tr");
        var cellElement = rowElement.createChild("td");
        cellElement.appendChild(document.createTextNode("\u03B1: "));
        this._alphaElement = this._createInput(cellElement, "device-orientation-override-alpha", String(deviceOrientation.alpha), this._applyDeviceOrientationUserInput.bind(this), true);
        cellElement.appendChild(document.createTextNode(" \u03B2: "));
        this._betaElement = this._createInput(cellElement, "device-orientation-override-beta", String(deviceOrientation.beta), this._applyDeviceOrientationUserInput.bind(this), true);
        cellElement.appendChild(document.createTextNode(" \u03B3: "));
        this._gammaElement = this._createInput(cellElement, "device-orientation-override-gamma", String(deviceOrientation.gamma), this._applyDeviceOrientationUserInput.bind(this), true);

        return fieldsetElement;
    },

    _createMediaEmulationElement: function()
    {
        const p = document.createElement("p");
        const labelElement = p.createChild("label");
        const checkboxElement = labelElement.createChild("input");
        checkboxElement.type = "checkbox";
        checkboxElement.checked = false;
        labelElement.appendChild(document.createTextNode(WebInspector.UIString("Emulate CSS media")));

        var mediaSelectElement = p.createChild("select");
        var mediaTypes = WebInspector.CSSStyleModel.MediaTypes;
        var defaultMedia = WebInspector.settings.emulatedCSSMedia.get();
        for (var i = 0; i < mediaTypes.length; ++i) {
            var mediaType = mediaTypes[i];
            if (mediaType === "all") {
                // "all" is not a device-specific media type.
                continue;
            }
            var option = document.createElement("option");
            option.text = mediaType;
            option.value = mediaType;
            mediaSelectElement.add(option);
            if (mediaType === defaultMedia)
                mediaSelectElement.selectedIndex = mediaSelectElement.options.length - 1;
        }
        mediaSelectElement.disabled = true;
        var boundListener = this._emulateMediaChanged.bind(this, checkboxElement, mediaSelectElement);
        checkboxElement.addEventListener("click", boundListener, false);
        mediaSelectElement.addEventListener("change", boundListener, false);
        return p;
    },

    _emulateMediaChanged: function(checkbox, select)
    {
        select.disabled = !checkbox.checked;
        if (checkbox.checked) {
            var media = select.options[select.selectedIndex].value;
            WebInspector.settings.emulatedCSSMedia.set(media);
            PageAgent.setEmulatedMedia(media);
        } else
            PageAgent.setEmulatedMedia("");
        WebInspector.cssModel.mediaQueryResultChanged();
    },

    __proto__: WebInspector.View.prototype
}
