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

/**
 * Collection of Core JavaScript utility functionalities.
 */

// Namespace "utils.core"
var utils = utils || {};
utils.core = utils.core || {};

/**
 * Wait until the test condition is true or a timeout occurs. Useful for waiting
 * on a server response or for a ui change (fadeIn, etc.) to occur.
 *
 * @param check javascript condition that evaluates to a boolean.
 * @param onTestPass what to do when 'check' condition is fulfilled.
 * @param onTimeout what to do when 'check' condition is not fulfilled and 'timeoutMs' has passed
 * @param timeoutMs the max amount of time to wait. Default value is 3 seconds
 * @param freqMs how frequently to repeat 'check'. Default value is 250 milliseconds
 */
utils.core.waitfor = function(check, onTestPass, onTimeout, timeoutMs, freqMs) {
    var timeoutMs = timeoutMs || 3000,      //< Default Timeout is 3s
        freqMs = freqMs || 250,             //< Default Freq is 250ms
        start = Date.now(),
        condition = false,
        timer = setTimeout(function() {
            var elapsedMs = Date.now() - start;
            if ((elapsedMs - start < timeoutMs) && !condition) {
                // If not time-out yet and condition not yet fulfilled
                condition = check(elapsedMs);
                timer = setTimeout(arguments.callee, freqMs);
            } else {
                clearTimeout(timer); //< house keeping
                if (!condition) {
                    // If condition still not fulfilled (timeout but condition is 'false')
                    onTimeout(elapsedMs);
                } else {
                    // Condition fulfilled (timeout and/or condition is 'true')
                    onTestPass(elapsedMs);
                }
            }
        }, freqMs);
};