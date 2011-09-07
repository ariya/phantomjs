/* jslint sloppy: true, nomen: true */
/* global phantom:true, exports:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com

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

// Builtin Module: 'webpage'

/**
 * Create a new 'webpage'.
 *
 * @param opts Options object to initialize the page
 * @returns A newly created webpage
 */
exports.create = function(opts) {
    var newpage = phantom.createWebPage(),
        handlers = {};

    function checkType(o, type) {
        return typeof o === type;
    }

    function isObject(o) {
        return checkType(o, 'object');
    }

    function isUndefined(o) {
        return checkType(o, 'undefined');
    }

    function isUndefinedOrNull(o) {
        return isUndefined(o) || null === o;
    }

    function copyInto(target, source) {
        if (target === source || isUndefinedOrNull(source)) {
            return target;
        }

        target = target || {};

        // Copy into objects only
        if (isObject(target)) {
            // Make sure source exists
            source = source || {};

            if (isObject(source)) {
                var i, newTarget, newSource;
                for (i in source) {
                    if (source.hasOwnProperty(i)) {
                        newTarget = target[i];
                        newSource = source[i];

                        if (newTarget && isObject(newSource)) {
                            // Deep copy
                            newTarget = copyInto(target[i], newSource);
                        } else {
                            newTarget = newSource;
                        }

                        if (!isUndefined(newTarget)) {
                            target[i] = newTarget;
                        }
                    }
                }
            } else {
                target = source;
            }
        }

        return target;
    }

    function defineSetter(handlerName, signalName) {
        newpage.__defineSetter__(handlerName, function (f) {
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
    newpage.settings = JSON.parse(JSON.stringify(phantom.defaultPageSettings));

    defineSetter("onInitialized", "initialized");

    defineSetter("onLoadStarted", "loadStarted");

    defineSetter("onLoadFinished", "loadFinished");

    defineSetter("onResourceRequested", "resourceRequested");

    defineSetter("onResourceReceived", "resourceReceived");

    defineSetter("onAlert", "javaScriptAlertSent");

    defineSetter("onConsoleMessage", "javaScriptConsoleMessageSent");

    /**
     * Open a URL.
     * Can be invoked in one of the following ways:
     * - open(url)
     * - open(url, [on load callback])
     * - open(url, [operation])
     * - open(url, [operation], [on load callback])
     * - open(url, [operation], [data])
     * - open(url, [operation], [data], [on load callback])
     *
     * @param url URL to open 
     */
    newpage.open = function (url, arg1, arg2, arg3) {
        if (arguments.length === 1) {
            newpage.openUrl(url, 'get', newpage.settings);
            return;
        }
        if (arguments.length === 2 && typeof arg1 === 'function') {
            newpage.onLoadFinished = arg1;
            newpage.openUrl(url, 'get', newpage.settings);
            return;
        } else if (arguments.length === 2) {
            newpage.openUrl(url, arg1, newpage.settings);
            return;
        } else if (arguments.length === 3 && typeof arg2 === 'function') {
            newpage.onLoadFinished = arg2;
            newpage.openUrl(url, arg1, newpage.settings);
            return;
        } else if (arguments.length === 3) {
            newpage.openUrl(url, {
                operation: arg1,
                data: arg2
            }, newpage.settings);
            return;
        } else if (arguments.length === 4) {
            newpage.onLoadFinished = arg3;
            newpage.openUrl(url, {
                operation: arg1,
                data: arg2
            }, newpage.settings);
            return;
        }
        throw "Wrong use of WebPage#open";
    };
    
    /**
     * Include JavaScript in the page as '<script>' tag.
     *
     * @param scriptUrl URL to the Script to include
     * @param onScriptLoaded Callback for when the Script is done loading
     */
    newpage.includeJs = function (scriptUrl, onScriptLoaded) {
        // Register temporary signal handler for 'alert()'
        newpage.javaScriptAlertSent.connect(function (msgFromAlert) {
            if (msgFromAlert === scriptUrl) {
                // Resource loaded, time to fire the callback
                onScriptLoaded(scriptUrl);
                // And disconnect the signal handler
                try {
                    newpage.javaScriptAlertSent.disconnect(arguments.callee);
                } catch (e) {}
            }
        });

        // Append the script tag to the body
        newpage._appendScriptElement(scriptUrl);
    };

    // Initialise the new 'webpage' with provided 'opts' object
    if (opts) {
        newpage = copyInto(newpage, opts);
    }
    
    return newpage;
};

