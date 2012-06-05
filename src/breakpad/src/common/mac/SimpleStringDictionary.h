// Copyright (c) 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  SimpleStringDictionary.h
//

#ifndef SimpleStringDictionary_H__
#define SimpleStringDictionary_H__

#import <string>
#import <vector>

namespace google_breakpad {

//==============================================================================
// SimpleStringDictionary (and associated class KeyValueEntry) implement a very
// basic dictionary container class.  It has the property of not making any
// memory allocations when getting and setting values.  But it is not very
// efficient, with calls to get and set values operating in linear time.
// It has the additional limitation of having a fairly small fixed capacity of
// SimpleStringDictionary::MAX_NUM_ENTRIES entries.  An assert() will fire if
// the client attempts to set more than this number of key/value pairs.
// Ordinarilly a C++ programmer would use something like the std::map template
// class, or on the Macintosh would often choose CFDictionary or NSDictionary.
// But these dictionary classes may call malloc() during get and set operations.
// Google Breakpad requires that no memory allocations be made in code running
// in its exception handling thread, so it uses SimpleStringDictionary as the
// underlying implementation for the GoogleBreakpad.framework APIs:
// GoogleBreakpadSetKeyValue(),  GoogleBreakpadKeyValue(), and
// GoogleBreakpadRemoveKeyValue()
//

//==============================================================================
// KeyValueEntry
//
// A helper class used by SimpleStringDictionary representing a single
// storage cell for a key/value pair.  Each key and value string are
// limited to MAX_STRING_STORAGE_SIZE-1 bytes (not glyphs).  This class
// performs no memory allocations.  It has methods for setting  and getting
// key and value strings.
//
class KeyValueEntry {
 public:
  KeyValueEntry() {
    Clear();
  }
  
  KeyValueEntry(const char *key, const char *value) {
    SetKeyValue(key, value);
  }

  void        SetKeyValue(const char *key, const char *value) {
    if (!key) {
      key = "";
    }
    if (!value) {
      value = "";
    }
    
    strlcpy(key_, key, sizeof(key_));
    strlcpy(value_, value, sizeof(value_));
  }  

  void        SetValue(const char *value) {
    if (!value) {
      value = "";
    }
    strlcpy(value_, value, sizeof(value_));
  };
  
  // Removes the key/value
  void        Clear() {
    memset(key_, 0, sizeof(key_));
    memset(value_, 0, sizeof(value_));
  }

  bool        IsActive() const { return key_[0] != '\0'; }
  const char *GetKey() const { return key_; }
  const char *GetValue() const { return value_; }

  // Don't change this without considering the fixed size
  // of MachMessage (in MachIPC.h)
  // (see also struct KeyValueMessageData in Inspector.h)
  enum {MAX_STRING_STORAGE_SIZE = 256};
  
 private:
  char key_[MAX_STRING_STORAGE_SIZE];
  char value_[MAX_STRING_STORAGE_SIZE];
};

//==============================================================================
// This class is not an efficient dictionary, but for the purposes of breakpad
// will be just fine.  We're just dealing with ten or so distinct
// key/value pairs.  The idea is to avoid any malloc() or free() calls
// in certain important methods to be called when a process is in a
// crashed state.  Each key and value string are limited to
// KeyValueEntry::MAX_STRING_STORAGE_SIZE-1 bytes (not glyphs).  Strings passed
// in exceeding this length will be truncated.
//
class SimpleStringDictionary {
 public:
  SimpleStringDictionary() {};  // entries will all be cleared
  
  // Returns the number of active key/value pairs.  The upper limit for this
  // is MAX_NUM_ENTRIES.
  int GetCount() const;

  // Given |key|, returns its corresponding |value|.
  // If |key| is NULL, an assert will fire or NULL will be returned.  If |key|
  // is not found or is an empty string, NULL is returned.
  const char *GetValueForKey(const char *key) const;
    
  // Stores a string |value| represented by |key|.  If |key| is NULL or an empty
  // string, this will assert (or do nothing).  If |value| is NULL then
  // the |key| will be removed.  An empty string is OK for |value|.
  void SetKeyValue(const char *key, const char *value);
  
  // Given |key|, removes any associated value.  It will assert (or do nothing)
  // if NULL is passed in.  It will do nothing if |key| is not found.
  void RemoveKey(const char *key);

  // This is the maximum number of key/value pairs which may be set in the
  // dictionary.  An assert may fire if more values than this are set.
  // Don't change this without also changing comment in GoogleBreakpad.h
  enum {MAX_NUM_ENTRIES = 64};

 private:
  friend class SimpleStringDictionaryIterator;

  const KeyValueEntry *GetEntry(int i) const;

  KeyValueEntry             entries_[MAX_NUM_ENTRIES];
};

//==============================================================================
class SimpleStringDictionaryIterator {
 public:
  SimpleStringDictionaryIterator(const SimpleStringDictionary &dict)
    : dict_(dict), i_(0) {
    }

  // Initializes iterator to the beginning (may later call Next() )
  void Start() {
    i_ = 0;
  }
  
  // like the nextObject method of NSEnumerator (in Cocoa)
  // returns NULL when there are no more entries
  //
  const KeyValueEntry* Next() {
    for (; i_ < SimpleStringDictionary::MAX_NUM_ENTRIES; ++i_) {
      const KeyValueEntry *entry = dict_.GetEntry(i_);
      if (entry->IsActive()) {
        i_++;   // move to next entry for next time
        return entry;
      }
    }

    return NULL;  // reached end of array
  }
  
 private:
  const SimpleStringDictionary&   dict_;
  int                             i_;
};

}  // namespace google_breakpad

#endif  // SimpleStringDictionary_H__
