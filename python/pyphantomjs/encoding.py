'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

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

import codecs


class Encode(object):
    def __init__(self, encoding, default):
        # check that encoding is valid
        try:
            codecs.lookup(encoding)
            self.encoding = encoding
        except LookupError:
            # fall back to default encoding
            self.encoding = default

    @property
    def name(self):
        return codecs.lookup(self.encoding).name
