/*jslint sloppy: true, nomen: true */
/*global window:true,phantom:true */

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

function require(name) {
    var code, func, exports;

    if (name === 'webpage' || name === 'fs' || name === 'webserver' || name === 'system') {
        code = phantom.loadModuleSource(name);
        func = new Function("exports", "window", code);
        exports = {};
        if (name === 'fs') {
            exports = phantom.createFilesystem();
        } else if (name === 'system') {
            exports = phantom.createSystem();
        }
        func.call({}, exports, {});
        return exports;
    }

    if (typeof exports === 'undefined') {
        throw 'Unknown module ' + name + ' for require()';
    }
}

phantom.__defineErrorSetter__ = function(obj, page) {
    var handler;
    var signal = page.javaScriptErrorSent;

    obj.__defineSetter__('onError', function(f) {
        if (handler && typeof handler === 'function') {
            try { signal.disconnect(handler); }
            catch (e) {}
        }

        if (typeof f === 'function') {
            handler = function(message, stack) {
              stack = JSON.parse(stack).map(function(item) {
                  return { file: item.url, line: item.lineNumber, function: item.functionName }
              });

              f(message, stack);
            };
            signal.connect(handler);
        } else {
            handler = null;
        }
    });
};

phantom.__defineErrorSetter__(phantom, phantom.page);

// TODO: Make this output to STDERR
phantom.defaultErrorHandler = function(message, stack) {
    console.log(message + "\n");

    stack.forEach(function(item) {
        var message = item.file + ":" + item.line;
        if (item.function)
            message += " in " + item.function;
        console.log("  " + message);
    });
};

phantom.callback = function(callback) {
    var ret = phantom.createCallback();
    ret.called.connect(function(args) {
        var retVal = callback.apply(this, args);
        ret.returnValue = retVal;
    });
    return ret;
};

phantom.onError = phantom.defaultErrorHandler;

// Legacy way to use WebPage
window.WebPage = require('webpage').create;
