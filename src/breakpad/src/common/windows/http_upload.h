// Copyright (c) 2006, Google Inc.
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

// HTTPUpload provides a "nice" API to send a multipart HTTP(S) POST
// request using wininet.  It currently supports requests that contain
// a set of string parameters (key/value pairs), and a file to upload.

#ifndef COMMON_WINDOWS_HTTP_UPLOAD_H_
#define COMMON_WINDOWS_HTTP_UPLOAD_H_

#pragma warning(push)
// Disable exception handler warnings.
#pragma warning(disable : 4530)

#include <Windows.h>
#include <WinInet.h>

#include <map>
#include <string>
#include <vector>

namespace google_breakpad {

using std::string;
using std::wstring;
using std::map;
using std::vector;

class HTTPUpload {
 public:
  // Sends the given set of parameters, along with the contents of
  // upload_file, as a multipart POST request to the given URL.
  // file_part_name contains the name of the file part of the request
  // (i.e. it corresponds to the name= attribute on an <input type="file">.
  // Parameter names must contain only printable ASCII characters,
  // and may not contain a quote (") character.
  // Only HTTP(S) URLs are currently supported.  Returns true on success.
  // If the request is successful and response_body is non-NULL,
  // the response body will be returned in response_body.
  // If response_code is non-NULL, it will be set to the HTTP response code
  // received (or 0 if the request failed before getting an HTTP response).
  static bool SendRequest(const wstring &url,
                          const map<wstring, wstring> &parameters,
                          const wstring &upload_file,
                          const wstring &file_part_name,
                          int *timeout,
                          wstring *response_body,
                          int *response_code);

 private:
  class AutoInternetHandle;

  // Retrieves the HTTP response.  If NULL is passed in for response,
  // this merely checks (via the return value) that we were successfully
  // able to retrieve exactly as many bytes of content in the response as
  // were specified in the Content-Length header.
  static bool HTTPUpload::ReadResponse(HINTERNET request, wstring* response);

  // Generates a new multipart boundary for a POST request
  static wstring GenerateMultipartBoundary();

  // Generates a HTTP request header for a multipart form submit.
  static wstring GenerateRequestHeader(const wstring &boundary);

  // Given a set of parameters, an upload filename, and a file part name,
  // generates a multipart request body string with these parameters
  // and minidump contents.  Returns true on success.
  static bool GenerateRequestBody(const map<wstring, wstring> &parameters,
                                  const wstring &upload_file,
                                  const wstring &file_part_name,
                                  const wstring &boundary,
                                  string *request_body);

  // Fills the supplied vector with the contents of filename.
  static bool GetFileContents(const wstring &filename, vector<char> *contents);

  // Converts a UTF8 string to UTF16.
  static wstring UTF8ToWide(const string &utf8);

  // Converts a UTF16 string to UTF8.
  static string WideToUTF8(const wstring &wide);

  // Checks that the given list of parameters has only printable
  // ASCII characters in the parameter name, and does not contain
  // any quote (") characters.  Returns true if so.
  static bool CheckParameters(const map<wstring, wstring> &parameters);

  // No instances of this class should be created.
  // Disallow all constructors, destructors, and operator=.
  HTTPUpload();
  explicit HTTPUpload(const HTTPUpload &);
  void operator=(const HTTPUpload &);
  ~HTTPUpload();
};

}  // namespace google_breakpad

#pragma warning(pop)

#endif  // COMMON_WINDOWS_HTTP_UPLOAD_H_
