/*jslint sloppy: true, nomen: true */
/*global exports:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2012 execjosh, http://execjosh.blogspot.com

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

var NOP = function () {}

/**
 * spawn(command, [args], [options])
 */
exports.spawn = function (cmd, args, opts) {
  var ctx = newContext()

  if (null == opts) {
    opts = {}
  }

  opts.encoding = opts.encoding || "utf8"
  ctx._setEncoding(opts.encoding)

  ctx._start(cmd, args)

  return ctx
}

/**
 * exec(command, [options], callback)
 */
exports.exec = function (cmd, opts, cb) {
  if (null == cb) {
    cb = NOP
  }

  return cb(new Error("NotYetImplemented"))
}

/**
 * execFile(file, args, options, callback)
 */
exports.execFile = function (cmd, args, opts, cb) {
  var ctx = newContext()

  if (null == cb) {
    cb = NOP
  }

  if (null == opts) {
    opts = {}
  }

  opts.encoding = opts.encoding || "utf8"
  ctx._setEncoding(opts.encoding)

  var stdout = ""
  ctx.stdout.on("data", function (chunk) {
    stdout += chunk
  })

  var stderr = ""
  ctx.stderr.on("data", function (chunk) {
    stderr += chunk
  })

  ctx.on("exit", function (code) {
    return cb(null, stdout, stderr)
  })

  ctx._start(cmd, args)

  return ctx
}

/**
 * fork(modulePath, [args], [options])
 */
exports.fork = function (modulePath, args, opts) {
  throw new Error("NotYetImplemented")
}


// private

function newContext() {
  var ctx = exports._createChildProcessContext()

  // TODO: "Buffer" the signals and redispatch them?

  ctx.on = function (evt, cb) {
    var handler

    switch (evt) {
      case "exit":
        handler = ctx[evt]
        break
      default:
        break
    }

    // Connect the callback to the signal
    if (isFunction(handler)) {
      handler.connect(cb)
    }
  }

  ctx.stdout = new FakeReadableStream("stdout")
  ctx.stderr = new FakeReadableStream("stderr")

  // Emulates `Readable Stream`
  function FakeReadableStream(streamName) {
    this.on = function (evt, cb) {
      switch (evt) {
        case 'data':
          ctx[streamName + "Data"].connect(cb)
          break
        default:
          break
      }
    }
  }

  ctx.stdin = new FakeWritableStream()

  // Emulates `Writable Stream`
  function FakeWritableStream() {
    /**
     * @param chunk String Data to write.
     * @param encoding String Optional.  Defaults to "utf8".
     * @returns Number Bytes written; `-1` for failure.
     */
    this.write = function write(chunk, encoding) {
      if ("string" !== typeof encoding) {
        encoding = "utf8"
      }

      var bytesWritten = ctx._write(chunk, encoding)

      return bytesWritten
    }

    this.close = function close() {
        ctx._close();
    }

    this.end = function () {
        ctx._close();
    }
  }

  return ctx
}

function delayCallback() {
  var args = 0 < arguments.length ? [].slice.call(arguments, 0) : []
  var fn = args.shift()
  if (!isFunc(fn)) {
    return
  }
  var that = this
  setTimeout(function () {
    fn.apply(that, args)
  }, 0)
}

function isFunction(o) {
  return typeof o === 'function'
}
