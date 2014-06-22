# Copyright (c) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from webkitpy.common.config import irc as config_irc

from webkitpy.common.thread.messagepump import MessagePump, MessagePumpDelegate
from webkitpy.thirdparty.autoinstalled.irc import ircbot
from webkitpy.thirdparty.autoinstalled.irc import irclib


class IRCBotDelegate(object):
    def irc_message_received(self, nick, message):
        raise NotImplementedError, "subclasses must implement"

    def irc_nickname(self):
        raise NotImplementedError, "subclasses must implement"

    def irc_password(self):
        raise NotImplementedError, "subclasses must implement"


class IRCBot(ircbot.SingleServerIRCBot, MessagePumpDelegate):
    # FIXME: We should get this information from a config file.
    def __init__(self,
                 message_queue,
                 delegate):
        self._message_queue = message_queue
        self._delegate = delegate
        ircbot.SingleServerIRCBot.__init__(
            self,
            [(
                config_irc.server,
                config_irc.port,
                self._delegate.irc_password()
            )],
            self._delegate.irc_nickname(),
            self._delegate.irc_nickname())
        self._channel = config_irc.channel

    # ircbot.SingleServerIRCBot methods

    def on_nicknameinuse(self, connection, event):
        connection.nick(connection.get_nickname() + "_")

    def on_welcome(self, connection, event):
        connection.join(self._channel)
        self._message_pump = MessagePump(self, self._message_queue)

    def on_pubmsg(self, connection, event):
        nick = irclib.nm_to_n(event.source())
        request = event.arguments()[0]

        if not irclib.irc_lower(request).startswith(irclib.irc_lower(connection.get_nickname())):
            return

        if len(request) <= len(connection.get_nickname()):
            return

        # Some IRC clients, like xchat-gnome, default to using a comma
        # when addressing someone.
        vocative_separator = request[len(connection.get_nickname())]
        if vocative_separator == ':':
            request = request.split(':', 1)
        elif vocative_separator == ',':
            request = request.split(',', 1)
        else:
            return

        if len(request) > 1:
            response = self._delegate.irc_message_received(nick, request[1])
            if response:
                connection.privmsg(self._channel, response)

    # MessagePumpDelegate methods

    def schedule(self, interval, callback):
        self.connection.execute_delayed(interval, callback)

    def message_available(self, message):
        self.connection.privmsg(self._channel, message)

    def final_message_delivered(self):
        self.die()
