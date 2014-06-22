// Copyright (c) 2011, Google Inc.
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

#import <fcntl.h>
#import <sys/stat.h>
#include <TargetConditionals.h>
#import <unistd.h>

#import <SystemConfiguration/SystemConfiguration.h>

#import "common/mac/HTTPMultipartUpload.h"

#import "client/apple/Framework/BreakpadDefines.h"
#import "client/mac/sender/uploader.h"
#import "common/mac/GTMLogger.h"

const int kMinidumpFileLengthLimit = 2 * 1024 * 1024;  // 2MB

#define kApplePrefsSyncExcludeAllKey \
  @"com.apple.PreferenceSync.ExcludeAllSyncKeys"

NSString *const kGoogleServerType = @"google";
NSString *const kSocorroServerType = @"socorro";
NSString *const kDefaultServerType = @"google";

#pragma mark -

namespace {
// Read one line from the configuration file.
NSString *readString(int fileId) {
  NSMutableString *str = [NSMutableString stringWithCapacity:32];
  char ch[2] = { 0 };

  while (read(fileId, &ch[0], 1) == 1) {
    if (ch[0] == '\n') {
      // Break if this is the first newline after reading some other string
      // data.
      if ([str length])
        break;
    } else {
      [str appendString:[NSString stringWithUTF8String:ch]];
    }
  }

  return str;
}

//=============================================================================
// Read |length| of binary data from the configuration file. This method will
// returns |nil| in case of error.
NSData *readData(int fileId, ssize_t length) {
  NSMutableData *data = [NSMutableData dataWithLength:length];
  char *bytes = (char *)[data bytes];

  if (read(fileId, bytes, length) != length)
    return nil;

  return data;
}

//=============================================================================
// Read the configuration from the config file.
NSDictionary *readConfigurationData(const char *configFile) {
  int fileId = open(configFile, O_RDONLY, 0600);
  if (fileId == -1) {
    GTMLoggerDebug(@"Couldn't open config file %s - %s",
                   configFile,
                   strerror(errno));
  }

  // we want to avoid a build-up of old config files even if they
  // have been incorrectly written by the framework
  if (unlink(configFile)) {
    GTMLoggerDebug(@"Couldn't unlink config file %s - %s",
                   configFile,
                   strerror(errno));
  }

  if (fileId == -1) {
    return nil;
  }

  NSMutableDictionary *config = [NSMutableDictionary dictionary];

  while (1) {
    NSString *key = readString(fileId);

    if (![key length])
      break;

    // Read the data.  Try to convert to a UTF-8 string, or just save
    // the data
    NSString *lenStr = readString(fileId);
    ssize_t len = [lenStr intValue];
    NSData *data = readData(fileId, len);
    id value = [[NSString alloc] initWithData:data
                                     encoding:NSUTF8StringEncoding];

    [config setObject:(value ? value : data) forKey:key];
    [value release];
  }

  close(fileId);
  return config;
}
}  // namespace

#pragma mark -

@interface Uploader(PrivateMethods)

// Update |parameters_| as well as the server parameters using |config|.
- (void)translateConfigurationData:(NSDictionary *)config;

// Read the minidump referenced in |parameters_| and update |minidumpContents_|
// with its content.
- (BOOL)readMinidumpData;

// Read the log files referenced in |parameters_| and update |logFileData_|
// with their content.
- (BOOL)readLogFileData;

// Returns a unique client id (user-specific), creating a persistent
// one in the user defaults, if necessary.
- (NSString*)clientID;

// Returns a dictionary that can be used to map Breakpad parameter names to
// URL parameter names.
- (NSMutableDictionary *)dictionaryForServerType:(NSString *)serverType;

// Helper method to set HTTP parameters based on server type.  This is
// called right before the upload - crashParameters will contain, on exit,
// URL parameters that should be sent with the minidump.
- (BOOL)populateServerDictionary:(NSMutableDictionary *)crashParameters;

// Initialization helper to create dictionaries mapping Breakpad
// parameters to URL parameters
- (void)createServerParameterDictionaries;

// Accessor method for the URL parameter dictionary
- (NSMutableDictionary *)urlParameterDictionary;

