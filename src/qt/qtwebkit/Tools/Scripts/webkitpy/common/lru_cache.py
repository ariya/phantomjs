# Copyright (C) 2011 Google Inc. All rights reserved.
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


class Node():
    def __init__(self, key, value):
        self.key = key
        self.value = value
        self.prev = None
        self.next = None


class LRUCache():
    """An implementation of Least Recently Used (LRU) Cache."""

    def __init__(self, capacity):
        """Initializes a lru cache with the given capacity.

        Args:
            capacity: The capacity of the cache.
        """
        assert capacity > 0, "capacity (%s) must be greater than zero." % capacity
        self._first = None
        self._last = None
        self._dict = {}
        self._capacity = capacity

    def __setitem__(self, key, value):
        if key in self._dict:
            self.__delitem__(key)
        if not self._first:
            self._one_node(key, value)
            return
        if len(self._dict) >= self._capacity:
            del self._dict[self._last.key]
            if self._capacity == 1:
                self._one_node(key, value)
                return
            self._last = self._last.next
            self._last.prev = None
        node = Node(key, value)
        node.prev = self._first
        self._first.next = node
        self._first = node
        self._dict[key] = node

    def _one_node(self, key, value):
        node = Node(key, value)
        self._dict[key] = node
        self._first = node
        self._last = node

    def __getitem__(self, key):
        if not self._first:
            raise KeyError(str(key))
        if self._first.key == key:
            return self._first.value

        if self._last.key == key:
            next_last = self._last.next
            next_last.prev = None
            next_first = self._last
            next_first.prev = self._first
            next_first.next = None
            self._first.next = next_first
            self._first = next_first
            self._last = next_last
            return self._first.value

        node = self._dict[key]
        node.next.prev = node.prev
        node.prev.next = node.next
        node.prev = self._first
        node.next = None
        self._first.next = node
        self._first = node
        return self._first.value

    def __delitem__(self, key):
        node = self._dict[key]
        del self._dict[key]
        if self._first is self._last:
            self._last = None
            self._first = None
            return
        if self._first is node:
            self._first = node.prev
            self._first.next = None
            return
        if self._last is node:
            self._last = node.next
            self._last.prev = None
            return
        node.next.prev = node.prev
        node.prev.next = node.next

    def __len__(self):
        return len(self._dict)

    def __contains__(self, key):
        return key in self._dict

    def __iter__(self):
        return iter(self._dict)

    def items(self):
        return [(key, node.value) for key, node in self._dict.items()]

    def values(self):
        return [node.value for node in self._dict.values()]

    def keys(self):
        return self._dict.keys()
