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

from datetime import datetime
import logging

from google.appengine.ext import db

MAX_DATA_ENTRY_PER_FILE = 10
MAX_ENTRY_LEN = 1000 * 1000


class DataEntry(db.Model):
    """Datastore entry that stores one segmant of file data
       (<1000*1000 bytes).
    """

    data = db.BlobProperty()

    @classmethod
    def get(cls, key):
        return db.get(key)

    def get_data(self, key):
        return db.get(key)


class DataStoreFile(db.Model):
    """This class stores file in datastore.
       If a file is oversize (>1000*1000 bytes), the file is split into
       multiple segments and stored in multiple datastore entries.
    """

    name = db.StringProperty()
    data_keys = db.ListProperty(db.Key)
    # keys to the data store entries that can be reused for new data.
    # If it is emtpy, create new DataEntry.
    new_data_keys = db.ListProperty(db.Key)
    date = db.DateTimeProperty(auto_now_add=True)

    data = None

    def delete_data(self, keys=None):
        if not keys:
            keys = self.data_keys

        for key in keys:
            data_entry = DataEntry.get(key)
            if data_entry:
                data_entry.delete()

    def save_data(self, data):
        if not data:
            logging.warning("No data to save.")
            return False

        if len(data) > (MAX_DATA_ENTRY_PER_FILE * MAX_ENTRY_LEN):
            logging.error("File too big, can't save to datastore: %dK",
                len(data) / 1024)
            return False

        start = 0
        # Use the new_data_keys to store new data. If all new data are saved
        # successfully, swap new_data_keys and data_keys so we can reuse the
        # data_keys entries in next run. If unable to save new data for any
        # reason, only the data pointed by new_data_keys may be corrupted,
        # the existing data_keys data remains untouched. The corrupted data
        # in new_data_keys will be overwritten in next update.
        keys = self.new_data_keys
        self.new_data_keys = []

        while start < len(data):
            if keys:
                key = keys[0]
                data_entry = DataEntry.get(key)
                if not data_entry:
                    logging.warning("Found key, but no data entry: %s", key)
                    data_entry = DataEntry()
            else:
                data_entry = DataEntry()

            data_entry.data = db.Blob(data[start: start + MAX_ENTRY_LEN])
            try:
                data_entry.put()
            except Exception, err:
                logging.error("Failed to save data store entry: %s", err)
                if keys:
                    self.delete_data(keys)
                return False

            logging.info("Data saved: %s.", data_entry.key())
            self.new_data_keys.append(data_entry.key())
            if keys:
                keys.pop(0)

            start = start + MAX_ENTRY_LEN

        if keys:
            self.delete_data(keys)

        temp_keys = self.data_keys
        self.data_keys = self.new_data_keys
        self.new_data_keys = temp_keys
        self.data = data

        return True

    def load_data(self):
        if not self.data_keys:
            logging.warning("No data to load.")
            return None

        data = []
        for key in self.data_keys:
            logging.info("Loading data for key: %s.", key)
            data_entry = DataEntry.get(key)
            if not data_entry:
                logging.error("No data found for key: %s.", key)
                return None

            data.append(data_entry.data)

        self.data = "".join(data)

        return self.data