// Records the uploaded crash ID to the log file.
- (void)logUploadWithID:(const char *)uploadID;
@end

@implementation Uploader

//=============================================================================
- (id)initWithConfigFile:(const char *)configFile {
  NSDictionary *config = readConfigurationData(configFile);
  if (!config)
    return nil;

  return [self initWithConfig:config];
}

//=============================================================================
- (id)initWithConfig:(NSDictionary *)config {
  if ((self = [super init])) {
    // Because the reporter is embedded in the framework (and many copies
    // of the framework may exist) its not completely certain that the OS
    // will obey the com.apple.PreferenceSync.ExcludeAllSyncKeys in our
    // Info.plist. To make sure, also set the key directly if needed.
    NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
    if (![ud boolForKey:kApplePrefsSyncExcludeAllKey]) {
      [ud setBool:YES forKey:kApplePrefsSyncExcludeAllKey];
    }

    [self createServerParameterDictionaries];

    [self translateConfigurationData:config];

    // Read the minidump into memory.
    [self readMinidumpData];
    [self readLogFileData];
  }
  return self;
}

//=============================================================================
- (void)translateConfigurationData:(NSDictionary *)config {
  parameters_ = [[NSMutableDictionary alloc] init];

  NSEnumerator *it = [config keyEnumerator];
  while (NSString *key = [it nextObject]) {
    // If the keyname is prefixed by BREAKPAD_SERVER_PARAMETER_PREFIX
    // that indicates that it should be uploaded to the server along
    // with the minidump, so we treat it specially.
    if ([key hasPrefix:@BREAKPAD_SERVER_PARAMETER_PREFIX]) {
      NSString *urlParameterKey =
        [key substringFromIndex:[@BREAKPAD_SERVER_PARAMETER_PREFIX length]];
      if ([urlParameterKey length]) {
        id value = [config objectForKey:key];
        if ([value isKindOfClass:[NSString class]]) {
          [self addServerParameter:(NSString *)value
                            forKey:urlParameterKey];
        } else {
          [self addServerParameter:(NSData *)value
                            forKey:urlParameterKey];
        }
      }
    } else {
      [parameters_ setObject:[config objectForKey:key] forKey:key];
    }
  }

  // generate a unique client ID based on this host's MAC address
  // then add a key/value pair for it
  NSString *clientID = [self clientID];
  [parameters_ setObject:clientID forKey:@"guid"];
}

// Per user per machine
- (NSString *)clientID {
  NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
  NSString *crashClientID = [ud stringForKey:kClientIdPreferenceKey];
  if (crashClientID) {
    return crashClientID;
  }

  // Otherwise, if we have no client id, generate one!
  srandom((int)[[NSDate date] timeIntervalSince1970]);
  long clientId1 = random();
  long clientId2 = random();
  long clientId3 = random();
  crashClientID = [NSString stringWithFormat:@"%lx%lx%lx",
                            clientId1, clientId2, clientId3];

  [ud setObject:crashClientID forKey:kClientIdPreferenceKey];
  [ud synchronize];
  return crashClientID;
}

