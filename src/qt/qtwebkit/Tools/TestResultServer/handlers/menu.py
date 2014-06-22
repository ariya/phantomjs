# Copyright (C) 2010 Google Inc. All rights reserved.
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

from google.appengine.api import users
from google.appengine.ext import webapp
from google.appengine.ext.webapp import template

dashboards = [
    ["Results", "/dashboards/flakiness_dashboard.html"],
    ["Timeline", "/dashboards/timeline_explorer.html"],
    ["Treemap", "/dashboards/treemap.html"],
    ["Stats", "/dashboards/aggregate_results.html"],
]

menu = [
    ["List of test files", "/testfile"],
    ["List of results.json files", "/testfile?name=results.json"],
    ["Upload test file", "/testfile/uploadform"],
]


class Menu(webapp.RequestHandler):
    def get(self):
        user = users.get_current_user()
        if user:
            user_email = user.email()
            login_text = "Sign out"
            login_url = users.create_logout_url(self.request.uri)
        else:
            user_email = ""
            login_text = "Sign in"
            login_url = users.create_login_url(self.request.uri)

        template_values = {
            "user_email": user_email,
            "login_text": login_text,
            "login_url": login_url,
            "menu": menu,
            "dashboards": dashboards,
        }

        self.response.out.write(
            template.render("templates/menu.html", template_values))
