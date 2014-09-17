// Copyright (c) 2008, Google Inc.
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

#import "SimpleStringDictionaryTest.h"
#import "SimpleStringDictionary.h"

using google_breakpad::KeyValueEntry;
using google_breakpad::SimpleStringDictionary;
using google_breakpad::SimpleStringDictionaryIterator;

@implementation SimpleStringDictionaryTest

//==============================================================================
- (void)testKeyValueEntry {
  KeyValueEntry entry;

  // Verify that initial state is correct
  STAssertFalse(entry.IsActive(), @"Initial key value entry is active!");
  STAssertEquals(strlen(entry.GetKey()), (size_t)0, @"Empty key value did not "
                 @"have length 0");
  STAssertEquals(strlen(entry.GetValue()), (size_t)0, @"Empty key value did not "
                 @"have length 0");

  // Try setting a key/value and then verify
  entry.SetKeyValue("key1", "value1");
  STAssertEqualCStrings(entry.GetKey(), "key1", @"key was not equal to key1");
  STAssertEqualCStrings(entry.GetValue(), "value1", @"value was not equal");

  // Try setting a new value
  entry.SetValue("value3");

  // Make sure the new value took
  STAssertEqualCStrings(entry.GetValue(), "value3", @"value was not equal");

  // Make sure the key didn't change
  STAssertEqualCStrings(entry.GetKey(), "key1", @"key changed after setting "
                        @"value!");

  // Try setting a new key/value and then verify
  entry.SetKeyValue("key2", "value2");
  STAssertEqualCStrings(entry.GetKey(), "key2", @"New key was not equal to "
                        @"key2");
  STAssertEqualCStrings(entry.GetValue(), "value2", @"New value was not equal "
                        @"to value2");

  // Clear the entry and verify the key and value are empty strings
  entry.Clear();
  STAssertFalse(entry.IsActive(), @"Key value clear did not clear object");
  STAssertEquals(strlen(entry.GetKey()), (size_t)0, @"Length of cleared key "
		 @"was not 0");
  STAssertEquals(strlen(entry.GetValue()), (size_t)0, @"Length of cleared "
		 @"value was not 0!");
}

- (void)testEmptyKeyValueCombos {
  KeyValueEntry entry;
  entry.SetKeyValue(NULL, NULL);
  STAssertEqualCStrings(entry.GetKey(), "", @"Setting NULL key did not return "
			@"empty key!");
  STAssertEqualCStrings(entry.GetValue(), "", @"Setting NULL value did not "
			@"set empty string value!");
}


//==============================================================================
- (void)testSimpleStringDictionary {
  // Make a new dictionary
  SimpleStringDictionary *dict = new SimpleStringDictionary();
  STAssertTrue(dict != NULL, nil);

  // try passing in NULL for key
  //dict->SetKeyValue(NULL, "bad");   // causes assert() to fire

  // Set three distinct values on three keys
  dict->SetKeyValue("key1", "value1");
  dict->SetKeyValue("key2", "value2");
  dict->SetKeyValue("key3", "value3");

  STAssertTrue(!strcmp(dict->GetValueForKey("key1"), "value1"), nil);
  STAssertTrue(!strcmp(dict->GetValueForKey("key2"), "value2"), nil);
  STAssertTrue(!strcmp(dict->GetValueForKey("key3"), "value3"), nil);
  STAssertEquals(dict->GetCount(), 3, @"GetCount did not return 3");
  // try an unknown key
  STAssertTrue(dict->GetValueForKey("key4") == NULL, nil);

  // try a NULL key
  //STAssertTrue(dict->GetValueForKey(NULL) == NULL, nil);  // asserts

  // Remove a key
  dict->RemoveKey("key3");

  // Now make sure it's not there anymore
  STAssertTrue(dict->GetValueForKey("key3") == NULL, nil);

  // Remove a NULL key
  //dict->RemoveKey(NULL);  // will cause assert() to fire

  // Remove by setting value to NULL
  dict->SetKeyValue("key2", NULL);

  // Now make sure it's not there anymore
  STAssertTrue(dict->GetValueForKey("key2") == NULL, nil);
}

