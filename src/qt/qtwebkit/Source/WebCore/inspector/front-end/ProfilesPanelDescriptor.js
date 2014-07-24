/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.PanelDescriptor}
 */
WebInspector.ProfilesPanelDescriptor = function()
{
    WebInspector.PanelDescriptor.call(this, "profiles", WebInspector.UIString("Profiles"), "ProfilesPanel", "ProfilesPanel.js");
}

WebInspector.ProfilesPanelDescriptor.prototype = {
    __proto__: WebInspector.PanelDescriptor.prototype
}

WebInspector.ProfilesPanelDescriptor.ProfileURLRegExp = /webkit-profile:\/\/(.+)\/(.+)#([0-9]+)/;

WebInspector.ProfilesPanelDescriptor.UserInitiatedProfileName = "org.webkit.profiles.user-initiated";

/**
 * @param {string} title
 * @return {boolean}
 */
WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile = function(title)
{
    return title.startsWith(WebInspector.ProfilesPanelDescriptor.UserInitiatedProfileName);
}

/**
 * @param {string} title
 * @return {number}
 * @throws {string}
 */
WebInspector.ProfilesPanelDescriptor.userInitiatedProfileIndex = function(title)
{
    if (!WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile(title))
        throw "Not user-initiated profile title.";
    var suffix = title.substring(WebInspector.ProfilesPanelDescriptor.UserInitiatedProfileName.length + 1);
    return parseInt(suffix, 10);
}

/**
 * @param {string} title
 * @return {string}
 */
WebInspector.ProfilesPanelDescriptor.resolveProfileTitle = function(title)
{
    if (!WebInspector.ProfilesPanelDescriptor.isUserInitiatedProfile(title))
        return title;
    return WebInspector.UIString("Profile %d", WebInspector.ProfilesPanelDescriptor.userInitiatedProfileIndex(title));
}
