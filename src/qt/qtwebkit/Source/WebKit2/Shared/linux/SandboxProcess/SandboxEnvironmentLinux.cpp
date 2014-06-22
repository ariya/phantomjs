/*
 * Copyright (C) 2013 University of Szeged
 * Copyright (C) 2013 Renata Hodovan <reni@inf.u-szeged.hu>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "SandboxEnvironmentLinux.h"

#include <dirent.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <link.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utime.h>
#include <vector>

static const unsigned maximumPathLength = 512;
static char sandboxDirectory[maximumPathLength];
static uid_t sandboxUserUID;
static uid_t sandboxUserGID;

static inline void strlcpy(char *destination, const char* source, int maxLength)
{
    destination[0] = '\0';
    strncat(destination, source, maxLength - 1);
}

static inline void strlcat(char* destination, const char* source, int maxLength)
{
    strncat(destination, source, maxLength - 1 - strnlen(destination, maxLength - 1));
}

static inline void appendDirectoryComponent(char* fullPath, const char* directoryPath, const char* fileName)
{
    strlcpy(fullPath, directoryPath, maximumPathLength);
    strlcat(fullPath, fileName, maximumPathLength);
}

// This function runs in a cloned process and it is waiting for a request message
// from WebProcess to perform the chroot(). If the operation was successful the function
// never returns. So this function has no return value.
static void launchChangeRootHelper(int helperSocket, int webProcessSocket)
{
    // We need to restrict the resources available to our process to avoid opening
    // a file by mistake. However, CAP_SYS_RESOURCE capability should be dropped
    // otherwise it won't work.
    struct rlimit restrictedResource = { 0, 0 };
    if (setrlimit(RLIMIT_NOFILE, &restrictedResource) == -1) {
        fprintf(stderr, "Helper couldn't set the resource limit: %s.\n", strerror(errno));
        return;
    }

    if (close(webProcessSocket) == -1) {
        fprintf(stderr, "Failed to close socket %d: %s.\n", webProcessSocket, strerror(errno));
        return;
    }

    char message;
    // We expect a 'C' (ChrootMe) message from the WebProcess.
    if (read(helperSocket, &message, 1) != 1) {
        fprintf(stderr, "Failed to read message from the web process: %s %d.\n", strerror(errno), errno);
        return;
    }

    if (message != MSG_CHROOTME) {
        fprintf(stderr, "Wrong message recieved: %x.\n", message);
        return;
    }

    struct stat sandboxDirectoryInfo;
    if (lstat(sandboxDirectory, &sandboxDirectoryInfo) == -1) {
        fprintf(stderr, "Sandbox directory (%s) is not available: %s.\n", sandboxDirectory, strerror(errno));
        return;
    }

    if (!S_ISDIR(sandboxDirectoryInfo.st_mode)) {
        fprintf(stderr, "%s is not a directory!\n", sandboxDirectory);
        return;
    }

    if (chroot(sandboxDirectory) == -1) {
        fprintf(stderr, "Chrooting failed: %s.\n", strerror(errno));
        return;
    }

    // Chroot only changes the root directory of the calling process but doesn't change
    // the current working directory. Therefore, if we don't do it manually a malicious user
    // could break out the jail with relative paths.
    if (chdir("/") == -1) {
        fprintf(stderr, "Couldn't change the working directory to /.: %s\n", strerror(errno));
        return;
    }

    // Sending acknowledgement to the WebProcess that the sandboxing was successfull.
    message = MSG_CHROOTED;
    if (write(helperSocket, &message, 1) != 1) {
        fprintf(stderr, "Couldn't send acknowledgement to WebProcess: %s.\n", strerror(errno));
        return;
    }
    exit(EXIT_SUCCESS);
}

static bool setEnvironmentVariablesForChangeRootHelper(pid_t pid, int helperSocket, int webProcessSocket)
{
    const int descriptorSize = 32;
    char socketDescriptor[descriptorSize];
    char sandboxHelperPID[descriptorSize];

    int length = snprintf(sandboxHelperPID, sizeof(sandboxHelperPID), "%u", pid);
    if (length < 0 || length >= sizeof(sandboxHelperPID)) {
        fprintf(stderr, "Failed to convert the sandbox helper PID to a string.\n");
        return false;
    }

    if (setenv(SANDBOX_HELPER_PID, sandboxHelperPID, 1) == -1) {
        fprintf(stderr, "Couldn't set the SBX_HELPER_PID environment variable: %s\n", strerror(errno));
        return false;
    }

    length = snprintf(socketDescriptor, sizeof(socketDescriptor), "%u", webProcessSocket);
    if (length < 0 || length >= sizeof(socketDescriptor)) {
        fprintf(stderr, "Failed to convert the sandbox helper file descriptor to a string.\n");
        return false;
    }

    if (setenv(SANDBOX_DESCRIPTOR, socketDescriptor, 1) == -1) {
        fprintf(stderr, "Failed to store the helper's file descriptor into an environment variable: %s.\n", strerror(errno));
        return false;
    }

    if (close(helperSocket) == -1) {
        fprintf(stderr, "Closing of %d failed: %s.\n", helperSocket, strerror(errno));
        return false;
    }

    return true;
}

static bool prepareAndStartChangeRootHelper()
{
    int socketPair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) == -1) {
        fprintf(stderr, "Couldn't create socketpair: %s\n", strerror(errno));
        return false;
    }

    pid_t pid = syscall(SYS_clone, CLONE_FS | SIGCHLD, 0, 0, 0);
    if (pid == -1) {
        fprintf(stderr, "Clone failed: %s\n", strerror(errno));
        return false;
    }
    if (!pid) {
        // Child process: we start the chroot helper which waits for the "ChrootMe"
        // message from the WebProcess. If we are successed, then we won't return.
        launchChangeRootHelper(socketPair[0], socketPair[1]);
        // We reach this part only if launchChrootHelper() failed, instead it should have exited.
        exit(EXIT_FAILURE);
        return false;
    }

    // Parent process: exports the pid of the helper and the socket id so the
    // helper and the WebProcess can communicate.
    return setEnvironmentVariablesForChangeRootHelper(pid, socketPair[0], socketPair[1]);
}

// Setting linux capabilities (permitted, effective and inheritable) for the current process.
// Permitted set indicates the capabilities what could be set for the process.
// Effective set is a subset of permitted set, they are actually effective.
// Inheritable set indicates the capabilities what the children will inherit from the current process.
static bool setCapabilities(cap_value_t* capabilityList, int length)
{
    // Capabilities should be initialized without flags.
    cap_t capabilities = cap_init();
    if (!capabilities) {
        fprintf(stderr, "Failed to initialize process capabilities: %s.\n", strerror(errno));
        return false;
    }

    if (cap_clear(capabilities) == -1) {
        fprintf(stderr, "Failed to clear process capabilities: %s.\n", strerror(errno));
        return false;
    }

    if (capabilityList && length) {
        if (cap_set_flag(capabilities, CAP_EFFECTIVE, length, capabilityList, CAP_SET) == -1
            || cap_set_flag(capabilities, CAP_INHERITABLE, length, capabilityList, CAP_SET) == -1
            || cap_set_flag(capabilities, CAP_PERMITTED, length, capabilityList, CAP_SET) == -1) {
            fprintf(stderr, "Failed to set process capability flags: %s.\n", strerror(errno));
            cap_free(capabilities);
            return false;
        }
    }

    if (cap_set_proc(capabilities) == -1) {
        fprintf(stderr, "Failed to set process capabilities: %s.\n", strerror(errno));
        cap_free(capabilities);
        return false;
    }

    cap_free(capabilities);
    return true;
}

static bool dropPrivileges()
{
    // We become explicitely non dumpable.
    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) == -1) {
        fprintf(stderr, "Setting dumpable is failed: %s\n", strerror(errno));
        return false;
    }

    if (setresgid(sandboxUserGID, sandboxUserGID, sandboxUserGID) == -1) {
        fprintf(stderr, "Failed to fallback to group: %d.\n", sandboxUserGID);
        return false;
    }

    if (setresuid(sandboxUserUID, sandboxUserUID, sandboxUserUID) == -1) {
        fprintf(stderr, "Failed to fallback to user: %d.\n", sandboxUserUID);
        return false;
    }

    // Drop all capabilities. Again, setuid() normally takes care of this if we had euid 0.
    return setCapabilities(0, 0);
}

static bool fileExists(const char* path)
{
    struct stat fileStat;
    if (lstat(path, &fileStat) == -1) {
        if (errno == ENOENT)
            return false;
    }
    return true;
}

static mode_t directoryPermissions(const char* directory)
{
    struct stat fileStat;
    if (lstat(directory, &fileStat) == -1) {
        fprintf(stderr, "Failed to obtain information about directory (%s): %s\n", directory, strerror(errno));
        return false;
    }
    return fileStat.st_mode;
}

static bool createDirectory(char* pathToCreate, const char* nextDirectoryToCreate)
{
    strlcat(pathToCreate, nextDirectoryToCreate, maximumPathLength);

    char pathToCreateInSandbox[maximumPathLength];
    appendDirectoryComponent(pathToCreateInSandbox, sandboxDirectory, pathToCreate);

    mode_t mode = directoryPermissions(pathToCreate);
    if (mkdir(pathToCreateInSandbox, mode) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "Creation of %s failed: %s\n", pathToCreateInSandbox, strerror(errno));
            return false;
        }
    }

    struct stat fileInfo;
    if (lstat(pathToCreate, &fileInfo) == -1) {
        fprintf(stderr, "Couldn't obtain information about directory (%s): %s\n", pathToCreate, strerror(errno));
        return false;
    }
    if (fileInfo.st_uid == getuid()) {
        if (chown(pathToCreateInSandbox, sandboxUserUID, sandboxUserGID) == -1) {
            fprintf(stderr, "Failed to assign the ownership of %s to the sandbox user: %s.\n", pathToCreateInSandbox, strerror(errno));
            return false;
        }
    }
    if (chmod(pathToCreateInSandbox, fileInfo.st_mode) == -1) {
        fprintf(stderr, "Failed to set the permissions of %s: %s.\n", pathToCreateInSandbox, strerror(errno));
        return false;
    }
    return true;
}

// This function creates a directory chain with the given path.
// First, it splits up the path by '/'-s and walks through the chunks from the base directory.
// It checks the existance of the actual path and creates it if it doesn't exist yet.
static bool createDirectoryChain(const char* path)
{
    char fullPathInSandbox[maximumPathLength];
    appendDirectoryComponent(fullPathInSandbox, sandboxDirectory, path);

    if (fileExists(fullPathInSandbox))
        return true;

    char alreadyCreatedPath[maximumPathLength];
    alreadyCreatedPath[0] = '\0';
    // startPos is (path + 1) because we skip the first '/'.
    const char* startPos = path + 1;
    const char* endPos;
    while ((endPos = strchr(startPos, '/'))) {
        char nextDirectoryToCreate[maximumPathLength];
        strlcpy(nextDirectoryToCreate, startPos - 1, strnlen(startPos - 1, endPos - startPos + 1) + 1);

        if (!createDirectory(alreadyCreatedPath, nextDirectoryToCreate))
            return false;
        startPos = endPos + 1;
    }
    // Create the last directory of the directory path.
    alreadyCreatedPath[0] = '\0';
    return createDirectory(alreadyCreatedPath, path);
}

static bool createDeviceFiles()
{
    const char* devDirectory = "/dev";
    if (!createDirectoryChain(devDirectory))
        return false;

    const char* devices[2] = { "/dev/random", "/dev/urandom" };
    for (int i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
        struct stat status;
        if (lstat(devices[i], &status)) {
            fprintf(stderr, "Failed to stat device file (%s): %s\n", devices[i], strerror(errno));
            return false;
        }
        dev_t dev = status.st_rdev;

        // Both needed device files (/dev/random and /dev/urandom) are character m_devices and their permissions should be: rw-rw-rw-.
        char device[maximumPathLength];
        appendDirectoryComponent(device, sandboxDirectory, devices[i]);

        if (mknod(device, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, makedev(major(dev), minor(dev))) == -1) {
            if (errno != EEXIST) {
                fprintf(stderr, "Couldn't create device file %s: %s\n", device, strerror(errno));
                return false;
            }
        }
    }
    return true;
}

static bool mountFileSystems()
{
    const char* procPath = "/proc";
    if (!createDirectoryChain(procPath))
        return false;
    char procPathInSandbox[maximumPathLength];
    appendDirectoryComponent(procPathInSandbox, sandboxDirectory, procPath);

    if (mount(procPath, procPathInSandbox, "proc", 0, 0) == -1) {
        if (errno != EBUSY) {
            fprintf(stderr, "Failed to mount '%s': %s\n", procPath, strerror(errno));
            return false;
        }
    }

    const char* sharedMemoryPath = "/run/shm";
    if (!createDirectoryChain(sharedMemoryPath)) {
        fprintf(stderr, "Failed to create directory for /run/shm in the sandbox: %s.\n", strerror(errno));
        return false;
    }
    char sharedMemoryPathInSandbox[maximumPathLength];
    appendDirectoryComponent(sharedMemoryPathInSandbox, sandboxDirectory, sharedMemoryPath);

    if (mount(sharedMemoryPath, sharedMemoryPathInSandbox, "tmpfs", 0, 0) == -1) {
        if (errno != EBUSY) {
            fprintf(stderr, "Failed to mount '%s': %s.\n", sharedMemoryPath, strerror(errno));
            return false;
        }
    }
    return true;
}

static bool linkFile(const char* sourceFile, const char* targetFile)
{
    char oldPath[maximumPathLength];
    char targetPath[maximumPathLength];
    strlcpy(oldPath, sourceFile, maximumPathLength);
    strlcpy(targetPath, targetFile, maximumPathLength);

    while (true) {
        struct stat fileInfo;
        if (lstat(oldPath, &fileInfo) == -1) {
            if (errno != ENOENT) {
                fprintf(stderr, "Couldn't obtain information about %s: %s\n", oldPath, strerror(errno));
                return false;
            }
            // If the original file doesn't exist (e.g. dangling links) then we can ignore it
            // in the sandbox too.
            return true;
        }
        const char* endOfBaseDirectoryInSource = strrchr(oldPath, '/');
        if (!endOfBaseDirectoryInSource) {
            fprintf(stderr, "Invalid source: %s.\n", oldPath);
            return false;
        }

        char baseDirectoryOfSource[maximumPathLength];
        // To determine the length of the base directory we have to consider the tailing
        // slash (+1) and adding plus one because strlcpy() copies (maxLength - 1) characters
        // from the source.
        strlcpy(baseDirectoryOfSource, oldPath, endOfBaseDirectoryInSource - oldPath + 2);

        if (!createDirectoryChain(baseDirectoryOfSource)) {
            fprintf(stderr, "Creating %s failed: %s.\n", baseDirectoryOfSource, strerror(errno));
            return false;
        }

        if (link(oldPath, targetPath) == -1) {
            if (errno != EEXIST && errno != ENOENT) {
                fprintf(stderr, "Linking %s failed: %s.\n", oldPath, strerror(errno));
                return false;
            }
        }

        // Handle symlinks. We don't want to have dangling links in the sandbox. So we have to
        // follow them and put the whole link chain into the sandbox.
        if ((fileInfo.st_mode & S_IFMT) != S_IFLNK)
            break;

        char symlinkTarget[maximumPathLength];
        int lengthOfTheLink = readlink(oldPath, symlinkTarget, sizeof(symlinkTarget) - 1);
        if (lengthOfTheLink > 0)
            symlinkTarget[lengthOfTheLink] = '\0';

        char symlinkTargetInRealWorld[maximumPathLength];
        char symlinkTargetInSandbox[maximumPathLength];

        // Making difference between relative and absolute paths.
        // If the symlinks target starts with '/' then we have nothing to do with it.
        // Otherwise it's a relative path and we have to concatenate it to the current
        // path to obtain the target.
        if (symlinkTarget[0] == '/') {
            strlcpy(symlinkTargetInRealWorld, symlinkTarget, maximumPathLength);
            appendDirectoryComponent(symlinkTargetInSandbox, sandboxDirectory, symlinkTarget);
        } else {
            appendDirectoryComponent(symlinkTargetInRealWorld, baseDirectoryOfSource, symlinkTarget);
            appendDirectoryComponent(symlinkTargetInSandbox, sandboxDirectory, symlinkTargetInRealWorld);
        }

        // Initialize oldPath and targetPath variables for the next loop of while.
        oldPath[0] = '\0';
        targetPath[0] = '\0';
        strlcat(oldPath, symlinkTargetInRealWorld, maximumPathLength);
        strlcat(targetPath, symlinkTargetInSandbox, maximumPathLength);
    }
    return true;
}

// This function extends the standard link function by linking directories and all their contents
// and subdirectories recursively.
static bool linkDirectory(const char* sourceDirectoryPath, const char* targetDirectoryPath)
{
    if (!createDirectoryChain(sourceDirectoryPath))
        return false;
    DIR* directory = opendir(sourceDirectoryPath);
    if (!directory) {
        fprintf(stderr, "Couldn't open directory %s: %s\n", sourceDirectoryPath, strerror(errno));
        return false;
    }

    while (struct dirent *directoryInfo = readdir(directory)) {
        char* fileName = directoryInfo->d_name;
        // We must not link '.' and ".." into the sandbox.
        if (!strcmp(fileName, ".") || !strcmp(fileName, ".."))
            continue;
        char sourceFile[maximumPathLength];
        char targetFile[maximumPathLength];
        appendDirectoryComponent(sourceFile, sourceDirectoryPath, fileName);
        appendDirectoryComponent(targetFile, targetDirectoryPath, fileName);

        bool returnValue;
        if (directoryInfo->d_type == DT_DIR) {
            strncat(sourceFile, "/", 1);
            strncat(targetFile, "/", 1);
            returnValue = linkDirectory(sourceFile, targetFile);
        } else
            returnValue = linkFile(sourceFile, targetFile);
        if (!returnValue)
            return false;
    }

    // Restore the original modification time of the directories because
    // it could have meaning e.g. in the hash generation of cache files.
    struct stat fileStat;
    if (lstat(sourceDirectoryPath, &fileStat) == -1) {
        fprintf(stderr, "Failed to obtain information about the directory '%s': %s\n", sourceDirectoryPath, strerror(errno));
        return false;
    }
    struct utimbuf times;
    times.actime = fileStat.st_atime;
    times.modtime = fileStat.st_mtime;
    if (utime(targetDirectoryPath, &times) == -1) {
        fprintf(stderr, "Couldn't set back the last modification time of '%s': %s\n", targetDirectoryPath, strerror(errno));
        return false;
    }
    return true;
}

static bool collectRunTimeDependencies()
{
    // The list of empirically gathered library dependencies.
    const char* runtimeDependencies[] = {
        "libnss_dns.so",
        "libresolv.so",
        "libssl.so",
        "libcrypto.so"
    };

    for (int i = 0; i < sizeof(runtimeDependencies) / sizeof(runtimeDependencies[0]); ++i) {
        // To obtain the path of the runtime dependencies we open them with dlopen.
        // With the handle supplied by dlopen we can obtain information about the dynamically
        // linked libraries, so the path where are they installed.
        void* handle = dlopen(runtimeDependencies[i], RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "Couldn't get the handler of %s: %s\n", runtimeDependencies[i], dlerror());
            return false;
        }

        struct link_map* linkMap;
        if (dlinfo(handle, RTLD_DI_LINKMAP, &linkMap) == -1) {
            fprintf(stderr, "Couldn't get information about %s: %s\n", runtimeDependencies[i], dlerror());
            return false;
        }

        if (!linkMap) {
            fprintf(stderr, "Couldn't get the linkmap of %s: %s.\n", runtimeDependencies[i], strerror(errno));
            return false;
        }

        char pathOfTheLibraryInSandbox[maximumPathLength];
        appendDirectoryComponent(pathOfTheLibraryInSandbox, sandboxDirectory, linkMap->l_name);
        if (!linkFile(linkMap->l_name, pathOfTheLibraryInSandbox)) {
            fprintf(stderr, "Linking runtime dependency: %s failed: %s\n", linkMap->l_name, strerror(errno));
            dlclose(handle);
            return false;
        }
        dlclose(handle);
    }
    return true;
}

static bool setupXauthorityForNobodyUser()
{
    // To be able use X inside the sandbox an .Xauthority file must be exist inside it,
    // owned by the sandboxuser. Furthermore, XAUTHORITY environment variable must point to it.
    char buffer[BUFSIZ];
    size_t size;
    struct passwd* realUser = getpwuid(getuid());
    if (!realUser) {
        fprintf(stderr, "Couldn't obtain the current user: %s\n", strerror(errno));
        return false;
    }

    char xauthorityOfRealUser[maximumPathLength];
    char xauthorityInSandbox[maximumPathLength];
    appendDirectoryComponent(xauthorityOfRealUser, realUser->pw_dir, "/.Xauthority");
    appendDirectoryComponent(xauthorityInSandbox, sandboxDirectory, xauthorityOfRealUser);

    FILE* source = fopen(xauthorityOfRealUser, "rb");
    if (!source) {
        fprintf(stderr, "Couldn't open %s: %s\n", xauthorityOfRealUser, strerror(errno));
        return false;
    }

    FILE* dest = fopen(xauthorityInSandbox, "wb");
    if (!dest) {
        fprintf(stderr, "Couldn't open %s: %s\n", xauthorityInSandbox, strerror(errno));
        return false;
    }

    // We copy the .Xauthority file of the real user (instead of linking) because 'nobody' user
    // should own it but we don't want to change the permissions of the original file.
    while ((size = fread(buffer, 1, BUFSIZ, source))) {
        if (fwrite(buffer, 1, size, dest) != size) {
            fprintf(stderr, "Failed to copy .Xauthority to the sandbox: %s.\n", strerror(errno));
            return false;
        }
    }

    if (fclose(source)) {
        fprintf(stderr, "Closing the .Xauthority file of the real user failed: %s\n", strerror(errno));
        return false;
    }

    if (fclose(dest)) {
        fprintf(stderr, "Closing the .Xauthority file of the sandbox user failed: %s\n", strerror(errno));
        return false;
    }

    if (chown(xauthorityInSandbox, sandboxUserUID, sandboxUserGID) == -1) {
        fprintf(stderr, "Chowning .Xauthority (%s) failed: %s.\n", xauthorityInSandbox, strerror(errno));
        return false;
    }

    if (setenv("XAUTHORITY", xauthorityInSandbox, 1) == -1) {
        fprintf(stderr, "Couldn't set the XAUTHORITY envrionment variable: %s\n", strerror(errno));
        return false;
    }
    return true;
}

static bool initializeSandbox()
{
    // Create the sandbox directory. We only need to enter it, so
    // the executable permission is needed only.
    if (mkdir(sandboxDirectory, S_IFDIR | S_IXUSR | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            fprintf(stderr, "Couldn't create the sandbox directory: %s\n", strerror(errno));
            return false;
        }
    }

    if (!createDeviceFiles())
        return false;

    if (!mountFileSystems())
        return false;

    // Hard link cache and font directories into the sandbox environment.
    struct passwd* userInfo = getpwuid(getuid());
    const char* home = userInfo->pw_dir;

    char localDirectory[maximumPathLength];
    char cacheDirectory[maximumPathLength];
    char fontDirectory[maximumPathLength];

    appendDirectoryComponent(localDirectory, home, "/.local/share/");
    appendDirectoryComponent(cacheDirectory, home, "/.cache/");
    appendDirectoryComponent(fontDirectory, home, "/.fontconfig/");

    const char* linkedDirectories[] = {
        cacheDirectory,
        fontDirectory,
        localDirectory,
        "/etc/fonts/",
        "/etc/ssl/certs/",
        "/var/cache/fontconfig/",
        "/usr/share/fonts/"
    };

    for (int i = 0; i < sizeof(linkedDirectories) / sizeof(linkedDirectories[0]); ++i) {
        char linkedDirectoryInSandbox[maximumPathLength];
        appendDirectoryComponent(linkedDirectoryInSandbox, sandboxDirectory, linkedDirectories[i]);

        if (!linkDirectory(linkedDirectories[i], linkedDirectoryInSandbox))
            return false;
    }

    if (!setupXauthorityForNobodyUser())
        return false;

    return collectRunTimeDependencies();
}

static bool restrictCapabilities()
{
    // Capabilities we need.
    // CAP_SYS_ADMIN capabilty is added because cloning with CLONE_NEWPID flag later will need it.
    cap_value_t capabilityList[] = { CAP_SETUID, CAP_SETGID, CAP_SYS_ADMIN, CAP_SYS_CHROOT};

    // Reduce capabilities to what we need.
    // Although we still have root euid and we keep root equivalent capabilities,
    // we removed (= didn't add) CAP_SYS_RESSOURCE capabilites and this resulted that
    // the setrlimit function with RLIMIT_NOFILE will be effective later.
    if (!setCapabilities(capabilityList, sizeof(capabilityList) / sizeof(capabilityList[0]))) {
        fprintf(stderr, "Could not adjust process capabilities: %s.\n", strerror(errno));
        return false;
    }
    return true;
}

static bool moveToNewPIDNamespace()
{
    // CLONE_NEWPID and CLONE_FS should be in that order.
    // We can't share filesystems accross namespaces.
    int status;
    pid_t expectedPID;
    pid_t pid = syscall(SYS_clone, CLONE_NEWPID | SIGCHLD, 0, 0, 0);

    if (pid == -1) {
        fprintf(stderr, "Cloning is failed: %s\n", strerror(errno));
        return false;
    }
    if (!pid) {
        // Child should run with pid number 1 in the new namespace.
        if (getpid() != 1) {
            fprintf(stderr, "Couldn't create a new PID namespace.\n");
            return false;
        }
        return true;
    }

    // We are waiting for our child (WebProcess).
    // If this wait is successful it means that our child is terminated.
    expectedPID = waitpid(pid, &status, 0);
    if (expectedPID != pid) {
        fprintf(stderr, "Process with PID %d terminated instead of the expected one with PID %d: %s.\n", expectedPID, pid, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (WIFEXITED(status))
        exit(WEXITSTATUS(status));
    exit(EXIT_SUCCESS);
}

static bool run(int argc, char *const argv[])
{
    struct passwd* userInfo = getpwuid(getuid());
    if (!userInfo) {
        fprintf(stderr, "Couldn't get the current user: %s.\n", strerror(errno));
        return false;
    }
    appendDirectoryComponent(sandboxDirectory, userInfo->pw_dir, "/.wk2-sandbox");

    // Currently we use 'nobody' user as the sandbox user and fall back to the real user
    // if we failed to get it (we could extend this in the future with a specific restricted user).
    if (struct passwd* nobodyUser = getpwnam("nobody")) {
        sandboxUserUID = nobodyUser->pw_uid;
        sandboxUserGID = nobodyUser->pw_gid;
    } else {
        sandboxUserUID = getuid();
        sandboxUserGID = getgid();
    }

    // We should have three parameters:
    // path_of_this_binary path_of_the_webprocess socket_to_communicate_with_uiprocess
    if (argc != 3) {
        fprintf(stderr, "Starting SandboxProcess requires 3 parameters!\n");
        return false;
    }

    // SandboxProcess should be run with suid flag ...
    if (geteuid()) {
        fprintf(stderr, "The sandbox is not seteuid root.\n");
        return false;
    }

    // ... but not as root (not with sudo).
    if (!getuid()) {
        fprintf(stderr, "The sandbox is not designed to be run by root.\n");
        return false;
    }

    if (!initializeSandbox())
        return false;

    if (!restrictCapabilities())
        return false;

    // We move ourself and our children into a new PID namespace,
    // where process IDs start from 0 again.
    if (!moveToNewPIDNamespace())
        return false;

    // Starting a helper what will waiting for the "chrootme" message from WebProcess.
    if (!prepareAndStartChangeRootHelper())
        return false;

    // We don't need any special privileges anymore.
    if (!dropPrivileges())
        return false;

    // Sanity check: if our effective or real uid/gid is still 0 (root) or
    // we can set any of them to 0, then the dropping of privileges is failed.
    // We ensure here that we cannot set root id after here.
    if (!geteuid() || !getegid() || !setuid(0) || !setgid(0)) {
        fprintf(stderr, "Dropping privileges failed!\n");
        return false;
    }

    // Start the WebProcess.
    if (execl(argv[1], argv[1], argv[2], reinterpret_cast<char*>(0)) == -1) {
        fprintf(stderr, "Couldn't start WebProcess: %s\n", strerror(errno));
        return false;
    }
    return true;
}

int main(int argc, char *const argv[])
{
    return run(argc, argv) ? 0 : 1;
}
