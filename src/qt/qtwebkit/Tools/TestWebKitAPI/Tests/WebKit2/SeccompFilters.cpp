/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <WebKit2/SeccompBroker.h>
#include <WebKit2/SeccompFilters.h>
#include <WebKit2/SyscallPolicy.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebKit;

namespace TestWebKitAPI {

DEFINE_STATIC_LOCAL(String, rootDir, (ASCIILiteral("/")));
DEFINE_STATIC_LOCAL(String, homeDir, (String(getenv("HOME"))));
DEFINE_STATIC_LOCAL(String, usrDir, (ASCIILiteral("/usr")));
DEFINE_STATIC_LOCAL(String, usrSbinDir, (ASCIILiteral("/usr/sbin")));
DEFINE_STATIC_LOCAL(String, testDirRead, (ASCIILiteral("/tmp/WebKitSeccompFilters/testRead")));
DEFINE_STATIC_LOCAL(String, testDirWrite, (ASCIILiteral("/tmp/WebKitSeccompFilters/testWrite")));
DEFINE_STATIC_LOCAL(String, testDirReadAndWrite, (ASCIILiteral("/tmp/WebKitSeccompFilters/testReadAndWrite")));
DEFINE_STATIC_LOCAL(String, testDirNotAllowed, (ASCIILiteral("/tmp/WebKitSeccompFilters/testNotAllowed")));
DEFINE_STATIC_LOCAL(String, testFileNotAllowed, (testDirReadAndWrite + "/testFilePolicy"));
DEFINE_STATIC_LOCAL(String, testFileReadAndWrite, (testDirNotAllowed + "/testFilePolicy"));

static const mode_t defaultMode = S_IRUSR | S_IWUSR | S_IXUSR;

class SeccompEnvironment : public testing::Environment {
public:
    virtual void SetUp()
    {
        ASSERT_TRUE(!homeDir.isEmpty());

        mkdir("/tmp/WebKitSeccompFilters", defaultMode);
        mkdir(testDirRead.utf8().data(), defaultMode);
        mkdir(testDirWrite.utf8().data(), defaultMode);
        mkdir(testDirReadAndWrite.utf8().data(), defaultMode);
        mkdir(testDirNotAllowed.utf8().data(), defaultMode);

        // Create a file for the Read only and NotAllowed directory before
        // loading the filters.
        String file = testDirRead + "/testFile";
        int fd = open(file.utf8().data(), O_RDWR | O_CREAT, defaultMode);
        ASSERT_NE(close(fd), -1);
        file = testDirNotAllowed + "/testFile";
        fd = open(file.utf8().data(), O_RDWR | O_CREAT, defaultMode);
        ASSERT_NE(close(fd), -1);

        // Create files for the file policy tests. File policies precedes the
        // directory policy. In this case, we create a file with read and write
        // policies inside a directory that is not allowed, and vice versa. 
        fd = open(testFileNotAllowed.utf8().data(), O_RDWR | O_CREAT, defaultMode);
        ASSERT_NE(close(fd), -1);
        fd = open(testFileReadAndWrite.utf8().data(), O_RDWR | O_CREAT, defaultMode);
        ASSERT_NE(close(fd), -1);

        SyscallPolicy policy;
        policy.addDirectoryPermission(rootDir, SyscallPolicy::NotAllowed);
        policy.addDirectoryPermission(usrDir, SyscallPolicy::Read);
        policy.addDirectoryPermission(usrSbinDir, SyscallPolicy::NotAllowed);
        policy.addDirectoryPermission(testDirRead, SyscallPolicy::Read);
        policy.addDirectoryPermission(testDirWrite, SyscallPolicy::Write);
        policy.addDirectoryPermission(testDirReadAndWrite, SyscallPolicy::ReadAndWrite);
        policy.addDirectoryPermission(testDirNotAllowed, SyscallPolicy::NotAllowed);
        policy.addFilePermission(testFileNotAllowed, SyscallPolicy::NotAllowed);
        policy.addFilePermission(testFileReadAndWrite, SyscallPolicy::ReadAndWrite);

        SeccompFilters seccompFilters(SeccompFilters::Allow);
        seccompFilters.addRule("open", SeccompFilters::Trap);
        seccompFilters.addRule("openat", SeccompFilters::Trap);
        seccompFilters.addRule("creat", SeccompFilters::Trap);

        SeccompBroker::launchProcess(&seccompFilters, policy);
        seccompFilters.initialize();
    }

