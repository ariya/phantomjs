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

ghostdriver.SessionManagerReqHand = function() {
    // private:
    var
    _protoParent = ghostdriver.SessionManagerReqHand.prototype,
    _sessions = {}, //< will store key/value pairs like 'SESSION_ID : SESSION_OBJECT'
    _sessionRHs = {},
    _errors = _protoParent.errors,
    _CLEANUP_WINDOWLESS_SESSIONS_TIMEOUT = 300000, // 5 minutes
    _log = ghostdriver.logger.create("SessionManagerReqHand"),

    _handle = function(req, res) {
        _protoParent.handle.call(this, req, res);

        if (req.urlParsed.chunks.length === 1 && req.urlParsed.file === "session" && req.method === "POST") {
            _postNewSessionCommand(req, res);
            return;
        } else if (req.urlParsed.chunks.length === 1 && req.urlParsed.file === "sessions" && req.method === "GET") {
            _getActiveSessionsCommand(req, res);
            return;
        } else if (req.urlParsed.directory === "/session/") {
            if (req.method === "GET") {
                _getSessionCapabilitiesCommand(req, res);
            } else if (req.method === "DELETE") {
                _deleteSessionCommand(req, res);
            }
            return;
        }

        throw _errors.createInvalidReqInvalidCommandMethodEH(req);
    },

    _postNewSessionCommand = function(req, res) {
        var newSession,
            postObj,
            redirectToHost;

        try {
            postObj = JSON.parse(req.post);
        } catch (e) {
            // If the parsing has failed, the error is reported at the end
        }

        if (typeof(postObj) === "object" &&
            typeof(postObj.desiredCapabilities) === "object") {
            // Create and store a new Session
            newSession = new ghostdriver.Session(postObj.desiredCapabilities);
            _sessions[newSession.getId()] = newSession;

            _log.info("_postNewSessionCommand", "New Session Created: " + newSession.getId());

            // Return newly created Session Capabilities
            res.success(newSession.getId(), newSession.getCapabilities());
            return;
        }

        throw _errors.createInvalidReqMissingCommandParameterEH(req);
    },

    _getActiveSessionsCommand = function(req, res) {
        var activeSessions = [],
            sessionId;

        // Create array of format '[{ "id" : SESSION_ID, "capabilities" : SESSION_CAPABILITIES_OBJECT }]'
        for (sessionId in _sessions) {
            activeSessions.push({
                "id" : sessionId,
                "capabilities" : _sessions[sessionId].getCapabilities()
            });
        }

        res.success(null, activeSessions);
    },

    _deleteSession = function(sessionId) {
        if (typeof(_sessions[sessionId]) !== "undefined") {
            // Prepare the session to be deleted
            _sessions[sessionId].aboutToDelete();
            // Delete the session and the handler
            delete _sessions[sessionId];
            delete _sessionRHs[sessionId];
        }
    },

    _deleteSessionCommand = function(req, res) {
        var sId = req.urlParsed.file;

        if (sId === "")
            throw _errors.createInvalidReqMissingCommandParameterEH(req);

        if (typeof(_sessions[sId]) !== "undefined") {
            _deleteSession(sId);
            res.success(sId);
        } else {
            throw _errors.createInvalidReqVariableResourceNotFoundEH(req);
        }
    },

    _getSessionCapabilitiesCommand = function(req, res) {
        var sId = req.urlParsed.file,
            session;

        if (sId === "")
            throw _errors.createInvalidReqMissingCommandParameterEH(req);

        session = _getSession(sId);
        if (session !== null) {
            res.success(sId, _sessions[sId].getCapabilities());
        } else {
            throw _errors.createInvalidReqVariableResourceNotFoundEH(req);
        }
    },

    _getSession = function(sessionId) {
        if (typeof(_sessions[sessionId]) !== "undefined") {
            return _sessions[sessionId];
        }
        return null;
    },

    _getSessionReqHand = function(sessionId) {
        if (_getSession(sessionId) !== null) {
            // The session exists: what about the relative Session Request Handler?
            if (typeof(_sessionRHs[sessionId]) === "undefined") {
                _sessionRHs[sessionId] = new ghostdriver.SessionReqHand(_getSession(sessionId));
            }
            return _sessionRHs[sessionId];
        }
        return null;
    },

    _cleanupWindowlessSessions = function() {
        var sId;

        // Do this cleanup only if there are sessions
        if (Object.keys(_sessions).length > 0) {
            _log.info("_cleanupWindowlessSessions", "Asynchronous Sessions clean-up phase starting NOW");
            for (sId in _sessions) {
                if (_sessions[sId].getWindowsCount() === 0) {
                    _deleteSession(sId);
                    _log.info("_cleanupWindowlessSessions", "Deleted Session '"+sId+"', because windowless");
                }
            }
        }
    };

    // Regularly cleanup un-used sessions
    setInterval(_cleanupWindowlessSessions, _CLEANUP_WINDOWLESS_SESSIONS_TIMEOUT);

    // public:
    return {
        handle : _handle,
        getSession : _getSession,
        getSessionReqHand : _getSessionReqHand
    };
};
// prototype inheritance:
ghostdriver.SessionManagerReqHand.prototype = new ghostdriver.RequestHandler();
