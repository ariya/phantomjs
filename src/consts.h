/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CONSTS_H
#define CONSTS_H

#define PHANTOMJS_VERSION_MAJOR     1
#define PHANTOMJS_VERSION_MINOR     9
#define PHANTOMJS_VERSION_PATCH     8
#define PHANTOMJS_VERSION_STRING    "1.9.8"

#define COFFEE_SCRIPT_EXTENSION     ".coffee"

#define JS_ELEMENT_CLICK "(function (el) { " \
        "var ev = document.createEvent('MouseEvents');" \
        "ev.initEvent(\"click\", true, true);" \
        "el.dispatchEvent(ev);" \
    "})(this);"

#define JS_APPEND_SCRIPT_ELEMENT "var el = document.createElement('script');" \
    "el.onload = function() { alert('%1'); };" \
    "el.src = '%1';" \
    "document.body.appendChild(el);"

#define PAGE_SETTINGS_LOAD_IMAGES           "loadImages"
#define PAGE_SETTINGS_JS_ENABLED            "javascriptEnabled"
#define PAGE_SETTINGS_XSS_AUDITING          "XSSAuditingEnabled"
#define PAGE_SETTINGS_USER_AGENT            "userAgent"
#define PAGE_SETTINGS_LOCAL_ACCESS_REMOTE   "localToRemoteUrlAccessEnabled"
#define PAGE_SETTINGS_USERNAME              "userName"
#define PAGE_SETTINGS_PASSWORD              "password"
#define PAGE_SETTINGS_MAX_AUTH_ATTEMPTS     "maxAuthAttempts"
#define PAGE_SETTINGS_RESOURCE_TIMEOUT      "resourceTimeout"
#define PAGE_SETTINGS_WEB_SECURITY_ENABLED  "webSecurityEnabled"
#define PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS   "javascriptCanOpenWindows"
#define PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS  "javascriptCanCloseWindows"

#define DEFAULT_WEBDRIVER_CONFIG            "127.0.0.1:8910"

#endif // CONSTS_H