//=============================================================================
- (BOOL)readLogFileData {
#if TARGET_OS_IPHONE
  return NO;
#else
  unsigned int logFileCounter = 0;

  NSString *logPath;
  size_t logFileTailSize =
      [[parameters_ objectForKey:@BREAKPAD_LOGFILE_UPLOAD_SIZE] intValue];

  NSMutableArray *logFilenames; // An array of NSString, one per log file
  logFilenames = [[NSMutableArray alloc] init];

  char tmpDirTemplate[80] = "/tmp/CrashUpload-XXXXX";
  char *tmpDir = mkdtemp(tmpDirTemplate);

  // Construct key names for the keys we expect to contain log file paths
  for(logFileCounter = 0;; logFileCounter++) {
    NSString *logFileKey = [NSString stringWithFormat:@"%@%d",
                                     @BREAKPAD_LOGFILE_KEY_PREFIX,
                                     logFileCounter];

    logPath = [parameters_ objectForKey:logFileKey];

    // They should all be consecutive, so if we don't find one, assume
    // we're done

    if (!logPath) {
      break;
    }

    NSData *entireLogFile = [[NSData alloc] initWithContentsOfFile:logPath];

    if (entireLogFile == nil) {
      continue;
    }

    NSRange fileRange;

    // Truncate the log file, only if necessary

    if ([entireLogFile length] <= logFileTailSize) {
      fileRange = NSMakeRange(0, [entireLogFile length]);
    } else {
      fileRange = NSMakeRange([entireLogFile length] - logFileTailSize,
                              logFileTailSize);
    }

    char tmpFilenameTemplate[100];

    // Generate a template based on the log filename
    sprintf(tmpFilenameTemplate,"%s/%s-XXXX", tmpDir,
            [[logPath lastPathComponent] fileSystemRepresentation]);

    char *tmpFile = mktemp(tmpFilenameTemplate);

    NSData *logSubdata = [entireLogFile subdataWithRange:fileRange];
    NSString *tmpFileString = [NSString stringWithUTF8String:tmpFile];
    [logSubdata writeToFile:tmpFileString atomically:NO];

    [logFilenames addObject:[tmpFileString lastPathComponent]];
    [entireLogFile release];
  }

  if ([logFilenames count] == 0) {
    [logFilenames release];
    logFileData_ =  nil;
    return NO;
  }

  // now, bzip all files into one
  NSTask *tarTask = [[NSTask alloc] init];

  [tarTask setCurrentDirectoryPath:[NSString stringWithUTF8String:tmpDir]];
  [tarTask setLaunchPath:@"/usr/bin/tar"];

  NSMutableArray *bzipArgs = [NSMutableArray arrayWithObjects:@"-cjvf",
                                             @"log.tar.bz2",nil];
  [bzipArgs addObjectsFromArray:logFilenames];

  [logFilenames release];

  [tarTask setArguments:bzipArgs];
  [tarTask launch];
  [tarTask waitUntilExit];
  [tarTask release];

  NSString *logTarFile = [NSString stringWithFormat:@"%s/log.tar.bz2",tmpDir];
  logFileData_ = [[NSData alloc] initWithContentsOfFile:logTarFile];
  if (logFileData_ == nil) {
    GTMLoggerDebug(@"Cannot find temp tar log file: %@", logTarFile);
    return NO;
  }
  return YES;
#endif  // TARGET_OS_IPHONE
}

//=============================================================================
- (BOOL)readMinidumpData {
  NSString *minidumpDir =
      [parameters_ objectForKey:@kReporterMinidumpDirectoryKey];
  NSString *minidumpID = [parameters_ objectForKey:@kReporterMinidumpIDKey];

  if (![minidumpID length])
    return NO;

  NSString *path = [minidumpDir stringByAppendingPathComponent:minidumpID];
  path = [path stringByAppendingPathExtension:@"dmp"];

  // check the size of the minidump and limit it to a reasonable size
  // before attempting to load into memory and upload
  const char *fileName = [path fileSystemRepresentation];
  struct stat fileStatus;

  BOOL success = YES;

  if (!stat(fileName, &fileStatus)) {
    if (fileStatus.st_size > kMinidumpFileLengthLimit) {
      fprintf(stderr, "Breakpad Uploader: minidump file too large " \
              "to upload : %d\n", (int)fileStatus.st_size);
      success = NO;
    }
  } else {
      fprintf(stderr, "Breakpad Uploader: unable to determine minidump " \
              "file length\n");
      success = NO;
  }

  if (success) {
    minidumpContents_ = [[NSData alloc] initWithContentsOfFile:path];
    success = ([minidumpContents_ length] ? YES : NO);
  }

  if (!success) {
    // something wrong with the minidump file -- delete it
    unlink(fileName);
  }

  return success;
}

#pragma mark -
//=============================================================================

