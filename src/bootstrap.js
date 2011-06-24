/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

// This allows creating a new web page using the construct "new WebPage",
// which feels more natural than "phantom.createWebPage()".
window.WebPage = function() {
    var page = phantom.createWebPage(),
        handlers = {};

    function defineSetter(handlerName, signalName) {
        page.__defineSetter__(handlerName, function(f) {
            if (handlers && typeof handlers[signalName] === 'function') {
                try {
                    this[signalName].disconnect(handlers[signalName]);
                } catch (e) {}
            }
            handlers[signalName] = f;
            this[signalName].connect(handlers[signalName]);
        });
    }

    // deep copy
    page.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    defineSetter("onLoadStarted", "loadStarted");

    defineSetter("onLoadFinished", "loadFinished");

    defineSetter("onResourceRequested", "resourceRequested");

    defineSetter("onResourceReceived", "resourceReceived");

    defineSetter("onAlert", "javaScriptAlertSent");

    defineSetter("onConsoleMessage", "javaScriptConsoleMessageSent");

    page.open = function () {
        if (arguments.length === 1) {
            this.openUrl(arguments[0], 'get', this.settings);
            return;
        }
        if (arguments.length === 2) {
            this.onLoadFinished = arguments[1];
            this.openUrl(arguments[0], 'get', this.settings);
            return;
        } else if (arguments.length === 3) {
            this.onLoadFinished = arguments[2];
            this.openUrl(arguments[0], arguments[1], this.settings);
            return;
        } else if (arguments.length === 4) {
            this.onLoadFinished = arguments[3];
            this.openUrl(arguments[0], {
                operation: arguments[1],
                data: arguments[2]
                }, this.settings);
            return;
        }
        throw "Wrong use of WebPage#open";
    };

    page.includeJs = function(scriptUrl, onScriptLoaded) {
        // Register temporary signal handler for 'alert()'
        this.javaScriptAlertSent.connect(function(msgFromAlert) {
            if ( msgFromAlert === scriptUrl ) {
                // Resource loaded, time to fire the callback
                onScriptLoaded(scriptUrl);
                // And disconnect the signal handler
                try {
                    this.javaScriptAlertSent.disconnect(arguments.callee);
                } catch (e) {}
            }
        });

        // Append the script tag to the body
        this._appendScriptElement(scriptUrl);
    };

    return page;
};
