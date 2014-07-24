# Copyright (C) 2010, Google Inc. All rights reserved.
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

"""GDB support for WebKit types.

Add this to your gdb by amending your ~/.gdbinit as follows:
  python
  import sys
  sys.path.insert(0, "/path/to/tools/gdb/")
  import webkit
"""

import gdb
import re
import struct


def guess_string_length(ptr):
    """Guess length of string pointed by ptr.

    Returns a tuple of (length, an error message).
    """
    # Try to guess at the length.
    for i in xrange(0, 2048):
        try:
            if int((ptr + i).dereference()) == 0:
                return i, ''
        except RuntimeError:
            # We indexed into inaccessible memory; give up.
            return i, ' (gdb hit inaccessible memory)'
    return 256, ' (gdb found no trailing NUL)'


def ustring_to_string(ptr, length=None):
    """Convert a pointer to UTF-16 data into a Python string encoded with utf-8.

    ptr and length are both gdb.Value objects.
    If length is unspecified, will guess at the length."""
    error_message = ''
    if length is None:
        length, error_message = guess_string_length(ptr)
    else:
        length = int(length)
    char_vals = [int((ptr + i).dereference()) for i in xrange(length)]
    string = struct.pack('H' * length, *char_vals).decode('utf-16', 'replace').encode('utf-8')
    return string + error_message


def lstring_to_string(ptr, length=None):
    """Convert a pointer to LChar* data into a Python (non-Unicode) string.

    ptr and length are both gdb.Value objects.
    If length is unspecified, will guess at the length."""
    error_message = ''
    if length is None:
        length, error_message = guess_string_length(ptr)
    else:
        length = int(length)
    string = ''.join([chr((ptr + i).dereference()) for i in xrange(length)])
    return string + error_message


class StringPrinter(object):
    "Shared code between different string-printing classes"
    def __init__(self, val):
        self.val = val

    def display_hint(self):
        return 'string'


class UCharStringPrinter(StringPrinter):
    "Print a UChar*; we must guess at the length"
    def to_string(self):
        return ustring_to_string(self.val)


class LCharStringPrinter(StringPrinter):
    "Print a LChar*; we must guess at the length"
    def to_string(self):
        return lstring_to_string(self.val)


class WTFAtomicStringPrinter(StringPrinter):
    "Print a WTF::AtomicString"
    def to_string(self):
        return self.val['m_string']


class WTFCStringPrinter(StringPrinter):
    "Print a WTF::CString"
    def to_string(self):
        # The CString holds a buffer, which is a refptr to a WTF::CStringBuffer.
        data = self.val['m_buffer']['m_ptr']['m_data'].cast(gdb.lookup_type('char').pointer())
        length = self.val['m_buffer']['m_ptr']['m_length']
        return ''.join([chr((data + i).dereference()) for i in xrange(length)])


class WTFStringImplPrinter(StringPrinter):
    "Print a WTF::StringImpl"
    def get_length(self):
        return self.val['m_length']

    def to_string(self):
        if self.is_8bit():
            return lstring_to_string(self.val['m_data8'], self.get_length())
        return ustring_to_string(self.val['m_data16'], self.get_length())

    def is_8bit(self):
        return self.val['m_hashAndFlags'] & self.val['s_hashFlag8BitBuffer']


class WTFStringPrinter(StringPrinter):
    "Print a WTF::String"
    def stringimpl_ptr(self):
        return self.val['m_impl']['m_ptr']

    def get_length(self):
        if not self.stringimpl_ptr():
            return 0
        return WTFStringImplPrinter(self.stringimpl_ptr().dereference()).get_length()

    def to_string(self):
        if not self.stringimpl_ptr():
            return '(null)'
        return self.stringimpl_ptr().dereference()



class JSCIdentifierPrinter(StringPrinter):
    "Print a JSC::Identifier"
    def to_string(self):
        return WTFStringPrinter(self.val['m_string']).to_string()


class JSCJSStringPrinter(StringPrinter):
    "Print a JSC::JSString"
    def to_string(self):
        if self.val['m_length'] == 0:
            return ''

        return WTFStringImplPrinter(self.val['m_value']).to_string()


class WebCoreKURLGooglePrivatePrinter(StringPrinter):
    "Print a WebCore::KURLGooglePrivate"
    def to_string(self):
        return WTFCStringPrinter(self.val['m_utf8']).to_string()


