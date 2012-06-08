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

// Disable exception handler warnings.
#pragma warning( disable : 4530 )

#include <errno.h>

#include "client/windows/sender/crash_report_sender.h"
#include "common/windows/http_upload.h"

#if _MSC_VER < 1400  // MSVC 2005/8
// Older MSVC doesn't have fscanf_s, but they are compatible as long as
// we don't use the string conversions (%s/%c/%S/%C).
#define fscanf_s fscanf
#endif

namespace google_breakpad {

static const char kCheckpointSignature[] = "GBP1\n";

CrashReportSender::CrashReportSender(const wstring &checkpoint_file)
    : checkpoint_file_(checkpoint_file),
      max_reports_per_day_(-1),
      last_sent_date_(-1),
      reports_sent_(0) {
  FILE *fd;
  if (OpenCheckpointFile(L"r", &fd) == 0) {
    ReadCheckpoint(fd);
    fclose(fd);
  }
}

ReportResult CrashReportSender::SendCrashReport(
    const wstring &url, const map<wstring, wstring> &parameters,
    const wstring &dump_file_name, wstring *report_code) {
  int today = GetCurrentDate();
  if (today == last_sent_date_ &&
      max_reports_per_day_ != -1 &&
      reports_sent_ >= max_reports_per_day_) {
    return RESULT_THROTTLED;
  }

  int http_response = 0;
  bool result = HTTPUpload::SendRequest(
    url, parameters, dump_file_name, L"upload_file_minidump", NULL, report_code,
    &http_response);

  if (result) {
    ReportSent(today);
    return RESULT_SUCCEEDED;
  } else if (http_response >= 400 && http_response < 500) {
    return RESULT_REJECTED;
  } else {
    return RESULT_FAILED;
  }
}

void CrashReportSender::ReadCheckpoint(FILE *fd) {
  char buf[128];
  if (!fgets(buf, sizeof(buf), fd) ||
      strcmp(buf, kCheckpointSignature) != 0) {
    return;
  }

  if (fscanf_s(fd, "%d\n", &last_sent_date_) != 1) {
    last_sent_date_ = -1;
    return;
  }
  if (fscanf_s(fd, "%d\n", &reports_sent_) != 1) {
    reports_sent_ = 0;
    return;
  }
}

void CrashReportSender::ReportSent(int today) {
  // Update the report stats
  if (today != last_sent_date_) {
    last_sent_date_ = today;
    reports_sent_ = 0;
  }
  ++reports_sent_;

  // Update the checkpoint file
  FILE *fd;
  if (OpenCheckpointFile(L"w", &fd) == 0) {
    fputs(kCheckpointSignature, fd);
    fprintf(fd, "%d\n", last_sent_date_);
    fprintf(fd, "%d\n", reports_sent_);
    fclose(fd);
  }
}

int CrashReportSender::GetCurrentDate() const {
  SYSTEMTIME system_time;
  GetSystemTime(&system_time);
  return (system_time.wYear * 10000) + (system_time.wMonth * 100) +
      system_time.wDay;
}

int CrashReportSender::OpenCheckpointFile(const wchar_t *mode, FILE **fd) {
  if (checkpoint_file_.empty()) {
    return ENOENT;
  }
#if _MSC_VER >= 1400  // MSVC 2005/8
  return _wfopen_s(fd, checkpoint_file_.c_str(), mode);
#else
  *fd = _wfopen(checkpoint_file_.c_str(), mode);
  if (*fd == NULL) {
    return errno;
  }
  return 0;
#endif
}

}  // namespace google_breakpad
