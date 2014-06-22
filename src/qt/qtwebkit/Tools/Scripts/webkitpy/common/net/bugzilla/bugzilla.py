# Copyright (c) 2011 Google Inc. All rights reserved.
# Copyright (c) 2009 Apple Inc. All rights reserved.
# Copyright (c) 2010 Research In Motion Limited. All rights reserved.
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
#
# WebKit's Python module for interacting with Bugzilla

import logging
import mimetypes
import re
import StringIO
import socket
import urllib

from datetime import datetime # used in timestamp()

from .attachment import Attachment
from .bug import Bug

from webkitpy.common.config import committers
import webkitpy.common.config.urls as config_urls
from webkitpy.common.net.credentials import Credentials
from webkitpy.common.system.user import User
from webkitpy.thirdparty.BeautifulSoup import BeautifulSoup, BeautifulStoneSoup, SoupStrainer

_log = logging.getLogger(__name__)


class EditUsersParser(object):
    def __init__(self):
        self._group_name_to_group_string_cache = {}

    def _login_and_uid_from_row(self, row):
        first_cell = row.find("td")
        # The first row is just headers, we skip it.
        if not first_cell:
            return None
        # When there were no results, we have a fake "<none>" entry in the table.
        if first_cell.find(text="<none>"):
            return None
        # Otherwise the <td> contains a single <a> which contains the login name or a single <i> with the string "<none>".
        anchor_tag = first_cell.find("a")
        login = unicode(anchor_tag.string).strip()
        user_id = int(re.search(r"userid=(\d+)", str(anchor_tag['href'])).group(1))
        return (login, user_id)

    def login_userid_pairs_from_edit_user_results(self, results_page):
        soup = BeautifulSoup(results_page, convertEntities=BeautifulStoneSoup.HTML_ENTITIES)
        results_table = soup.find(id="admin_table")
        login_userid_pairs = [self._login_and_uid_from_row(row) for row in results_table('tr')]
        # Filter out None from the logins.
        return filter(lambda pair: bool(pair), login_userid_pairs)

    def _group_name_and_string_from_row(self, row):
        label_element = row.find('label')
        group_string = unicode(label_element['for'])
        group_name = unicode(label_element.find('strong').string).rstrip(':')
        return (group_name, group_string)

    def user_dict_from_edit_user_page(self, page):
        soup = BeautifulSoup(page, convertEntities=BeautifulStoneSoup.HTML_ENTITIES)
        user_table = soup.find("table", {'class': 'main'})
        user_dict = {}
        for row in user_table('tr'):
            label_element = row.find('label')
            if not label_element:
                continue  # This must not be a row we know how to parse.
            if row.find('table'):
                continue  # Skip the <tr> holding the groups table.

            key = label_element['for']
            if "group" in key:
                key = "groups"
                value = user_dict.get('groups', set())
                # We must be parsing a "tr" inside the inner group table.
                (group_name, _) = self._group_name_and_string_from_row(row)
                if row.find('input', {'type': 'checkbox', 'checked': 'checked'}):
                    value.add(group_name)
            else:
                value = unicode(row.find('td').string).strip()
            user_dict[key] = value
        return user_dict

    def _group_rows_from_edit_user_page(self, edit_user_page):
        soup = BeautifulSoup(edit_user_page, convertEntities=BeautifulSoup.HTML_ENTITIES)
        return soup('td', {'class': 'groupname'})

    def group_string_from_name(self, edit_user_page, group_name):
        # Bugzilla uses "group_NUMBER" strings, which may be different per install
        # so we just look them up once and cache them.
        if not self._group_name_to_group_string_cache:
            rows = self._group_rows_from_edit_user_page(edit_user_page)
            name_string_pairs = map(self._group_name_and_string_from_row, rows)
            self._group_name_to_group_string_cache = dict(name_string_pairs)
        return self._group_name_to_group_string_cache[group_name]


def timestamp():
    return datetime.now().strftime("%Y%m%d%H%M%S")


