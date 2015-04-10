# Copyright (C) 2009 Google Inc. All rights reserved.
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

import re

from django.template.defaultfilters import stringfilter
from google.appengine.ext import webapp

register = webapp.template.create_template_register()

bug_regexp = re.compile(r"bug (?P<bug_id>\d+)")
patch_regexp = re.compile(r"patch (?P<patch_id>\d+)")


@register.filter
@stringfilter
def webkit_linkify(value):
    value = bug_regexp.sub(r'<a href="http://webkit.org/b/\g<bug_id>">bug \g<bug_id></a>', value)
    value = patch_regexp.sub(r'<a href="https://bugs.webkit.org/attachment.cgi?id=\g<patch_id>&action=prettypatch">patch \g<patch_id></a>', value)
    return value


@register.filter
@stringfilter
def webkit_bug_id(value):
    return '<a href="http://webkit.org/b/%s">%s</a>' % (value, value)


@register.filter
@stringfilter
def webkit_attachment_id(value):
    return '<a href="https://bugs.webkit.org/attachment.cgi?id=%s&action=prettypatch">%s</a>' % (value, value)


@register.filter
@stringfilter
def results_link(status_id):
    return '<a href="/results/%s">results</a>' % status_id


@register.filter
@stringfilter
def queue_status_link(queue_name, text):
    return '<a href="/queue-status/%s">%s</a>' % (queue_name, text)


@register.filter
@stringfilter
def queue_charts_link(queue_name, text):
    return '<a href="/queue-charts/%s">%s</a>' % (queue_name, text)
