/*jslint sloppy: true, nomen: true */
/*global exports:true,phantom:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com
  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

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

exports.create = function (opts) {
    var server = phantom.createWebServer(),
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
        server.__defineSetter__(handlerName, function (f) {
            if (handlers && typeof handlers[signalName] === 'function') {
                try {
                    this[signalName].disconnect(handlers[signalName]);
                } catch (e) {}
            }
            handlers[signalName] = f;
            this[signalName].connect(handlers[signalName]);
        });
    }

    defineSetter("onNewRequest", "newRequest");

    server.listen = function (port, arg1, arg2) {
        if (arguments.length === 2 && typeof arg1 === 'function') {
            this.onNewRequest = arg1;
            return this.listenOnPort(port, {});
        }
        if (arguments.length === 3 && typeof arg2 === 'function') {
            this.onNewRequest = arg2;
            // arg1 == settings
            return this.listenOnPort(port, arg1);
        }
        throw "Wrong use of WebServer#listen";
    };

    // Copy options into server
    if (opts) {
        server = copyInto(server, opts);
    }

    return server;
};