    virtual void TearDown()
    {
        // This will have to move to a separated process created before loading
        // the filters when we put the rmdir/unlink policies in place.
        unlink("/tmp/WebKitSeccompFilters/testNotAllowed/testFile");
        unlink("/tmp/WebKitSeccompFilters/testNotAllowed/testFilePolicy");
        unlink("/tmp/WebKitSeccompFilters/testReadAndWrite/testFile");
        unlink("/tmp/WebKitSeccompFilters/testReadAndWrite/testFile2");
        unlink("/tmp/WebKitSeccompFilters/testReadAndWrite/testFile3");
        unlink("/tmp/WebKitSeccompFilters/testReadAndWrite/testFilePolicy");
        unlink("/tmp/WebKitSeccompFilters/testWrite/testFile");
        unlink("/tmp/WebKitSeccompFilters/testWrite/testFile2");
        unlink("/tmp/WebKitSeccompFilters/testRead/testFile");
        rmdir("/tmp/WebKitSeccompFilters/testNotAllowed");
        rmdir("/tmp/WebKitSeccompFilters/testReadAndWrite");
        rmdir("/tmp/WebKitSeccompFilters/testWrite");
        rmdir("/tmp/WebKitSeccompFilters/testRead");
        rmdir("/tmp/WebKitSeccompFilters");
    }
};

::testing::Environment* const env = ::testing::AddGlobalTestEnvironment(new SeccompEnvironment);

static void dummyHandler(int, siginfo_t*, void*)
{
}

TEST(WebKit2, sigaction)
{
    // Setting a handler should be enough to break any subsequent test if
    // not silently ignored by the sandbox.
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = &dummyHandler;
    action.sa_flags = SA_SIGINFO;

    ASSERT_NE(sigaction(SIGSYS, &action, 0), -1);
}

TEST(WebKit2, sigprocmask)
{
    // We test here the mechanism installed to prevent SIGSYS to be blocked. Any
    // attemp to add SIGSYS to the set of blocked signals will be silently
    // ignored (but other signals will be blocked just fine).
    sigset_t set, oldSet;
    sigemptyset(&set);
    sigaddset(&set, SIGSYS);
    sigaddset(&set, SIGUSR1);

    ASSERT_NE(sigprocmask(SIG_BLOCK, &set, 0), -1);
    ASSERT_NE(sigprocmask(SIG_BLOCK, 0, &oldSet), -1);
    ASSERT_FALSE(sigismember(&oldSet, SIGSYS)) << "SIGSYS should not be blocked.";
    ASSERT_TRUE(sigismember(&oldSet, SIGUSR1)) << "Other signals should be blocked normally.";

    sigemptyset(&set);
    sigaddset(&set, SIGSYS);
    sigaddset(&set, SIGUSR2);

    ASSERT_NE(sigprocmask(SIG_SETMASK, &set, &oldSet), -1);
    ASSERT_NE(sigprocmask(SIG_SETMASK, 0, &set), -1);
    ASSERT_FALSE(sigismember(&set, SIGSYS)) << "SIGSYS should not be blocked.";
    ASSERT_TRUE(sigismember(&set, SIGUSR2)) << "Other signals should be blocked normally.";
    ASSERT_FALSE(sigismember(&oldSet, SIGUSR2));

    ASSERT_NE(sigprocmask(SIG_SETMASK, &oldSet, 0), -1) << "Should restore the old signal set just fine.";
    ASSERT_NE(sigprocmask(SIG_SETMASK, 0, &set), -1);
    ASSERT_FALSE(sigismember(&set, SIGUSR2)) << "The restored set doesn't have SIGUSR2.";
}

TEST(WebKit2, open)
{
    // Read only directory.
    String file = testDirRead + "/testFile";
    int fd = open(file.utf8().data(), O_RDWR);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_RDONLY | O_CREAT, defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    file = testDirRead + "/ThisFileDoesNotExist";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == ENOENT) << "Should return ENOENT when trying " \
        "to open a file that does not exit and the permissions are OK.";

