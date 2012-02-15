/*jslint sloppy: true, nomen: true */
/*global exports:true */

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

// JavaScript "shim" to throw exceptions in case a critical operation fails.

/** Open and return a "file" object.
 * It will throw exception if it fails.
 *
 * @param path Path of the file to open
 * @param modeOrOpts
 *  mode: Open Mode. A string made of 'r', 'w', 'a/+' characters.
 *  opts: Options.
 *          - mode (see Open Mode above)
 *          - charset An IANA, case insensitive, charset name.
 * @return "file" object
 */
exports.open = function (path, modeOrOpts) {
    var file, opts;

    // Extract charset from opts
    if (modeOrOpts == null) {
        // Empty options
        opts = {};
    } else if (typeof modeOrOpts !== 'object') {
        opts = {
            mode: modeOrOpts
        };
    } else {
        opts = modeOrOpts;
    }

    // Open file
    file = exports._open(path, opts);
    if (file) {
        return file;
    }
    throw "Unable to open file '" + path + "'";
};

/** Open, read and return text content of a file.
 * It will throw an exception if it fails.
 *
 * NOTE: do not use this for binary files.
 *
 * @param path Path of the file to read from
 * @param opts Options.
 *               - charset An IANA, case insensitive, charset name.
 * @return file content
 */
exports.read = function (path, opts) {
    if (opts == null || typeof opts !== 'object') {
        opts = {};
    }
    opts.mode = 'r';
    var f = exports.open(path, opts),
        content = f.read();

    f.close();
    return content;
};

/** Open, read and return raw binary content of a file.
 * It will throw an exception if it fails.
 *
 * @param path Path of the file to read from
 * @return file content
 */
exports.readRaw = function (path, opts) {
    var opts = {'mode': 'r'};
    var f = exports.open(path, opts),
        content = f.readRaw();

    f.close();
    return content;
};

/** Open and write text content to a file
 * It will throw an exception if it fails.
 *
 * @param path Path of the file to read from
 * @param content Content to write to the file
 * @param modeOrOpts
 *  mode: Open Mode. A string made of 'r', 'w', 'a/+' characters.
 *  opts: Options.
 *          - mode (see Open Mode above)
 *          - charset An IANA, case insensitive, charset name.
 */
exports.write = function (path, content, modeOrOpts) {
    if (modeOrOpts == null) {
        modeOrOpts = {};
    }
    var f = exports.open(path, modeOrOpts);

    f.write(content);
    f.close();
};

/** Open and write raw binary content to a file
 * It will throw an exception if it fails.
 *
 * @param path Path of the file to read from
 * @param content Content to write to the file
 * @param modeOrOpts
 *  mode: Open Mode. A string made of 'r', 'w', 'a/+' characters.
 *  opts: Options.
 *          - mode (see Open Mode above)
 */
exports.writeRaw = function (path, content, modeOrOpts) {
    if (modeOrOpts == null) {
        modeOrOpts = {};
    }
    var f = exports.open(path, modeOrOpts);

    f.writeRaw(content);
    f.close();
};

/** Return the size of a file, in bytes.
 * It will throw an exception if it fails.
 *
 * @param path Path of the file to read the size of
 * @return File size in bytes
 */
exports.size = function (path) {
    var size = exports._size(path);
    if (size !== -1) {
        return size;
    }
    throw "Unable to read file '" + path + "' size";
};

/** Copy a file.
 * It will throw an exception if it fails.
 *
 * @param source Path of the source file
 * @param destination Path of the destination file
 */
exports.copy = function (source, destination) {
    if (!exports._copy(source, destination)) {
        throw "Unable to copy file '" + source + "' at '" + destination + "'";
    }
};

/** Copy a directory tree.
 * It will throw an exception if it fails.
 *
 * @param source Path of the source directory tree
 * @param destination Path of the destination directory tree
 */
exports.copyTree = function (source, destination) {
    if (!exports._copyTree(source, destination)) {
        throw "Unable to copy directory tree '" + source + "' at '" + destination + "'";
    }
};

/** Move a file.
 * It will throw an exception if it fails.
 *
 * @param source Path of the source file
 * @param destination Path of the destination file
 */
exports.move = function (source, destination) {
    exports.copy(source, destination);
    exports.remove(source);
};

/** Removes a file.
 * It will throw an exception if it fails.
 *
 * @param path Path of the file to remove
 */
exports.remove = function (path) {
    if (!exports._remove(path)) {
        throw "Unable to remove file '" + path + "'";
    }
};

/** Removes a directory.
 * It will throw an exception if it fails.
 *
 * @param path Path of the directory to remove
 */
exports.removeDirectory = function (path) {
    if (!exports._removeDirectory(path)) {
        throw "Unable to remove directory '" + path + "'";
    }
};

/** Removes a directory tree.
 * It will throw an exception if it fails.
 *
 * @param path Path of the directory tree to remove
 */
exports.removeTree = function (path) {
    if (!exports._removeTree(path)) {
        throw "Unable to remove directory tree '" + path + "'";
    }
};

exports.touch = function (path) {
    exports.write(path, "", 'a');
};
