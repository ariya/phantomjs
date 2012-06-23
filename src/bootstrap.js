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


(function() {
    window.global = window;

    function nativeRequire(name) {
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

    function dirname(path) {
        return path.replace(/\/[^\/]*\/?$/, '');
    };

    function basename(path) {
        return path.replace(/.*\//, '');
    };

    function joinPath() {
        var args = Array.prototype.slice.call(arguments);
        return args.join(fs.separator);
    };

    var fs = nativeRequire('fs');
    var phantomModules = ['fs', 'webpage', 'webserver', 'system'];

    var rootPath = fs.absolute(phantom.libraryPath);
    var mainScript = joinPath(rootPath, basename(nativeRequire('system').args[0]) || 'repl');
    var sourceIds = {};

    var cache = {};
    var extensions = {
        '.js': function(module, filename) {
            var code = fs.read(filename);
            module._compile(code);
        },

        '.coffee': function(module, filename) {
            var code = fs.read(filename);
            var CoffeeScript = require('_coffee-script');
            try {
                code = CoffeeScript.compile(code);
            } catch (e) {
                e.fileName = filename;
                throw e;
            }
            module._compile(code);
        },

        '.json': function(module, filename) {
            module.exports = JSON.parse(fs.read(filename));
        }
    };

    function tryFile(path) {
        if (fs.isFile(path)) return path;
        return null;
    }

    function tryExtensions(path) {
        var filename;
        var exts = Object.keys(extensions);
        for (var i=0; i<exts.length; ++i) {
            filename = tryFile(path + exts[i]);
            if (filename) return filename;
        }
        return null;
    }

    function tryPackage(path) {
        var filename, package, packageFile = joinPath(path, 'package.json');
        if (fs.isFile(packageFile)) {
            package = JSON.parse(fs.read(packageFile));
            if (!package || !package.main) return null;

            filename = fs.absolute(joinPath(path, package.main));

            return tryFile(filename) || tryExtensions(filename) ||
                tryExtensions(joinPath(filename, 'index'));
        }
        return null;
    }

    function Module(filename, stubs) {
        this.id = this.filename = filename;
        this.dirname = dirname(filename);
        this.exports = {};
        this.stubs = {};
        for (var name in stubs) {
            this.stubs[name] = stubs[name];
        }
    }

    Module.prototype._getPaths = function(request) {
        var paths = [], dir;

        if (request[0] === '.') {
            paths.push(fs.absolute(joinPath(this.dirname, request)));
        } else if (request[0] === '/') {
            paths.push(fs.absolute(request));
        } else {
            dir = this.dirname;
            while (dir !== '') {
                paths.push(joinPath(dir, 'node_modules', request));
                dir = dirname(dir);
            }
            //paths.push(joinPath(nodifyPath, 'modules', request));
        }

        return paths;
    };

    Module.prototype._getFilename = function(request) {
        var path, filename = null, paths = this._getPaths(request);

        for (var i=0; i<paths.length && !filename; ++i) {
            path = paths[i];
            filename = tryFile(path) || tryExtensions(path) || tryPackage(path) ||
                tryExtensions(joinPath(path, 'index'));
        }

        return filename;
    };

    Module.prototype._getRequire = function() {
        var self = this;

        function require(request) {
            return self.require(request);
        }
        require.cache = cache;
        require.stub = function(request, exports) {
            self.stubs[request] = { exports: exports };
        };

        return require;
    };

    Module.prototype._load = function() {
        var ext = this.filename.match(/\.[^.]+$/);
        if (!ext) ext = '.js';
        extensions[ext](this, this.filename);
    };

    Module.prototype._compile = function(code) {
        // a trick to associate Error's sourceId with file
        code += ";throw new Error('__sourceId__');";
        try {
            var fn = new Function('require', 'exports', 'module', code);
            fn(this._getRequire(), this.exports, this);
        } catch (e) {
            // assign source id (check if already assigned to avoid reassigning
            // on exceptions propagated from other files)
            if (!sourceIds.hasOwnProperty(e.sourceId)) {
                sourceIds[e.sourceId] = this.filename;
            }
            // if it's not the error we added, propagate it
            if (e.message !== '__sourceId__') {
                throw e;
            }
        }
    };

    Module.prototype.require = function(request) {
        if (phantomModules.indexOf(request) !== -1) {
            return nativeRequire(request);
        }

        if (this.stubs.hasOwnProperty(request)) {
            return this.stubs[request].exports;
        }

        var filename = this._getFilename(request);
        if (!filename) {
            var e = new Error("Cannot find module '" + request + "'");
            e.fileName = this.filename;
            e.line = '';
            throw e;
        }

        if (cache.hasOwnProperty(filename)) {
            return cache[filename].exports;
        }

        var module = new Module(filename, this.stubs);
        cache[filename] = module;
        module._load();

        return module.exports;
    };

    window.require = new Module(mainScript)._getRequire();
}());

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