    fd = open(file.utf8().data(), O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES) << "Should return EACCES when trying " \
        "to open a file that does not exit and the permissions are not OK.";

    // Write only directory.
    file = testDirWrite + "/testFile";
    fd = open(file.utf8().data(), O_WRONLY | O_CREAT, defaultMode);
    ASSERT_NE(fd, -1);
    close(fd);

    fd = open(file.utf8().data(), O_RDWR);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_WRONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    // Read an write directory.
    file = testDirReadAndWrite + "/testFile";
    fd = open(file.utf8().data(), O_WRONLY | O_CREAT, defaultMode);
    ASSERT_NE(fd, -1);
    close(fd);

    fd = open(file.utf8().data(), O_RDWR);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = open(file.utf8().data(), O_WRONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    // NotAllowed directory.
    file = testDirNotAllowed + "/testFile";
    fd = open(file.utf8().data(), O_WRONLY | O_CREAT, defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_RDWR);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(file.utf8().data(), O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);


    // The /usr directory here has read permissions, so it's subdirectories
    // should resolve to the /usr permissions unless explicitly specified.
    file = usrDir + "/bin/basename";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_NE(fd, -1) << "Subdirectories should with no policy should " \
        "inherit the parent's policies.";
    close(fd);

    file = usrSbinDir + "/adduser";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES) << "This directory should have " \
        "its own policy instead of the parent's.";

