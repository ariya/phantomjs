// Copyright 2006 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Bootstrap for the Google JS Library (Closure).
 *
 * In uncompiled mode base.js will write out Closure's deps file, unless the
 * global <code>CLOSURE_NO_DEPS</code> is set to true.  This allows projects to
 * include their own deps file(s) from different locations.
 *
 *
 * @provideGoog
 */


/**
 * @define {boolean} Overridden to true by the compiler when --closure_pass
 *     or --mark_as_compiled is specified.
 */
var COMPILED = false;


/**
 * Base namespace for the Closure library.  Checks to see goog is
 * already defined in the current scope before assigning to prevent
 * clobbering if base.js is loaded more than once.
 *
 * @const
 */
var goog = goog || {}; // Identifies this file as the Closure base.


/**
 * Reference to the global context.  In most cases this will be 'window'.
 */
goog.global = this;


/**
 * @define {boolean} DEBUG is provided as a convenience so that debugging code
 * that should not be included in a production js_binary can be easily stripped
 * by specifying --define goog.DEBUG=false to the JSCompiler. For example, most
 * toString() methods should be declared inside an "if (goog.DEBUG)" conditional
 * because they are generally used for debugging purposes and it is difficult
 * for the JSCompiler to statically determine whether they are used.
 */
goog.DEBUG = true;


/**
 * @define {string} LOCALE defines the locale being used for compilation. It is
 * used to select locale specific data to be compiled in js binary. BUILD rule
 * can specify this value by "--define goog.LOCALE=<locale_name>" as JSCompiler
 * option.
 *
 * Take into account that the locale code format is important. You should use
 * the canonical Unicode format with hyphen as a delimiter. Language must be
 * lowercase, Language Script - Capitalized, Region - UPPERCASE.
 * There are few examples: pt-BR, en, en-US, sr-Latin-BO, zh-Hans-CN.
 *
 * See more info about locale codes here:
 * http://www.unicode.org/reports/tr35/#Unicode_Language_and_Locale_Identifiers
 *
 * For language codes you should use values defined by ISO 693-1. See it here
 * http://www.w3.org/WAI/ER/IG/ert/iso639.htm. There is only one exception from
 * this rule: the Hebrew language. For legacy reasons the old code (iw) should
 * be used instead of the new code (he), see http://wiki/Main/IIISynonyms.
 */
goog.LOCALE = 'en';  // default to en


/**
 * Creates object stubs for a namespace.  The presence of one or more
 * goog.provide() calls indicate that the file defines the given
 * objects/namespaces.  Build tools also scan for provide/require statements
 * to discern dependencies, build dependency files (see deps.js), etc.
 * @see goog.require
 * @param {string} name Namespace provided by this file in the form
 *     "goog.package.part".
 */
goog.provide = function(name) {
  if (!COMPILED) {
    // Ensure that the same namespace isn't provided twice. This is intended
    // to teach new developers that 'goog.provide' is effectively a variable
    // declaration. And when JSCompiler transforms goog.provide into a real
    // variable declaration, the compiled JS should work the same as the raw
    // JS--even when the raw JS uses goog.provide incorrectly.
    if (goog.isProvided_(name)) {
      throw Error('Namespace "' + name + '" already declared.');
    }
    delete goog.implicitNamespaces_[name];

    var namespace = name;
    while ((namespace = namespace.substring(0, namespace.lastIndexOf('.')))) {
      if (goog.getObjectByName(namespace)) {
        break;
      }
      goog.implicitNamespaces_[namespace] = true;
    }
  }

  goog.exportPath_(name);
};


/**
 * Marks that the current file should only be used for testing, and never for
 * live code in production.
 * @param {string=} opt_message Optional message to add to the error that's
 *     raised when used in production code.
 */
goog.setTestOnly = function(opt_message) {
  if (COMPILED && !goog.DEBUG) {
    opt_message = opt_message || '';
    throw Error('Importing test-only code into non-debug environment' +
                opt_message ? ': ' + opt_message : '.');
  }
};


if (!COMPILED) {

  /**
   * Check if the given name has been goog.provided. This will return false for
   * names that are available only as implicit namespaces.
   * @param {string} name name of the object to look for.
   * @return {boolean} Whether the name has been provided.
   * @private
   */
  goog.isProvided_ = function(name) {
    return !goog.implicitNamespaces_[name] && !!goog.getObjectByName(name);
  };

  /**
   * Namespaces implicitly defined by goog.provide. For example,
   * goog.provide('goog.events.Event') implicitly declares
   * that 'goog' and 'goog.events' must be namespaces.
   *
   * @type {Object}
   * @private
   */
  goog.implicitNamespaces_ = {};
}


/**
 * Builds an object structure for the provided namespace path,
 * ensuring that names that already exist are not overwritten. For
 * example:
 * "a.b.c" -> a = {};a.b={};a.b.c={};
 * Used by goog.provide and goog.exportSymbol.
 * @param {string} name name of the object that this file defines.
 * @param {*=} opt_object the object to expose at the end of the path.
 * @param {Object=} opt_objectToExportTo The object to add the path to; default
 *     is |goog.global|.
 * @private
 */
goog.exportPath_ = function(name, opt_object, opt_objectToExportTo) {
  var parts = name.split('.');
  var cur = opt_objectToExportTo || goog.global;

  // Internet Explorer exhibits strange behavior when throwing errors from
  // methods externed in this manner.  See the testExportSymbolExceptions in
  // base_test.html for an example.
  if (!(parts[0] in cur) && cur.execScript) {
    cur.execScript('var ' + parts[0]);
  }

  // Certain browsers cannot parse code in the form for((a in b); c;);
  // This pattern is produced by the JSCompiler when it collapses the
  // statement above into the conditional loop below. To prevent this from
  // happening, use a for-loop and reserve the init logic as below.

  // Parentheses added to eliminate strict JS warning in Firefox.
  for (var part; parts.length && (part = parts.shift());) {
    if (!parts.length && goog.isDef(opt_object)) {
      // last part and we have an object; use it
      cur[part] = opt_object;
    } else if (cur[part]) {
      cur = cur[part];
    } else {
      cur = cur[part] = {};
    }
  }
};


/**
 * Returns an object based on its fully qualified external name.  If you are
 * using a compilation pass that renames property names beware that using this
 * function will not find renamed properties.
 *
 * @param {string} name The fully qualified name.
 * @param {Object=} opt_obj The object within which to look; default is
 *     |goog.global|.
 * @return {?} The value (object or primitive) or, if not found, null.
 */
goog.getObjectByName = function(name, opt_obj) {
  var parts = name.split('.');
  var cur = opt_obj || goog.global;
  for (var part; part = parts.shift(); ) {
    if (goog.isDefAndNotNull(cur[part])) {
      cur = cur[part];
    } else {
      return null;
    }
  }
  return cur;
};


/**
 * Globalizes a whole namespace, such as goog or goog.lang.
 *
 * @param {Object} obj The namespace to globalize.
 * @param {Object=} opt_global The object to add the properties to.
 * @deprecated Properties may be explicitly exported to the global scope, but
 *     this should no longer be done in bulk.
 */
goog.globalize = function(obj, opt_global) {
  var global = opt_global || goog.global;
  for (var x in obj) {
    global[x] = obj[x];
  }
};


/**
 * Adds a dependency from a file to the files it requires.
 * @param {string} relPath The path to the js file.
 * @param {Array} provides An array of strings with the names of the objects
 *                         this file provides.
 * @param {Array} requires An array of strings with the names of the objects
 *                         this file requires.
 */
goog.addDependency = function(relPath, provides, requires) {
  if (!COMPILED) {
    var provide, require;
    var path = relPath.replace(/\\/g, '/');
    var deps = goog.dependencies_;
    for (var i = 0; provide = provides[i]; i++) {
      deps.nameToPath[provide] = path;
      if (!(path in deps.pathToNames)) {
        deps.pathToNames[path] = {};
      }
      deps.pathToNames[path][provide] = true;
    }
    for (var j = 0; require = requires[j]; j++) {
      if (!(path in deps.requires)) {
        deps.requires[path] = {};
      }
      deps.requires[path][require] = true;
    }
  }
};




// NOTE(nnaze): The debug DOM loader was included in base.js as an orignal
// way to do "debug-mode" development.  The dependency system can sometimes
// be confusing, as can the debug DOM loader's asyncronous nature.
//
// With the DOM loader, a call to goog.require() is not blocking -- the
// script will not load until some point after the current script.  If a
// namespace is needed at runtime, it needs to be defined in a previous
// script, or loaded via require() with its registered dependencies.
// User-defined namespaces may need their own deps file.  See http://go/js_deps,
// http://go/genjsdeps, or, externally, DepsWriter.
// http://code.google.com/closure/library/docs/depswriter.html
//
// Because of legacy clients, the DOM loader can't be easily removed from
// base.js.  Work is being done to make it disableable or replaceable for
// different environments (DOM-less JavaScript interpreters like Rhino or V8,
// for example). See bootstrap/ for more information.


/**
 * @define {boolean} Whether to enable the debug loader.
 *
 * If enabled, a call to goog.require() will attempt to load the namespace by
 * appending a script tag to the DOM (if the namespace has been registered).
 *
 * If disabled, goog.require() will simply assert that the namespace has been
 * provided (and depend on the fact that some outside tool correctly ordered
 * the script).
 */
goog.ENABLE_DEBUG_LOADER = true;


/**
 * Implements a system for the dynamic resolution of dependencies
 * that works in parallel with the BUILD system. Note that all calls
 * to goog.require will be stripped by the JSCompiler when the
 * --closure_pass option is used.
 * @see goog.provide
 * @param {string} name Namespace to include (as was given in goog.provide())
 *     in the form "goog.package.part".
 */
goog.require = function(name) {

  // if the object already exists we do not need do do anything
  // TODO(arv): If we start to support require based on file name this has
  //            to change
  // TODO(arv): If we allow goog.foo.* this has to change
  // TODO(arv): If we implement dynamic load after page load we should probably
  //            not remove this code for the compiled output
  if (!COMPILED) {
    if (goog.isProvided_(name)) {
      return;
    }

    if (goog.ENABLE_DEBUG_LOADER) {
      var path = goog.getPathFromDeps_(name);
      if (path) {
        goog.included_[path] = true;
        goog.writeScripts_();
        return;
      }
    }

    var errorMessage = 'goog.require could not find: ' + name;
    if (goog.global.console) {
      goog.global.console['error'](errorMessage);
    }


      throw Error(errorMessage);

  }
};


/**
 * Path for included scripts
 * @type {string}
 */
goog.basePath = '';


/**
 * A hook for overriding the base path.
 * @type {string|undefined}
 */
goog.global.CLOSURE_BASE_PATH;


/**
 * Whether to write out Closure's deps file. By default,
 * the deps are written.
 * @type {boolean|undefined}
 */
goog.global.CLOSURE_NO_DEPS;


/**
 * A function to import a single script. This is meant to be overridden when
 * Closure is being run in non-HTML contexts, such as web workers. It's defined
 * in the global scope so that it can be set before base.js is loaded, which
 * allows deps.js to be imported properly.
 *
 * The function is passed the script source, which is a relative URI. It should
 * return true if the script was imported, false otherwise.
 */
goog.global.CLOSURE_IMPORT_SCRIPT;


/**
 * Null function used for default values of callbacks, etc.
 * @return {void} Nothing.
 */
goog.nullFunction = function() {};


/**
 * The identity function. Returns its first argument.
 *
 * @param {*=} opt_returnValue The single value that will be returned.
 * @param {...*} var_args Optional trailing arguments. These are ignored.
 * @return {?} The first argument. We can't know the type -- just pass it along
 *      without type.
 * @deprecated Use goog.functions.identity instead.
 */
goog.identityFunction = function(opt_returnValue, var_args) {
  return opt_returnValue;
};


/**
 * When defining a class Foo with an abstract method bar(), you can do:
 *
 * Foo.prototype.bar = goog.abstractMethod
 *
 * Now if a subclass of Foo fails to override bar(), an error
 * will be thrown when bar() is invoked.
 *
 * Note: This does not take the name of the function to override as
 * an argument because that would make it more difficult to obfuscate
 * our JavaScript code.
 *
 * @type {!Function}
 * @throws {Error} when invoked to indicate the method should be
 *   overridden.
 */
goog.abstractMethod = function() {
  throw Error('unimplemented abstract method');
};


/**
 * Adds a {@code getInstance} static method that always return the same instance
 * object.
 * @param {!Function} ctor The constructor for the class to add the static
 *     method to.
 */
goog.addSingletonGetter = function(ctor) {
  ctor.getInstance = function() {
    if (ctor.instance_) {
      return ctor.instance_;
    }
    if (goog.DEBUG) {
      // NOTE: JSCompiler can't optimize away Array#push.
      goog.instantiatedSingletons_[goog.instantiatedSingletons_.length] = ctor;
    }
    return ctor.instance_ = new ctor;
  };
};


/**
 * All singleton classes that have been instantiated, for testing. Don't read
 * it directly, use the {@code goog.testing.singleton} module. The compiler
 * removes this variable if unused.
 * @type {!Array.<!Function>}
 * @private
 */
goog.instantiatedSingletons_ = [];


if (!COMPILED && goog.ENABLE_DEBUG_LOADER) {
  /**
   * Object used to keep track of urls that have already been added. This
   * record allows the prevention of circular dependencies.
   * @type {Object}
   * @private
   */
  goog.included_ = {};


  /**
   * This object is used to keep track of dependencies and other data that is
   * used for loading scripts
   * @private
   * @type {Object}
   */
  goog.dependencies_ = {
    pathToNames: {}, // 1 to many
    nameToPath: {}, // 1 to 1
    requires: {}, // 1 to many
    // used when resolving dependencies to prevent us from
    // visiting the file twice
    visited: {},
    written: {} // used to keep track of script files we have written
  };


  /**
   * Tries to detect whether is in the context of an HTML document.
   * @return {boolean} True if it looks like HTML document.
   * @private
   */
  goog.inHtmlDocument_ = function() {
    var doc = goog.global.document;
    return typeof doc != 'undefined' &&
           'write' in doc;  // XULDocument misses write.
  };


  /**
   * Tries to detect the base path of the base.js script that bootstraps Closure
   * @private
   */
  goog.findBasePath_ = function() {
    if (goog.global.CLOSURE_BASE_PATH) {
      goog.basePath = goog.global.CLOSURE_BASE_PATH;
      return;
    } else if (!goog.inHtmlDocument_()) {
      return;
    }
    var doc = goog.global.document;
    var scripts = doc.getElementsByTagName('script');
    // Search backwards since the current script is in almost all cases the one
    // that has base.js.
    for (var i = scripts.length - 1; i >= 0; --i) {
      var src = scripts[i].src;
      var qmark = src.lastIndexOf('?');
      var l = qmark == -1 ? src.length : qmark;
      if (src.substr(l - 7, 7) == 'base.js') {
        goog.basePath = src.substr(0, l - 7);
        return;
      }
    }
  };


  /**
   * Imports a script if, and only if, that script hasn't already been imported.
   * (Must be called at execution time)
   * @param {string} src Script source.
   * @private
   */
  goog.importScript_ = function(src) {
    var importScript = goog.global.CLOSURE_IMPORT_SCRIPT ||
        goog.writeScriptTag_;
    if (!goog.dependencies_.written[src] && importScript(src)) {
      goog.dependencies_.written[src] = true;
    }
  };


  /**
   * The default implementation of the import function. Writes a script tag to
   * import the script.
   *
   * @param {string} src The script source.
   * @return {boolean} True if the script was imported, false otherwise.
   * @private
   */
  goog.writeScriptTag_ = function(src) {
    if (goog.inHtmlDocument_()) {
      var doc = goog.global.document;

      // If the user tries to require a new symbol after document load,
      // something has gone terribly wrong. Doing a document.write would
      // wipe out the page.
      if (doc.readyState == 'complete') {
        // Certain test frameworks load base.js multiple times, which tries
        // to write deps.js each time. If that happens, just fail silently.
        // These frameworks wipe the page between each load of base.js, so this
        // is OK.
        var isDeps = /\bdeps.js$/.test(src);
        if (isDeps) {
          return false;
        } else {
          throw Error('Cannot write "' + src + '" after document load');
        }
      }

      doc.write(
          '<script type="text/javascript" src="' + src + '"></' + 'script>');
      return true;
    } else {
      return false;
    }
  };


  /**
   * Resolves dependencies based on the dependencies added using addDependency
   * and calls importScript_ in the correct order.
   * @private
   */
  goog.writeScripts_ = function() {
    // the scripts we need to write this time
    var scripts = [];
    var seenScript = {};
    var deps = goog.dependencies_;

    function visitNode(path) {
      if (path in deps.written) {
        return;
      }

      // we have already visited this one. We can get here if we have cyclic
      // dependencies
      if (path in deps.visited) {
        if (!(path in seenScript)) {
          seenScript[path] = true;
          scripts.push(path);
        }
        return;
      }

      deps.visited[path] = true;

      if (path in deps.requires) {
        for (var requireName in deps.requires[path]) {
          // If the required name is defined, we assume that it was already
          // bootstrapped by other means.
          if (!goog.isProvided_(requireName)) {
            if (requireName in deps.nameToPath) {
              visitNode(deps.nameToPath[requireName]);
            } else {
              throw Error('Undefined nameToPath for ' + requireName);
            }
          }
        }
      }

      if (!(path in seenScript)) {
        seenScript[path] = true;
        scripts.push(path);
      }
    }

    for (var path in goog.included_) {
      if (!deps.written[path]) {
        visitNode(path);
      }
    }

    for (var i = 0; i < scripts.length; i++) {
      if (scripts[i]) {
        goog.importScript_(goog.basePath + scripts[i]);
      } else {
        throw Error('Undefined script input');
      }
    }
  };


  /**
   * Looks at the dependency rules and tries to determine the script file that
   * fulfills a particular rule.
   * @param {string} rule In the form goog.namespace.Class or project.script.
   * @return {?string} Url corresponding to the rule, or null.
   * @private
   */
  goog.getPathFromDeps_ = function(rule) {
    if (rule in goog.dependencies_.nameToPath) {
      return goog.dependencies_.nameToPath[rule];
    } else {
      return null;
    }
  };

  goog.findBasePath_();

  // Allow projects to manage the deps files themselves.
  if (!goog.global.CLOSURE_NO_DEPS) {
    goog.importScript_(goog.basePath + 'deps.js');
  }
}



//==============================================================================
// Language Enhancements
//==============================================================================


/**
 * This is a "fixed" version of the typeof operator.  It differs from the typeof
 * operator in such a way that null returns 'null' and arrays return 'array'.
 * @param {*} value The value to get the type of.
 * @return {string} The name of the type.
 */
goog.typeOf = function(value) {
  var s = typeof value;
  if (s == 'object') {
    if (value) {
      // Check these first, so we can avoid calling Object.prototype.toString if
      // possible.
      //
      // IE improperly marshals tyepof across execution contexts, but a
      // cross-context object will still return false for "instanceof Object".
      if (value instanceof Array) {
        return 'array';
      } else if (value instanceof Object) {
        return s;
      }

      // HACK: In order to use an Object prototype method on the arbitrary
      //   value, the compiler requires the value be cast to type Object,
      //   even though the ECMA spec explicitly allows it.
      var className = Object.prototype.toString.call(
          /** @type {Object} */ (value));
      // In Firefox 3.6, attempting to access iframe window objects' length
      // property throws an NS_ERROR_FAILURE, so we need to special-case it
      // here.
      if (className == '[object Window]') {
        return 'object';
      }

      // We cannot always use constructor == Array or instanceof Array because
      // different frames have different Array objects. In IE6, if the iframe
      // where the array was created is destroyed, the array loses its
      // prototype. Then dereferencing val.splice here throws an exception, so
      // we can't use goog.isFunction. Calling typeof directly returns 'unknown'
      // so that will work. In this case, this function will return false and
      // most array functions will still work because the array is still
      // array-like (supports length and []) even though it has lost its
      // prototype.
      // Mark Miller noticed that Object.prototype.toString
      // allows access to the unforgeable [[Class]] property.
      //  15.2.4.2 Object.prototype.toString ( )
      //  When the toString method is called, the following steps are taken:
      //      1. Get the [[Class]] property of this object.
      //      2. Compute a string value by concatenating the three strings
      //         "[object ", Result(1), and "]".
      //      3. Return Result(2).
      // and this behavior survives the destruction of the execution context.
      if ((className == '[object Array]' ||
           // In IE all non value types are wrapped as objects across window
           // boundaries (not iframe though) so we have to do object detection
           // for this edge case
           typeof value.length == 'number' &&
           typeof value.splice != 'undefined' &&
           typeof value.propertyIsEnumerable != 'undefined' &&
           !value.propertyIsEnumerable('splice')

          )) {
        return 'array';
      }
      // HACK: There is still an array case that fails.
      //     function ArrayImpostor() {}
      //     ArrayImpostor.prototype = [];
      //     var impostor = new ArrayImpostor;
      // this can be fixed by getting rid of the fast path
      // (value instanceof Array) and solely relying on
      // (value && Object.prototype.toString.vall(value) === '[object Array]')
      // but that would require many more function calls and is not warranted
      // unless closure code is receiving objects from untrusted sources.

      // IE in cross-window calls does not correctly marshal the function type
      // (it appears just as an object) so we cannot use just typeof val ==
      // 'function'. However, if the object has a call property, it is a
      // function.
      if ((className == '[object Function]' ||
          typeof value.call != 'undefined' &&
          typeof value.propertyIsEnumerable != 'undefined' &&
          !value.propertyIsEnumerable('call'))) {
        return 'function';
      }


    } else {
      return 'null';
    }

  } else if (s == 'function' && typeof value.call == 'undefined') {
    // In Safari typeof nodeList returns 'function', and on Firefox
    // typeof behaves similarly for HTML{Applet,Embed,Object}Elements
    // and RegExps.  We would like to return object for those and we can
    // detect an invalid function by making sure that the function
    // object has a call method.
    return 'object';
  }
  return s;
};


/**
 * Returns true if the specified value is not |undefined|.
 * WARNING: Do not use this to test if an object has a property. Use the in
 * operator instead.  Additionally, this function assumes that the global
 * undefined variable has not been redefined.
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is defined.
 */
goog.isDef = function(val) {
  return val !== undefined;
};


/**
 * Returns true if the specified value is |null|
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is null.
 */
goog.isNull = function(val) {
  return val === null;
};


/**
 * Returns true if the specified value is defined and not null
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is defined and not null.
 */
goog.isDefAndNotNull = function(val) {
  // Note that undefined == null.
  return val != null;
};


/**
 * Returns true if the specified value is an array
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is an array.
 */
goog.isArray = function(val) {
  return goog.typeOf(val) == 'array';
};


/**
 * Returns true if the object looks like an array. To qualify as array like
 * the value needs to be either a NodeList or an object with a Number length
 * property.
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is an array.
 */
goog.isArrayLike = function(val) {
  var type = goog.typeOf(val);
  return type == 'array' || type == 'object' && typeof val.length == 'number';
};


/**
 * Returns true if the object looks like a Date. To qualify as Date-like
 * the value needs to be an object and have a getFullYear() function.
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is a like a Date.
 */
goog.isDateLike = function(val) {
  return goog.isObject(val) && typeof val.getFullYear == 'function';
};


/**
 * Returns true if the specified value is a string
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is a string.
 */
goog.isString = function(val) {
  return typeof val == 'string';
};


/**
 * Returns true if the specified value is a boolean
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is boolean.
 */
goog.isBoolean = function(val) {
  return typeof val == 'boolean';
};


/**
 * Returns true if the specified value is a number
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is a number.
 */
goog.isNumber = function(val) {
  return typeof val == 'number';
};


/**
 * Returns true if the specified value is a function
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is a function.
 */
goog.isFunction = function(val) {
  return goog.typeOf(val) == 'function';
};


/**
 * Returns true if the specified value is an object.  This includes arrays
 * and functions.
 * @param {*} val Variable to test.
 * @return {boolean} Whether variable is an object.
 */
goog.isObject = function(val) {
  var type = typeof val;
  return type == 'object' && val != null || type == 'function';
  // return Object(val) === val also works, but is slower, especially if val is
  // not an object.
};


/**
 * Gets a unique ID for an object. This mutates the object so that further
 * calls with the same object as a parameter returns the same value. The unique
 * ID is guaranteed to be unique across the current session amongst objects that
 * are passed into {@code getUid}. There is no guarantee that the ID is unique
 * or consistent across sessions. It is unsafe to generate unique ID for
 * function prototypes.
 *
 * @param {Object} obj The object to get the unique ID for.
 * @return {number} The unique ID for the object.
 */
goog.getUid = function(obj) {
  // TODO(arv): Make the type stricter, do not accept null.

  // In Opera window.hasOwnProperty exists but always returns false so we avoid
  // using it. As a consequence the unique ID generated for BaseClass.prototype
  // and SubClass.prototype will be the same.
  return obj[goog.UID_PROPERTY_] ||
      (obj[goog.UID_PROPERTY_] = ++goog.uidCounter_);
};


/**
 * Removes the unique ID from an object. This is useful if the object was
 * previously mutated using {@code goog.getUid} in which case the mutation is
 * undone.
 * @param {Object} obj The object to remove the unique ID field from.
 */
goog.removeUid = function(obj) {
  // TODO(arv): Make the type stricter, do not accept null.

  // DOM nodes in IE are not instance of Object and throws exception
  // for delete. Instead we try to use removeAttribute
  if ('removeAttribute' in obj) {
    obj.removeAttribute(goog.UID_PROPERTY_);
  }
  /** @preserveTry */
  try {
    delete obj[goog.UID_PROPERTY_];
  } catch (ex) {
  }
};


/**
 * Name for unique ID property. Initialized in a way to help avoid collisions
 * with other closure javascript on the same page.
 * @type {string}
 * @private
 */
goog.UID_PROPERTY_ = 'closure_uid_' +
    Math.floor(Math.random() * 2147483648).toString(36);


/**
 * Counter for UID.
 * @type {number}
 * @private
 */
goog.uidCounter_ = 0;


/**
 * Adds a hash code field to an object. The hash code is unique for the
 * given object.
 * @param {Object} obj The object to get the hash code for.
 * @return {number} The hash code for the object.
 * @deprecated Use goog.getUid instead.
 */
goog.getHashCode = goog.getUid;


/**
 * Removes the hash code field from an object.
 * @param {Object} obj The object to remove the field from.
 * @deprecated Use goog.removeUid instead.
 */
goog.removeHashCode = goog.removeUid;


/**
 * Clones a value. The input may be an Object, Array, or basic type. Objects and
 * arrays will be cloned recursively.
 *
 * WARNINGS:
 * <code>goog.cloneObject</code> does not detect reference loops. Objects that
 * refer to themselves will cause infinite recursion.
 *
 * <code>goog.cloneObject</code> is unaware of unique identifiers, and copies
 * UIDs created by <code>getUid</code> into cloned results.
 *
 * @param {*} obj The value to clone.
 * @return {*} A clone of the input value.
 * @deprecated goog.cloneObject is unsafe. Prefer the goog.object methods.
 */
goog.cloneObject = function(obj) {
  var type = goog.typeOf(obj);
  if (type == 'object' || type == 'array') {
    if (obj.clone) {
      return obj.clone();
    }
    var clone = type == 'array' ? [] : {};
    for (var key in obj) {
      clone[key] = goog.cloneObject(obj[key]);
    }
    return clone;
  }

  return obj;
};


/**
 * A native implementation of goog.bind.
 * @param {Function} fn A function to partially apply.
 * @param {Object|undefined} selfObj Specifies the object which |this| should
 *     point to when the function is run.
 * @param {...*} var_args Additional arguments that are partially
 *     applied to the function.
 * @return {!Function} A partially-applied form of the function bind() was
 *     invoked as a method of.
 * @private
 * @suppress {deprecated} The compiler thinks that Function.prototype.bind
 *     is deprecated because some people have declared a pure-JS version.
 *     Only the pure-JS version is truly deprecated.
 */
goog.bindNative_ = function(fn, selfObj, var_args) {
  return /** @type {!Function} */ (fn.call.apply(fn.bind, arguments));
};


/**
 * A pure-JS implementation of goog.bind.
 * @param {Function} fn A function to partially apply.
 * @param {Object|undefined} selfObj Specifies the object which |this| should
 *     point to when the function is run.
 * @param {...*} var_args Additional arguments that are partially
 *     applied to the function.
 * @return {!Function} A partially-applied form of the function bind() was
 *     invoked as a method of.
 * @private
 */
goog.bindJs_ = function(fn, selfObj, var_args) {
  if (!fn) {
    throw new Error();
  }

  if (arguments.length > 2) {
    var boundArgs = Array.prototype.slice.call(arguments, 2);
    return function() {
      // Prepend the bound arguments to the current arguments.
      var newArgs = Array.prototype.slice.call(arguments);
      Array.prototype.unshift.apply(newArgs, boundArgs);
      return fn.apply(selfObj, newArgs);
    };

  } else {
    return function() {
      return fn.apply(selfObj, arguments);
    };
  }
};


/**
 * Partially applies this function to a particular 'this object' and zero or
 * more arguments. The result is a new function with some arguments of the first
 * function pre-filled and the value of |this| 'pre-specified'.<br><br>
 *
 * Remaining arguments specified at call-time are appended to the pre-
 * specified ones.<br><br>
 *
 * Also see: {@link #partial}.<br><br>
 *
 * Usage:
 * <pre>var barMethBound = bind(myFunction, myObj, 'arg1', 'arg2');
 * barMethBound('arg3', 'arg4');</pre>
 *
 * @param {Function} fn A function to partially apply.
 * @param {Object|undefined} selfObj Specifies the object which |this| should
 *     point to when the function is run.
 * @param {...*} var_args Additional arguments that are partially
 *     applied to the function.
 * @return {!Function} A partially-applied form of the function bind() was
 *     invoked as a method of.
 * @suppress {deprecated} See above.
 */
goog.bind = function(fn, selfObj, var_args) {
  // TODO(nicksantos): narrow the type signature.
  if (Function.prototype.bind &&
      // NOTE(nicksantos): Somebody pulled base.js into the default
      // Chrome extension environment. This means that for Chrome extensions,
      // they get the implementation of Function.prototype.bind that
      // calls goog.bind instead of the native one. Even worse, we don't want
      // to introduce a circular dependency between goog.bind and
      // Function.prototype.bind, so we have to hack this to make sure it
      // works correctly.
      Function.prototype.bind.toString().indexOf('native code') != -1) {
    goog.bind = goog.bindNative_;
  } else {
    goog.bind = goog.bindJs_;
  }
  return goog.bind.apply(null, arguments);
};


/**
 * Like bind(), except that a 'this object' is not required. Useful when the
 * target function is already bound.
 *
 * Usage:
 * var g = partial(f, arg1, arg2);
 * g(arg3, arg4);
 *
 * @param {Function} fn A function to partially apply.
 * @param {...*} var_args Additional arguments that are partially
 *     applied to fn.
 * @return {!Function} A partially-applied form of the function bind() was
 *     invoked as a method of.
 */
goog.partial = function(fn, var_args) {
  var args = Array.prototype.slice.call(arguments, 1);
  return function() {
    // Prepend the bound arguments to the current arguments.
    var newArgs = Array.prototype.slice.call(arguments);
    newArgs.unshift.apply(newArgs, args);
    return fn.apply(this, newArgs);
  };
};


/**
 * Copies all the members of a source object to a target object. This method
 * does not work on all browsers for all objects that contain keys such as
 * toString or hasOwnProperty. Use goog.object.extend for this purpose.
 * @param {Object} target Target.
 * @param {Object} source Source.
 */
goog.mixin = function(target, source) {
  for (var x in source) {
    target[x] = source[x];
  }

  // For IE7 or lower, the for-in-loop does not contain any properties that are
  // not enumerable on the prototype object (for example, isPrototypeOf from
  // Object.prototype) but also it will not include 'replace' on objects that
  // extend String and change 'replace' (not that it is common for anyone to
  // extend anything except Object).
};


/**
 * @return {number} An integer value representing the number of milliseconds
 *     between midnight, January 1, 1970 and the current time.
 */
goog.now = Date.now || (function() {
  // Unary plus operator converts its operand to a number which in the case of
  // a date is done by calling getTime().
  return +new Date();
});


/**
 * Evals javascript in the global scope.  In IE this uses execScript, other
 * browsers use goog.global.eval. If goog.global.eval does not evaluate in the
 * global scope (for example, in Safari), appends a script tag instead.
 * Throws an exception if neither execScript or eval is defined.
 * @param {string} script JavaScript string.
 */
goog.globalEval = function(script) {
  if (goog.global.execScript) {
    goog.global.execScript(script, 'JavaScript');
  } else if (goog.global.eval) {
    // Test to see if eval works
    if (goog.evalWorksForGlobals_ == null) {
      goog.global.eval('var _et_ = 1;');
      if (typeof goog.global['_et_'] != 'undefined') {
        delete goog.global['_et_'];
        goog.evalWorksForGlobals_ = true;
      } else {
        goog.evalWorksForGlobals_ = false;
      }
    }

    if (goog.evalWorksForGlobals_) {
      goog.global.eval(script);
    } else {
      var doc = goog.global.document;
      var scriptElt = doc.createElement('script');
      scriptElt.type = 'text/javascript';
      scriptElt.defer = false;
      // Note(user): can't use .innerHTML since "t('<test>')" will fail and
      // .text doesn't work in Safari 2.  Therefore we append a text node.
      scriptElt.appendChild(doc.createTextNode(script));
      doc.body.appendChild(scriptElt);
      doc.body.removeChild(scriptElt);
    }
  } else {
    throw Error('goog.globalEval not available');
  }
};


/**
 * Indicates whether or not we can call 'eval' directly to eval code in the
 * global scope. Set to a Boolean by the first call to goog.globalEval (which
 * empirically tests whether eval works for globals). @see goog.globalEval
 * @type {?boolean}
 * @private
 */
goog.evalWorksForGlobals_ = null;


/**
 * Optional map of CSS class names to obfuscated names used with
 * goog.getCssName().
 * @type {Object|undefined}
 * @private
 * @see goog.setCssNameMapping
 */
goog.cssNameMapping_;


/**
 * Optional obfuscation style for CSS class names. Should be set to either
 * 'BY_WHOLE' or 'BY_PART' if defined.
 * @type {string|undefined}
 * @private
 * @see goog.setCssNameMapping
 */
goog.cssNameMappingStyle_;


/**
 * Handles strings that are intended to be used as CSS class names.
 *
 * This function works in tandem with @see goog.setCssNameMapping.
 *
 * Without any mapping set, the arguments are simple joined with a
 * hyphen and passed through unaltered.
 *
 * When there is a mapping, there are two possible styles in which
 * these mappings are used. In the BY_PART style, each part (i.e. in
 * between hyphens) of the passed in css name is rewritten according
 * to the map. In the BY_WHOLE style, the full css name is looked up in
 * the map directly. If a rewrite is not specified by the map, the
 * compiler will output a warning.
 *
 * When the mapping is passed to the compiler, it will replace calls
 * to goog.getCssName with the strings from the mapping, e.g.
 *     var x = goog.getCssName('foo');
 *     var y = goog.getCssName(this.baseClass, 'active');
 *  becomes:
 *     var x= 'foo';
 *     var y = this.baseClass + '-active';
 *
 * If one argument is passed it will be processed, if two are passed
 * only the modifier will be processed, as it is assumed the first
 * argument was generated as a result of calling goog.getCssName.
 *
 * @param {string} className The class name.
 * @param {string=} opt_modifier A modifier to be appended to the class name.
 * @return {string} The class name or the concatenation of the class name and
 *     the modifier.
 */
goog.getCssName = function(className, opt_modifier) {
  var getMapping = function(cssName) {
    return goog.cssNameMapping_[cssName] || cssName;
  };

  var renameByParts = function(cssName) {
    // Remap all the parts individually.
    var parts = cssName.split('-');
    var mapped = [];
    for (var i = 0; i < parts.length; i++) {
      mapped.push(getMapping(parts[i]));
    }
    return mapped.join('-');
  };

  var rename;
  if (goog.cssNameMapping_) {
    rename = goog.cssNameMappingStyle_ == 'BY_WHOLE' ?
        getMapping : renameByParts;
  } else {
    rename = function(a) {
      return a;
    };
  }

  if (opt_modifier) {
    return className + '-' + rename(opt_modifier);
  } else {
    return rename(className);
  }
};


/**
 * Sets the map to check when returning a value from goog.getCssName(). Example:
 * <pre>
 * goog.setCssNameMapping({
 *   "goog": "a",
 *   "disabled": "b",
 * });
 *
 * var x = goog.getCssName('goog');
 * // The following evaluates to: "a a-b".
 * goog.getCssName('goog') + ' ' + goog.getCssName(x, 'disabled')
 * </pre>
 * When declared as a map of string literals to string literals, the JSCompiler
 * will replace all calls to goog.getCssName() using the supplied map if the
 * --closure_pass flag is set.
 *
 * @param {!Object} mapping A map of strings to strings where keys are possible
 *     arguments to goog.getCssName() and values are the corresponding values
 *     that should be returned.
 * @param {string=} opt_style The style of css name mapping. There are two valid
 *     options: 'BY_PART', and 'BY_WHOLE'.
 * @see goog.getCssName for a description.
 */
goog.setCssNameMapping = function(mapping, opt_style) {
  goog.cssNameMapping_ = mapping;
  goog.cssNameMappingStyle_ = opt_style;
};


/**
 * To use CSS renaming in compiled mode, one of the input files should have a
 * call to goog.setCssNameMapping() with an object literal that the JSCompiler
 * can extract and use to replace all calls to goog.getCssName(). In uncompiled
 * mode, JavaScript code should be loaded before this base.js file that declares
 * a global variable, CLOSURE_CSS_NAME_MAPPING, which is used below. This is
 * to ensure that the mapping is loaded before any calls to goog.getCssName()
 * are made in uncompiled mode.
 *
 * A hook for overriding the CSS name mapping.
 * @type {Object|undefined}
 */
goog.global.CLOSURE_CSS_NAME_MAPPING;


if (!COMPILED && goog.global.CLOSURE_CSS_NAME_MAPPING) {
  // This does not call goog.setCssNameMapping() because the JSCompiler
  // requires that goog.setCssNameMapping() be called with an object literal.
  goog.cssNameMapping_ = goog.global.CLOSURE_CSS_NAME_MAPPING;
}


/**
 * Gets a localized message.
 *
 * This function is a compiler primitive. If you give the compiler a localized
 * message bundle, it will replace the string at compile-time with a localized
 * version, and expand goog.getMsg call to a concatenated string.
 *
 * Messages must be initialized in the form:
 * <code>
 * var MSG_NAME = goog.getMsg('Hello {$placeholder}', {'placeholder': 'world'});
 * </code>
 *
 * @param {string} str Translatable string, places holders in the form {$foo}.
 * @param {Object=} opt_values Map of place holder name to value.
 * @return {string} message with placeholders filled.
 */
goog.getMsg = function(str, opt_values) {
  var values = opt_values || {};
  for (var key in values) {
    var value = ('' + values[key]).replace(/\$/g, '$$$$');
    str = str.replace(new RegExp('\\{\\$' + key + '\\}', 'gi'), value);
  }
  return str;
};


/**
 * Gets a localized message. If the message does not have a translation, gives a
 * fallback message.
 *
 * This is useful when introducing a new message that has not yet been
 * translated into all languages.
 *
 * This function is a compiler primtive. Must be used in the form:
 * <code>var x = goog.getMsgWithFallback(MSG_A, MSG_B);</code>
 * where MSG_A and MSG_B were initialized with goog.getMsg.
 *
 * @param {string} a The preferred message.
 * @param {string} b The fallback message.
 * @return {string} The best translated message.
 */
goog.getMsgWithFallback = function(a, b) {
  return a;
};


/**
 * Exposes an unobfuscated global namespace path for the given object.
 * Note that fields of the exported object *will* be obfuscated,
 * unless they are exported in turn via this function or
 * goog.exportProperty
 *
 * <p>Also handy for making public items that are defined in anonymous
 * closures.
 *
 * ex. goog.exportSymbol('public.path.Foo', Foo);
 *
 * ex. goog.exportSymbol('public.path.Foo.staticFunction',
 *                       Foo.staticFunction);
 *     public.path.Foo.staticFunction();
 *
 * ex. goog.exportSymbol('public.path.Foo.prototype.myMethod',
 *                       Foo.prototype.myMethod);
 *     new public.path.Foo().myMethod();
 *
 * @param {string} publicPath Unobfuscated name to export.
 * @param {*} object Object the name should point to.
 * @param {Object=} opt_objectToExportTo The object to add the path to; default
 *     is |goog.global|.
 */
goog.exportSymbol = function(publicPath, object, opt_objectToExportTo) {
  goog.exportPath_(publicPath, object, opt_objectToExportTo);
};


/**
 * Exports a property unobfuscated into the object's namespace.
 * ex. goog.exportProperty(Foo, 'staticFunction', Foo.staticFunction);
 * ex. goog.exportProperty(Foo.prototype, 'myMethod', Foo.prototype.myMethod);
 * @param {Object} object Object whose static property is being exported.
 * @param {string} publicName Unobfuscated name to export.
 * @param {*} symbol Object the name should point to.
 */
goog.exportProperty = function(object, publicName, symbol) {
  object[publicName] = symbol;
};


/**
 * Inherit the prototype methods from one constructor into another.
 *
 * Usage:
 * <pre>
 * function ParentClass(a, b) { }
 * ParentClass.prototype.foo = function(a) { }
 *
 * function ChildClass(a, b, c) {
 *   goog.base(this, a, b);
 * }
 * goog.inherits(ChildClass, ParentClass);
 *
 * var child = new ChildClass('a', 'b', 'see');
 * child.foo(); // works
 * </pre>
 *
 * In addition, a superclass' implementation of a method can be invoked
 * as follows:
 *
 * <pre>
 * ChildClass.prototype.foo = function(a) {
 *   ChildClass.superClass_.foo.call(this, a);
 *   // other code
 * };
 * </pre>
 *
 * @param {Function} childCtor Child class.
 * @param {Function} parentCtor Parent class.
 */
goog.inherits = function(childCtor, parentCtor) {
  /** @constructor */
  function tempCtor() {};
  tempCtor.prototype = parentCtor.prototype;
  childCtor.superClass_ = parentCtor.prototype;
  childCtor.prototype = new tempCtor();
  /** @override */
  childCtor.prototype.constructor = childCtor;
};


/**
 * Call up to the superclass.
 *
 * If this is called from a constructor, then this calls the superclass
 * contructor with arguments 1-N.
 *
 * If this is called from a prototype method, then you must pass
 * the name of the method as the second argument to this function. If
 * you do not, you will get a runtime error. This calls the superclass'
 * method with arguments 2-N.
 *
 * This function only works if you use goog.inherits to express
 * inheritance relationships between your classes.
 *
 * This function is a compiler primitive. At compile-time, the
 * compiler will do macro expansion to remove a lot of
 * the extra overhead that this function introduces. The compiler
 * will also enforce a lot of the assumptions that this function
 * makes, and treat it as a compiler error if you break them.
 *
 * @param {!Object} me Should always be "this".
 * @param {*=} opt_methodName The method name if calling a super method.
 * @param {...*} var_args The rest of the arguments.
 * @return {*} The return value of the superclass method.
 */
goog.base = function(me, opt_methodName, var_args) {
  var caller = arguments.callee.caller;
  if (caller.superClass_) {
    // This is a constructor. Call the superclass constructor.
    return caller.superClass_.constructor.apply(
        me, Array.prototype.slice.call(arguments, 1));
  }

  var args = Array.prototype.slice.call(arguments, 2);
  var foundCaller = false;
  for (var ctor = me.constructor;
       ctor; ctor = ctor.superClass_ && ctor.superClass_.constructor) {
    if (ctor.prototype[opt_methodName] === caller) {
      foundCaller = true;
    } else if (foundCaller) {
      return ctor.prototype[opt_methodName].apply(me, args);
    }
  }

  // If we did not find the caller in the prototype chain,
  // then one of two things happened:
  // 1) The caller is an instance method.
  // 2) This method was not called by the right caller.
  if (me[opt_methodName] === caller) {
    return me.constructor.prototype[opt_methodName].apply(me, args);
  } else {
    throw Error(
        'goog.base called from a method of one name ' +
        'to a method of a different name');
  }
};


/**
 * Allow for aliasing within scope functions.  This function exists for
 * uncompiled code - in compiled code the calls will be inlined and the
 * aliases applied.  In uncompiled code the function is simply run since the
 * aliases as written are valid JavaScript.
 * @param {function()} fn Function to call.  This function can contain aliases
 *     to namespaces (e.g. "var dom = goog.dom") or classes
 *    (e.g. "var Timer = goog.Timer").
 */
goog.scope = function(fn) {
  fn.call(goog.global);
};


// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing operations on binary expressions.
 */


goog.provide('wgxpath.BinaryExpr');

goog.require('wgxpath.DataType');
goog.require('wgxpath.Expr');
goog.require('wgxpath.Node');



/**
 * Constructor for BinaryExpr.
 *
 * @param {!wgxpath.BinaryExpr.Op} op A binary operator.
 * @param {!wgxpath.Expr} left The left hand side of the expression.
 * @param {!wgxpath.Expr} right The right hand side of the expression.
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.BinaryExpr = function(op, left, right) {
  var opCast = /** @type {!wgxpath.BinaryExpr.Op_} */ (op);
  wgxpath.Expr.call(this, opCast.dataType_);

  /**
   * @private
   * @type {!wgxpath.BinaryExpr.Op_}
   */
  this.op_ = opCast;

  /**
   * @private
   * @type {!wgxpath.Expr}
   */
  this.left_ = left;

  /**
   * @private
   * @type {!wgxpath.Expr}
   */
  this.right_ = right;

  this.setNeedContextPosition(left.doesNeedContextPosition() ||
      right.doesNeedContextPosition());
  this.setNeedContextNode(left.doesNeedContextNode() ||
      right.doesNeedContextNode());

  // Optimize [@id="foo"] and [@name="bar"]
  if (this.op_ == wgxpath.BinaryExpr.Op.EQUAL) {
    if (!right.doesNeedContextNode() && !right.doesNeedContextPosition() &&
        right.getDataType() != wgxpath.DataType.NODESET &&
        right.getDataType() != wgxpath.DataType.VOID && left.getQuickAttr()) {
      this.setQuickAttr({
        name: left.getQuickAttr().name,
        valueExpr: right});
    } else if (!left.doesNeedContextNode() && !left.doesNeedContextPosition() &&
        left.getDataType() != wgxpath.DataType.NODESET &&
        left.getDataType() != wgxpath.DataType.VOID && right.getQuickAttr()) {
      this.setQuickAttr({
        name: right.getQuickAttr().name,
        valueExpr: left});
    }
  }
};
goog.inherits(wgxpath.BinaryExpr, wgxpath.Expr);


/**
 * Performs comparison between the left hand side and the right hand side.
 *
 * @private
 * @param {function((string|number|boolean), (string|number|boolean))}
 *        comp A comparison function that takes two parameters.
 * @param {!wgxpath.Expr} lhs The left hand side of the expression.
 * @param {!wgxpath.Expr} rhs The right hand side of the expression.
 * @param {!wgxpath.Context} ctx The context to perform the comparison in.
 * @param {boolean=} opt_equChk Whether the comparison checks for equality.
 * @return {boolean} True if comp returns true, false otherwise.
 */
wgxpath.BinaryExpr.compare_ = function(comp, lhs, rhs, ctx, opt_equChk) {
  var left = lhs.evaluate(ctx);
  var right = rhs.evaluate(ctx);
  var lIter, rIter, lNode, rNode;
  if (left instanceof wgxpath.NodeSet && right instanceof wgxpath.NodeSet) {
    lIter = left.iterator();
    for (lNode = lIter.next(); lNode; lNode = lIter.next()) {
      rIter = right.iterator();
      for (rNode = rIter.next(); rNode; rNode = rIter.next()) {
        if (comp(wgxpath.Node.getValueAsString(lNode),
            wgxpath.Node.getValueAsString(rNode))) {
          return true;
        }
      }
    }
    return false;
  }
  if ((left instanceof wgxpath.NodeSet) ||
      (right instanceof wgxpath.NodeSet)) {
    var nodeset, primitive;
    if ((left instanceof wgxpath.NodeSet)) {
      nodeset = left, primitive = right;
    } else {
      nodeset = right, primitive = left;
    }
    var iter = nodeset.iterator();
    var type = typeof primitive;
    for (var node = iter.next(); node; node = iter.next()) {
      var stringValue;
      switch (type) {
        case 'number':
          stringValue = wgxpath.Node.getValueAsNumber(node);
          break;
        case 'boolean':
          stringValue = wgxpath.Node.getValueAsBool(node);
          break;
        case 'string':
          stringValue = wgxpath.Node.getValueAsString(node);
          break;
        default:
          throw Error('Illegal primitive type for comparison.');
      }
      if (comp(stringValue,
          /** @type {(string|number|boolean)} */ (primitive))) {
        return true;
      }
    }
    return false;
  }
  if (opt_equChk) {
    if (typeof left == 'boolean' || typeof right == 'boolean') {
      return comp(!!left, !!right);
    }
    if (typeof left == 'number' || typeof right == 'number') {
      return comp(+left, +right);
    }
    return comp(left, right);
  }
  return comp(+left, +right);
};


/**
 * @override
 * @return {(boolean|number)} The boolean or number result.
 */
wgxpath.BinaryExpr.prototype.evaluate = function(ctx) {
  return this.op_.evaluate_(this.left_, this.right_, ctx);
};


/**
 * @override
 */
wgxpath.BinaryExpr.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'binary expression: ' + this.op_ + '\n';
  indent += wgxpath.Expr.INDENT;
  text += this.left_.toString(indent) + '\n';
  text += this.right_.toString(indent);
  return text;
};



/**
 * A binary operator.
 *
 * @param {string} opString The operator string.
 * @param {number} precedence The precedence when evaluated.
 * @param {!wgxpath.DataType} dataType The dataType to return when evaluated.
 * @param {function(!wgxpath.Expr, !wgxpath.Expr, !wgxpath.Context)}
 *         evaluate An evaluation function.
 * @constructor
 * @private
 */
wgxpath.BinaryExpr.Op_ = function(opString, precedence, dataType, evaluate) {

  /**
   * @private
   * @type {string}
   */
  this.opString_ = opString;

  /**
   * @private
   * @type {number}
   */
  this.precedence_ = precedence;

  /**
   * @private
   * @type {!wgxpath.DataType}
   */
  this.dataType_ = dataType;

  /**
   * @private
   * @type {function(!wgxpath.Expr, !wgxpath.Expr, !wgxpath.Context)}
   */
  this.evaluate_ = evaluate;
};


/**
 * Returns the precedence for the operator.
 *
 * @return {number} The precedence.
 */
wgxpath.BinaryExpr.Op_.prototype.getPrecedence = function() {
  return this.precedence_;
};


/**
 * @override
 */
wgxpath.BinaryExpr.Op_.prototype.toString = function() {
  return this.opString_;
};


/**
 * A mapping from operator strings to operator objects.
 *
 * @private
 * @type {!Object.<string, !wgxpath.BinaryExpr.Op>}
 */
wgxpath.BinaryExpr.stringToOpMap_ = {};


/**
 * Creates a binary operator.
 *
 * @param {string} opString The operator string.
 * @param {number} precedence The precedence when evaluated.
 * @param {!wgxpath.DataType} dataType The dataType to return when evaluated.
 * @param {function(!wgxpath.Expr, !wgxpath.Expr, !wgxpath.Context)}
 *         evaluate An evaluation function.
 * @return {!wgxpath.BinaryExpr.Op} A binary expression operator.
 * @private
 */
wgxpath.BinaryExpr.createOp_ = function(opString, precedence, dataType,
    evaluate) {
  if (opString in wgxpath.BinaryExpr.stringToOpMap_) {
    throw new Error('Binary operator already created: ' + opString);
  }
  // The upcast and then downcast for the JSCompiler.
  var op = (/** @type {!Object} */ new wgxpath.BinaryExpr.Op_(
      opString, precedence, dataType, evaluate));
  op = (/** @type {!wgxpath.BinaryExpr.Op} */ op);
  wgxpath.BinaryExpr.stringToOpMap_[op.toString()] = op;
  return op;
};


/**
 * Returns the operator with this opString or null if none.
 *
 * @param {string} opString The opString.
 * @return {!wgxpath.BinaryExpr.Op} The operator.
 */
wgxpath.BinaryExpr.getOp = function(opString) {
  return wgxpath.BinaryExpr.stringToOpMap_[opString] || null;
};


/**
 * Binary operator enumeration.
 *
 * @enum {{getPrecedence: function(): number}}
 */
wgxpath.BinaryExpr.Op = {
  DIV: wgxpath.BinaryExpr.createOp_('div', 6, wgxpath.DataType.NUMBER,
      function(left, right, ctx) {
        return left.asNumber(ctx) / right.asNumber(ctx);
      }),
  MOD: wgxpath.BinaryExpr.createOp_('mod', 6, wgxpath.DataType.NUMBER,
      function(left, right, ctx) {
        return left.asNumber(ctx) % right.asNumber(ctx);
      }),
  MULT: wgxpath.BinaryExpr.createOp_('*', 6, wgxpath.DataType.NUMBER,
      function(left, right, ctx) {
        return left.asNumber(ctx) * right.asNumber(ctx);
      }),
  PLUS: wgxpath.BinaryExpr.createOp_('+', 5, wgxpath.DataType.NUMBER,
      function(left, right, ctx) {
        return left.asNumber(ctx) + right.asNumber(ctx);
      }),
  MINUS: wgxpath.BinaryExpr.createOp_('-', 5, wgxpath.DataType.NUMBER,
      function(left, right, ctx) {
        return left.asNumber(ctx) - right.asNumber(ctx);
      }),
  LESSTHAN: wgxpath.BinaryExpr.createOp_('<', 4, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a < b;},
            left, right, ctx);
      }),
  GREATERTHAN: wgxpath.BinaryExpr.createOp_('>', 4, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a > b;},
            left, right, ctx);
      }),
  LESSTHAN_EQUAL: wgxpath.BinaryExpr.createOp_(
      '<=', 4, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a <= b;},
            left, right, ctx);
      }),
  GREATERTHAN_EQUAL: wgxpath.BinaryExpr.createOp_('>=', 4,
      wgxpath.DataType.BOOLEAN, function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a >= b;},
            left, right, ctx);
      }),
  EQUAL: wgxpath.BinaryExpr.createOp_('=', 3, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a == b;},
            left, right, ctx, true);
      }),
  NOT_EQUAL: wgxpath.BinaryExpr.createOp_('!=', 3, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return wgxpath.BinaryExpr.compare_(function(a, b) {return a != b},
            left, right, ctx, true);
      }),
  AND: wgxpath.BinaryExpr.createOp_('and', 2, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return left.asBool(ctx) && right.asBool(ctx);
      }),
  OR: wgxpath.BinaryExpr.createOp_('or', 1, wgxpath.DataType.BOOLEAN,
      function(left, right, ctx) {
        return left.asBool(ctx) || right.asBool(ctx);
      })
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Context information about nodes in their nodeset.
 */

goog.provide('wgxpath.Context');



/**
 * Provides information for where something is in the DOM.
 *
 * @param {!wgxpath.Node} node A node in the DOM.
 * @param {number=} opt_position The position of this node in its nodeset,
 *     defaults to 1.
 * @param {number=} opt_last Index of the last node in this nodeset,
 *     defaults to 1.
 * @constructor
 */
wgxpath.Context = function(node, opt_position, opt_last) {

  /**
    * @private
    * @type {!wgxpath.Node}
    */
  this.node_ = node;

  /**
   * @private
   * @type {number}
   */
  this.position_ = opt_position || 1;

  /**
   * @private
   * @type {number} opt_last
   */
  this.last_ = opt_last || 1;
};


/**
 * Returns the node for this context object.
 *
 * @return {!wgxpath.Node} The node for this context object.
 */
wgxpath.Context.prototype.getNode = function() {
  return this.node_;
};


/**
 * Returns the position for this context object.
 *
 * @return {number} The position for this context object.
 */
wgxpath.Context.prototype.getPosition = function() {
  return this.position_;
};


/**
 * Returns the last field for this context object.
 *
 * @return {number} The last field for this context object.
 */
wgxpath.Context.prototype.getLast = function() {
  return this.last_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Enumeration of internal data types.
 */

goog.provide('wgxpath.DataType');


/**
 * Enum for data types.
 * @enum {number}
 */
wgxpath.DataType = {
  VOID: 0,
  NUMBER: 1,
  BOOLEAN: 2,
  STRING: 3,
  NODESET: 4
};
/*  JavaScript-XPath 0.1.11
 *  (c) 2007 Cybozu Labs, Inc.
 *
 *  JavaScript-XPath is freely distributable under the terms of an MIT-style
 *  license. For details, see the JavaScript-XPath web site:
 *  http://coderepos.org/share/wiki/JavaScript-XPath
 *
/*--------------------------------------------------------------------------*/

// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Exports the wgxpath.install function.
 *
 */

goog.require('wgxpath');

goog.exportSymbol('wgxpath.install', wgxpath.install);
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview An abstract class representing basic expressions.
 */

goog.provide('wgxpath.Expr');

goog.require('wgxpath.NodeSet');



/**
 * Abstract constructor for an XPath expression.
 *
 * @param {!wgxpath.DataType} dataType The data type that the expression
 *                                    will be evaluated into.
 * @constructor
 */
wgxpath.Expr = function(dataType) {

  /**
   * @type {!wgxpath.DataType}
   * @private
   */
  this.dataType_ = dataType;

  /**
   * @type {boolean}
   * @private
   */
  this.needContextPosition_ = false;

  /**
   * @type {boolean}
   * @private
   */
  this.needContextNode_ = false;

  /**
   * @type {?{name: string, valueExpr: wgxpath.Expr}}
   * @private
   */
  this.quickAttr_ = null;
};


/**
 * A default indentation for pretty printing.
 *
 * @const
 * @type {string}
 */
wgxpath.Expr.INDENT = '  ';


/**
 * Evaluates the expression.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {!(string|boolean|number|wgxpath.NodeSet)} The evaluation result.
 */
wgxpath.Expr.prototype.evaluate = goog.abstractMethod;


/**
 * Returns the string representation of the expression for debugging.
 *
 * @param {string=} opt_indent An optional indentation.
 * @return {string} The string representation.
 */
wgxpath.Expr.prototype.toString = goog.abstractMethod;


/**
 * Returns the data type of the expression.
 *
 * @return {!wgxpath.DataType} The data type that the expression
 *                            will be evaluated into.
 */
wgxpath.Expr.prototype.getDataType = function() {
  return this.dataType_;
};


/**
 * Returns whether the expression needs context position to be evaluated.
 *
 * @return {boolean} Whether context position is needed.
 */
wgxpath.Expr.prototype.doesNeedContextPosition = function() {
  return this.needContextPosition_;
};


/**
 * Sets whether the expression needs context position to be evaluated.
 *
 * @param {boolean} flag Whether context position is needed.
 */
wgxpath.Expr.prototype.setNeedContextPosition = function(flag) {
  this.needContextPosition_ = flag;
};


/**
 * Returns whether the expression needs context node to be evaluated.
 *
 * @return {boolean} Whether context node is needed.
 */
wgxpath.Expr.prototype.doesNeedContextNode = function() {
  return this.needContextNode_;
};


/**
 * Sets whether the expression needs context node to be evaluated.
 *
 * @param {boolean} flag Whether context node is needed.
 */
wgxpath.Expr.prototype.setNeedContextNode = function(flag) {
  this.needContextNode_ = flag;
};


/**
 * Returns the quick attribute information, if exists.
 *
 * @return {?{name: string, valueExpr: wgxpath.Expr}} The attribute
 *         information.
 */
wgxpath.Expr.prototype.getQuickAttr = function() {
  return this.quickAttr_;
};


/**
 * Sets up the quick attribute info.
 *
 * @param {?{name: string, valueExpr: wgxpath.Expr}} attrInfo The attribute
 *        information.
 */
wgxpath.Expr.prototype.setQuickAttr = function(attrInfo) {
  this.quickAttr_ = attrInfo;
};


/**
 * Evaluate and interpret the result as a number.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {number} The evaluated number value.
 */
wgxpath.Expr.prototype.asNumber = function(ctx) {
  var exrs = this.evaluate(ctx);
  if (exrs instanceof wgxpath.NodeSet) {
    return exrs.number();
  }
  return +exrs;
};


/**
 * Evaluate and interpret the result as a string.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {string} The evaluated string.
 */
wgxpath.Expr.prototype.asString = function(ctx) {
  var exrs = this.evaluate(ctx);
  if (exrs instanceof wgxpath.NodeSet) {
    return exrs.string();
  }
  return '' + exrs;
};


/**
 * Evaluate and interpret the result as a boolean value.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {boolean} The evaluated boolean value.
 */
wgxpath.Expr.prototype.asBool = function(ctx) {
  var exrs = this.evaluate(ctx);
  if (exrs instanceof wgxpath.NodeSet) {
    return !!exrs.getLength();
  }
  return !!exrs;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing operations on filter expressions.
 */

goog.provide('wgxpath.FilterExpr');

goog.require('wgxpath.Expr');



/**
 * Constructor for FilterExpr.
 *
 * @param {!wgxpath.Expr} primary The primary expression.
 * @param {!wgxpath.Predicates} predicates The predicates.
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.FilterExpr = function(primary, predicates) {
  if (predicates.getLength() && primary.getDataType() !=
      wgxpath.DataType.NODESET) {
    throw Error('Primary expression must evaluate to nodeset ' +
        'if filter has predicate(s).');
  }
  wgxpath.Expr.call(this, primary.getDataType());

  /**
   * @type {!wgxpath.Expr}
   * @private
   */
  this.primary_ = primary;


  /**
   * @type {!wgxpath.Predicates}
   * @private
   */
  this.predicates_ = predicates;

  this.setNeedContextPosition(primary.doesNeedContextPosition());
  this.setNeedContextNode(primary.doesNeedContextNode());
};
goog.inherits(wgxpath.FilterExpr, wgxpath.Expr);


/**
 * @override
 * @return {!wgxpath.NodeSet} The nodeset result.
 */
wgxpath.FilterExpr.prototype.evaluate = function(ctx) {
  var result = this.primary_.evaluate(ctx);
  return this.predicates_.evaluatePredicates(
      /** @type {!wgxpath.NodeSet} */ (result));
};


/**
 * @override
 */
wgxpath.FilterExpr.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'Filter: ' + '\n';
  indent += wgxpath.Expr.INDENT;
  text += this.primary_.toString(indent);
  text += this.predicates_.toString(indent);
  return text;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A function call expression.
 */

goog.provide('wgxpath.FunctionCall');

goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.NodeType');
goog.require('goog.string');
goog.require('wgxpath.Expr');
goog.require('wgxpath.Node');
goog.require('wgxpath.NodeSet');
goog.require('wgxpath.userAgent');



/**
 * A function call expression.
 *
 * @constructor
 * @extends {wgxpath.Expr}
 * @param {!wgxpath.FunctionCall.Func} func Function.
 * @param {!Array.<!wgxpath.Expr>} args Arguments to the function.
 */
wgxpath.FunctionCall = function(func, args) {
  // Check the provided arguments match the function parameters.
  if (args.length < func.minArgs_) {
    throw new Error('Function ' + func.name_ + ' expects at least' +
        func.minArgs_ + ' arguments, ' + args.length + ' given');
  }
  if (!goog.isNull(func.maxArgs_) && args.length > func.maxArgs_) {
    throw new Error('Function ' + func.name_ + ' expects at most ' +
        func.maxArgs_ + ' arguments, ' + args.length + ' given');
  }
  if (func.nodesetsRequired_) {
    goog.array.forEach(args, function(arg, i) {
      if (arg.getDataType() != wgxpath.DataType.NODESET) {
        throw new Error('Argument ' + i + ' to function ' + func.name_ +
            ' is not of type Nodeset: ' + arg);
      }
    });
  }
  wgxpath.Expr.call(this, func.dataType_);

  /**
   * @type {!wgxpath.FunctionCall.Func}
   * @private
   */
  this.func_ = func;

  /**
   * @type {!Array.<!wgxpath.Expr>}
   * @private
   */
  this.args_ = args;

  this.setNeedContextPosition(func.needContextPosition_ ||
      goog.array.some(args, function(arg) {
        return arg.doesNeedContextPosition();
      }));
  this.setNeedContextNode(
      (func.needContextNodeWithoutArgs_ && !args.length) ||
      (func.needContextNodeWithArgs_ && !!args.length) ||
      goog.array.some(args, function(arg) {
        return arg.doesNeedContextNode();
      }));
};
goog.inherits(wgxpath.FunctionCall, wgxpath.Expr);


/**
 * @override
 */
wgxpath.FunctionCall.prototype.evaluate = function(ctx) {
  var result = this.func_.evaluate_.apply(null,
      goog.array.concat(ctx, this.args_));
  return /** @type {!(string|boolean|number|wgxpath.NodeSet)} */ (result);
};


/**
 * @override
 */
wgxpath.FunctionCall.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'Function: ' + this.func_ + '\n';
  indent += wgxpath.Expr.INDENT;
  if (this.args_.length) {
    text += indent + 'Arguments:';
    indent += wgxpath.Expr.INDENT;
    text = goog.array.reduce(this.args_, function(prev, curr) {
      return prev + '\n' + curr.toString(indent);
    }, text);
  }
  return text;
};



/**
 * A function in a function call expression.
 *
 * @constructor
 * @param {string} name Name of the function.
 * @param {wgxpath.DataType} dataType Datatype of the function return value.
 * @param {boolean} needContextPosition Whether the function needs a context
 *     position.
 * @param {boolean} needContextNodeWithoutArgs Whether the function needs a
 *     context node when not given arguments.
 * @param {boolean} needContextNodeWithArgs Whether the function needs a context
 *     node when the function is given arguments.
 * @param {function(!wgxpath.Context, ...[!wgxpath.Expr]):*} evaluate
 *     Evaluates the function in a context with any number of expression
 *     arguments.
 * @param {number} minArgs Minimum number of arguments accepted by the function.
 * @param {?number=} opt_maxArgs Maximum number of arguments accepted by the
 *     function; null means there is no max; defaults to minArgs.
 * @param {boolean=} opt_nodesetsRequired Whether the args must be nodesets.
 * @private
 */
wgxpath.FunctionCall.Func_ = function(name, dataType, needContextPosition,
    needContextNodeWithoutArgs, needContextNodeWithArgs, evaluate, minArgs,
    opt_maxArgs, opt_nodesetsRequired) {

  /**
   * @type {string}
   * @private
   */
  this.name_ = name;

  /**
   * @type {wgxpath.DataType}
   * @private
   */
  this.dataType_ = dataType;

  /**
   * @type {boolean}
   * @private
   */
  this.needContextPosition_ = needContextPosition;

  /**
   * @type {boolean}
   * @private
   */
  this.needContextNodeWithoutArgs_ = needContextNodeWithoutArgs;

  /**
   * @type {boolean}
   * @private
   */
  this.needContextNodeWithArgs_ = needContextNodeWithArgs;

  /**
   * @type {function(!wgxpath.Context, ...[!wgxpath.Expr]):*}
   * @private
   */
  this.evaluate_ = evaluate;

  /**
   * @type {number}
   * @private
   */
  this.minArgs_ = minArgs;

  /**
   * @type {?number}
   * @private
   */
  this.maxArgs_ = goog.isDef(opt_maxArgs) ? opt_maxArgs : minArgs;

  /**
   * @type {boolean}
   * @private
   */
  this.nodesetsRequired_ = !!opt_nodesetsRequired;
};


/**
 * @override
 */
wgxpath.FunctionCall.Func_.prototype.toString = function() {
  return this.name_;
};


/**
 * A mapping from function names to Func objects.
 *
 * @private
 * @type {!Object.<string, !wgxpath.FunctionCall.Func>}
 */
wgxpath.FunctionCall.nameToFuncMap_ = {};


/**
 * Constructs a Func and maps its name to it.
 *
 * @param {string} name Name of the function.
 * @param {wgxpath.DataType} dataType Datatype of the function return value.
 * @param {boolean} needContextPosition Whether the function needs a context
 *     position.
 * @param {boolean} needContextNodeWithoutArgs Whether the function needs a
 *     context node when not given arguments.
 * @param {boolean} needContextNodeWithArgs Whether the function needs a context
 *     node when the function is given arguments.
 * @param {function(!wgxpath.Context, ...[!wgxpath.Expr]):*} evaluate
 *     Evaluates the function in a context with any number of expression
 *     arguments.
 * @param {number} minArgs Minimum number of arguments accepted by the function.
 * @param {?number=} opt_maxArgs Maximum number of arguments accepted by the
 *     function; null means there is no max; defaults to minArgs.
 * @param {boolean=} opt_nodesetsRequired Whether the args must be nodesets.
 * @return {!wgxpath.FunctionCall.Func} The function created.
 * @private
 */
wgxpath.FunctionCall.createFunc_ = function(name, dataType,
    needContextPosition, needContextNodeWithoutArgs, needContextNodeWithArgs,
    evaluate, minArgs, opt_maxArgs, opt_nodesetsRequired) {
  if (name in wgxpath.FunctionCall.nameToFuncMap_) {
    throw new Error('Function already created: ' + name + '.');
  }
  var func = new wgxpath.FunctionCall.Func_(name, dataType,
      needContextPosition, needContextNodeWithoutArgs, needContextNodeWithArgs,
      evaluate, minArgs, opt_maxArgs, opt_nodesetsRequired);
  func = (/** @type {!wgxpath.FunctionCall.Func} */ func);
  wgxpath.FunctionCall.nameToFuncMap_[name] = func;
  return func;
};


/**
 * Returns the function object for this name.
 *
 * @param {string} name The function's name.
 * @return {wgxpath.FunctionCall.Func} The function object.
 */
wgxpath.FunctionCall.getFunc = function(name) {
  return wgxpath.FunctionCall.nameToFuncMap_[name] || null;
};


/**
 * An XPath function enumeration.
 *
 * <p>A list of XPath 1.0 functions:
 * http://www.w3.org/TR/xpath/#corelib
 *
 * @enum {!Object}
 */
wgxpath.FunctionCall.Func = {
  BOOLEAN: wgxpath.FunctionCall.createFunc_('boolean',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx, expr) {
        return expr.asBool(ctx);
      }, 1),
  CEILING: wgxpath.FunctionCall.createFunc_('ceiling',
      wgxpath.DataType.NUMBER, false, false, false,
      function(ctx, expr) {
        return Math.ceil(expr.asNumber(ctx));
      }, 1),
  CONCAT: wgxpath.FunctionCall.createFunc_('concat',
      wgxpath.DataType.STRING, false, false, false,
      function(ctx, var_args) {
        var exprs = goog.array.slice(arguments, 1);
        return goog.array.reduce(exprs, function(prev, curr) {
          return prev + curr.asString(ctx);
        }, '');
      }, 2, null),
  CONTAINS: wgxpath.FunctionCall.createFunc_('contains',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx, expr1, expr2) {
        return goog.string.contains(expr1.asString(ctx), expr2.asString(ctx));
      }, 2),
  COUNT: wgxpath.FunctionCall.createFunc_('count',
      wgxpath.DataType.NUMBER, false, false, false,
      function(ctx, expr) {
        return expr.evaluate(ctx).getLength();
      }, 1, 1, true),
  FALSE: wgxpath.FunctionCall.createFunc_('false',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx) {
        return false;
      }, 0),
  FLOOR: wgxpath.FunctionCall.createFunc_('floor',
      wgxpath.DataType.NUMBER, false, false, false,
      function(ctx, expr) {
        return Math.floor(expr.asNumber(ctx));
      }, 1),
  ID: wgxpath.FunctionCall.createFunc_('id',
      wgxpath.DataType.NODESET, false, false, false,
      function(ctx, expr) {
        var ctxNode = ctx.getNode();
        var doc = ctxNode.nodeType == goog.dom.NodeType.DOCUMENT ? ctxNode :
            ctxNode.ownerDocument;
        var ids = expr.asString(ctx).split(/\s+/);
        var nsArray = [];
        goog.array.forEach(ids, function(id) {
          var elem = idSingle(id);
          if (elem && !goog.array.contains(nsArray, elem)) {
            nsArray.push(elem);
          }
        });
        nsArray.sort(goog.dom.compareNodeOrder);
        var ns = new wgxpath.NodeSet();
        goog.array.forEach(nsArray, function(n) {
          ns.add(n);
        });
        return ns;

        function idSingle(id) {
          if (wgxpath.userAgent.IE_DOC_PRE_9) {
            var allId = doc.all[id];
            if (allId) {
              if (allId.nodeType && id == allId.id) {
                return allId;
              } else if (allId.length) {
                return goog.array.find(allId, function(elem) {
                  return id == elem.id;
                });
              }
            }
            return null;
          } else {
            return doc.getElementById(id);
          }
        }
      }, 1),
  LANG: wgxpath.FunctionCall.createFunc_('lang',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx, expr) {
        // TODO(user): Fully implement this.
        return false;
      }, 1),
  LAST: wgxpath.FunctionCall.createFunc_('last',
      wgxpath.DataType.NUMBER, true, false, false,
      function(ctx) {
        if (arguments.length != 1) {
          throw Error('Function last expects ()');
        }
        return ctx.getLast();
      }, 0),
  LOCAL_NAME: wgxpath.FunctionCall.createFunc_('local-name',
      wgxpath.DataType.STRING, false, true, false,
      function(ctx, opt_expr) {
        var node = opt_expr ? opt_expr.evaluate(ctx).getFirst() : ctx.getNode();
        return node ? node.nodeName.toLowerCase() : '';
      }, 0, 1, true),
  NAME: wgxpath.FunctionCall.createFunc_('name',
      wgxpath.DataType.STRING, false, true, false,
      function(ctx, opt_expr) {
        // TODO(user): Fully implement this.
        var node = opt_expr ? opt_expr.evaluate(ctx).getFirst() : ctx.getNode();
        return node ? node.nodeName.toLowerCase() : '';
      }, 0, 1, true),
  NAMESPACE_URI: wgxpath.FunctionCall.createFunc_('namespace-uri',
      wgxpath.DataType.STRING, true, false, false,
      function(ctx, opt_expr) {
        // TODO(user): Fully implement this.
        return '';
      }, 0, 1, true),
  NORMALIZE_SPACE: wgxpath.FunctionCall.createFunc_('normalize-space',
      wgxpath.DataType.STRING, false, true, false,
      function(ctx, opt_expr) {
        var str = opt_expr ? opt_expr.asString(ctx) :
            wgxpath.Node.getValueAsString(ctx.getNode());
        return goog.string.collapseWhitespace(str);
      }, 0, 1),
  NOT: wgxpath.FunctionCall.createFunc_('not',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx, expr) {
        return !expr.asBool(ctx);
      }, 1),
  NUMBER: wgxpath.FunctionCall.createFunc_('number',
      wgxpath.DataType.NUMBER, false, true, false,
      function(ctx, opt_expr) {
        return opt_expr ? opt_expr.asNumber(ctx) :
            wgxpath.Node.getValueAsNumber(ctx.getNode());
      }, 0, 1),
  POSITION: wgxpath.FunctionCall.createFunc_('position',
      wgxpath.DataType.NUMBER, true, false, false,
      function(ctx) {
        return ctx.getPosition();
      }, 0),
  ROUND: wgxpath.FunctionCall.createFunc_('round',
      wgxpath.DataType.NUMBER, false, false, false,
      function(ctx, expr) {
        return Math.round(expr.asNumber(ctx));
      }, 1),
  STARTS_WITH: wgxpath.FunctionCall.createFunc_('starts-with',
      wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx, expr1, expr2) {
        return goog.string.startsWith(expr1.asString(ctx), expr2.asString(ctx));
      }, 2),
  STRING: wgxpath.FunctionCall.createFunc_(
      'string', wgxpath.DataType.STRING, false, true, false,
      function(ctx, opt_expr) {
        return opt_expr ? opt_expr.asString(ctx) :
            wgxpath.Node.getValueAsString(ctx.getNode());
      }, 0, 1),
  STRING_LENGTH: wgxpath.FunctionCall.createFunc_('string-length',
      wgxpath.DataType.NUMBER, false, true, false,
      function(ctx, opt_expr) {
        var str = opt_expr ? opt_expr.asString(ctx) :
            wgxpath.Node.getValueAsString(ctx.getNode());
        return str.length;
      }, 0, 1),
  SUBSTRING: wgxpath.FunctionCall.createFunc_('substring',
      wgxpath.DataType.STRING, false, false, false,
      function(ctx, expr1, expr2, opt_expr3) {
        var startRaw = expr2.asNumber(ctx);
        if (isNaN(startRaw) || startRaw == Infinity || startRaw == -Infinity) {
          return '';
        }
        var lengthRaw = opt_expr3 ? opt_expr3.asNumber(ctx) : Infinity;
        if (isNaN(lengthRaw) || lengthRaw === -Infinity) {
          return '';
        }

        // XPath indices are 1-based.
        var startInt = Math.round(startRaw) - 1;
        var start = Math.max(startInt, 0);
        var str = expr1.asString(ctx);

        if (lengthRaw == Infinity) {
          return str.substring(start);
        } else {
          var lengthInt = Math.round(lengthRaw);
          // Length is from startInt, not start!
          return str.substring(start, startInt + lengthInt);
        }
      }, 2, 3),
  SUBSTRING_AFTER: wgxpath.FunctionCall.createFunc_('substring-after',
      wgxpath.DataType.STRING, false, false, false,
      function(ctx, expr1, expr2) {
        var str1 = expr1.asString(ctx);
        var str2 = expr2.asString(ctx);
        var str2Index = str1.indexOf(str2);
        return str2Index == -1 ? '' : str1.substring(str2Index + str2.length);
      }, 2),
  SUBSTRING_BEFORE: wgxpath.FunctionCall.createFunc_('substring-before',
      wgxpath.DataType.STRING, false, false, false,
      function(ctx, expr1, expr2) {
        var str1 = expr1.asString(ctx);
        var str2 = expr2.asString(ctx);
        var str2Index = str1.indexOf(str2);
        return str2Index == -1 ? '' : str1.substring(0, str2Index);
      }, 2),
  SUM: wgxpath.FunctionCall.createFunc_('sum',
      wgxpath.DataType.NUMBER, false, false, false,
      function(ctx, expr) {
        var ns = expr.evaluate(ctx);
        var iter = ns.iterator();
        var prev = 0;
        for (var node = iter.next(); node; node = iter.next()) {
          prev += wgxpath.Node.getValueAsNumber(node);
        }
        return prev;
      }, 1, 1, true),
  TRANSLATE: wgxpath.FunctionCall.createFunc_('translate',
      wgxpath.DataType.STRING, false, false, false,
      function(ctx, expr1, expr2, expr3) {
        var str1 = expr1.asString(ctx);
        var str2 = expr2.asString(ctx);
        var str3 = expr3.asString(ctx);

        var map = [];
        for (var i = 0; i < str2.length; i++) {
          var ch = str2.charAt(i);
          if (!(ch in map)) {
            // If i >= str3.length, charAt will return the empty string.
            map[ch] = str3.charAt(i);
          }
        }

        var translated = '';
        for (var i = 0; i < str1.length; i++) {
          var ch = str1.charAt(i);
          translated += (ch in map) ? map[ch] : ch;
        }
        return translated;
      }, 3),
  TRUE: wgxpath.FunctionCall.createFunc_(
      'true', wgxpath.DataType.BOOLEAN, false, false, false,
      function(ctx) {
        return true;
      }, 0)
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Wrapper classes for attribute nodes in old IE browsers.
 */

goog.provide('wgxpath.IEAttrWrapper');

goog.require('goog.dom.NodeType');
goog.require('wgxpath.userAgent');



/**
 * A wrapper for an attribute node in old IE.
 *
 * <p> Note: Although sourceIndex is equal to node.sourceIndex, it is
 * denormalized into a separate parameter for performance, so that clients
 * constructing multiple IEAttrWrappers can pass in the same sourceIndex
 * rather than re-querying it each time.
 *
 * @constructor
 * @extends {Attr}
 * @param {!Node} node The attribute node.
 * @param {!Node} parent The parent of the attribute node.
 * @param {string} nodeName The name of the attribute node.
 * @param {(string|number|boolean)} nodeValue The value of the attribute node.
 * @param {number} sourceIndex The source index of the parent node.
 */
wgxpath.IEAttrWrapper = function(node, parent, nodeName, nodeValue,
    sourceIndex) {
  /**
   * @type {!Node}
   * @private
   */
  this.node_ = node;

  /**
   * @type {string}
   */
  this.nodeName = nodeName;

  /**
   * @type {(string|number|boolean)}
   */
  this.nodeValue = nodeValue;

  /**
   * @type {goog.dom.NodeType}
   */
  this.nodeType = goog.dom.NodeType.ATTRIBUTE;

  /**
   * @type {!Node}
   */
  this.ownerElement = parent;

  /**
   * @type {number}
   * @private
   */
  this.parentSourceIndex_ = sourceIndex;

  /**
   * @type {!Node}
   */
  this.parentNode = parent;
};


/**
 * Creates a wrapper for an attribute node in old IE.
 *
 * @param {!Node} parent The parent of the attribute node.
 * @param {!Node} attr The attribute node.
 * @param {number} sourceIndex The source index of the parent node.
 * @return {!wgxpath.IEAttrWrapper} The constcuted wrapper.
 */
wgxpath.IEAttrWrapper.forAttrOf = function(parent, attr, sourceIndex) {
  var nodeValue = (wgxpath.userAgent.IE_DOC_PRE_8 && attr.nodeName == 'href') ?
      parent.getAttribute(attr.nodeName, 2) : attr.nodeValue;
  return new wgxpath.IEAttrWrapper(attr, parent, attr.nodeName, nodeValue,
      sourceIndex);
};


/**
 * Creates a wrapper for a style attribute node in old IE.
 *
 * @param {!Node} parent The parent of the attribute node.
 * @param {number} sourceIndex The source index of the parent node.
 * @return {!wgxpath.IEAttrWrapper} The constcuted wrapper.
 */
wgxpath.IEAttrWrapper.forStyleOf = function(parent, sourceIndex) {
  return new wgxpath.IEAttrWrapper(parent.style, parent, 'style',
      parent.style.cssText, sourceIndex);
};


/**
 * Returns the source index of the parent of the attribute node.
 *
 * @return {number} The source index of the parent.
 */
wgxpath.IEAttrWrapper.prototype.getParentSourceIndex = function() {
  return this.parentSourceIndex_;
};


/**
 * Returns the attribute node contained in the wrapper.
 *
 * @return {!Node} The original attribute node.
 */
wgxpath.IEAttrWrapper.prototype.getNode = function() {
  return this.node_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class implementing the xpath 1.0 subset of the
 *               KindTest construct.
 */

goog.provide('wgxpath.KindTest');

goog.require('goog.dom.NodeType');
goog.require('wgxpath.NodeTest');



/**
 * Constructs a subset of KindTest based on the xpath grammar:
 * http://www.w3.org/TR/xpath20/#prod-xpath-KindTest
 *
 * @param {string} typeName Type name to be tested.
 * @param {wgxpath.Literal=} opt_literal Optional literal for
 *        processing-instruction nodes.
 * @constructor
 * @implements {wgxpath.NodeTest}
 */
wgxpath.KindTest = function(typeName, opt_literal) {

  /**
   * @type {string}
   * @private
   */
  this.typeName_ = typeName;

  /**
   * @type {wgxpath.Literal}
   * @private
   */
  this.literal_ = goog.isDef(opt_literal) ? opt_literal : null;

  /**
   * @type {?goog.dom.NodeType}
   * @private
   */
  this.type_ = null;
  switch (typeName) {
    case 'comment':
      this.type_ = goog.dom.NodeType.COMMENT;
      break;
    case 'text':
      this.type_ = goog.dom.NodeType.TEXT;
      break;
    case 'processing-instruction':
      this.type_ = goog.dom.NodeType.PROCESSING_INSTRUCTION;
      break;
    case 'node':
      break;
    default:
      throw Error('Unexpected argument');
  }
};


/**
 * Checks if a type name is a valid KindTest parameter.
 *
 * @param {string} typeName The type name to be checked.
 * @return {boolean} Whether the type name is legal.
 */
wgxpath.KindTest.isValidType = function(typeName) {
  return typeName == 'comment' || typeName == 'text' ||
      typeName == 'processing-instruction' || typeName == 'node';
};


/**
 * @override
 */
wgxpath.KindTest.prototype.matches = function(node) {
  return goog.isNull(this.type_) || this.type_ == node.nodeType;
};


/**
 * Returns the type of the node.
 *
 * @return {?number} The type of the node, or null if any type.
 */
wgxpath.KindTest.prototype.getType = function() {
  return this.type_;
};


/**
 * @override
 */
wgxpath.KindTest.prototype.getName = function() {
  return this.typeName_;
};


/**
 * @override
 * @param {string=} opt_indent Optional indentation.
 * @return {string} The string representation.
 */
wgxpath.KindTest.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'kindtest: ' + this.typeName_;
  if (!goog.isNull(this.literal_)) {
    text += '\n' + this.literal_.toString(indent + wgxpath.Expr.INDENT);
  }
  return text;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview The lexer class for tokenizing xpath expressions.
 */

goog.provide('wgxpath.Lexer');



/**
 * Constructs a lexer.
 *
 * @param {!Array.<string>} tokens Tokens to iterate over.
 * @constructor
 */
wgxpath.Lexer = function(tokens) {
  /**
   * @type {!Array.<string>}
   * @private
   */
  this.tokens_ = tokens;

  /**
   * @type {number}
   * @private
   */
  this.index_ = 0;
};


/**
 * Tokenizes a source string into an array of tokens.
 *
 * @param {string} source Source string to tokenize.
 * @return {!wgxpath.Lexer} Essentially an iterator over the tokens.
 */
wgxpath.Lexer.tokenize = function(source) {
  var tokens = source.match(wgxpath.Lexer.TOKEN_);

  // Removes tokens starting with whitespace from the array.
  for (var i = 0; i < tokens.length; i++) {
    if (wgxpath.Lexer.LEADING_WHITESPACE_.test(tokens[i])) {
      tokens.splice(i, 1);
    }
  }
  return new wgxpath.Lexer(tokens);
};


/**
 * Regular expressions to match XPath productions.
 *
 * @const
 * @type {!RegExp}
 * @private
 */
wgxpath.Lexer.TOKEN_ = new RegExp(
    '\\$?(?:(?![0-9-])[\\w-]+:)?(?![0-9-])[\\w-]+' +
        // Nodename (possibly with namespace) or variable.
    '|\\/\\/' + // Double slash.
    '|\\.\\.' + // Double dot.
    '|::' + // Double colon.
    '|\\d+(?:\\.\\d*)?' + // Number starting with digit.
    '|\\.\\d+' + // Number starting with decimal point.
    '|"[^"]*"' + // Double quoted string.
    '|\'[^\']*\'' + // Single quoted string.
    '|[!<>]=' + // Operators
    '|\\s+' + // Whitespaces.
    '|.', // Any single character.
    'g');


/**
 * Regex to check if a string starts with a whitespace character.
 *
 * @const
 * @type {!RegExp}
 * @private
 */
wgxpath.Lexer.LEADING_WHITESPACE_ = /^\s/;


/**
 * Peeks at the lexer. An optional index can be
 * used to specify the token peek at.
 *
 * @param {number=} opt_i Index to peek at. Defaults to zero.
 * @return {string} Token peeked.
 */
wgxpath.Lexer.prototype.peek = function(opt_i) {
  return this.tokens_[this.index_ + (opt_i || 0)];
};


/**
 * Returns the next token from the lexer and increments the index.
 *
 * @return {string} The next token.
 */
wgxpath.Lexer.prototype.next = function() {
  return this.tokens_[this.index_++];
};


/**
 * Decrements the index by one.
 */
wgxpath.Lexer.prototype.back = function() {
  this.index_--;
};


/**
 * Checks whether the lexer is empty.
 *
 * @return {boolean} Whether the lexer is empty.
 */
wgxpath.Lexer.prototype.empty = function() {
  return this.tokens_.length <= this.index_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing the string literals.
 */

goog.provide('wgxpath.Literal');

goog.require('wgxpath.Expr');



/**
 * Constructs a string literal expression.
 *
 * @param {string} text The text value of the literal.
 * @constructor
 * @extends {wgxpath.Expr}
 */
wgxpath.Literal = function(text) {
  wgxpath.Expr.call(this, wgxpath.DataType.STRING);

  /**
   * @type {string}
   * @private
   */
  this.text_ = text.substring(1, text.length - 1);
};
goog.inherits(wgxpath.Literal, wgxpath.Expr);


/**
 * @override
 * @return {string} The string result.
 */
wgxpath.Literal.prototype.evaluate = function(context) {
  return this.text_;
};


/**
 * @override
 */
wgxpath.Literal.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  return indent + 'literal: ' + this.text_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class implementing the NameTest construct.
 */

goog.provide('wgxpath.NameTest');

goog.require('goog.dom.NodeType');



/**
 * Constructs a NameTest based on the xpath grammar:
 * http://www.w3.org/TR/xpath/#NT-NameTest
 *
 * @param {string} name Name to be tested.
 * @constructor
 * @implements {wgxpath.NodeTest}
 */
wgxpath.NameTest = function(name) {
  /**
   * @type {string}
   * @private
   */
  this.name_ = name.toLowerCase();
};


/**
 * The default namespace for XHTML nodes.
 *
 * @const
 * @type {string}
 * @private
 */
wgxpath.NameTest.HTML_NAMESPACE_ = 'http://www.w3.org/1999/xhtml';


/**
 * @override
 */
wgxpath.NameTest.prototype.matches = function(node) {
  var type = node.nodeType;
  if (type == goog.dom.NodeType.ELEMENT ||
      type == goog.dom.NodeType.ATTRIBUTE) {
    if (this.name_ == '*' || this.name_ == node.nodeName.toLowerCase()) {
      return true;
    } else {
      var namespace = node.namespaceURI || wgxpath.NameTest.HTML_NAMESPACE_;
      return this.name_ == namespace + ':*';
    }
  }
};


/**
 * @override
 */
wgxpath.NameTest.prototype.getName = function() {
  return this.name_;
};


/**
 * @override
 * @param {string=} opt_indent Optional indentation.
 * @return {string} The string representation.
 */
wgxpath.NameTest.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  return indent + 'nametest: ' + this.name_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Node utilities.
 */

goog.provide('wgxpath.Node');

goog.require('goog.array');
goog.require('goog.dom.NodeType');
goog.require('goog.userAgent');
goog.require('wgxpath.IEAttrWrapper');
goog.require('wgxpath.userAgent');


/** @typedef {!(Node|wgxpath.IEAttrWrapper)} */
wgxpath.Node = {};


/**
 * Returns whether two nodes are equal.
 *
 * @param {wgxpath.Node} a The first node.
 * @param {wgxpath.Node} b The second node.
 * @return {boolean} Whether the nodes are equal.
 */
wgxpath.Node.equal = function(a, b) {
  return (a == b) || (a instanceof wgxpath.IEAttrWrapper &&
      b instanceof wgxpath.IEAttrWrapper && a.getNode() ==
      b.getNode());
};


/**
 * Returns the string-value of the required type from a node.
 *
 * @param {!wgxpath.Node} node The node to get value from.
 * @return {string} The value required.
 */
wgxpath.Node.getValueAsString = function(node) {
  var t = null, type = node.nodeType;
  // Old IE title problem.
  var needTitleFix = function(node) {
    return wgxpath.userAgent.IE_DOC_PRE_9 &&
        node.nodeName.toLowerCase() == 'title';
  };
  // goog.dom.getTextContent doesn't seem to work
  if (type == goog.dom.NodeType.ELEMENT) {
    t = node.textContent;
    t = (t == undefined || t == null) ? node.innerText : t;
    t = (t == undefined || t == null) ? '' : t;
  }
  if (typeof t != 'string') {
    if (needTitleFix(node) && type == goog.dom.NodeType.ELEMENT) {
      t = node.text;
    } else if (type == goog.dom.NodeType.DOCUMENT ||
        type == goog.dom.NodeType.ELEMENT) {
      node = (type == goog.dom.NodeType.DOCUMENT) ?
          node.documentElement : node.firstChild;
      var i = 0, stack = [];
      for (t = ''; node;) {
        do {
          if (node.nodeType != goog.dom.NodeType.ELEMENT) {
            t += node.nodeValue;
          }
          if (needTitleFix(node)) {
            t += node.text;
          }
          stack[i++] = node; // push
        } while (node = node.firstChild);
        while (i && !(node = stack[--i].nextSibling)) {}
      }
    } else {
      t = node.nodeValue;
    }
  }
  return '' + t;
};


/**
 * Returns the string-value of the required type from a node, casted to number.
 *
 * @param {!wgxpath.Node} node The node to get value from.
 * @return {number} The value required.
 */
wgxpath.Node.getValueAsNumber = function(node) {
  return +wgxpath.Node.getValueAsString(node);
};


/**
 * Returns the string-value of the required type from a node, casted to boolean.
 *
 * @param {!wgxpath.Node} node The node to get value from.
 * @return {boolean} The value required.
 */
wgxpath.Node.getValueAsBool = function(node) {
  return !!wgxpath.Node.getValueAsString(node);
};


/**
 * Returns if the attribute matches the given value.
 *
 * @param {!wgxpath.Node} node The node to get value from.
 * @param {?string} name The attribute name to match, if any.
 * @param {?string} value The attribute value to match, if any.
 * @return {boolean} Whether the node matches the attribute, if any.
 */
wgxpath.Node.attrMatches = function(node, name, value) {
  // No attribute.
  if (goog.isNull(name)) {
    return true;
  }
  // TODO(user): If possible, figure out why this throws an exception in some
  // cases on IE < 9.
  try {
    if (!node.getAttribute) {
      return false;
    }
  } catch (e) {
    return false;
  }
  if (wgxpath.userAgent.IE_DOC_PRE_8 && name == 'class') {
    name = 'className';
  }
  return value == null ? !!node.getAttribute(name) :
      (node.getAttribute(name, 2) == value);
};


/**
 * Returns the descendants of a node.
 *
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get descendants from.
 * @param {?string=} opt_attrName The attribute name to match, if any.
 * @param {?string=} opt_attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet=} opt_nodeset The node set to add descendants to.
 * @return {!wgxpath.NodeSet} The nodeset with descendants.
 */
wgxpath.Node.getDescendantNodes = function(test, node, opt_attrName,
    opt_attrValue, opt_nodeset) {
  var nodeset = opt_nodeset || new wgxpath.NodeSet();
  var func = wgxpath.userAgent.IE_DOC_PRE_9 ?
      wgxpath.Node.getDescendantNodesIEPre9_ :
      wgxpath.Node.getDescendantNodesGeneric_;
  var attrName = goog.isString(opt_attrName) ? opt_attrName : null;
  var attrValue = goog.isString(opt_attrValue) ? opt_attrValue : null;
  return func.call(null, test, node, attrName, attrValue, nodeset);
};


/**
 * Returns the descendants of a node for IE.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get descendants from.
 * @param {?string} attrName The attribute name to match, if any.
 * @param {?string} attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet} nodeset The node set to add descendants to.
 * @return {!wgxpath.NodeSet} The nodeset with descendants.
 */
wgxpath.Node.getDescendantNodesIEPre9_ = function(test, node, attrName,
    attrValue, nodeset) {
  if (wgxpath.Node.doesNeedSpecialHandlingIEPre9_(test, attrName)) {
    var descendants = node.all;
    if (!descendants) {
      return nodeset;
    }
    var name = wgxpath.Node.getNameFromTestIEPre9_(test);
    // all.tags not working.
    if (name != '*') {
      descendants = node.getElementsByTagName(name);
      if (!descendants) {
        return nodeset;
      }
    }
    if (attrName) {
      /**
       * The length property of the "all" collection is overwritten
       * if there exists an element with id="length", therefore we
       * have to iterate without knowing the length.
       */
      var result = [];
      var i = 0;
      while (node = descendants[i++]) {
        if (wgxpath.Node.attrMatches(node, attrName, attrValue)) {
          result.push(node);
        }
      }
      descendants = result;
    }
    var i = 0;
    while (node = descendants[i++]) {
      if (name != '*' || node.tagName != '!') {
        nodeset.add(node);
      }
    }
    return nodeset;
  }
  wgxpath.Node.doRecursiveAttrMatch_(test, node, attrName,
      attrValue, nodeset);
  return nodeset;
};


/**
 * Returns the descendants of a node for browsers other than IE.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get descendants from.
 * @param {?string} attrName The attribute name to match, if any.
 * @param {?string} attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet} nodeset The node set to add descendants to.
 * @return {!wgxpath.NodeSet} The nodeset with descendants.
 */
wgxpath.Node.getDescendantNodesGeneric_ = function(test, node,
    attrName, attrValue, nodeset) {
  if (node.getElementsByName && attrValue && attrName == 'name' &&
      !goog.userAgent.IE) {
    var nodes = node.getElementsByName(attrValue);
    goog.array.forEach(nodes, function(node) {
      if (test.matches(node)) {
        nodeset.add(node);
      }
    });
  } else if (node.getElementsByClassName && attrValue && attrName == 'class') {
    var nodes = node.getElementsByClassName(attrValue);
    goog.array.forEach(nodes, function(node) {
      if (node.className == attrValue && test.matches(node)) {
        nodeset.add(node);
      }
    });
  } else if (test instanceof wgxpath.KindTest) {
    wgxpath.Node.doRecursiveAttrMatch_(test, node, attrName,
        attrValue, nodeset);
  } else if (node.getElementsByTagName) {
    var nodes = node.getElementsByTagName(test.getName());
    goog.array.forEach(nodes, function(node) {
      if (wgxpath.Node.attrMatches(node, attrName, attrValue)) {
        nodeset.add(node);
      }
    });
  }
  return nodeset;
};


/**
 * Returns the child nodes of a node.
 *
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get child nodes from.
 * @param {?string=} opt_attrName The attribute name to match, if any.
 * @param {?string=} opt_attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet=} opt_nodeset The node set to add child nodes to.
 * @return {!wgxpath.NodeSet} The nodeset with child nodes.
 */
wgxpath.Node.getChildNodes = function(test, node,
    opt_attrName, opt_attrValue, opt_nodeset) {
  var nodeset = opt_nodeset || new wgxpath.NodeSet();
  var func = wgxpath.userAgent.IE_DOC_PRE_9 ?
      wgxpath.Node.getChildNodesIEPre9_ : wgxpath.Node.getChildNodesGeneric_;
  var attrName = goog.isString(opt_attrName) ? opt_attrName : null;
  var attrValue = goog.isString(opt_attrValue) ? opt_attrValue : null;
  return func.call(null, test, node, attrName, attrValue, nodeset);
};


/**
 * Returns the child nodes of a node for IE browsers.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get child nodes from.
 * @param {?string} attrName The attribute name to match, if any.
 * @param {?string} attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet} nodeset The node set to add child nodes to.
 * @return {!wgxpath.NodeSet} The nodeset with child nodes.
 */
wgxpath.Node.getChildNodesIEPre9_ = function(test, node,
    attrName, attrValue, nodeset) {
  var children;
  if (wgxpath.Node.doesNeedSpecialHandlingIEPre9_(test, attrName) &&
      (children = node.childNodes)) { // node.children seems buggy.
    var name = wgxpath.Node.getNameFromTestIEPre9_(test);
    if (name != '*') {
      //children = children.tags(name); // children.tags seems buggy.
      children = goog.array.filter(children, function(child) {
        return child.tagName && child.tagName.toLowerCase() == name;
      });
      if (!children) {
        return nodeset;
      }
    }
    if (attrName) {
      // TODO(user): See if an optimization is possible.
      children = goog.array.filter(children, function(n) {
        return wgxpath.Node.attrMatches(n, attrName, attrValue);
      });
    }
    goog.array.forEach(children, function(node) {
      if (name != '*' || node.tagName != '!' &&
          !(name == '*' && node.nodeType != goog.dom.NodeType.ELEMENT)) {
        nodeset.add(node);
      }
    });
    return nodeset;
  }
  return wgxpath.Node.getChildNodesGeneric_(test, node, attrName,
      attrValue, nodeset);
};


/**
 * Returns the child nodes of a node genericly.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The node to get child nodes from.
 * @param {?string} attrName The attribute name to match, if any.
 * @param {?string} attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet} nodeset The node set to add child nodes to.
 * @return {!wgxpath.NodeSet} The nodeset with child nodes.
 */
wgxpath.Node.getChildNodesGeneric_ = function(test, node, attrName,
    attrValue, nodeset) {
  for (var current = node.firstChild; current; current = current.nextSibling) {
    if (wgxpath.Node.attrMatches(current, attrName, attrValue)) {
      if (test.matches(current)) {
        nodeset.add(current);
      }
    }
  }
  return nodeset;
};


/**
 * Returns whether a getting descendants/children call
 * needs special handling on IE browsers.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {!wgxpath.Node} node The root node to start the recursive call on.
 * @param {?string} attrName The attribute name to match, if any.
 * @param {?string} attrValue The attribute value to match, if any.
 * @param {!wgxpath.NodeSet} nodeset The NodeSet to add nodes to.
 */
wgxpath.Node.doRecursiveAttrMatch_ = function(test, node,
    attrName, attrValue, nodeset) {
  for (var n = node.firstChild; n; n = n.nextSibling) {
    if (wgxpath.Node.attrMatches(n, attrName, attrValue) &&
        test.matches(n)) {
      nodeset.add(n);
    }
    wgxpath.Node.doRecursiveAttrMatch_(test, n, attrName,
        attrValue, nodeset);
  }
};


/**
 * Returns whether a getting descendants/children call
 * needs special handling on IE browsers.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest for matching nodes.
 * @param {?string} attrName The attribute name to match, if any.
 * @return {boolean} Whether the call needs special handling.
 */
wgxpath.Node.doesNeedSpecialHandlingIEPre9_ = function(test, attrName) {
  return test instanceof wgxpath.NameTest ||
      test.getType() == goog.dom.NodeType.COMMENT ||
      (!!attrName && goog.isNull(test.getType()));
};


/**
 * Returns a fixed name of a NodeTest for IE browsers.
 *
 * @private
 * @param {!wgxpath.NodeTest} test A NodeTest.
 * @return {string} The name of the NodeTest.
 */
wgxpath.Node.getNameFromTestIEPre9_ = function(test) {
  if (test instanceof wgxpath.KindTest) {
    if (test.getType() == goog.dom.NodeType.COMMENT) {
      return '!';
    } else if (goog.isNull(test.getType())) {
      return '*';
    }
  }
  return test.getName();
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Context information about nodes in their nodeset.
 */

goog.provide('wgxpath.NodeSet');

goog.require('goog.dom');
goog.require('wgxpath.Node');



/**
 * A set of nodes sorted by their prefix order in the document.
 *
 * @constructor
 */
wgxpath.NodeSet = function() {
  // In violation of standard Closure practice, we initialize properties to
  // immutable constants in the constructor instead of on the prototype,
  // because we have empirically measured better performance by doing so.

  /**
   * A pointer to the first node in the linked list.
   *
   * @private
   * @type {wgxpath.NodeSet.Entry_}
   */
  this.first_ = null;

  /**
   * A pointer to the last node in the linked list.
   *
   * @private
   * @type {wgxpath.NodeSet.Entry_}
   */
  this.last_ = null;

  /**
   * Length of the linked list.
   *
   * @private
   * @type {number}
   */
  this.length_ = 0;
};



/**
 * A entry for a node in a linked list
 *
 * @param {!wgxpath.Node} node The node to be added.
 * @constructor
 * @private
 */
wgxpath.NodeSet.Entry_ = function(node) {
  // In violation of standard Closure practice, we initialize properties to
  // immutable constants in the constructor instead of on the prototype,
  // because we have empirically measured better performance by doing so.

  /**
   * @type {!wgxpath.Node}
   */
  this.node = node;

  /**
   * @type {wgxpath.NodeSet.Entry_}
   */
  this.prev = null;

  /**
   * @type {wgxpath.NodeSet.Entry_}
   */
  this.next = null;
};


/**
 * Merges two nodesets, removing duplicates. This function may modify both
 * nodesets, and will return a reference to one of the two.
 *
 * <p> Note: We assume that the two nodesets are already sorted in DOM order.
 *
 * @param {!wgxpath.NodeSet} a The first nodeset.
 * @param {!wgxpath.NodeSet} b The second nodeset.
 * @return {!wgxpath.NodeSet} The merged nodeset.
 */
wgxpath.NodeSet.merge = function(a, b) {
  if (!a.first_) {
    return b;
  } else if (!b.first_) {
    return a;
  }
  var aCurr = a.first_;
  var bCurr = b.first_;
  var merged = a, tail = null, next = null, length = 0;
  while (aCurr && bCurr) {
    if (wgxpath.Node.equal(aCurr.node, bCurr.node)) {
      next = aCurr;
      aCurr = aCurr.next;
      bCurr = bCurr.next;
    } else {
      var compareResult = goog.dom.compareNodeOrder(
          /** @type {!Node} */ (aCurr.node),
          /** @type {!Node} */ (bCurr.node));
      if (compareResult > 0) {
        next = bCurr;
        bCurr = bCurr.next;
      } else {
        next = aCurr;
        aCurr = aCurr.next;
      }
    }
    next.prev = tail;
    if (tail) {
      tail.next = next;
    } else {
      merged.first_ = next;
    }
    tail = next;
    length++;
  }
  next = aCurr || bCurr;
  while (next) {
    next.prev = tail;
    tail.next = next;
    tail = next;
    length++;
    next = next.next;
  }
  merged.last_ = tail;
  merged.length_ = length;
  return merged;
};


/**
 * Prepends a node to this nodeset.
 *
 * @param {!wgxpath.Node} node The node to be added.
 */
wgxpath.NodeSet.prototype.unshift = function(node) {
  var entry = new wgxpath.NodeSet.Entry_(node);
  entry.next = this.first_;
  if (!this.last_) {
    this.first_ = this.last_ = entry;
  } else {
    this.first_.prev = entry;
  }
  this.first_ = entry;
  this.length_++;
};


/**
 * Adds a node to this nodeset.
 *
 * @param {!wgxpath.Node} node The node to be added.
 */
wgxpath.NodeSet.prototype.add = function(node) {
  var entry = new wgxpath.NodeSet.Entry_(node);
  entry.prev = this.last_;
  if (!this.first_) {
    this.first_ = this.last_ = entry;
  } else {
    this.last_.next = entry;
  }
  this.last_ = entry;
  this.length_++;
};


/**
 * Returns the first node of the nodeset.
 *
 * @return {?wgxpath.Node} The first node of the nodeset
                                     if the nodeset is non-empty;
 *     otherwise null.
 */
wgxpath.NodeSet.prototype.getFirst = function() {
  var first = this.first_;
  if (first) {
    return first.node;
  } else {
    return null;
  }
};


/**
 * Return the length of this nodeset.
 *
 * @return {number} The length of the nodeset.
 */
wgxpath.NodeSet.prototype.getLength = function() {
  return this.length_;
};


/**
 * Returns the string representation of this nodeset.
 *
 * @return {string} The string representation of this nodeset.
 */
wgxpath.NodeSet.prototype.string = function() {
  var node = this.getFirst();
  return node ? wgxpath.Node.getValueAsString(node) : '';
};


/**
 * Returns the number representation of this nodeset.
 *
 * @return {number} The number representation of this nodeset.
 */
wgxpath.NodeSet.prototype.number = function() {
  return +this.string();
};


/**
 * Returns an iterator over this nodeset. Once this iterator is made, DO NOT
 *     add to this nodeset until the iterator is done.
 *
 * @param {boolean=} opt_reverse Whether to iterate right to left or vice versa.
 * @return {!wgxpath.NodeSet.Iterator} An iterator over the nodes.
 */
wgxpath.NodeSet.prototype.iterator = function(opt_reverse) {
  return new wgxpath.NodeSet.Iterator(this, !!opt_reverse);
};



/**
 * An iterator over the nodes of this nodeset.
 *
 * @param {!wgxpath.NodeSet} nodeset The nodeset to be iterated over.
 * @param {boolean} reverse Whether to iterate in ascending or descending
 *     order.
 * @constructor
 */
wgxpath.NodeSet.Iterator = function(nodeset, reverse) {
  // In violation of standard Closure practice, we initialize properties to
  // immutable constants in the constructor instead of on the prototype,
  // because we have empirically measured better performance by doing so.

  /**
   * @type {!wgxpath.NodeSet}
   * @private
   */
  this.nodeset_ = nodeset;

  /**
   * @type {boolean}
   * @private
   */
  this.reverse_ = reverse;

  /**
   * @type {wgxpath.NodeSet.Entry_}
   * @private
   */
  this.current_ = reverse ? nodeset.last_ : nodeset.first_;

  /**
   * @type {wgxpath.NodeSet.Entry_}
   * @private
   */
  this.lastReturned_ = null;
};


/**
 * Returns the next value of the iteration or null if passes the end.
 *
 * @return {?wgxpath.Node} The next node from this iterator.
 */
wgxpath.NodeSet.Iterator.prototype.next = function() {
  var current = this.current_;
  if (current == null) {
    return null;
  } else {
    var lastReturned = this.lastReturned_ = current;
    if (this.reverse_) {
      this.current_ = current.prev;
    } else {
      this.current_ = current.next;
    }
    return lastReturned.node;
  }
};


/**
 * Deletes the last node that was returned from this iterator.
 */
wgxpath.NodeSet.Iterator.prototype.remove = function() {
  /* TODO: to implement delDescendent(node) we shouldn't have to iterate over
   * the entire array, only the things whose index comes after node.
   * If the nodeset is already sorted we know all the descendents lie in a
   * continguous block starting at nodecould start the iterator at node.
   */
  var nodeset = this.nodeset_;
  var entry = this.lastReturned_;
  if (!entry) {
    throw Error('Next must be called at least once before remove.');
  }
  var prev = entry.prev;
  var next = entry.next;

  // Modify the pointers of prev and next
  if (prev) {
    prev.next = next;
  } else {
    // If there was no prev node entry must've been first_, so update first_.
    nodeset.first_ = next;
  }
  if (next) {
    next.prev = prev;
  } else {
    // If there was no prev node entry must've been last_, so update last_.
    nodeset.last_ = prev;
  }
  nodeset.length_--;
  this.lastReturned_ = null;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview An interface for the NodeTest construct.
 */

goog.provide('wgxpath.NodeTest');



/**
 * The NodeTest interface to represent the NodeTest production
 * in the xpath grammar:
 * http://www.w3.org/TR/xpath-30/#prod-xpath30-NodeTest
 *
 * @interface
 */
wgxpath.NodeTest = function() {};


/**
 * Tests if a node matches the stored characteristics.
 *
 * @param {wgxpath.Node} node The node to be tested.
 * @return {boolean} Whether the node passes the test.
 */
wgxpath.NodeTest.prototype.matches = goog.abstractMethod;


/**
 * Returns the name of the test.
 *
 * @return {string} The name, either nodename or type name.
 */
wgxpath.NodeTest.prototype.getName = goog.abstractMethod;


/**
 * Returns the string representation of the NodeTest for debugging.
 *
 * @param {string=} opt_indent Optional indentation.
 * @return {string} The string representation.
 */
wgxpath.NodeTest.prototype.toString = goog.abstractMethod;
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing number literals.
 */

goog.provide('wgxpath.Number');

goog.require('wgxpath.Expr');



/**
 * Constructs a number expression.
 *
 * @param {number} value The number value.
 * @constructor
 * @extends {wgxpath.Expr}
 */
wgxpath.Number = function(value) {
  wgxpath.Expr.call(this, wgxpath.DataType.NUMBER);

  /**
   * @type {number}
   * @private
   */
  this.value_ = value;
};
goog.inherits(wgxpath.Number, wgxpath.Expr);


/**
 * @override
 * @return {number} The number result.
 */
wgxpath.Number.prototype.evaluate = function(ctx) {
  return this.value_;
};


/**
 * @override
 */
wgxpath.Number.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  return indent + 'number: ' + this.value_;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A recursive descent Parser.
 */

goog.provide('wgxpath.Parser');

goog.require('wgxpath.BinaryExpr');
goog.require('wgxpath.FilterExpr');
goog.require('wgxpath.FunctionCall');
goog.require('wgxpath.KindTest');
goog.require('wgxpath.Literal');
goog.require('wgxpath.NameTest');
goog.require('wgxpath.Number');
goog.require('wgxpath.PathExpr');
goog.require('wgxpath.Predicates');
goog.require('wgxpath.Step');
goog.require('wgxpath.UnaryExpr');
goog.require('wgxpath.UnionExpr');



/**
 * The recursive descent parser.
 *
 * @constructor
 * @param {!wgxpath.Lexer} lexer The lexer.
 */
wgxpath.Parser = function(lexer) {

  /**
   * @private
   * @type {!wgxpath.Lexer}
   */
  this.lexer_ = lexer;
};


/**
 * Apply recursive descent parsing on the input to construct an
 * abstract syntax tree.
 *
 * @return {!wgxpath.Expr} The root of the constructed tree.
 */
wgxpath.Parser.prototype.parseExpr = function() {
  var expr, stack = [];
  while (true) {
    this.checkNotEmpty_('Missing right hand side of binary expression.');
    expr = this.parseUnaryExpr_(); // See if it's just a UnaryExpr.
    var opString = this.lexer_.next();
    if (!opString) {
      break; // Done, we have only a UnaryExpr.
    }

    var op = wgxpath.BinaryExpr.getOp(opString);
    var precedence = op && op.getPrecedence();
    if (!precedence) {
      this.lexer_.back();
      break;
    }
    // Precedence climbing
    while (stack.length &&
        precedence <= stack[stack.length - 1].getPrecedence()) {
      expr = new wgxpath.BinaryExpr(stack.pop(), stack.pop(), expr);
    }
    stack.push(expr, op);
  }
  while (stack.length) {
    expr = new wgxpath.BinaryExpr(stack.pop(), stack.pop(),
        /** @type {!wgxpath.Expr} */ (expr));
  }
  return /** @type {!wgxpath.Expr} */ (expr);
};


/**
 * Checks that the lexer is not empty,
 *     displays the given error message if it is.
 *
 * @private
 * @param {string} msg The error message to display.
 */
wgxpath.Parser.prototype.checkNotEmpty_ = function(msg) {
  if (this.lexer_.empty()) {
    throw Error(msg);
  }
};


/**
 * Checks that the next token of the error message is the expected token.
 *
 * @private
 * @param {string} expected The expected token.
 */
wgxpath.Parser.prototype.checkNextEquals_ = function(expected) {
  var got = this.lexer_.next();
  if (got != expected) {
    throw Error('Bad token, expected: ' + expected + ' got: ' + got);
  }
};


/**
 * Checks that the next token of the error message is not the given token.
 *
 * @private
 * @param {string} token The token.
 */
wgxpath.Parser.prototype.checkNextNotEquals_ = function(token) {
  var next = this.lexer_.next();
  if (next != token) {
    throw Error('Bad token: ' + next);
  }
};


/**
 * Attempts to parse the input as a FilterExpr.
 *
 * @private
 * @return {wgxpath.Expr} The root of the constructed tree.
 */
wgxpath.Parser.prototype.parseFilterExpr_ = function() {
  var expr;
  var token = this.lexer_.peek();
  var ch = token.charAt(0);
  switch (ch) {
    case '$':
      throw Error('Variable reference not allowed in HTML XPath');
    case '(':
      this.lexer_.next();
      expr = this.parseExpr();
      this.checkNotEmpty_('unclosed "("');
      this.checkNextEquals_(')');
      break;
    case '"':
    case "'":
      expr = this.parseLiteral_();
      break;
    default:
      if (!isNaN(+token)) {
        expr = this.parseNumber_();
      } else if (wgxpath.KindTest.isValidType(token)) {
        return null;
      } else if (/(?![0-9])[\w]/.test(ch) && this.lexer_.peek(1) == '(') {
        expr = this.parseFunctionCall_();
      } else {
        return null;
      }
  }
  if (this.lexer_.peek() != '[') {
    return expr;
  }
  var predicates = new wgxpath.Predicates(this.parsePredicates_());
  return new wgxpath.FilterExpr(expr, predicates);
};


/**
 * Parses FunctionCall.
 *
 * @private
 * @return {!wgxpath.FunctionCall} The parsed expression.
 */
wgxpath.Parser.prototype.parseFunctionCall_ = function() {
  var funcName = this.lexer_.next();
  var func = wgxpath.FunctionCall.getFunc(funcName);
  this.lexer_.next();

  var args = [];
  while (this.lexer_.peek() != ')') {
    this.checkNotEmpty_('Missing function argument list.');
    args.push(this.parseExpr());
    if (this.lexer_.peek() != ',') {
      break;
    }
    this.lexer_.next();
  }
  this.checkNotEmpty_('Unclosed function argument list.');
  this.checkNextNotEquals_(')');

  return new wgxpath.FunctionCall(func, args);
};


/**
 * Parses the input to construct a KindTest.
 *
 * @private
 * @return {!wgxpath.KindTest} The KindTest constructed.
 */
wgxpath.Parser.prototype.parseKindTest_ = function() {
  var typeName = this.lexer_.next();
  if (!wgxpath.KindTest.isValidType(typeName)) {
    throw Error('Invalid type name: ' + typeName);
  }
  this.checkNextEquals_('(');
  this.checkNotEmpty_('Bad nodetype');
  var ch = this.lexer_.peek().charAt(0);

  var literal = null;
  if (ch == '"' || ch == "'") {
    literal = this.parseLiteral_();
  }
  this.checkNotEmpty_('Bad nodetype');
  this.checkNextNotEquals_(')');
  return new wgxpath.KindTest(typeName, literal);
};


/**
 * Parses the input to construct a Literal.
 *
 * @private
 * @return {!wgxpath.Literal} The Literal constructed.
 */
wgxpath.Parser.prototype.parseLiteral_ = function() {
  var token = this.lexer_.next();
  if (token.length < 2) {
    throw Error('Unclosed literal string');
  }
  return new wgxpath.Literal(token);
};


/**
 * Parses the input to construct a NameTest.
 *
 * @private
 * @return {!wgxpath.NameTest} The NameTest constructed.
 */
wgxpath.Parser.prototype.parseNameTest_ = function() {
  // Has namespace
  if (this.lexer_.peek() != '*' && this.lexer_.peek(1) == ':' &&
      this.lexer_.peek(2) == '*') {
    return new wgxpath.NameTest(this.lexer_.next() + this.lexer_.next() +
        this.lexer_.next());
  }
  return new wgxpath.NameTest(this.lexer_.next());
};


/**
 * Parses the input to construct a Number.
 *
 * @private
 * @return {!wgxpath.Number} The Number constructed.
 */
wgxpath.Parser.prototype.parseNumber_ = function() {
  return new wgxpath.Number(+this.lexer_.next());
};


/**
 * Attempts to parse the input as a PathExpr.
 *
 * @private
 * @return {!wgxpath.Expr} The root of the constructed tree.
 */
wgxpath.Parser.prototype.parsePathExpr_ = function() {
  var op, expr;
  var steps = [];
  var filterExpr;
  if (wgxpath.PathExpr.isValidOp(this.lexer_.peek())) {
    op = this.lexer_.next();
    var token = this.lexer_.peek();
    if (op == '/' && (this.lexer_.empty() ||
        (token != '.' && token != '..' && token != '@' && token != '*' &&
        !/(?![0-9])[\w]/.test(token)))) {
      return new wgxpath.PathExpr.RootHelperExpr();
    }
    filterExpr = new wgxpath.PathExpr.RootHelperExpr();

    this.checkNotEmpty_('Missing next location step.');
    expr = this.parseStep_(op);
    steps.push(expr);
  } else {
    expr = this.parseFilterExpr_();
    if (!expr) {
      expr = this.parseStep_('/');
      filterExpr = new wgxpath.PathExpr.ContextHelperExpr();
      steps.push(expr);
    } else if (!wgxpath.PathExpr.isValidOp(this.lexer_.peek())) {
      return expr; // Done.
    } else {
      filterExpr = expr;
    }
  }
  while (true) {
    if (!wgxpath.PathExpr.isValidOp(this.lexer_.peek())) {
      break;
    }
    op = this.lexer_.next();
    this.checkNotEmpty_('Missing next location step.');
    expr = this.parseStep_(op);
    steps.push(expr);
  }
  return new wgxpath.PathExpr(filterExpr, steps);
};


/**
 * Parses Step.
 *
 * @private
 * @param {string} op The op for this step.
 * @return {!wgxpath.Step} The parsed expression.
 */
wgxpath.Parser.prototype.parseStep_ = function(op) {
  // TODO (evanrthomas) : Let parseStep see op instead of passing it
  //     as a parameter
  var test, step, token, predicates;
  if (op != '/' && op != '//') {
    throw Error('Step op should be "/" or "//"');
  }
  if (this.lexer_.peek() == '.') {
    step = new wgxpath.Step(wgxpath.Step.Axis.SELF,
        new wgxpath.KindTest('node'));
    this.lexer_.next();
    return step;
  }
  else if (this.lexer_.peek() == '..') {
    step = new wgxpath.Step(wgxpath.Step.Axis.PARENT,
        new wgxpath.KindTest('node'));
    this.lexer_.next();
    return step;
  } else {
    // Grab the axis.
    var axis;
    if (this.lexer_.peek() == '@') {
      axis = wgxpath.Step.Axis.ATTRIBUTE;
      this.lexer_.next();
      this.checkNotEmpty_('Missing attribute name');
    } else {
      if (this.lexer_.peek(1) == '::') {
        if (!/(?![0-9])[\w]/.test(this.lexer_.peek().charAt(0))) {
          throw Error('Bad token: ' + this.lexer_.next());
        }
        var axisName = this.lexer_.next();
        axis = wgxpath.Step.getAxis(axisName);
        if (!axis) {
          throw Error('No axis with name: ' + axisName);
        }
        this.lexer_.next();
        this.checkNotEmpty_('Missing node name');
      } else {
        axis = wgxpath.Step.Axis.CHILD;
      }
    }

    // Grab the test.
    token = this.lexer_.peek();
    if (!/(?![0-9])[\w]/.test(token.charAt(0))) {
      if (token == '*') {
        test = this.parseNameTest_();
      } else {
        throw Error('Bad token: ' + this.lexer_.next());
      }
    } else {
      if (this.lexer_.peek(1) == '(') {
        if (!wgxpath.KindTest.isValidType(token)) {
          throw Error('Invalid node type: ' + token);
        }
        test = this.parseKindTest_();
      } else {
        test = this.parseNameTest_();
      }
    }
    predicates = new wgxpath.Predicates(this.parsePredicates_(),
        axis.isReverse());
    return step || new wgxpath.Step(axis, test, predicates, op == '//');
  }
};


/**
 * Parses and returns the predicates from the this.lexer_.
 *
 * @private
 * @return {!Array.<!wgxpath.Expr>} An array of the predicates.
 */
wgxpath.Parser.prototype.parsePredicates_ = function() {
  var predicates = [];
  while (this.lexer_.peek() == '[') {
    this.lexer_.next();
    this.checkNotEmpty_('Missing predicate expression.');
    var predicate = this.parseExpr();
    predicates.push(predicate);
    this.checkNotEmpty_('Unclosed predicate expression.');
    this.checkNextEquals_(']');
  }
  return predicates;
};


/**
 * Attempts to parse the input as a unary expression with
 * recursive descent parsing.
 *
 * @private
 * @return {!wgxpath.Expr} The root of the constructed tree.
 */
wgxpath.Parser.prototype.parseUnaryExpr_ = function() {
  if (this.lexer_.peek() == '-') {
    this.lexer_.next();
    return new wgxpath.UnaryExpr(this.parseUnaryExpr_());
  } else {
    return this.parseUnionExpr_();
  }
};


/**
 * Attempts to parse the input as a union expression with
 * recursive descent parsing.
 *
 * @private
 * @return {!wgxpath.Expr} The root of the constructed tree.
 */
wgxpath.Parser.prototype.parseUnionExpr_ = function() {
  var expr = this.parsePathExpr_();
  if (!(this.lexer_.peek() == '|')) {
    return expr;  // Not a UnionExpr, returning as is.
  }
  var paths = [expr];
  while (this.lexer_.next() == '|') {
    this.checkNotEmpty_('Missing next union location path.');
    paths.push(this.parsePathExpr_());
  }
  this.lexer_.back();
  return new wgxpath.UnionExpr(paths);
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview An class representing operations on path expressions.
 */

goog.provide('wgxpath.PathExpr');


goog.require('goog.array');
goog.require('goog.dom.NodeType');
goog.require('wgxpath.DataType');
goog.require('wgxpath.Expr');
goog.require('wgxpath.NodeSet');



/**
 * Constructor for PathExpr.
 *
 * @param {!wgxpath.Expr} filter A filter expression.
 * @param {!Array.<!wgxpath.Step>} steps The steps in the location path.
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.PathExpr = function(filter, steps) {
  wgxpath.Expr.call(this, filter.getDataType());

  /**
   * @type {!wgxpath.Expr}
   * @private
   */
  this.filter_ = filter;

  /**
   * @type {!Array.<!wgxpath.Step>}
   * @private
   */
  this.steps_ = steps;

  this.setNeedContextPosition(filter.doesNeedContextPosition());
  this.setNeedContextNode(filter.doesNeedContextNode());
  if (this.steps_.length == 1) {
    var firstStep = this.steps_[0];
    if (!firstStep.doesIncludeDescendants() &&
        firstStep.getAxis() == wgxpath.Step.Axis.ATTRIBUTE) {
      var test = firstStep.getTest();
      if (test.getName() != '*') {
        this.setQuickAttr({
          name: test.getName(),
          valueExpr: null
        });
      }
    }
  }
};
goog.inherits(wgxpath.PathExpr, wgxpath.Expr);



/**
 * Constructor for RootHelperExpr.
 *
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.PathExpr.RootHelperExpr = function() {
  wgxpath.Expr.call(this, wgxpath.DataType.NODESET);
};
goog.inherits(wgxpath.PathExpr.RootHelperExpr, wgxpath.Expr);


/**
 * Evaluates the root-node helper expression.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {!wgxpath.NodeSet} The evaluation result.
 */
wgxpath.PathExpr.RootHelperExpr.prototype.evaluate = function(ctx) {
  var nodeset = new wgxpath.NodeSet();
  var node = ctx.getNode();
  if (node.nodeType == goog.dom.NodeType.DOCUMENT) {
    nodeset.add(node);
  } else {
    nodeset.add(/** @type {!Node} */ (node.ownerDocument));
  }
  return nodeset;
};


/**
 * Returns the string representation of the RootHelperExpr for debugging.
 *
 * @param {string=} opt_indent An optional indentation.
 * @return {string} The string representation.
 */
wgxpath.PathExpr.RootHelperExpr.prototype.toString = function(opt_indent) {
  return opt_indent + 'RootHelperExpr';
};



/**
 * Constructor for ContextHelperExpr.
 *
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.PathExpr.ContextHelperExpr = function() {
  wgxpath.Expr.call(this, wgxpath.DataType.NODESET);
};
goog.inherits(wgxpath.PathExpr.ContextHelperExpr, wgxpath.Expr);


/**
 * Evaluates the context-node helper expression.
 *
 * @param {!wgxpath.Context} ctx The context to evaluate the expression in.
 * @return {!wgxpath.NodeSet} The evaluation result.
 */
wgxpath.PathExpr.ContextHelperExpr.prototype.evaluate = function(ctx) {
  var nodeset = new wgxpath.NodeSet();
  nodeset.add(ctx.getNode());
  return nodeset;
};


/**
 * Returns the string representation of the ContextHelperExpr for debugging.
 *
 * @param {string=} opt_indent An optional indentation.
 * @return {string} The string representation.
 */
wgxpath.PathExpr.ContextHelperExpr.prototype.toString = function(opt_indent) {
  return opt_indent + 'ContextHelperExpr';
};


/**
 * Returns whether the token is a valid PathExpr operator.
 *
 * @param {string} token The token to be checked.
 * @return {boolean} Whether the token is a valid operator.
 */
wgxpath.PathExpr.isValidOp = function(token) {
  return token == '/' || token == '//';
};


/**
 * @override
 * @return {!wgxpath.NodeSet} The nodeset result.
 */
wgxpath.PathExpr.prototype.evaluate = function(ctx) {
  var nodeset = this.filter_.evaluate(ctx);
  if (!(nodeset instanceof wgxpath.NodeSet)) {
    throw Error('FilterExpr must evaluate to nodeset.');
  }
  var steps = this.steps_;
  for (var i = 0, l0 = steps.length; i < l0 && nodeset.getLength(); i++) {
    var step = steps[i];
    var reverse = step.getAxis().isReverse();
    var iter = nodeset.iterator(reverse);
    nodeset = null;
    var node, next;
    if (!step.doesNeedContextPosition() &&
        step.getAxis() == wgxpath.Step.Axis.FOLLOWING) {
      for (node = iter.next(); next = iter.next(); node = next) {
        if (node.contains && !node.contains(next)) {
          break;
        } else {
          if (!(next.compareDocumentPosition(/** @type {!Node} */ (node)) &
              8)) {
            break;
          }
        }
      }
      nodeset = step.evaluate(new
          wgxpath.Context(/** @type {wgxpath.Node} */ (node)));
    } else if (!step.doesNeedContextPosition() &&
        step.getAxis() == wgxpath.Step.Axis.PRECEDING) {
      node = iter.next();
      nodeset = step.evaluate(new
          wgxpath.Context(/** @type {wgxpath.Node} */ (node)));
    } else {
      node = iter.next();
      nodeset = step.evaluate(new
          wgxpath.Context(/** @type {wgxpath.Node} */ (node)));
      while ((node = iter.next()) != null) {
        var result = step.evaluate(new
            wgxpath.Context(/** @type {wgxpath.Node} */ (node)));
        nodeset = wgxpath.NodeSet.merge(nodeset, result);
      }
    }
  }
  return /** @type {!wgxpath.NodeSet} */ (nodeset);
};


/**
 * @override
 */
wgxpath.PathExpr.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'PathExpr:' + '\n';
  indent += wgxpath.Expr.INDENT;
  text += this.filter_.toString(indent);
  if (this.steps_.length) {
    text += indent + 'Steps:' + '\n';
    indent += wgxpath.Expr.INDENT;
    goog.array.forEach(this.steps_, function(step) {
      text += step.toString(indent);
    });
  }
  return text;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview An abstract class representing expressions with predicates.
 *     baseExprWithPredictes are immutable objects that evaluate their
 *     predicates against nodesets and return the modified nodesets.
 *
 */


goog.provide('wgxpath.Predicates');

goog.require('goog.array');
goog.require('wgxpath.Context');
goog.require('wgxpath.Expr');



/**
 * An abstract class for expressions with predicates.
 *
 * @constructor
 * @param {!Array.<!wgxpath.Expr>} predicates The array of predicates.
 * @param {boolean=} opt_reverse Whether to iterate over the nodeset in reverse.
 */
wgxpath.Predicates = function(predicates, opt_reverse) {

  /**
   * List of predicates
   *
   * @private
   * @type {!Array.<!wgxpath.Expr>}
   */
  this.predicates_ = predicates;


  /**
   * Which direction to iterate over the predicates
   *
   * @private
   * @type {boolean}
   */
  this.reverse_ = !!opt_reverse;
};


/**
 * Evaluates the predicates against the given nodeset.
 *
 * @param {!wgxpath.NodeSet} nodeset The nodes against which to evaluate
 *     the predicates.
 * @param {number=} opt_start The index of the first predicate to evaluate,
 *     defaults to 0.
 * @return {!wgxpath.NodeSet} nodeset The filtered nodeset.
 */
wgxpath.Predicates.prototype.evaluatePredicates =
    function(nodeset, opt_start) {
  for (var i = opt_start || 0; i < this.predicates_.length; i++) {
    var predicate = this.predicates_[i];
    var iter = nodeset.iterator();
    var l = nodeset.getLength();
    var node;
    for (var j = 0; node = iter.next(); j++) {
      var position = this.reverse_ ? (l - j) : (j + 1);
      var exrs = predicate.evaluate(new
          wgxpath.Context(/** @type {wgxpath.Node} */ (node), position, l));
      var keep;
      if (typeof exrs == 'number') {
        keep = (position == exrs);
      } else if (typeof exrs == 'string' || typeof exrs == 'boolean') {
        keep = !!exrs;
      } else if (exrs instanceof wgxpath.NodeSet) {
        keep = (exrs.getLength() > 0);
      } else {
        throw Error('Predicate.evaluate returned an unexpected type.');
      }
      if (!keep) {
        iter.remove();
      }
    }
  }
  return nodeset;
};


/**
 * Returns the quickAttr info.
 *
 * @return {?{name: string, valueExpr: wgxpath.Expr}}
 */
wgxpath.Predicates.prototype.getQuickAttr = function() {
  return this.predicates_.length > 0 ?
      this.predicates_[0].getQuickAttr() : null;
};


/**
 * Returns whether this set of predicates needs context position.
 *
 * @return {boolean} Whether something needs context position.
 */
wgxpath.Predicates.prototype.doesNeedContextPosition = function() {
  for (var i = 0; i < this.predicates_.length; i++) {
    var predicate = this.predicates_[i];
    if (predicate.doesNeedContextPosition() ||
        predicate.getDataType() == wgxpath.DataType.NUMBER ||
        predicate.getDataType() == wgxpath.DataType.VOID) {
      return true;
    }
  }
  return false;
};


/**
 * Returns the length of this set of predicates.
 *
 * @return {number} The number of expressions.
 */
wgxpath.Predicates.prototype.getLength = function() {
  return this.predicates_.length;
};


/**
 * Returns a string represendation of this set of predicates for debugging.
 *
 * @param {string=} opt_indent An optional indentation.
 * @return {string} The string representation.
 */
wgxpath.Predicates.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var header = indent + 'Predicates:';
  indent += wgxpath.Expr.INDENT;
  return goog.array.reduce(this.predicates_, function(prev, curr) {
    return prev + '\n' + indent + curr.toString(indent);
  }, header);
};
goog.provide('wgxpath.Step');

goog.require('goog.dom.NodeType');
goog.require('wgxpath.DataType');
goog.require('wgxpath.Expr');
goog.require('wgxpath.KindTest');
goog.require('wgxpath.Node');
goog.require('wgxpath.Predicates');
goog.require('wgxpath.userAgent');



/**
 * Class for a step in a path expression
 * http://www.w3.org/TR/xpath20/#id-steps.
 *
 * @extends {wgxpath.Expr}
 * @constructor
 * @param {!wgxpath.Step.Axis} axis The axis for this Step.
 * @param {!wgxpath.NodeTest} test The test for this Step.
 * @param {!wgxpath.Predicates=} opt_predicates The predicates for this
 *     Step.
 * @param {boolean=} opt_descendants Whether descendants are to be included in
 *     this step ('//' vs '/').
 */
wgxpath.Step = function(axis, test, opt_predicates, opt_descendants) {
  var axisCast = /** @type {!wgxpath.Step.Axis_} */ (axis);
  wgxpath.Expr.call(this, wgxpath.DataType.NODESET);

  /**
   * @type {!wgxpath.Step.Axis_}
   * @private
   */
  this.axis_ = axisCast;


  /**
   * @type {!wgxpath.NodeTest}
   * @private
   */
  this.test_ = test;

  /**
   * @type {!wgxpath.Predicates}
   * @private
   */
  this.predicates_ = opt_predicates || new wgxpath.Predicates([]);


  /**
   * Whether decendants are included in this step
   *
   * @private
   * @type {boolean}
   */
  this.descendants_ = !!opt_descendants;

  var quickAttrInfo = this.predicates_.getQuickAttr();
  if (axis.supportsQuickAttr_ && quickAttrInfo) {
    var attrName = quickAttrInfo.name;
    attrName = wgxpath.userAgent.IE_DOC_PRE_9 ?
        attrName.toLowerCase() : attrName;
    var attrValueExpr = quickAttrInfo.valueExpr;
    this.setQuickAttr({
      name: attrName,
      valueExpr: attrValueExpr
    });
  }
  this.setNeedContextPosition(this.predicates_.doesNeedContextPosition());
};
goog.inherits(wgxpath.Step, wgxpath.Expr);


/**
 * @override
 * @return {!wgxpath.NodeSet} The nodeset result.
 */
wgxpath.Step.prototype.evaluate = function(ctx) {
  var node = ctx.getNode();
  var nodeset = null;
  var quickAttr = this.getQuickAttr();
  var attrName = null;
  var attrValue = null;
  var pstart = 0;
  if (quickAttr) {
    attrName = quickAttr.name;
    attrValue = quickAttr.valueExpr ?
        quickAttr.valueExpr.asString(ctx) : null;
    pstart = 1;
  }
  if (this.descendants_) {
    if (!this.doesNeedContextPosition() &&
        this.axis_ == wgxpath.Step.Axis.CHILD) {
      nodeset = wgxpath.Node.getDescendantNodes(this.test_, node,
          attrName, attrValue);
      nodeset = this.predicates_.evaluatePredicates(nodeset, pstart);
    } else {
      var step = new wgxpath.Step(wgxpath.Step.Axis.DESCENDANT_OR_SELF,
          new wgxpath.KindTest('node'));
      var iter = step.evaluate(ctx).iterator();
      var n = iter.next();
      if (!n) {
        nodeset = new wgxpath.NodeSet();
      } else {
        nodeset = this.evaluate_(/** @type {!wgxpath.Node} */ (n),
            attrName, attrValue, pstart);
        while ((n = iter.next()) != null) {
          nodeset = wgxpath.NodeSet.merge(nodeset,
              this.evaluate_(/** @type {!wgxpath.Node} */ (n), attrName,
              attrValue, pstart));
        }
      }
    }
  } else {
    nodeset = this.evaluate_(ctx.getNode(), attrName, attrValue, pstart);
  }
  return nodeset;
};


/**
 * Evaluates this step on the given context to a nodeset.
 *     (assumes this.descendants_ = false)
 *
 * @private
 * @param {!wgxpath.Node} node The context node.
 * @param {?string} attrName The name of the attribute.
 * @param {?string} attrValue The value of the attribute.
 * @param {number} pstart The first predicate to evaluate.
 * @return {!wgxpath.NodeSet} The nodeset from evaluating this Step.
 */
wgxpath.Step.prototype.evaluate_ = function(
    node, attrName, attrValue, pstart) {
  var nodeset = this.axis_.func_(this.test_, node, attrName, attrValue);
  nodeset = this.predicates_.evaluatePredicates(nodeset, pstart);
  return nodeset;
};


/**
 * Returns whether the step evaluation should include descendants.
 *
 * @return {boolean} Whether descendants are included.
 */
wgxpath.Step.prototype.doesIncludeDescendants = function() {
  return this.descendants_;
};


/**
 * Returns the step's axis.
 *
 * @return {!wgxpath.Step.Axis} The axis.
 */
wgxpath.Step.prototype.getAxis = function() {
  return (/** @type {!wgxpath.Step.Axis} */ this.axis_);
};


/**
 * Returns the test for this step.
 *
 * @return {!wgxpath.NodeTest} The test for this step.
 */
wgxpath.Step.prototype.getTest = function() {
  return this.test_;
};


/**
 * @override
 */
wgxpath.Step.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'Step: ' + '\n';
  indent += wgxpath.Expr.INDENT;
  text += indent + 'Operator: ' + (this.descendants_ ? '//' : '/') + '\n';
  if (this.axis_.name_) {
    text += indent + 'Axis: ' + this.axis_ + '\n';
  }
  text += this.test_.toString(indent);
  if (this.predicates_.length) {
    text += indent + 'Predicates: ' + '\n';
    for (var i = 0; i < this.predicates_.length; i++) {
      var tail = i < this.predicates_.length - 1 ? ', ' : '';
      text += this.predicates_[i].toString(indent) + tail;
    }
  }
  return text;
};



/**
 * A step axis.
 *
 * @constructor
 * @param {string} name The axis name.
 * @param {function(!wgxpath.NodeTest, wgxpath.Node, ?string, ?string):
 *     !wgxpath.NodeSet} func The function for this axis.
 * @param {boolean} reverse Whether to iterate over the nodeset in reverse.
 * @param {boolean} supportsQuickAttr Whether quickAttr should be enabled for
 *     this axis.
 * @private
 */
wgxpath.Step.Axis_ = function(name, func, reverse, supportsQuickAttr) {

  /**
   * @private
   * @type {string}
   */
  this.name_ = name;

  /**
   * @private
   * @type {function(!wgxpath.NodeTest, wgxpath.Node, ?string, ?string):
   *     !wgxpath.NodeSet}
   */
  this.func_ = func;

  /**
   * @private
   * @type {boolean}
   */
  this.reverse_ = reverse;

  /**
   * @private
   * @type {boolean}
   */
  this.supportsQuickAttr_ = supportsQuickAttr;
};


/**
 * Returns whether the nodes in the step should be iterated over in reverse.
 *
 * @return {boolean} Whether the nodes should be iterated over in reverse.
 */
wgxpath.Step.Axis_.prototype.isReverse = function() {
  return this.reverse_;
};


/**
 * @override
 */
wgxpath.Step.Axis_.prototype.toString = function() {
  return this.name_;
};


/**
 * A map from axis name to Axis.
 *
 * @type {!Object.<string, !wgxpath.Step.Axis>}
 * @private
 */
wgxpath.Step.nameToAxisMap_ = {};


/**
 * Creates an axis and maps the axis's name to that axis.
 *
 * @param {string} name The axis name.
 * @param {function(!wgxpath.NodeTest, wgxpath.Node, ?string, ?string):
 *     !wgxpath.NodeSet} func The function for this axis.
 * @param {boolean} reverse Whether to iterate over nodesets in reverse.
 * @param {boolean=} opt_supportsQuickAttr Whether quickAttr can be enabled
 *     for this axis.
 * @return {!wgxpath.Step.Axis} The axis.
 * @private
 */
wgxpath.Step.createAxis_ =
    function(name, func, reverse, opt_supportsQuickAttr) {
  if (name in wgxpath.Step.nameToAxisMap_) {
    throw Error('Axis already created: ' + name);
  }
  // The upcast and then downcast for the JSCompiler.
  var axis = (/** @type {!Object} */ new wgxpath.Step.Axis_(
      name, func, reverse, !!opt_supportsQuickAttr));
  axis = (/** @type {!wgxpath.Step.Axis} */ axis);
  wgxpath.Step.nameToAxisMap_[name] = axis;
  return axis;
};


/**
 * Returns the axis for this axisname or null if none.
 *
 * @param {string} name The axis name.
 * @return {wgxpath.Step.Axis} The axis.
 */
wgxpath.Step.getAxis = function(name) {
  return wgxpath.Step.nameToAxisMap_[name] || null;
};


/**
 * Axis enumeration.
 *
 * @enum {{isReverse: function(): boolean}}
 */
wgxpath.Step.Axis = {
  ANCESTOR: wgxpath.Step.createAxis_('ancestor',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        var parent = node;
        while (parent = parent.parentNode) {
          if (test.matches(parent)) {
            nodeset.unshift(parent);
          }
        }
        return nodeset;
      }, true),
  ANCESTOR_OR_SELF: wgxpath.Step.createAxis_('ancestor-or-self',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        var toMatch = node;
        do {
          if (test.matches(toMatch)) {
            nodeset.unshift(toMatch);
          }
        } while (toMatch = toMatch.parentNode);
        return nodeset;
      }, true),
  ATTRIBUTE: wgxpath.Step.createAxis_('attribute',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        var testName = test.getName();
        // IE8 doesn't allow access to the style attribute using getNamedItem.
        // It returns an object with nodeValue = null.
        if (testName == 'style' && node.style &&
            wgxpath.userAgent.IE_DOC_PRE_9) {
          nodeset.add(wgxpath.IEAttrWrapper.forStyleOf(
              /** @type {!Node} */ (node), node.sourceIndex));
          return nodeset;
        }
        var attrs = node.attributes;
        if (attrs) {
          if ((test instanceof wgxpath.KindTest &&
              goog.isNull(test.getType())) || testName == '*') {
            var sourceIndex = node.sourceIndex;
            for (var i = 0, attr; attr = attrs[i]; i++) {
              if (wgxpath.userAgent.IE_DOC_PRE_9) {
                if (attr.nodeValue) {
                  nodeset.add(wgxpath.IEAttrWrapper.forAttrOf(
                      /** @type {!Node} */ (node), attr, sourceIndex));
                }
              } else {
                nodeset.add(attr);
              }
            }
          } else {
            var attr = attrs.getNamedItem(testName);
            if (attr) {
              if (wgxpath.userAgent.IE_DOC_PRE_9) {
                if (attr.nodeValue) {
                  nodeset.add(wgxpath.IEAttrWrapper.forAttrOf(
                      /** @type {!Node} */ (node), attr, node.sourceIndex));
                }
              } else {
                nodeset.add(attr);
              }
            }
          }
        }
        return nodeset;
      }, false),
  CHILD: wgxpath.Step.createAxis_('child',
      wgxpath.Node.getChildNodes, false, true),
  DESCENDANT: wgxpath.Step.createAxis_('descendant',
      wgxpath.Node.getDescendantNodes, false, true),
  DESCENDANT_OR_SELF: wgxpath.Step.createAxis_('descendant-or-self',
      function(test, node, attrName, attrValue) {
        var nodeset = new wgxpath.NodeSet();
        if (wgxpath.Node.attrMatches(node, attrName, attrValue)) {
          if (test.matches(node)) {
            nodeset.add(node);
          }
        }
        return wgxpath.Node.getDescendantNodes(test, node,
            attrName, attrValue, nodeset);
      }, false, true),
  FOLLOWING: wgxpath.Step.createAxis_('following',
      function(test, node, attrName, attrValue) {
        var nodeset = new wgxpath.NodeSet();
        var parent = node;
        do {
          var child = parent;
          while (child = child.nextSibling) {
            if (wgxpath.Node.attrMatches(child, attrName, attrValue)) {
              if (test.matches(child)) {
                nodeset.add(child);
              }
            }
            nodeset = wgxpath.Node.getDescendantNodes(test, child,
                attrName, attrValue, nodeset);
          }
        } while (parent = parent.parentNode);
        return nodeset;
      }, false, true),
  FOLLOWING_SIBLING: wgxpath.Step.createAxis_('following-sibling',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        var toMatch = node;
        while (toMatch = toMatch.nextSibling) {
          if (test.matches(toMatch)) {
            nodeset.add(toMatch);
          }
        }
        return nodeset;
      }, false),
  NAMESPACE: wgxpath.Step.createAxis_('namespace',
      function(test, node) {
        // not implemented
        return new wgxpath.NodeSet();
      }, false),
  PARENT: wgxpath.Step.createAxis_('parent',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        if (node.nodeType == goog.dom.NodeType.DOCUMENT) {
          return nodeset;
        } else if (node.nodeType == goog.dom.NodeType.ATTRIBUTE) {
          nodeset.add(node.ownerElement);
          return nodeset;
        }
        var parent = /** @type {!Node} */ (node.parentNode);
        if (test.matches(parent)) {
          nodeset.add(parent);
        }
        return nodeset;
      }, false),
  PRECEDING: wgxpath.Step.createAxis_('preceding',
      function(test, node, attrName, attrValue) {
        var nodeset = new wgxpath.NodeSet();
        var parents = [];
        var parent = node;
        do {
          parents.unshift(parent);
        } while (parent = parent.parentNode);
        for (var i = 1, l0 = parents.length; i < l0; i++) {
          var siblings = [];
          node = parents[i];
          while (node = node.previousSibling) {
            siblings.unshift(node);
          }
          for (var j = 0, l1 = siblings.length; j < l1; j++) {
            node = siblings[j];
            if (wgxpath.Node.attrMatches(node, attrName, attrValue)) {
              if (test.matches(node)) nodeset.add(node);
            }
            nodeset = wgxpath.Node.getDescendantNodes(test, node,
                attrName, attrValue, nodeset);
          }
        }
        return nodeset;
      }, true, true),
  PRECEDING_SIBLING: wgxpath.Step.createAxis_('preceding-sibling',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        var toMatch = node;
        while (toMatch = toMatch.previousSibling) {
          if (test.matches(toMatch)) {
            nodeset.unshift(toMatch);
          }
        }
        return nodeset;
      }, true),
  SELF: wgxpath.Step.createAxis_('self',
      function(test, node) {
        var nodeset = new wgxpath.NodeSet();
        if (test.matches(node)) {
          nodeset.add(node);
        }
        return nodeset;
      }, false)
};
// This file was autogenerated by calcdeps.py
goog.addDependency("../../../wicked-good-xpath/test_js_deps.js", [], []);
goog.addDependency("../../../wicked-good-xpath/functionCall.js", ['wgxpath.FunctionCall'], ['goog.array', 'goog.dom', 'goog.dom.NodeType', 'goog.string', 'wgxpath.Expr', 'wgxpath.Node', 'wgxpath.NodeSet', 'wgxpath.userAgent']);
goog.addDependency("../../../wicked-good-xpath/unaryExpr.js", ['wgxpath.UnaryExpr'], ['wgxpath.DataType', 'wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/nodeset.js", ['wgxpath.NodeSet'], ['goog.dom', 'wgxpath.Node']);
goog.addDependency("../../../wicked-good-xpath/pathExpr.js", ['wgxpath.PathExpr'], ['goog.array', 'goog.dom.NodeType', 'wgxpath.DataType', 'wgxpath.Expr', 'wgxpath.NodeSet']);
goog.addDependency("../../../wicked-good-xpath/literal.js", ['wgxpath.Literal'], ['wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/node.js", ['wgxpath.Node'], ['goog.array', 'goog.dom.NodeType', 'goog.userAgent', 'wgxpath.IEAttrWrapper', 'wgxpath.userAgent']);
goog.addDependency("../../../wicked-good-xpath/step.js", ['wgxpath.Step'], ['goog.dom.NodeType', 'wgxpath.DataType', 'wgxpath.Expr', 'wgxpath.KindTest', 'wgxpath.Node', 'wgxpath.Predicates', 'wgxpath.userAgent']);
goog.addDependency("../../../wicked-good-xpath/parser.js", ['wgxpath.Parser'], ['wgxpath.BinaryExpr', 'wgxpath.FilterExpr', 'wgxpath.FunctionCall', 'wgxpath.KindTest', 'wgxpath.Literal', 'wgxpath.NameTest', 'wgxpath.Number', 'wgxpath.PathExpr', 'wgxpath.Predicates', 'wgxpath.Step', 'wgxpath.UnaryExpr', 'wgxpath.UnionExpr']);
goog.addDependency("../../../wicked-good-xpath/ieAttrWrapper.js", ['wgxpath.IEAttrWrapper'], ['goog.dom.NodeType', 'wgxpath.userAgent']);
goog.addDependency("../../../wicked-good-xpath/expr.js", ['wgxpath.Expr'], ['wgxpath.NodeSet']);
goog.addDependency("../../../wicked-good-xpath/number.js", ['wgxpath.Number'], ['wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/kindTest.js", ['wgxpath.KindTest'], ['goog.dom.NodeType', 'wgxpath.NodeTest']);
goog.addDependency("../../../wicked-good-xpath/lexer.js", ['wgxpath.Lexer'], []);
goog.addDependency("../../../wicked-good-xpath/wgxpath.js", ['wgxpath'], ['wgxpath.Context', 'wgxpath.IEAttrWrapper', 'wgxpath.Lexer', 'wgxpath.NodeSet', 'wgxpath.Parser']);
goog.addDependency("../../../wicked-good-xpath/nameTest.js", ['wgxpath.NameTest'], ['goog.dom.NodeType']);
goog.addDependency("../../../wicked-good-xpath/unionExpr.js", ['wgxpath.UnionExpr'], ['goog.array', 'wgxpath.DataType', 'wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/userAgent.js", ['wgxpath.userAgent'], ['goog.userAgent']);
goog.addDependency("../../../wicked-good-xpath/binaryExpr.js", ['wgxpath.BinaryExpr'], ['wgxpath.DataType', 'wgxpath.Expr', 'wgxpath.Node']);
goog.addDependency("../../../wicked-good-xpath/export.js", [], ['wgxpath']);
goog.addDependency("../../../wicked-good-xpath/dataType.js", ['wgxpath.DataType'], []);
goog.addDependency("../../../wicked-good-xpath/filterExpr.js", ['wgxpath.FilterExpr'], ['wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/nodeTest.js", ['wgxpath.NodeTest'], []);
goog.addDependency("../../../wicked-good-xpath/context.js", ['wgxpath.Context'], []);
goog.addDependency("../../../wicked-good-xpath/predicates.js", ['wgxpath.Predicates'], ['goog.array', 'wgxpath.Context', 'wgxpath.Expr']);
goog.addDependency("../../../wicked-good-xpath/test/test_js_deps.js", [], []);
goog.addDependency("../../../wicked-good-xpath/test/test.js", ['wgxpath.test'], ['goog.dom', 'goog.dom.NodeType', 'goog.userAgent']);
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing operations on unary expressions.
 */

goog.provide('wgxpath.UnaryExpr');

goog.require('wgxpath.DataType');
goog.require('wgxpath.Expr');



/**
 * Constructor for UnaryExpr.
 *
 * @param {!wgxpath.Expr} expr The unary expression.
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.UnaryExpr = function(expr) {
  wgxpath.Expr.call(this, wgxpath.DataType.NUMBER);

  /**
   * @private
   * @type {!wgxpath.Expr}
   */
  this.expr_ = expr;

  this.setNeedContextPosition(expr.doesNeedContextPosition());
  this.setNeedContextNode(expr.doesNeedContextNode());
};
goog.inherits(wgxpath.UnaryExpr, wgxpath.Expr);


/**
 * @override
 * @return {number} The number result.
 */
wgxpath.UnaryExpr.prototype.evaluate = function(ctx) {
  return -this.expr_.asNumber(ctx);
};


/**
 * @override
 */
wgxpath.UnaryExpr.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'UnaryExpr: -' + '\n';
  indent += wgxpath.Expr.INDENT;
  text += this.expr_.toString(indent);
  return text;
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview A class representing operations on union expressions.
 */

goog.provide('wgxpath.UnionExpr');

goog.require('goog.array');
goog.require('wgxpath.DataType');
goog.require('wgxpath.Expr');



/**
 * Constructor for UnionExpr.
 *
 * @param {!Array.<!wgxpath.Expr>} paths The paths in the union.
 * @extends {wgxpath.Expr}
 * @constructor
 */
wgxpath.UnionExpr = function(paths) {
  wgxpath.Expr.call(this, wgxpath.DataType.NODESET);

  /**
   * @type {!Array.<!wgxpath.Expr>}
   * @private
   */
  this.paths_ = paths;
  this.setNeedContextPosition(goog.array.some(this.paths_, function(p) {
    return p.doesNeedContextPosition();
  }));
  this.setNeedContextNode(goog.array.some(this.paths_, function(p) {
    return p.doesNeedContextNode();
  }));
};
goog.inherits(wgxpath.UnionExpr, wgxpath.Expr);


/**
 * @override
 * @return {!wgxpath.NodeSet} The nodeset result.
 */
wgxpath.UnionExpr.prototype.evaluate = function(ctx) {
  var nodeset = new wgxpath.NodeSet();
  goog.array.forEach(this.paths_, function(p) {
    var result = p.evaluate(ctx);
    if (!(result instanceof wgxpath.NodeSet)) {
      throw Error('PathExpr must evaluate to NodeSet.');
    }
    nodeset = wgxpath.NodeSet.merge(nodeset, result);
  });
  return nodeset;
};


/**
 * @override
 */
wgxpath.UnionExpr.prototype.toString = function(opt_indent) {
  var indent = opt_indent || '';
  var text = indent + 'UnionExpr:' + '\n';
  indent += wgxpath.Expr.INDENT;
  goog.array.forEach(this.paths_, function(p) {
    text += p.toString(indent) + '\n';
  });
  return text.substring(0, text.length); // Remove trailing newline.
};
// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * @fileoverview Constants for user agent detection.
 */

goog.provide('wgxpath.userAgent');

goog.require('goog.userAgent');


/**
 * @type {boolean}
 * @const
 */
wgxpath.userAgent.IE_DOC_PRE_9 = goog.userAgent.IE &&
    !goog.userAgent.isDocumentMode(9);


/**
 * @type {boolean}
 * @const
 */
wgxpath.userAgent.IE_DOC_PRE_8 = goog.userAgent.IE &&
    !goog.userAgent.isDocumentMode(8);
/*  JavaScript-XPath 0.1.11
 *  (c) 2007 Cybozu Labs, Inc.
 *
 *  JavaScript-XPath is freely distributable under the terms of an MIT-style
 *  license. For details, see the JavaScript-XPath web site:
 *  http://coderepos.org/share/wiki/JavaScript-XPath
 *
/*--------------------------------------------------------------------------*/

// Copyright 2012 Google Inc. All Rights Reserved.

/**
 * Wicked Good XPath
 *
 * @fileoverview A cross-browser XPath library forked from the
 * JavaScript-XPath project by Cybozu Labs.
 *
 */

goog.provide('wgxpath');

goog.require('wgxpath.Context');
goog.require('wgxpath.IEAttrWrapper');
goog.require('wgxpath.Lexer');
goog.require('wgxpath.NodeSet');
goog.require('wgxpath.Parser');


/**
 * Enum for XPathResult types.
 *
 * @private
 * @enum {number}
 */
wgxpath.XPathResultType_ = {
  ANY_TYPE: 0,
  NUMBER_TYPE: 1,
  STRING_TYPE: 2,
  BOOLEAN_TYPE: 3,
  UNORDERED_NODE_ITERATOR_TYPE: 4,
  ORDERED_NODE_ITERATOR_TYPE: 5,
  UNORDERED_NODE_SNAPSHOT_TYPE: 6,
  ORDERED_NODE_SNAPSHOT_TYPE: 7,
  ANY_UNORDERED_NODE_TYPE: 8,
  FIRST_ORDERED_NODE_TYPE: 9
};



/**
 * The exported XPathExpression type.
 *
 * @constructor
 * @extends {XPathExpression}
 * @param {string} expr The expression string.
 * @private
 */
wgxpath.XPathExpression_ = function(expr) {
  if (!expr.length) {
    throw Error('Empty XPath expression.');
  }

  var lexer = wgxpath.Lexer.tokenize(expr);

  if (lexer.empty()) {
    throw Error('Invalid XPath expression.');
  }
  var gexpr = new wgxpath.Parser(lexer).parseExpr();
  if (!lexer.empty()) {
    throw Error('Bad token: ' + lexer.next());
  }
  this['evaluate'] = function(node, type) {
    var value = gexpr.evaluate(new wgxpath.Context(node));
    return new wgxpath.XPathResult_(value, type);
  };
};



/**
 * The exported XPathResult type.
 *
 * @constructor
 * @extends {XPathResult}
 * @param {(!wgxpath.NodeSet|number|string|boolean)} value The result value.
 * @param {number} type The result type.
 * @private
 */
wgxpath.XPathResult_ = function(value, type) {
  if (type == wgxpath.XPathResultType_.ANY_TYPE) {
    if (value instanceof wgxpath.NodeSet) {
      type = wgxpath.XPathResultType_.UNORDERED_NODE_ITERATOR_TYPE;
    } else if (typeof value == 'string') {
      type = wgxpath.XPathResultType_.STRING_TYPE;
    } else if (typeof value == 'number') {
      type = wgxpath.XPathResultType_.NUMBER_TYPE;
    } else if (typeof value == 'boolean') {
      type = wgxpath.XPathResultType_.BOOLEAN_TYPE;
    } else {
      throw Error('Unexpected evaluation result.');
    }
  }
  if (type != wgxpath.XPathResultType_.STRING_TYPE &&
      type != wgxpath.XPathResultType_.NUMBER_TYPE &&
      type != wgxpath.XPathResultType_.BOOLEAN_TYPE &&
      !(value instanceof wgxpath.NodeSet)) {
    throw Error('document.evaluate called with wrong result type.');
  }
  this['resultType'] = type;
  var nodes;
  switch (type) {
    case wgxpath.XPathResultType_.STRING_TYPE:
      this['stringValue'] = (value instanceof wgxpath.NodeSet) ?
          value.string() : '' + value;
      break;
    case wgxpath.XPathResultType_.NUMBER_TYPE:
      this['numberValue'] = (value instanceof wgxpath.NodeSet) ?
          value.number() : +value;
      break;
    case wgxpath.XPathResultType_.BOOLEAN_TYPE:
      this['booleanValue'] = (value instanceof wgxpath.NodeSet) ?
          value.getLength() > 0 : !!value;
      break;
    case wgxpath.XPathResultType_.UNORDERED_NODE_ITERATOR_TYPE:
    case wgxpath.XPathResultType_.ORDERED_NODE_ITERATOR_TYPE:
    case wgxpath.XPathResultType_.UNORDERED_NODE_SNAPSHOT_TYPE:
    case wgxpath.XPathResultType_.ORDERED_NODE_SNAPSHOT_TYPE:
      var iter = value.iterator();
      nodes = [];
      for (var node = iter.next(); node; node = iter.next()) {
        nodes.push(node instanceof wgxpath.IEAttrWrapper ?
            node.getNode() : node);
      }
      this['snapshotLength'] = value.getLength();
      this['invalidIteratorState'] = false;
      break;
    case wgxpath.XPathResultType_.ANY_UNORDERED_NODE_TYPE:
    case wgxpath.XPathResultType_.FIRST_ORDERED_NODE_TYPE:
      var firstNode = value.getFirst();
      this['singleNodeValue'] =
          firstNode instanceof wgxpath.IEAttrWrapper ?
          firstNode.getNode() : firstNode;
      break;
    default:
      throw Error('Unknown XPathResult type.');
  }
  var index = 0;
  this['iterateNext'] = function() {
    if (type != wgxpath.XPathResultType_.UNORDERED_NODE_ITERATOR_TYPE &&
        type != wgxpath.XPathResultType_.ORDERED_NODE_ITERATOR_TYPE) {
      throw Error('iterateNext called with wrong result type.');
    }
    return (index >= nodes.length) ? null : nodes[index++];
  };
  this['snapshotItem'] = function(i) {
    if (type != wgxpath.XPathResultType_.UNORDERED_NODE_SNAPSHOT_TYPE &&
        type != wgxpath.XPathResultType_.ORDERED_NODE_SNAPSHOT_TYPE) {
      throw Error('snapshotItem called with wrong result type.');
    }
    return (i >= nodes.length || i < 0) ? null : nodes[i];
  };
};
wgxpath.XPathResult_['ANY_TYPE'] = wgxpath.XPathResultType_.ANY_TYPE;
wgxpath.XPathResult_['NUMBER_TYPE'] = wgxpath.XPathResultType_.NUMBER_TYPE;
wgxpath.XPathResult_['STRING_TYPE'] = wgxpath.XPathResultType_.STRING_TYPE;
wgxpath.XPathResult_['BOOLEAN_TYPE'] = wgxpath.XPathResultType_.BOOLEAN_TYPE;
wgxpath.XPathResult_['UNORDERED_NODE_ITERATOR_TYPE'] =
    wgxpath.XPathResultType_.UNORDERED_NODE_ITERATOR_TYPE;
wgxpath.XPathResult_['ORDERED_NODE_ITERATOR_TYPE'] =
    wgxpath.XPathResultType_.ORDERED_NODE_ITERATOR_TYPE;
wgxpath.XPathResult_['UNORDERED_NODE_SNAPSHOT_TYPE'] =
    wgxpath.XPathResultType_.UNORDERED_NODE_SNAPSHOT_TYPE;
wgxpath.XPathResult_['ORDERED_NODE_SNAPSHOT_TYPE'] =
    wgxpath.XPathResultType_.ORDERED_NODE_SNAPSHOT_TYPE;
wgxpath.XPathResult_['ANY_UNORDERED_NODE_TYPE'] =
    wgxpath.XPathResultType_.ANY_UNORDERED_NODE_TYPE;
wgxpath.XPathResult_['FIRST_ORDERED_NODE_TYPE'] =
    wgxpath.XPathResultType_.FIRST_ORDERED_NODE_TYPE;


/**
 * Installs the library. This is a noop if native XPath is available.
 *
 * @param {Window=} opt_win The window to install the library on.
 */
wgxpath.install = function(opt_win) {
  var win = opt_win || goog.global;
  var doc = win.document;

  // Installation is a noop if native XPath is available.
  if (!doc['evaluate']) {
    win['XPathResult'] = wgxpath.XPathResult_;
    doc['evaluate'] = function(expr, context, nsresolver, type, result) {
      return new wgxpath.XPathExpression_(expr).evaluate(context, type);
    };
    doc['createExpression'] = function(expr) {
      return new wgxpath.XPathExpression_(expr);
    };
  }
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms for simulating user actions against the DOM.
 * The bot.action namespace is required since these atoms would otherwise form a
 * circular dependency between bot.dom and bot.events.
 *
 */

goog.provide('bot.action');

goog.require('bot');
goog.require('bot.Device');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.Keyboard');
goog.require('bot.Mouse');
goog.require('bot.Touchscreen');
goog.require('bot.dom');
goog.require('bot.events');
goog.require('bot.events.EventType');
goog.require('bot.userAgent');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.math.Coordinate');
goog.require('goog.math.Rect');
goog.require('goog.math.Vec2');
goog.require('goog.style');
goog.require('goog.userAgent');


/**
 * Throws an exception if an element is not shown to the user, ignoring its
 * opacity.

 *
 * @param {!Element} element The element to check.
 * @see bot.dom.isShown.
 * @private
 */
bot.action.checkShown_ = function(element) {
  if (!bot.dom.isShown(element, /*ignoreOpacity=*/true)) {
    throw new bot.Error(bot.ErrorCode.ELEMENT_NOT_VISIBLE,
        'Element is not currently visible and may not be manipulated');
  }
};


/**
 * Throws an exception if the given element cannot be interacted with.
 *
 * @param {!Element} element The element to check.
 * @throws {bot.Error} If the element cannot be interacted with.
 * @see bot.dom.isInteractable.
 * @private
 */
bot.action.checkInteractable_ = function(element) {
  if (!bot.dom.isInteractable(element)) {
    throw new bot.Error(bot.ErrorCode.INVALID_ELEMENT_STATE,
        'Element is not currently interactable and may not be manipulated');

  }
};


/**
 * Clears the given {@code element} if it is a editable text field.
 *
 * @param {!Element} element The element to clear.
 * @throws {bot.Error} If the element is not an editable text field.
 */
bot.action.clear = function(element) {
  bot.action.checkInteractable_(element);
  if (!bot.dom.isEditable(element)) {
    throw new bot.Error(bot.ErrorCode.INVALID_ELEMENT_STATE,
        'Element must be user-editable in order to clear it.');
  }

  bot.action.LegacyDevice_.focusOnElement(element);
  if (element.value) {
    element.value = '';
    bot.events.fire(element, bot.events.EventType.CHANGE);
  }

  if (bot.dom.isContentEditable(element)) {
    // A single space is required, if you put empty string here you'll not be
    // able to interact with this element anymore in Firefox.
    element.innerHTML = ' ';
    // contentEditable does not generate onchange event.
  }
};


/**
 * Focuses on the given element if it is not already the active element.
 *
 * @param {!Element} element The element to focus on.
 */
bot.action.focusOnElement = function(element) {
  bot.action.checkInteractable_(element);
  bot.action.LegacyDevice_.focusOnElement(element);
};


/**
 * Types keys on the given {@code element} with a virtual keyboard.
 *
 * <p>Callers can pass in a string, a key in bot.Keyboard.Key, or an array
 * of strings or keys. If a modifier key is provided, it is pressed but not
 * released, until it is either is listed again or the function ends.
 *
 * <p>Example:
 *   bot.keys.type(element, ['ab', bot.Keyboard.Key.LEFT,
 *                           bot.Keyboard.Key.SHIFT, 'cd']);
 *
 * @param {!Element} element The element receiving the event.
 * @param {(string|!bot.Keyboard.Key|
 *          !Array.<(string|!bot.Keyboard.Key)>)} values Value or values to
 *     type on the element.
 * @param {bot.Keyboard=} opt_keyboard Keyboard to use; if not provided,
 *     constructs one.
 * @param {boolean=} opt_persistModifiers Whether modifier keys should remain
 *     pressed when this function ends.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.type = function(
    element, values, opt_keyboard, opt_persistModifiers) {
  bot.action.checkShown_(element);
  bot.action.checkInteractable_(element);
  var keyboard = opt_keyboard || new bot.Keyboard();
  keyboard.moveCursor(element);

  function typeValue(value) {
    if (goog.isString(value)) {
      goog.array.forEach(value.split(''), function(ch) {
        var keyShiftPair = bot.Keyboard.Key.fromChar(ch);
        var shiftIsPressed = keyboard.isPressed(bot.Keyboard.Keys.SHIFT);
        if (keyShiftPair.shift && !shiftIsPressed) {
          keyboard.pressKey(bot.Keyboard.Keys.SHIFT);
        }
        keyboard.pressKey(keyShiftPair.key);
        keyboard.releaseKey(keyShiftPair.key);
        if (keyShiftPair.shift && !shiftIsPressed) {
          keyboard.releaseKey(bot.Keyboard.Keys.SHIFT);
        }
      });
    } else if (goog.array.contains(bot.Keyboard.MODIFIERS, value)) {
      if (keyboard.isPressed(value)) {
        keyboard.releaseKey(value);
      } else {
        keyboard.pressKey(value);
      }
    } else {
      keyboard.pressKey(value);
      keyboard.releaseKey(value);
    }
  }

  // mobile safari (iPhone / iPad). one cannot 'type' in a date field
  // chrome implements this, but desktop Safari doesn't, what's webkit again?
  if ((!(goog.userAgent.product.SAFARI && !goog.userAgent.MOBILE)) &&
      goog.userAgent.WEBKIT && element.type == 'date') {
    var val = goog.isArray(values)? values = values.join("") : values;
    var datePattern = /\d{4}-\d{2}-\d{2}/;
    if (val.match(datePattern)) {
      // The following events get fired on iOS first
      if (goog.userAgent.MOBILE && goog.userAgent.product.SAFARI) {
        bot.events.fire(element, bot.events.EventType.TOUCHSTART);
        bot.events.fire(element, bot.events.EventType.TOUCHEND);
      }
      bot.events.fire(element, bot.events.EventType.FOCUS);
      element.value = val.match(datePattern)[0];
      bot.events.fire(element, bot.events.EventType.CHANGE);
      bot.events.fire(element, bot.events.EventType.BLUR);
      return;
    }
  }

  if (goog.isArray(values)) {
    goog.array.forEach(values, typeValue);
  } else {
    typeValue(values);
  }

  if (!opt_persistModifiers) {
    // Release all the modifier keys.
    goog.array.forEach(bot.Keyboard.MODIFIERS, function(key) {
      if (keyboard.isPressed(key)) {
        keyboard.releaseKey(key);
      }
    });
  }
};


/**
 * Submits the form containing the given {@code element}.
 *
 * <p>Note this function submits the form, but does not simulate user input
 * (a click or key press).
 *
 * @param {!Element} element The element to submit.
 * @deprecated Click on a submit button or type ENTER in a text box instead.
 */
bot.action.submit = function(element) {
  var form = bot.action.LegacyDevice_.findAncestorForm(element);
  if (!form) {
    throw new bot.Error(bot.ErrorCode.NO_SUCH_ELEMENT,
                        'Element was not in a form, so could not submit.');
  }
  bot.action.LegacyDevice_.submitForm(element, form);
};


/**
 * Moves the mouse over the given {@code element} with a virtual mouse.
 *
 * @param {!Element} element The element to click.
 * @param {goog.math.Coordinate=} opt_coords Mouse position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.moveMouse = function(element, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
};


/**
 * Clicks on the given {@code element} with a virtual mouse.
 *
 * @param {!Element} element The element to click.
 * @param {goog.math.Coordinate=} opt_coords Mouse position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.click = function(element, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
  mouse.pressButton(bot.Mouse.Button.LEFT);
  mouse.releaseButton();
};


/**
 * Right-clicks on the given {@code element} with a virtual mouse.
 *
 * @param {!Element} element The element to click.
 * @param {goog.math.Coordinate=} opt_coords Mouse position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.rightClick = function(element, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
  mouse.pressButton(bot.Mouse.Button.RIGHT);
  mouse.releaseButton();
};


/**
 * Double-clicks on the given {@code element} with a virtual mouse.
 *
 * @param {!Element} element The element to click.
 * @param {goog.math.Coordinate=} opt_coords Mouse position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.doubleClick = function(element, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
  mouse.pressButton(bot.Mouse.Button.LEFT);
  mouse.releaseButton();
  mouse.pressButton(bot.Mouse.Button.LEFT);
  mouse.releaseButton();
};


/**
 * Scrolls the mouse wheel on the given {@code element} with a virtual mouse.
 *
 * @param {!Element} element The element to scroll the mouse wheel on.
 * @param {number} ticks Number of ticks to scroll the mouse wheel; a positive
 *   number scrolls down and a negative scrolls up.
 * @param {goog.math.Coordinate=} opt_coords Mouse position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.scrollMouse = function(element, ticks, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
  mouse.scroll(ticks);
};


/**
 * Drags the given {@code element} by (dx, dy) with a virtual mouse.
 *
 * @param {!Element} element The element to drag.
 * @param {number} dx Increment in x coordinate.
 * @param {number} dy Increment in y coordinate.
 * @param {goog.math.Coordinate=} opt_coords Drag start position relative to the
 *   element.
 * @param {bot.Mouse=} opt_mouse Mouse to use; if not provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.drag = function(element, dx, dy, opt_coords, opt_mouse) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var mouse = opt_mouse || new bot.Mouse();
  mouse.move(element, coords);
  mouse.pressButton(bot.Mouse.Button.LEFT);

  // Fire two mousemoves (middle and destination) to trigger a drag action.
  var initPos = goog.style.getClientPosition(element);
  var midXY = new goog.math.Coordinate(coords.x + Math.floor(dx / 2),
                                       coords.y + Math.floor(dy / 2));
  mouse.move(element, midXY);

  var midPos = goog.style.getClientPosition(element);
  var finalXY = new goog.math.Coordinate(initPos.x + coords.x + dx - midPos.x,
                                         initPos.y + coords.y + dy - midPos.y);
  mouse.move(element, finalXY);

  mouse.releaseButton();
};


/**
 * Taps on the given {@code element} with a virtual touch screen.
 *
 * @param {!Element} element The element to tap.
 * @param {goog.math.Coordinate=} opt_coords Finger position relative to the
 *   target.
 * @param {bot.Touchscreen=} opt_touchscreen Touchscreen to use; if not
 *    provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.tap = function(element, opt_coords, opt_touchscreen) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var touchscreen = opt_touchscreen || new bot.Touchscreen();
  touchscreen.move(element, coords);
  touchscreen.press();
  touchscreen.release();
};


/**
 * Swipes the given {@code element} by (dx, dy) with a virtual touch screen.
 *
 * @param {!Element} element The element to swipe.
 * @param {number} dx Increment in x coordinate.
 * @param {number} dy Increment in y coordinate.
 * @param {goog.math.Coordinate=} opt_coords Swipe start position relative to
 *   the element.
 * @param {bot.Touchscreen=} opt_touchscreen Touchscreen to use; if not
 *    provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.swipe = function(element, dx, dy, opt_coords, opt_touchscreen) {
  var coords = bot.action.prepareToInteractWith_(element, opt_coords);
  var touchscreen = opt_touchscreen || new bot.Touchscreen();
  touchscreen.move(element, coords);
  touchscreen.press();

  // Fire two touchmoves (middle and destination) to trigger a drag action.
  var initPos = goog.style.getClientPosition(element);
  var midXY = new goog.math.Coordinate(coords.x + Math.floor(dx / 2),
                                       coords.y + Math.floor(dy / 2));
  touchscreen.move(element, midXY);

  var midPos = goog.style.getClientPosition(element);
  var finalXY = new goog.math.Coordinate(initPos.x + coords.x + dx - midPos.x,
                                         initPos.y + coords.y + dy - midPos.y);
  touchscreen.move(element, finalXY);

  touchscreen.release();
};


/**
 * Pinches the given {@code element} by the given distance with a virtual touch
 * screen. A positive distance moves two fingers inward toward each and a
 * negative distances spreds them outward. The optional coordinate is the point
 * the fingers move towards (for positive distances) or away from (for negative
 * distances); and if not provided, defaults to the center of the element.
 *
 * @param {!Element} element The element to pinch.
 * @param {number} distance The distance by which to pinch the element.
 * @param {goog.math.Coordinate=} opt_coords Position relative to the element
 *   at the center of the pinch.
 * @param {bot.Touchscreen=} opt_touchscreen Touchscreen to use; if not
 *    provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.pinch = function(element, distance, opt_coords, opt_touchscreen) {
  if (distance == 0) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
                        'Cannot pinch by a distance of zero.');
  }
  function startSoThatEndsAtMax(offsetVec) {
    if (distance < 0) {
      var magnitude = offsetVec.magnitude();
      offsetVec.scale(magnitude ? (magnitude + distance) / magnitude : 0);
    }
  }
  var halfDistance = distance / 2;
  function scaleByHalfDistance(offsetVec) {
    var magnitude = offsetVec.magnitude();
    offsetVec.scale(magnitude ? (magnitude - halfDistance) / magnitude : 0);
  }
  bot.action.multiTouchAction_(element,
                               startSoThatEndsAtMax,
                               scaleByHalfDistance,
                               opt_coords,
                               opt_touchscreen);
};


/**
 * Rotates the given {@code element} by the given angle with a virtual touch
 * screen. A positive angle moves two fingers clockwise and a negative angle
 * moves them counter-clockwise. The optional coordinate is the point to
 * rotate around; and if not provided, defaults to the center of the element.
 *
 * @param {!Element} element The element to rotate.
 * @param {number} angle The angle by which to rotate the element.
 * @param {goog.math.Coordinate=} opt_coords Position relative to the element
 *   at the center of the rotation.
 * @param {bot.Touchscreen=} opt_touchscreen Touchscreen to use; if not
 *    provided, constructs one.
 * @throws {bot.Error} If the element cannot be interacted with.
 */
bot.action.rotate = function(element, angle, opt_coords, opt_touchscreen) {
  if (angle == 0) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
                        'Cannot rotate by an angle of zero.');
  }
  function startHalfwayToMax(offsetVec) {
    offsetVec.scale(0.5);
  }
  var halfRadians = Math.PI * (angle / 180) / 2;
  function rotateByHalfAngle(offsetVec) {
    offsetVec.rotate(halfRadians);
  }
  bot.action.multiTouchAction_(element,
                               startHalfwayToMax,
                               rotateByHalfAngle,
                               opt_coords,
                               opt_touchscreen);
};


/**
 * Performs a multi-touch action with two fingers on the given element. This
 * helper function works by manipulating an "offsetVector", which is the vector
 * away from the center of the interaction at which the fingers are positioned.
 * It computes the maximum offset vector and passes it to transformStart to
 * find the starting position of the fingers; it then passes it to transformHalf
 * twice to find the midpoint and final position of the fingers.
 *
 * @param {!Element} element Element to interact with.
 * @param {function(goog.math.Vec2)} transformStart Function to transform the
 *   maximum offset vector to the starting offset vector.
 * @param {function(goog.math.Vec2)} transformHalf Function to transform the
 *   offset vector halfway to its destination.
 * @param {goog.math.Coordinate=} opt_coords Position relative to the element
 *   at the center of the pinch.
 * @param {bot.Touchscreen=} opt_touchscreen Touchscreen to use; if not
 *    provided, constructs one.
 * @private
 */
bot.action.multiTouchAction_ = function(element, transformStart, transformHalf,
                                        opt_coords, opt_touchscreen) {
  var center = bot.action.prepareToInteractWith_(element, opt_coords);
  var size = bot.action.getInteractableSize(element);
  var offsetVec = new goog.math.Vec2(
      Math.min(center.x, size.width - center.x),
      Math.min(center.y, size.height - center.y));

  var touchScreen = opt_touchscreen || new bot.Touchscreen();
  transformStart(offsetVec);
  var start1 = goog.math.Vec2.sum(center, offsetVec);
  var start2 = goog.math.Vec2.difference(center, offsetVec);
  touchScreen.move(element, start1, start2);
  touchScreen.press(/*Two Finger Press*/ true);

  var initPos = goog.style.getClientPosition(element);
  transformHalf(offsetVec);
  var mid1 = goog.math.Vec2.sum(center, offsetVec);
  var mid2 = goog.math.Vec2.difference(center, offsetVec);
  touchScreen.move(element, mid1, mid2);

  var movedVec = goog.math.Vec2.difference(
      goog.style.getClientPosition(element), initPos);
  transformHalf(offsetVec);
  var end1 = goog.math.Vec2.sum(center, offsetVec).subtract(movedVec);
  var end2 = goog.math.Vec2.difference(center, offsetVec).subtract(movedVec);
  touchScreen.move(element, end1, end2);
  touchScreen.release();
};


/**
 * Prepares to interact with the given {@code element}. It checks if the the
 * element is shown, scrolls the element into view, and returns the coordinates
 * of the interaction, which if not provided, is the center of the element.
 *
 * @param {!Element} element The element to be interacted with.
 * @param {goog.math.Coordinate=} opt_coords Position relative to the target.
 * @return {!goog.math.Vec2} Coordinates at the center of the interaction.
 * @throws {bot.Error} If the element cannot be interacted with.
 * @private
 */
bot.action.prepareToInteractWith_ = function(element, opt_coords) {
  bot.action.checkShown_(element);

  // Unlike element.scrollIntoView(), this scrolls the minimal amount
  // necessary, not scrolling at all if the element is already in view.
  var doc = goog.dom.getOwnerDocument(element);
  goog.style.scrollIntoContainerView(element,
      goog.userAgent.WEBKIT ? doc.body : doc.documentElement);

  // NOTE(user): Ideally, we would check that any provided coordinates fall
  // within the bounds of the element, but this has proven difficult, because:
  // (1) Browsers sometimes lie about the true size of elements, e.g. when text
  // overflows the bounding box of an element, browsers report the size of the
  // box even though the true area that can be interacted with is larger; and
  // (2) Elements with children styled as position:absolute will often not have
  // a bounding box that surrounds all of their children, but it is useful for
  // the user to be able to interact with this parent element as if it does.
  if (opt_coords) {
    return goog.math.Vec2.fromCoordinate(opt_coords);
  } else {
    var size = bot.action.getInteractableSize(element);
    return new goog.math.Vec2(size.width / 2, size.height / 2);
  }
};


/**
 * Returns the interactable size of an element.
 *
 * @param {!Element} elem Element.
 * @return {!goog.math.Size} size Size of the element.
 */
bot.action.getInteractableSize = function(elem) {
  var size = goog.style.getSize(elem);
  return ((size.width > 0 && size.height > 0) || !elem.offsetParent) ? size :
      bot.action.getInteractableSize(elem.offsetParent);
};



/**
 * A Device that is intended to allows access to protected members of the
 * Device superclass. A singleton.
 *
 * @constructor
 * @extends {bot.Device}
 * @private
 */
bot.action.LegacyDevice_ = function() {
  goog.base(this);
};
goog.inherits(bot.action.LegacyDevice_, bot.Device);
goog.addSingletonGetter(bot.action.LegacyDevice_);


/**
 * Focuses on the given element.  See {@link bot.device.focusOnElement}.
 * @param {!Element} element The element to focus on.
 * @return {boolean} True if element.focus() was called on the element.
 */
bot.action.LegacyDevice_.focusOnElement = function(element) {
  var instance = bot.action.LegacyDevice_.getInstance();
  instance.setElement(element);
  return instance.focusOnElement();
};


/**
 * Submit the form for the element.  See {@link bot.device.submit}.
 * @param {!Element} element The element to submit a form on.
 * @param {!Element} form The form to submit.
 */
bot.action.LegacyDevice_.submitForm = function(element, form) {
  var instance = bot.action.LegacyDevice_.getInstance();
  instance.setElement(element);
  instance.submitForm(form);
};


/**
 * Find FORM element that is an ancestor of the passed in element.  See
 * {@link bot.device.findAncestorForm}.
 * @param {!Element} element The element to find an ancestor form.
 * @return {Element} form The ancestor form, or null if none.
 */
bot.action.LegacyDevice_.findAncestorForm = function(element) {
  return bot.Device.findAncestorForm(element);
};


/**
 * Scrolls the given {@code element} in to the current viewport. Aims to do the
 * minimum scrolling necessary, but prefers too much scrolling to too little.
 *
 * @param {!Element} element The element to scroll in to view.
 * @param {!goog.math.Coordinate=} opt_coords Offset relative to the top-left
 *     corner of the element, to ensure is scrolled in to view.
 * @return {boolean} Whether the element is in view after scrolling.
 */
bot.action.scrollIntoView = function(element, opt_coords) {
  if (!bot.dom.isScrolledIntoView(element, opt_coords) && !bot.dom.isInParentOverflow(element, opt_coords)) {
    // Some elements may not have a scrollIntoView function - for example,
    // elements under an SVG element. Call those only if they exist.
    if (typeof element.scrollIntoView == 'function') {
      element.scrollIntoView();
    }
    // In Opera 10, scrollIntoView only scrolls the element into the viewport of
    // its immediate parent window, so we explicitly scroll the ancestor frames
    // into view of their respective windows. Note that scrolling the top frame
    // first --- and so on down to the element itself --- does not work, because
    // Opera 10 apparently treats element.scrollIntoView() as a noop when it
    // immediately follows a scrollIntoView() call on its parent frame.
    if (goog.userAgent.OPERA && !bot.userAgent.isEngineVersion(11)) {
      var win = goog.dom.getWindow(goog.dom.getOwnerDocument(element));
      for (var frame = win.frameElement; frame; frame = win.frameElement) {
        frame.scrollIntoView();
        win = goog.dom.getWindow(goog.dom.getOwnerDocument(frame));
      }
    }
  }
  if (opt_coords) {
    var rect = new goog.math.Rect(opt_coords.x, opt_coords.y, 1, 1);
    bot.dom.scrollElementRegionIntoClientView(element, rect);
  }
  var isInView = bot.dom.isScrolledIntoView(element, opt_coords);
  if (!isInView && opt_coords) {
    // It's possible that the element has been scrolled in to view, but the
    // coords passed aren't in view; if this is the case, scroll those
    // coordinates into view.
    var elementCoordsInViewport = goog.style.getClientPosition(element);
    var desiredPointInViewport =
        goog.math.Coordinate.sum(elementCoordsInViewport, opt_coords);
    try {
      bot.dom.getInViewLocation(
          desiredPointInViewport,
          goog.dom.getWindow(goog.dom.getOwnerDocument(element)));
      isInView = true;
    } catch (ex) {
      // Point couldn't be scrolled into view.
      isInView = false;
    }
  }

  return isInView;
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Overall configuration of the browser automation atoms.
 */


goog.provide('bot');


/**
 * Frameworks using the atoms keep track of which window or frame is currently
 * being used for command execution. Note that "window" may not always be
 * defined (for example in firefox extensions)
 *
 * @private {!Window}
 */
bot.window_;

try {
  bot.window_ = window;
} catch (ignored) {
  // We only reach this place in a firefox extension.
  bot.window_ = goog.global;
}


/**
 * Returns the window currently being used for command execution.
 *
 * @return {!Window} The window for command execution.
 */
bot.getWindow = function() {
  return bot.window_;
};


/**
 * Sets the window to be used for command execution.
 *
 * @param {!Window} win The window for command execution.
 */
bot.setWindow = function(win) {
  bot.window_ = win;
};


/**
 * Returns the document of the window currently being used for
 * command execution.
 *
 * @return {!Document} The current window's document.
 */
bot.getDocument = function() {
  return bot.window_.document;
};
// Copyright 2012 Software Freedom Conservancy
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utilities related to color and color conversion.
 * Some of this code is borrowed and modified from goog.color and
 * goog.color.alpha.
 */

goog.provide('bot.color');

goog.require('goog.array');
goog.require('goog.color.names');


/**
 * Returns a property, with a standardized color if it contains a
 * convertible color.
 * @param {string} propertyName Name of the CSS property in selector-case.
 * @param {string} propertyValue The value of the CSS property.
 * @return {string} The value, in a standardized format
 *    if it is a color property.
 */
bot.color.standardizeColor = function(propertyName, propertyValue) {
  if (bot.color.isColorProperty(propertyName) &&
      bot.color.isConvertibleColor(propertyValue)) {
    return bot.color.standardizeToRgba_(propertyValue);
  }
  return propertyValue;
};


/**
 * Returns a color in RGBA format - rgba(r, g, b, a).
 * @param {string} propertyValue The value of the CSS property.
 * @return {string} The value, in RGBA format.
 * @private
 */
bot.color.standardizeToRgba_ = function(propertyValue) {
  var rgba = bot.color.parseRgbaColor(propertyValue);
  if (!rgba.length) {
    rgba = bot.color.convertToRgba_(propertyValue);
    bot.color.addAlphaIfNecessary_(rgba);
  }
  if (rgba.length != 4) {
    return propertyValue;
  }
  return bot.color.toRgbaStyle_(rgba);
};


/**
 * Coverts a color to RGBA.
 * @param {string} propertyValue The value of the CSS property.
 * @return {!Array.<number>} array containing [r, g, b, a]
 *  with r, g, b as ints in [0, 255] and a as a float in [0, 1].
 * @private
 */
bot.color.convertToRgba_ = function(propertyValue) {
  var rgba = bot.color.parseRgbColor_(propertyValue);
  if (rgba.length) {
    return rgba;
  }
  var hex = goog.color.names[propertyValue.toLowerCase()];
  hex = (!hex) ? bot.color.prependHashIfNecessary_(propertyValue) : hex;
  if (bot.color.isValidHexColor_(hex)) {
    rgba = bot.color.hexToRgb(bot.color.normalizeHex(hex));
    if (rgba.length) {
      return rgba;
    }
  }
  return [];
};


/**
 * Determines if the given string is a color that can be converted to RGBA.
 * Browsers can return colors in the following formats:
 * RGB, RGBA, Hex, NamedColor
 * So only those are supported by this module and therefore considered
 * convertible.
 *
 * @param {string} str Potential color string.
 * @return {boolean} True if str is in a format that can be converted to RGBA.
 */
bot.color.isConvertibleColor = function(str) {
  return !!(bot.color.isValidHexColor_(
      bot.color.prependHashIfNecessary_(str)) ||
      bot.color.parseRgbColor_(str).length ||
      goog.color.names && goog.color.names[str.toLowerCase()] ||
      bot.color.parseRgbaColor(str).length
  );
};


/**
 * Used to determine whether a css property contains a color and
 * should therefore be standardized to rgba.
 * These are  extracted from the W3C CSS spec:
 *
 * http://www.w3.org/TR/CSS/#properties
 *
 * Used by bot.color.isColorProperty()
 * @const
 * @private {!Array.<string>}
 */
bot.color.COLOR_PROPERTIES_ = [
  'background-color',
  'border-top-color',
  'border-right-color',
  'border-bottom-color',
  'border-left-color',
  'color',
  'outline-color'
];


/**
 * Determines if the given property can contain a color.
 * @param {string} str CSS property name.
 * @return {boolean} True if str is a property that can contain a color.
 */
bot.color.isColorProperty = function(str) {
  return goog.array.contains(bot.color.COLOR_PROPERTIES_, str);
};


/**
 * Regular expression for extracting the digits in a hex color triplet.
 * @private {!RegExp}
 * @const
 */
bot.color.HEX_TRIPLET_RE_ = /#([0-9a-fA-F])([0-9a-fA-F])([0-9a-fA-F])/;


/**
 * Normalize an hex representation of a color
 * @param {string} hexColor an hex color string.
 * @return {string} hex color in the format '#rrggbb' with all lowercase
 *     literals.
 */
bot.color.normalizeHex = function(hexColor) {
  if (!bot.color.isValidHexColor_(hexColor)) {
    throw Error("'" + hexColor + "' is not a valid hex color");
  }
  if (hexColor.length == 4) { // of the form #RGB
    hexColor = hexColor.replace(bot.color.HEX_TRIPLET_RE_, '#$1$1$2$2$3$3');
  }
  return hexColor.toLowerCase();
};


/**
 * Converts a hex representation of a color to RGB.
 * @param {string} hexColor Color to convert.
 * @return {!Array} array containing [r, g, b] as ints in [0, 255].
 */
bot.color.hexToRgb = function(hexColor) {
  hexColor = bot.color.normalizeHex(hexColor);
  var r = parseInt(hexColor.substr(1, 2), 16);
  var g = parseInt(hexColor.substr(3, 2), 16);
  var b = parseInt(hexColor.substr(5, 2), 16);

  return [r, g, b];
};


/**
 * Helper for isValidHexColor_.
 * @private {!RegExp}
 * @const
 */
bot.color.VALID_HEX_COLOR_RE_ = /^#(?:[0-9a-f]{3}){1,2}$/i;


/**
 * Checks if a string is a valid hex color.  We expect strings of the format
 * #RRGGBB (ex: #1b3d5f) or #RGB (ex: #3CA == #33CCAA).
 * @param {string} str String to check.
 * @return {boolean} Whether the string is a valid hex color.
 * @private
 */
bot.color.isValidHexColor_ = function(str) {
  return bot.color.VALID_HEX_COLOR_RE_.test(str);
};


/**
 * Helper for isNormalizedHexColor_.
 * @private {!RegExp}
 * @const
 */
bot.color.NORMALIZED_HEX_COLOR_RE_ = /^#[0-9a-f]{6}$/;


/**
 * Checks if a string is a normalized hex color.
 * We expect strings of the format #RRGGBB (ex: #1b3d5f)
 * using only lowercase letters.
 * @param {string} str String to check.
 * @return {boolean} Whether the string is a normalized hex color.
 * @private
 */
bot.color.isNormalizedHexColor_ = function(str) {
  return bot.color.NORMALIZED_HEX_COLOR_RE_.test(str);
};


/**
 * Regular expression for matching and capturing RGBA style strings.
 * Helper for parseRgbaColor.
 * @private {!RegExp}
 * @const
 */
bot.color.RGBA_COLOR_RE_ =
    /^(?:rgba)?\((\d{1,3}),\s?(\d{1,3}),\s?(\d{1,3}),\s?(0|1|0\.\d*)\)$/i;


/**
 * Attempts to parse a string as an rgba color.  We expect strings of the
 * format '(r, g, b, a)', or 'rgba(r, g, b, a)', where r, g, b are ints in
 * [0, 255] and a is a float in [0, 1].
 * @param {string} str String to check.
 * @return {!Array.<number>} the integers [r, g, b, a] for valid colors or the
 *     empty array for invalid colors.
 */
bot.color.parseRgbaColor = function(str) {
  // Each component is separate (rather than using a repeater) so we can
  // capture the match. Also, we explicitly set each component to be either 0,
  // or start with a non-zero, to prevent octal numbers from slipping through.
  var regExpResultArray = str.match(bot.color.RGBA_COLOR_RE_);
  if (regExpResultArray) {
    var r = Number(regExpResultArray[1]);
    var g = Number(regExpResultArray[2]);
    var b = Number(regExpResultArray[3]);
    var a = Number(regExpResultArray[4]);
    if (r >= 0 && r <= 255 &&
        g >= 0 && g <= 255 &&
        b >= 0 && b <= 255 &&
        a >= 0 && a <= 1) {
      return [r, g, b, a];
    }
  }
  return [];
};


/**
 * Regular expression for matching and capturing RGB style strings. Helper for
 * parseRgbColor_.
 * @private {!RegExp}
 * @const
 */
bot.color.RGB_COLOR_RE_ =
    /^(?:rgb)?\((0|[1-9]\d{0,2}),\s?(0|[1-9]\d{0,2}),\s?(0|[1-9]\d{0,2})\)$/i;


/**
 * Attempts to parse a string as an rgb color.  We expect strings of the format
 * '(r, g, b)', or 'rgb(r, g, b)', where each color component is an int in
 * [0, 255].
 * @param {string} str String to check.
 * @return {!Array.<number>} the integers [r, g, b] for valid colors or the
 *     empty array for invalid colors.
 * @private
 */
bot.color.parseRgbColor_ = function(str) {
  // Each component is separate (rather than using a repeater) so we can
  // capture the match. Also, we explicitly set each component to be either 0,
  // or start with a non-zero, to prevent octal numbers from slipping through.
  var regExpResultArray = str.match(bot.color.RGB_COLOR_RE_);
  if (regExpResultArray) {
    var r = Number(regExpResultArray[1]);
    var g = Number(regExpResultArray[2]);
    var b = Number(regExpResultArray[3]);
    if (r >= 0 && r <= 255 &&
        g >= 0 && g <= 255 &&
        b >= 0 && b <= 255) {
      return [r, g, b];
    }
  }
  return [];
};


/**
 * Takes a string a prepends a '#' sign if one doesn't exist.
 * Small helper method for use by bot.color and friends.
 * @param {string} str String to check.
 * @return {string} The value passed in, prepended with a '#' if it didn't
 *     already have one.
 * @private
 */
bot.color.prependHashIfNecessary_ = function(str) {
  return str.charAt(0) == '#' ? str : '#' + str;
};


/**
 * Takes an array and appends a 1 to it if the array only contains 3 elements.
 * @param {!Array.<number>} arr The array to check.
 * @return {!Array.<number>} The same array with a 1 appended
 *  if it only contained 3 elements.
 * @private
 */
bot.color.addAlphaIfNecessary_ = function(arr) {
  if (arr.length == 3) {
    arr.push(1);
  }
  return arr;
};


/**
 * Takes an array of [r, g, b, a] and converts it into a string appropriate for
 * CSS styles.
 * @param {!Array.<number>} rgba An array with four elements.
 * @return {string} string of the form 'rgba(r, g, b, a)'.
 * @private
 */
bot.color.toRgbaStyle_ = function(rgba) {
  return 'rgba(' + rgba.join(', ') + ')';
};

// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview The file contains the base class for input devices such as
 * the keyboard, mouse, and touchscreen.
 */

goog.provide('bot.Device');

goog.require('bot');
goog.require('bot.dom');
goog.require('bot.locators');
goog.require('bot.userAgent');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.userAgent');
goog.require('goog.userAgent.product');



/**
 * A Device class that provides common functionality for input devices.
 * @param {bot.Device.ModifiersState=} opt_modifiersState state of modifier
 * keys. The state is shared, not copied from this parameter.
 *
 * @constructor
 */
bot.Device = function(opt_modifiersState) {
  /**
   * Element being interacted with.
   * @private {!Element}
   */
  this.element_ = bot.getDocument().documentElement;

  /**
   * If the element is an option, this is its parent select element.
   * @private {Element}
   */
  this.select_ = null;

  // If there is an active element, make that the current element instead.
  var activeElement = bot.dom.getActiveElement(this.element_);
  if (activeElement) {
    this.setElement(activeElement);
  }

  /**
   * State of modifier keys for this device.
   * @type {bot.Device.ModifiersState}
   * @protected
   */
  this.modifiersState = opt_modifiersState || new bot.Device.ModifiersState();
};


/**
 * Returns the element with which the device is interacting.
 *
 * @return {!Element} Element being interacted with.
 * @protected
 */
bot.Device.prototype.getElement = function() {
  return this.element_;
};


/**
 * Sets the element with which the device is interacting.
 *
 * @param {!Element} element Element being interacted with.
 * @protected
 */
bot.Device.prototype.setElement = function(element) {
  this.element_ = element;
  if (bot.dom.isElement(element, goog.dom.TagName.OPTION)) {
    this.select_ = /** @type {Element} */ (goog.dom.getAncestor(element,
        function(node) {
          return bot.dom.isElement(node, goog.dom.TagName.SELECT);
        }));
  } else {
    this.select_ = null;
  }
};


/**
 * Fires an HTML event given the state of the device.
 *
 * @param {bot.events.EventType} type HTML Event type.
 * @return {boolean} Whether the event fired successfully; false if cancelled.
 * @protected
 */
bot.Device.prototype.fireHtmlEvent = function(type) {
  return bot.events.fire(this.element_, type);
};


/**
 * Fires a keyboard event given the state of the device and the given arguments.
 * TODO(user): Populate the modifier keys in this method.
 *
 * @param {bot.events.EventType} type Keyboard event type.
 * @param {bot.events.KeyboardArgs} args Keyboard event arguments.
 * @return {boolean} Whether the event fired successfully; false if cancelled.
 * @protected
 */
bot.Device.prototype.fireKeyboardEvent = function(type, args) {
  return bot.events.fire(this.element_, type, args);
};


/**
 * Fires a mouse event given the state of the device and the given arguments.
 * TODO(user): Populate the modifier keys in this method.
 *
 * @param {bot.events.EventType} type Mouse event type.
 * @param {!goog.math.Coordinate} coord The coordinate where event will fire.
 * @param {number} button The mouse button value for the event.
 * @param {Element=} opt_related The related element of this event.
 * @param {?number=} opt_wheelDelta The wheel delta value for the event.
 * @param {boolean=} opt_force Whether the event should be fired even if the
 *     element is not interactable, such as the case of a mousemove or
 *     mouseover event that immediately follows a mouseout.
 * @return {boolean} Whether the event fired successfully; false if cancelled.
 * @protected
 */
bot.Device.prototype.fireMouseEvent = function(type, coord, button,
    opt_related, opt_wheelDelta, opt_force) {
  if (!opt_force && !bot.dom.isInteractable(this.element_)) {
    return false;
  }

  if (opt_related &&
      !(bot.events.EventType.MOUSEOVER == type ||
        bot.events.EventType.MOUSEOUT == type)) {
    throw new bot.Error(bot.ErrorCode.INVALID_ELEMENT_STATE,
                        'Event type does not allow related target: ' + type);
  }

  var args = {
    clientX: coord.x,
    clientY: coord.y,
    button: button,
    altKey: this.modifiersState.isAltPressed(),
    ctrlKey: this.modifiersState.isControlPressed(),
    shiftKey: this.modifiersState.isShiftPressed(),
    metaKey: this.modifiersState.isMetaPressed(),
    wheelDelta: opt_wheelDelta || 0,
    relatedTarget: opt_related || null
  };

  var target = this.select_ ?
      this.getTargetOfOptionMouseEvent_(type) : this.element_;
  return target ? bot.events.fire(target, type, args) : true;
};


/**
 * Fires a touch event given the state of the deive and the given arguments.
 *
 * @param {bot.events.EventType} type Event type.
 * @param {number} id The touch identifier.
 * @param {!goog.math.Coordinate} coord The coordinate where event will fire.
 * @param {number=} opt_id2 The touch identifier of the second finger.
 * @param {!goog.math.Coordinate=} opt_coord2 The coordinate of the second
 *    finger, if any.
 * @return {boolean} Whether the event fired successfully or was cancelled.
 * @protected
 */
bot.Device.prototype.fireTouchEvent = function(type, id, coord, opt_id2,
                                               opt_coord2) {
  var args = {
    touches: [],
    targetTouches: [],
    changedTouches: [],
    altKey: this.modifiersState.isAltPressed(),
    ctrlKey: this.modifiersState.isControlPressed(),
    shiftKey: this.modifiersState.isShiftPressed(),
    metaKey: this.modifiersState.isMetaPressed(),
    relatedTarget: null,
    scale: 0,
    rotation: 0
  };

  function addTouch(identifier, coords) {
    // Android devices leave identifier to zero.
    var id = goog.userAgent.product.ANDROID ? 0 : identifier;
    var touch = {
      identifier: identifier,
      screenX: coords.x,
      screenY: coords.y,
      clientX: coords.x,
      clientY: coords.y,
      pageX: coords.x,
      pageY: coords.y
    };

    args.changedTouches.push(touch);
    if (type == bot.events.EventType.TOUCHSTART ||
        type == bot.events.EventType.TOUCHMOVE) {
      args.touches.push(touch);
      args.targetTouches.push(touch);
    }
  }

  addTouch(id, coord);
  if (goog.isDef(opt_id2)) {
    addTouch(opt_id2, opt_coord2);
  }

  return bot.events.fire(this.element_, type, args);
};


/**
 * Fires a MSPointer event given the state of the device and the given
 * arguments.
 *
 * @param {bot.events.EventType} type MSPointer event type.
 * @param {!goog.math.Coordinate} coord The coordinate where event will fire.
 * @param {number} button The mouse button value for the event.
 * @param {number} pointerId The pointer id for this event.
 * @param {number} device The device type used for this event.
 * @param {boolean} isPrimary Whether the pointer represents the primary point
 *     of contact.
 * @param {Element=} opt_related The related element of this event.
 * @param {boolean=} opt_force Whether the event should be fired even if the
 *     element is not interactable, such as the case of a mousemove or
 *     mouseover event that immediately follows a mouseout.
 * @return {boolean} Whether the event fired successfully; false if cancelled.
 * @protected
 */
bot.Device.prototype.fireMSPointerEvent = function(type, coord, button,
    pointerId, device, isPrimary, opt_related, opt_force) {
  if (!opt_force && !bot.dom.isInteractable(this.element_)) {
    return false;
  }

  if (opt_related &&
      !(bot.events.EventType.MSPOINTEROVER == type ||
        bot.events.EventType.MSPOINTEROUT == type)) {
    throw new bot.Error(bot.ErrorCode.INVALID_ELEMENT_STATE,
                        'Event type does not allow related target: ' + type);
  }

  var args = {
    clientX: coord.x,
    clientY: coord.y,
    button: button,
    altKey: false,
    ctrlKey: false,
    shiftKey: false,
    metaKey: false,
    relatedTarget: opt_related || null,
    width: 0,
    height: 0,
    pressure: 0, // Pressure is only given when a stylus is used.
    rotation: 0,
    pointerId: pointerId,
    tiltX: 0,
    tiltY: 0,
    pointerType: device,
    isPrimary: isPrimary
  };

  var target = this.select_ ?
      this.getTargetOfOptionMouseEvent_(type) : this.element_;
  return target ? bot.events.fire(target, type, args) : true;
};


/**
 * A mouse event fired "on" an <option> element, doesn't always fire on the
 * <option> element itself. Sometimes it fires on the parent <select> element
 * and sometimes not at all, depending on the browser and event type. This
 * returns the true target element of the event, or null if none is fired.
 *
 * @param {bot.events.EventType} type Type of event.
 * @return {Element} Element the event should be fired on, null if none.
 * @private
 */
bot.Device.prototype.getTargetOfOptionMouseEvent_ = function(type) {
  // IE either fires the event on the parent select or not at all.
  if (goog.userAgent.IE) {
    switch (type) {
      case bot.events.EventType.MOUSEOVER:
      case bot.events.EventType.MSPOINTEROVER:
        return null;
      case bot.events.EventType.CONTEXTMENU:
      case bot.events.EventType.MOUSEMOVE:
      case bot.events.EventType.MSPOINTERMOVE:
        return this.select_.multiple ? this.select_ : null;
      default:
        return this.select_;
    }
  }

  // Opera only skips mouseovers and contextmenus on single selects.
  if (goog.userAgent.OPERA) {
    switch (type) {
      case bot.events.EventType.CONTEXTMENU:
      case bot.events.EventType.MOUSEOVER:
        return this.select_.multiple ? this.element_ : null;
      default:
        return this.element_;
    }
  }

  // WebKit always fires on the option element of multi-selects.
  // On single-selects, it either fires on the parent or not at all.
  if (goog.userAgent.WEBKIT) {
    switch (type) {
      case bot.events.EventType.CLICK:
      case bot.events.EventType.MOUSEUP:
        return this.select_.multiple ? this.element_ : this.select_;
      default:
        return this.select_.multiple ? this.element_ : null;
    }
  }

  // Firefox fires every event or the option element.
  return this.element_;
};


/**
 * A helper function to fire click events.  This method is shared between
 * the mouse and touchscreen devices.
 *
 * @param {!goog.math.Coordinate} coord The coordinate where event will fire.
 * @param {number} button The mouse button value for the event.
 * @protected
 */
bot.Device.prototype.clickElement = function(coord, button) {
  if (!bot.dom.isInteractable(this.element_)) {
    return;
  }

  // bot.events.fire(element, 'click') can trigger all onclick events, but may
  // not follow links (FORM.action or A.href).
  //     TAG      IE   GECKO  WebKit Opera
  // A(href)      No    No     Yes    Yes
  // FORM(action) No    Yes    Yes    Yes
  var targetLink = null;
  var targetButton = null;
  if (!bot.Device.ALWAYS_FOLLOWS_LINKS_ON_CLICK_) {
    for (var e = this.element_; e; e = e.parentNode) {
      if (bot.dom.isElement(e, goog.dom.TagName.A)) {
        targetLink = /**@type {!Element}*/ (e);
        break;
      } else if (bot.Device.isFormSubmitElement(e)) {
        targetButton = e;
        break;
      }
    }
  }

  var selectable = bot.dom.isSelectable(this.element_);
  var wasSelected = selectable && bot.dom.isSelected(this.element_);

  // When an element is toggled as the result of a click, the toggling and the
  // change event happens before the click event. However, on radio buttons and
  // checkboxes, the click handler can prevent the toggle from happening, so
  // for those we need to fire a click before toggling to see if the click was
  // cancelled. For option elements, we toggle unconditionally before the click.
  if (this.select_) {
    this.toggleOption_(wasSelected);
  }

  // NOTE(user): Clicking on a form submit button is a little broken:
  // (1) When clicking a form submit button in IE, firing a click event or
  // calling Form.submit() will not by itself submit the form, so we call
  // Element.click() explicitly, but as a result, the coordinates of the click
  // event are not provided. Also, when clicking on an <input type=image>, the
  // coordinates click that are submitted with the form are always (0, 0).
  // (2) When clicking a form submit button in GECKO, while the coordinates of
  // the click event are correct, those submitted with the form are always (0,0)
  // .
  // TODO(user): See if either of these can be resolved, perhaps by adding
  // hidden form elements with the coordinates before the form is submitted.
  if (goog.userAgent.IE && targetButton) {
    targetButton.click();
    return;
  }

  var performDefault = this.fireMouseEvent(
      bot.events.EventType.CLICK, coord, button);
  if (!performDefault) {
    return;
  }

  if (targetLink && bot.Device.shouldFollowHref_(targetLink)) {
    bot.Device.followHref_(targetLink);
  } else if (selectable && !this.select_) {
    this.toggleRadioButtonOrCheckbox_(wasSelected);
  }
};


/**
 * Focuses on the given element and returns true if it supports being focused
 * and does not already have focus; otherwise, returns false. If another element
 * has focus, that element will be blurred before focusing on the given element.
 *
 * @return {boolean} Whether the element was given focus.
 * @protected
 */
bot.Device.prototype.focusOnElement = function() {
  // Focusing on an <option> always focuses on the parent <select>.
  var elementToFocus = this.select_ || this.element_;

  var activeElement = bot.dom.getActiveElement(elementToFocus);
  if (elementToFocus == activeElement) {
    return false;
  }

  // If there is a currently active element, try to blur it.
  if (activeElement && (goog.isFunction(activeElement.blur) ||
      // IE reports native functions as being objects.
      goog.userAgent.IE && goog.isObject(activeElement.blur))) {
    // In IE, the focus() and blur() functions fire their respective events
    // asynchronously, and as the result, the focus/blur events fired by the
    // the atoms actions will often be in the wrong order on IE. Firing a blur
    // out of order sometimes causes IE to throw an "Unspecified error", so we
    // wrap it in a try-catch and catch and ignore the error in this case.
    try {
      if (activeElement.tagName.toLowerCase() !== 'body') {
        activeElement.blur();
      }
    } catch (e) {
      if (!(goog.userAgent.IE && e.message == 'Unspecified error.')) {
        throw e;
      }
    }

    // Sometimes IE6 and IE7 will not fire an onblur event after blur()
    // is called, unless window.focus() is called immediately afterward.
    // Note that IE8 will hit this branch unless the page is forced into
    // IE8-strict mode. This shouldn't hurt anything, we just use the
    // useragent sniff so we can compile this out for proper browsers.
    if (goog.userAgent.IE && !bot.userAgent.isEngineVersion(8)) {
      goog.dom.getWindow(goog.dom.getOwnerDocument(elementToFocus)).focus();
    }
  }

  // Try to focus on the element.
  if (goog.isFunction(elementToFocus.focus) ||
      goog.userAgent.IE && goog.isObject(elementToFocus.focus)) {
    // Opera fires focus events on hidden elements (e.g. that are hidden after
    // mousedown in a click sequence), but as of Opera 11 the focus() command
    // does not, so we fire a focus event on the hidden element explicitly.
    if (goog.userAgent.OPERA && bot.userAgent.isEngineVersion(11) &&
        !bot.dom.isShown(elementToFocus)) {
      bot.events.fire(elementToFocus, bot.events.EventType.FOCUS);
    } else {
      elementToFocus.focus();
    }
    return true;
  }

  return false;
};


/**
 * Whether links must be manually followed when clicking (because firing click
 * events doesn't follow them).
 *
 * @private {boolean}
 * @const
 */
bot.Device.ALWAYS_FOLLOWS_LINKS_ON_CLICK_ =
    goog.userAgent.WEBKIT || goog.userAgent.OPERA ||
    (bot.userAgent.FIREFOX_EXTENSION && bot.userAgent.isProductVersion(3.6));


/**
 * @param {Node} element The element to check.
 * @return {boolean} Whether the element is a submit element in form.
 * @protected
 */
bot.Device.isFormSubmitElement = function(element) {
  if (bot.dom.isElement(element, goog.dom.TagName.INPUT)) {
    var type = element.type.toLowerCase();
    if (type == 'submit' || type == 'image') {
      return true;
    }
  }

  if (bot.dom.isElement(element, goog.dom.TagName.BUTTON)) {
    var type = element.type.toLowerCase();
    if (type == 'submit') {
      return true;
    }
  }
  return false;
};


/**
 * Indicates whether we should manually follow the href of the element we're
 * clicking.
 *
 * Versions of firefox from 4+ will handle links properly when this is used in
 * an extension. Versions of Firefox prior to this may or may not do the right
 * thing depending on whether a target window is opened and whether the click
 * has caused a change in just the hash part of the URL.
 *
 * @param {!Element} element The element to consider.
 * @return {boolean} Whether following an href should be skipped.
 * @private
 */
bot.Device.shouldFollowHref_ = function(element) {
  if (bot.Device.ALWAYS_FOLLOWS_LINKS_ON_CLICK_ || !element.href) {
    return false;
  }

  if (!bot.userAgent.FIREFOX_EXTENSION) {
    return true;
  }

  if (element.target || element.href.toLowerCase().indexOf('javascript') == 0) {
    return false;
  }

  var owner = goog.dom.getWindow(goog.dom.getOwnerDocument(element));
  var sourceUrl = owner.location.href;
  var destinationUrl = bot.Device.resolveUrl_(owner.location, element.href);
  var isOnlyHashChange =
      sourceUrl.split('#')[0] === destinationUrl.split('#')[0];

  return !isOnlyHashChange;
};


/**
 * Explicitly follows the href of an anchor.
 *
 * @param {!Element} anchorElement An anchor element.
 * @private
 */
bot.Device.followHref_ = function(anchorElement) {
  var targetHref = anchorElement.href;
  var owner = goog.dom.getWindow(goog.dom.getOwnerDocument(anchorElement));

  // IE7 and earlier incorrect resolve a relative href against the top window
  // location instead of the window to which the href is assigned. As a result,
  // we have to resolve the relative URL ourselves. We do not use Closure's
  // goog.Uri to resolve, because it incorrectly fails to support empty but
  // undefined query and fragment components and re-encodes the given url.
  if (goog.userAgent.IE && !bot.userAgent.isEngineVersion(8)) {
    targetHref = bot.Device.resolveUrl_(owner.location, targetHref);
  }

  if (anchorElement.target) {
    owner.open(targetHref, anchorElement.target);
  } else {
    owner.location.href = targetHref;
  }
};


/**
 * Toggles the selected state of an option element. This is a noop if the option
 * is selected and belongs to a single-select, because it can't be toggled off.
 *
 * @param {boolean} wasSelected Whether the element was originally selected.
 * @private
 */
bot.Device.prototype.toggleOption_ = function(wasSelected) {
  var select = /** @type {!Element} */ (this.select_);
  // Cannot toggle off options in single-selects.
  if (wasSelected && !select.multiple) {
    return;
  }
  this.element_.selected = !wasSelected;
  // Only WebKit fires the change event itself and only for multi-selects,
  // except for Android versions >= 4.0.
  if (!(goog.userAgent.WEBKIT && select.multiple) ||
      (goog.userAgent.product.ANDROID && bot.userAgent.isProductVersion(4))) {
    bot.events.fire(select, bot.events.EventType.CHANGE);
  }
};


/**
 * Toggles the selected state of a radio button or checkbox. This is a noop if
 * it is a radio button that is selected, because it can't be toggled off.
 *
 * @param {boolean} wasSelected Whether the element was originally selected.
 * @private
 */
bot.Device.prototype.toggleRadioButtonOrCheckbox_ = function(wasSelected) {
  // Gecko and WebKit toggle the element as a result of a click.
  if (goog.userAgent.GECKO || goog.userAgent.WEBKIT) {
    return;
  }
  // Cannot toggle off radio buttons.
  if (wasSelected && this.element_.type.toLowerCase() == 'radio') {
    return;
  }
  this.element_.checked = !wasSelected;
  // Only Opera versions < 11 do not fire the change event themselves.
  if (goog.userAgent.OPERA && !bot.userAgent.isEngineVersion(11)) {
    bot.events.fire(this.element_, bot.events.EventType.CHANGE);
  }
};


/**
 * Find FORM element that is an ancestor of the passed in element.
 * @param {Node} node The node to find a FORM for.
 * @return {Element} The ancestor FORM element if it exists.
 * @protected
 */
bot.Device.findAncestorForm = function(node) {
  return (/** @type {Element} */ goog.dom.getAncestor(
      node, bot.Device.isForm_, /*includeNode=*/true));
};


/**
 * @param {Node} node The node to test.
 * @return {boolean} Whether the node is a FORM element.
 * @private
 */
bot.Device.isForm_ = function(node) {
  return bot.dom.isElement(node, goog.dom.TagName.FORM);
};


/**
 * Submits the specified form. Unlike the public function, it expects to be
 * given a <form> element and fails if it is not.
 * @param {!Element} form The form to submit.
 * @protected
 */
bot.Device.prototype.submitForm = function(form) {
  if (!bot.Device.isForm_(form)) {
    throw new bot.Error(bot.ErrorCode.INVALID_ELEMENT_STATE,
                        'Element is not a form, so could not submit.');
  }
  if (bot.events.fire(form, bot.events.EventType.SUBMIT)) {
    // When a form has an element with an id or name exactly equal to "submit"
    // (not uncommon) it masks the form.submit function. We  can avoid this by
    // calling the prototype's submit function, except in IE < 8, where DOM id
    // elements don't let you reference their prototypes. For IE < 8, can change
    // the id and names of the elements and revert them back, but they must be
    // reverted before the submit call, because the onsubmit handler might rely
    // on their being correct, and the HTTP request might otherwise be left with
    // incorrect value names. Fortunately, saving the submit function and
    // calling it after reverting the ids and names works! Oh, and goog.typeOf
    // (and thus goog.isFunction) doesn't work for form.submit in IE < 8.
    if (!bot.dom.isElement(form.submit)) {
      form.submit();
    } else if (!goog.userAgent.IE || bot.userAgent.isEngineVersion(8)) {
      /** @type {!Object} */ (form.constructor.prototype).submit.call(form);
    } else {
      var idMasks = bot.locators.findElements({'id': 'submit'}, form);
      var nameMasks = bot.locators.findElements({'name': 'submit'}, form);
      goog.array.forEach(idMasks, function(m) {
        m.removeAttribute('id');
      });
      goog.array.forEach(nameMasks, function(m) {
        m.removeAttribute('name');
      });
      var submitFunction = form.submit;
      goog.array.forEach(idMasks, function(m) {
        m.setAttribute('id', 'submit');
      });
      goog.array.forEach(nameMasks, function(m) {
        m.setAttribute('name', 'submit');
      });
      submitFunction();
    }
  }
};


/**
 * Regular expression for splitting up a URL into components.
 * @private {!RegExp}
 * @const
 */
bot.Device.URL_REGEXP_ = new RegExp(
    '^' +
    '([^:/?#.]+:)?' +   // protocol
    '(?://([^/]*))?' +  // host
    '([^?#]+)?' +       // pathname
    '(\\?[^#]*)?' +     // search
    '(#.*)?' +          // hash
    '$');


/**
 * Resolves a potentially relative URL against a base location.
 * @param {!Location} base Base location against which to resolve.
 * @param {string} rel Url to resolve against the location.
 * @return {string} Resolution of url against base location.
 * @private
 */
bot.Device.resolveUrl_ = function(base, rel) {
  var m = rel.match(bot.Device.URL_REGEXP_);
  if (!m) {
    return '';
  }
  var target = {
    protocol: m[1] || '',
    host: m[2] || '',
    pathname: m[3] || '',
    search: m[4] || '',
    hash: m[5] || ''
  };

  if (!target.protocol) {
    target.protocol = base.protocol;
    if (!target.host) {
      target.host = base.host;
      if (!target.pathname) {
        target.pathname = base.pathname;
        target.search = target.search || base.search;
      } else if (target.pathname.charAt(0) != '/') {
        var lastSlashIndex = base.pathname.lastIndexOf('/');
        if (lastSlashIndex != -1) {
          var directory = base.pathname.substr(0, lastSlashIndex + 1);
          target.pathname = directory + target.pathname;
        }
      }
    }
  }

  return target.protocol + '//' + target.host + target.pathname +
      target.search + target.hash;
};



/**
 * Stores the state of modifier keys
 *
 * @constructor
 */
bot.Device.ModifiersState = function() {
  /**
   * State of the modifier keys.
   * @private {number}
   */
  this.pressedModifiers_ = 0;
};


/**
 * An enum for the various modifier keys (keycode-independent).
 * @enum {number}
 */
bot.Device.Modifier = {
  SHIFT: 0x1,
  CONTROL: 0x2,
  ALT: 0x4,
  META: 0x8
};


/**
 * Checks whether a specific modifier is pressed
 * @param {!bot.Device.Modifier} modifier The modifier to check.
 * @return {boolean} Whether the modifier is pressed.
 */
bot.Device.ModifiersState.prototype.isPressed = function(modifier) {
  return (this.pressedModifiers_ & modifier) != 0;
};


/**
 * Sets the state of a given modifier.
 * @param {!bot.Device.Modifier} modifier The modifier to set.
 * @param {boolean} isPressed whether the modifier is set or released.
 */
bot.Device.ModifiersState.prototype.setPressed = function(
    modifier, isPressed) {
  if (isPressed) {
    this.pressedModifiers_ = this.pressedModifiers_ | modifier;
  } else {
    this.pressedModifiers_ = this.pressedModifiers_ & (~modifier);
  }
};


/**
 * @return {boolean} State of the Shift key.
 */
bot.Device.ModifiersState.prototype.isShiftPressed = function() {
  return this.isPressed(bot.Device.Modifier.SHIFT);
};


/**
 * @return {boolean} State of the Control key.
 */
bot.Device.ModifiersState.prototype.isControlPressed = function() {
  return this.isPressed(bot.Device.Modifier.CONTROL);
};


/**
 * @return {boolean} State of the Alt key.
 */
bot.Device.ModifiersState.prototype.isAltPressed = function() {
  return this.isPressed(bot.Device.Modifier.ALT);
};


/**
 * @return {boolean} State of the Meta key.
 */
bot.Device.ModifiersState.prototype.isMetaPressed = function() {
  return this.isPressed(bot.Device.Modifier.META);
};
// Copyright 2012 Software Freedom Conservancy
// Copyright 2010 WebDriver committers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview DOM manipulation and querying routines.
 */

goog.provide('bot.dom');

goog.require('bot');
goog.require('bot.color');
goog.require('bot.locators.xpath');
goog.require('bot.userAgent');
goog.require('bot.window');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.NodeType');
goog.require('goog.dom.TagName');
goog.require('goog.math.Box');
goog.require('goog.math.Coordinate');
goog.require('goog.math.Rect');
goog.require('goog.string');
goog.require('goog.style');
goog.require('goog.userAgent');


/**
 * Retrieves the active element for a node's owner document.
 * @param {!(Node|Window)} nodeOrWindow The node whose owner document to get
 *     the active element for.
 * @return {Element} The active element, if any.
 */
bot.dom.getActiveElement = function(nodeOrWindow) {
  return goog.dom.getActiveElement(
      goog.dom.getOwnerDocument(nodeOrWindow));
};


/**
 * Returns whether the given node is an element and, optionally, whether it has
 * the given tag name. If the tag name is not provided, returns true if the node
 * is an element, regardless of the tag name.h
 *
 * @param {Node} node The node to test.
 * @param {string=} opt_tagName Tag name to test the node for.
 * @return {boolean} Whether the node is an element with the given tag name.
 */
bot.dom.isElement = function(node, opt_tagName) {
  return !!node && node.nodeType == goog.dom.NodeType.ELEMENT &&
      (!opt_tagName || node.tagName.toUpperCase() == opt_tagName);
};


/**
 * Returns whether an element is in an interactable state: whether it is shown
 * to the user, ignoring its opacity, and whether it is enabled.
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element is interactable.
 * @see bot.dom.isShown.
 * @see bot.dom.isEnabled
 */
bot.dom.isInteractable = function(element) {
  return bot.dom.isShown(element, /*ignoreOpacity=*/true) &&
      bot.dom.isEnabled(element) &&
      !bot.dom.hasPointerEventsDisabled_(element);
};


/**
 * @param {!Element} element Element.
 * @return {boolean} Whether element is set by the CSS pointer-events property
 *     not to be interactable.
 * @private
 */
bot.dom.hasPointerEventsDisabled_ = function(element) {
  if (goog.userAgent.IE || goog.userAgent.OPERA ||
      (goog.userAgent.GECKO && !bot.userAgent.isEngineVersion('1.9.2'))) {
    // Don't support pointer events
    return false;
  }
  return bot.dom.getEffectiveStyle(element, 'pointer-events') == 'none';
};


/**
 * Returns whether the element can be checked or selected.
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element could be checked or selected.
 */
bot.dom.isSelectable = function(element) {
  if (bot.dom.isElement(element, goog.dom.TagName.OPTION)) {
    return true;
  }

  if (bot.dom.isElement(element, goog.dom.TagName.INPUT)) {
    var type = element.type.toLowerCase();
    return type == 'checkbox' || type == 'radio';
  }

  return false;
};


/**
 * Returns whether the element is checked or selected.
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element is checked or selected.
 */
bot.dom.isSelected = function(element) {
  if (!bot.dom.isSelectable(element)) {
    throw new bot.Error(bot.ErrorCode.ELEMENT_NOT_SELECTABLE,
        'Element is not selectable');
  }

  var propertyName = 'selected';
  var type = element.type && element.type.toLowerCase();
  if ('checkbox' == type || 'radio' == type) {
    propertyName = 'checked';
  }

  return !!bot.dom.getProperty(element, propertyName);
};


/**
 * List of the focusable fields, according to
 * http://www.w3.org/TR/html401/interact/scripts.html#adef-onfocus
 * @const
 * @private {!Array.<!goog.dom.TagName>}
 */
bot.dom.FOCUSABLE_FORM_FIELDS_ = [
  goog.dom.TagName.A,
  goog.dom.TagName.AREA,
  goog.dom.TagName.BUTTON,
  goog.dom.TagName.INPUT,
  goog.dom.TagName.LABEL,
  goog.dom.TagName.SELECT,
  goog.dom.TagName.TEXTAREA
];


/**
 * Returns whether a node is a focusable element.  An element may receive focus
 * if it is a form field or has a positive tabindex.
 * @param {!Element} element The node to test.
 * @return {boolean} Whether the node is focusable.
 */
bot.dom.isFocusable = function(element) {
  return goog.array.some(bot.dom.FOCUSABLE_FORM_FIELDS_, function(tagName) {
    return element.tagName.toUpperCase() == tagName;
  }) || (bot.dom.getAttribute(element, 'tabindex') != null &&
         Number(bot.dom.getProperty(element, 'tabIndex')) >= 0);
};


/**
 * Looks up the given property (not to be confused with an attribute) on the
 * given element.
 *
 * @param {!Element} element The element to use.
 * @param {string} propertyName The name of the property.
 * @return {*} The value of the property.
 */
bot.dom.getProperty = function(element, propertyName) {
  // When an <option>'s value attribute is not set, its value property should be
  // its text content, but IE < 8 does not adhere to that behavior, so fix it.
  // http://www.w3.org/TR/1999/REC-html401-19991224/interact/forms.html#adef-value-OPTION
  if (bot.userAgent.IE_DOC_PRE8 && propertyName == 'value' &&
      bot.dom.isElement(element, goog.dom.TagName.OPTION) &&
      goog.isNull(bot.dom.getAttribute(element, 'value'))) {
    return goog.dom.getRawTextContent(element);
  }
  return element[propertyName];
};


/**
 * Regex to split on semicolons, but not when enclosed in parens or quotes.
 * Helper for {@link bot.dom.standardizeStyleAttribute_}.
 * If the style attribute ends with a semicolon this will include an empty
 * string at the end of the array
 * @const
 * @private {!RegExp}
 */
bot.dom.SPLIT_STYLE_ATTRIBUTE_ON_SEMICOLONS_REGEXP_ =
    new RegExp('[;]+' +
               '(?=(?:(?:[^"]*"){2})*[^"]*$)' +
               '(?=(?:(?:[^\']*\'){2})*[^\']*$)' +
               '(?=(?:[^()]*\\([^()]*\\))*[^()]*$)');


/**
 * Standardize a style attribute value, which includes:
 *  (1) converting all property names lowercase
 *  (2) ensuring it ends in a trailing semi-colon
 *  (3) removing empty style values (which only appear on Opera).
 * @param {string} value The style attribute value.
 * @return {string} The identical value, with the formatting rules described
 *     above applied.
 * @private
 */
bot.dom.standardizeStyleAttribute_ = function(value) {
  var styleArray = value.split(
      bot.dom.SPLIT_STYLE_ATTRIBUTE_ON_SEMICOLONS_REGEXP_);
  var css = [];
  goog.array.forEach(styleArray, function(pair) {
    var i = pair.indexOf(':');
    if (i > 0) {
      var keyValue = [pair.slice(0, i), pair.slice(i + 1)];
      if (keyValue.length == 2) {
        css.push(keyValue[0].toLowerCase(), ':', keyValue[1], ';');
      }
    }
  });
  css = css.join('');
  css = css.charAt(css.length - 1) == ';' ? css : css + ';';
  return goog.userAgent.OPERA ? css.replace(/\w+:;/g, '') : css;
};


/**
 * Get the user-specified value of the given attribute of the element, or null
 * if the attribute is not present.
 *
 * <p>For boolean attributes such as "selected" or "checked", this method
 * returns the value of element.getAttribute(attributeName) cast to a String
 * when attribute is present. For modern browsers, this will be the string the
 * attribute is given in the HTML, but for IE8 it will be the name of the
 * attribute, and for IE7, it will be the string "true". To test whether a
 * boolean attribute is present, test whether the return value is non-null, the
 * same as one would for non-boolean attributes. Specifically, do *not* test
 * whether the boolean evaluation of the return value is true, because the value
 * of a boolean attribute that is present will often be the empty string.
 *
 * <p>For the style attribute, it standardizes the value by lower-casing the
 * property names and always including a trailing semi-colon.
 *
 * @param {!Element} element The element to use.
 * @param {string} attributeName The name of the attribute to return.
 * @return {?string} The value of the attribute or "null" if entirely missing.
 */
bot.dom.getAttribute = function(element, attributeName) {
  attributeName = attributeName.toLowerCase();

  // The style attribute should be a css text string that includes only what
  // the HTML element specifies itself (excluding what is inherited from parent
  // elements or style sheets). We standardize the format of this string via the
  // standardizeStyleAttribute method.
  if (attributeName == 'style') {
    return bot.dom.standardizeStyleAttribute_(element.style.cssText);
  }

  // In IE doc mode < 8, the "value" attribute of an <input> is only accessible
  // as a property.
  if (bot.userAgent.IE_DOC_PRE8 && attributeName == 'value' &&
      bot.dom.isElement(element, goog.dom.TagName.INPUT)) {
    return element['value'];
  }

  // In IE < 9, element.getAttributeNode will return null for some boolean
  // attributes that are present, such as the selected attribute on <option>
  // elements. This if-statement is sufficient if these cases are restricted
  // to boolean attributes whose reflected property names are all lowercase
  // (as attributeName is by this point), like "selected". We have not
  // found a boolean attribute for which this does not work.
  if (bot.userAgent.IE_DOC_PRE9 && element[attributeName] === true) {
    return String(element.getAttribute(attributeName));
  }

  // When the attribute is not present, either attr will be null or
  // attr.specified will be false.
  var attr = element.getAttributeNode(attributeName);
  return (attr && attr.specified) ? attr.value : null;
};


/**
 * List of elements that support the "disabled" attribute, as defined by the
 * HTML 4.01 specification.
 * @const
 * @private {!Array.<goog.dom.TagName>}
 * @see http://www.w3.org/TR/html401/interact/forms.html#h-17.12.1
 */
bot.dom.DISABLED_ATTRIBUTE_SUPPORTED_ = [
  goog.dom.TagName.BUTTON,
  goog.dom.TagName.INPUT,
  goog.dom.TagName.OPTGROUP,
  goog.dom.TagName.OPTION,
  goog.dom.TagName.SELECT,
  goog.dom.TagName.TEXTAREA
];


/**
 * Determines if an element is enabled. An element is considered enabled if it
 * does not support the "disabled" attribute, or if it is not disabled.
 * @param {!Element} el The element to test.
 * @return {boolean} Whether the element is enabled.
 */
bot.dom.isEnabled = function(el) {
  var tagName = el.tagName.toUpperCase();
  if (!goog.array.contains(bot.dom.DISABLED_ATTRIBUTE_SUPPORTED_, tagName)) {
    return true;
  }

  if (bot.dom.getProperty(el, 'disabled')) {
    return false;
  }

  // The element is not explicitly disabled, but if it is an OPTION or OPTGROUP,
  // we must test if it inherits its state from a parent.
  if (el.parentNode &&
      el.parentNode.nodeType == goog.dom.NodeType.ELEMENT &&
      goog.dom.TagName.OPTGROUP == tagName ||
      goog.dom.TagName.OPTION == tagName) {
    return bot.dom.isEnabled((/**@type{!Element}*/el.parentNode));
  }

  // Is there an ancestor of the current element that is a disabled fieldset
  // and whose child is also an ancestor-or-self of the current element but is
  // not the first legend child of the fieldset. If so then the element is
  // disabled.
  if (goog.dom.getAncestor(el, function (e) {
    var parent = e.parentNode;

    if (parent &&
        bot.dom.isElement(parent, goog.dom.TagName.FIELDSET) &&
        bot.dom.getProperty(/** @type {!Element} */ (parent), 'disabled')) {
      if (!bot.dom.isElement(e, goog.dom.TagName.LEGEND)) {
        return true;
      }

      var sibling = e;
      // Are there any previous legend siblings? If so then we are not the
      // first and the element is disabled
      while (sibling = goog.dom.getPreviousElementSibling(sibling)) {
        if (bot.dom.isElement(sibling, goog.dom.TagName.LEGEND)) {
          return true;
        }
      }
    }
    return false;
  }, true)) {
    return false;
  }

  return true;
};


/**
 * List of input types that create text fields.
 * @const
 * @private {!Array.<String>}
 * @see http://www.whatwg.org/specs/web-apps/current-work/multipage/the-input-element.html#attr-input-type
 */
bot.dom.TEXTUAL_INPUT_TYPES_ = [
  'text',
  'search',
  'tel',
  'url',
  'email',
  'password',
  'number'
];


/**
 * TODO(gdennis): Add support for designMode elements.
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element accepts user-typed text.
 */
bot.dom.isTextual = function(element) {
  if (bot.dom.isElement(element, goog.dom.TagName.TEXTAREA)) {
    return true;
  }

  if (bot.dom.isElement(element, goog.dom.TagName.INPUT)) {
    var type = element.type.toLowerCase();
    return goog.array.contains(bot.dom.TEXTUAL_INPUT_TYPES_, type);
  }

  if (bot.dom.isContentEditable(element)) {
    return true;
  }

  return false;
};


/**
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element is contentEditable.
 */
bot.dom.isContentEditable = function(element) {
  // Check if browser supports contentEditable.
  if (!goog.isDef(element['contentEditable'])) {
    return false;
  }

  // Checking the element's isContentEditable property is preferred except for
  // IE where that property is not reliable on IE versions 7, 8, and 9.
  if (!goog.userAgent.IE && goog.isDef(element['isContentEditable'])) {
    return element.isContentEditable;
  }

  // For IE and for browsers where contentEditable is supported but
  // isContentEditable is not, traverse up the ancestors:
  function legacyIsContentEditable(e) {
    if (e.contentEditable == 'inherit') {
      var parent = bot.dom.getParentElement(e);
      return parent ? legacyIsContentEditable(parent) : false;
    } else {
      return e.contentEditable == 'true';
    }
  }
  return legacyIsContentEditable(element);
};


/**
 * TODO(gdennis): Merge isTextual into this function and move to bot.dom.
 * For Puppet, requires adding support to getVisibleText for grabbing
 * text from all textual elements.
 *
 * Whether the element may contain text the user can edit.
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element accepts user-typed text.
 */
bot.dom.isEditable = function(element) {
  return bot.dom.isTextual(element) &&
      !bot.dom.getProperty(element, 'readOnly');
};


/**
 * Returns the parent element of the given node, or null. This is required
 * because the parent node may not be another element.
 *
 * @param {!Node} node The node who's parent is desired.
 * @return {Element} The parent element, if available, null otherwise.
 */
bot.dom.getParentElement = function(node) {
  var elem = node.parentNode;

  while (elem &&
         elem.nodeType != goog.dom.NodeType.ELEMENT &&
         elem.nodeType != goog.dom.NodeType.DOCUMENT &&
         elem.nodeType != goog.dom.NodeType.DOCUMENT_FRAGMENT) {
    elem = elem.parentNode;
  }
  return (/** @type {Element} */ bot.dom.isElement(elem) ? elem : null);
};


/**
 * Retrieves an explicitly-set, inline style value of an element. This returns
 * '' if there isn't a style attribute on the element or if this style property
 * has not been explicitly set in script.
 *
 * @param {!Element} elem Element to get the style value from.
 * @param {string} styleName Name of the style property in selector-case.
 * @return {string} The value of the style property.
 */
bot.dom.getInlineStyle = function(elem, styleName) {
  return goog.style.getStyle(elem, styleName);
};


/**
 * Retrieves the implicitly-set, effective style of an element, or null if it is
 * unknown. It returns the computed style where available; otherwise it looks
 * up the DOM tree for the first style value not equal to 'inherit,' using the
 * IE currentStyle of each node if available, and otherwise the inline style.
 * Since the computed, current, and inline styles can be different, the return
 * value of this function is not always consistent across browsers. See:
 * http://code.google.com/p/doctype/wiki/ArticleComputedStyleVsCascadedStyle
 *
 * @param {!Element} elem Element to get the style value from.
 * @param {string} propertyName Name of the CSS property in selector-case.
 * @return {?string} The value of the style property, or null.
 */
bot.dom.getEffectiveStyle = function(elem, propertyName) {
  var styleName = goog.string.toCamelCase(propertyName);
  if (styleName == 'float' ||
      styleName == 'cssFloat' ||
      styleName == 'styleFloat') {
    styleName = bot.userAgent.IE_DOC_PRE9 ? 'styleFloat' : 'cssFloat';
  }
  var style = goog.style.getComputedStyle(elem, styleName) ||
      bot.dom.getCascadedStyle_(elem, styleName);
  if (style === null) {
    return null;
  }
  return bot.color.standardizeColor(propertyName, style);
};


/**
 * Looks up the DOM tree for the first style value not equal to 'inherit,' using
 * the currentStyle of each node if available, and otherwise the inline style.
 *
 * @param {!Element} elem Element to get the style value from.
 * @param {string} styleName CSS style property in camelCase.
 * @return {?string} The value of the style property, or null.
 * @private
 */
bot.dom.getCascadedStyle_ = function(elem, styleName) {
  var style = elem.currentStyle || elem.style;
  var value = style[styleName];
  if (!goog.isDef(value) && goog.isFunction(style['getPropertyValue'])) {
    value = style['getPropertyValue'](styleName);
  }

  if (value != 'inherit') {
    return goog.isDef(value) ? value : null;
  }
  var parent = bot.dom.getParentElement(elem);
  return parent ? bot.dom.getCascadedStyle_(parent, styleName) : null;
};


/**
 * Would a user see scroll bars on the BODY element? In the case where the BODY
 * has "overflow: hidden", and HTML has "overflow: auto" or "overflow: scroll"
 * set, there's a scroll bar, so it's as if the BODY has "overflow: auto" set.
 * In all other cases where BODY has "overflow: hidden", there are no
 * scrollbars. http://www.w3.org/TR/CSS21/visufx.html#overflow
 *
 * @param {!Element} bodyElement The element, which must be a BODY element.
 * @return {boolean} Whether scrollbars would be visible to a user.
 * @private
 */
bot.dom.isBodyScrollBarShown_ = function(bodyElement) {
  if (!bot.dom.isElement(bodyElement, goog.dom.TagName.BODY)) {
    // bail
    }

  var bodyOverflow = bot.dom.getEffectiveStyle(bodyElement, 'overflow');
  if (bodyOverflow != 'hidden') {
    return true;
  }

  var html = bot.dom.getParentElement(bodyElement);
  if (!html || !bot.dom.isElement(html, goog.dom.TagName.HTML)) {
    return true; // Seems like a reasonable default.
  }

  var viewportOverflow = bot.dom.getEffectiveStyle(html, 'overflow');
  return viewportOverflow == 'auto' || viewportOverflow == 'scroll';
};


/**
 * @param {!Element} element The element to use.
 * @return {!goog.math.Size} The dimensions of the element.
 */
bot.dom.getElementSize = function(element) {
  if (goog.isFunction(element['getBBox']) && !bot.dom.isElement(element, goog.dom.TagName.SVG)) {
    try {
      var bb = element['getBBox']();
      if (bb) {
        // Opera will return an undefined bounding box for SVG elements.
        // Which makes sense, but isn't useful.
        return bb;
      }
    } catch (e) {
      // Firefox will always throw for certain SVG elements,
      // even if the function exists.
    }
  }

  // If the element is the BODY, then get the visible size.
  if (bot.dom.isElement(element, goog.dom.TagName.BODY)) {
    var doc = goog.dom.getOwnerDocument(element);
    var win = goog.dom.getWindow(doc) || undefined;
    if (!bot.dom.isBodyScrollBarShown_(element)) {
      return goog.dom.getViewportSize(win);
    }
    return bot.window.getInteractableSize(win);
  }

  return goog.style.getSize(element);
};


/**
 * Determines whether an element is what a user would call "shown". This means
 * that the element is shown in the viewport of the browser, and only has
 * height and width greater than 0px, and that its visibility is not "hidden"
 * and its display property is not "none".
 * Options and Optgroup elements are treated as special cases: they are
 * considered shown iff they have a enclosing select element that is shown.
 *
 * @param {!Element} elem The element to consider.
 * @param {boolean=} opt_ignoreOpacity Whether to ignore the element's opacity
 *     when determining whether it is shown; defaults to false.
 * @return {boolean} Whether or not the element is visible.
 */
bot.dom.isShown = function(elem, opt_ignoreOpacity) {
  if (!bot.dom.isElement(elem)) {
    throw new Error('Argument to isShown must be of type Element');
  }

  // Option or optgroup is shown iff enclosing select is shown (ignoring the
  // select's opacity).
  if (bot.dom.isElement(elem, goog.dom.TagName.OPTION) ||
      bot.dom.isElement(elem, goog.dom.TagName.OPTGROUP)) {
    var select = /**@type {Element}*/ (goog.dom.getAncestor(elem, function(e) {
      return bot.dom.isElement(e, goog.dom.TagName.SELECT);
    }));
    return !!select && bot.dom.isShown(select, /*ignoreOpacity=*/true);
  }

  // Map is shown iff image that uses it is shown.
  if (bot.dom.isElement(elem, goog.dom.TagName.MAP)) {
    if (!elem.name) {
      return false;
    }
    var mapDoc = goog.dom.getOwnerDocument(elem);
    var mapImage;
    // TODO(gdennis): Avoid brute-force search once a cross-browser xpath
    // locator is available.
    if (mapDoc['evaluate']) {
      // The "//*" XPath syntax can confuse the closure compiler, so we use
      // the "/descendant::*" syntax instead.
      // TODO(jleyba): Try to find a reproducible case for the compiler bug.
      // TODO(jleyba): Restrict to applet, img, input:image, and object nodes.
      var imageXpath = '/descendant::*[@usemap = "#' + elem.name + '"]';

      // TODO(gdennis): Break dependency of bot.locators on bot.dom,
      // so bot.locators.findElement can be called here instead.
      mapImage = bot.locators.xpath.single(imageXpath, mapDoc);
    } else {
      mapImage = goog.dom.findNode(mapDoc, function(n) {
        return bot.dom.isElement(n) &&
               bot.dom.getAttribute(
                   /** @type {!Element} */ (n), 'usemap') == '#' + elem.name;
      });
    }
    return !!mapImage && bot.dom.isShown((/** @type {!Element} */ mapImage),
        opt_ignoreOpacity);
  }

  // Area is shown iff enclosing map is shown.
  if (bot.dom.isElement(elem, goog.dom.TagName.AREA)) {
    var map = /**@type {Element}*/ (goog.dom.getAncestor(elem, function(e) {
      return bot.dom.isElement(e, goog.dom.TagName.MAP);
    }));
    return !!map && bot.dom.isShown(map, opt_ignoreOpacity);
  }

  // Any hidden input is not shown.
  if (bot.dom.isElement(elem, goog.dom.TagName.INPUT) &&
      elem.type.toLowerCase() == 'hidden') {
    return false;
  }

  // Any NOSCRIPT element is not shown.
  if (bot.dom.isElement(elem, goog.dom.TagName.NOSCRIPT)) {
    return false;
  }

  // Any element with hidden visibility is not shown.
  if (bot.dom.getEffectiveStyle(elem, 'visibility') == 'hidden') {
    return false;
  }

  // Any element with a display style equal to 'none' or that has an ancestor
  // with display style equal to 'none' is not shown.
  function displayed(e) {
    if (bot.dom.getEffectiveStyle(e, 'display') == 'none') {
      return false;
    }
    var parent = bot.dom.getParentElement(e);
    return !parent || displayed(parent);
  }
  if (!displayed(elem)) {
    return false;
  }

  // Any transparent element is not shown.
  if (!opt_ignoreOpacity && bot.dom.getOpacity(elem) == 0) {
    return false;
  }

  // Any element without positive size dimensions is not shown.
  function positiveSize(e) {
    var size = bot.dom.getElementSize(e);
    if (size.height > 0 && size.width > 0) {
      return true;
    }
    // A vertical or horizontal SVG Path element will report zero width or
    // height but is "shown" if it has a positive stroke-width.
    if (bot.dom.isElement(e, 'PATH') && (size.height > 0 || size.width > 0)) {
      var strokeWidth = bot.dom.getEffectiveStyle(e, 'stroke-width');
      return !!strokeWidth && (parseInt(strokeWidth, 10) > 0);
    }
    // Zero-sized elements should still be considered to have positive size
    // if they have a child element or text node with positive size.
    return goog.array.some(e.childNodes, function(n) {
      return (n.nodeType == goog.dom.NodeType.TEXT &&
              bot.dom.getEffectiveStyle(e, 'overflow') != 'hidden') ||
              (bot.dom.isElement(n) && positiveSize(n));
    });
  }
  if (!positiveSize(elem)) {
    return false;
  }

  // Elements should be hidden if their parent has a fixed size AND has the
  // style overflow:hidden AND the element's location is not within the fixed
  // size of the parent
  function isOverflowHiding(e, block) {
    var parent;
    if (block == null) {
      parent = goog.dom.getParentElement(e);
    } else {
      parent = goog.dom.getParentElement(block);
    }

    if (parent && (bot.dom.getEffectiveStyle(parent, 'overflow-x') == 'hidden' ||
        bot.dom.getEffectiveStyle(parent, 'overflow-y') == 'hidden')) {
      var sizeOfParent = bot.dom.getElementSize(parent);
      var locOfParent = goog.style.getClientPosition(parent);
      var locOfElement = goog.style.getClientPosition(e);
      if (locOfParent.x + sizeOfParent.width <= locOfElement.x &&
          bot.dom.getEffectiveStyle(parent, 'overflow-x') == 'hidden') {
        return false;
      }
      if (locOfParent.y + sizeOfParent.height <= locOfElement.y &&
          bot.dom.getEffectiveStyle(parent, 'overflow-y') == 'hidden') {
        return false;
      }
      return true;
    }
    return !parent || isOverflowHiding(e, parent);
  }

  if (!isOverflowHiding(elem, null)) {
    return false;
  }

  function isTransformHiding(e) {
    var transform = bot.dom.getEffectiveStyle(e, '-o-transform') ||
                    bot.dom.getEffectiveStyle(e, '-webkit-transform') ||
                    bot.dom.getEffectiveStyle(e, '-ms-transform') ||
                    bot.dom.getEffectiveStyle(e, '-moz-transform') ||
                    bot.dom.getEffectiveStyle(e, 'transform');

    // Not all browsers know what a transform is so if we have a returned value
    // lets carry on checking up the tree just in case. If we ask for the 
    // transform matrix and look at the details there it will return the centre
    // of the element
    if (transform && transform !== "none") {
      var locOfElement = goog.style.getClientPosition(e);
      var sizeOfElement = bot.dom.getElementSize(e);
      if ((locOfElement.x + (sizeOfElement.width)) >= 0 && 
          (locOfElement.y + (sizeOfElement.height)) >= 0){
        return true;
      } else {
        return false;
      }
    } else {
      var parent = bot.dom.getParentElement(e);
      return !parent || isTransformHiding(parent);
    }
  }
  return isTransformHiding(elem);
};

/**
* Checks whether the element is currently scrolled into the parent's overflow
* region, such that the offset given, relative to the top-left corner of the
* element, is currently in the overflow region.
*
* @param {!Element} element The element to check.
* @param {!goog.math.Coordinate=} opt_coords Coordinate in the element,
*     relative to the top-left corner of the element, to check. If none are
*     specified, checks that the center of the element is in in the overflow.
* @return {boolean} Whether the coordinates specified, relative to the element,
*     are scrolled in the parent overflow.
*/
bot.dom.isInParentOverflow = function (element, opt_coords) {
  var parent = goog.style.getOffsetParent(element);
  var parentNode = goog.userAgent.GECKO || goog.userAgent.IE ||
      goog.userAgent.OPERA ? bot.dom.getParentElement(element) : parent;

  // Gecko will skip the BODY tag when calling getOffsetParent. However, the
  // combination of the overflow values on the BODY _and_ HTML tags determine
  // whether scroll bars are shown, so we need to guarantee that both values
  // are checked.
  if ((goog.userAgent.GECKO || goog.userAgent.IE || goog.userAgent.OPERA) &&
      bot.dom.isElement(parentNode, goog.dom.TagName.BODY)) {
    parent = parentNode;
  }

  if (parent && (bot.dom.getEffectiveStyle(parent, 'overflow') == 'scroll' ||
                 bot.dom.getEffectiveStyle(parent, 'overflow') == 'auto')) {
    var sizeOfParent = bot.dom.getElementSize(parent);
    var locOfParent = goog.style.getClientPosition(parent);
    var locOfElement = goog.style.getClientPosition(element);
    var offsetX, offsetY;
    if (opt_coords) {
      offsetX = opt_coords.x;
      offsetY = opt_coords.y;
    } else {
      var sizeOfElement = bot.dom.getElementSize(element);
      offsetX = sizeOfElement.width / 2;
      offsetY = sizeOfElement.height / 2;
    }
    var elementPointX = locOfElement.x + offsetX;
    var elementPointY = locOfElement.y + offsetY;
    if (elementPointX >= locOfParent.x + sizeOfParent.width) {
      return true;
    }
    if (elementPointX <= locOfParent.x) {
      return true;
    }
    if (elementPointY >= locOfParent.y + sizeOfParent.height) {
      return true;
    }
    if (elementPointY <= locOfParent.y) {
      return true;
    }
    return bot.dom.isInParentOverflow(parent);
  }
  return false;
};

/**
 * Trims leading and trailing whitespace from strings, leaving non-breaking
 * space characters in place.
 *
 * @param {string} str The string to trim.
 * @return {string} str without any leading or trailing whitespace characters
 *     except non-breaking spaces.
 * @private
 */
bot.dom.trimExcludingNonBreakingSpaceCharacters_ = function(str) {
  return str.replace(/^[^\S\xa0]+|[^\S\xa0]+$/g, '');
};


/**
 * @param {!Element} elem The element to consider.
 * @return {string} visible text.
 */
bot.dom.getVisibleText = function(elem) {
  var lines = [];
  bot.dom.appendVisibleTextLinesFromElement_(elem, lines);
  lines = goog.array.map(
      lines,
      bot.dom.trimExcludingNonBreakingSpaceCharacters_);
  var joined = lines.join('\n');
  var trimmed = bot.dom.trimExcludingNonBreakingSpaceCharacters_(joined);

  // Replace non-breakable spaces with regular ones.
  return trimmed.replace(/\xa0/g, ' ');
};


/**
 * @param {!Element} elem Element.
 * @param {!Array.<string>} lines Accumulated visible lines of text.
 * @private
 */
bot.dom.appendVisibleTextLinesFromElement_ = function(elem, lines) {
  function currLine() {
    return (/** @type {string|undefined} */ goog.array.peek(lines)) || '';
  }

  // TODO(gdennis): Add case here for textual form elements.
  if (bot.dom.isElement(elem, goog.dom.TagName.BR)) {
    lines.push('');
  } else {
    // TODO: properly handle display:run-in
    var isTD = bot.dom.isElement(elem, goog.dom.TagName.TD);
    var display = bot.dom.getEffectiveStyle(elem, 'display');
    // On some browsers, table cells incorrectly show up with block styles.
    var isBlock = !isTD &&
        !goog.array.contains(bot.dom.INLINE_DISPLAY_BOXES_, display);

    // Add a newline before block elems when there is text on the current line,
    // except when the previous sibling has a display: run-in.
    // Also, do not run-in the previous sibling if this element is floated.

    var previousElementSibling = goog.dom.getPreviousElementSibling(elem);
    var prevDisplay = (previousElementSibling) ?
        bot.dom.getEffectiveStyle(previousElementSibling, 'display') : '';
    // TODO(dawagner): getEffectiveStyle should mask this for us
    var thisFloat = bot.dom.getEffectiveStyle(elem, 'float') ||
        bot.dom.getEffectiveStyle(elem, 'cssFloat') ||
        bot.dom.getEffectiveStyle(elem, 'styleFloat');
    var runIntoThis = prevDisplay == 'run-in' && thisFloat == 'none';
    if (isBlock && !runIntoThis && !goog.string.isEmpty(currLine())) {
      lines.push('');
    }

    // This element may be considered unshown, but have a child that is
    // explicitly shown (e.g. this element has "visibility:hidden").
    // Nevertheless, any text nodes that are direct descendants of this
    // element will not contribute to the visible text.
    var shown = bot.dom.isShown(elem);

    // All text nodes that are children of this element need to know the
    // effective "white-space" and "text-transform" styles to properly
    // compute their contribution to visible text. Compute these values once.
    var whitespace = null, textTransform = null;
    if (shown) {
      whitespace = bot.dom.getEffectiveStyle(elem, 'white-space');
      textTransform = bot.dom.getEffectiveStyle(elem, 'text-transform');
    }

    goog.array.forEach(elem.childNodes, function(node) {
      if (node.nodeType == goog.dom.NodeType.TEXT && shown) {
        var textNode = (/** @type {!Text} */ node);
        bot.dom.appendVisibleTextLinesFromTextNode_(textNode, lines,
            whitespace, textTransform);
      } else if (bot.dom.isElement(node)) {
        var castElem = (/** @type {!Element} */ node);
        bot.dom.appendVisibleTextLinesFromElement_(castElem, lines);
      }
    });

    var line = currLine();

    // Here we differ from standard innerText implementations (if there were
    // such a thing). Usually, table cells are separated by a tab, but we
    // normalize tabs into single spaces.
    if ((isTD || display == 'table-cell') && line &&
        !goog.string.endsWith(line, ' ')) {
      lines[lines.length - 1] += ' ';
    }

    // Add a newline after block elems when there is text on the current line,
    // and the current element isn't marked as run-in.
    if (isBlock && display != 'run-in' && !goog.string.isEmpty(line)) {
      lines.push('');
    }
  }
};


/**
 * Elements with one of these effective "display" styles are treated as inline
 * display boxes and have their visible text appended to the current line.
 * @private {!Array.<string>}
 * @const
 */
bot.dom.INLINE_DISPLAY_BOXES_ = [
  'inline',
  'inline-block',
  'inline-table',
  'none',
  'table-cell',
  'table-column',
  'table-column-group'
];


/**
 * @param {!Text} textNode Text node.
 * @param {!Array.<string>} lines Accumulated visible lines of text.
 * @param {?string} whitespace Parent element's "white-space" style.
 * @param {?string} textTransform Parent element's "text-transform" style.
 * @private
 */
bot.dom.appendVisibleTextLinesFromTextNode_ = function(textNode, lines,
    whitespace, textTransform) {
  // First, replace all zero-width spaces. Do this before regularizing spaces
  // as the zero-width space is, by definition, a space.
  var text = textNode.nodeValue.replace(/\u200b/g, '');

  // Canonicalize the new lines, and then collapse new lines
  // for the whitespace styles that collapse. See:
  // https://developer.mozilla.org/en/CSS/white-space
  text = goog.string.canonicalizeNewlines(text);
  if (whitespace == 'normal' || whitespace == 'nowrap') {
    text = text.replace(/\n/g, ' ');
  }

  // For pre and pre-wrap whitespace styles, convert all breaking spaces to be
  // non-breaking, otherwise, collapse all breaking spaces. Breaking spaces are
  // converted to regular spaces by getVisibleText().
  if (whitespace == 'pre' || whitespace == 'pre-wrap') {
    text = text.replace(/[ \f\t\v\u2028\u2029]/g, '\xa0');
  } else {
    text = text.replace(/[\ \f\t\v\u2028\u2029]+/g, ' ');
  }

  if (textTransform == 'capitalize') {
    text = text.replace(/(^|\s)(\S)/g, function() {
      return arguments[1] + arguments[2].toUpperCase();
    });
  } else if (textTransform == 'uppercase') {
    text = text.toUpperCase();
  } else if (textTransform == 'lowercase') {
    text = text.toLowerCase();
  }

  var currLine = lines.pop() || '';
  if (goog.string.endsWith(currLine, ' ') &&
      goog.string.startsWith(text, ' ')) {
    text = text.substr(1);
  }
  lines.push(currLine + text);
};


/**
 * Gets the opacity of a node (x-browser).
 * This gets the inline style opacity of the node and takes into account the
 * cascaded or the computed style for this node.
 *
 * @param {!Element} elem Element whose opacity has to be found.
 * @return {number} Opacity between 0 and 1.
 */
bot.dom.getOpacity = function(elem) {
  // TODO(bsilverberg): Does this need to deal with rgba colors?
  if (!bot.userAgent.IE_DOC_PRE10) {
    return bot.dom.getOpacityNonIE_(elem);
  } else {
    if (bot.dom.getEffectiveStyle(elem, 'position') == 'relative') {
      // Filter does not apply to non positioned elements.
      return 1;
    }

    var opacityStyle = bot.dom.getEffectiveStyle(elem, 'filter');
    var groups = opacityStyle.match(/^alpha\(opacity=(\d*)\)/) ||
        opacityStyle.match(
        /^progid:DXImageTransform.Microsoft.Alpha\(Opacity=(\d*)\)/);

    if (groups) {
      return Number(groups[1]) / 100;
    } else {
      return 1; // Opaque.
    }
  }
};


/**
 * Implementation of getOpacity for browsers that do support
 * the "opacity" style.
 *
 * @param {!Element} elem Element whose opacity has to be found.
 * @return {number} Opacity between 0 and 1.
 * @private
 */
bot.dom.getOpacityNonIE_ = function(elem) {
  // By default the element is opaque.
  var elemOpacity = 1;

  var opacityStyle = bot.dom.getEffectiveStyle(elem, 'opacity');
  if (opacityStyle) {
    elemOpacity = Number(opacityStyle);
  }

  // Let's apply the parent opacity to the element.
  var parentElement = bot.dom.getParentElement(elem);
  if (parentElement) {
    elemOpacity = elemOpacity * bot.dom.getOpacityNonIE_(parentElement);
  }
  return elemOpacity;
};


/**
 * This function calculates the amount of scrolling necessary to bring the
 * target location into view.
 *
 * @param {number} targetLocation The target location relative to the current
 *     viewport.
 * @param {number} viewportDimension The size of the current viewport.
 * @return {number} Returns the scroll offset necessary to bring the given
 *     target location into view.
 * @private
 */
bot.dom.calculateViewportScrolling_ =
    function(targetLocation, viewportDimension) {

  if (targetLocation >= viewportDimension) {
    // Scroll until the target location appears on the right/bottom side of
    // the viewport.
    return targetLocation - (viewportDimension - 1);
  }

  if (targetLocation < 0) {
    // Scroll until the target location appears on the left/top side of the
    // viewport.
    return targetLocation;
  }

  // The location is already within the viewport. No scrolling necessary.
  return 0;
};


/**
 * This function takes a relative location according to the current viewport. If
 * this location is not visible in the viewport, it scrolls the location into
 * view. The function returns the new relative location after scrolling.
 *
 * @param {!goog.math.Coordinate} targetLocation The target location relative
 *     to (0, 0) coordinate of the viewport.
 * @param {Window=} opt_currentWindow The current browser window.
 * @return {!goog.math.Coordinate} The target location within the viewport
 *     after scrolling.
 */
bot.dom.getInViewLocation =
    function(targetLocation, opt_currentWindow) {
  var currentWindow = opt_currentWindow || bot.getWindow();
  var viewportSize = goog.dom.getViewportSize(currentWindow);

  var xScrolling = bot.dom.calculateViewportScrolling_(
      targetLocation.x,
      viewportSize.width);

  var yScrolling = bot.dom.calculateViewportScrolling_(
      targetLocation.y,
      viewportSize.height);

  var scrollOffset =
      goog.dom.getDomHelper(currentWindow.document).getDocumentScroll();

  if (xScrolling != 0 || yScrolling != 0) {
    currentWindow.scrollBy(xScrolling, yScrolling);
  }

  // It is difficult to determine the size of the web page in some browsers.
  // We check if the scrolling we intended to do really happened. If not we
  // assume that the target location is not on the web page.
  var newScrollOffset =
      goog.dom.getDomHelper(currentWindow.document).getDocumentScroll();

  if ((scrollOffset.x + xScrolling != newScrollOffset.x) ||
      (scrollOffset.y + yScrolling != newScrollOffset.y)) {
    throw new bot.Error(bot.ErrorCode.MOVE_TARGET_OUT_OF_BOUNDS,
        'The target location (' + (targetLocation.x + scrollOffset.x) +
        ', ' + (targetLocation.y + scrollOffset.y) + ') is not on the ' +
        'webpage.');
  }

  var inViewLocation = new goog.math.Coordinate(
      targetLocation.x - xScrolling,
      targetLocation.y - yScrolling);

  // The target location should be within the viewport after scrolling.
  // This is assertion code. We do not expect them ever to become true.
  if (0 > inViewLocation.x || inViewLocation.x >= viewportSize.width) {
    throw new bot.Error(bot.ErrorCode.MOVE_TARGET_OUT_OF_BOUNDS,
        'The target location (' +
        inViewLocation.x + ', ' + inViewLocation.y +
        ') should be within the viewport (' +
        viewportSize.width + ':' + viewportSize.height +
        ') after scrolling.');
  }

  if (0 > inViewLocation.y || inViewLocation.y >= viewportSize.height) {
    throw new bot.Error(bot.ErrorCode.MOVE_TARGET_OUT_OF_BOUNDS,
        'The target location (' +
        inViewLocation.x + ', ' + inViewLocation.y +
        ') should be within the viewport (' +
        viewportSize.width + ':' + viewportSize.height +
        ') after scrolling.');
  }

  return inViewLocation;
};


/**
 * Scrolls the scrollable element so that the region is fully visible.
 * If the region is too large, it will be aligned to the top-left of the
 * scrollable element. The region should be relative to the scrollable
 * element's current scroll position.
 *
 * @param {!goog.math.Rect} region The region to use.
 * @param {!Element} scrollable The scrollable element to scroll.
 * @private
 */
bot.dom.scrollRegionIntoView_ = function(region, scrollable) {
  scrollable.scrollLeft += Math.min(
      region.left, Math.max(region.left - region.width, 0));
  scrollable.scrollTop += Math.min(
      region.top, Math.max(region.top - region.height, 0));
};


/**
 * Scrolls the region of an element into the container's view. If the
 * region is too large to fit in the view, it will be aligned to the
 * top-left of the container.
 *
 * The element and container should be attached to the current document.
 *
 * @param {!Element} elem The element to use.
 * @param {!goog.math.Rect} elemRegion The region relative to the element to be
 *     scrolled into view.
 * @param {!Element} container A container of the given element.
 * @private
 */
bot.dom.scrollElementRegionIntoContainerView_ = function(elem, elemRegion,
                                                         container) {
  // Based largely from goog.style.scrollIntoContainerView.
  var elemPos = goog.style.getPageOffset(elem);
  var containerPos = goog.style.getPageOffset(container);
  var containerBorder = goog.style.getBorderBox(container);

  // Relative pos. of the element's border box to the container's content box.
  var relX = elemPos.x + elemRegion.left - containerPos.x -
             containerBorder.left;
  var relY = elemPos.y + elemRegion.top - containerPos.y - containerBorder.top;

  // How much the element can move in the container.
  var spaceX = container.clientWidth - elemRegion.width;
  var spaceY = container.clientHeight - elemRegion.height;

  bot.dom.scrollRegionIntoView_(new goog.math.Rect(relX, relY, spaceX, spaceY),
                                container);
};


/**
 * Scrolls the element into the client's view. If the element or region is
 * too large to fit in the view, it will be aligned to the top-left of the
 * container.
 *
 * The element should be attached to the current document.
 *
 * @param {!Element} elem The element to use.
 * @param {!goog.math.Rect} elemRegion The region relative to the element to be
 *     scrolled into view.
 */
bot.dom.scrollElementRegionIntoClientView = function(elem, elemRegion) {
  var doc = goog.dom.getOwnerDocument(elem);

  // Scroll the containers.
  for (var container = bot.dom.getParentElement(elem);
       container && container != doc.body && container != doc.documentElement;
       container = bot.dom.getParentElement(container)) {
    bot.dom.scrollElementRegionIntoContainerView_(elem, elemRegion, container);
  }

  // Scroll the actual window.
  var elemPageOffset = goog.style.getPageOffset(elem);

  var viewportSize = goog.dom.getDomHelper(doc).getViewportSize();

  var region = new goog.math.Rect(
      elemPageOffset.x + elemRegion.left - (doc.body ? doc.body.scrollLeft : 0),
      elemPageOffset.y + elemRegion.top - (doc.body ? doc.body.scrollTop : 0),
      viewportSize.width - elemRegion.width,
      viewportSize.height - elemRegion.height);

  bot.dom.scrollRegionIntoView_(region, doc.body || doc.documentElement);
};


/**
 * Scrolls the element into the client's view and returns its position
 * relative to the client viewport. If the element or region is too
 * large to fit in the view, it will be aligned to the top-left of the
 * container.
 *
 * The element should be attached to the current document.
 *
 * @param {!Element} elem The element to use.
 * @param {!goog.math.Rect=} opt_elemRegion The region relative to the element
 *     to be scrolled into view.
 * @return {!goog.math.Coordinate} The coordinate of the element in client
 *     space.
 */
bot.dom.getLocationInView = function(elem, opt_elemRegion) {
  var elemRegion;
  if (opt_elemRegion) {
    elemRegion = new goog.math.Rect(
        opt_elemRegion.left, opt_elemRegion.top,
        opt_elemRegion.width, opt_elemRegion.height);
  } else {
    elemRegion = new goog.math.Rect(0, 0, elem.offsetWidth, elem.offsetHeight);
  }
  bot.dom.scrollElementRegionIntoClientView(elem, elemRegion);

  // This is needed for elements that are split across multiple lines.
  var rect = elem.getClientRects ? elem.getClientRects()[0] : null;
  var elemClientPos = rect ?
      new goog.math.Coordinate(rect.left, rect.top) :
      goog.style.getClientPosition(elem);
  return new goog.math.Coordinate(elemClientPos.x + elemRegion.left,
                                  elemClientPos.y + elemRegion.top);
};


/**
 * Checks whether the element is currently scrolled in to view, such that the
 * offset given, relative to the top-left corner of the element, is currently
 * displayed in the viewport.
 *
 * @param {!Element} element The element to check.
 * @param {!goog.math.Coordinate=} opt_coords Coordinate in the element,
 *     relative to the top-left corner of the element, to check. If none are
 *     specified, checks that any part of the element is in view.
 * @return {boolean} Whether the coordinates specified, relative to the element,
 *     are scrolled in to view.
 */
bot.dom.isScrolledIntoView = function(element, opt_coords) {
  var ownerWindow = goog.dom.getWindow(goog.dom.getOwnerDocument(element));
  var topWindow = ownerWindow.top;
  var elSize = goog.style.getSize(element);

  for (var win = ownerWindow;; win = win.parent) {
    var scroll = goog.dom.getDomHelper(win.document).getDocumentScroll();
    var size = goog.dom.getViewportSize(win);
    var viewportRect = new goog.math.Rect(scroll.x,
                                          scroll.y,
                                          size.width,
                                          size.height);

    var elCoords = goog.style.getFramedPageOffset(element, win);
    var elementRect = new goog.math.Rect(elCoords.x,
                                         elCoords.y,
                                         elSize.width,
                                         elSize.height);
    if (!goog.math.Rect.intersects(viewportRect, elementRect)) {
      return false;
    }
    if (win == topWindow) {
      break;
    }
  }

  var visibleBox = goog.style.getVisibleRectForElement(element);
  if (!visibleBox) {
    return false;
  }
  if (opt_coords) {
    var elementOffset = goog.style.getPageOffset(element);
    var desiredPoint = goog.math.Coordinate.sum(elementOffset, opt_coords);
    return visibleBox.contains(desiredPoint);
  } else {
    var elementBox = goog.style.getBounds(element).toBox();
    return goog.math.Box.intersects(visibleBox, elementBox);
  }
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utilities for working with errors as defined by WebDriver's
 * wire protocol: http://code.google.com/p/selenium/wiki/JsonWireProtocol.
 */

goog.provide('bot.Error');
goog.provide('bot.ErrorCode');


/**
 * Error codes from the WebDriver wire protocol:
 * http://code.google.com/p/selenium/wiki/JsonWireProtocol#Response_Status_Codes
 *
 * @enum {number}
 */
bot.ErrorCode = {
  SUCCESS: 0,  // Included for completeness

  NO_SUCH_ELEMENT: 7,
  NO_SUCH_FRAME: 8,
  UNKNOWN_COMMAND: 9,
  UNSUPPORTED_OPERATION: 9,  // Alias.
  STALE_ELEMENT_REFERENCE: 10,
  ELEMENT_NOT_VISIBLE: 11,
  INVALID_ELEMENT_STATE: 12,
  UNKNOWN_ERROR: 13,
  ELEMENT_NOT_SELECTABLE: 15,
  JAVASCRIPT_ERROR: 17,
  XPATH_LOOKUP_ERROR: 19,
  TIMEOUT: 21,
  NO_SUCH_WINDOW: 23,
  INVALID_COOKIE_DOMAIN: 24,
  UNABLE_TO_SET_COOKIE: 25,
  MODAL_DIALOG_OPENED: 26,
  NO_MODAL_DIALOG_OPEN: 27,
  SCRIPT_TIMEOUT: 28,
  INVALID_ELEMENT_COORDINATES: 29,
  IME_NOT_AVAILABLE: 30,
  IME_ENGINE_ACTIVATION_FAILED: 31,
  INVALID_SELECTOR_ERROR: 32,
  SESSION_NOT_CREATED: 33,
  MOVE_TARGET_OUT_OF_BOUNDS: 34,
  SQL_DATABASE_ERROR: 35,
  INVALID_XPATH_SELECTOR: 51,
  INVALID_XPATH_SELECTOR_RETURN_TYPE: 52,
  // The following error codes are derived straight from HTTP return codes.
  METHOD_NOT_ALLOWED: 405
};



/**
 * Error extension that includes error status codes from the WebDriver wire
 * protocol:
 * http://code.google.com/p/selenium/wiki/JsonWireProtocol#Response_Status_Codes
 *
 * @param {!bot.ErrorCode} code The error's status code.
 * @param {string=} opt_message Optional error message.
 * @constructor
 * @extends {Error}
 */
bot.Error = function(code, opt_message) {

  /**
   * This error's status code.
   * @type {!bot.ErrorCode}
   */
  this.code = code;

  /** @type {string} */
  this.state =
      bot.Error.CODE_TO_STATE_[code] || bot.Error.State.UNKNOWN_ERROR;

  /** @override */
  this.message = opt_message || '';

  var name = this.state.replace(/((?:^|\s+)[a-z])/g, function(str) {
    // IE<9 does not support String#trim(). Also, IE does not include 0xa0
    // (the non-breaking-space) in the \s character class, so we have to
    // explicitly include it.
    return str.toUpperCase().replace(/^[\s\xa0]+/g, '');
  });
  var l = name.length - 'Error'.length;
  if (l < 0 || name.indexOf('Error', l) != l) {
    name += 'Error';
  }

  /** @override */
  this.name = name;

  // Generate a stacktrace for our custom error; ensure the error has our
  // custom name and message so the stack prints correctly in all browsers.
  var template = new Error(this.message);
  template.name = this.name;

  /** @override */
  this.stack = template.stack || '';
};
goog.inherits(bot.Error, Error);


/**
 * Status strings enumerated in the W3C WebDriver working draft.
 * @enum {string}
 * @see http://www.w3.org/TR/webdriver/#status-codes
 */
bot.Error.State = {
  ELEMENT_NOT_SELECTABLE: 'element not selectable',
  ELEMENT_NOT_VISIBLE: 'element not visible',
  IME_ENGINE_ACTIVATION_FAILED: 'ime engine activation failed',
  IME_NOT_AVAILABLE: 'ime not available',
  INVALID_COOKIE_DOMAIN: 'invalid cookie domain',
  INVALID_ELEMENT_COORDINATES: 'invalid element coordinates',
  INVALID_ELEMENT_STATE: 'invalid element state',
  INVALID_SELECTOR: 'invalid selector',
  JAVASCRIPT_ERROR: 'javascript error',
  MOVE_TARGET_OUT_OF_BOUNDS: 'move target out of bounds',
  NO_SUCH_ALERT: 'no such alert',
  NO_SUCH_DOM: 'no such dom',
  NO_SUCH_ELEMENT: 'no such element',
  NO_SUCH_FRAME: 'no such frame',
  NO_SUCH_WINDOW: 'no such window',
  SCRIPT_TIMEOUT: 'script timeout',
  SESSION_NOT_CREATED: 'session not created',
  STALE_ELEMENT_REFERENCE: 'stale element reference',
  SUCCESS: 'success',
  TIMEOUT: 'timeout',
  UNABLE_TO_SET_COOKIE: 'unable to set cookie',
  UNEXPECTED_ALERT_OPEN: 'unexpected alert open',
  UNKNOWN_COMMAND: 'unknown command',
  UNKNOWN_ERROR: 'unknown error',
  UNSUPPORTED_OPERATION: 'unsupported operation'
};


/**
 * A map of error codes to state string.
 * @private {!Object.<bot.ErrorCode, bot.Error.State>}
 */
bot.Error.CODE_TO_STATE_ = {};
goog.scope(function() {
  var map = bot.Error.CODE_TO_STATE_;
  var code = bot.ErrorCode;
  var state = bot.Error.State;

  map[code.ELEMENT_NOT_SELECTABLE] = state.ELEMENT_NOT_SELECTABLE;
  map[code.ELEMENT_NOT_VISIBLE] = state.ELEMENT_NOT_VISIBLE;
  map[code.IME_ENGINE_ACTIVATION_FAILED] = state.IME_ENGINE_ACTIVATION_FAILED;
  map[code.IME_NOT_AVAILABLE] = state.IME_NOT_AVAILABLE;
  map[code.INVALID_COOKIE_DOMAIN] = state.INVALID_COOKIE_DOMAIN;
  map[code.INVALID_ELEMENT_COORDINATES] = state.INVALID_ELEMENT_COORDINATES;
  map[code.INVALID_ELEMENT_STATE] = state.INVALID_ELEMENT_STATE;
  map[code.INVALID_SELECTOR_ERROR] = state.INVALID_SELECTOR;
  map[code.INVALID_XPATH_SELECTOR] = state.INVALID_SELECTOR;
  map[code.INVALID_XPATH_SELECTOR_RETURN_TYPE] = state.INVALID_SELECTOR;
  map[code.JAVASCRIPT_ERROR] = state.JAVASCRIPT_ERROR;
  map[code.METHOD_NOT_ALLOWED] = state.UNSUPPORTED_OPERATION;
  map[code.MOVE_TARGET_OUT_OF_BOUNDS] = state.MOVE_TARGET_OUT_OF_BOUNDS;
  map[code.NO_MODAL_DIALOG_OPEN] = state.NO_SUCH_ALERT;
  map[code.NO_SUCH_ELEMENT] = state.NO_SUCH_ELEMENT;
  map[code.NO_SUCH_FRAME] = state.NO_SUCH_FRAME;
  map[code.NO_SUCH_WINDOW] = state.NO_SUCH_WINDOW;
  map[code.SCRIPT_TIMEOUT] = state.SCRIPT_TIMEOUT;
  map[code.SESSION_NOT_CREATED] = state.SESSION_NOT_CREATED;
  map[code.STALE_ELEMENT_REFERENCE] = state.STALE_ELEMENT_REFERENCE;
  map[code.SUCCESS] = state.SUCCESS;
  map[code.TIMEOUT] = state.TIMEOUT;
  map[code.UNABLE_TO_SET_COOKIE] = state.UNABLE_TO_SET_COOKIE;
  map[code.MODAL_DIALOG_OPENED] = state.UNEXPECTED_ALERT_OPEN;
  map[code.UNKNOWN_ERROR] = state.UNKNOWN_ERROR;
  map[code.UNSUPPORTED_OPERATION] = state.UNKNOWN_COMMAND;
});  // goog.scope


/**
 * Flag used for duck-typing when this code is embedded in a Firefox extension.
 * This is required since an Error thrown in one component and then reported
 * to another will fail instanceof checks in the second component.
 * @type {boolean}
 */
bot.Error.prototype.isAutomationError = true;


if (goog.DEBUG) {
  /** @return {string} The string representation of this error. */
  bot.Error.prototype.toString = function() {
    return this.name + ': ' + this.message;
  };
}
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Functions to do with firing and simulating events.
 */


goog.provide('bot.events');
goog.provide('bot.events.EventArgs');
goog.provide('bot.events.EventType');
goog.provide('bot.events.KeyboardArgs');
goog.provide('bot.events.MSGestureArgs');
goog.provide('bot.events.MSPointerArgs');
goog.provide('bot.events.MouseArgs');
goog.provide('bot.events.Touch');
goog.provide('bot.events.TouchArgs');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.userAgent');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.style');
goog.require('goog.userAgent');
goog.require('goog.userAgent.product');


/**
 * Whether the browser supports the construction of touch events.
 *
 * @const
 * @type {boolean}
 */
bot.events.SUPPORTS_TOUCH_EVENTS = !(goog.userAgent.IE &&
                                     !bot.userAgent.isEngineVersion(10)) &&
                                   !goog.userAgent.OPERA;


/**
 * Whether the browser supports a native touch api.
 *
 * @const
 * @private {boolean}
 */
bot.events.BROKEN_TOUCH_API_ = (function() {
  if (goog.userAgent.product.ANDROID) {
    // Native touch api supported starting in version 4.0 (Ice Cream Sandwich).
    return !bot.userAgent.isProductVersion(4);
  }
  return !bot.userAgent.IOS;
})();


/**
 * Whether the browser supports the construction of MSPointer events.
 *
 * @const
 * @type {boolean}
 */
bot.events.SUPPORTS_MSPOINTER_EVENTS =
    goog.userAgent.IE && bot.getWindow().navigator.msPointerEnabled;


/**
 * Arguments to initialize an event.
 *
 * @typedef {bot.events.MouseArgs|bot.events.KeyboardArgs|bot.events.TouchArgs|
             bot.events.MSGestureArgs|bot.events.MSPointerArgs}
 */
bot.events.EventArgs;


/**
 * Arguments to initialize a mouse event.
 *
 * @typedef {{clientX: number,
 *            clientY: number,
 *            button: number,
 *            altKey: boolean,
 *            ctrlKey: boolean,
 *            shiftKey: boolean,
 *            metaKey: boolean,
 *            relatedTarget: Element,
 *            wheelDelta: number}}
 */
bot.events.MouseArgs;


/**
 * Arguments to initialize a keyboard event.
 *
 * @typedef {{keyCode: number,
 *            charCode: number,
 *            altKey: boolean,
 *            ctrlKey: boolean,
 *            shiftKey: boolean,
 *            metaKey: boolean,
 *            preventDefault: boolean}}
 */
bot.events.KeyboardArgs;


/**
 * Argument to initialize a touch event.
 *
 * @typedef {{touches: !Array.<bot.events.Touch>,
 *            targetTouches: !Array.<bot.events.Touch>,
 *            changedTouches: !Array.<bot.events.Touch>,
 *            altKey: boolean,
 *            ctrlKey: boolean,
 *            shiftKey: boolean,
 *            metaKey: boolean,
 *            relatedTarget: Element,
 *            scale: number,
 *            rotation: number}}
 */
bot.events.TouchArgs;


/**
 * @typedef {{identifier: number,
 *            screenX: number,
 *            screenY: number,
 *            clientX: number,
 *            clientY: number,
 *            pageX: number,
 *            pageY: number}}
 */
bot.events.Touch;


/**
 * Arguments to initialize an MSGesture event.
 *
 * @typedef {{clientX: number,
 *            clientY: number,
 *            translationX: number,
 *            translationY: number,
 *            scale: number,
 *            expansion: number,
 *            rotation: number,
 *            velocityX: number,
 *            velocityY: number,
 *            velocityExpansion: number,
 *            velocityAngular: number,
 *            relatedTarget: Element}}
 */
bot.events.MSGestureArgs;


/**
 * Arguments to initialize an MSPointer event.
 *
 * @typedef {{clientX: number,
 *            clientY: number,
 *            button: number,
 *            altKey: boolean,
 *            ctrlKey: boolean,
 *            shiftKey: boolean,
 *            metaKey: boolean,
 *            relatedTarget: Element,
 *            width: number,
 *            height: number,
 *            pressure: number,
 *            rotation: number,
 *            pointerId: number,
 *            tiltX: number,
 *            tiltY: number,
 *            pointerType: number,
 *            isPrimary: boolean}}
 */
bot.events.MSPointerArgs;



/**
 * Factory for event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @private
 */
bot.events.EventFactory_ = function(type, bubbles, cancelable) {
  /** @private {string} */
  this.type_ = type;

  /** @private {boolean} */
  this.bubbles_ = bubbles;

  /** @private {boolean} */
  this.cancelable_ = cancelable;
};


/**
 * Creates an event.
 *
 * @param {!Element} target Target element of the event.
 * @param {bot.events.EventArgs=} opt_args Event arguments.
 * @return {!Event} Newly created event.
 */
bot.events.EventFactory_.prototype.create = function(target, opt_args) {
  var doc = goog.dom.getOwnerDocument(target);
  var event;

  if (bot.userAgent.IE_DOC_PRE9) {
    event = doc.createEventObject();
  } else {
    event = doc.createEvent('HTMLEvents');
    event.initEvent(this.type_, this.bubbles_, this.cancelable_);
  }

  return event;
};


/**
 * Overriding toString to return the unique type string improves debugging,
 * and it allows event types to be mapped in JS objects without collisions.
 *
 * @return {string} String representation of the event type.
 */
bot.events.EventFactory_.prototype.toString = function() {
  return this.type_;
};



/**
 * Factory for mouse event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @extends {bot.events.EventFactory_}
 * @private
 */
bot.events.MouseEventFactory_ = function(type, bubbles, cancelable) {
  goog.base(this, type, bubbles, cancelable);
};
goog.inherits(bot.events.MouseEventFactory_, bot.events.EventFactory_);


/**
 * @override
 */
bot.events.MouseEventFactory_.prototype.create = function(target, opt_args) {
  // Only Gecko supports the mouse pixel scroll event.
  if (!goog.userAgent.GECKO && this == bot.events.EventType.MOUSEPIXELSCROLL) {
    throw new bot.Error(bot.ErrorCode.UNSUPPORTED_OPERATION,
        'Browser does not support a mouse pixel scroll event.');
  }

  var args = (/** @type {!bot.events.MouseArgs} */ opt_args);
  var doc = goog.dom.getOwnerDocument(target);
  var event;

  if (bot.userAgent.IE_DOC_PRE9) {
    event = doc.createEventObject();
    event.altKey = args.altKey;
    event.ctrlKey = args.ctrlKey;
    event.metaKey = args.metaKey;
    event.shiftKey = args.shiftKey;
    event.button = args.button;

    // NOTE: ie8 does a strange thing with the coordinates passed in the event:
    // - if offset{X,Y} coordinates are specified, they are also used for
    //   client{X,Y}, event if client{X,Y} are also specified.
    // - if only client{X,Y} are specified, they are also used for offset{x,y}
    // Thus, for ie8, it is impossible to set both offset and client
    // and have them be correct when they come out on the other side.
    event.clientX = args.clientX;
    event.clientY = args.clientY;

    // Sets a property of the event object using Object.defineProperty.
    // Some readonly properties of the IE event object can only be set this way.
    var setEventProperty = function(prop, value) {
      Object.defineProperty(event, prop, {
        get: function() {
          return value;
        }
      });
    };

    // IE has fromElement and toElement properties, no relatedTarget property.
    // IE does not allow fromElement and toElement to be set directly, but
    // Object.defineProperty can redefine them, when it is available. Do not
    // use Object.defineProperties (plural) because it is even less supported.
    // If defineProperty is unavailable, fall back to setting the relatedTarget,
    // which many event frameworks, including jQuery and Closure, forgivingly
    // pass on as the relatedTarget on their event object abstraction.
    if (this == bot.events.EventType.MOUSEOUT ||
        this == bot.events.EventType.MOUSEOVER) {
      if (Object.defineProperty) {
        var out = (this == bot.events.EventType.MOUSEOUT);
        setEventProperty('fromElement', out ? target : args.relatedTarget);
        setEventProperty('toElement', out ? args.relatedTarget : target);
      } else {
        event.relatedTarget = args.relatedTarget;
      }
    }

    // IE does not allow the wheelDelta property to be set directly, so we can
    // only do it where defineProperty is supported; otherwise store the wheel
    // delta in the event "detail" as a last resort in case the app looks there.
    if (this == bot.events.EventType.MOUSEWHEEL) {
      if (Object.defineProperty) {
        setEventProperty('wheelDelta', args.wheelDelta);
      } else {
        event.detail = args.wheelDelta;
      }
    }
  } else {
    var view = goog.dom.getWindow(doc);
    event = doc.createEvent('MouseEvents');
    var detail = 1;

    // All browser but Firefox provide the wheelDelta value in the event.
    // Firefox provides the scroll amount in the detail field, where it has the
    // opposite polarity of the wheelDelta (upward scroll is negative) and is a
    // factor of 40 less than the wheelDelta value. Opera provides both values.
    // The wheelDelta value is normally some multiple of 40.
    if (this == bot.events.EventType.MOUSEWHEEL) {
      if (!goog.userAgent.GECKO) {
        event.wheelDelta = args.wheelDelta;
      }
      if (goog.userAgent.GECKO || goog.userAgent.OPERA) {
        detail = args.wheelDelta / -40;
      }
    }

    // Only Gecko supports a mouse pixel scroll event, so we use it as the
    // "standard" and pass it along as is as the "detail" of the event.
    if (goog.userAgent.GECKO && this == bot.events.EventType.MOUSEPIXELSCROLL) {
      detail = args.wheelDelta;
    }

    event.initMouseEvent(this.type_, this.bubbles_, this.cancelable_, view,
        detail, /*screenX*/ 0, /*screenY*/ 0, args.clientX, args.clientY,
        args.ctrlKey, args.altKey, args.shiftKey, args.metaKey, args.button,
        args.relatedTarget);

    // Trying to modify the properties throws an error,
    // so we define getters to return the correct values.
    if (goog.userAgent.IE &&
        event.pageX === 0 && event.pageY === 0 && Object.defineProperty) {
      var scrollElem = goog.dom.getDomHelper(target).getDocumentScrollElement();
      var clientElem = goog.style.getClientViewportElement(target);
      var pageX = args.clientX + scrollElem.scrollLeft - clientElem.clientLeft;
      var pageY = args.clientY + scrollElem.scrollTop - clientElem.clientTop;

      Object.defineProperty(event, 'pageX', {
        get: function() {
          return pageX;
        }
      });
      Object.defineProperty(event, 'pageY', {
        get: function() {
          return pageY;
        }
      });
    }
  }

  return event;
};



/**
 * Factory for keyboard event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @extends {bot.events.EventFactory_}
 * @private
 */
bot.events.KeyboardEventFactory_ = function(type, bubbles, cancelable) {
  goog.base(this, type, bubbles, cancelable);
};
goog.inherits(bot.events.KeyboardEventFactory_, bot.events.EventFactory_);


/**
 * @override
 */
bot.events.KeyboardEventFactory_.prototype.create = function(target, opt_args) {
  var args = (/** @type {!bot.events.KeyboardArgs} */ opt_args);
  var doc = goog.dom.getOwnerDocument(target);
  var event;

  if (goog.userAgent.GECKO) {
    var view = goog.dom.getWindow(doc);
    var keyCode = args.charCode ? 0 : args.keyCode;
    event = doc.createEvent('KeyboardEvent');
    event.initKeyEvent(this.type_, this.bubbles_, this.cancelable_, view,
        args.ctrlKey, args.altKey, args.shiftKey, args.metaKey, keyCode,
        args.charCode);
    // https://bugzilla.mozilla.org/show_bug.cgi?id=501496
    if (this.type_ == bot.events.EventType.KEYPRESS && args.preventDefault) {
      event.preventDefault();
    }
  } else {
    if (bot.userAgent.IE_DOC_PRE9) {
      event = doc.createEventObject();
    } else {  // WebKit, Opera, and IE 9+ in Standards mode.
      event = doc.createEvent('Events');
      event.initEvent(this.type_, this.bubbles_, this.cancelable_);
    }
    event.altKey = args.altKey;
    event.ctrlKey = args.ctrlKey;
    event.metaKey = args.metaKey;
    event.shiftKey = args.shiftKey;
    event.keyCode = args.charCode || args.keyCode;
    if (goog.userAgent.WEBKIT) {
      event.charCode = (this == bot.events.EventType.KEYPRESS) ?
          event.keyCode : 0;
    }
  }

  return event;
};



/**
 * Factory for touch event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @extends {bot.events.EventFactory_}
 * @private
 */
bot.events.TouchEventFactory_ = function(type, bubbles, cancelable) {
  goog.base(this, type, bubbles, cancelable);
};
goog.inherits(bot.events.TouchEventFactory_, bot.events.EventFactory_);


/**
 * @override
 */
bot.events.TouchEventFactory_.prototype.create = function(target, opt_args) {
  if (!bot.events.SUPPORTS_TOUCH_EVENTS) {
    throw new bot.Error(bot.ErrorCode.UNSUPPORTED_OPERATION,
        'Browser does not support firing touch events.');
  }

  var args = (/** @type {!bot.events.TouchArgs} */ opt_args);
  var doc = goog.dom.getOwnerDocument(target);
  var view = goog.dom.getWindow(doc);

  // Creates a TouchList, using native touch Api, for touch events.
  function createNativeTouchList(touchListArgs) {
    var touches = goog.array.map(touchListArgs, function(touchArg) {
      return doc.createTouch(view, target, touchArg.identifier,
          touchArg.pageX, touchArg.pageY, touchArg.screenX, touchArg.screenY);
    });

    return doc.createTouchList.apply(doc, touches);
  }

  // Creates a TouchList, using simulated touch Api, for touch events.
  function createGenericTouchList(touchListArgs) {
    var touches = goog.array.map(touchListArgs, function(touchArg) {
      // The target field is not part of the W3C spec, but both android and iOS
      // add the target field to each touch.
      return {
        identifier: touchArg.identifier,
        screenX: touchArg.screenX,
        screenY: touchArg.screenY,
        clientX: touchArg.clientX,
        clientY: touchArg.clientY,
        pageX: touchArg.pageX,
        pageY: touchArg.pageY,
        target: target
      };
    });
    touches.item = function(i) {
      return touches[i];
    };
    return touches;
  }

  function createTouchList(touches) {
    return bot.events.BROKEN_TOUCH_API_ ?
        createGenericTouchList(touches) :
        createNativeTouchList(touches);
  }

  // As a performance optimization, reuse the created touchlist when the lists
  // are the same, which is often the case in practice.
  var changedTouches = createTouchList(args.changedTouches);
  var touches = (args.touches == args.changedTouches) ?
      changedTouches : createTouchList(args.touches);
  var targetTouches = (args.targetTouches == args.changedTouches) ?
      changedTouches : createTouchList(args.targetTouches);

  var event;
  if (bot.events.BROKEN_TOUCH_API_) {
    event = doc.createEvent('MouseEvents');
    event.initMouseEvent(this.type_, this.bubbles_, this.cancelable_, view,
        /*detail*/ 1, /*screenX*/ 0, /*screenY*/ 0, args.clientX, args.clientY,
        args.ctrlKey, args.altKey, args.shiftKey, args.metaKey, /*button*/ 0,
        args.relatedTarget);
    event.touches = touches;
    event.targetTouches = targetTouches;
    event.changedTouches = changedTouches;
    event.scale = args.scale;
    event.rotation = args.rotation;
  } else {
    event = doc.createEvent('TouchEvent');
    if (goog.userAgent.product.ANDROID) {
      // Android's initTouchEvent method is not compliant with the W3C spec.
      event.initTouchEvent(touches, targetTouches, changedTouches,
          this.type_, view, /*screenX*/ 0, /*screenY*/ 0, args.clientX,
          args.clientY, args.ctrlKey, args.altKey, args.shiftKey, args.metaKey);
    } else {
      event.initTouchEvent(this.type_, this.bubbles_, this.cancelable_, view,
          /*detail*/ 1, /*screenX*/ 0, /*screenY*/ 0, args.clientX,
          args.clientY, args.ctrlKey, args.altKey, args.shiftKey, args.metaKey,
          touches, targetTouches, changedTouches, args.scale, args.rotation);
    }
    event.relatedTarget = args.relatedTarget;
  }

  return event;
};



/**
 * Factory for MSGesture event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @extends {bot.events.EventFactory_}
 * @private
 */
bot.events.MSGestureEventFactory_ = function(type, bubbles, cancelable) {
  goog.base(this, type, bubbles, cancelable);
};
goog.inherits(bot.events.MSGestureEventFactory_, bot.events.EventFactory_);


/**
 * @override
 */
bot.events.MSGestureEventFactory_.prototype.create = function(target,
                                                              opt_args) {
  if (!bot.events.SUPPORTS_MSPOINTER_EVENTS) {
    throw new bot.Error(bot.ErrorCode.UNSUPPORTED_OPERATION,
        'Browser does not support MSGesture events.');
  }

  var args = (/** @type {!bot.events.MSGestureArgs} */ opt_args);
  var doc = goog.dom.getOwnerDocument(target);
  var view = goog.dom.getWindow(doc);
  var event = doc.createEvent('MSGestureEvent');
  var timestamp = (new Date).getTime();

  // See http://msdn.microsoft.com/en-us/library/windows/apps/hh441187.aspx
  event.initGestureEvent(this.type_, this.bubbles_, this.cancelable_, view,
                         /*detail*/ 1, /*screenX*/ 0, /*screenY*/ 0,
                         args.clientX, args.clientY, /*offsetX*/ 0,
                         /*offsetY*/ 0, args.translationX, args.translationY,
                         args.scale, args.expansion, args.rotation,
                         args.velocityX, args.velocityY, args.velocityExpansion,
                         args.velocityAngular, timestamp, args.relatedTarget);
  return event;
};



/**
 * Factory for MSPointer event objects of a specific type.
 *
 * @constructor
 * @param {string} type Type of the created events.
 * @param {boolean} bubbles Whether the created events bubble.
 * @param {boolean} cancelable Whether the created events are cancelable.
 * @extends {bot.events.EventFactory_}
 * @private
 */
bot.events.MSPointerEventFactory_ = function(type, bubbles, cancelable) {
  goog.base(this, type, bubbles, cancelable);
};
goog.inherits(bot.events.MSPointerEventFactory_, bot.events.EventFactory_);


/**
 * @override
 * @suppress {checkTypes} Closure compiler externs don't know about pointer
 *     events
 */
bot.events.MSPointerEventFactory_.prototype.create = function(target,
                                                              opt_args) {
  if (!bot.events.SUPPORTS_MSPOINTER_EVENTS) {
    throw new bot.Error(bot.ErrorCode.UNSUPPORTED_OPERATION,
        'Browser does not support MSPointer events.');
  }

  var args = (/** @type {!bot.events.MSPointerArgs} */ opt_args);
  var doc = goog.dom.getOwnerDocument(target);
  var view = goog.dom.getWindow(doc);
  var event = doc.createEvent('MSPointerEvent');

  // See http://msdn.microsoft.com/en-us/library/ie/hh772109(v=vs.85).aspx
  event.initPointerEvent(this.type_, this.bubbles_, this.cancelable_, view,
                         /*detail*/ 0, /*screenX*/ 0, /*screenY*/ 0,
                         args.clientX, args.clientY, args.ctrlKey, args.altKey,
                         args.shiftKey, args.metaKey, args.button,
                         args.relatedTarget, /*offsetX*/ 0, /*offsetY*/ 0,
                         args.width, args.height, args.pressure, args.rotation,
                         args.tiltX, args.tiltY, args.pointerId,
                         args.pointerType, /*hwTimeStamp*/ 0, args.isPrimary);

  return event;
};


/**
 * The types of events this modules supports firing.
 *
 * <p>To see which events bubble and are cancelable, see:
 * http://en.wikipedia.org/wiki/DOM_events
 *
 * @enum {!Object}
 */
bot.events.EventType = {
  BLUR: new bot.events.EventFactory_('blur', false, false),
  CHANGE: new bot.events.EventFactory_('change', true, false),
  FOCUS: new bot.events.EventFactory_('focus', false, false),
  FOCUSIN: new bot.events.EventFactory_('focusin', true, false),
  FOCUSOUT: new bot.events.EventFactory_('focusout', true, false),
  INPUT: new bot.events.EventFactory_('input', false, false),
  PROPERTYCHANGE: new bot.events.EventFactory_('propertychange', false, false),
  SELECT: new bot.events.EventFactory_('select', true, false),
  SUBMIT: new bot.events.EventFactory_('submit', true, true),
  TEXTINPUT: new bot.events.EventFactory_('textInput', true, true),

  // Mouse events.
  CLICK: new bot.events.MouseEventFactory_('click', true, true),
  CONTEXTMENU: new bot.events.MouseEventFactory_('contextmenu', true, true),
  DBLCLICK: new bot.events.MouseEventFactory_('dblclick', true, true),
  MOUSEDOWN: new bot.events.MouseEventFactory_('mousedown', true, true),
  MOUSEMOVE: new bot.events.MouseEventFactory_('mousemove', true, false),
  MOUSEOUT: new bot.events.MouseEventFactory_('mouseout', true, true),
  MOUSEOVER: new bot.events.MouseEventFactory_('mouseover', true, true),
  MOUSEUP: new bot.events.MouseEventFactory_('mouseup', true, true),
  MOUSEWHEEL: new bot.events.MouseEventFactory_(
      goog.userAgent.GECKO ? 'DOMMouseScroll' : 'mousewheel', true, true),
  MOUSEPIXELSCROLL: new bot.events.MouseEventFactory_(
      'MozMousePixelScroll', true, true),

  // Keyboard events.
  KEYDOWN: new bot.events.KeyboardEventFactory_('keydown', true, true),
  KEYPRESS: new bot.events.KeyboardEventFactory_('keypress', true, true),
  KEYUP: new bot.events.KeyboardEventFactory_('keyup', true, true),

  // Touch events.
  TOUCHEND: new bot.events.TouchEventFactory_('touchend', true, true),
  TOUCHMOVE: new bot.events.TouchEventFactory_('touchmove', true, true),
  TOUCHSTART: new bot.events.TouchEventFactory_('touchstart', true, true),

  // MSGesture events
  MSGESTURECHANGE: new bot.events.MSGestureEventFactory_(
      'MSGestureChange', true, true),
  MSGESTUREEND: new bot.events.MSGestureEventFactory_(
      'MSGestureEnd', true, true),
  MSGESTUREHOLD: new bot.events.MSGestureEventFactory_(
      'MSGestureHold', true, true),
  MSGESTURESTART: new bot.events.MSGestureEventFactory_(
      'MSGestureStart', true, true),
  MSGESTURETAP: new bot.events.MSGestureEventFactory_(
      'MSGestureTap', true, true),
  MSINERTIASTART: new bot.events.MSGestureEventFactory_(
      'MSInertiaStart', true, true),

  // MSPointer events
  MSPOINTERDOWN: new bot.events.MSPointerEventFactory_(
      'MSPointerDown', true, true),
  MSPOINTERMOVE: new bot.events.MSPointerEventFactory_(
      'MSPointerMove', true, true),
  MSPOINTEROVER: new bot.events.MSPointerEventFactory_(
      'MSPointerOver', true, true),
  MSPOINTEROUT: new bot.events.MSPointerEventFactory_(
      'MSPointerOut', true, true),
  MSPOINTERUP: new bot.events.MSPointerEventFactory_(
      'MSPointerUp', true, true)
};


/**
 * Fire a named event on a particular element.
 *
 * @param {!Element} target The element on which to fire the event.
 * @param {!bot.events.EventType} type Event type.
 * @param {bot.events.EventArgs=} opt_args Arguments to initialize the event.
 * @return {boolean} Whether the event fired successfully or was cancelled.
 */
bot.events.fire = function(target, type, opt_args) {
  var factory = /** @type {!bot.events.EventFactory_} */ (type);
  var event = factory.create(target, opt_args);

  // Ensure the event's isTrusted property is set to false, so that
  // bot.events.isSynthetic() can identify synthetic events from native ones.
  if (!('isTrusted' in event)) {
    event['isTrusted'] = false;
  }

  if (bot.userAgent.IE_DOC_PRE9) {
    return target.fireEvent('on' + factory.type_, event);
  } else {
    return target.dispatchEvent(event);
  }
};


/**
 * Returns whether the event was synthetically created by the atoms;
 * if false, was created by the browser in response to a live user action.
 *
 * @param {!(Event|goog.events.BrowserEvent)} event An event.
 * @return {boolean} Whether the event was synthetically created.
 */
bot.events.isSynthetic = function(event) {
  var e = event.getBrowserEvent ? event.getBrowserEvent() : event;
  return 'isTrusted' in e ? !e['isTrusted'] : false;
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms for frame handling.
 *
 */


goog.provide('bot.frame');

goog.require('bot.locators');
goog.require('goog.dom');
goog.require('goog.dom.TagName');


/**
 * @return {!Window} The top window.
 */
bot.frame.defaultContent = function() {
  return bot.getWindow().top;
};


/**
 * @return {!Element} The currently active element.
 */
bot.frame.activeElement = function() {
  return document.activeElement || document.body;
};


/**
 * Returns a reference to the window object corresponding to the given element.
 * Note that the element must be a frame or an iframe.
 *
 * @param {!(HTMLIFrameElement|HTMLFrameElement)} element The iframe or frame
 *     element.
 * @return {Window} The window reference for the given iframe or frame element.
 */
bot.frame.getFrameWindow = function(element) {
  if (bot.frame.isFrame_(element)) {
    var frame = /** @type {HTMLFrameElement|HTMLIFrameElement} */ (element);
    return goog.dom.getFrameContentWindow(frame);
  }
  throw new bot.Error(bot.ErrorCode.NO_SUCH_FRAME,
      "The given element isn't a frame or an iframe.");
};


/**
 * Returns whether an element is a frame (or iframe).
 *
 * @param {!Element} element The element to check.
 * @return {boolean} Whether the element is a frame (or iframe).
 * @private
 */
bot.frame.isFrame_ = function(element) {
  return bot.dom.isElement(element, goog.dom.TagName.FRAME) ||
         bot.dom.isElement(element, goog.dom.TagName.IFRAME);
};


/**
 * Looks for a frame by its name or id (preferring name over id)
 * under the given root. If no frame was found, we look for an
 * iframe by name or id.
 *
 * @param {(string|number)} nameOrId The frame's name, the frame's id, or the
 *     index of the frame in the containing window.
 * @param {!Window=} opt_root The window to perform the search under.
 *     Defaults to {@code bot.getWindow()}.
 * @return {Window} The window if found, null otherwise.
 */
bot.frame.findFrameByNameOrId = function(nameOrId, opt_root) {
  var domWindow = opt_root || bot.getWindow();

  // Lookup frame by name
  var numFrames = domWindow.frames.length;
  for (var i = 0; i < numFrames; i++) {
    var frame = domWindow.frames[i];
    var frameElement = frame.frameElement || frame;
    if (frameElement.name == nameOrId) {
      // This is needed because Safari 4 returns
      // an HTMLFrameElement instead of a Window object.
      if (frame.document) {
        return frame;
      } else {
        return goog.dom.getFrameContentWindow(frame);
      }
    }
  }

  // Lookup frame by id
  var elements = bot.locators.findElements({id: nameOrId}, domWindow.document);
  for (var i = 0; i < elements.length; i++) {
    if (bot.frame.isFrame_(elements[i])) {
      return goog.dom.getFrameContentWindow(elements[i]);
    }
  }
  return null;
};


/**
 * Looks for a frame by its index under the given root.
 *
 * @param {number} index The frame's index.
 * @param {!Window=} opt_root The window to perform
 *     the search under. Defaults to {@code bot.getWindow()}.
 * @return {Window} The frame if found, null otherwise.
 */
bot.frame.findFrameByIndex = function(index, opt_root) {
  var domWindow = opt_root || bot.getWindow();
  return domWindow.frames[index] || null;
};


/**
 * Gets the index of a frame in the given window. Note that the element must
 * be a frame or an iframe.
 *
 * @param {!(HTMLIFrameElement|HTMLFrameElement)} element The iframe or frame
 *     element.
 * @param {!Window=} opt_root The window to perform the search under. Defaults
 *     to {@code bot.getWindow()}.
 * @return {?number} The index of the frame if found, null otherwise.
 */
bot.frame.getFrameIndex = function(element, opt_root) {
  try {
    var elementWindow = element.contentWindow;
  } catch (e) {
    // Happens in IE{7,8} if a frame doesn't have an enclosing frameset.
    return null;
  }

  if (!bot.frame.isFrame_(element)) {
    return null;
  }

  var domWindow = opt_root || bot.getWindow();
  for (var i = 0; i < domWindow.frames.length; i++) {
    if (elementWindow == domWindow.frames[i]) {
      return i;
    }
  }
  return null;
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Browser atom for injecting JavaScript into the page under
 * test. There is no point in using this atom directly from JavaScript.
 * Instead, it is intended to be used in its compiled form when injecting
 * script from another language (e.g. C++).
 *
 * TODO(jleyba): Add an example
 */

goog.provide('bot.inject');
goog.provide('bot.inject.cache');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.json');
/**
 * @suppress {extraRequire} Used as a forward declaration which causes
 * compilation errors if missing.
 */
goog.require('bot.response.ResponseObject');
goog.require('goog.array');
goog.require('goog.dom.NodeType');
goog.require('goog.object');


/**
 * Key used to identify DOM elements in the WebDriver wire protocol.
 * @type {string}
 * @const
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol
 */
bot.inject.ELEMENT_KEY = 'ELEMENT';


/**
 * Key used to identify Window objects in the WebDriver wire protocol.
 * @type {string}
 * @const
 */
bot.inject.WINDOW_KEY = 'WINDOW';


/**
 * Converts an element to a JSON friendly value so that it can be
 * stringified for transmission to the injector. Values are modified as
 * follows:
 * <ul>
 * <li>booleans, numbers, strings, and null are returned as is</li>
 * <li>undefined values are returned as null</li>
 * <li>functions are returned as a string</li>
 * <li>each element in an array is recursively processed</li>
 * <li>DOM Elements are wrapped in object-literals as dictated by the
 *     WebDriver wire protocol</li>
 * <li>all other objects will be treated as hash-maps, and will be
 *     recursively processed for any string and number key types (all
 *     other key types are discarded as they cannot be converted to JSON).
 * </ul>
 *
 * @param {*} value The value to make JSON friendly.
 * @return {*} The JSON friendly value.
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol
 */
bot.inject.wrapValue = function(value) {
  switch (goog.typeOf(value)) {
    case 'string':
    case 'number':
    case 'boolean':
      return value;

    case 'function':
      return value.toString();

    case 'array':
      return goog.array.map((/**@type {goog.array.ArrayLike}*/value),
          bot.inject.wrapValue);

    case 'object':
      // Since {*} expands to {Object|boolean|number|string|undefined}, the
      // JSCompiler complains that it is too broad a type for the remainder of
      // this block where {!Object} is expected. Downcast to prevent generating
      // a ton of compiler warnings.
      value = (/**@type {!Object}*/value);

      // Sniff out DOM elements. We're using duck-typing instead of an
      // instanceof check since the instanceof might not always work
      // (e.g. if the value originated from another Firefox component)
      if (goog.object.containsKey(value, 'nodeType') &&
          (value['nodeType'] == goog.dom.NodeType.ELEMENT ||
           value['nodeType'] == goog.dom.NodeType.DOCUMENT)) {
        var ret = {};
        ret[bot.inject.ELEMENT_KEY] =
            bot.inject.cache.addElement((/**@type {!Element}*/value));
        return ret;
      }

      // Check if this is a Window
      if (goog.object.containsKey(value, 'document')) {
        var ret = {};
        ret[bot.inject.WINDOW_KEY] =
            bot.inject.cache.addElement((/**@type{!Window}*/value));
        return ret;
      }

      if (goog.isArrayLike(value)) {
        return goog.array.map((/**@type {goog.array.ArrayLike}*/value),
            bot.inject.wrapValue);
      }

      var filtered = goog.object.filter(value, function(val, key) {
        return goog.isNumber(key) || goog.isString(key);
      });
      return goog.object.map(filtered, bot.inject.wrapValue);

    default:  // goog.typeOf(value) == 'undefined' || 'null'
      return null;
  }
};


/**
 * Unwraps any DOM element's encoded in the given {@code value}.
 * @param {*} value The value to unwrap.
 * @param {Document=} opt_doc The document whose cache to retrieve wrapped
 *     elements from. Defaults to the current document.
 * @return {*} The unwrapped value.
 * @private
 */
bot.inject.unwrapValue_ = function(value, opt_doc) {
  if (goog.isArray(value)) {
    return goog.array.map((/**@type {goog.array.ArrayLike}*/value),
        function(v) { return bot.inject.unwrapValue_(v, opt_doc); });
  } else if (goog.isObject(value)) {
    if (typeof value == 'function') {
      return value;
    }

    if (goog.object.containsKey(value, bot.inject.ELEMENT_KEY)) {
      return bot.inject.cache.getElement(value[bot.inject.ELEMENT_KEY],
          opt_doc);
    }

    if (goog.object.containsKey(value, bot.inject.WINDOW_KEY)) {
      return bot.inject.cache.getElement(value[bot.inject.WINDOW_KEY],
          opt_doc);
    }

    return goog.object.map(value, function(val) {
      return bot.inject.unwrapValue_(val, opt_doc);
    });
  }
  return value;
};


/**
 * Recompiles {@code fn} in the context of another window so that the
 * correct symbol table is used when the function is executed. This
 * function assumes the {@code fn} can be decompiled to its source using
 * {@code Function.prototype.toString} and that it only refers to symbols
 * defined in the target window's context.
 *
 * @param {!(Function|string)} fn Either the function that shold be
 *     recompiled, or a string defining the body of an anonymous function
 *     that should be compiled in the target window's context.
 * @param {!Window} theWindow The window to recompile the function in.
 * @return {!Function} The recompiled function.
 * @private
 */
bot.inject.recompileFunction_ = function(fn, theWindow) {
  if (goog.isString(fn)) {
    return new theWindow['Function'](fn);
  }
  return theWindow == window ? fn : new theWindow['Function'](
      'return (' + fn + ').apply(null,arguments);');
};


/**
 * Executes an injected script. This function should never be called from
 * within JavaScript itself. Instead, it is used from an external source that
 * is injecting a script for execution.
 *
 * <p/>For example, in a WebDriver Java test, one might have:
 * <pre><code>
 * Object result = ((JavascriptExecutor) driver).executeScript(
 *     "return arguments[0] + arguments[1];", 1, 2);
 * </code></pre>
 *
 * <p/>Once transmitted to the driver, this command would be injected into the
 * page for evaluation as:
 * <pre><code>
 * bot.inject.executeScript(
 *     function() {return arguments[0] + arguments[1];},
 *     [1, 2]);
 * </code></pre>
 *
 * <p/>The details of how this actually gets injected for evaluation is left
 * as an implementation detail for clients of this library.
 *
 * @param {!(Function|string)} fn Either the function to execute, or a string
 *     defining the body of an anonymous function that should be executed. This
 *     function should only contain references to symbols defined in the context
 *     of the current window.
 * @param {Array.<*>} args An array of wrapped script arguments, as defined by
 *     the WebDriver wire protocol.
 * @param {boolean=} opt_stringify Whether the result should be returned as a
 *     serialized JSON string.
 * @param {!Window=} opt_window The window in whose context the function should
 *     be invoked; defaults to the current window.
 * @return {!(string|bot.response.ResponseObject)} The response object. If
 *     opt_stringify is true, the result will be serialized and returned in
 *     string format.
 */
bot.inject.executeScript = function(fn, args, opt_stringify, opt_window) {
  var win = opt_window || bot.getWindow();
  var ret;
  try {
    fn = bot.inject.recompileFunction_(fn, win);
    var unwrappedArgs = (/**@type {Object}*/bot.inject.unwrapValue_(args,
        win.document));
    ret = bot.inject.wrapResponse(fn.apply(null, unwrappedArgs));
  } catch (ex) {
    ret = bot.inject.wrapError(ex);
  }
  return opt_stringify ? bot.json.stringify(ret) : ret;
};


/**
 * Executes an injected script, which is expected to finish asynchronously
 * before the given {@code timeout}. When the script finishes or an error
 * occurs, the given {@code onDone} callback will be invoked. This callback
 * will have a single argument, a {@link bot.response.ResponseObject} object.
 *
 * The script signals its completion by invoking a supplied callback given
 * as its last argument. The callback may be invoked with a single value.
 *
 * The script timeout event will be scheduled with the provided window,
 * ensuring the timeout is synchronized with that window's event queue.
 * Furthermore, asynchronous scripts do not work across new page loads; if an
 * "unload" event is fired on the window while an asynchronous script is
 * pending, the script will be aborted and an error will be returned.
 *
 * Like {@code bot.inject.executeScript}, this function should only be called
 * from an external source. It handles wrapping and unwrapping of input/output
 * values.
 *
 * @param {(function()|string)} fn Either the function to execute, or a string
 *     defining the body of an anonymous function that should be executed.
 * @param {Array.<*>} args An array of wrapped script arguments, as defined by
 *     the WebDriver wire protocol.
 * @param {number} timeout The amount of time, in milliseconds, the script
 *     should be permitted to run; must be non-negative.
 * @param {function(string)|function(!bot.response.ResponseObject)} onDone
 *     The function to call when the given {@code fn} invokes its callback,
 *     or when an exception or timeout occurs. This will always be called.
 * @param {boolean=} opt_stringify Whether the result should be returned as a
 *     serialized JSON string.
 * @param {!Window=} opt_window The window to synchronize the script with;
 *     defaults to the current window.
 */
bot.inject.executeAsyncScript = function(fn, args, timeout, onDone,
                                         opt_stringify, opt_window) {
  var win = opt_window || window;
  var timeoutId;
  var responseSent = false;

  function sendResponse(status, value) {
    if (!responseSent) {
      if (win.removeEventListener) {
        win.removeEventListener('unload', onunload, true);
      } else {
        win.detachEvent('onunload', onunload);
      }

      win.clearTimeout(timeoutId);
      if (status != bot.ErrorCode.SUCCESS) {
        var err = new bot.Error(status, value.message || value + '');
        err.stack = value.stack;
        value = bot.inject.wrapError(err);
      } else {
        value = bot.inject.wrapResponse(value);
      }
      onDone(opt_stringify ? bot.json.stringify(value) : value);
      responseSent = true;
    }
  }
  var sendError = goog.partial(sendResponse, bot.ErrorCode.UNKNOWN_ERROR);

  if (win.closed) {
    sendError('Unable to execute script; the target window is closed.');
    return;
  }

  fn = bot.inject.recompileFunction_(fn, win);

  args = /** @type {Array.<*>} */ (bot.inject.unwrapValue_(args, win.document));
  args.push(goog.partial(sendResponse, bot.ErrorCode.SUCCESS));

  if (win.addEventListener) {
    win.addEventListener('unload', onunload, true);
  } else {
    win.attachEvent('onunload', onunload);
  }

  var startTime = goog.now();
  try {
    fn.apply(win, args);

    // Register our timeout *after* the function has been invoked. This will
    // ensure we don't timeout on a function that invokes its callback after
    // a 0-based timeout.
    timeoutId = win.setTimeout(function() {
      sendResponse(bot.ErrorCode.SCRIPT_TIMEOUT,
                   Error('Timed out waiting for asyncrhonous script result ' +
                         'after ' + (goog.now() - startTime) + ' ms'));
    }, Math.max(0, timeout));
  } catch (ex) {
    sendResponse(ex.code || bot.ErrorCode.UNKNOWN_ERROR, ex);
  }

  function onunload() {
    sendResponse(bot.ErrorCode.UNKNOWN_ERROR,
        Error('Detected a page unload event; asynchronous script ' +
              'execution does not work across page loads.'));
  }
};


/**
 * Wraps the response to an injected script that executed successfully so it
 * can be JSON-ified for transmission to the process that injected this
 * script.
 * @param {*} value The script result.
 * @return {{status:bot.ErrorCode,value:*}} The wrapped value.
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol#Responses
 */
bot.inject.wrapResponse = function(value) {
  return {
    'status': bot.ErrorCode.SUCCESS,
    'value': bot.inject.wrapValue(value)
  };
};


/**
 * Wraps a JavaScript error in an object-literal so that it can be JSON-ified
 * for transmission to the process that injected this script.
 * @param {Error} err The error to wrap.
 * @return {{status:bot.ErrorCode,value:*}} The wrapped error object.
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol#Failed_Commands
 */
bot.inject.wrapError = function(err) {
  // TODO(user): Parse stackTrace
  return {
    'status': goog.object.containsKey(err, 'code') ?
        err['code'] : bot.ErrorCode.UNKNOWN_ERROR,
    // TODO(user): Parse stackTrace
    'value': {
      'message': err.message
    }
  };
};


/**
 * The property key used to store the element cache on the DOCUMENT node
 * when it is injected into the page. Since compiling each browser atom results
 * in a different symbol table, we must use this known key to access the cache.
 * This ensures the same object is used between injections of different atoms.
 * @const
 * @private {string}
 */
bot.inject.cache.CACHE_KEY_ = '$wdc_';


/**
 * The prefix for each key stored in an cache.
 * @type {string}
 * @const
 */
bot.inject.cache.ELEMENT_KEY_PREFIX = ':wdc:';


/**
 * Retrieves the cache object for the given window. Will initialize the cache
 * if it does not yet exist.
 * @param {Document=} opt_doc The document whose cache to retrieve. Defaults to
 *     the current document.
 * @return {Object.<string, (Element|Window)>} The cache object.
 * @private
 */
bot.inject.cache.getCache_ = function(opt_doc) {
  var doc = opt_doc || document;
  var cache = doc[bot.inject.cache.CACHE_KEY_];
  if (!cache) {
    cache = doc[bot.inject.cache.CACHE_KEY_] = {};
    // Store the counter used for generated IDs in the cache so that it gets
    // reset whenever the cache does.
    cache.nextId = goog.now();
  }
  // Sometimes the nextId does not get initialized and returns NaN
  // TODO: Generate UID on the fly instead.
  if (!cache.nextId) {
    cache.nextId = goog.now();
  }
  return cache;
};


/**
 * Adds an element to its ownerDocument's cache.
 * @param {(Element|Window)} el The element or Window object to add.
 * @return {string} The key generated for the cached element.
 */
bot.inject.cache.addElement = function(el) {
  // Check if the element already exists in the cache.
  var cache = bot.inject.cache.getCache_(el.ownerDocument);
  var id = goog.object.findKey(cache, function(value) {
    return value == el;
  });
  if (!id) {
    id = bot.inject.cache.ELEMENT_KEY_PREFIX + cache.nextId++;
    cache[id] = el;
  }
  return id;
};


/**
 * Retrieves an element from the cache. Will verify that the element is
 * still attached to the DOM before returning.
 * @param {string} key The element's key in the cache.
 * @param {Document=} opt_doc The document whose cache to retrieve the element
 *     from. Defaults to the current document.
 * @return {Element|Window} The cached element.
 */
bot.inject.cache.getElement = function(key, opt_doc) {
  key = decodeURIComponent(key);
  var doc = opt_doc || document;
  var cache = bot.inject.cache.getCache_(doc);
  if (!goog.object.containsKey(cache, key)) {
    // Throw STALE_ELEMENT_REFERENCE instead of NO_SUCH_ELEMENT since the
    // key may have been defined by a prior document's cache.
    throw new bot.Error(bot.ErrorCode.STALE_ELEMENT_REFERENCE,
        'Element does not exist in cache');
  }

  var el = cache[key];

  // If this is a Window check if it's closed
  if (goog.object.containsKey(el, 'setInterval')) {
    if (el.closed) {
      delete cache[key];
      throw new bot.Error(bot.ErrorCode.NO_SUCH_WINDOW,
          'Window has been closed.');
    }
    return el;
  }

  // Make sure the element is still attached to the DOM before returning.
  var node = el;
  while (node) {
    if (node == doc.documentElement) {
      return el;
    }
    node = node.parentNode;
  }
  delete cache[key];
  throw new bot.Error(bot.ErrorCode.STALE_ELEMENT_REFERENCE,
      'Element is no longer attached to the DOM');
};

// Copyright 2012 WebDriver committers
// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Provides JSON utilities that uses native JSON parsing where
 * possible (a feature not currently offered by Closure).
 */

goog.provide('bot.json');

goog.require('bot.userAgent');
goog.require('goog.json');
goog.require('goog.userAgent');


/**
 * @define {boolean} NATIVE_JSON indicates whether the code should rely on the
 * native {@ocde JSON} functions, if available.
 *
 * <p>The JSON functions can be defined by external libraries like Prototype
 * and setting this flag to false forces the use of Closure's goog.json
 * implementation.
 *
 * <p>If your JavaScript can be loaded by a third_party site and you are wary
 * about relying on the native functions, specify
 * "--define bot.json.NATIVE_JSON=false" to the Closure compiler.
 */
bot.json.NATIVE_JSON = true;


/**
 * Whether the current browser supports the native JSON interface.
 * @const
 * @see http://caniuse.com/#search=JSON
 * @private {boolean}
 */
bot.json.SUPPORTS_NATIVE_JSON_ =
    // List WebKit and Opera first since every supported version of these
    // browsers supports native JSON (and we can compile away large chunks of
    // code for individual fragments by setting the appropriate compiler flags).
    goog.userAgent.WEBKIT || goog.userAgent.OPERA ||
        (goog.userAgent.GECKO && bot.userAgent.isEngineVersion(3.5)) ||
        (goog.userAgent.IE && bot.userAgent.isEngineVersion(8));


/**
 * Converts a JSON object to its string representation.
 * @param {*} jsonObj The input object.
 * @param {?(function(string, *): *)=} opt_replacer A replacer function called
 *     for each (key, value) pair that determines how the value should be
 *     serialized. By default, this just returns the value and allows default
 *     serialization to kick in.
 * @return {string} A JSON string representation of the input object.
 */
bot.json.stringify = bot.json.NATIVE_JSON && bot.json.SUPPORTS_NATIVE_JSON_ ?
    JSON.stringify : goog.json.serialize;


/**
 * Parses a JSON string and returns the result.
 * @param {string} jsonStr The string to parse.
 * @return {*} The JSON object.
 * @throws {Error} If the input string is an invalid JSON string.
 */
bot.json.parse = bot.json.NATIVE_JSON && bot.json.SUPPORTS_NATIVE_JSON_ ?
    JSON.parse : goog.json.parse;
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview The file contains an abstraction of a keyboad
 * for simulating the presing and releasing of keys.
 */

goog.provide('bot.Keyboard');
goog.provide('bot.Keyboard.Key');
goog.provide('bot.Keyboard.Keys');

goog.require('bot.Device');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.dom');
goog.require('bot.events.EventType');
goog.require('goog.array');
goog.require('goog.dom.TagName');
goog.require('goog.dom.selection');
goog.require('goog.structs.Map');
goog.require('goog.structs.Set');
goog.require('goog.userAgent');



/**
 * A keyboard that provides atomic typing actions.
 *
 * @constructor
 * @param {{pressed: !Array.<!bot.Keyboard.Key>,
            currentPos: number}=} opt_state Optional keyboard state.
 * @extends {bot.Device}
 */
bot.Keyboard = function(opt_state) {
  goog.base(this);

  /** @private {boolean} */
  this.editable_ = bot.dom.isEditable(this.getElement());

  /** @private {number} */
  this.currentPos_ = 0;

  /** @private {!goog.structs.Set.<!bot.Keyboard.Key>} */
  this.pressed_ = new goog.structs.Set();

  if (opt_state) {
    // If a state is passed, let's assume we were passed an object with
    // the correct properties.
    goog.array.forEach(opt_state['pressed'], function(key) {
      this.setKeyPressed_(key, true);
    }, this);

    this.currentPos_ = opt_state['currentPos'];
  }
};
goog.inherits(bot.Keyboard, bot.Device);


/**
 * Maps characters to (key,boolean) pairs, where the key generates the
 * character and the boolean is true when the shift must be pressed.
 *
 * @private {!Object.<string, {key: !bot.Keyboard.Key, shift: boolean}>}
 * @const
 */
bot.Keyboard.CHAR_TO_KEY_ = {};


/**
 * Constructs a new key and, if it is a character key, adds a mapping from the
 * character to is in the CHAR_TO_KEY_ map. Using this factory function instead
 * of the new keyword, also helps reduce the size of the compiled Js fragment.
 *
 * @param {?number|{gecko: ?number, ieWebkit: ?number, opera: ?number}} code
 *     Either a single keycode or a record of per-browser keycodes.
 * @param {string=} opt_char Character when shift is not pressed.
 * @param {string=} opt_shiftChar Character when shift is pressed.
 * @return {!bot.Keyboard.Key} The new key.
 * @private
 */
bot.Keyboard.newKey_ = function(code, opt_char, opt_shiftChar) {
  if (goog.isObject(code)) {
    if (goog.userAgent.GECKO) {
      code = code.gecko;
    } else if (goog.userAgent.OPERA) {
      code = code.opera;
    } else {  // IE and Webkit
      code = code.ieWebkit;
    }
  }
  var key = new bot.Keyboard.Key(code, opt_char, opt_shiftChar);

  // For a character key, potentially map the character to the key in the
  // CHAR_TO_KEY_ map. Because of numpad, multiple keys may have the same
  // character. To avoid mapping numpad keys, we overwrite a mapping only if
  // the key has a distinct shift character.
  if (opt_char && (!(opt_char in bot.Keyboard.CHAR_TO_KEY_) || opt_shiftChar)) {
    bot.Keyboard.CHAR_TO_KEY_[opt_char] = {key: key, shift: false};
    if (opt_shiftChar) {
      bot.Keyboard.CHAR_TO_KEY_[opt_shiftChar] = {key: key, shift: true};
    }
  }

  return key;
};



/**
 * A key on the keyboard.
 *
 * @constructor
 * @param {?number} code Keycode for the key; null for the (rare) case
 *     that pressing the key issues no key events.
 * @param {string=} opt_char Character when shift is not pressed; null
 *     when the key does not cause a character to be typed.
 * @param {string=} opt_shiftChar Character when shift is pressed; null
 *     when the key does not cause a character to be typed.
 */
bot.Keyboard.Key = function(code, opt_char, opt_shiftChar) {
  /** @type {?number} */
  this.code = code;

  /** @type {?string} */
  this.character = opt_char || null;

  /** @type {?string} */
  this.shiftChar = opt_shiftChar || this.character;
};


/**
 * An enumeration of keys known to this module.
 *
 * @enum {!bot.Keyboard.Key}
 */
bot.Keyboard.Keys = {
  BACKSPACE: bot.Keyboard.newKey_(8),
  TAB: bot.Keyboard.newKey_(9),
  ENTER: bot.Keyboard.newKey_(13),
  SHIFT: bot.Keyboard.newKey_(16),
  CONTROL: bot.Keyboard.newKey_(17),
  ALT: bot.Keyboard.newKey_(18),
  PAUSE: bot.Keyboard.newKey_(19),
  CAPS_LOCK: bot.Keyboard.newKey_(20),
  ESC: bot.Keyboard.newKey_(27),
  SPACE: bot.Keyboard.newKey_(32, ' '),
  PAGE_UP: bot.Keyboard.newKey_(33),
  PAGE_DOWN: bot.Keyboard.newKey_(34),
  END: bot.Keyboard.newKey_(35),
  HOME: bot.Keyboard.newKey_(36),
  LEFT: bot.Keyboard.newKey_(37),
  UP: bot.Keyboard.newKey_(38),
  RIGHT: bot.Keyboard.newKey_(39),
  DOWN: bot.Keyboard.newKey_(40),
  PRINT_SCREEN: bot.Keyboard.newKey_(44),
  INSERT: bot.Keyboard.newKey_(45),
  DELETE: bot.Keyboard.newKey_(46),

  // Number keys
  ZERO: bot.Keyboard.newKey_(48, '0', ')'),
  ONE: bot.Keyboard.newKey_(49, '1', '!'),
  TWO: bot.Keyboard.newKey_(50, '2', '@'),
  THREE: bot.Keyboard.newKey_(51, '3', '#'),
  FOUR: bot.Keyboard.newKey_(52, '4', '$'),
  FIVE: bot.Keyboard.newKey_(53, '5', '%'),
  SIX: bot.Keyboard.newKey_(54, '6', '^'),
  SEVEN: bot.Keyboard.newKey_(55, '7', '&'),
  EIGHT: bot.Keyboard.newKey_(56, '8', '*'),
  NINE: bot.Keyboard.newKey_(57, '9', '('),

  // Letter keys
  A: bot.Keyboard.newKey_(65, 'a', 'A'),
  B: bot.Keyboard.newKey_(66, 'b', 'B'),
  C: bot.Keyboard.newKey_(67, 'c', 'C'),
  D: bot.Keyboard.newKey_(68, 'd', 'D'),
  E: bot.Keyboard.newKey_(69, 'e', 'E'),
  F: bot.Keyboard.newKey_(70, 'f', 'F'),
  G: bot.Keyboard.newKey_(71, 'g', 'G'),
  H: bot.Keyboard.newKey_(72, 'h', 'H'),
  I: bot.Keyboard.newKey_(73, 'i', 'I'),
  J: bot.Keyboard.newKey_(74, 'j', 'J'),
  K: bot.Keyboard.newKey_(75, 'k', 'K'),
  L: bot.Keyboard.newKey_(76, 'l', 'L'),
  M: bot.Keyboard.newKey_(77, 'm', 'M'),
  N: bot.Keyboard.newKey_(78, 'n', 'N'),
  O: bot.Keyboard.newKey_(79, 'o', 'O'),
  P: bot.Keyboard.newKey_(80, 'p', 'P'),
  Q: bot.Keyboard.newKey_(81, 'q', 'Q'),
  R: bot.Keyboard.newKey_(82, 'r', 'R'),
  S: bot.Keyboard.newKey_(83, 's', 'S'),
  T: bot.Keyboard.newKey_(84, 't', 'T'),
  U: bot.Keyboard.newKey_(85, 'u', 'U'),
  V: bot.Keyboard.newKey_(86, 'v', 'V'),
  W: bot.Keyboard.newKey_(87, 'w', 'W'),
  X: bot.Keyboard.newKey_(88, 'x', 'X'),
  Y: bot.Keyboard.newKey_(89, 'y', 'Y'),
  Z: bot.Keyboard.newKey_(90, 'z', 'Z'),

  // Branded keys
  META: bot.Keyboard.newKey_(
      goog.userAgent.WINDOWS ? {gecko: 91, ieWebkit: 91, opera: 219} :
          (goog.userAgent.MAC ? {gecko: 224, ieWebkit: 91, opera: 17} :
              {gecko: 0, ieWebkit: 91, opera: null})),  // Linux
  META_RIGHT: bot.Keyboard.newKey_(
      goog.userAgent.WINDOWS ? {gecko: 92, ieWebkit: 92, opera: 220} :
          (goog.userAgent.MAC ? {gecko: 224, ieWebkit: 93, opera: 17} :
              {gecko: 0, ieWebkit: 92, opera: null})),  // Linux
  CONTEXT_MENU: bot.Keyboard.newKey_(
      goog.userAgent.WINDOWS ? {gecko: 93, ieWebkit: 93, opera: 0} :
          (goog.userAgent.MAC ? {gecko: 0, ieWebkit: 0, opera: 16} :
              {gecko: 93, ieWebkit: null, opera: 0})),  // Linux

  // Numpad keys
  NUM_ZERO: bot.Keyboard.newKey_({gecko: 96, ieWebkit: 96, opera: 48}, '0'),
  NUM_ONE: bot.Keyboard.newKey_({gecko: 97, ieWebkit: 97, opera: 49}, '1'),
  NUM_TWO: bot.Keyboard.newKey_({gecko: 98, ieWebkit: 98, opera: 50}, '2'),
  NUM_THREE: bot.Keyboard.newKey_({gecko: 99, ieWebkit: 99, opera: 51}, '3'),
  NUM_FOUR: bot.Keyboard.newKey_({gecko: 100, ieWebkit: 100, opera: 52}, '4'),
  NUM_FIVE: bot.Keyboard.newKey_({gecko: 101, ieWebkit: 101, opera: 53}, '5'),
  NUM_SIX: bot.Keyboard.newKey_({gecko: 102, ieWebkit: 102, opera: 54}, '6'),
  NUM_SEVEN: bot.Keyboard.newKey_({gecko: 103, ieWebkit: 103, opera: 55}, '7'),
  NUM_EIGHT: bot.Keyboard.newKey_({gecko: 104, ieWebkit: 104, opera: 56}, '8'),
  NUM_NINE: bot.Keyboard.newKey_({gecko: 105, ieWebkit: 105, opera: 57}, '9'),
  NUM_MULTIPLY: bot.Keyboard.newKey_(
      {gecko: 106, ieWebkit: 106, opera: goog.userAgent.LINUX ? 56 : 42}, '*'),
  NUM_PLUS: bot.Keyboard.newKey_(
      {gecko: 107, ieWebkit: 107, opera: goog.userAgent.LINUX ? 61 : 43}, '+'),
  NUM_MINUS: bot.Keyboard.newKey_(
      {gecko: 109, ieWebkit: 109, opera: goog.userAgent.LINUX ? 109 : 45}, '-'),
  NUM_PERIOD: bot.Keyboard.newKey_(
      {gecko: 110, ieWebkit: 110, opera: goog.userAgent.LINUX ? 190 : 78}, '.'),
  NUM_DIVISION: bot.Keyboard.newKey_(
      {gecko: 111, ieWebkit: 111, opera: goog.userAgent.LINUX ? 191 : 47}, '/'),
  NUM_LOCK: bot.Keyboard.newKey_(
      (goog.userAgent.LINUX && goog.userAgent.OPERA) ? null : 144),

  // Function keys
  F1: bot.Keyboard.newKey_(112),
  F2: bot.Keyboard.newKey_(113),
  F3: bot.Keyboard.newKey_(114),
  F4: bot.Keyboard.newKey_(115),
  F5: bot.Keyboard.newKey_(116),
  F6: bot.Keyboard.newKey_(117),
  F7: bot.Keyboard.newKey_(118),
  F8: bot.Keyboard.newKey_(119),
  F9: bot.Keyboard.newKey_(120),
  F10: bot.Keyboard.newKey_(121),
  F11: bot.Keyboard.newKey_(122),
  F12: bot.Keyboard.newKey_(123),

  // Punctuation keys
  EQUALS: bot.Keyboard.newKey_(
      {gecko: 107, ieWebkit: 187, opera: 61}, '=', '+'),
  SEPARATOR: bot.Keyboard.newKey_(108, ','),
  HYPHEN: bot.Keyboard.newKey_(
      {gecko: 109, ieWebkit: 189, opera: 109}, '-', '_'),
  COMMA: bot.Keyboard.newKey_(188, ',', '<'),
  PERIOD: bot.Keyboard.newKey_(190, '.', '>'),
  SLASH: bot.Keyboard.newKey_(191, '/', '?'),
  BACKTICK: bot.Keyboard.newKey_(192, '`', '~'),
  OPEN_BRACKET: bot.Keyboard.newKey_(219, '[', '{'),
  BACKSLASH: bot.Keyboard.newKey_(220, '\\', '|'),
  CLOSE_BRACKET: bot.Keyboard.newKey_(221, ']', '}'),
  SEMICOLON: bot.Keyboard.newKey_(
      {gecko: 59, ieWebkit: 186, opera: 59}, ';', ':'),
  APOSTROPHE: bot.Keyboard.newKey_(222, '\'', '"')
};


/**
 * Given a character, returns a pair of a key and a boolean: the key being one
 * that types the character and the boolean indicating whether the key must be
 * shifted to type it. This function will never return a numpad key; that is,
 * it will always return a symbol key when given a number or math symbol.
 *
 * If given a character for which this module does not know the key (the key
 * is not in the bot.Keyboard.Keys enumeration), returns a key that types the
 * given character but has a (likely incorrect) keycode of zero.
 *
 * @param {string} ch Single character.
 * @return {{key: !bot.Keyboard.Key, shift: boolean}} A pair of a key and
 *     a boolean indicating whether shift must be pressed for the character.
 */
bot.Keyboard.Key.fromChar = function(ch) {
  if (ch.length != 1) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
                        'Argument not a single character: ' + ch);
  }
  var keyShiftPair = bot.Keyboard.CHAR_TO_KEY_[ch];
  if (!keyShiftPair) {
    // We don't know the true keycode of non-US keyboard characters, but
    // ch.toUpperCase().charCodeAt(0) should occasionally be right, and
    // at least yield a positive number.
    var upperCase = ch.toUpperCase();
    var keyCode = upperCase.charCodeAt(0);
    var key = bot.Keyboard.newKey_(keyCode, ch.toLowerCase(), upperCase);
    keyShiftPair = {key: key, shift: (ch != key.character)};
  }
  return keyShiftPair;
};


/**
 * Array of modifier keys.
 *
 * @type {!Array.<!bot.Keyboard.Key>}
 * @const
 */
bot.Keyboard.MODIFIERS = [
  bot.Keyboard.Keys.ALT,
  bot.Keyboard.Keys.CONTROL,
  bot.Keyboard.Keys.META,
  bot.Keyboard.Keys.SHIFT
];

/**
 * Map of modifier to key.
 * @private {!goog.structs.Map.<!bot.Device.Modifier, !bot.Keyboard.Key>}
 */
bot.Keyboard.MODIFIER_TO_KEY_MAP_ = (function() {
  var modifiersMap = new goog.structs.Map();
  modifiersMap.set(bot.Device.Modifier.SHIFT,
      bot.Keyboard.Keys.SHIFT);
  modifiersMap.set(bot.Device.Modifier.CONTROL,
      bot.Keyboard.Keys.CONTROL);
  modifiersMap.set(bot.Device.Modifier.ALT,
      bot.Keyboard.Keys.ALT);
  modifiersMap.set(bot.Device.Modifier.META,
      bot.Keyboard.Keys.META);

  return modifiersMap;
})();


/**
 * The reverse map - key to modifier.
 * @private {!goog.structs.Map.<number, !bot.Device.Modifier>}
 */
bot.Keyboard.KEY_TO_MODIFIER_ = (function(modifiersMap) {
  var keyToModifierMap = new goog.structs.Map();
  goog.array.forEach(modifiersMap.getKeys(), function(m) {
      keyToModifierMap.set(modifiersMap.get(m).code, m);
  });

  return keyToModifierMap;
})(bot.Keyboard.MODIFIER_TO_KEY_MAP_);


/**
 * Set the modifier state if the provided key is one, otherwise just add
 * to the list of pressed keys.
 * @param {bot.Keyboard.Key} key
 * @param {boolean} isPressed
 * @private
 */
bot.Keyboard.prototype.setKeyPressed_ = function(key, isPressed) {
  if (goog.array.contains(bot.Keyboard.MODIFIERS, key)) {
    var modifier = /** @type {bot.Device.Modifier}*/ (
        bot.Keyboard.KEY_TO_MODIFIER_.get(key.code));
    this.modifiersState.setPressed(modifier, isPressed);
  }

  if (isPressed) {
    this.pressed_.add(key);
  } else {
    this.pressed_.remove(key);
  }
};


/**
 * The value used for newlines in the current browser/OS combination. Although
 * the line endings look platform dependent, they are browser dependent. In
 * particular, Opera uses \r\n on all platforms.
 * @private {string}
 * @const
 */
bot.Keyboard.NEW_LINE_ =
    goog.userAgent.IE || goog.userAgent.OPERA ? '\r\n' : '\n';


/**
 * Returns whether the key is currently pressed.
 *
 * @param {bot.Keyboard.Key} key Key.
 * @return {boolean} Whether the key is pressed.
 */
bot.Keyboard.prototype.isPressed = function(key) {
  return this.pressed_.contains(key);
};


/**
 * Presses the given key on the keyboard. Keys that are pressed can be pressed
 * again before releasing, to simulate repeated keys, except for modifier keys,
 * which must be released before they can be pressed again.
 *
 * @param {!bot.Keyboard.Key} key Key to press.
 */
bot.Keyboard.prototype.pressKey = function(key) {
  if (goog.array.contains(bot.Keyboard.MODIFIERS, key) && this.isPressed(key)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot press a modifier key that is already pressed.');
  }

  // Note that GECKO is special-cased below because of
  // https://bugzilla.mozilla.org/show_bug.cgi?id=501496. "preventDefault on
  // keydown does not cancel following keypress"
  var performDefault = !goog.isNull(key.code) &&
      this.fireKeyEvent_(bot.events.EventType.KEYDOWN, key);

  // Fires keydown and stops if unsuccessful.
  if (performDefault || goog.userAgent.GECKO) {
    // Fires keypress if required and stops if unsuccessful.
    if (!this.requiresKeyPress_(key) ||
        this.fireKeyEvent_(
            bot.events.EventType.KEYPRESS, key, !performDefault)) {
      if (performDefault) {
        this.maybeSubmitForm_(key);
        if (this.editable_) {
          this.maybeEditText_(key);
        }
      }
    }
  }

  this.setKeyPressed_(key, true);
};


/**
 * Whether the given key currently requires a keypress.
 * TODO(user): Make this dependent on the state of the modifier keys.
 *
 * @param {bot.Keyboard.Key} key Key.
 * @return {boolean} Whether it requires a keypress event.
 * @private
 */
bot.Keyboard.prototype.requiresKeyPress_ = function(key) {
  if (key.character || key == bot.Keyboard.Keys.ENTER) {
    return true;
  } else if (goog.userAgent.WEBKIT) {
    return false;
  } else if (goog.userAgent.IE) {
    return key == bot.Keyboard.Keys.ESC;
  } else { // Gecko and Opera
    switch (key) {
      case bot.Keyboard.Keys.SHIFT:
      case bot.Keyboard.Keys.CONTROL:
      case bot.Keyboard.Keys.ALT:
        return false;
      case bot.Keyboard.Keys.META:
      case bot.Keyboard.Keys.META_RIGHT:
      case bot.Keyboard.Keys.CONTEXT_MENU:
        return goog.userAgent.GECKO;
      default:
        return true;
    }
  }
};


/**
 * Maybe submit a form if the ENTER key is released.  On non-FF browsers, firing
 * the keyPress and keyRelease events for the ENTER key does not result in a
 * form being submitted so we have to fire the form submit event as well.
 *
 * @param {bot.Keyboard.Key} key Key.
 * @private
 */
bot.Keyboard.prototype.maybeSubmitForm_ = function(key) {
  if (key != bot.Keyboard.Keys.ENTER) {
    return;
  }
  if (goog.userAgent.GECKO ||
      !bot.dom.isElement(this.getElement(), goog.dom.TagName.INPUT)) {
    return;
  }

  var form = bot.Device.findAncestorForm(this.getElement());
  if (form) {
    var inputs = form.getElementsByTagName('input');
    var hasSubmit = goog.array.some(inputs, function(e) {
      return bot.Device.isFormSubmitElement(e);
    });
    // The second part of this if statement will always include forms on Safari
    // version < 5.
    if (hasSubmit || inputs.length == 1 ||
        (goog.userAgent.WEBKIT && !bot.userAgent.isEngineVersion(534))) {
      this.submitForm(form);
    }
  }
};


/**
 * Maybe edit text when a key is pressed in an editable form.
 *
 * @param {!bot.Keyboard.Key} key Key that was pressed.
 * @private
 */
bot.Keyboard.prototype.maybeEditText_ = function(key) {
  if (key.character) {
    this.updateOnCharacter_(key);
  } else {
    switch (key) {
      case bot.Keyboard.Keys.ENTER:
        this.updateOnEnter_();
        break;
      case bot.Keyboard.Keys.BACKSPACE:
      case bot.Keyboard.Keys.DELETE:
        this.updateOnBackspaceOrDelete_(key);
        break;
      case bot.Keyboard.Keys.LEFT:
      case bot.Keyboard.Keys.RIGHT:
        this.updateOnLeftOrRight_(key);
        break;
      case bot.Keyboard.Keys.HOME:
      case bot.Keyboard.Keys.END:
        this.updateOnHomeOrEnd_(key);
        break;
    }
  }
};


/**
 * Releases the given key on the keyboard. Releasing a key that is not
 * pressed results in an exception.
 *
 * @param {!bot.Keyboard.Key} key Key to release.
 */
bot.Keyboard.prototype.releaseKey = function(key) {
  if (!this.isPressed(key)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot release a key that is not pressed. (' + key.code + ')');
  }
  if (!goog.isNull(key.code)) {
    this.fireKeyEvent_(bot.events.EventType.KEYUP, key);
  }

  this.setKeyPressed_(key, false);
};


/**
 * Given the current state of the SHIFT and CAPS_LOCK key, returns the
 * character that will be typed is the specified key is pressed.
 *
 * @param {!bot.Keyboard.Key} key Key.
 * @return {string} Character to be typed.
 * @private
 */
bot.Keyboard.prototype.getChar_ = function(key) {
  if (!key.character) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR, 'not a character key');
  }
  var shiftPressed = this.isPressed(bot.Keyboard.Keys.SHIFT);
  return /** @type {string} */ (shiftPressed ? key.shiftChar : key.character);
};


/**
 * Whether firing a keypress event causes text to be edited without any
 * additional logic to surgically apply the edit.
 *
 * @const
 * @private {boolean}
 */
bot.Keyboard.KEYPRESS_EDITS_TEXT_ = goog.userAgent.GECKO &&
    !bot.userAgent.isEngineVersion(12);


/**
 * @param {!bot.Keyboard.Key} key Key with character to insert.
 * @private
 */
bot.Keyboard.prototype.updateOnCharacter_ = function(key) {
  if (bot.Keyboard.KEYPRESS_EDITS_TEXT_) {
    return;
  }

  var character = this.getChar_(key);
  var newPos = goog.dom.selection.getStart(this.getElement()) + 1;
  goog.dom.selection.setText(this.getElement(), character);
  goog.dom.selection.setStart(this.getElement(), newPos);
  if (goog.userAgent.WEBKIT) {
    this.fireHtmlEvent(bot.events.EventType.TEXTINPUT);
  }
  if (!bot.userAgent.IE_DOC_PRE9) {
    this.fireHtmlEvent(bot.events.EventType.INPUT);
  }
  this.updateCurrentPos_(newPos);
};


/** @private */
bot.Keyboard.prototype.updateOnEnter_ = function() {
  if (bot.Keyboard.KEYPRESS_EDITS_TEXT_) {
    return;
  }

  // WebKit fires text input regardless of whether a new line is added, see:
  // https://bugs.webkit.org/show_bug.cgi?id=54152
  if (goog.userAgent.WEBKIT) {
    this.fireHtmlEvent(bot.events.EventType.TEXTINPUT);
  }
  if (bot.dom.isElement(this.getElement(), goog.dom.TagName.TEXTAREA)) {
    var newPos = goog.dom.selection.getStart(this.getElement()) + 
        bot.Keyboard.NEW_LINE_.length;
    goog.dom.selection.setText(this.getElement(), bot.Keyboard.NEW_LINE_);
    goog.dom.selection.setStart(this.getElement(), newPos);
    if (!goog.userAgent.IE) {
      this.fireHtmlEvent(bot.events.EventType.INPUT);
    }
    this.updateCurrentPos_(newPos);
  }
};


/**
 * @param {!bot.Keyboard.Key} key Backspace or delete key.
 * @private
 */
bot.Keyboard.prototype.updateOnBackspaceOrDelete_ = function(key) {
  if (bot.Keyboard.KEYPRESS_EDITS_TEXT_) {
    return;
  }

  // Determine what should be deleted.  If text is already selected, that
  // text is deleted, else we move left/right from the current cursor.
  var endpoints = goog.dom.selection.getEndPoints(this.getElement());
  if (endpoints[0] == endpoints[1]) {
    if (key == bot.Keyboard.Keys.BACKSPACE) {
      goog.dom.selection.setStart(this.getElement(), endpoints[1] - 1);
      // On IE, changing goog.dom.selection.setStart also changes the end.
      goog.dom.selection.setEnd(this.getElement(), endpoints[1]);
    } else {
      goog.dom.selection.setEnd(this.getElement(), endpoints[1] + 1);
    }
  }

  // If the endpoints are equal (e.g., the cursor was at the beginning/end
  // of the input), the text field won't be changed.
  endpoints = goog.dom.selection.getEndPoints(this.getElement());
  var textChanged = !(endpoints[0] == this.getElement().value.length ||
                      endpoints[1] == 0);
  goog.dom.selection.setText(this.getElement(), '');

  // Except for IE and GECKO, we need to fire the input event manually, but
  // only if the text was actually changed.
  // Note: Gecko has some strange behavior with the input event.  In a
  //  textarea, backspace always sends an input event, while delete only
  //  sends one if you actually change the text.
  //  In a textbox/password box, backspace always sends an input event unless
  //  the box has no text.  Delete behaves the same way in Firefox 3.0, but
  //  in later versions it only fires an input event if no text changes.
  if (!goog.userAgent.IE && textChanged ||
      (goog.userAgent.GECKO && key == bot.Keyboard.Keys.BACKSPACE)) {
    this.fireHtmlEvent(bot.events.EventType.INPUT);
  }

  // Update the cursor position
  endpoints = goog.dom.selection.getEndPoints(this.getElement());
  this.updateCurrentPos_(endpoints[1]);
};


/**
 * @param {!bot.Keyboard.Key} key Special key to press.
 * @private
 */
bot.Keyboard.prototype.updateOnLeftOrRight_ = function(key) {
  var element = this.getElement();
  var start = goog.dom.selection.getStart(element);
  var end = goog.dom.selection.getEnd(element);

  var newPos, startPos = 0, endPos = 0;
  if (key == bot.Keyboard.Keys.LEFT) {
    if (this.isPressed(bot.Keyboard.Keys.SHIFT)) {
      // If the current position of the cursor is at the start of the
      // selection, pressing left expands the selection one character to the
      // left; otherwise, pressing left collapses it one character to the
      // left.
      if (this.currentPos_ == start) {
        // Never attempt to move further left than the beginning of the text.
        startPos = Math.max(start - 1, 0);
        endPos = end;
        newPos = startPos;
      } else {
        startPos = start;
        endPos = end - 1;
        newPos = endPos;
      }
    } else {
      // With no current selection, pressing left moves the cursor one
      // character to the left; with an existing selection, it collapses the
      // selection to the beginning of the selection.
      newPos = start == end ? Math.max(start - 1, 0) : start;
    }
  } else {  // (key == bot.Keyboard.Keys.RIGHT)
    if (this.isPressed(bot.Keyboard.Keys.SHIFT)) {
      // If the current position of the cursor is at the end of the selection,
      // pressing right expands the selection one character to the right;
      // otherwise, pressing right collapses it one character to the right.
      if (this.currentPos_ == end) {
        startPos = start;
        // Never attempt to move further right than the end of the text.
        endPos = Math.min(end + 1, element.value.length);
        newPos = endPos;
      } else {
        startPos = start + 1;
        endPos = end;
        newPos = startPos;
      }
    } else {
      // With no current selection, pressing right moves the cursor one
      // character to the right; with an existing selection, it collapses the
      // selection to the end of the selection.
      newPos = start == end ? Math.min(end + 1, element.value.length) : end;
    }
  }

  if (this.isPressed(bot.Keyboard.Keys.SHIFT)) {
    goog.dom.selection.setStart(element, startPos);
    // On IE, changing goog.dom.selection.setStart also changes the end.
    goog.dom.selection.setEnd(element, endPos);
  } else {
    goog.dom.selection.setCursorPosition(element, newPos);
  }
  this.updateCurrentPos_(newPos);
};


/**
 * @param {!bot.Keyboard.Key} key Special key to press.
 * @private
 */
bot.Keyboard.prototype.updateOnHomeOrEnd_ = function(key) {
  var element = this.getElement();
  var start = goog.dom.selection.getStart(element);
  var end = goog.dom.selection.getEnd(element);
  // TODO: Handle multiline (TEXTAREA) elements.
  if (key == bot.Keyboard.Keys.HOME) {
    if (this.isPressed(bot.Keyboard.Keys.SHIFT)) {
      goog.dom.selection.setStart(element, 0);
      // If current position is at the end of the selection, typing home
      // changes the selection to begin at the beginning of the text, running
      // to the where the current selection begins. 
      var endPos = this.currentPos_ == start ? end : start;
      // On IE, changing goog.dom.selection.setStart also changes the end.
      goog.dom.selection.setEnd(element, endPos);
    } else {
      goog.dom.selection.setCursorPosition(element, 0);
    }
    this.updateCurrentPos_(0);
  } else {  // (key == bot.Keyboard.Keys.END)
    if (this.isPressed(bot.Keyboard.Keys.SHIFT)) {
      if (this.currentPos_ == start) {
        // Current position is at the beginning of the selection. Typing end
        // changes the selection to begin where the current selection ends, 
        // running to the end of the text. 
        goog.dom.selection.setStart(element, end);
      }
      goog.dom.selection.setEnd(element, element.value.length);
    } else {
      goog.dom.selection.setCursorPosition(element, element.value.length);
    }
    this.updateCurrentPos_(element.value.length);
  }
};

/**
* @param {number} pos New position of the cursor
* @private
*/
bot.Keyboard.prototype.updateCurrentPos_ = function(pos) {
  this.currentPos_ = pos;
};


/**
* @param {bot.events.EventType} type Event type.
* @param {!bot.Keyboard.Key} key Key.
* @param {boolean=} opt_preventDefault Whether the default event should be
*     prevented. Defaults to false.
* @return {boolean} Whether the event fired successfully or was cancelled.
* @private
*/
bot.Keyboard.prototype.fireKeyEvent_ = function(type, key, opt_preventDefault) {
  if (goog.isNull(key.code)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Key must have a keycode to be fired.');
  }

  var args = {
    altKey: this.isPressed(bot.Keyboard.Keys.ALT),
    ctrlKey: this.isPressed(bot.Keyboard.Keys.CONTROL),
    metaKey: this.isPressed(bot.Keyboard.Keys.META),
    shiftKey: this.isPressed(bot.Keyboard.Keys.SHIFT),
    keyCode: key.code,
    charCode: (key.character && type == bot.events.EventType.KEYPRESS) ?
        this.getChar_(key).charCodeAt(0) : 0,
    preventDefault: !!opt_preventDefault
  };

  return this.fireKeyboardEvent(type, args);
};


/**
 * Sets focus to the element. If the element does not have focus, place cursor
 * at the end of the text in the element.
 *
 * @param {!Element} element Element that is moved to.
 */
bot.Keyboard.prototype.moveCursor = function(element) {
  this.setElement(element);
  this.editable_ = bot.dom.isEditable(element);

  var focusChanged = this.focusOnElement();
  if (this.editable_ && focusChanged) {
    goog.dom.selection.setCursorPosition(element, element.value.length);
    this.updateCurrentPos_(element.value.length);
  }
};


/**
 * Serialize the current state of the keyboard.
 *
 * @return {{pressed: !Array.<!bot.Keyboard.Key>, currentPos: number}} The 
 *     current keyboard state.
 */
bot.Keyboard.prototype.getState = function () {
  // Need to use quoted literals here, so the compiler will not rename the
  // properties of the emitted object. When the object is created via the
  // "constructor", we will look for these *specific* properties. Everywhere
  // else internally, we use the dot-notation, so it's okay if the compiler
  // renames the internal variable name.
  return {
    'pressed': this.pressed_.getValues(),
    'currentPos': this.currentPos_
  };
};

/**
 * Returns the state of the modifier keys, to be shared with other input
 * devices.
 *
 * @return {bot.Device.ModifiersState} Modifiers state.
 */
bot.Keyboard.prototype.getModifiersState = function() {
  return this.modifiersState;
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview The file contains an abstraction of a mouse for
 * simulating the mouse actions.
 */

goog.provide('bot.Mouse');
goog.provide('bot.Mouse.Button');
goog.provide('bot.Mouse.State');

goog.require('bot');
goog.require('bot.Device');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.dom');
goog.require('bot.events.EventType');
goog.require('bot.userAgent');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('goog.userAgent');



/**
 * A mouse that provides atomic mouse actions. This mouse currently only
 * supports having one button pressed at a time.
 * @param {bot.Mouse.State=} opt_state The mouse's initial state.
 * @param {bot.Device.ModifiersState=} opt_modifiersState State of the keyboard.
 *
 * @constructor
 * @extends {bot.Device}
 */
bot.Mouse = function(opt_state, opt_modifiersState) {
  goog.base(this, opt_modifiersState);

  /** @private {?bot.Mouse.Button} */
  this.buttonPressed_ = null;

  /** @private {Element} */
  this.elementPressed_ = null;

  /** @private {!goog.math.Coordinate} */
  this.clientXY_ = new goog.math.Coordinate(0, 0);

  /** @private {boolean} */
  this.nextClickIsDoubleClick_ = false;

  /**
   * Whether this Mouse has ever explicitly interacted with any element.
   *
   * @private {boolean}
   */
  this.hasEverInteracted_ = false;

  if (opt_state) {
    this.buttonPressed_ = opt_state.buttonPressed;

    try {
      if (bot.dom.isElement(opt_state.elementPressed)) {
        this.elementPressed_ = opt_state.elementPressed;
      }
    } catch (ignored) {
      this.buttonPressed_ = null;
    }

    this.clientXY_ = opt_state.clientXY;
    this.nextClickIsDoubleClick_ = opt_state.nextClickIsDoubleClick;
    this.hasEverInteracted_ = opt_state.hasEverInteracted;

    try {
      if (bot.dom.isElement(opt_state.element)) {
        this.setElement((/** @type {!Element} */opt_state.element));
      }
    } catch (ignored) {
      this.buttonPressed_ = null;
    }
  }
};
goog.inherits(bot.Mouse, bot.Device);


/**
  * @typedef {{buttonPressed: ?bot.Mouse.Button,
  *           elementPressed: Element,
  *           clientXY: !goog.math.Coordinate,
  *           nextClickIsDoubleClick: boolean,
  *           hasEverInteracted: boolean,
  *           element: Element}}
  */
bot.Mouse.State;


/**
 * Enumeration of mouse buttons that can be pressed.
 *
 * @enum {number}
 */
bot.Mouse.Button = {
  LEFT: 0,
  MIDDLE: 1,
  RIGHT: 2
};


/**
 * Index to indicate no button pressed in bot.Mouse.MOUSE_BUTTON_VALUE_MAP_.
 *
 * @private {number}
 * @const
 */
bot.Mouse.NO_BUTTON_VALUE_INDEX_ = 3;


/**
 * Maps mouse events to an array of button argument value for each mouse button.
 * The array is indexed by the bot.Mouse.Button values. It encodes this table,
 * where each cell contains the (left/middle/right/none) button values.
 *               click/    mouseup/   mouseout/  mousemove  contextmenu
 *               dblclick/ mousedown  mouseover
 * IE_DOC_PRE9   0 0 0 X   1 4 2 X    0 0 0 0    1 4 2 0    X X 0 X
 * WEBKIT/IE9    0 1 2 X   0 1 2 X    0 1 2 0    0 1 2 0    X X 2 X
 * GECKO/OPERA   0 1 2 X   0 1 2 X    0 0 0 0    0 0 0 0    X X 2 X
 *
 * @private {!Object.<bot.events.EventType, !Array.<?number>>}
 * @const
 */
bot.Mouse.MOUSE_BUTTON_VALUE_MAP_ = (function() {
  // EventTypes can safely be used as keys without collisions in a JS Object,
  // because its toString method returns a unique string (the event type name).
  var buttonValueMap = {};
  if (bot.userAgent.IE_DOC_PRE9) {
    buttonValueMap[bot.events.EventType.CLICK] = [0, 0, 0, null];
    buttonValueMap[bot.events.EventType.CONTEXTMENU] = [null, null, 0, null];
    buttonValueMap[bot.events.EventType.MOUSEUP] = [1, 4, 2, null];
    buttonValueMap[bot.events.EventType.MOUSEOUT] = [0, 0, 0, 0];
    buttonValueMap[bot.events.EventType.MOUSEMOVE] = [1, 4, 2, 0];
  } else if (goog.userAgent.WEBKIT || bot.userAgent.IE_DOC_9) {
    buttonValueMap[bot.events.EventType.CLICK] = [0, 1, 2, null];
    buttonValueMap[bot.events.EventType.CONTEXTMENU] = [null, null, 2, null];
    buttonValueMap[bot.events.EventType.MOUSEUP] = [0, 1, 2, null];
    buttonValueMap[bot.events.EventType.MOUSEOUT] = [0, 1, 2, 0];
    buttonValueMap[bot.events.EventType.MOUSEMOVE] = [0, 1, 2, 0];
  } else {
    buttonValueMap[bot.events.EventType.CLICK] = [0, 1, 2, null];
    buttonValueMap[bot.events.EventType.CONTEXTMENU] = [null, null, 2, null];
    buttonValueMap[bot.events.EventType.MOUSEUP] = [0, 1, 2, null];
    buttonValueMap[bot.events.EventType.MOUSEOUT] = [0, 0, 0, 0];
    buttonValueMap[bot.events.EventType.MOUSEMOVE] = [0, 0, 0, 0];
  }

  if (bot.userAgent.IE_DOC_10) {
    buttonValueMap[bot.events.EventType.MSPOINTERDOWN] =
        buttonValueMap[bot.events.EventType.MOUSEUP];
    buttonValueMap[bot.events.EventType.MSPOINTERUP] =
        buttonValueMap[bot.events.EventType.MOUSEUP];
    buttonValueMap[bot.events.EventType.MSPOINTERMOVE] = [-1, -1, -1, -1];
    buttonValueMap[bot.events.EventType.MSPOINTEROUT] =
        buttonValueMap[bot.events.EventType.MSPOINTERMOVE];
    buttonValueMap[bot.events.EventType.MSPOINTEROVER] =
        buttonValueMap[bot.events.EventType.MSPOINTERMOVE];
  }

  buttonValueMap[bot.events.EventType.DBLCLICK] =
      buttonValueMap[bot.events.EventType.CLICK];
  buttonValueMap[bot.events.EventType.MOUSEDOWN] =
      buttonValueMap[bot.events.EventType.MOUSEUP];
  buttonValueMap[bot.events.EventType.MOUSEOVER] =
      buttonValueMap[bot.events.EventType.MOUSEOUT];
  return buttonValueMap;
})();


/**
 * Maps mouse events to corresponding MSPointer event.
 * @private {!Object.<bot.events.EventType, bot.events.EventType>}
 */
bot.Mouse.MOUSE_EVENT_MAP_ = {
  mousedown: bot.events.EventType.MSPOINTERDOWN,
  mousemove: bot.events.EventType.MSPOINTERMOVE,
  mouseout: bot.events.EventType.MSPOINTEROUT,
  mouseover: bot.events.EventType.MSPOINTEROVER,
  mouseup: bot.events.EventType.MSPOINTERUP
};


/**
 * Attempts to fire a mousedown event and then returns whether or not the
 * element should receive focus as a result of the mousedown.
 *
 * @return {boolean} Whether to focus on the element after the mousedown.
 * @private
 */
bot.Mouse.prototype.fireMousedown_ = function() {
  // On some browsers, a mouse down event on an OPTION or SELECT element cause
  // the SELECT to open, blocking further JS execution. This is undesirable,
  // and so needs to be detected. We always focus in this case.
  // TODO(simon): This is a nasty way to avoid locking the browser
  var isFirefox3 = goog.userAgent.GECKO && !bot.userAgent.isProductVersion(4);
  var blocksOnMousedown = (goog.userAgent.WEBKIT || isFirefox3) &&
      (bot.dom.isElement(this.getElement(), goog.dom.TagName.OPTION) ||
       bot.dom.isElement(this.getElement(), goog.dom.TagName.SELECT));
  if (blocksOnMousedown) {
    return true;
  }

  // On some browsers, if the mousedown event handler makes a focus() call to
  // change the active element, this preempts the focus that would happen by
  // default on the mousedown, so we should not explicitly focus in this case.
  var beforeActiveElement;
  var mousedownCanPreemptFocus = goog.userAgent.GECKO || goog.userAgent.IE;
  if (mousedownCanPreemptFocus) {
    beforeActiveElement = bot.dom.getActiveElement(this.getElement());
  }
  var performFocus = this.fireMouseEvent_(bot.events.EventType.MOUSEDOWN);
  if (performFocus && mousedownCanPreemptFocus &&
      beforeActiveElement != bot.dom.getActiveElement(this.getElement())) {
    return false;
  }
  return performFocus;
};


/**
 * Press a mouse button on an element that the mouse is interacting with.
 *
 * @param {!bot.Mouse.Button} button Button.
*/
bot.Mouse.prototype.pressButton = function(button) {
  if (!goog.isNull(this.buttonPressed_)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot press more then one button or an already pressed button.');
  }
  this.buttonPressed_ = button;
  this.elementPressed_ = this.getElement();

  var performFocus = this.fireMousedown_();
  if (performFocus) {
    this.focusOnElement();
  }
};


/**
 * Releases the pressed mouse button. Throws exception if no button pressed.
 *
 */
bot.Mouse.prototype.releaseButton = function() {
  if (goog.isNull(this.buttonPressed_)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot release a button when no button is pressed.');
  }

  this.fireMouseEvent_(bot.events.EventType.MOUSEUP);

  // TODO(user): Middle button can also trigger click.
  if (this.buttonPressed_ == bot.Mouse.Button.LEFT &&
      this.getElement() == this.elementPressed_) {
    this.clickElement(this.clientXY_,
        this.getButtonValue_(bot.events.EventType.CLICK));
    this.maybeDoubleClickElement_();

  // TODO(user): In Linux, this fires after mousedown event.
  } else if (this.buttonPressed_ == bot.Mouse.Button.RIGHT) {
    this.fireMouseEvent_(bot.events.EventType.CONTEXTMENU);
  }
  this.buttonPressed_ = null;
  this.elementPressed_ = null;
};


/**
 * A helper function to fire mouse double click events.
 *
 * @private
 */
bot.Mouse.prototype.maybeDoubleClickElement_ = function() {
  // Trigger an additional double click event if it is the second click.
  if (this.nextClickIsDoubleClick_) {
    this.fireMouseEvent_(bot.events.EventType.DBLCLICK);
  }
  this.nextClickIsDoubleClick_ = !this.nextClickIsDoubleClick_;
};


/**
 * Given a coordinates (x,y) related to an element, move mouse to (x,y) of the
 * element. The top-left point of the element is (0,0).
 *
 * @param {!Element} element The destination element.
 * @param {!goog.math.Coordinate} coords Mouse position related to the target.
 */
bot.Mouse.prototype.move = function(element, coords) {
  // If the element is interactable at the start of the move, it receives the
  // full event sequence, even if hidden by an element mid sequence.
  var toElemWasInteractable = bot.dom.isInteractable(element);

  var pos = goog.style.getClientPosition(element);
  this.clientXY_.x = coords.x + pos.x;
  this.clientXY_.y = coords.y + pos.y;
  var fromElement = this.getElement();

  if (element != fromElement) {
    // If the window of fromElement is closed, set fromElement to null as a flag
    // to skip the mouseout event and so relatedTarget of the mouseover is null.
    try {
      if (goog.dom.getWindow(goog.dom.getOwnerDocument(fromElement)).closed) {
        fromElement = null;
      }
    } catch (ignore) {
      // Sometimes accessing a window that no longer exists causes an error.
      fromElement = null;
    }

    if (fromElement) {
      // For the first mouse interaction on a page, if the mouse was over the
      // browser window, the browser will pass null as the relatedTarget for the
      // mouseover event. For subsequent interactions, it will pass the
      // last-focused element. Unfortunately, we don't have anywhere to keep the
      // state of which elements have been focused across Mouse instances, so we
      // treat every Mouse initially positioned over the documentElement or body
      // as if it's on a new page. Accordingly, for complex actions (e.g.
      // drag-and-drop), a single Mouse instance should be used for the whole
      // action, to ensure the correct relatedTargets are fired for any events.
      var isRoot = fromElement === bot.getDocument().documentElement ||
                   fromElement === bot.getDocument().body;
      fromElement = (!this.hasEverInteracted_ && isRoot) ? null : fromElement;
      this.fireMouseEvent_(bot.events.EventType.MOUSEOUT, element);
    }
    this.setElement(element);

    // All browsers except IE fire the mouseover before the mousemove.
    if (!goog.userAgent.IE) {
      this.fireMouseEvent_(bot.events.EventType.MOUSEOVER, fromElement, null,
          toElemWasInteractable);
    }
  }

  this.fireMouseEvent_(bot.events.EventType.MOUSEMOVE, null, null,
      toElemWasInteractable);

  // IE fires the mouseover event after the mousemove.
  if (goog.userAgent.IE && element != fromElement) {
    this.fireMouseEvent_(bot.events.EventType.MOUSEOVER, fromElement, null,
        toElemWasInteractable);
  }

  this.nextClickIsDoubleClick_ = false;
};


/**
 * Scrolls the wheel of the mouse by the given number of ticks, where a positive
 * number indicates a downward scroll and a negative is upward scroll.
 *
 * @param {number} ticks Number of ticks to scroll the mouse wheel.
 */
bot.Mouse.prototype.scroll = function(ticks) {
  if (ticks == 0) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Must scroll a non-zero number of ticks.');
  }

  // The wheelDelta value for a single up-tick of the mouse wheel is 120, and
  // a single down-tick is -120. The deltas in pixels (which is only relevant
  // for Firefox) appears to be -57 and 57, respectively.
  var wheelDelta = ticks > 0 ? -120 : 120;
  var pixelDelta = ticks > 0 ? 57 : -57;

  // Browsers fire a separate event (or pair of events in Gecko) for each tick.
  for (var i = 0; i < Math.abs(ticks); i++) {
    this.fireMouseEvent_(bot.events.EventType.MOUSEWHEEL, null, wheelDelta);
    if (goog.userAgent.GECKO) {
      this.fireMouseEvent_(bot.events.EventType.MOUSEPIXELSCROLL, null,
                           pixelDelta);
    }
  }
};


/**
 * A helper function to fire mouse events.
 *
 * @param {bot.events.EventType} type Event type.
 * @param {Element=} opt_related The related element of this event.
 * @param {?number=} opt_wheelDelta The wheel delta value for the event.
 * @param {boolean=} opt_force Whether the event should be fired even if the
 *     element is not interactable.
 * @return {boolean} Whether the event fired successfully or was cancelled.
 * @private
 */
bot.Mouse.prototype.fireMouseEvent_ = function(type, opt_related,
                                               opt_wheelDelta, opt_force) {
  this.hasEverInteracted_ = true;
  if (bot.userAgent.IE_DOC_10) {
    var msPointerEvent = bot.Mouse.MOUSE_EVENT_MAP_[type];
    if (msPointerEvent) {
      // The pointerId for mouse events is always 1 and the mouse event is never
      // fired if the MSPointer event fails.
      if (!this.fireMSPointerEvent(msPointerEvent, this.clientXY_,
          this.getButtonValue_(msPointerEvent),  /* pointerId */ 1,
          MSPointerEvent.MSPOINTER_TYPE_MOUSE, /* isPrimary */ true,
          opt_related, opt_force)) {
        return false;
      }
    }
  }
  return this.fireMouseEvent(type, this.clientXY_,
      this.getButtonValue_(type), opt_related, opt_wheelDelta, opt_force);
};


/**
 * Given an event type and a mouse button, sets the mouse button value used
 * for that event on the current browser. The mouse button value is 0 for any
 * event not covered by bot.Mouse.MOUSE_BUTTON_VALUE_MAP_.
 *
 * @param {bot.events.EventType} eventType Type of mouse event.
 * @return {number} The mouse button ID value to the current browser.
 * @private
*/
bot.Mouse.prototype.getButtonValue_ = function(eventType) {
  if (!(eventType in bot.Mouse.MOUSE_BUTTON_VALUE_MAP_)) {
    return 0;
  }

  var buttonIndex = goog.isNull(this.buttonPressed_) ?
      bot.Mouse.NO_BUTTON_VALUE_INDEX_ : this.buttonPressed_;
  var buttonValue = bot.Mouse.MOUSE_BUTTON_VALUE_MAP_[eventType][buttonIndex];
  if (goog.isNull(buttonValue)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Event does not permit the specified mouse button.');
  }
  return buttonValue;
};


/**
 * Serialize the current state of the mouse.
 * @return {!bot.Mouse.State} The current mouse state.
 */
bot.Mouse.prototype.getState = function() {
  var state = {};
  state.buttonPressed = this.buttonPressed_;
  state.elementPressed = this.elementPressed_;
  state.clientXY = this.clientXY_;
  state.nextClickIsDoubleClick = this.nextClickIsDoubleClick_;
  state.hasEverInteracted = this.hasEverInteracted_;
  state.element = this.getElement();
  return state;
};
// Copyright 2011 Software Freedom Conservancy. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utilities for working with WebDriver response objects.
 * @see: http://code.google.com/p/selenium/wiki/JsonWireProtocol#Responses
 */

goog.provide('bot.response');
goog.provide('bot.response.ResponseObject');

goog.require('bot.Error');
goog.require('bot.ErrorCode');


/**
 * Type definition for a response object, as defined by the JSON wire protocol.
 * @typedef {{status: bot.ErrorCode, value: (*|{message: string})}}
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol#Responses
 */
bot.response.ResponseObject;


/**
 * @param {*} value The value to test.
 * @return {boolean} Whether the given value is a response object.
 */
bot.response.isResponseObject = function(value) {
  return goog.isObject(value) && goog.isNumber(value['status']);
};


/**
 * Creates a new success response object with the provided value.
 * @param {*} value The response value.
 * @return {!bot.response.ResponseObject} The new response object.
 */
bot.response.createResponse = function(value) {
  if (bot.response.isResponseObject(value)) {
    return (/** @type {!bot.response.ResponseObject} */value);
  }
  return {
    'status': bot.ErrorCode.SUCCESS,
    'value': value
  };
};


/**
 * Converts an error value into its JSON representation as defined by the
 * WebDriver wire protocol.
 * @param {(bot.Error|Error|*)} error The error value to convert.
 * @return {!bot.response.ResponseObject} The new response object.
 */
bot.response.createErrorResponse = function(error) {
  if (bot.response.isResponseObject(error)) {
    return (/** @type {!bot.response.ResponseObject} */error);
  }

  var statusCode = error && goog.isNumber(error.code) ? error.code :
      bot.ErrorCode.UNKNOWN_ERROR;
  return {
    'status': (/** @type {bot.ErrorCode} */statusCode),
    'value': {
      'message': (error && error.message || error) + ''
    }
  };
};


/**
 * Checks that a response object does not specify an error as defined by the
 * WebDriver wire protocol. If the response object defines an error, it will
 * be thrown. Otherwise, the response will be returned as is.
 * @param {!bot.response.ResponseObject} responseObj The response object to
 *     check.
 * @return {!bot.response.ResponseObject} The checked response object.
 * @throws {bot.Error} If the response describes an error.
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol#Failed_Commands
 */
bot.response.checkResponse = function(responseObj) {
  var status = responseObj['status'];
  if (status == bot.ErrorCode.SUCCESS) {
    return responseObj;
  }

  // If status is not defined, assume an unknown error.
  status = status || bot.ErrorCode.UNKNOWN_ERROR;

  var value = responseObj['value'];
  if (!value || !goog.isObject(value)) {
    throw new bot.Error(status, value + '');
  }

  throw new bot.Error(status, value['message'] + '');
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview The file contains an abstraction of a touch screen
 * for simulating atomic touchscreen actions.
 */

goog.provide('bot.Touchscreen');

goog.require('bot');
goog.require('bot.Device');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.events.EventType');
goog.require('goog.math.Coordinate');
goog.require('goog.style');



/**
 * A TouchScreen that provides atomic touch actions.  The metaphor
 * for this abstraction is a finger moving above the touchscreen that
 * can press and then release the touchscreen when specified.
 *
 * The touchscreen supports three actions: press, release, and move.
 *
 * @constructor
 * @extends {bot.Device}
 */
bot.Touchscreen = function() {
  goog.base(this);

  /** @private {!goog.math.Coordinate} */
  this.clientXY_ = new goog.math.Coordinate(0, 0);

  /** @private {!goog.math.Coordinate} */
  this.clientXY2_ = new goog.math.Coordinate(0, 0);
};
goog.inherits(bot.Touchscreen, bot.Device);


/** @private {boolean} */
bot.Touchscreen.prototype.hasMovedAfterPress_ = false;


/** @private {number} */
bot.Touchscreen.prototype.touchIdentifier_ = 0;


/** @private {number} */
bot.Touchscreen.prototype.touchIdentifier2_ = 0;


/** @private {number} */
bot.Touchscreen.prototype.touchCounter_ = 1;


/**
 * Press the touch screen.  Pressing before moving results in an exception.
 * Pressing while already pressed also results in an exception.
 *
 * @param {boolean=} opt_press2 Whether or not press the second finger during
 *     the press.  If not defined or false, only the primary finger will be
 *     pressed.
 */
bot.Touchscreen.prototype.press = function(opt_press2) {
  if (this.isPressed()) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot press touchscreen when already pressed.');
  }

  this.hasMovedAfterPress_ = false;
  this.touchIdentifier_ = this.touchCounter_++;
  if (opt_press2) {
    this.touchIdentifier2_ = this.touchCounter_++;
  }

  if (bot.userAgent.IE_DOC_10) {
    this.firePointerEvents_(bot.Touchscreen.fireSinglePressPointer_);
  } else {
    this.fireTouchEvent_(bot.events.EventType.TOUCHSTART);
  }
};


/**
 * Releases an element on a touchscreen.  Releasing an element that is not
 * pressed results in an exception.
 */
bot.Touchscreen.prototype.release = function() {
  if (!this.isPressed()) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Cannot release touchscreen when not already pressed.');
  }

  if (bot.userAgent.IE_DOC_10) {
    this.firePointerEvents_(bot.Touchscreen.fireSingleReleasePointer_);
  } else {
    this.fireTouchReleaseEvents_();
  }
  this.touchIdentifier_ = 0;
  this.touchIdentifier2_ = 0;
};


/**
 * Moves finger along the touchscreen.
 *
 * @param {!Element} element Element that is being pressed.
 * @param {!goog.math.Coordinate} coords Coordinates relative to
 *   currentElement.
 * @param {goog.math.Coordinate=} opt_coords2 Coordinates relative to
 *   currentElement.
 */
bot.Touchscreen.prototype.move = function(element, coords, opt_coords2) {
  // The target element for touch actions is the original element. Hence, the
  // element is set only when the touchscreen is not currently being pressed.
  // The exception is IE10 which fire events on the moved to element.
  if (!this.isPressed() || bot.userAgent.IE_DOC_10) {
    this.setElement(element);
  }

  var pos = goog.style.getClientPosition(element);
  this.clientXY_.x = coords.x + pos.x;
  this.clientXY_.y = coords.y + pos.y;

  if (goog.isDef(opt_coords2)) {
    this.clientXY2_.x = opt_coords2.x + pos.x;
    this.clientXY2_.y = opt_coords2.y + pos.y;
  }

  if (this.isPressed()) {
    this.hasMovedAfterPress_ = true;
    if (bot.userAgent.IE_DOC_10) {
      this.firePointerEvents_(bot.Touchscreen.fireSingleMovePointer_);
    } else {
      this.fireTouchEvent_(bot.events.EventType.TOUCHMOVE);
    }
  }
};


/**
 * Returns whether the touchscreen is currently pressed.
 *
 * @return {boolean} Whether the touchscreen is pressed.
 */
bot.Touchscreen.prototype.isPressed = function() {
  return !!this.touchIdentifier_;
};


/**
 * A helper function to fire touch events.
 *
 * @param {bot.events.EventType} type Event type.
 * @private
 */
bot.Touchscreen.prototype.fireTouchEvent_ = function(type) {
  if (!this.isPressed()) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Should never fire event when touchscreen is not pressed.');
  }
  var touchIdentifier2;
  var coords2;
  if (this.touchIdentifier2_) {
    touchIdentifier2 = this.touchIdentifier2_;
    coords2 = this.clientXY2_;
  }
  this.fireTouchEvent(type, this.touchIdentifier_, this.clientXY_,
                      touchIdentifier2, coords2);
};


/**
 * A helper function to fire touch events that occur on a release.
 *
 * @private
 */
bot.Touchscreen.prototype.fireTouchReleaseEvents_ = function() {
  this.fireTouchEvent_(bot.events.EventType.TOUCHEND);

  // If no movement occurred since press, TouchScreen.Release will fire the
  // legacy mouse events: mousemove, mousedown, mouseup, and click
  // after the touch events have been fired. The click button should be zero
  // and only one mousemove should fire.
  if (!this.hasMovedAfterPress_) {
    this.fireMouseEvent(bot.events.EventType.MOUSEMOVE, this.clientXY_, 0);
    var performFocus = this.fireMouseEvent(bot.events.EventType.MOUSEDOWN,
                                           this.clientXY_, 0);
    // Element gets focus after the mousedown event only if the mousedown was
    // not cancelled.
    if (performFocus) {
      this.focusOnElement();
    }
    this.fireMouseEvent(bot.events.EventType.MOUSEUP, this.clientXY_, 0);

    // Special click logic to follow links and to perform form actions.
    this.clickElement(this.clientXY_, /* button value */ 0);
  }
};


/**
 * A helper function to fire a sequence of Pointer events.
 * @param {function(!bot.Touchscreen, !goog.math.Coordinate, number, boolean)}
 *     fireSinglePointer A function that fires a set of events for one finger.
 * @private
 */
bot.Touchscreen.prototype.firePointerEvents_ = function(fireSinglePointer) {
  fireSinglePointer(this, this.clientXY_, this.touchIdentifier_, true);
  if (this.touchIdentifier2_) {
    fireSinglePointer(this, this.clientXY2_, this.touchIdentifier2_, false);
  }
};


/**
 * A helper function to fire Pointer events related to a press.
 *
 * @param {!bot.Touchscreen} ts A touchscreen object.
 * @param {!goog.math.Coordinate} coords Coordinates relative to
 *   currentElement.
 * @param {number} id The touch identifier.
 * @param {boolean} isPrimary Whether the pointer represents the primary point
 *     of contact.
 * @private
 */
bot.Touchscreen.fireSinglePressPointer_ = function(ts, coords, id, isPrimary) {
  // Fire a mousemove event.
  ts.fireMouseEvent(bot.events.EventType.MOUSEMOVE, coords, 0);

  // Fire a MSPointerOver and mouseover events.
  ts.fireMSPointerEvent(bot.events.EventType.MSPOINTEROVER, coords, 0, id,
      MSPointerEvent.MSPOINTER_TYPE_TOUCH, isPrimary);
  ts.fireMouseEvent(bot.events.EventType.MOUSEOVER, coords, 0);

  // Fire a MSPointerDown and mousedown events.
  ts.fireMSPointerEvent(bot.events.EventType.MSPOINTERDOWN, coords, 0, id,
      MSPointerEvent.MSPOINTER_TYPE_TOUCH, isPrimary);

  // Element gets focus after the mousedown event.
  if (ts.fireMouseEvent(bot.events.EventType.MOUSEDOWN, coords, 0)) {
    ts.focusOnElement();
  }
};


/**
 * A helper function to fire Pointer events related to a release.
 *
 * @param {!bot.Touchscreen} ts A touchscreen object.
 * @param {!goog.math.Coordinate} coords Coordinates relative to
 *   currentElement.
 * @param {number} id The touch identifier.
 * @param {boolean} isPrimary Whether the pointer represents the primary point
 *     of contact.
 * @private
 */
bot.Touchscreen.fireSingleReleasePointer_ = function(ts, coords, id,
                                                     isPrimary) {
  // Fire a MSPointerUp and mouseup events.
  ts.fireMSPointerEvent(bot.events.EventType.MSPOINTERUP, coords, 0, id,
      MSPointerEvent.MSPOINTER_TYPE_TOUCH, isPrimary);
  ts.fireMouseEvent(bot.events.EventType.MOUSEUP, coords, 0);

  // Fire a click.
  ts.clickElement(coords, 0);

  // Fire a MSPointerOut and mouseout events.
  ts.fireMSPointerEvent(bot.events.EventType.MSPOINTEROUT, coords, -1, id,
      MSPointerEvent.MSPOINTER_TYPE_TOUCH, isPrimary);
  ts.fireMouseEvent(bot.events.EventType.MOUSEOUT, coords, 0);
};


/**
 * A helper function to fire Pointer events related to a move.
 *
 * @param {!bot.Touchscreen} ts A touchscreen object.
 * @param {!goog.math.Coordinate} coords Coordinates relative to
 *   currentElement.
 * @param {number} id The touch identifier.
 * @param {boolean} isPrimary Whether the pointer represents the primary point
 *     of contact.
 * @private
 */
bot.Touchscreen.fireSingleMovePointer_ = function(ts, coords, id, isPrimary) {
  // Fire a MSPointerMove and mousemove events.
  ts.fireMSPointerEvent(bot.events.EventType.MSPOINTERMOVE, coords, -1, id,
      MSPointerEvent.MSPOINTER_TYPE_TOUCH, isPrimary);
  ts.fireMouseEvent(bot.events.EventType.MOUSEMOVE, coords, 0);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Similar to goog.userAgent.isVersion, but with support for
 * getting the version information when running in a firefox extension.
 */
goog.provide('bot.userAgent');

goog.require('goog.string');
goog.require('goog.userAgent');
goog.require('goog.userAgent.product');
goog.require('goog.userAgent.product.isVersion');


/**
 * Whether the rendering engine version of the current browser is equal to or
 * greater than the given version. This implementation differs from
 * goog.userAgent.isVersion in the following ways:
 * <ol>
 * <li>in a Firefox extension, tests the engine version through the XUL version
 *     comparator service, because no window.navigator object is available
 * <li>in IE, compares the given version to the current documentMode
 * </ol>
 *
 * @param {string|number} version The version number to check.
 * @return {boolean} Whether the browser engine version is the same or higher
 *     than the given version.
 */
bot.userAgent.isEngineVersion = function(version) {
  if (bot.userAgent.FIREFOX_EXTENSION) {
    return bot.userAgent.FIREFOX_EXTENSION_IS_ENGINE_VERSION_(version);
  } else if (goog.userAgent.IE) {
    return goog.string.compareVersions(
        /** @type {number} */ (goog.userAgent.DOCUMENT_MODE), version) >= 0;
  } else {
    return goog.userAgent.isVersion(version);
  }
};


/**
 * Whether the product version of the current browser is equal to or greater
 * than the given version. This implementation differs from
 * goog.userAgent.product.isVersion in the following ways:
 * <ol>
 * <li>in a Firefox extension, tests the product version through the XUL version
 *     comparator service, because no window.navigator object is available
 * <li>on Android, always compares to the version to the OS version
 * </ol>
 *
 * @param {string|number} version The version number to check.
 * @return {boolean} Whether the browser product version is the same or higher
 *     than the given version.
 */
bot.userAgent.isProductVersion = function(version) {
  if (bot.userAgent.FIREFOX_EXTENSION) {
    return bot.userAgent.FIREFOX_EXTENSION_IS_PRODUCT_VERSION_(version);
  } else if (goog.userAgent.product.ANDROID) {
    return goog.string.compareVersions(
        bot.userAgent.ANDROID_VERSION_, version) >= 0;
  } else {
    return goog.userAgent.product.isVersion(version);
  }
};


/**
 * When we are in a Firefox extension, this is a function that accepts a version
 * and returns whether the version of Gecko we are on is the same or higher
 * than the given version. When we are not in a Firefox extension, this is null.
 *
 * @private {(undefined|function((string|number)): boolean)}
 */
bot.userAgent.FIREFOX_EXTENSION_IS_ENGINE_VERSION_;


/**
 * When we are in a Firefox extension, this is a function that accepts a version
 * and returns whether the version of Firefox we are on is the same or higher
 * than the given version. When we are not in a Firefox extension, this is null.
 *
 * @private {(undefined|function((string|number)): boolean)}
 */
bot.userAgent.FIREFOX_EXTENSION_IS_PRODUCT_VERSION_;


/**
 * Whether we are in a Firefox extension.
 *
 * @const
 * @type {boolean}
 */
bot.userAgent.FIREFOX_EXTENSION = (function() {
  // False if this browser is not a Gecko browser.
  if (!goog.userAgent.GECKO) {
    return false;
  }

  // False if this code isn't running in an extension.
  var Components = goog.global.Components;
  if (!Components) {
    return false;
  }
  try {
    if (!Components['classes']) {
      return false;
    }
  } catch (e) {
    return false;
  }

  // Populate the version checker functions.
  var cc = Components['classes'];
  var ci = Components['interfaces'];
  var versionComparator = cc['@mozilla.org/xpcom/version-comparator;1'][
      'getService'](ci['nsIVersionComparator']);
  var appInfo = cc['@mozilla.org/xre/app-info;1']['getService'](
      ci['nsIXULAppInfo']);
  var geckoVersion = appInfo['platformVersion'];
  var firefoxVersion = appInfo['version'];

  bot.userAgent.FIREFOX_EXTENSION_IS_ENGINE_VERSION_ = function(version) {
    return versionComparator.compare(geckoVersion, '' + version) >= 0;
  };
  bot.userAgent.FIREFOX_EXTENSION_IS_PRODUCT_VERSION_ = function(version) {
    return versionComparator.compare(firefoxVersion, '' + version) >= 0;
  };

  return true;
})();


/**
 * Whether we are on IOS.
 *
 * @const
 * @type {boolean}
 */
bot.userAgent.IOS = goog.userAgent.product.IPAD ||
                    goog.userAgent.product.IPHONE;


/**
 * Whether we are on a mobile browser.
 *
 * @const
 * @type {boolean}
 */
bot.userAgent.MOBILE = bot.userAgent.IOS || goog.userAgent.product.ANDROID;


/**
 * Android Operating System Version.
 *
 * @const
 * @private {string}
 */
bot.userAgent.ANDROID_VERSION_ = (function() {
  if (goog.userAgent.product.ANDROID) {
    var userAgentString = goog.userAgent.getUserAgentString();
    var match = /Android\s+([0-9\.]+)/.exec(userAgentString);
    return match ? match[1] : '0';
  } else {
    return '0';
  }
})();


/**
 * Whether the current document is IE in a documentMode older than 8.
 * @type {boolean}
 * @const
 */
bot.userAgent.IE_DOC_PRE8 = goog.userAgent.IE &&
    !goog.userAgent.isDocumentMode(8);


/**
 * Whether the current document is IE in IE9 (or newer) standards mode.
 * @type {boolean}
 * @const
 */
bot.userAgent.IE_DOC_9 = goog.userAgent.isDocumentMode(9);


/**
 * Whether the current document is IE in a documentMode older than 9.
 * @type {boolean}
 * @const
 */
bot.userAgent.IE_DOC_PRE9 = goog.userAgent.IE &&
    !goog.userAgent.isDocumentMode(9);


/**
 * Whether the current document is IE in IE10 (or newer) standards mode.
 * @type {boolean}
 * @const
 */
bot.userAgent.IE_DOC_10 = goog.userAgent.isDocumentMode(10);


/**
 * Whether the current document is IE in a documentMode older than 10.
 * @type {boolean}
 * @const
 */
bot.userAgent.IE_DOC_PRE10 = goog.userAgent.IE &&
    !goog.userAgent.isDocumentMode(10);


/**
 * Whether the current browser is Android pre-gingerbread.
 * @type {boolean}
 * @const
 */
bot.userAgent.ANDROID_PRE_GINGERBREAD = goog.userAgent.product.ANDROID &&
    !bot.userAgent.isProductVersion(2.3);

// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms for simulating user actions against the browser window.
 */

goog.provide('bot.window');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.userAgent');
goog.require('goog.math.Coordinate');
goog.require('goog.math.Size');
goog.require('goog.userAgent');


/**
 * Whether the value of history.length includes a newly loaded page. If not,
 * after a new page load history.length is the number of pages that have loaded,
 * minus 1, but becomes the total number of pages on a subsequent back() call.
 *
 * @const
 * @private {boolean}
 */
bot.window.HISTORY_LENGTH_INCLUDES_NEW_PAGE_ = !goog.userAgent.IE &&
    !goog.userAgent.OPERA;


/**
 * Whether value of history.length includes the pages ahead of the current one
 * in the history. If not, history.length equals the number of prior pages.
 * Here is the WebKit bug for this behavior that was fixed by version 533:
 * https://bugs.webkit.org/show_bug.cgi?id=24472
 *
 * @const
 * @private {boolean}
 */
bot.window.HISTORY_LENGTH_INCLUDES_FORWARD_PAGES_ = !goog.userAgent.OPERA &&
    (!goog.userAgent.WEBKIT || bot.userAgent.isEngineVersion('533'));


/**
 * Go back in the browser history. The number of pages to go back can
 * optionally be specified and defaults to 1.
 *
 * @param {number=} opt_numPages Number of pages to go back.
 */
bot.window.back = function(opt_numPages) {
  // Relax the upper bound by one for browsers that do not count
  // newly loaded pages towards the value of window.history.length.
  var maxPages = bot.window.HISTORY_LENGTH_INCLUDES_NEW_PAGE_ ?
      bot.getWindow().history.length - 1 : bot.getWindow().history.length;
  var numPages = bot.window.checkNumPages_(maxPages, opt_numPages);
  bot.getWindow().history.go(-numPages);
};


/**
 * Go forward in the browser history. The number of pages to go forward can
 * optionally be specified and defaults to 1.
 *
 * @param {number=} opt_numPages Number of pages to go forward.
 */
bot.window.forward = function(opt_numPages) {
  // Do not check the upper bound (use null for infinity) for browsers that
  // do not count forward pages towards the value of window.history.length.
  var maxPages = bot.window.HISTORY_LENGTH_INCLUDES_FORWARD_PAGES_ ?
      bot.getWindow().history.length - 1 : null;
  var numPages = bot.window.checkNumPages_(maxPages, opt_numPages);
  bot.getWindow().history.go(numPages);
};


/**
 * @param {?number} maxPages Upper bound on number of pages; null for infinity.
 * @param {number=} opt_numPages Number of pages to move in history.
 * @return {number} Correct number of pages to move in history.
 * @private
 */
bot.window.checkNumPages_ = function(maxPages, opt_numPages) {
  var numPages = goog.isDef(opt_numPages) ? opt_numPages : 1;
  if (numPages <= 0) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'number of pages must be positive');
  }
  if (maxPages !== null && numPages > maxPages) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'number of pages must be less than the length of the browser history');
  }
  return numPages;
};


/**
 * Determine the size of the window that a user could interact with. This will
 * be the greatest of document.body.(width|scrollWidth), the same for
 * document.documentElement or the size of the viewport.
 *
 * @param {!Window=} opt_win Window to determine the size of. Defaults to
 *   bot.getWindow().
 * @return {!goog.math.Size} The calculated size.
 */
bot.window.getInteractableSize = function(opt_win) {
  var win = opt_win || bot.getWindow();
  var doc = win.document;
  var elem = doc.documentElement;
  var body = doc.body;
  if (!body) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'No BODY element present');
  }

  var widths = [
    elem.clientWidth, elem.scrollWidth, elem.offsetWidth,
    body.scrollWidth, body.offsetWidth
  ];
  var heights = [
    elem.clientHeight, elem.scrollHeight, elem.offsetHeight,
    body.scrollHeight, body.offsetHeight
  ];

  var width = Math.max.apply(null, widths);
  var height = Math.max.apply(null, heights);

  return new goog.math.Size(width, height);
};


/**
 * Determine the outer size of the window.
 *
 * @param {!Window=} opt_win Window to determine the size of. Defaults to
 *   bot.getWindow().
 * @return {!goog.math.Size} The calculated size.
 */
bot.window.getSize = function(opt_win) {
  var win = opt_win || bot.getWindow();

  var width = win.outerWidth;
  var height = win.outerHeight;

  return new goog.math.Size(width, height);
};


/**
 * Set the outer size of the window.
 *
 * @param {!goog.math.Size} size The new window size.
 * @param {!Window=} opt_win Window to determine the size of. Defaults to
 *   bot.getWindow().
 */
bot.window.setSize = function(size, opt_win) {
  var win = opt_win || bot.getWindow();

  win.resizeTo(size.width, size.height);
};


/**
 * Get the position of the window.
 *
 * @param {!Window=} opt_win Window to determine the position of. Defaults to
 *   bot.getWindow().
 * @return {!goog.math.Coordinate} The position of the window.
 */
bot.window.getPosition = function(opt_win) {
  var win = opt_win || bot.getWindow();
  var x, y;

  if (goog.userAgent.IE) {
    x = win.screenLeft;
    y = win.screenTop;
  } else {
    x = win.screenX;
    y = win.screenY;
  }

  return new goog.math.Coordinate(x, y);
};


/**
 * Set the position of the window.
 *
 * @param {!goog.math.Coordinate} targetPosition The target position.
 * @param {!Window=} opt_win Window to set the position of. Defaults to
 *   bot.getWindow().
 */
bot.window.setPosition = function(targetPosition, opt_win) {
  var win = opt_win || bot.getWindow();
  win.moveTo(targetPosition.x, targetPosition.y);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('bot.locators.className');

goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.DomHelper');
goog.require('goog.string');


/**
 * Tests whether the standardized W3C Selectors API are available on an
 * element.
 * @param {!(Document|Element)} root The document or element to test for CSS
 *     selector support.
 * @return {boolean} Whether or not the root supports query selector APIs.
 * @see http://www.w3.org/TR/selectors-api/
 * @private
 */
bot.locators.className.canUseQuerySelector_ = function(root) {
  return !!(root.querySelectorAll && root.querySelector);
};


/**
 * Find an element by its class name.
 * @param {string} target The class name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.className.single = function(target, root) {
  if (!target) {
    throw Error('No class name specified');
  }

  target = goog.string.trim(target);
  if (target.split(/\s+/).length > 1) {
    throw Error('Compound class names not permitted');
  }

  // Closure will not properly escape class names that contain a '.' when using
  // the native selectors API, so we have to handle this ourselves.
  if (bot.locators.className.canUseQuerySelector_(root)) {
    return root.querySelector('.' + target.replace(/\./g, '\\.')) || null;
  }
  var elements = goog.dom.getDomHelper(root).getElementsByTagNameAndClass(
      /*tagName=*/'*', /*className=*/target, root);
  return elements.length ? elements[0] : null;
};


/**
 * Find an element by its class name.
 * @param {string} target The class name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {!goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.className.many = function(target, root) {
  if (!target) {
    throw Error('No class name specified');
  }

  target = goog.string.trim(target);
  if (target.split(/\s+/).length > 1) {
    throw Error('Compound class names not permitted');
  }

  // Closure will not properly escape class names that contain a '.' when using
  // the native selectors API, so we have to handle this ourselves.
  if (bot.locators.className.canUseQuerySelector_(root)) {
    return root.querySelectorAll('.' + target.replace(/\./g, '\\.'));
  }
  return goog.dom.getDomHelper(root).getElementsByTagNameAndClass(
      /*tagName=*/'*', /*className=*/target, root);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// TODO(simon): Add support for using sizzle to locate elements

goog.provide('bot.locators.css');

goog.require('bot.userAgent');
goog.require('goog.dom.NodeType');
goog.require('goog.string');
goog.require('goog.userAgent');


/**
 * Find an element by using a CSS selector
 *
 * @param {string} target The selector to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.css.single = function(target, root) {
  if (!goog.isFunction(root['querySelector']) &&
      // IE8 in non-compatibility mode reports querySelector as an object.
      goog.userAgent.IE && bot.userAgent.isEngineVersion(8) &&
      !goog.isObject(root['querySelector'])) {
    throw Error('CSS selection is not supported');
  }

  if (!target) {
    throw Error('No selector specified');
  }

  target = goog.string.trim(target);

  var element = root.querySelector(target);

  return element && element.nodeType == goog.dom.NodeType.ELEMENT ?
      (/**@type {Element}*/element) : null;
};


/**
 * Find all elements matching a CSS selector.
 *
 * @param {string} target The selector to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {!goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.css.many = function(target, root) {
  if (!goog.isFunction(root['querySelectorAll']) &&
      // IE8 in non-compatibility mode reports querySelector as an object.
      goog.userAgent.IE && bot.userAgent.isEngineVersion(8) &&
      !goog.isObject(root['querySelector'])) {
    throw Error('CSS selection is not supported');
  }

  if (!target) {
    throw Error('No selector specified');
  }

  target = goog.string.trim(target);

  return root.querySelectorAll(target);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('bot.locators.id');

goog.require('bot.dom');
goog.require('goog.array');
goog.require('goog.dom');


/**
 * Find an element by using the value of the ID attribute.
 * @param {string} target The id to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.id.single = function(target, root) {
  var dom = goog.dom.getDomHelper(root);

  var e = dom.getElement(target);
  if (!e) {
    return null;
  }

  // On IE getting by ID returns the first match by id _or_ name.
  if (bot.dom.getAttribute(e, 'id') == target && goog.dom.contains(root, e)) {
    return e;
  }

  var elements = dom.getElementsByTagNameAndClass('*');
  var element = goog.array.find(elements, function(element) {
    return bot.dom.getAttribute(element, 'id') == target &&
        goog.dom.contains(root, element);
  });
  return (/**@type{Element}*/element);
};


/**
 * Find many elements by using the value of the ID attribute.
 * @param {string} target The id to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {!goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.id.many = function(target, root) {
  var dom = goog.dom.getDomHelper(root);
  var elements = dom.getElementsByTagNameAndClass('*', null, root);
  return goog.array.filter(elements, function(e) {
    return bot.dom.getAttribute(e, 'id') == target;
  });
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('bot.locators.linkText');
goog.provide('bot.locators.partialLinkText');

goog.require('bot');
goog.require('bot.dom');
goog.require('bot.locators.css');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.DomHelper');


/**
 * Find an element by using the text value of a link
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @param {boolean} opt_isPartial Whether the link text needs to be matched
 *     only partially.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 * @private
 */
bot.locators.linkText.single_ = function(target, root, opt_isPartial) {
  var elements;
  try {
    elements = bot.locators.css.many('a', root);
  } catch (e) {
    // Old versions of browsers don't support CSS. They won't have XHTML
    // support. Sorry.
    elements = goog.dom.getDomHelper(root).getElementsByTagNameAndClass(
        goog.dom.TagName.A, /*className=*/null, root);
  }

  var element = goog.array.find(elements, function(element) {
    var text = bot.dom.getVisibleText(element);
    return (opt_isPartial && text.indexOf(target) != -1) || text == target;
  });
  return (/**@type{Element}*/element);
};


/**
 * Find many elements by using the value of the link text
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @param {boolean} opt_isPartial Whether the link text needs to be matched
 *     only partially.
 * @return {goog.array.ArrayLike} All matching elements, or an empty list.
 * @private
 */
bot.locators.linkText.many_ = function(target, root, opt_isPartial) {
  var elements;
  try {
    elements = bot.locators.css.many('a', root);
  } catch (e) {
    // Old versions of browsers don't support CSS. They won't have XHTML
    // support. Sorry.
    elements = goog.dom.getDomHelper(root).getElementsByTagNameAndClass(
        goog.dom.TagName.A, /*className=*/null, root);
  }

  return goog.array.filter(elements, function(element) {
    var text = bot.dom.getVisibleText(element);
    return (opt_isPartial && text.indexOf(target) != -1) || text == target;
  });
};


/**
 * Find an element by using the text value of a link
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.linkText.single = function(target, root) {
  return bot.locators.linkText.single_(target, root, false);
};


/**
 * Find many elements by using the value of the link text
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.linkText.many = function(target, root) {
  return bot.locators.linkText.many_(target, root, false);
};


/**
 * Find an element by using part of the text value of a link.
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.partialLinkText.single = function(target, root) {
  return bot.locators.linkText.single_(target, root, true);
};


/**
 * Find many elements by using part of the value of the link text.
 * @param {string} target The link text to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.partialLinkText.many = function(target, root) {
  return bot.locators.linkText.many_(target, root, true);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Element locator functions.
 */


goog.provide('bot.locators');

goog.require('bot');
goog.require('bot.locators.className');
goog.require('bot.locators.css');
goog.require('bot.locators.id');
goog.require('bot.locators.linkText');
goog.require('bot.locators.name');
goog.require('bot.locators.partialLinkText');
goog.require('bot.locators.tagName');
goog.require('bot.locators.xpath');
goog.require('goog.array');  // for the goog.array.ArrayLike typedef
goog.require('goog.object');


/**
 * @typedef {{single:function(string,!(Document|Element)):Element,
 *     many:function(string,!(Document|Element)):!goog.array.ArrayLike}}
 */
bot.locators.strategy;


/**
 * Known element location strategies. The returned objects have two
 * methods on them, "single" and "many", for locating a single element
 * or multiple elements, respectively.
 *
 * Note that the versions with spaces are synonyms for those without spaces,
 * and are specified at:
 * https://code.google.com/p/selenium/wiki/JsonWireProtocol
 *
 * @const
 * @private {Object.<string,bot.locators.strategy>}
 */
bot.locators.STRATEGIES_ = {
  'className': bot.locators.className,
  'class name': bot.locators.className,

  'css': bot.locators.css,
  'css selector': bot.locators.css,

  'id': bot.locators.id,

  'linkText': bot.locators.linkText,
  'link text': bot.locators.linkText,

  'name': bot.locators.name,

  'partialLinkText': bot.locators.partialLinkText,
  'partial link text': bot.locators.partialLinkText,

  'tagName': bot.locators.tagName,
  'tag name': bot.locators.tagName,

  'xpath': bot.locators.xpath
};


/**
 * Add or override an existing strategy for locating elements.
 *
 * @param {string} name The name of the strategy.
 * @param {!bot.locators.strategy} strategy The strategy to use.
 */
bot.locators.add = function(name, strategy) {
  bot.locators.STRATEGIES_[name] = strategy;
};


/**
 * Returns one key from the object map that is not present in the
 * Object.prototype, if any exists.
 *
 * @param {Object} target The object to pick a key from.
 * @return {string?} The key or null if the object is empty.
 */
bot.locators.getOnlyKey = function(target) {
  for (var k in target) {
    if (target.hasOwnProperty(k)) {
      return k;
    }
  }
  return null;
};


/**
 * Find the first element in the DOM matching the target. The target
 * object should have a single key, the name of which determines the
 * locator strategy and the value of which gives the value to be
 * searched for. For example {id: 'foo'} indicates that the first
 * element on the DOM with the ID 'foo' should be returned.
 *
 * @param {!Object} target The selector to search for.
 * @param {(Document|Element)=} opt_root The node from which to start the
 *     search. If not specified, will use {@code document} as the root.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.findElement = function(target, opt_root) {
  var key = bot.locators.getOnlyKey(target);

  if (key) {
    var strategy = bot.locators.STRATEGIES_[key];
    if (strategy && goog.isFunction(strategy.single)) {
      var root = opt_root || bot.getDocument();
      return strategy.single(target[key], root);
    }
  }
  throw Error('Unsupported locator strategy: ' + key);
};


/**
 * Find all elements in the DOM matching the target. The target object
 * should have a single key, the name of which determines the locator
 * strategy and the value of which gives the value to be searched
 * for. For example {name: 'foo'} indicates that all elements with the
 * 'name' attribute equal to 'foo' should be returned.
 *
 * @param {!Object} target The selector to search for.
 * @param {(Document|Element)=} opt_root The node from which to start the
 *     search. If not specified, will use {@code document} as the root.
 * @return {!goog.array.ArrayLike.<Element>} All matching elements found in the
 *     DOM.
 */
bot.locators.findElements = function(target, opt_root) {
  var key = bot.locators.getOnlyKey(target);

  if (key) {
    var strategy = bot.locators.STRATEGIES_[key];
    if (strategy && goog.isFunction(strategy.many)) {
      var root = opt_root || bot.getDocument();
      return strategy.many(target[key], root);
    }
  }
  throw Error('Unsupported locator strategy: ' + key);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('bot.locators.name');

goog.require('bot.dom');
goog.require('goog.array');
goog.require('goog.dom');


/**
 * Find an element by the value of the name attribute
 *
 * @param {string} target The name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.name.single = function(target, root) {
  var dom = goog.dom.getDomHelper(root);
  var allElements = dom.getElementsByTagNameAndClass('*', null, root);
  var element = goog.array.find(allElements, function(element) {
    return bot.dom.getAttribute(element, 'name') == target;
  });
  return (/**@type{Element}*/element);
};


/**
 * Find all elements by the value of the name attribute
 *
 * @param {string} target The name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {!goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.name.many = function(target, root) {
  var dom = goog.dom.getDomHelper(root);
  var allElements = dom.getElementsByTagNameAndClass('*', null, root);
  return goog.array.filter(allElements, function(element) {
    return bot.dom.getAttribute(element, 'name') == target;
  });
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('bot.locators.tagName');

goog.require('goog.array');


/**
 * Find an element by its tag name.
 * @param {string} target The tag name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.tagName.single = function(target, root) {
  return root.getElementsByTagName(target)[0] || null;
};


/**
 * Find all elements with a given tag name.
 * @param {string} target The tag name to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.tagName.many = function(target, root) {
  return root.getElementsByTagName(target);
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Functions to locate elements by XPath.
 *
 * <p>The locator implementations below differ from the Closure functions
 * goog.dom.xml.{selectSingleNode,selectNodes} in three important ways:
 * <ol>
 * <li>they do not refer to "document" which is undefined in the context of a
 * Firefox extension;
 * <li> they use a default NsResolver for browsers that do not provide
 * document.createNSResolver (e.g. Android); and
 * <li> they prefer document.evaluate to node.{selectSingleNode,selectNodes}
 * because the latter silently return nothing when the xpath resolves to a
 * non-Node type, limiting the error-checking the implementation can provide.
 * </ol>
 *
 * TODO(user): Add support for browsers without native xpath
 */

goog.provide('bot.locators.xpath');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.dom.NodeType');
goog.require('goog.userAgent');
goog.require('goog.userAgent.product');
goog.require('wgxpath');


/**
 * XPathResult enum values. These are defined separately since
 * the context running this script may not support the XPathResult
 * type.
 * @enum {number}
 * @see http://www.w3.org/TR/DOM-Level-3-XPath/xpath.html#XPathResult
 * @private
 */
// TODO(berrada): Move this enum back to bot.locators.xpath namespace.
// The problem is that we alias bot.locators.xpath in locators.js, while
// we set the flag --collapse_properties (http://goo.gl/5W6cP).
// The compiler should have thrown the error anyways, it's a bug that it fails
// only when introducing this enum.
// Solution: remove --collapase_properties from the js_binary rule or
// use goog.exportSymbol to export the public methods and get rid of the alias.
bot.locators.XPathResult_ = {
  ORDERED_NODE_SNAPSHOT_TYPE: 7,
  FIRST_ORDERED_NODE_TYPE: 9
};


/**
 * Default XPath namespace resolver.
 * @private
 */
bot.locators.xpath.DEFAULT_RESOLVER_ = (function() {
  var namespaces = {svg: 'http://www.w3.org/2000/svg'};
  return function(prefix) {
    return namespaces[prefix] || null;
  };
})();


/**
 * Evaluates an XPath expression using a W3 XPathEvaluator.
 * @param {!(Document|Element)} node The document or element to perform the
 *     search under.
 * @param {string} path The xpath to search for.
 * @param {!bot.locators.XPathResult_} resultType The desired result type.
 * @return {XPathResult} The XPathResult or null if the root's ownerDocument
 *     does not support XPathEvaluators.
 * @private
 * @see http://www.w3.org/TR/DOM-Level-3-XPath/xpath.html#XPathEvaluator-evaluate
 */
bot.locators.xpath.evaluate_ = function(node, path, resultType) {
  var doc = goog.dom.getOwnerDocument(node);

  // Let the wgxpath library be compiled away unless we are on IE or Android.
  // TODO(gdennis): Restrict this to just IE when we drop support for Froyo.
  if (goog.userAgent.IE || goog.userAgent.product.ANDROID) {
    wgxpath.install(goog.dom.getWindow(doc));
  }

  try {
    var resolver = doc.createNSResolver ?
        doc.createNSResolver(doc.documentElement) :
        bot.locators.xpath.DEFAULT_RESOLVER_;
    if (goog.userAgent.IE && !goog.userAgent.isVersion(7)) {
      // IE6, and only IE6, has an issue where calling a custom function
      // directly attached to the document object does not correctly propagate
      // thrown errors. So in that case *only* we will use apply().
      return doc.evaluate.call(doc, path, node, resolver, resultType, null);
    } else {
      return doc.evaluate(path, node, resolver, resultType, null);
    }
  } catch (ex) {
    // The Firefox XPath evaluator can throw an exception if the document is
    // queried while it's in the midst of reloading, so we ignore it. In all
    // other cases, we assume an invalid xpath has caused the exception.
    if (!(goog.userAgent.GECKO && ex.name == 'NS_ERROR_ILLEGAL_VALUE')) {
      throw new bot.Error(bot.ErrorCode.INVALID_SELECTOR_ERROR,
          'Unable to locate an element with the xpath expression ' + path +
          ' because of the following error:\n' + ex);
    }
  }
};


/**
 * @param {Node|undefined} node Node to check whether it is an Element.
 * @param {string} path XPath expression to include in the error message.
 * @private
 */
bot.locators.xpath.checkElement_ = function(node, path) {
  if (!node || node.nodeType != goog.dom.NodeType.ELEMENT) {
    throw new bot.Error(bot.ErrorCode.INVALID_SELECTOR_ERROR,
        'The result of the xpath expression "' + path +
        '" is: ' + node + '. It should be an element.');
  }
};


/**
 * Find an element by using an xpath expression
 * @param {string} target The xpath to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {Element} The first matching element found in the DOM, or null if no
 *     such element could be found.
 */
bot.locators.xpath.single = function(target, root) {

  function selectSingleNode() {
    var result = bot.locators.xpath.evaluate_(root, target,
        bot.locators.XPathResult_.FIRST_ORDERED_NODE_TYPE);
    if (result) {
      var node = result.singleNodeValue;
      // On Opera, a singleNodeValue of undefined indicates a type error, while
      // other browsers may use it to indicate something has not been found.
      return goog.userAgent.OPERA ? node : (node || null);
    } else if (root.selectSingleNode) {
      var doc = goog.dom.getOwnerDocument(root);
      if (doc.setProperty) {
        doc.setProperty('SelectionLanguage', 'XPath');
      }
      return root.selectSingleNode(target);
    }
    return null;
  }

  var node = selectSingleNode();
  if (!goog.isNull(node)) {
    bot.locators.xpath.checkElement_(node, target);
  }
  return (/** @type {Element} */node);
};


/**
 * Find elements by using an xpath expression
 * @param {string} target The xpath to search for.
 * @param {!(Document|Element)} root The document or element to perform the
 *     search under.
 * @return {!goog.array.ArrayLike} All matching elements, or an empty list.
 */
bot.locators.xpath.many = function(target, root) {

  function selectNodes() {
    var result = bot.locators.xpath.evaluate_(root, target,
        bot.locators.XPathResult_.ORDERED_NODE_SNAPSHOT_TYPE);
    if (result) {
      var count = result.snapshotLength;
      // On Opera, if the XPath evaluates to a non-Node value, snapshotLength
      // will be undefined and the result empty, so fail immediately.
      if (goog.userAgent.OPERA && !goog.isDef(count)) {
        bot.locators.xpath.checkElement_(null, target);
      }
      var results = [];
      for (var i = 0; i < count; ++i) {
        results.push(result.snapshotItem(i));
      }
      return results;
    } else if (root.selectNodes) {
      var doc = goog.dom.getOwnerDocument(root);
      if (doc.setProperty) {
        doc.setProperty('SelectionLanguage', 'XPath');
      }
      return root.selectNodes(target);
    }
    return [];
  }

  var nodes = selectNodes();
  goog.array.forEach(nodes, function(n) {
    bot.locators.xpath.checkElement_(n, target);
  });
  return (/** @type {!goog.array.ArrayLike} */nodes);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atom to access application cache status.
 *
 */

goog.provide('bot.appcache');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.html5');


/**
 * Returns the current state of the application cache.
 *
 * @param {Window=} opt_window The window object whose cache is checked;
 *     defaults to the main window.
 * @return {number} The state.
 */
bot.appcache.getStatus = function(opt_window) {
  var win = opt_window || bot.getWindow();

  if (bot.html5.isSupported(bot.html5.API.APPCACHE, win)) {
    return win.applicationCache.status;
  } else {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Undefined application cache');
  }
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms to check to connection state of a browser.
 *
 */

goog.provide('bot.connection');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.html5');


/**
 * @return {boolean} Whether the browser currently has an internet
 *     connection.
 */
bot.connection.isOnline = function() {

  if (bot.html5.isSupported(bot.html5.API.BROWSER_CONNECTION)) {
    var win = bot.getWindow();
    return win.navigator.onLine;
  } else {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
        'Undefined browser connection state');
  }
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms for executing SQL queries on web client database.
 *
 */

goog.provide('bot.storage.database');
goog.provide('bot.storage.database.ResultSet');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');


/**
 * Opens the database to access its contents. This function will create the
 * database if it does not exist. For details,
 * @see http://www.w3.org/TR/webdatabase/#databases
 *
 * @param {string} databaseName The name of the database.
 * @param {string=} opt_version The expected database version to be opened;
 *     defaults to the empty string.
 * @param {string=} opt_displayName The name to be displayed to the user;
 *     defaults to the databaseName.
 * @param {number=} opt_size The estimated initial quota size of the database;
 *     default value is 5MB.
 * @param {Window=} opt_window The window associated with the database;
 *     defaults to the main window.
 * @return {Database} The object to access the web database.
 *
 */
bot.storage.database.openOrCreate = function(databaseName, opt_version,
    opt_displayName, opt_size, opt_window) {
  var version = opt_version || '';
  var displayName = opt_displayName || (databaseName + 'name');
  var size = opt_size || 5 * 1024 * 1024;
  var win = opt_window || bot.getWindow();
  var db;

  return win.openDatabase(databaseName, version, displayName, size);
};


/**
 * It executes a single SQL query on a given web database storage.
 *
 * @param {string} databaseName The name of the database.
 * @param {string} query The SQL statement.
 * @param {Array.<*>} args Arguments needed for the SQL statement.
 * @param {!function(!SQLTransaction, !bot.storage.database.ResultSet)}
 *     queryResultCallback Callback function to be invoked on successful query
 *     statement execution.
 * @param {!function(SQLError)} txErrorCallback
 *     Callback function to be invoked on transaction (commit) failure.
 * @param {!function()=} opt_txSuccessCallback
 *     Callback function to be invoked on successful transaction execution.
 * @param {function(!SQLTransaction, !SQLError)=} opt_queryErrorCallback
 *     Callback function to be invoked on successful query statement execution.
 * @see http://www.w3.org/TR/webdatabase/#executing-sql-statements
 */
bot.storage.database.executeSql = function(databaseName, query, args,
    queryResultCallback, txErrorCallback, opt_txSuccessCallback,
    opt_queryErrorCallback) {

  var db;

  try {
    db = bot.storage.database.openOrCreate(databaseName);
  } catch (e) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR, e.message);
  }

  var queryCallback = function(tx, result) {
    var wrappedResult = new bot.storage.database.ResultSet(result);
    queryResultCallback(tx, wrappedResult);
  }

  var transactionCallback = function(tx) {
    tx.executeSql(query, args, queryCallback, opt_queryErrorCallback);
  }

  db.transaction(transactionCallback, txErrorCallback,
      opt_txSuccessCallback);
};



/**
 * A wrapper of the SQLResultSet object returned by the SQL statement.
 *
 * @param {!SQLResultSet} sqlResultSet The original SQLResultSet object.
 * @constructor
 */
bot.storage.database.ResultSet = function(sqlResultSet) {

  /**
   * The database rows retuned from the SQL query.
   * @type {!Array.<*>}
   */
  this.rows = [];
  for (var i = 0; i < sqlResultSet.rows.length; i++) {
    this.rows[i] = sqlResultSet.rows.item(i);
  }

  /**
   * The number of rows that were changed by the SQL statement
   * @type {number}
   */
  this.rowsAffected = sqlResultSet.rowsAffected;

  /**
   * The row ID of the row that the SQLResultSet object's SQL statement
   * inserted into the database, if the statement inserted a row; else
   * it is assigned to -1. Originally, accessing insertId attribute of
   * a SQLResultSet object returns the exception INVALID_ACCESS_ERR
   * if no rows are inserted.
   * @type {number}
   */
  this.insertId = -1;
  try {
    this.insertId = sqlResultSet.insertId;
  } catch (error) {
    // If accessing sqlResultSet.insertId results in INVALID_ACCESS_ERR
    // exception, this.insertId will be assigned to -1.
  }
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Helper function to determine which HTML5 features are
 * supported by browsers..
 */

goog.provide('bot.html5');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.userAgent');
goog.require('goog.userAgent');
goog.require('goog.userAgent.product');


/**
 * Identifier for supported HTML5 API in Webdriver.
 *
 * @enum {string}
 */
bot.html5.API = {
  APPCACHE: 'appcache',
  BROWSER_CONNECTION: 'browser_connection',
  DATABASE: 'database',
  GEOLOCATION: 'location',
  LOCAL_STORAGE: 'local_storage',
  SESSION_STORAGE: 'session_storage',
  VIDEO: 'video',
  AUDIO: 'audio',
  CANVAS: 'canvas'
};


/**
 * True if the current browser is IE8.
 *
 * @private {boolean}
 * @const
 */
bot.html5.IS_IE8_ = goog.userAgent.IE &&
    bot.userAgent.isEngineVersion(8) && !bot.userAgent.isEngineVersion(9);


/**
 * True if the current browser is Safari 4.
 *
 * @private {boolean}
 * @const
 */
bot.html5.IS_SAFARI4_ = goog.userAgent.product.SAFARI &&
    bot.userAgent.isProductVersion(4) && !bot.userAgent.isProductVersion(5);


/**
 * True if the browser is Android 2.2 (Froyo).
 *
 * @private {boolean}
 * @const
 */
bot.html5.IS_ANDROID_FROYO_ = goog.userAgent.product.ANDROID &&
    bot.userAgent.isProductVersion(2.2) && !bot.userAgent.isProductVersion(2.3);


/**
 * True if the current browser is Safari 5 on Windows.
 *
 * @private {boolean}
 * @const
 */
bot.html5.IS_SAFARI_WINDOWS_ = goog.userAgent.WINDOWS &&
    goog.userAgent.product.SAFARI &&
    (bot.userAgent.isProductVersion(4)) &&
    !bot.userAgent.isProductVersion(6);


/**
 * Checks if the browser supports an HTML5 feature.
 *
 * @param {bot.html5.API} api HTML5 API identifier.
 * @param {!Window=} opt_window The window to be accessed;
 *     defaults to the main window.
 * @return {boolean} Whether the browser supports the feature.
 */
bot.html5.isSupported = function(api, opt_window) {
  var win = opt_window || bot.getWindow();

  switch (api) {
    case bot.html5.API.APPCACHE:
      // IE8 does not support application cache, though the APIs exist.
      if (bot.html5.IS_IE8_) {
        return false;
      }
      return goog.isDefAndNotNull(win.applicationCache);

    case bot.html5.API.BROWSER_CONNECTION:
      return goog.isDefAndNotNull(win.navigator) &&
          goog.isDefAndNotNull(win.navigator.onLine);

    case bot.html5.API.DATABASE:
      // Safari4 database API does not allow writes.
      if (bot.html5.IS_SAFARI4_) {
        return false;
      }
      // Android Froyo does not support database, though the APIs exist.
      if (bot.html5.IS_ANDROID_FROYO_) {
        return false;
      }
      return goog.isDefAndNotNull(win.openDatabase);

    case bot.html5.API.GEOLOCATION:
      // Safari 4,5 on Windows do not support geolocation, see:
      // https://discussions.apple.com/thread/3547900
      if (bot.html5.IS_SAFARI_WINDOWS_) {
        return false;
      }
      return goog.isDefAndNotNull(win.navigator) &&
          goog.isDefAndNotNull(win.navigator.geolocation);

    case bot.html5.API.LOCAL_STORAGE:
      // IE8 does not support local storage, though the APIs exist.
      if (bot.html5.IS_IE8_) {
        return false;
      }
      return goog.isDefAndNotNull(win.localStorage);

    case bot.html5.API.SESSION_STORAGE:
      // IE8 does not support session storage, though the APIs exist.
      if (bot.html5.IS_IE8_) {
        return false;
      }
      return goog.isDefAndNotNull(win.sessionStorage) &&
          // To avoid browsers that only support this API partically
          // like some versions of FF.
          goog.isDefAndNotNull(win.sessionStorage.clear);

    default:
      throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
          'Unsupported API identifier provided as parameter');
  }
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atom to retrieve the physical location of the device.
 *
 */

goog.provide('bot.geolocation');

goog.require('bot');
goog.require('bot.html5');


/**
 * Default parameters used to configure the geolocation.getCurrentPosition
 * method. These parameters mean retrieval of any cached position with high
 * accuracy within a timeout interval of 5s.
 * @const
 * @type {GeolocationPositionOptions}
 * @see http://dev.w3.org/geo/api/spec-source.html#position-options
 */
bot.geolocation.DEFAULT_OPTIONS = /** @type {GeolocationPositionOptions} */ ({
  enableHighAccuracy: true,
  maximumAge: Infinity,
  timeout: 5000
});


/**
 * Provides a mechanism to retrieve the geolocation of the device.  It invokes
 * the navigator.geolocation.getCurrentPosition method of the HTML5 API which
 * later callbacks with either position value or any error. The position/
 * error is updated with the callback functions.
 *
 * @param {function(?GeolocationPosition)} successCallback The callback method
 *     which is invoked on success.
 * @param {function(GeolocationPositionError)=} opt_errorCallback The callback
 *     method which is invoked on error.
 * @param {GeolocationPositionOptions=} opt_options The optional parameters to
 *     navigator.geolocation.getCurrentPosition; defaults to
 *     bot.geolocation.DEFAULT_OPTIONS.
 */
bot.geolocation.getCurrentPosition = function(successCallback,
    opt_errorCallback, opt_options) {
  var win = bot.getWindow();
  var posOptions = opt_options || bot.geolocation.DEFAULT_OPTIONS;

  if (bot.html5.isSupported(bot.html5.API.GEOLOCATION, win)) {
    var geolocation = win.navigator.geolocation;
    geolocation.getCurrentPosition(successCallback,
        opt_errorCallback, posOptions);
  } else {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR, 'Geolocation undefined');
  }
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Atoms for accessing HTML5 web storage maps (localStorage,
 * sessionStorage). These storage objects store each item as a key-value
 * mapping pair.
 *
 */

goog.provide('bot.storage');
goog.provide('bot.storage.Storage');

goog.require('bot');
goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.html5');


/**
 * A factory method to create a wrapper to access the HTML5 localStorage
 * object.
 * Note: We are not using Closure from goog.storage,
 * Closure uses "window" object directly, which may not always be
 * defined (for example in firefox extensions).
 * We use bot.window() from bot.js instead to keep track of the window or frame
 * is currently being used for command execution. The implementation is
 * otherwise similar to the implementation in the Closure library
 * (goog.storage.mechansim.HTML5LocalStorage).
 *
 * @param {Window=} opt_window The window whose storage to access;
 *     defaults to the main window.
 * @return {!bot.storage.Storage} The wrapper Storage object.
 */
bot.storage.getLocalStorage = function(opt_window) {
  var win = opt_window || bot.getWindow();

  if (!bot.html5.isSupported(bot.html5.API.LOCAL_STORAGE, win)) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR, 'Local storage undefined');
  }
  var storageMap = win.localStorage;
  return new bot.storage.Storage(storageMap);
};


/**
 * A factory method to create a wrapper to access the HTML5 sessionStorage
 * object.
 *
 * @param {Window=} opt_window The window whose storage to access;
 *     defaults to the main window.
 * @return {!bot.storage.Storage} The wrapper Storage object.
 */
bot.storage.getSessionStorage = function(opt_window) {
  var win = opt_window || bot.getWindow();

  if (bot.html5.isSupported(bot.html5.API.SESSION_STORAGE, win)) {
    var storageMap = win.sessionStorage;
    return new bot.storage.Storage(storageMap);
  }
  throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR,
      'Session storage undefined');
};



/**
 * Provides a wrapper object to the HTML5 web storage object.
 * @constructor
 *
 * @param {Storage} storageMap HTML5 storage object e.g. localStorage,
 *     sessionStorage.
 */
bot.storage.Storage = function(storageMap) {
  /**
   * Member variable to access the assigned HTML5 storage object.
   * @private {Storage}
   */
  this.storageMap_ = storageMap;
};


/**
 * Sets the value item of a key/value pair in the Storage object.
 * If the value given is null, the string 'null' will be inserted
 * instead.
 *
 * @param {string} key The key of the item.
 * @param {*} value The value of the item.
 */
bot.storage.Storage.prototype.setItem = function(key, value) {
  try {
    // Note: Ideally, browsers should set a null value. But the browsers
    // report arbitrarily. Firefox returns <null>, while Chrome reports
    // the string "null". We are setting the value to the string "null".
    this.storageMap_.setItem(key, value + '');
  } catch (e) {
    throw new bot.Error(bot.ErrorCode.UNKNOWN_ERROR, e.message);
  }
};


/**
 * Returns the value item of a key in the Storage object.
 *
 * @param {string} key The key of the returned value.
 * @return {?string} The mapped value if present in the storage object,
 *     otherwise null. If a null value  was inserted for a given
 *     key, then the string 'null' is returned.
 */
bot.storage.Storage.prototype.getItem = function(key) {
  var value = this.storageMap_.getItem(key);
  return  /** @type {?string} */ (value);
};


/**
 * Returns an array of keys of all keys of the Storage object.
 *
 * @return {Array.<string>} The array of stored keys..
 */
bot.storage.Storage.prototype.keySet = function() {
  var keys = [];
  var length = this.size();
  for (var i = 0; i < length; i++) {
    keys[i] = this.storageMap_.key(i);
  }
  return keys;
};


/**
 * Removes an item with a given key.
 *
 * @param {string} key The key item of the key/value pair.
 * @return {?string} The removed value if present, otherwise null.
 */
bot.storage.Storage.prototype.removeItem = function(key) {
  var value = this.getItem(key);
  this.storageMap_.removeItem(key);
  return value;
};


/**
 * Removes all items.
 */
bot.storage.Storage.prototype.clear = function() {
  this.storageMap_.clear();
};


/**
 * Returns the number of items in the Storage object.
 *
 * @return {number} The number of the key/value pairs.
 */
bot.storage.Storage.prototype.size = function() {
  return this.storageMap_.length;
};


/**
 * Returns the key item of the key/value pairs in the Storage object
 * of a given index.
 *
 * @param {number} index The index of the key/value pair list.
 * @return {?string} The key item of a given index.
 */
bot.storage.Storage.prototype.key = function(index) {
  return this.storageMap_.key(index);
};


/**
 * Returns HTML5 storage object of the wrapper Storage object
 *
 * @return {Storage} The storageMap attribute.
 */
bot.storage.Storage.prototype.getStorageMap = function() {
  return this.storageMap_;
};
// Copyright 2012 Software Freedom Conservancy. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

goog.provide('webdriver.Key');


/**
 * Representations of pressable keys that aren't text.  These are stored in
 * the Unicode PUA (Private Use Area) code points, 0xE000-0xF8FF.  Refer to
 * http://www.google.com.au/search?&q=unicode+pua&btnG=Search
 *
 * @enum {string}
 */
webdriver.Key = {
  NULL:         '\uE000',
  CANCEL:       '\uE001',  // ^break
  HELP:         '\uE002',
  BACK_SPACE:   '\uE003',
  TAB:          '\uE004',
  CLEAR:        '\uE005',
  RETURN:       '\uE006',
  ENTER:        '\uE007',
  SHIFT:        '\uE008',
  CONTROL:      '\uE009',
  ALT:          '\uE00A',
  PAUSE:        '\uE00B',
  ESCAPE:       '\uE00C',
  SPACE:        '\uE00D',
  PAGE_UP:      '\uE00E',
  PAGE_DOWN:    '\uE00F',
  END:          '\uE010',
  HOME:         '\uE011',
  ARROW_LEFT:   '\uE012',
  LEFT:         '\uE012',
  ARROW_UP:     '\uE013',
  UP:           '\uE013',
  ARROW_RIGHT:  '\uE014',
  RIGHT:        '\uE014',
  ARROW_DOWN:   '\uE015',
  DOWN:         '\uE015',
  INSERT:       '\uE016',
  DELETE:       '\uE017',
  SEMICOLON:    '\uE018',
  EQUALS:       '\uE019',

  NUMPAD0:      '\uE01A',  // number pad keys
  NUMPAD1:      '\uE01B',
  NUMPAD2:      '\uE01C',
  NUMPAD3:      '\uE01D',
  NUMPAD4:      '\uE01E',
  NUMPAD5:      '\uE01F',
  NUMPAD6:      '\uE020',
  NUMPAD7:      '\uE021',
  NUMPAD8:      '\uE022',
  NUMPAD9:      '\uE023',
  MULTIPLY:     '\uE024',
  ADD:          '\uE025',
  SEPARATOR:    '\uE026',
  SUBTRACT:     '\uE027',
  DECIMAL:      '\uE028',
  DIVIDE:       '\uE029',

  F1:           '\uE031',  // function keys
  F2:           '\uE032',
  F3:           '\uE033',
  F4:           '\uE034',
  F5:           '\uE035',
  F6:           '\uE036',
  F7:           '\uE037',
  F8:           '\uE038',
  F9:           '\uE039',
  F10:          '\uE03A',
  F11:          '\uE03B',
  F12:          '\uE03C',

  COMMAND:      '\uE03D',  // Apple command key
  META:         '\uE03D'   // alias for Windows key
};
// Copyright 2010 WebDriver committers
// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/**
 * @fileoverview Atoms-based implementation of the webelement interface.
 */

goog.provide('webdriver.atoms.element');

goog.require('bot.Keyboard.Keys');
goog.require('bot.action');
goog.require('bot.dom');
goog.require('goog.dom');
goog.require('goog.dom.TagName');
goog.require('goog.math');
goog.require('goog.string');
goog.require('goog.style');
goog.require('webdriver.Key');


/**
 * @param {!Element} element The element to use.
 * @return {boolean} Whether the element is checked or selected.
 */
webdriver.atoms.element.isSelected = function(element) {
  // Although this method looks unloved, its compiled form is used by
  // the Chrome and OperaDrivers.
  if (!bot.dom.isSelectable(element)) {
    return false;
  }

  return bot.dom.isSelected(element);
};


/**
 * Common aliases for properties. This maps names that users use to the correct
 * property name.
 *
 * @const
 * @private {!Object.<string, string>}
 */
webdriver.atoms.element.PROPERTY_ALIASES_ = {
  'class': 'className',
  'readonly': 'readOnly'
};


/**
 * Used to determine whether we should return a boolean value from getAttribute.
 * These are all extracted from the WHATWG spec:
 *
 *   http://www.whatwg.org/specs/web-apps/current-work/
 *
 * These must all be lower-case.
 *
 * @const
 * @private {!Array.<string>}
 */
webdriver.atoms.element.BOOLEAN_PROPERTIES_ = [
  'async',
  'autofocus',
  'autoplay',
  'checked',
  'compact',
  'complete',
  'controls',
  'declare',
  'defaultchecked',
  'defaultselected',
  'defer',
  'disabled',
  'draggable',
  'ended',
  'formnovalidate',
  'hidden',
  'indeterminate',
  'iscontenteditable',
  'ismap',
  'itemscope',
  'loop',
  'multiple',
  'muted',
  'nohref',
  'noresize',
  'noshade',
  'novalidate',
  'nowrap',
  'open',
  'paused',
  'pubdate',
  'readonly',
  'required',
  'reversed',
  'scoped',
  'seamless',
  'seeking',
  'selected',
  'spellcheck',
  'truespeed',
  'willvalidate'
];


/**
 * Get the value of the given property or attribute. If the "attribute" is for
 * a boolean property, we return null in the case where the value is false. If
 * the attribute name is "style" an attempt to convert that style into a string
 * is done.
 *
 * @param {!Element} element The element to use.
 * @param {string} attribute The name of the attribute to look up.
 * @return {?string} The string value of the attribute or property, or null.
 */
webdriver.atoms.element.getAttribute = function(element, attribute) {
  var value = null;
  var name = attribute.toLowerCase();

  if ('style' == name) {
    value = element.style;

    if (value && !goog.isString(value)) {
      value = value.cssText;
    }

    return (/** @type {?string} */value);
  }

  if (('selected' == name || 'checked' == name) &&
      bot.dom.isSelectable(element)) {
    return bot.dom.isSelected(element) ? 'true' : null;
  }

  // Our tests suggest that returning the attribute is desirable for
  // the href attribute of <a> tags and the src attribute of <img> tags,
  // but we normally attempt to get the property value before the attribute.
  var isLink = bot.dom.isElement(element, goog.dom.TagName.A);
  var isImg = bot.dom.isElement(element, goog.dom.TagName.IMG);

  // Although the attribute matters, the property is consistent. Return that in
  // preference to the attribute for links and images.
  if ((isImg && name == 'src') || (isLink && name == 'href')) {
    value = bot.dom.getAttribute(element, name);
    if (value) {
      // We want the full URL if present
      value = bot.dom.getProperty(element, name);
    }
    return (/** @type {?string} */value);
  }

  var propName = webdriver.atoms.element.PROPERTY_ALIASES_[attribute] ||
      attribute;
  if (goog.array.contains(webdriver.atoms.element.BOOLEAN_PROPERTIES_, name)) {
    value = !goog.isNull(bot.dom.getAttribute(element, attribute)) ||
        bot.dom.getProperty(element, propName);
    return value ? 'true' : null;
  }

  var property;
  try {
    property = bot.dom.getProperty(element, propName);
  } catch (e) {
    // Leaves property undefined or null
  }

  // 1- Call getAttribute if getProperty fails,
  // i.e. property is null or undefined.
  // This happens for event handlers in Firefox.
  // For example, calling getProperty for 'onclick' would
  // fail while getAttribute for 'onclick' will succeed and
  // return the JS code of the handler.
  //
  // 2- When property is an object we fall back to the
  // actual attribute instead.
  // See issue http://code.google.com/p/selenium/issues/detail?id=966
  if (!goog.isDefAndNotNull(property) || goog.isObject(property)) {
    value = bot.dom.getAttribute(element, attribute);
  } else {
    value = property;
  }

  // The empty string is a valid return value.
  return goog.isDefAndNotNull(value) ? value.toString() : null;
};


/**
 * Get the location of the element in page space, if it's displayed.
 *
 * @param {!Element} element The element to get the location for.
 * @return {goog.math.Rect} The bounding rectangle of the element.
 */
webdriver.atoms.element.getLocation = function(element) {
  if (!bot.dom.isShown(element)) {
    return null;
  }
  return goog.style.getBounds(element);
};


/**
 * @param {Node} element The element to use.
 * @return {boolean} Whether the element is in the HEAD tag.
 * @private
 */
webdriver.atoms.element.isInHead_ = function(element) {
  while (element) {
    if (element.tagName && element.tagName.toLowerCase() == 'head') {
      return true;
    }
    try {
      element = element.parentNode;
    } catch (e) {
      // Fine. the DOM has dispeared from underneath us
      return false;
    }
  }

  return false;
};


/**
 * @param {!Element} element The element to get the text from.
 * @return {string} The visible text or an empty string.
 */
webdriver.atoms.element.getText = function(element) {
  return bot.dom.getVisibleText(element);
};


/**
 * Types keys on the given {@code element} with a virtual keyboard. Converts
 * special characters from the WebDriver JSON wire protocol to the appropriate
 * {@link bot.Keyboard.Key} value.
 *
 * @param {!Element} element The element to type upon.
 * @param {!Array.<string>} keys The keys to type on the element.
 * @param {bot.Keyboard=} opt_keyboard Keyboard to use; if not provided,
 *    constructs one.
 * @param {boolean=} opt_persistModifiers Whether modifier keys should remain
 *     pressed when this function ends.
 * @see bot.action.type
 * @see http://code.google.com/p/selenium/wiki/JsonWireProtocol
 */
webdriver.atoms.element.type = function(
    element, keys, opt_keyboard, opt_persistModifiers) {
  var persistModifierKeys = !!opt_persistModifiers;
  function createSequenceRecord() {
    return {persist: persistModifierKeys, keys: []};
  }

  /**
   * @type {!Array.<{persist: boolean,
   *                 keys: !Array.<(string|!bot.Keyboard.Key)>}>}
   */
  var convertedSequences = [];

  /**
   * @type {{persist: boolean,
   *         keys: !Array.<(string|!bot.Keyboard.Key)>}}
   */
  var current = createSequenceRecord();
  convertedSequences.push(current);

  goog.array.forEach(keys, function(sequence) {
    goog.array.forEach(sequence.split(''), function(key) {
      if (isWebDriverKey(key)) {
        var webdriverKey = webdriver.atoms.element.type.JSON_TO_KEY_MAP_[key];
        // goog.isNull uses ==, which accepts undefined.
        if (webdriverKey === null) {
          // bot.action.type does not support a "null" key, so we have to
          // terminate the entire sequence to release modifier keys. If
          // we currently allow modifier key state to persist across key
          // sequences, we need to inject a dummy sequence that does not
          // persist state so every modifier key gets released.
          convertedSequences.push(current = createSequenceRecord());
          if (persistModifierKeys) {
            current.persist = false;
            convertedSequences.push(current = createSequenceRecord());
          }
        } else if (goog.isDef(webdriverKey)) {
          current.keys.push(webdriverKey);
        } else {
          throw Error('Unsupported WebDriver key: \\u' +
              key.charCodeAt(0).toString(16));
        }
      } else {
        // Handle common aliases.
        switch (key) {
          case '\n':
            current.keys.push(bot.Keyboard.Keys.ENTER);
            break;
          case '\t':
            current.keys.push(bot.Keyboard.Keys.TAB);
            break;
          case '\b':
            current.keys.push(bot.Keyboard.Keys.BACKSPACE);
            break;
          default:
            current.keys.push(key);
            break;
        }
      }
    });
  });

  goog.array.forEach(convertedSequences, function(sequence) {
    bot.action.type(element, sequence.keys, opt_keyboard,
        sequence.persist);
  });

  function isWebDriverKey(c) {
    return '\uE000' <= c && c <= '\uE03D';
  }
};


/**
 * Maps JSON wire protocol values to their {@link bot.Keyboard.Key} counterpart.
 * @private {!Object.<bot.Keyboard.Key>}
 * @const
 */
webdriver.atoms.element.type.JSON_TO_KEY_MAP_ = {};
goog.scope(function() {
var map = webdriver.atoms.element.type.JSON_TO_KEY_MAP_;
var key = webdriver.Key;
var botKey = bot.Keyboard.Keys;

map[key.NULL] = null;
map[key.BACK_SPACE] = botKey.BACKSPACE;
map[key.TAB] = botKey.TAB;
map[key.RETURN] = botKey.ENTER;
// This not correct, but most browsers will do the right thing.
map[key.ENTER] = botKey.ENTER;
map[key.SHIFT] = botKey.SHIFT;
map[key.CONTROL] = botKey.CONTROL;
map[key.ALT] = botKey.ALT;
map[key.PAUSE] = botKey.PAUSE;
map[key.ESCAPE] = botKey.ESC;
map[key.SPACE] = botKey.SPACE;
map[key.PAGE_UP] = botKey.PAGE_UP;
map[key.PAGE_DOWN] = botKey.PAGE_DOWN;
map[key.END] = botKey.END;
map[key.HOME] = botKey.HOME;
map[key.LEFT] = botKey.LEFT;
map[key.UP] = botKey.UP;
map[key.RIGHT] = botKey.RIGHT;
map[key.DOWN] = botKey.DOWN;
map[key.INSERT] = botKey.INSERT;
map[key.DELETE] = botKey.DELETE;
map[key.SEMICOLON] = botKey.SEMICOLON;
map[key.EQUALS] = botKey.EQUALS;
map[key.NUMPAD0] = botKey.NUM_ZERO;
map[key.NUMPAD1] = botKey.NUM_ONE;
map[key.NUMPAD2] = botKey.NUM_TWO;
map[key.NUMPAD3] = botKey.NUM_THREE;
map[key.NUMPAD4] = botKey.NUM_FOUR;
map[key.NUMPAD5] = botKey.NUM_FIVE;
map[key.NUMPAD6] = botKey.NUM_SIX;
map[key.NUMPAD7] = botKey.NUM_SEVEN;
map[key.NUMPAD8] = botKey.NUM_EIGHT;
map[key.NUMPAD9] = botKey.NUM_NINE;
map[key.MULTIPLY] = botKey.NUM_MULTIPLY;
map[key.ADD] = botKey.NUM_PLUS;
map[key.SUBTRACT] = botKey.NUM_MINUS;
map[key.DECIMAL] = botKey.NUM_PERIOD;
map[key.DIVIDE] = botKey.NUM_DIVISION;
map[key.SEPARATOR] = botKey.SEPARATOR;
map[key.F1] = botKey.F1;
map[key.F2] = botKey.F2;
map[key.F3] = botKey.F3;
map[key.F4] = botKey.F4;
map[key.F5] = botKey.F5;
map[key.F6] = botKey.F6;
map[key.F7] = botKey.F7;
map[key.F8] = botKey.F8;
map[key.F9] = botKey.F9;
map[key.F10] = botKey.F10;
map[key.F11] = botKey.F11;
map[key.F12] = botKey.F12;
map[key.META] = botKey.META;
});  // goog.scope
// Copyright 2012 WebDriver committers
// Copyright 2012 Software Freedom Conservancy
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Synthetic events for fun and profit.
 */

goog.provide('webdriver.atoms.inputs');

goog.require('bot.Keyboard');
goog.require('bot.Mouse');
goog.require('bot.action');
goog.require('goog.array');
goog.require('goog.dom');
goog.require('goog.math.Coordinate');
goog.require('goog.style');
goog.require('webdriver.atoms.element');


/**
 * Send keyboard input to a particular element.
 *
 * @param {Element} element The element to send the keyboard input to, or
 *     {@code null} to use the document's active element.
 * @param {!Array.<string>} keys The keys to type on the element.
 * @param {Array.<!bot.Keyboard.Key>=} opt_state The keyboard to use, or
 *     construct one.
 * @param {boolean=} opt_persistModifiers Whether modifier keys should remain
 *     pressed when this function ends.
 * @return {Array.<!bot.Keyboard.Key>} The keyboard state.
 */
webdriver.atoms.inputs.sendKeys = function(
    element, keys, opt_state, opt_persistModifiers) {
  var keyboard = new bot.Keyboard(opt_state);
  if (!element) {
    element = bot.dom.getActiveElement(document);
  }
  if (!element) {
    throw Error('No element to send keys to');
  }
  webdriver.atoms.element.type(element, keys, keyboard, opt_persistModifiers);

  return keyboard.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.sendKeys',
                  webdriver.atoms.inputs.sendKeys);


/**
 * Click on an element.
 *
 * @param {Element} element The element to click.
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.click = function(element, opt_state) {
  var mouse = new bot.Mouse(opt_state);
  if (!element) {
    element = mouse.getState().element;
  }
  if (!element) {
    throw Error('No element to send keys to');
  }
  bot.action.click(element, null, mouse);
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.click',
                  webdriver.atoms.inputs.click);


/**
 * Move the mouse to a specific element and/or coordinate location.
 *
 * @param {!Element} element The element to move the mouse to.
 * @param {number} x_offset The x coordinate to use as an offset.
 * @param {number} y_offset The y coordinate to use as an offset.
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.mouseMove = function(element, x_offset, y_offset,
    opt_state) {
  var mouse = new bot.Mouse(opt_state);
  var target = element || mouse.getState().element;

  var offset_specified = (x_offset != null) && (y_offset != null);
  x_offset = x_offset || 0;
  y_offset = y_offset || 0;

  // If we have specified an element and no offset, we should
  // move the mouse to the center of the specified element.
  if (element) {
    if (!offset_specified) {
      var source_element_size = bot.action.getInteractableSize(element);
      x_offset = Math.floor(source_element_size.width / 2);
      y_offset = Math.floor(source_element_size.height / 2);
    }
  } else {
    // Moving to an absolute offset from the current target element,
    // so we have to account for the existing offset of the current
    // mouse position to the element origin (upper-left corner).
    var pos = goog.style.getClientPosition(target);
    x_offset += (mouse.getState().clientXY.x - pos.x);
    y_offset += (mouse.getState().clientXY.y - pos.y);
  }

  var doc = goog.dom.getOwnerDocument(target);
  var win = goog.dom.getWindow(doc);
  var inViewAfterScroll = bot.action.scrollIntoView(
      target,
      new goog.math.Coordinate(x_offset, y_offset));

  var coords = new goog.math.Coordinate(x_offset, y_offset);
  mouse.move(target, coords);
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.mouseMove',
                  webdriver.atoms.inputs.mouseMove);


/**
 * Presses the primary mouse button at the current location.
 *
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.mouseButtonDown = function(opt_state) {
  var mouse = new bot.Mouse(opt_state);
  mouse.pressButton(bot.Mouse.Button.LEFT);
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.mouseButtonDown',
                  webdriver.atoms.inputs.mouseButtonDown);


/**
 * Releases the primary mouse button at the current location.
 *
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.mouseButtonUp = function(opt_state) {
  var mouse = new bot.Mouse(opt_state);
  mouse.releaseButton();
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.mouseButtonUp',
                  webdriver.atoms.inputs.mouseButtonUp);


/**
 * Double-clicks primary mouse button at the current location.
 *
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.doubleClick = function(opt_state) {
  var mouse = new bot.Mouse(opt_state);
  mouse.pressButton(bot.Mouse.Button.LEFT);
  mouse.releaseButton();
  mouse.pressButton(bot.Mouse.Button.LEFT);
  mouse.releaseButton();
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.doubleClick',
                  webdriver.atoms.inputs.doubleClick);


/**
 * Right-clicks mouse button at the current location.
 *
 * @param {bot.Mouse.State=} opt_state The serialized state of the mouse.
 * @return {!bot.Mouse.State} The mouse state.
 */
webdriver.atoms.inputs.rightClick = function(opt_state) {
  var mouse = new bot.Mouse(opt_state);
  mouse.pressButton(bot.Mouse.Button.RIGHT);
  mouse.releaseButton();
  return mouse.getState();
};
goog.exportSymbol('webdriver.atoms.inputs.rightClick',
                  webdriver.atoms.inputs.rightClick);
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for manipulating the DOM.
 */

goog.provide('webdriver.atoms.inject.action');

goog.require('bot.action');
goog.require('goog.dom.selection');
goog.require('webdriver.atoms.element');
goog.require('webdriver.atoms.inject');


/**
 * Sends key events to simulating typing on an element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to submit.
 * @param {!Array.<string>} keys The keys to type.
 * @return {string} A stringified {@link bot.response.ResponseObject}.
 */
webdriver.atoms.inject.action.type = function(element, keys) {
  return webdriver.atoms.inject.executeScript(webdriver.atoms.element.type,
      [element, keys]);
};


/**
 * Submits the form containing the given element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to submit.
 * @return {string} A stringified {@link bot.response.ResponseObject}.
 * @deprecated Click on a submit button or type ENTER in a text box instead.
 */
webdriver.atoms.inject.action.submit = function(element) {
  return webdriver.atoms.inject.executeScript(bot.action.submit, [element]);
};


/**
 * Clear an element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to clear.
 * @return {string} A stringified {@link bot.response.ResponseObject}.
 * @see bot.action.clear
 */
webdriver.atoms.inject.action.clear = function(element) {
  return webdriver.atoms.inject.executeScript(bot.action.clear, [element]);
};


/**
 * Click an element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to click.
 * @return {string} A stringified {@link bot.response.ResponseObject}.
 * @see bot.action.click
 */
webdriver.atoms.inject.action.click = function(element) {
  return webdriver.atoms.inject.executeScript(bot.action.click, [element]);
};

// Copyright 2012 WebDriver committers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for handling application cache.
 */

goog.provide('webdriver.atoms.inject.storage.appcache');

goog.require('bot.inject');
goog.require('webdriver.atoms.storage.appcache');


/**
 * Gets the status of the application cache.
 *
 * @return {string} The status of the application cache.
 */
webdriver.atoms.inject.storage.appcache.getStatus = function() {
  return /**@type {string}*/(bot.inject.executeScript(
      webdriver.atoms.storage.appcache.getStatus, [], true));
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for querying the DOM.
 */

goog.provide('webdriver.atoms.inject.dom');

goog.require('bot.action');
goog.require('bot.dom');
goog.require('webdriver.atoms.element');
goog.require('webdriver.atoms.inject');


/**
 * Gets the visisble text for the given element.
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @return {string} The visible text wrapped in a JSON string as defined by the
 *     WebDriver wire protocol.
 */
webdriver.atoms.inject.dom.getText = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.getVisibleText,
      [element]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @return {string} A boolean describing whether the element is
 *     checked or selected wrapped in a JSON string as defined by
 *     the wire protocol.
 */
webdriver.atoms.inject.dom.isSelected = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.isSelected, [element]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @return {string} The coordinates of the top left corner in a JSON
 *     string as defined by the wire protocol.
 */
webdriver.atoms.inject.dom.getTopLeftCoordinates = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.getLocationInView,
      [element]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @param {string} attribute The attribute to look up.
 * @return {string} The requested attribute value in a JSON string
 *     as defined by the wire protocol.
 */
webdriver.atoms.inject.dom.getAttributeValue = function(element, attribute) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.element.getAttribute, [element, attribute]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @return {string} The element size in a JSON string as
 *     defined by the wire protocol.
 */
webdriver.atoms.inject.dom.getSize = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.getElementSize,
      [element]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @param {string} property The property to look up.
 * @return {string} The value of the requested CSS property in a JSON
 *     string as defined by the wire protocol.
 */
webdriver.atoms.inject.dom.getValueOfCssProperty =
    function(element, property) {
  return webdriver.atoms.inject.executeScript(bot.dom.getEffectiveStyle,
      [element, property]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to query.
 * @return {string} A boolean describing whether the element is enabled
 *     in a JSON string as defined by the wire protocol.
 */
webdriver.atoms.inject.dom.isEnabled = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.isEnabled, [element]);
};


/**
 * @param {{bot.inject.ELEMENT_KEY: string}} element The element to check.
 * @return {string} true if the element is visisble, false otherwise.
 *     The result is wrapped in a JSON string as defined by the wire
 *     protocol.
 */
webdriver.atoms.inject.dom.isDisplayed = function(element) {
  return webdriver.atoms.inject.executeScript(bot.dom.isShown,
      [element, /*ignoreOpacity=*/true]);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Wrapping execute script to use a serialized window object.
 *
 */

goog.provide('webdriver.atoms.inject');

goog.require('bot.inject');
goog.require('bot.inject.cache');


/**
 * Wrapper to allow passing a seliazed window object to executeScript.
 *
 * @param {!(string|Function)} fn The function to execute.
 * @param {Array.<*>} args Array of arguments to pass to fn.
 * @param {{bot.inject.WINDOW_KEY:string}=} opt_window The serialized window
 *     object to be read from the cache.
 * @return {string} The response object, serialized and returned in string
 *     format.
 */
webdriver.atoms.inject.executeScript = function(fn, args, opt_window) {
  return /**@type {string}*/(bot.inject.executeScript(fn, args, true,
      webdriver.atoms.inject.getWindow_(opt_window)));
};


/**
 *
 * @param {!(string|Function)} fn The function to execute.
 * @param {Array.<*>} args Array of arguments to pass to fn.
 * @param {number} timeout The timeout to wait up to in millis.
 * @param {function(string)|function(!bot.response.ResponseObject)} onDone
 *     The function to call when the given {@code fn} invokes its callback,
 *     or when an exception or timeout occurs. This will always be called.
 * @param {{bot.inject.WINDOW_KEY:string}=} opt_window The serialized window
 *     object to be read from the cache.
 */
webdriver.atoms.inject.executeAsyncScript =
    function(fn, args, timeout, onDone, opt_window) {
  bot.inject.executeAsyncScript(
      fn, args, timeout, onDone, true,
      webdriver.atoms.inject.getWindow_(opt_window));
};


/**
 * Get the window to use.
 *
 * @param {{bot.inject.WINDOW_KEY:string}=} opt_window The serialized window
 *     object to be read from the cache.
 * @return {!Window} A reference to a window.
 * @private
 */
webdriver.atoms.inject.getWindow_ = function(opt_window) {
  var win;
  if (opt_window) {
    win = bot.inject.cache.getElement(opt_window[bot.inject.WINDOW_KEY]);
  } else {
    win = window;
  }
  return /**@type {!Window}*/(win);
};// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms to find elements in the page.
 */

goog.provide('webdriver.atoms.inject.locators');

goog.require('bot.locators');
goog.require('webdriver.atoms.inject');

/**
 * Finds an element by using the given lookup strategy.
 * @param {string} strategy The strategy to use to locate the element.
 * @param {string} using The locator to use.
 * @param {(Document|Element)=} opt_root The document or element to perform
 *     the search under. If not specified, will use {@code document}
 *     as the root.
 * @return {string} The result wrapped
 *     in a JSON string as defined by the WebDriver wire protocol.
 */
webdriver.atoms.inject.locators.findElement =
    function(strategy, using, opt_root) {
  var locator = {};
  locator[strategy] = using;
  return webdriver.atoms.inject.executeScript(bot.locators.findElement,
      [locator, opt_root]);
};


/**
 * Finds all elements by using the given lookup strategy.
 * @param {string} strategy The strategy to use to locate the element.
 * @param {string} using The locator to use.
 * @param {(Document|Element)=} opt_root The document or element to perform
 *     the search under. If not specified, will use {@code document}
 *     as the root.
 * @return {string} The result wrapped
 *     in a JSON string as defined by the WebDriver wire protocol.
 */
webdriver.atoms.inject.locators.findElements =
    function(strategy, using, opt_root) {
  var locator = {};
  locator[strategy] = using;
  return webdriver.atoms.inject.executeScript(bot.locators.findElements,
      [locator, opt_root]);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for handling frames.
 */

goog.provide('webdriver.atoms.inject.frame');

goog.require('bot.frame');
goog.require('bot.inject.cache');
goog.require('webdriver.atoms.inject');


/**
 * Finds a frame by id or name.
 *
 * @param {string} idOrName The frame id or name.
 * @param {{bot.inject.WINDOW_KEY: string}=} opt_root The wrapped window to
 *     perform the search under. Defaults to window.
 * @return {string} A frame element wrapped in a JSON string as defined by
 *     the wire protocol.
 */
webdriver.atoms.inject.frame.findFrameByIdOrName =
    function(idOrName, opt_root) {
  return webdriver.atoms.inject.executeScript(bot.frame.findFrameByNameOrId,
      [idOrName, opt_root]);
};


/**
 * @return {string} A string representing the currently active element.
 */
webdriver.atoms.inject.frame.activeElement = function() {
  return webdriver.atoms.inject.executeScript(bot.frame.activeElement, []);
};


/**
 * Finds a frame by index.
 *
 * @param {number} index The index of the frame to search for.
 * @param {!Window=} opt_root The window to perform the search under.
 * If not specified window is used as the default.
 * @return {string} A frame element wrapped in a JSON string as defined by
 *     the wire protocol.
 */
webdriver.atoms.inject.frame.findFrameByIndex = function(index, opt_root) {
  return webdriver.atoms.inject.executeScript(bot.frame.findFrameByIndex,
      [index, opt_root]);
};


/**
 * @return {string} The default content of the current page,
 *     which is the top window.
 */
webdriver.atoms.inject.frame.defaultContent = function() {
  return webdriver.atoms.inject.executeScript(bot.frame.defaultContent, []);
};


/**
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to query.
 * @return {string} The window corresponding to the frame element
 *     wrapped in a JSON string as defined by the wire protocol.
 */
webdriver.atoms.inject.frame.getFrameWindow = function(element) {
  return webdriver.atoms.inject.executeScript(bot.frame.getFrameWindow,
      [element]);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for handling local storage.
 */

goog.provide('webdriver.atoms.inject.storage.local');

goog.require('webdriver.atoms.inject');
goog.require('webdriver.atoms.storage.local');


/**
 * Sets an item in the local storage.
 *
 * @param {string} key The key of the item.
 * @param {*} value The value of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.setItem = function(key, value) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.setItem, [key, value]);
};


/**
 * Gets an item from the local storage.
 *
 * @param {string} key The key of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.getItem = function(key) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.getItem, [key]);
};


/**
 * Gets the key set of the entries.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.keySet = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.keySet, []);
};


/**
 * Removes an item in the local storage.
 *
 * @param {string} key The key of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.removeItem = function(key) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.removeItem, [key]);
};


/**
 * Clears the local storage.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.clear = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.clear, []);
};


/**
 * Gets the size of the local storage.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.local.size = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.local.size, []);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for handling session storage.
 */

goog.provide('webdriver.atoms.inject.storage.session');

goog.require('webdriver.atoms.inject');
goog.require('webdriver.atoms.storage.session');


/**
 * Sets an item in the session storage.
 *
 * @param {string} key The key of the item.
 * @param {*} value The value of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.setItem = function(key, value) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.setItem, [key, value]);
};


/**
 * Gets an item from the session storage.
 *
 * @param {string} key The key of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.getItem = function(key) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.getItem, [key]);
};


/**
 * Gets the key set of the entries.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.keySet = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.keySet, []);
};


/**
 * Removes an item in the session storage.
 *
 * @param {string} key The key of the item.
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.removeItem = function(key) {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.removeItem, [key]);
};


/**
 * Clears the session storage.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.clear = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.clear, []);
};


/**
 * Gets the size of the session storage.
 *
 * @return {string} The stringified result wrapped according to the wire
 *     protocol.
 */
webdriver.atoms.inject.storage.session.size = function() {
  return webdriver.atoms.inject.executeScript(
      webdriver.atoms.storage.session.size, []);
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Ready to inject atoms for handling web SQL database.
 */

goog.provide('webdriver.atoms.inject.storage.database');

goog.require('bot.Error');
goog.require('bot.ErrorCode');
goog.require('bot.storage.database');
goog.require('webdriver.atoms.inject');


/**
 * Executes the given query in the Web SQL database specified.
 *
 * @param {string} databaseName The name of the database.
 * @param {string} query The SQL statement.
 * @param {Array.<*>} args Arguments to pass to the query.
 * @param {function(string)} onDone The callback to invoke when done. The
 *     result, according to the wire protocol, will be passed to this callback.
 */
webdriver.atoms.inject.storage.database.executeSql =
    function(databaseName, query, args, onDone) {
  var onSuccessCallback = function(tx, result) {
    onDone(webdriver.atoms.inject.executeScript(function(res) {
      return result;
    }, [result]));
  };

  var onErrorCallback = function(error) {
    onDone(webdriver.atoms.inject.executeScript(function() {
      throw new bot.Error(bot.ErrorCode.SQL_DATABASE_ERROR,
          'SQL Error Code: ' + error.code + '. SQL Error Message: ' +
          error.message);
    }, []));
  };

  bot.storage.database.executeSql(
      databaseName, query, args, onSuccessCallback, onErrorCallback);
};
// Copyright 2012 WebDriver committers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utility functions for accessing HTML5 appcache object.
 * These functions are wrapper of the functions of individual method of
 * bot.storage.Storage class. An extra redirection is used to define
 * individual functional unit (atom) for injecting in Webdriver.
 */

goog.provide('webdriver.atoms.storage.appcache');

goog.require('bot.appcache');


/**
 * Returns the status of the appcache.
 * @return {number} status of the appcache.
 */
webdriver.atoms.storage.appcache.getStatus = function() {
  return bot.appcache.getStatus();
};
// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utility functions for accessing HTML5 localStorage object.
 * These functions are wrapper of the functions of individual method of
 * bot.storage.Storage class. An extra redirection is used to define
 * individual functional unit (atom) for injecting in Webdriver.
 */

goog.provide('webdriver.atoms.storage.local');

goog.require('bot.storage');


/**
 * Utility function to set the value of a key/value pair in localStorage.
 * @param {string} key The key of the item.
 * @param {*} value The value of the item.
 */
webdriver.atoms.storage.local.setItem = function(key, value) {
  bot.storage.getLocalStorage().setItem(key, value);
};


/**
 * Returns the value item of a key in the localStorage object.
 * @param {string} key The key of the returned value.
 * @return {?string} The mapped value if present in the localStorage object,
 *     otherwise null.
 */
webdriver.atoms.storage.local.getItem = function(key) {
  return bot.storage.getLocalStorage().getItem(key);
};


/**
 * Returns an array of keys of all keys of the localStorage object.
 * @return {Array.<string>} The array of stored keys.
 */
webdriver.atoms.storage.local.keySet = function() {
  return bot.storage.getLocalStorage().keySet();
};


/**
 * Removes an item with a given key.
 * @param {string} key The key of the key/value pair.
 * @return {?string} The removed value if present, otherwise null.
 */
webdriver.atoms.storage.local.removeItem = function(key) {
  return bot.storage.getLocalStorage().removeItem(key);
};


/**
 * Removes all items from the localStorage object.
 */
webdriver.atoms.storage.local.clear = function() {
  bot.storage.getLocalStorage().clear();
};


/**
 * Returns the number of items in the localStorage object.
 * @return {number} The number of the key/value pairs.
 */
webdriver.atoms.storage.local.size = function() {
  return bot.storage.getLocalStorage().size();
};


/**
 * Returns the key item of the key/value pairs in the localStorage object
 * of a given index.
 * @param {number} index The index of the key/value pair list.
 * @return {?string} The key item of a given index.
 */
webdriver.atoms.storage.local.key = function(index) {
  return bot.storage.getLocalStorage().key(index);
};

// Copyright 2011 WebDriver committers
// Copyright 2011 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @fileoverview Utility functions for accessing HTML5 sessionStorage object.
 * These functions are wrapper of the functions of individual method of
 * bot.storage.Storage class. An extra redirection is used to define
 * individual functional unit (atom) for injecting in Webdriver.
 */

goog.provide('webdriver.atoms.storage.session');

goog.require('bot.storage');


/**
 * Utility function to set the value of a key/value pair in sessionStorage.
 * @param {string} key The key of the item.
 * @param {*} value The value of the item.
 */
webdriver.atoms.storage.session.setItem = function(key, value) {
  bot.storage.getSessionStorage().setItem(key, value);
};


/**
 * Returns the value item of a key in the sessionStorage object.
 * @param {string} key The key of the returned value.
 * @return {?string} The mapped value if present in the sessionStorage object,
 *     otherwise null.
 */
webdriver.atoms.storage.session.getItem = function(key) {
  return bot.storage.getSessionStorage().getItem(key);
};


/**
 * Returns an array of keys of all keys of the sessionStorage object.
 * @return {Array.<string>} The array of stored keys..
 */
webdriver.atoms.storage.session.keySet = function() {
  return bot.storage.getSessionStorage().keySet();
};


/**
 * Removes an item with a given key.
 * @param {string} key The key of the key/value pair.
 * @return {?string} The removed value if present, otherwise null.
 */
webdriver.atoms.storage.session.removeItem = function(key) {
  return bot.storage.getSessionStorage().removeItem(key);
};


/**
 * Removes all items from the sessionStorage object.
 */
webdriver.atoms.storage.session.clear = function() {
  bot.storage.getSessionStorage().clear();
};


/**
 * Returns the number of items in the sessionStorage object.
 * @return {number} The number of the key/value pairs.
 */
webdriver.atoms.storage.session.size = function() {
  return bot.storage.getSessionStorage().size();
};


/**
 * Returns the key item of the key/value pairs in the sessionStorage object
 * of a given index.
 * @param {number} index The index of the key/value pair list.
 * @return {?string} The key item of a given index.
 */
webdriver.atoms.storage.session.key = function(index) {
  return bot.storage.getSessionStorage().key(index);
};

/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012, Ivan De Marino <http://ivandemarino.me>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @fileoverview Ready to inject atoms for manipulating the DOM.
 */

goog.provide('phantomjs.atoms.inject.action');

goog.require('bot.action');
goog.require('bot.inject');
goog.require('goog.dom.selection');
goog.require('webdriver.atoms.element');

/**
 * Focuses on the given element if it is not already the active element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to focus on.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 * @see bot.action.focusOnElement
 */
phantomjs.atoms.inject.action.focusOnElement = function(element) {
    return bot.inject.executeScript(bot.action.focusOnElement, [element], true);
};

/**
 * Moves the mouse over the given {@code element} with a virtual mouse.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to move to.
 * @param {!{x:number,y:number}} opt_coords Mouse position relative to the element (optional).
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.moveMouse = function(element, opt_coords) {
    return bot.inject.executeScript(bot.action.moveMouse, [element, opt_coords], true);
};

/**
 * Right-clicks on the given {@code element} with a virtual mouse.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to right-click.
 * @param {!{x:number,y:number}} opt_coords Mouse position relative to the element (optional).
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.rightClick = function(element, opt_coords) {
    return bot.inject.executeScript(bot.action.rightClick, [element, opt_coords], true);
};

/**
 * Double-clicks on the given {@code element} with a virtual mouse.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to double-click.
 * @param {!{x:number,y:number}} opt_coords Mouse position relative to the element (optional).
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.doubleClick = function(element, opt_coords) {
    return bot.inject.executeScript(bot.action.doubleClick, [element, opt_coords], true);
};

/**
 * Scrolls the mouse wheel on the given {@code element} with a virtual mouse.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to scroll the mouse wheel on.
 * @param {number} ticks Number of ticks to scroll the mouse wheel;
 *   a positive number scrolls down and a negative scrolls up.
 * @param {!{x:number,y:number}} opt_coords Mouse position relative to the element (optional).
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.scrollMouse = function(element, ticks, opt_coords) {
    return bot.inject.executeScript(bot.action.scrollMouse, [element, ticks, opt_coords], true);
};

/**
 * Drags the given {@code element} by (dx, dy) with a virtual mouse.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to drag.
 * @param {number} dx Increment in x coordinate.
 * @param {number} dy Increment in y coordinate.
 * @param {!{x:number,y:number}} opt_coords Drag start position relative to the element.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.drag = function(element, dx, dy, opt_coords) {
    return bot.inject.executeScript(bot.action.drag, [element, dx, dy, opt_coords], true);
};

/**
 * Scrolls the given {@code element} in to the current viewport. Aims to do the
 * minimum scrolling necessary, but prefers too much scrolling to too little.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to scroll into view.
 * @param {!{x:number,y:number}} opt_coords Offset relative to the top-left
 *     corner of the element, to ensure is scrolled in to view.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject};
 *     whether the element is in view after scrolling.
 */
phantomjs.atoms.inject.action.scrollIntoView = function(element, opt_coords) {
    return bot.inject.executeScript(bot.action.scrollIntoView, [element, opt_coords], true);
};

/**
 * Taps on the given {@code element} with a virtual touch screen.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to tap.
 * @param {!{x:number,y:number}} opt_coords Finger position relative to the target.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.tap = function(element, opt_coords) {
    return bot.inject.executeScript(bot.action.tap, [element, opt_coords], true);
};

/**
 * Swipes the given {@code element} by (dx, dy) with a virtual touch screen.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to swipe.
 * @param {number} dx Increment in x coordinate.
 * @param {number} dy Increment in y coordinate.
 * @param {!{x:number,y:number}} opt_coords Swipe start position relative to the element.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.swipe = function(element, dx, dy, opt_coords) {
    return bot.inject.executeScript(bot.action.swipe, [element, dx, dy, opt_coords], true);
};

/**
 * Pinches the given {@code element} by the given distance with a virtual touch
 * screen. A positive distance moves two fingers inward toward each and a
 * negative distances spreds them outward. The optional coordinate is the point
 * the fingers move towards (for positive distances) or away from (for negative
 * distances); and if not provided, defaults to the center of the element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to pinch.
 * @param {number} distance The distance by which to pinch the element.
 * @param {!{x:number,y:number}} opt_coords Position relative to the element at the center of the pinch.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.pinch = function(element, distance, opt_coords) {
    return bot.inject.executeScript(bot.action.pinch, [element, distance, opt_coords], true);
};

/**
 * Rotates the given {@code element} by the given angle with a virtual touch
 * screen. A positive angle moves two fingers clockwise and a negative angle
 * moves them counter-clockwise. The optional coordinate is the point to
 * rotate around; and if not provided, defaults to the center of the element.
 *
 * @param {!{bot.inject.ELEMENT_KEY:string}} element The element to rotate.
 * @param {number} angle The angle by which to rotate the element.
 * @param {!{x:number,y:number}} opt_coords Position relative to the element at the center of the rotation.
 * @return {(string|{status: bot.ErrorCode.<number>, value: *})} A stringified {@link bot.response.ResponseObject}.
 */
phantomjs.atoms.inject.action.rotate = function(element, angle, opt_coords) {
    return bot.inject.executeScript(bot.action.rotate, [element, angle, opt_coords], true);
};
