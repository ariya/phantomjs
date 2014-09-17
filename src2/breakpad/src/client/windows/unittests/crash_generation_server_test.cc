// Copyright 2010, Google Inc.
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


#include "testing/gtest/include/gtest/gtest.h"
#include "testing/include/gmock/gmock.h"

#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/common/ipc_protocol.h"

using testing::_;

namespace {

const wchar_t kPipeName[] =
  L"\\\\.\\pipe\\CrashGenerationServerTest\\TestCaseServer";

const DWORD kPipeDesiredAccess = FILE_READ_DATA |
                                 FILE_WRITE_DATA |
                                 FILE_WRITE_ATTRIBUTES;

const DWORD kPipeFlagsAndAttributes = SECURITY_IDENTIFICATION |
                                      SECURITY_SQOS_PRESENT;

const DWORD kPipeMode = PIPE_READMODE_MESSAGE;

int kCustomInfoCount = 2;

google_breakpad::CustomInfoEntry kCustomInfoEntries[] = {
    google_breakpad::CustomInfoEntry(L"prod", L"CrashGenerationServerTest"),
    google_breakpad::CustomInfoEntry(L"ver", L"1.0"),
};

class CrashGenerationServerTest : public ::testing::Test {
 public:
  CrashGenerationServerTest()
      : crash_generation_server_(kPipeName,
                                 NULL,
                                 CallOnClientConnected, &mock_callbacks_,
                                 CallOnClientDumpRequested, &mock_callbacks_,
                                 CallOnClientExited, &mock_callbacks_,
                                 CallOnClientUploadRequested, &mock_callbacks_,
                                 false,
                                 NULL),
        thread_id_(0),
        exception_pointers_(NULL) {
    memset(&assert_info_, 0, sizeof(assert_info_));
  }

 protected:
  class MockCrashGenerationServerCallbacks {
   public:
    MOCK_METHOD1(OnClientConnected,
                 void(const google_breakpad::ClientInfo* client_info));
    MOCK_METHOD2(OnClientDumpRequested,
                 void(const google_breakpad::ClientInfo* client_info,
                      const std::wstring* file_path));
    MOCK_METHOD1(OnClientExited,
                 void(const google_breakpad::ClientInfo* client_info));
    MOCK_METHOD1(OnClientUploadRequested,
                 void(const DWORD crash_id));
  };

  enum ClientFault {
    NO_FAULT,
    CLOSE_AFTER_CONNECT,
    SEND_INVALID_REGISTRATION,
    TRUNCATE_REGISTRATION,
    CLOSE_AFTER_REGISTRATION,
    RESPONSE_BUFFER_TOO_SMALL,
    CLOSE_AFTER_RESPONSE,
    SEND_INVALID_ACK
  };

  void SetUp() {
    ASSERT_TRUE(crash_generation_server_.Start());
  }

  void FaultyClient(ClientFault fault_type) {
    HANDLE pipe = CreateFile(kPipeName,
                             kPipeDesiredAccess,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             kPipeFlagsAndAttributes,
                             NULL);

    if (pipe == INVALID_HANDLE_VALUE) {
      ASSERT_EQ(ERROR_PIPE_BUSY, GetLastError());

      // Cannot continue retrying if wait on pipe fails.
      ASSERT_TRUE(WaitNamedPipe(kPipeName, 500));

      pipe = CreateFile(kPipeName,
                        kPipeDesiredAccess,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        kPipeFlagsAndAttributes,
                        NULL);
    }

    ASSERT_NE(pipe, INVALID_HANDLE_VALUE);

    DWORD mode = kPipeMode;
    ASSERT_TRUE(SetNamedPipeHandleState(pipe, &mode, NULL, NULL));

    DoFaultyClient(fault_type, pipe);

    CloseHandle(pipe);
  }

  void DoTestFault(ClientFault fault) {
    EXPECT_CALL(mock_callbacks_, OnClientConnected(_)).Times(0);
    ASSERT_NO_FATAL_FAILURE(FaultyClient(fault));
    ASSERT_NO_FATAL_FAILURE(FaultyClient(fault));
    ASSERT_NO_FATAL_FAILURE(FaultyClient(fault));

    EXPECT_CALL(mock_callbacks_, OnClientConnected(_));

    ASSERT_NO_FATAL_FAILURE(FaultyClient(NO_FAULT));

    // Slight hack. The OnClientConnected is only invoked after the ack is
    // received by the server. At that point, the FaultyClient call has already
    // returned. The best way to wait until the server is done handling that is
    // to send one more ping, whose processing will be blocked by delivery of
    // the OnClientConnected message.
    ASSERT_NO_FATAL_FAILURE(FaultyClient(CLOSE_AFTER_CONNECT));
  }

  MockCrashGenerationServerCallbacks mock_callbacks_;

