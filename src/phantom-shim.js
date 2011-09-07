/* jslint sloppy: true, nomen: true */
/* global phantom:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>

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

// Shim: 'phantom'

/**
 * CommonJS Modules/1.1.1 'require' implementation for PhantomJS.
 * Specs: http://wiki.commonjs.org/wiki/Modules/1.1.1
 *
 * @param moduleId Id of the Module to Load
 * @param params Parameters passed to the Module at construction time
 * @returns Loaded module
 * @throws Error if it fails to load the required module
 */
window.require = function(moduleId, params) {
	var exports = {},
		moduleSourcePath = "";
	
	// Prepopulate the "exports" object with native QObjects (if requested)
	if (moduleId === "fs") {
		exports = phantom.createFileSystem();
		moduleSourcePath = ":/builtin-modules/filesystem.js";
	} else if (moduleId === "webpage") {
		moduleSourcePath = ":/builtin-modules/webpage.js";
	} else {
		// TODO - For now we just handle plain ".js" files in the path.
		//        This needs to handle proper CommonJS Modules.
		moduleSourcePath = moduleId + ".js";
	}
	
	// Evaluate the "moduleScript"
	(function(exports){
		var moduleSource = phantom.loadModuleSource(moduleSourcePath);
		
		if (moduleSource !== "") {
			eval(moduleSource);
		} else {
			throw "Unable to load module '"+ moduleId +"'"; 
		}
	})(exports);
	
	return exports;
};

/**
 * Backward compatible "WebPage" global function.
 *
 * @deprecated
 * @param opts Options to pass to the WebPage
 * @returns An instance of 'webpage'
 */
window.WebPage = function(opts) {
    // NOTE: this might seem weird, but the idea behind is that the "new"
    //       operator is supposed to always return an object.
    //       When invoking "new WebPage()", the new operator returns the returned value,
    //       not the new function object.
    return require('webpage').create(opts);
};

