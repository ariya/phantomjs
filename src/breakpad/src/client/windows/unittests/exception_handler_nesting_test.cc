// Copyright 2012, Google Inc.
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

#include <windows.h>

#include <string>

#include "breakpad_googletest_includes.h"
#include "client/windows/handler/exception_handler.h"
#include "client/windows/unittests/exception_handler_test.h"

namespace {

const char kFoo[] = "foo";
const char kBar[] = "bar";

const char kStartOfLine[] = "^";
const char kEndOfLine[] = "$";

const char kFilterReturnsTrue[] = "filter_returns_true";
const char kFilterReturnsFalse[] = "filter_returns_false";

const char kCallbackReturnsTrue[] = "callback_returns_true";
const char kCallbackReturnsFalse[] = "callback_returns_false";

bool DoesPathExist(const wchar_t *path_name) {
  DWORD flags = GetFileAttributes(path_name);
  if (flags == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return true;
}

// A callback function to run before Breakpad performs any substantial
// processing of an exception.  A FilterCallback is called before writing
// a minidump.  context is the parameter supplied by the user as
// callback_context when the handler was created.  exinfo points to the
// exception record, if any; assertion points to assertion information,
// if any.
//
// If a FilterCallback returns true, Breakpad will continue processing,
// attempting to write a minidump.  If a FilterCallback returns false,
// Breakpad will immediately report the exception as unhandled without
// writing a minidump, allowing another handler the opportunity to handle it.
template <bool filter_return_value>
bool CrashHandlerFilter(void* context,
                        EXCEPTION_POINTERS* exinfo,
                        MDRawAssertionInfo* assertion) {
  if (filter_return_value) {
    fprintf(stderr, kFilterReturnsTrue);
  } else {
    fprintf(stderr, kFilterReturnsFalse);
  }
  fflush(stderr);

  return filter_return_value;
}

// A callback function to run after the minidump has been written.
// minidump_id is a unique id for the dump, so the minidump
// file is <dump_path>\<minidump_id>.dmp.  context is the parameter supplied
// by the user as callback_context when the handler was created.  exinfo
// points to the exception record, or NULL if no exception occurred.
// succeeded indicates whether a minidump file was successfully written.
// assertion points to information about an assertion if the handler was
// invoked by an assertion.
//
// If an exception occurred and the callback returns true, Breakpad will treat
// the exception as fully-handled, suppressing any other handlers from being
// notified of the exception.  If the callback returns false, Breakpad will
// treat the exception as unhandled, and allow another handler to handle it.
// If there are no other handlers, Breakpad will report the exception to the
// system as unhandled, allowing a debugger or native crash dialog the
// opportunity to handle the exception.  Most callback implementations
// should normally return the value of |succeeded|, or when they wish to
// not report an exception of handled, false.  Callbacks will rarely want to
// return true directly (unless |succeeded| is true).
//
// For out-of-process dump generation, dump path and minidump ID will always
// be NULL. In case of out-of-process dump generation, the dump path and
// minidump id are controlled by the server process and are not communicated
// back to the crashing process.
template <bool callback_return_value>
bool MinidumpWrittenCallback(const wchar_t* dump_path,
                             const wchar_t* minidump_id,
                             void* context,
                             EXCEPTION_POINTERS* exinfo,
                             MDRawAssertionInfo* assertion,
                             bool succeeded) {
  bool rv = false;
  if (callback_return_value &&
      succeeded &&
      DoesPathExist(dump_path)) {
    rv = true;
    fprintf(stderr, kCallbackReturnsTrue);
  } else {
    fprintf(stderr, kCallbackReturnsFalse);
  }
  fflush(stderr);

  return rv;
}


void DoCrash(const char *message) {
  if (message) {
    fprintf(stderr, "%s", message);
    fflush(stderr);
  }
  int *i = NULL;
  (*i)++;

  ASSERT_TRUE(false);
}

void InstallExceptionHandlerAndCrash(bool install_filter,
                                     bool filter_return_value,
                                     bool install_callback,
                                     bool callback_return_value) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      install_filter ?
        (filter_return_value ?
          &CrashHandlerFilter<true> :
          &CrashHandlerFilter<false>) :
        NULL,
      install_callback ?
        (callback_return_value ?
          &MinidumpWrittenCallback<true> :
          &MinidumpWrittenCallback<false>) :
        NULL,
      NULL,  // callback_context
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  // Disable GTest SEH handler
  testing::DisableExceptionHandlerInScope disable_exception_handler;

  DoCrash(NULL);
}

TEST(AssertDeathSanity, Simple) {
  ASSERT_DEATH(DoCrash(NULL), "");
}

TEST(AssertDeathSanity, Regex) {
  ASSERT_DEATH(DoCrash(kFoo),
    std::string(kStartOfLine) +
      std::string(kFoo) +
      std::string(kEndOfLine));

  ASSERT_DEATH(DoCrash(kBar),
    std::string(kStartOfLine) +
      std::string(kBar) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterTrue_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    // install_filter
                                    true,    // filter_return_value
                                    false,   // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterTrue_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    // install_filter
                                    true,    // filter_return_value
                                    true,    // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +
      std::string(kCallbackReturnsFalse) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, FilterFalse_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    // install_filter
                                    false,   // filter_return_value
                                    false,   // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +
      std::string(kEndOfLine));
}

// Callback shouldn't be executed when filter returns false
TEST(ExceptionHandlerCallbacks, FilterFalse_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    // install_filter
                                    false,   // filter_return_value
                                    true,    // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, No_Filter_No_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(false,   // install_filter
                                    true,    // filter_return_value
                                    false,   // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerCallbacks, No_Filter_Callback) {
  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(false,   // install_filter
                                    true,    // filter_return_value
                                    true,    // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kCallbackReturnsFalse) +
      std::string(kEndOfLine));
}


TEST(ExceptionHandlerNesting, Skip_From_Inner_Filter) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<false>,
      NULL,  // callback_context
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,   // install_filter
                                    false,  // filter_return_value
                                    true,   // install_callback
                                    true),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsFalse) +    // inner filter
      std::string(kFilterReturnsTrue) +     // outer filter
      std::string(kCallbackReturnsFalse) +  // outer callback
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerNesting, Skip_From_Inner_Callback) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<false>,
      NULL,  // callback_context
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,    // install_filter
                                    true,    // filter_return_value
                                    true,    // install_callback
                                    false),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +      // inner filter
      std::string(kCallbackReturnsFalse) +   // inner callback
      std::string(kFilterReturnsTrue) +      // outer filter
      std::string(kCallbackReturnsFalse) +   // outer callback
      std::string(kEndOfLine));
}

TEST(ExceptionHandlerNesting, Handled_By_Inner_Handler) {
  wchar_t temp_path[MAX_PATH] = { '\0' };
  GetTempPath(MAX_PATH, temp_path);

  ASSERT_TRUE(DoesPathExist(temp_path));
  google_breakpad::ExceptionHandler exc(
      temp_path,
      &CrashHandlerFilter<true>,
      &MinidumpWrittenCallback<true>,
      NULL,  // callback_context
      google_breakpad::ExceptionHandler::HANDLER_EXCEPTION);

  ASSERT_DEATH(
    InstallExceptionHandlerAndCrash(true,   // install_filter
                                    true,   // filter_return_value
                                    true,   // install_callback
                                    true),  // callback_return_value
    std::string(kStartOfLine) +
      std::string(kFilterReturnsTrue) +    // inner filter
      std::string(kCallbackReturnsTrue) +  // inner callback
      std::string(kEndOfLine));
}

}  // namespace