# A container for all of the logic for making and parsing bugzilla queries.
class BugzillaQueries(object):

    def __init__(self, bugzilla):
        self._bugzilla = bugzilla

    def _is_xml_bugs_form(self, form):
        # ClientForm.HTMLForm.find_control throws if the control is not found,
        # so we do a manual search instead:
        return "xml" in [control.id for control in form.controls]

    # This is kinda a hack.  There is probably a better way to get this information from bugzilla.
    def _parse_result_count(self, results_page):
        result_count_text = BeautifulSoup(results_page).find(attrs={'class': 'bz_result_count'}).string
        result_count_parts = result_count_text.strip().split(" ")
        if result_count_parts[0] == "Zarro":
            return 0
        if result_count_parts[0] == "One":
            return 1
        return int(result_count_parts[0])

    # Note: _load_query, _fetch_bug and _fetch_bugs_from_advanced_query
    # are the only methods which access self._bugzilla.

    def _load_query(self, query):
        self._bugzilla.authenticate()
        full_url = "%s%s" % (config_urls.bug_server_url, query)
        return self._bugzilla.browser.open(full_url)

    def _fetch_bugs_from_advanced_query(self, query):
        results_page = self._load_query(query)
        # Some simple searches can return a single result.
        results_url = results_page.geturl()
        if results_url.find("/show_bug.cgi?id=") != -1:
            bug_id = int(results_url.split("=")[-1])
            return [self._fetch_bug(bug_id)]
        if not self._parse_result_count(results_page):
            return []
        # Bugzilla results pages have an "XML" submit button at the bottom
        # which can be used to get an XML page containing all of the <bug> elements.
        # This is slighty lame that this assumes that _load_query used
        # self._bugzilla.browser and that it's in an acceptable state.
        self._bugzilla.browser.select_form(predicate=self._is_xml_bugs_form)
        bugs_xml = self._bugzilla.browser.submit()
        return self._bugzilla._parse_bugs_from_xml(bugs_xml)

    def _fetch_bug(self, bug_id):
        return self._bugzilla.fetch_bug(bug_id)

    def _fetch_bug_ids_advanced_query(self, query):
        soup = BeautifulSoup(self._load_query(query))
        # The contents of the <a> inside the cells in the first column happen
        # to be the bug id.
        return [int(bug_link_cell.find("a").string)
                for bug_link_cell in soup('td', "first-child")]

    def _parse_attachment_ids_request_query(self, page):
        digits = re.compile("\d+")
        attachment_href = re.compile("attachment.cgi\?id=\d+&action=review")
        attachment_links = SoupStrainer("a", href=attachment_href)
        return [int(digits.search(tag["href"]).group(0))
                for tag in BeautifulSoup(page, parseOnlyThese=attachment_links)]

    def _fetch_attachment_ids_request_query(self, query):
        return self._parse_attachment_ids_request_query(self._load_query(query))

    def _parse_quips(self, page):
        soup = BeautifulSoup(page, convertEntities=BeautifulSoup.HTML_ENTITIES)
        quips = soup.find(text=re.compile(r"Existing quips:")).findNext("ul").findAll("li")
        return [unicode(quip_entry.string) for quip_entry in quips]

    def fetch_quips(self):
        return self._parse_quips(self._load_query("/quips.cgi?action=show"))

    # List of all r+'d bugs.
    def fetch_bug_ids_from_pending_commit_list(self):
        needs_commit_query_url = "buglist.cgi?query_format=advanced&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=flagtypes.name&type0-0-0=equals&value0-0-0=review%2B"
        return self._fetch_bug_ids_advanced_query(needs_commit_query_url)

    def fetch_bugs_matching_quicksearch(self, search_string):
        # We may want to use a more explicit query than "quicksearch".
        # If quicksearch changes we should probably change to use
        # a normal buglist.cgi?query_format=advanced query.
        quicksearch_url = "buglist.cgi?quicksearch=%s" % urllib.quote(search_string)
        return self._fetch_bugs_from_advanced_query(quicksearch_url)

    # Currently this returns all bugs across all components.
    # In the future we may wish to extend this API to construct more restricted searches.
    def fetch_bugs_matching_search(self, search_string):
        query = "buglist.cgi?query_format=advanced"
        if search_string:
            query += "&short_desc_type=allwordssubstr&short_desc=%s" % urllib.quote(search_string)
        return self._fetch_bugs_from_advanced_query(query)

    def fetch_patches_from_pending_commit_list(self):
        return sum([self._fetch_bug(bug_id).reviewed_patches()
            for bug_id in self.fetch_bug_ids_from_pending_commit_list()], [])

    def fetch_bugs_from_review_queue(self, cc_email=None):
        query = "buglist.cgi?query_format=advanced&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=flagtypes.name&type0-0-0=equals&value0-0-0=review?"

        if cc_email:
            query += "&emailcc1=1&emailtype1=substring&email1=%s" % urllib.quote(cc_email)

        return self._fetch_bugs_from_advanced_query(query)

    def fetch_bug_ids_from_commit_queue(self):
        commit_queue_url = "buglist.cgi?query_format=advanced&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=flagtypes.name&type0-0-0=equals&value0-0-0=commit-queue%2B&order=Last+Changed"
        return self._fetch_bug_ids_advanced_query(commit_queue_url)

    def fetch_patches_from_commit_queue(self):
        # This function will only return patches which have valid committers
        # set.  It won't reject patches with invalid committers/reviewers.
        return sum([self._fetch_bug(bug_id).commit_queued_patches()
                    for bug_id in self.fetch_bug_ids_from_commit_queue()], [])

    def fetch_bug_ids_from_review_queue(self):
        review_queue_url = "buglist.cgi?query_format=advanced&bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=flagtypes.name&type0-0-0=equals&value0-0-0=review?"
        return self._fetch_bug_ids_advanced_query(review_queue_url)

    # This method will make several requests to bugzilla.
    def fetch_patches_from_review_queue(self, limit=None):
        # [:None] returns the whole array.
        return sum([self._fetch_bug(bug_id).unreviewed_patches()
            for bug_id in self.fetch_bug_ids_from_review_queue()[:limit]], [])

    # NOTE: This is the only client of _fetch_attachment_ids_request_query
    # This method only makes one request to bugzilla.
    def fetch_attachment_ids_from_review_queue(self):
        review_queue_url = "request.cgi?action=queue&type=review&group=type"
        return self._fetch_attachment_ids_request_query(review_queue_url)

    # This only works if your account has edituser privileges.
    # We could easily parse https://bugs.webkit.org/userprefs.cgi?tab=permissions to
    # check permissions, but bugzilla will just return an error if we don't have them.
    def fetch_login_userid_pairs_matching_substring(self, search_string):
        review_queue_url = "editusers.cgi?action=list&matchvalue=login_name&matchstr=%s&matchtype=substr" % urllib.quote(search_string)
        results_page = self._load_query(review_queue_url)
        # We could pull the EditUsersParser off Bugzilla if needed.
        return EditUsersParser().login_userid_pairs_from_edit_user_results(results_page)

    # FIXME: We should consider adding a BugzillaUser class.
    def fetch_logins_matching_substring(self, search_string):
        pairs = self.fetch_login_userid_pairs_matching_substring(search_string)
        return map(lambda pair: pair[0], pairs)