- (void)createServerParameterDictionaries {
  serverDictionary_ = [[NSMutableDictionary alloc] init];
  socorroDictionary_ = [[NSMutableDictionary alloc] init];
  googleDictionary_ = [[NSMutableDictionary alloc] init];
  extraServerVars_ = [[NSMutableDictionary alloc] init];

  [serverDictionary_ setObject:socorroDictionary_ forKey:kSocorroServerType];
  [serverDictionary_ setObject:googleDictionary_ forKey:kGoogleServerType];

  [googleDictionary_ setObject:@"ptime" forKey:@BREAKPAD_PROCESS_UP_TIME];
  [googleDictionary_ setObject:@"email" forKey:@BREAKPAD_EMAIL];
  [googleDictionary_ setObject:@"comments" forKey:@BREAKPAD_COMMENTS];
  [googleDictionary_ setObject:@"prod" forKey:@BREAKPAD_PRODUCT];
  [googleDictionary_ setObject:@"ver" forKey:@BREAKPAD_VERSION];
  [googleDictionary_ setObject:@"guid" forKey:@"guid"];

  [socorroDictionary_ setObject:@"Comments" forKey:@BREAKPAD_COMMENTS];
  [socorroDictionary_ setObject:@"CrashTime"
                         forKey:@BREAKPAD_PROCESS_CRASH_TIME];
  [socorroDictionary_ setObject:@"StartupTime"
                         forKey:@BREAKPAD_PROCESS_START_TIME];
  [socorroDictionary_ setObject:@"Version"
                         forKey:@BREAKPAD_VERSION];
  [socorroDictionary_ setObject:@"ProductName"
                         forKey:@BREAKPAD_PRODUCT];
  [socorroDictionary_ setObject:@"Email"
                         forKey:@BREAKPAD_EMAIL];
}

- (NSMutableDictionary *)dictionaryForServerType:(NSString *)serverType {
  if (serverType == nil || [serverType length] == 0) {
    return [serverDictionary_ objectForKey:kDefaultServerType];
  }
  return [serverDictionary_ objectForKey:serverType];
}

- (NSMutableDictionary *)urlParameterDictionary {
  NSString *serverType = [parameters_ objectForKey:@BREAKPAD_SERVER_TYPE];
  return [self dictionaryForServerType:serverType];

}

- (BOOL)populateServerDictionary:(NSMutableDictionary *)crashParameters {
  NSDictionary *urlParameterNames = [self urlParameterDictionary];

  id key;
  NSEnumerator *enumerator = [parameters_ keyEnumerator];

  while ((key = [enumerator nextObject])) {
    // The key from parameters_ corresponds to a key in
    // urlParameterNames.  The value in parameters_ gets stored in
    // crashParameters with a key that is the value in
    // urlParameterNames.

    // For instance, if parameters_ has [PRODUCT_NAME => "FOOBAR"] and
    // urlParameterNames has [PRODUCT_NAME => "pname"] the final HTTP
    // URL parameter becomes [pname => "FOOBAR"].
    NSString *breakpadParameterName = (NSString *)key;
    NSString *urlParameter = [urlParameterNames
                                   objectForKey:breakpadParameterName];
    if (urlParameter) {
      [crashParameters setObject:[parameters_ objectForKey:key]
                          forKey:urlParameter];
    }
  }

  // Now, add the parameters that were added by the application.
  enumerator = [extraServerVars_ keyEnumerator];

  while ((key = [enumerator nextObject])) {
    NSString *urlParameterName = (NSString *)key;
    NSString *urlParameterValue =
      [extraServerVars_ objectForKey:urlParameterName];
    [crashParameters setObject:urlParameterValue
                        forKey:urlParameterName];
  }
  return YES;
}

- (void)addServerParameter:(id)value forKey:(NSString *)key {
  [extraServerVars_ setObject:value forKey:key];
}

