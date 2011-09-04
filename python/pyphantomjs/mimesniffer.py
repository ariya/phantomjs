'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  Big thanks to the Chromium Authors, as much of this code was derived
  from their own hard work. :)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''


class Sniff(object):
    def __init__(self, data):
        # Use only 1024 bytes at maximum to compare against
        data = data[:1024]

        # Returns: magic_type, magic, is_string
        # :NOTE: Magic strings are case insensitive
        magic_number = lambda type_, magic: (type_, magic, False)
        magic_string = lambda type_, magic: (type_, magic, True)

        # HTML Tags
        self._magic_tags = (
            # XML processing directive. Although this is not an HTML mime type, we sniff
            # for this in the HTML phase because text/xml is just as powerful as HTML.
            magic_string('text/xml', '<?xml'), # Mozilla
            # DOCTYPEs
            magic_string('text/html', '<!DOCTYPE html'), # HTML5 spec
            # Sniffable tags, ordered by how often they occur in sniffable documents.
            magic_string('text/html', '<script'), # HTML5 spec, Mozilla
            magic_string('text/html', '<html'), # HTML5 spec, Mozilla
            magic_string('text/html', '<!--'),
            magic_string('text/html', '<head'), # HTML5 spec, Mozilla
            magic_string('text/html', '<iframe'), # Mozilla
            magic_string('text/html', '<h1'), # Mozilla
            magic_string('text/html', '<div'), # Mozilla
            magic_string('text/html', '<font'), # Mozilla
            magic_string('text/html', '<table'), # Mozilla
            magic_string('text/html', '<a'), # Mozilla
            magic_string('text/html', '<style'), # Mozilla
            magic_string('text/html', '<title'), # Mozilla
            magic_string('text/html', '<b'), # Mozilla
            magic_string('text/html', '<body'), # Mozilla
            magic_string('text/html', '<br'),
            magic_string('text/html', '<p') # Mozilla
        )

        # XML
        self._magic_Xml = (
            # We want to be very conservative in interpreting text/xml content as
            # XHTML -- we just want to sniff enough to make things pass.
            # So we match explicitly on this, and don't match other ways of writing
            # it in semantically-equivalent ways.
            magic_string('application/xhtml+xml', '<html xmlns="http://www.w3.org/1999/xhtml"'),
            magic_string('application/atom+xml', '<feed'),
            magic_string('application/rss+xml', '<rss') # UTF-8
        )

        # Images
        self._magic_images = (
            # Source: HTML 5 specification
            magic_number('image/gif', 'GIF87a'),
            magic_number('image/gif', 'GIF89a'),
            magic_number('image/png', '\x89PNG\x0D\x0A\x1A\x0A'),
            magic_number('image/jpeg', '\xFF\xD8\xFF'),
            magic_number('image/bmp', 'BM'),
            # Source: Chrome
            magic_number('image/tiff', 'I I'),
            magic_number('image/tiff', 'II*'),
            magic_number('image/tiff', 'MM\x00*'),
            magic_number('image/webp', 'RIFF....WEBPVP8 ')
        )

        # General magic numbers
        self._magic_numbers = (
            # Source: HTML 5 specification
            magic_number('application/pdf', '%PDF-'),
            magic_number('application/postscript', '%!PS-Adobe-'),
            # Source: Mozilla
            magic_number('text/plain', '#!'), # Script
            magic_number('text/plain', '%!'), # Script, similar to PS
            magic_number('text/plain', 'From'),
            magic_number('text/plain', '>From'),
            # Source: Chrome
            magic_number('application/x-gzip', '\x1F\x8B\x08'),
            magic_number('application/zip', 'PK\x03\x04'),
            magic_number('application/x-rar-compressed', 'Rar!\x1A\x07\x00'),
            magic_number('application/x-msmetafile', '\xD7\xCD\xC6\x9A'),
            magic_number('application/octet-stream', 'MZ') # EXE
        )

        # Audio
        self._magic_audio = (
            # Source: Chrome
            # :TODO: we don't handle partial byte matches yet
            # magic_number('audio/mpeg', '\xFF\xE'),
            # magic_number('audio/mpeg', '\xFF\xF'),
            magic_number('audio/x-pn-realaudio', '\x2E\x52\x4D\x46'),
            magic_number('audio/mpeg', 'ID3')
        )

        # Video
        self._magic_video = (
            # Source: Chrome
            # :TODO: we don't handle partial byte matches yet
            # magic_number('video/mpeg', '\x00\x00\x01\xB'),
            magic_number('video/x-ms-asf', '\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C'),
            magic_number('video/webm', '\x1A\x45\xDF\xA3')
        )

        # Byte order marks
        self._magic_Bom = (
            magic_number('text/plain', '\xFE\xFF'), # UTF-16BE
            magic_number('text/plain', '\xFF\xFE'), # UTF-16LE
            magic_number('text/plain', '\xEF\xBB\xBF') # UTF-8
        )

        # Whether a given byte looks like it might be part of binary content.
        # Source: HTML5 specification
        magic_byte_looks_binary = (
            1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 1, # 0x00 - 0x0F
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, # 0x10 - 0x1F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x20 - 0x2F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x30 - 0x3F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x40 - 0x4F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x50 - 0x5F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x60 - 0x6F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x70 - 0x7F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x80 - 0x8F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0x90 - 0x9F
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0xA0 - 0xAF
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0xB0 - 0xBF
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0xC0 - 0xCF
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0xD0 - 0xDF
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, # 0xE0 - 0xEF
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  # 0xF0 - 0xFF
        )

        # Start the auto-sniffing functionality

        # HTML sniffer; for this we will skip leading whitespace
        if self._check_for_magic_numbers(data.lstrip(), self._magic_tags):
            return

        # XML sniffer
        if self._check_for_magic_numbers(data, self._magic_Xml):
            return

        # Image sniffer
        if self._check_for_magic_numbers(data, self._magic_images):
            return

        # Magic Number sniffer
        if self._check_for_magic_numbers(data, self._magic_numbers):
            return

        # Audio sniffer
        if self._check_for_magic_numbers(data, self._magic_audio):
            return

        # Video sniffer
        if self._check_for_magic_numbers(data, self._magic_video):
            return

        # BOM sniffer; if we have a BOM, buffer is probably not binary content
        if self._check_for_magic_numbers(data, self._magic_Bom):
            return

        # Binary sniffer; checks if any bytes look binary
        for i in range(len(data)):
            if magic_byte_looks_binary[ord(data[i])]:
                self.mime_type = 'application/octet-stream'
                return

        # fall back to text/plain
        self.mime_type = 'text/plain'

    # Magic helper methods

    def _check_for_magic_numbers(self, data, magic):
        for i in range(len(magic)):
            if self._match_magic_number(data, magic[i]):
                return True
        return False

    def _match_magic_number(self, data, magic_entry):
        # we have a match unless explicitly set to False
        match = True

        # if it's a string
        if magic_entry[2]:
            match = data.startswith(magic_entry[1]) or magic_entry[1] in data
        else:
            for i in range(len(magic_entry[1])):
                if magic_entry[1][i] != '.' and magic_entry[1][i] != data[i]:
                    match = False
                    break

        if match:
            self.mime_type = magic_entry[0]
            return True

        return False

    # Public methods

    @property
    def isAudio(self):
        for magic_entry in self._magic_audio:
            if self.mime_type == magic_entry[0]:
                return True
        return False

    @property
    def isBinary(self):
        return self.mime_type == 'application/octet-stream'

    @property
    def isHtml(self):
        for magic_entry in self._magic_tags:
            if self.mime_type == magic_entry[0]:
                return True
        return False

    @property
    def isImage(self):
        for magic_entry in self._magic_images:
            if self.mime_type == magic_entry[0]:
                return True
        return False

    @property
    def isText(self):
        return self.mime_type == 'text/plain'

    @property
    def isVideo(self):
        for magic_entry in self._magic_video:
            if self.mime_type == magic_entry[0]:
                return True
        return False

    @property
    def isXml(self):
        for magic_entry in self._magic_Xml:
            if self.mime_type == magic_entry[0]:
                return True
        return False
