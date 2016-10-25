/*
This file is part of the GhostDriver by Ivan De Marino <http://ivandemarino.me>.

Copyright (c) 2012-2014, Ivan De Marino <http://ivandemarino.me>
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

var ghostdriver = ghostdriver || {};

/**
 * This Class does first level routing: based on the REST Path, sends Request and Response to the right Request Handler.
 */
ghostdriver.RouterReqHand = function() {
    // private:
    const
    _const = {
        STATUS          : "status",
        SESSION         : "session",
        SESSIONS        : "sessions",
        SESSION_DIR     : "/session/",
        SHUTDOWN        : "shutdown"
    };

    var
    _protoParent = ghostdriver.RouterReqHand.prototype,
    _statusRH = new ghostdriver.StatusReqHand(),
    _shutdownRH = new ghostdriver.ShutdownReqHand(),
    _sessionManRH = new ghostdriver.SessionManagerReqHand(),
    _errors = _protoParent.errors,
    _log = ghostdriver.logger.create("RouterReqHand"),

    _handle = function(req, res) {
        var session,
            sessionRH;

        // Invoke parent implementation
        _protoParent.handle.call(this, req, res);

        _log.debug("_handle", JSON.stringify(req));

        try {
            if (req.urlParsed.chunks.length === 1 && req.urlParsed.file === _const.STATUS) {                 // GET '/status'
                _statusRH.handle(req, res);
            } else if (req.urlParsed.chunks.length === 1 && req.urlParsed.file === _const.SHUTDOWN) {        // GET '/shutdown'
                _shutdownRH.handle(req, res);
                phantom.exit();
            } else if ((req.urlParsed.chunks.length === 1 && req.urlParsed.file === _const.SESSION) ||         // POST '/session'
                (req.urlParsed.chunks.length === 1 && req.urlParsed.file === _const.SESSIONS) ||               // GET '/sessions'
                req.urlParsed.directory === _const.SESSION_DIR) {       // GET or DELETE '/session/:id'
                _sessionManRH.handle(req, res);
            } else if (req.urlParsed.chunks[0] === _const.SESSION) {    // GET, POST or DELETE '/session/:id/...'
                // Retrieve session
                session = _sessionManRH.getSession(req.urlParsed.chunks[1]);

                if (session !== null) {
                    // Create a new Session Request Handler and re-route the request to it
                    sessionRH = _sessionManRH.getSessionReqHand(req.urlParsed.chunks[1]);
                    _protoParent.reroute.call(sessionRH, req, res, _const.SESSION_DIR + session.getId());
                } else {
                    throw _errors.createInvalidReqVariableResourceNotFoundEH(req);
                }
            } else {
                throw _errors.createInvalidReqUnknownCommandEH(req);
            }
        } catch (e) {
            _log.error("_handle.error", JSON.stringify(e));

            if (typeof(e.handle) === "function") {
                e.handle(res);
            } else {
                // This should never happen, if we handle all the possible error scenario
                res.statusCode = 404; //< "404 Not Found"
                res.setHeader("Content-Type", "text/plain");
                res.writeAndClose(e.name + " - " + e.message);
            }
        }
    };

    // public:
    return {
        handle : _handle
    };
};
// prototype inheritance:
ghostdriver.RouterReqHand.prototype = new ghostdriver.RequestHandler();