 private:
  // Depends on the caller to successfully open the pipe before invocation and
  // to close it immediately afterwards.
  void DoFaultyClient(ClientFault fault_type, HANDLE pipe) {
    if (fault_type == CLOSE_AFTER_CONNECT) {
      return;
    }

    google_breakpad::CustomClientInfo custom_info = {kCustomInfoEntries,
                                                     kCustomInfoCount};

    google_breakpad::ProtocolMessage msg(
      fault_type == SEND_INVALID_REGISTRATION ?
        google_breakpad::MESSAGE_TAG_NONE :
        google_breakpad::MESSAGE_TAG_REGISTRATION_REQUEST,
      GetCurrentProcessId(),
      MiniDumpNormal,
      &thread_id_,
      &exception_pointers_,
      &assert_info_,
      custom_info,
      NULL,
      NULL,
      NULL);

    DWORD bytes_count = 0;

    ASSERT_TRUE(WriteFile(pipe,
                          &msg,
                          fault_type == TRUNCATE_REGISTRATION ?
                            sizeof(msg) / 2 : sizeof(msg),
                          &bytes_count,
                          NULL));

    if (fault_type == CLOSE_AFTER_REGISTRATION) {
      return;
    }

    google_breakpad::ProtocolMessage reply;

    if (!ReadFile(pipe,
                  &reply,
                  fault_type == RESPONSE_BUFFER_TOO_SMALL ?
                    sizeof(google_breakpad::ProtocolMessage) / 2 :
                    sizeof(google_breakpad::ProtocolMessage),
                  &bytes_count,
                  NULL)) {
      switch (fault_type) {
        case TRUNCATE_REGISTRATION:
        case RESPONSE_BUFFER_TOO_SMALL:
        case SEND_INVALID_REGISTRATION:
          return;

        default:
          FAIL() << "Unexpectedly failed to register.";
      }
    }

    if (fault_type == CLOSE_AFTER_RESPONSE) {
      return;
    }

    google_breakpad::ProtocolMessage ack_msg;
    ack_msg.tag = google_breakpad::MESSAGE_TAG_REGISTRATION_ACK;

    ASSERT_TRUE(WriteFile(pipe,
                          &ack_msg,
                          SEND_INVALID_ACK ?
                            sizeof(ack_msg) : sizeof(ack_msg) / 2,
                          &bytes_count,
                          NULL));

    return;
  }

  static void CallOnClientConnected(
    void* context, const google_breakpad::ClientInfo* client_info) {
    static_cast<MockCrashGenerationServerCallbacks*>(context)->
      OnClientConnected(client_info);
  }

  static void CallOnClientDumpRequested(
    void* context,
    const google_breakpad::ClientInfo* client_info,
    const std::wstring* file_path) {
    static_cast<MockCrashGenerationServerCallbacks*>(context)->
      OnClientDumpRequested(client_info, file_path);
  }

  static void CallOnClientExited(
    void* context, const google_breakpad::ClientInfo* client_info) {
    static_cast<MockCrashGenerationServerCallbacks*>(context)->
      OnClientExited(client_info);
  }

  static void CallOnClientUploadRequested(void* context, const DWORD crash_id) {
    static_cast<MockCrashGenerationServerCallbacks*>(context)->
      OnClientUploadRequested(crash_id);
  }

  DWORD thread_id_;
  EXCEPTION_POINTERS* exception_pointers_;
  MDRawAssertionInfo assert_info_;

  google_breakpad::CrashGenerationServer crash_generation_server_;
};

TEST_F(CrashGenerationServerTest, PingServerTest) {
  DoTestFault(CLOSE_AFTER_CONNECT);
}

TEST_F(CrashGenerationServerTest, InvalidRegistration) {
  DoTestFault(SEND_INVALID_REGISTRATION);
}

TEST_F(CrashGenerationServerTest, TruncateRegistration) {
  DoTestFault(TRUNCATE_REGISTRATION);
}

TEST_F(CrashGenerationServerTest, CloseAfterRegistration) {
  DoTestFault(CLOSE_AFTER_REGISTRATION);
}

TEST_F(CrashGenerationServerTest, ResponseBufferTooSmall) {
  DoTestFault(RESPONSE_BUFFER_TOO_SMALL);
}

TEST_F(CrashGenerationServerTest, CloseAfterResponse) {
  DoTestFault(CLOSE_AFTER_RESPONSE);
}

// It turns out that, as long as you send one byte, the ACK is accepted and
// registration succeeds.
TEST_F(CrashGenerationServerTest, SendInvalidAck) {
  EXPECT_CALL(mock_callbacks_, OnClientConnected(_));
  ASSERT_NO_FATAL_FAILURE(FaultyClient(SEND_INVALID_ACK));

  // See DoTestFault for an explanation of this line
  ASSERT_NO_FATAL_FAILURE(FaultyClient(CLOSE_AFTER_CONNECT));

  EXPECT_CALL(mock_callbacks_, OnClientConnected(_));
  ASSERT_NO_FATAL_FAILURE(FaultyClient(NO_FAULT));

  // See DoTestFault for an explanation of this line
  ASSERT_NO_FATAL_FAILURE(FaultyClient(CLOSE_AFTER_CONNECT));
}

}  // anonymous namespace