    // Access to the rest of the files system is blocked and should
    // never return anything else other than EACCES regardless if the
    // file exists or not. The reason is because it will fallback to the
    // policy of the Root directory, marked as NotAllowed.
    file = homeDir + "/testFile";
    fd = open(file.utf8().data(), O_RDWR | O_CREAT, defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open("/etc/passwd", O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    file = testDirReadAndWrite + "/../../../etc/passwd";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    file = testDirReadAndWrite + "/../../.." + testDirReadAndWrite + "/../../../etc/passwd";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    // Here we test file policies. The have precedence over directory policies.
    // The file bellow lives inside a directory with ReadAndWrite policy.
    fd = open(testFileNotAllowed.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(testFileNotAllowed.utf8().data(), O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = open(testFileNotAllowed.utf8().data(), O_RDWR);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    file = testDirReadAndWrite + "/../../.." + testDirReadAndWrite + "/testFilePolicy";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    // The next file is located inside a directory marked as NotAllowed, but
    // it has its own file policy that precedes the directory policy.
    fd = open(testFileReadAndWrite.utf8().data(), O_RDONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = open(testFileReadAndWrite.utf8().data(), O_WRONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = open(testFileReadAndWrite.utf8().data(), O_RDWR);
    EXPECT_NE(fd, -1);
    close(fd);

    file = testDirReadAndWrite + "/../../.." + testDirNotAllowed + "/testFilePolicy";
    fd = open(file.utf8().data(), O_RDONLY);
    EXPECT_NE(fd, -1);
    close(fd);
}

TEST(WebKit2, creat)
{
    // Read only directory.
    String file = testDirRead + "/testFile2";
    int fd = creat(file.utf8().data(), defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    // Write only directory.
    file = testDirWrite + "/testFile2";
    fd = creat(file.utf8().data(), defaultMode);
    EXPECT_NE(fd, -1);
    close(fd);

    // Read an write directory.
    file = testDirReadAndWrite + "/testFile2";
    fd = creat(file.utf8().data(), defaultMode);
    EXPECT_NE(fd, -1);
    close(fd);

    // NotAllowed directory.
    file = testDirNotAllowed + "/testFile2";
    fd = creat(file.utf8().data(), defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);
}

TEST(WebKit2, openat)
{
    int dirFd = open(testDirReadAndWrite.utf8().data(), O_RDONLY);
    ASSERT_NE(dirFd, -1);

    int fd = openat(dirFd, "testFile3", O_RDWR | O_CREAT, defaultMode);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = openat(dirFd, "testFile3", O_RDWR);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = openat(dirFd, "testFile3", O_RDONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    fd = openat(dirFd, "testFile3", O_WRONLY);
    EXPECT_NE(fd, -1);

    fd = openat(fd, "testFile3", O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == ENOTDIR) << "Should return ENOTDIR when the fd is a file.";
    close(fd);

    String file = "../../.." + testDirReadAndWrite + "/testFile3";
    fd = openat(dirFd, file.utf8().data(), O_WRONLY);
    EXPECT_NE(fd, -1);
    close(fd);

    file = "../../.." + testDirRead + "/testFile3";
    fd = openat(dirFd, file.utf8().data(), O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    file = testDirReadAndWrite + "/testFile3";
    fd = openat(-1, file.utf8().data(), O_WRONLY);
    EXPECT_NE(fd, -1) << "Directory fd should be ignored when the path is absolute.";
    close(fd);

    fd = openat(-1, "testFile3", O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EBADF) << "Should return EBADF when the fd is invalid.";
    close(dirFd);

    dirFd = open(testDirNotAllowed.utf8().data(), O_RDONLY);
    EXPECT_TRUE(dirFd == -1 && errno == EACCES);

    dirFd = open(testDirRead.utf8().data(), O_RDONLY);
    ASSERT_NE(dirFd, -1);

    fd = openat(dirFd, "testFile2", O_RDONLY | O_CREAT, defaultMode);
    EXPECT_TRUE(fd == -1 && errno == EACCES);

    fd = openat(dirFd, "testFile", O_WRONLY);
    EXPECT_TRUE(fd == -1 && errno == EACCES);
    close(dirFd);
}

static void* stressTest(void*)
{
    for (int i = 0; i < 500; ++i) {
        int fd = open("/tmp/WebKitSeccompFilters/testRead/testFile", O_RDWR);
        EXPECT_TRUE(fd == -1 && errno == EACCES);

        fd = open("/tmp/WebKitSeccompFilters/testRead/testFile", O_RDONLY);
        EXPECT_NE(fd, -1);
        close(fd);

        fd = open("/tmp/WebKitSeccompFilters/testNotAllowed/testFile", O_RDONLY);
        EXPECT_TRUE(fd == -1 && errno == EACCES);

        fd = creat("/tmp/WebKitSeccompFilters/testNotAllowed/SholdNotBeAllowed", defaultMode);
        EXPECT_TRUE(fd == -1 && errno == EACCES);

        int dirFd = open("/tmp/WebKitSeccompFilters/testRead", O_RDONLY);
        EXPECT_NE(dirFd, -1);

        fd = openat(dirFd, "testFile", O_RDONLY);
        EXPECT_NE(fd, -1);
        close(fd);
        close(dirFd);
    }

    return 0;
}

TEST(WebKit2, threading)
{
    // Tests if concurrent syscall execution works fine. It can be
    // also used for performance testing and leak detection. The test
    // is disabled on Debug mode because it can be way too verbose.
    pthread_t threads[5];

    for (int i = 0; i < sizeof(threads) / sizeof(pthread_t); ++i)
        pthread_create(&threads[i], 0, stressTest, 0);

    for (int i = 0; i < sizeof(threads) / sizeof(pthread_t); ++i)
        pthread_join(threads[i], 0);
}

} // namespace TestWebKitAPI