//==============================================================================
// The idea behind this test is to add a bunch of values to the dictionary,
// remove some in the middle, then add a few more in.  We then create a
// SimpleStringDictionaryIterator and iterate through the dictionary, taking
// note of the key/value pairs we see.  We then verify that it iterates
// through exactly the number of key/value pairs we expect, and that they
// match one-for-one with what we would expect.  In all cases we're setting
// key value pairs of the form:
//
// key<n>/value<n>   (like key0/value0, key17,value17, etc.)
//
- (void)testSimpleStringDictionaryIterator {
  SimpleStringDictionary *dict = new SimpleStringDictionary();
  STAssertTrue(dict != NULL, nil);

  char key[KeyValueEntry::MAX_STRING_STORAGE_SIZE];
  char value[KeyValueEntry::MAX_STRING_STORAGE_SIZE];

  const int kDictionaryCapacity = SimpleStringDictionary::MAX_NUM_ENTRIES;
  const int kPartitionIndex = kDictionaryCapacity - 5;

  // We assume at least this size in the tests below
  STAssertTrue(kDictionaryCapacity >= 64, nil);

  // We'll keep track of the number of key/value pairs we think should
  // be in the dictionary
  int expectedDictionarySize = 0;

  // Set a bunch of key/value pairs like key0/value0, key1/value1, ...
  for (int i = 0; i < kPartitionIndex; ++i) {
    sprintf(key, "key%d", i);
    sprintf(value, "value%d", i);
    dict->SetKeyValue(key, value);
  }
  expectedDictionarySize = kPartitionIndex;

  // set a couple of the keys twice (with the same value) - should be nop
  dict->SetKeyValue("key2", "value2");
  dict->SetKeyValue("key4", "value4");
  dict->SetKeyValue("key15", "value15");

  // Remove some random elements in the middle
  dict->RemoveKey("key7");
  dict->RemoveKey("key18");
  dict->RemoveKey("key23");
  dict->RemoveKey("key31");
  expectedDictionarySize -= 4;  // we just removed four key/value pairs

  // Set some more key/value pairs like key59/value59, key60/value60, ...
  for (int i = kPartitionIndex; i < kDictionaryCapacity; ++i) {
    sprintf(key, "key%d", i);
    sprintf(value, "value%d", i);
    dict->SetKeyValue(key, value);
  }
  expectedDictionarySize += kDictionaryCapacity - kPartitionIndex;

  // Now create an iterator on the dictionary
  SimpleStringDictionaryIterator iter(*dict);

  // We then verify that it iterates through exactly the number of
  // key/value pairs we expect, and that they match one-for-one with what we
  // would expect.  The ordering of the iteration does not matter...

  // used to keep track of number of occurrences found for key/value pairs
  int count[kDictionaryCapacity];
  memset(count, 0, sizeof(count));

  int totalCount = 0;

  const KeyValueEntry *entry;

  while ((entry = iter.Next())) {
    totalCount++;

    // Extract keyNumber from a string of the form key<keyNumber>
    int keyNumber;
    sscanf(entry->GetKey(), "key%d", &keyNumber);

    // Extract valueNumber from a string of the form value<valueNumber>
    int valueNumber;
    sscanf(entry->GetValue(), "value%d", &valueNumber);

    // The value number should equal the key number since that's how we set them
    STAssertTrue(keyNumber == valueNumber, nil);

    // Key and value numbers should be in proper range:
    // 0 <= keyNumber < kDictionaryCapacity
    bool isKeyInGoodRange =
      (keyNumber >= 0 && keyNumber < kDictionaryCapacity);
    bool isValueInGoodRange =
      (valueNumber >= 0 && valueNumber < kDictionaryCapacity);
    STAssertTrue(isKeyInGoodRange, nil);
    STAssertTrue(isValueInGoodRange, nil);

    if (isKeyInGoodRange && isValueInGoodRange) {
      ++count[keyNumber];
    }
  }

  // Make sure each of the key/value pairs showed up exactly one time, except
  // for the ones which we removed.
  for (int i = 0; i < kDictionaryCapacity; ++i) {
    // Skip over key7, key18, key23, and key31, since we removed them
    if (!(i == 7 || i == 18 || i == 23 || i == 31)) {
      STAssertTrue(count[i] == 1, nil);
    }
  }

  // Make sure the number of iterations matches the expected dictionary size.
  STAssertTrue(totalCount == expectedDictionarySize, nil);
}

@end
