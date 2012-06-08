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
//  SimpleStringDictionary.mm
//  Simple string dictionary that does not allocate memory
//

#include <assert.h>

#import "SimpleStringDictionary.h"

namespace google_breakpad {

//==============================================================================
const KeyValueEntry *SimpleStringDictionary::GetEntry(int i) const {
  return (i >= 0 && i < MAX_NUM_ENTRIES) ? &entries_[i] : NULL;
}

//==============================================================================
int SimpleStringDictionary::GetCount() const {
  int count = 0;
  for (int i = 0; i < MAX_NUM_ENTRIES; ++i) {
    if (entries_[i].IsActive() ) {
      ++count;
    }
  }
  
  return count;
}

//==============================================================================
const char *SimpleStringDictionary::GetValueForKey(const char *key) const {
  assert(key);
  if (!key)
    return NULL;

  for (int i = 0; i < MAX_NUM_ENTRIES; ++i) {
    const KeyValueEntry &entry = entries_[i];
    if (entry.IsActive() && !strcmp(entry.GetKey(), key)) {
      return entry.GetValue();
    }
  }

  return NULL;
}

//==============================================================================
void SimpleStringDictionary::SetKeyValue(const char *key,
                                         const char *value) {
  if (!value) {
    RemoveKey(key);
    return;
  }

  // key must not be NULL
  assert(key);
  if (!key)
    return;
  
  // key must not be empty string
  assert(key[0] != '\0');
  if (key[0] == '\0')
    return;
  
  int free_index = -1;
  
  // check if key already exists
  for (int i = 0; i < MAX_NUM_ENTRIES; ++i) {
    KeyValueEntry &entry = entries_[i];
    
    if (entry.IsActive()) {
      if (!strcmp(entry.GetKey(), key)) {
        entry.SetValue(value);
        return;
      }
    } else {
      // Make a note of an empty slot
      if (free_index == -1) {
        free_index = i;
      }
    }
  }
  
  // check if we've run out of space
  assert(free_index != -1);
  
  // Put new key into an empty slot (if found)
  if (free_index != -1) {
    entries_[free_index].SetKeyValue(key, value);
  }
}

//==============================================================================
void SimpleStringDictionary::RemoveKey(const char *key) {
  assert(key);
  if (!key)
    return;

  for (int i = 0; i < MAX_NUM_ENTRIES; ++i) {
    if (!strcmp(entries_[i].GetKey(), key)) {
      entries_[i].Clear();
      return;
    }
  }
}

}  // namespace google_breakpad