class Bugzilla(object):
    def __init__(self, committers=committers.CommitterList()):
        self.authenticated = False
        self.queries = BugzillaQueries(self)
        self.committers = committers
        self.cached_quips = []
        self.edit_user_parser = EditUsersParser()
        self._browser = None

    def _get_browser(self):
        if not self._browser:
            self.setdefaulttimeout(600)
            from webkitpy.thirdparty.autoinstalled.mechanize import Browser
            self._browser = Browser()
            # Ignore bugs.webkit.org/robots.txt until we fix it to allow this script.
            self._browser.set_handle_robots(False)
        return self._browser

    def _set_browser(self, value):
        self._browser = value

    browser = property(_get_browser, _set_browser)

    def setdefaulttimeout(self, value):
        socket.setdefaulttimeout(value)

    def fetch_user(self, user_id):
        self.authenticate()
        edit_user_page = self.browser.open(self.edit_user_url_for_id(user_id))
        return self.edit_user_parser.user_dict_from_edit_user_page(edit_user_page)

    def add_user_to_groups(self, user_id, group_names):
        self.authenticate()
        user_edit_page = self.browser.open(self.edit_user_url_for_id(user_id))
        self.browser.select_form(nr=1)
        for group_name in group_names:
            group_string = self.edit_user_parser.group_string_from_name(user_edit_page, group_name)
            self.browser.find_control(group_string).items[0].selected = True
        self.browser.submit()

    def quips(self):
        # We only fetch and parse the list of quips once per instantiation
        # so that we do not burden bugs.webkit.org.
        if not self.cached_quips:
            self.cached_quips = self.queries.fetch_quips()
        return self.cached_quips

    def bug_url_for_bug_id(self, bug_id, xml=False):
        if not bug_id:
            return None
        content_type = "&ctype=xml&excludefield=attachmentdata" if xml else ""
        return "%sshow_bug.cgi?id=%s%s" % (config_urls.bug_server_url, bug_id, content_type)

    def short_bug_url_for_bug_id(self, bug_id):
        if not bug_id:
            return None
        return "http://webkit.org/b/%s" % bug_id

    def add_attachment_url(self, bug_id):
        return "%sattachment.cgi?action=enter&bugid=%s" % (config_urls.bug_server_url, bug_id)

    def attachment_url_for_id(self, attachment_id, action="view"):
        if not attachment_id:
            return None
        action_param = ""
        if action and action != "view":
            action_param = "&action=%s" % action
        return "%sattachment.cgi?id=%s%s" % (config_urls.bug_server_url,
                                             attachment_id,
                                             action_param)

    def edit_user_url_for_id(self, user_id):
        return "%seditusers.cgi?action=edit&userid=%s" % (config_urls.bug_server_url, user_id)

    def _parse_attachment_flag(self,
                               element,
                               flag_name,
                               attachment,
                               result_key):
        flag = element.find('flag', attrs={'name': flag_name})
        if flag:
            attachment[flag_name] = flag['status']
            if flag['status'] == '+':
                attachment[result_key] = flag['setter']
        # Sadly show_bug.cgi?ctype=xml does not expose the flag modification date.

    def _string_contents(self, soup):
        # WebKit's bugzilla instance uses UTF-8.
        # BeautifulStoneSoup always returns Unicode strings, however
        # the .string method returns a (unicode) NavigableString.
        # NavigableString can confuse other parts of the code, so we
        # convert from NavigableString to a real unicode() object using unicode().
        return unicode(soup.string)

    # Example: 2010-01-20 14:31 PST
    # FIXME: Some bugzilla dates seem to have seconds in them?
    # Python does not support timezones out of the box.
    # Assume that bugzilla always uses PST (which is true for bugs.webkit.org)
    _bugzilla_date_format = "%Y-%m-%d %H:%M:%S"

    @classmethod
    def _parse_date(cls, date_string):
        (date, time, time_zone) = date_string.split(" ")
        if time.count(':') == 1:
            # Add seconds into the time.
            time += ':0'
        # Ignore the timezone because python doesn't understand timezones out of the box.
        date_string = "%s %s" % (date, time)
        return datetime.strptime(date_string, cls._bugzilla_date_format)

    def _date_contents(self, soup):
        return self._parse_date(self._string_contents(soup))

    def _parse_attachment_element(self, element, bug_id):
        attachment = {}
        attachment['bug_id'] = bug_id
        attachment['is_obsolete'] = (element.has_key('isobsolete') and element['isobsolete'] == "1")
        attachment['is_patch'] = (element.has_key('ispatch') and element['ispatch'] == "1")
        attachment['id'] = int(element.find('attachid').string)
        # FIXME: No need to parse out the url here.
        attachment['url'] = self.attachment_url_for_id(attachment['id'])
        attachment["attach_date"] = self._date_contents(element.find("date"))
        attachment['name'] = self._string_contents(element.find('desc'))
        attachment['attacher_email'] = self._string_contents(element.find('attacher'))
        attachment['type'] = self._string_contents(element.find('type'))
        self._parse_attachment_flag(
                element, 'review', attachment, 'reviewer_email')
        self._parse_attachment_flag(
                element, 'commit-queue', attachment, 'committer_email')
        return attachment

    def _parse_log_descr_element(self, element):
        comment = {}
        comment['comment_email'] = self._string_contents(element.find('who'))
        comment['comment_date'] = self._date_contents(element.find('bug_when'))
        comment['text'] = self._string_contents(element.find('thetext'))
        return comment

    def _parse_bugs_from_xml(self, page):
        soup = BeautifulSoup(page)
        # Without the unicode() call, BeautifulSoup occasionally complains of being
        # passed None for no apparent reason.
        return [Bug(self._parse_bug_dictionary_from_xml(unicode(bug_xml)), self) for bug_xml in soup('bug')]

    def _parse_bug_dictionary_from_xml(self, page):
        soup = BeautifulStoneSoup(page, convertEntities=BeautifulStoneSoup.XML_ENTITIES)
        bug = {}
        bug["id"] = int(soup.find("bug_id").string)
        bug["title"] = self._string_contents(soup.find("short_desc"))
        bug["bug_status"] = self._string_contents(soup.find("bug_status"))
        dup_id = soup.find("dup_id")
        if dup_id:
            bug["dup_id"] = self._string_contents(dup_id)
        bug["reporter_email"] = self._string_contents(soup.find("reporter"))
        bug["assigned_to_email"] = self._string_contents(soup.find("assigned_to"))
        bug["cc_emails"] = [self._string_contents(element) for element in soup.findAll('cc')]
        bug["attachments"] = [self._parse_attachment_element(element, bug["id"]) for element in soup.findAll('attachment')]
        bug["comments"] = [self._parse_log_descr_element(element) for element in soup.findAll('long_desc')]

        return bug

    # Makes testing fetch_*_from_bug() possible until we have a better
    # BugzillaNetwork abstration.

    def _fetch_bug_page(self, bug_id):
        bug_url = self.bug_url_for_bug_id(bug_id, xml=True)
        _log.info("Fetching: %s" % bug_url)
        return self.browser.open(bug_url)

    def fetch_bug_dictionary(self, bug_id):
        try:
            return self._parse_bug_dictionary_from_xml(self._fetch_bug_page(bug_id))
        except KeyboardInterrupt:
            raise
        except:
            self.authenticate()
            return self._parse_bug_dictionary_from_xml(self._fetch_bug_page(bug_id))

    # FIXME: A BugzillaCache object should provide all these fetch_ methods.

    def fetch_bug(self, bug_id):
        return Bug(self.fetch_bug_dictionary(bug_id), self)

    def fetch_attachment_contents(self, attachment_id):
        attachment_url = self.attachment_url_for_id(attachment_id)
        # We need to authenticate to download patches from security bugs.
        self.authenticate()
        return self.browser.open(attachment_url).read()

    def _parse_bug_id_from_attachment_page(self, page):
        # The "Up" relation happens to point to the bug.
        up_link = BeautifulSoup(page).find('link', rel='Up')
        if not up_link:
            # This attachment does not exist (or you don't have permissions to
            # view it).
            return None
        match = re.search("show_bug.cgi\?id=(?P<bug_id>\d+)", up_link['href'])
        return int(match.group('bug_id'))

    def bug_id_for_attachment_id(self, attachment_id):
        self.authenticate()

        attachment_url = self.attachment_url_for_id(attachment_id, 'edit')
        _log.info("Fetching: %s" % attachment_url)
        page = self.browser.open(attachment_url)
        return self._parse_bug_id_from_attachment_page(page)

    # FIXME: This should just return Attachment(id), which should be able to
    # lazily fetch needed data.

    def fetch_attachment(self, attachment_id):
        # We could grab all the attachment details off of the attachment edit
        # page but we already have working code to do so off of the bugs page,
        # so re-use that.
        bug_id = self.bug_id_for_attachment_id(attachment_id)
        if not bug_id:
            return None
        attachments = self.fetch_bug(bug_id).attachments(include_obsolete=True)
        for attachment in attachments:
            if attachment.id() == int(attachment_id):
                return attachment
        return None # This should never be hit.

    def authenticate(self):
        if self.authenticated:
            return

        credentials = Credentials(config_urls.bug_server_host, git_prefix="bugzilla")

        attempts = 0
        while not self.authenticated:
            attempts += 1
            username, password = credentials.read_credentials()

            _log.info("Logging in as %s..." % username)
            self.browser.open(config_urls.bug_server_url +
                              "index.cgi?GoAheadAndLogIn=1")
            self.browser.select_form(name="login")
            self.browser['Bugzilla_login'] = username
            self.browser['Bugzilla_password'] = password
            self.browser.find_control("Bugzilla_restrictlogin").items[0].selected = False
            response = self.browser.submit()

            match = re.search("<title>(.+?)</title>", response.read())
            # If the resulting page has a title, and it contains the word
            # "invalid" assume it's the login failure page.
            if match and re.search("Invalid", match.group(1), re.IGNORECASE):
                errorMessage = "Bugzilla login failed: %s" % match.group(1)
                # raise an exception only if this was the last attempt
                if attempts < 5:
                    _log.error(errorMessage)
                else:
                    raise Exception(errorMessage)
            else:
                self.authenticated = True
                self.username = username

    # FIXME: Use enum instead of two booleans
    def _commit_queue_flag(self, mark_for_landing, mark_for_commit_queue):
        if mark_for_landing:
            user = self.committers.contributor_by_email(self.username)
            mark_for_commit_queue = True
            if not user:
                _log.warning("Your Bugzilla login is not listed in committers.py. Uploading with cq? instead of cq+")
                mark_for_landing = False
            elif not user.can_commit:
                _log.warning("You're not a committer yet or haven't updated committers.py yet. Uploading with cq? instead of cq+")
                mark_for_landing = False

        if mark_for_landing:
            return '+'
        if mark_for_commit_queue:
            return '?'
        return 'X'

    # FIXME: mark_for_commit_queue and mark_for_landing should be joined into a single commit_flag argument.
    def _fill_attachment_form(self,
                              description,
                              file_object,
                              mark_for_review=False,
                              mark_for_commit_queue=False,
                              mark_for_landing=False,
                              is_patch=False,
                              filename=None,
                              mimetype=None):
        self.browser['description'] = description
        if is_patch:
            self.browser['ispatch'] = ("1",)
        # FIXME: Should this use self._find_select_element_for_flag?
        self.browser['flag_type-1'] = ('?',) if mark_for_review else ('X',)
        self.browser['flag_type-3'] = (self._commit_queue_flag(mark_for_landing, mark_for_commit_queue),)

        filename = filename or "%s.patch" % timestamp()
        if not mimetype:
            mimetypes.add_type('text/plain', '.patch')  # Make sure mimetypes knows about .patch
            mimetype, _ = mimetypes.guess_type(filename)
        if not mimetype:
            mimetype = "text/plain"  # Bugzilla might auto-guess for us and we might not need this?
        self.browser.add_file(file_object, mimetype, filename, 'data')

    def _file_object_for_upload(self, file_or_string):
        if hasattr(file_or_string, 'read'):
            return file_or_string
        # Only if file_or_string is not already encoded do we want to encode it.
        if isinstance(file_or_string, unicode):
            file_or_string = file_or_string.encode('utf-8')
        return StringIO.StringIO(file_or_string)

    # timestamp argument is just for unittests.
    def _filename_for_upload(self, file_object, bug_id, extension="txt", timestamp=timestamp):
        if hasattr(file_object, "name"):
            return file_object.name
        return "bug-%s-%s.%s" % (bug_id, timestamp(), extension)

    def add_attachment_to_bug(self, bug_id, file_or_string, description, filename=None, comment_text=None, mimetype=None):
        self.authenticate()
        _log.info('Adding attachment "%s" to %s' % (description, self.bug_url_for_bug_id(bug_id)))
        self.browser.open(self.add_attachment_url(bug_id))
        self.browser.select_form(name="entryform")
        file_object = self._file_object_for_upload(file_or_string)
        filename = filename or self._filename_for_upload(file_object, bug_id)
        self._fill_attachment_form(description, file_object, filename=filename, mimetype=mimetype)
        if comment_text:
            _log.info(comment_text)
            self.browser['comment'] = comment_text
        self.browser.submit()

    # FIXME: The arguments to this function should be simplified and then
    # this should be merged into add_attachment_to_bug
    def add_patch_to_bug(self,
                         bug_id,
                         file_or_string,
                         description,
                         comment_text=None,
                         mark_for_review=False,
                         mark_for_commit_queue=False,
                         mark_for_landing=False):
        self.authenticate()
        _log.info('Adding patch "%s" to %s' % (description, self.bug_url_for_bug_id(bug_id)))

        self.browser.open(self.add_attachment_url(bug_id))
        self.browser.select_form(name="entryform")
        file_object = self._file_object_for_upload(file_or_string)
        filename = self._filename_for_upload(file_object, bug_id, extension="patch")
        self._fill_attachment_form(description,
                                   file_object,
                                   mark_for_review=mark_for_review,
                                   mark_for_commit_queue=mark_for_commit_queue,
                                   mark_for_landing=mark_for_landing,
                                   is_patch=True,
                                   filename=filename)
        if comment_text:
            _log.info(comment_text)
            self.browser['comment'] = comment_text
        self.browser.submit()

    # FIXME: There has to be a more concise way to write this method.
    def _check_create_bug_response(self, response_html):
        match = re.search("<title>Bug (?P<bug_id>\d+) Submitted[^<]*</title>",
                          response_html)
        if match:
            return match.group('bug_id')

        match = re.search(
            '<div id="bugzilla-body">(?P<error_message>.+)<div id="footer">',
            response_html,
            re.DOTALL)
        error_message = "FAIL"
        if match:
            text_lines = BeautifulSoup(
                    match.group('error_message')).findAll(text=True)
            error_message = "\n" + '\n'.join(
                    ["  " + line.strip()
                     for line in text_lines if line.strip()])
        raise Exception("Bug not created: %s" % error_message)

    def create_bug(self,
                   bug_title,
                   bug_description,
                   component=None,
                   diff=None,
                   patch_description=None,
                   cc=None,
                   blocked=None,
                   assignee=None,
                   mark_for_review=False,
                   mark_for_commit_queue=False):
        self.authenticate()

        _log.info('Creating bug with title "%s"' % bug_title)
        self.browser.open(config_urls.bug_server_url + "enter_bug.cgi?product=WebKit")
        self.browser.select_form(name="Create")
        component_items = self.browser.find_control('component').items
        component_names = map(lambda item: item.name, component_items)
        if not component:
            component = "New Bugs"
        if component not in component_names:
            component = User.prompt_with_list("Please pick a component:", component_names)
        self.browser["component"] = [component]
        if cc:
            self.browser["cc"] = cc
        if blocked:
            self.browser["blocked"] = unicode(blocked)
        if not assignee:
            assignee = self.username
        if assignee and not self.browser.find_control("assigned_to").disabled:
            self.browser["assigned_to"] = assignee
        self.browser["short_desc"] = bug_title
        self.browser["comment"] = bug_description

        if diff:
            # _fill_attachment_form expects a file-like object
            # Patch files are already binary, so no encoding needed.
            assert(isinstance(diff, str))
            patch_file_object = StringIO.StringIO(diff)
            self._fill_attachment_form(
                    patch_description,
                    patch_file_object,
                    mark_for_review=mark_for_review,
                    mark_for_commit_queue=mark_for_commit_queue,
                    is_patch=True)

        response = self.browser.submit()

        bug_id = self._check_create_bug_response(response.read())
        _log.info("Bug %s created." % bug_id)
        _log.info("%sshow_bug.cgi?id=%s" % (config_urls.bug_server_url, bug_id))
        return bug_id

    def _find_select_element_for_flag(self, flag_name):
        # FIXME: This will break if we ever re-order attachment flags
        if flag_name == "review":
            return self.browser.find_control(type='select', nr=0)
        elif flag_name == "commit-queue":
            return self.browser.find_control(type='select', nr=1)
        raise Exception("Don't know how to find flag named \"%s\"" % flag_name)

    def clear_attachment_flags(self,
                               attachment_id,
                               additional_comment_text=None):
        self.authenticate()

        comment_text = "Clearing flags on attachment: %s" % attachment_id
        if additional_comment_text:
            comment_text += "\n\n%s" % additional_comment_text
        _log.info(comment_text)

        self.browser.open(self.attachment_url_for_id(attachment_id, 'edit'))
        self.browser.select_form(nr=1)
        self.browser.set_value(comment_text, name='comment', nr=0)
        self._find_select_element_for_flag('review').value = ("X",)
        self._find_select_element_for_flag('commit-queue').value = ("X",)
        self.browser.submit()

    def set_flag_on_attachment(self,
                               attachment_id,
                               flag_name,
                               flag_value,
                               comment_text=None):
        # FIXME: We need a way to test this function on a live bugzilla
        # instance.

        self.authenticate()
        _log.info(comment_text)
        self.browser.open(self.attachment_url_for_id(attachment_id, 'edit'))
        self.browser.select_form(nr=1)

        if comment_text:
            self.browser.set_value(comment_text, name='comment', nr=0)

        self._find_select_element_for_flag(flag_name).value = (flag_value,)
        self.browser.submit()

    # FIXME: All of these bug editing methods have a ridiculous amount of
    # copy/paste code.

    def obsolete_attachment(self, attachment_id, comment_text=None):
        self.authenticate()

        _log.info("Obsoleting attachment: %s" % attachment_id)
        self.browser.open(self.attachment_url_for_id(attachment_id, 'edit'))
        self.browser.select_form(nr=1)
        self.browser.find_control('isobsolete').items[0].selected = True
        # Also clear any review flag (to remove it from review/commit queues)
        self._find_select_element_for_flag('review').value = ("X",)
        self._find_select_element_for_flag('commit-queue').value = ("X",)
        if comment_text:
            _log.info(comment_text)
            # Bugzilla has two textareas named 'comment', one is somehow
            # hidden.  We want the first.
            self.browser.set_value(comment_text, name='comment', nr=0)
        self.browser.submit()

    def add_cc_to_bug(self, bug_id, email_address_list):
        self.authenticate()

        _log.info("Adding %s to the CC list for bug %s" % (email_address_list, bug_id))
        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        self.browser["newcc"] = ", ".join(email_address_list)
        self.browser.submit()

    def post_comment_to_bug(self, bug_id, comment_text, cc=None):
        self.authenticate()

        _log.info("Adding comment to bug %s" % bug_id)
        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        self.browser["comment"] = comment_text
        if cc:
            self.browser["newcc"] = ", ".join(cc)
        self.browser.submit()

    def close_bug_as_fixed(self, bug_id, comment_text=None):
        self.authenticate()

        _log.info("Closing bug %s as fixed" % bug_id)
        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        if comment_text:
            self.browser['comment'] = comment_text
        self.browser['bug_status'] = ['RESOLVED']
        self.browser['resolution'] = ['FIXED']
        self.browser.submit()

    def _has_control(self, form, id):
        return id in [control.id for control in form.controls]

    def reassign_bug(self, bug_id, assignee=None, comment_text=None):
        self.authenticate()

        if not assignee:
            assignee = self.username

        _log.info("Assigning bug %s to %s" % (bug_id, assignee))
        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")

        if not self._has_control(self.browser, "assigned_to"):
            _log.warning("""Failed to assign bug to you (can't find assigned_to) control.
Ignore this message if you don't have EditBugs privileges (https://bugs.webkit.org/userprefs.cgi?tab=permissions)""")
            return

        if comment_text:
            _log.info(comment_text)
            self.browser["comment"] = comment_text
        self.browser["assigned_to"] = assignee
        self.browser.submit()

    def reopen_bug(self, bug_id, comment_text):
        self.authenticate()

        _log.info("Re-opening bug %s" % bug_id)
        # Bugzilla requires a comment when re-opening a bug, so we know it will
        # never be None.
        _log.info(comment_text)
        self.browser.open(self.bug_url_for_bug_id(bug_id))
        self.browser.select_form(name="changeform")
        bug_status = self.browser.find_control("bug_status", type="select")
        # This is a hack around the fact that ClientForm.ListControl seems to
        # have no simpler way to ask if a control has an item named "REOPENED"
        # without using exceptions for control flow.
        possible_bug_statuses = map(lambda item: item.name, bug_status.items)
        if "REOPENED" in possible_bug_statuses:
            bug_status.value = ["REOPENED"]
        # If the bug was never confirmed it will not have a "REOPENED"
        # state, but only an "UNCONFIRMED" state.
        elif "UNCONFIRMED" in possible_bug_statuses:
            bug_status.value = ["UNCONFIRMED"]
        else:
            # FIXME: This logic is slightly backwards.  We won't print this
            # message if the bug is already open with state "UNCONFIRMED".
            _log.info("Did not reopen bug %s, it appears to already be open with status %s." % (bug_id, bug_status.value))
        self.browser['comment'] = comment_text
        self.browser.submit()