//=============================================================================
- (void)report {
  NSURL *url = [NSURL URLWithString:[parameters_ objectForKey:@BREAKPAD_URL]];
  HTTPMultipartUpload *upload = [[HTTPMultipartUpload alloc] initWithURL:url];
  NSMutableDictionary *uploadParameters = [NSMutableDictionary dictionary];

  if (![self populateServerDictionary:uploadParameters]) {
    [upload release];
    return;
  }

  [upload setParameters:uploadParameters];

  // Add minidump file
  if (minidumpContents_) {
    [upload addFileContents:minidumpContents_ name:@"upload_file_minidump"];

    // If there is a log file, upload it together with the minidump.
    if (logFileData_) {
      [upload addFileContents:logFileData_ name:@"log"];
    }

    // Send it
    NSError *error = nil;
    NSData *data = [upload send:&error];
    NSString *result = [[NSString alloc] initWithData:data
                                         encoding:NSUTF8StringEncoding];
    const char *reportID = "ERR";

    if (error) {
      fprintf(stderr, "Breakpad Uploader: Send Error: %s\n",
              [[error description] UTF8String]);
    } else {
      NSCharacterSet *trimSet =
          [NSCharacterSet whitespaceAndNewlineCharacterSet];
      reportID = [[result stringByTrimmingCharactersInSet:trimSet] UTF8String];
      [self logUploadWithID:reportID];
    }

    // rename the minidump file according to the id returned from the server
    NSString *minidumpDir =
        [parameters_ objectForKey:@kReporterMinidumpDirectoryKey];
    NSString *minidumpID = [parameters_ objectForKey:@kReporterMinidumpIDKey];

    NSString *srcString = [NSString stringWithFormat:@"%@/%@.dmp",
                                    minidumpDir, minidumpID];
    NSString *destString = [NSString stringWithFormat:@"%@/%s.dmp",
                                     minidumpDir, reportID];

    const char *src = [srcString fileSystemRepresentation];
    const char *dest = [destString fileSystemRepresentation];

    if (rename(src, dest) == 0) {
      GTMLoggerInfo(@"Breakpad Uploader: Renamed %s to %s after successful " \
                    "upload",src, dest);
    }
    else {
      // can't rename - don't worry - it's not important for users
      GTMLoggerDebug(@"Breakpad Uploader: successful upload report ID = %s\n",
                     reportID );
    }
    [result release];
  } else {
    // Minidump is missing -- upload just the log file.
    if (logFileData_) {
      [self uploadData:logFileData_ name:@"log"];
    }
  }
  [upload release];
}

- (void)uploadData:(NSData *)data name:(NSString *)name {
  NSURL *url = [NSURL URLWithString:[parameters_ objectForKey:@BREAKPAD_URL]];
  NSMutableDictionary *uploadParameters = [NSMutableDictionary dictionary];

  if (![self populateServerDictionary:uploadParameters])
    return;

  HTTPMultipartUpload *upload =
      [[HTTPMultipartUpload alloc] initWithURL:url];

  [uploadParameters setObject:name forKey:@"type"];
  [upload setParameters:uploadParameters];
  [upload addFileContents:data name:name];

  [upload send:nil];
  [upload release];
}

- (void)logUploadWithID:(const char *)uploadID {
  NSString *minidumpDir =
      [parameters_ objectForKey:@kReporterMinidumpDirectoryKey];
  NSString *logFilePath = [NSString stringWithFormat:@"%@/%s",
      minidumpDir, kReporterLogFilename];
  NSString *logLine = [NSString stringWithFormat:@"%0.f,%s\n",
      [[NSDate date] timeIntervalSince1970], uploadID];
  NSData *logData = [logLine dataUsingEncoding:NSUTF8StringEncoding];

  NSFileManager *fileManager = [NSFileManager defaultManager];
  if ([fileManager fileExistsAtPath:logFilePath]) {
    NSFileHandle *logFileHandle =
       [NSFileHandle fileHandleForWritingAtPath:logFilePath];
    [logFileHandle seekToEndOfFile];
    [logFileHandle writeData:logData];
    [logFileHandle closeFile];
  } else {
    [fileManager createFileAtPath:logFilePath
                         contents:logData
                       attributes:nil];
  }
}

//=============================================================================
- (NSMutableDictionary *)parameters {
  return parameters_;
}

//=============================================================================
- (void)dealloc {
  [parameters_ release];
  [minidumpContents_ release];
  [logFileData_ release];
  [googleDictionary_ release];
  [socorroDictionary_ release];
  [serverDictionary_ release];
  [extraServerVars_ release];
  [super dealloc];
}

@end