class WebCoreQualifiedNamePrinter(StringPrinter):
    "Print a WebCore::QualifiedName"

    def __init__(self, val):
        super(WebCoreQualifiedNamePrinter, self).__init__(val)
        self.prefix_length = 0
        self.length = 0
        if self.val['m_impl']:
            self.prefix_printer = WTFStringPrinter(
                self.val['m_impl']['m_prefix']['m_string'])
            self.local_name_printer = WTFStringPrinter(
                self.val['m_impl']['m_localName']['m_string'])
            self.prefix_length = self.prefix_printer.get_length()
            if self.prefix_length > 0:
                self.length = (self.prefix_length + 1 +
                    self.local_name_printer.get_length())
            else:
                self.length = self.local_name_printer.get_length()

    def get_length(self):
        return self.length

    def to_string(self):
        if self.get_length() == 0:
            return "(null)"
        else:
            if self.prefix_length > 0:
                return (self.prefix_printer.to_string() + ":" +
                    self.local_name_printer.to_string())
            else:
                return self.local_name_printer.to_string()


class WTFVectorPrinter:
    """Pretty Printer for a WTF::Vector.

    The output of this pretty printer is similar to the output of std::vector's
    pretty printer, which is bundled in gcc.

    Example gdb session should look like:
    (gdb) p v
    $3 = WTF::Vector of length 7, capacity 16 = {7, 17, 27, 37, 47, 57, 67}
    (gdb) set print elements 3
    (gdb) p v
    $6 = WTF::Vector of length 7, capacity 16 = {7, 17, 27...}
    (gdb) set print array
    (gdb) p v
    $7 = WTF::Vector of length 7, capacity 16 = {
      7,
      17,
      27
      ...
    }
    (gdb) set print elements 200
    (gdb) p v
    $8 = WTF::Vector of length 7, capacity 16 = {
      7,
      17,
      27,
      37,
      47,
      57,
      67
    }
    """

    class Iterator:
        def __init__(self, start, finish):
            self.item = start
            self.finish = finish
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.item == self.finish:
                raise StopIteration
            count = self.count
            self.count += 1
            element = self.item.dereference()
            self.item += 1
            return ('[%d]' % count, element)

    def __init__(self, val):
        self.val = val

    def children(self):
        start = self.val['m_buffer']
        return self.Iterator(start, start + self.val['m_size'])

    def to_string(self):
        return ('%s of length %d, capacity %d'
                % ('WTF::Vector', self.val['m_size'], self.val['m_capacity']))

    def display_hint(self):
        return 'array'

def add_pretty_printers():
    pretty_printers = (
        (re.compile("^WTF::Vector<.*>$"), WTFVectorPrinter),
        (re.compile("^WTF::AtomicString$"), WTFAtomicStringPrinter),
        (re.compile("^WTF::CString$"), WTFCStringPrinter),
        (re.compile("^WTF::String$"), WTFStringPrinter),
        (re.compile("^WTF::StringImpl$"), WTFStringImplPrinter),
        (re.compile("^WebCore::KURLGooglePrivate$"), WebCoreKURLGooglePrivatePrinter),
        (re.compile("^WebCore::QualifiedName$"), WebCoreQualifiedNamePrinter),
        (re.compile("^JSC::Identifier$"), JSCIdentifierPrinter),
        (re.compile("^JSC::JSString$"), JSCJSStringPrinter),
    )

    def lookup_function(val):
        """Function used to load pretty printers; will be passed to GDB."""
        type = val.type
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()
        type = type.unqualified().strip_typedefs()
        tag = type.tag
        if tag:
            for function, pretty_printer in pretty_printers:
                if function.search(tag):
                    return pretty_printer(val)

        if type.code == gdb.TYPE_CODE_PTR:
            name = str(type.target().unqualified())
            if name == 'UChar':
                return UCharStringPrinter(val)
            if name == 'LChar':
                return LCharStringPrinter(val)
        return None

    gdb.pretty_printers.append(lookup_function)


add_pretty_printers()


class PrintPathToRootCommand(gdb.Command):
    """Command for printing WebKit Node trees.

    Usage: printpathtoroot variable_name"""

    def __init__(self):
        super(PrintPathToRootCommand, self).__init__("printpathtoroot",
            gdb.COMMAND_SUPPORT,
            gdb.COMPLETE_NONE)

    def invoke(self, arg, from_tty):
        element_type = gdb.lookup_type('WebCore::Element')
        node_type = gdb.lookup_type('WebCore::Node')
        frame = gdb.selected_frame()
        try:
            val = gdb.Frame.read_var(frame, arg)
        except:
            print "No such variable, or invalid type"
            return

        target_type = str(val.type.target().strip_typedefs())
        if target_type == str(node_type):
            stack = []
            while val:
                stack.append([val,
                    val.cast(element_type.pointer()).dereference()['m_tagName']])
                val = val.dereference()['m_parent']

            padding = ''
            while len(stack) > 0:
                pair = stack.pop()
                print padding, pair[1], pair[0]
                padding = padding + '  '
        else:
            print 'Sorry: I don\'t know how to deal with %s yet.' % target_type


PrintPathToRootCommand()
