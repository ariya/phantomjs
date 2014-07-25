//
//  GTMLogger.m
//
//  Copyright 2007-2008 Google Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not
//  use this file except in compliance with the License.  You may obtain a copy
//  of the License at
// 
//  http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
//  License for the specific language governing permissions and limitations under
//  the License.
//

#import "GTMLogger.h"
#import "GTMGarbageCollection.h"
#import <fcntl.h>
#import <unistd.h>
#import <stdlib.h>
#import <pthread.h>


// Define a trivial assertion macro to avoid dependencies
#ifdef DEBUG
  #define GTMLOGGER_ASSERT(expr) assert(expr)
#else
  #define GTMLOGGER_ASSERT(expr)
#endif


@interface GTMLogger (PrivateMethods)

- (void)logInternalFunc:(const char *)func
                 format:(NSString *)fmt
                 valist:(va_list)args 
                  level:(GTMLoggerLevel)level;

@end


// Reference to the shared GTMLogger instance. This is not a singleton, it's 
// just an easy reference to one shared instance.
static GTMLogger *gSharedLogger = nil;


@implementation GTMLogger

// Returns a pointer to the shared logger instance. If none exists, a standard 
// logger is created and returned.
+ (id)sharedLogger {
  @synchronized(self) {
    if (gSharedLogger == nil) {
      gSharedLogger = [[self standardLogger] retain];
    }
    GTMLOGGER_ASSERT(gSharedLogger != nil);
  }
  return [[gSharedLogger retain] autorelease];
}

+ (void)setSharedLogger:(GTMLogger *)logger {
  @synchronized(self) {
    [gSharedLogger autorelease];
    gSharedLogger = [logger retain];
  }
}

+ (id)standardLogger {
  id<GTMLogWriter> writer = [NSFileHandle fileHandleWithStandardOutput];
  id<GTMLogFormatter> fr = [[[GTMLogStandardFormatter alloc] init] autorelease];
  id<GTMLogFilter> filter = [[[GTMLogLevelFilter alloc] init] autorelease];
  return [self loggerWithWriter:writer formatter:fr filter:filter];
}

+ (id)standardLoggerWithStderr {
  id me = [self standardLogger];
  [me setWriter:[NSFileHandle fileHandleWithStandardError]];
  return me;
}

+ (id)standardLoggerWithPath:(NSString *)path {
  NSFileHandle *fh = [NSFileHandle fileHandleForLoggingAtPath:path mode:0644];
  if (fh == nil) return nil;
  id me = [self standardLogger];
  [me setWriter:fh];
  return me;
}

+ (id)loggerWithWriter:(id<GTMLogWriter>)writer
             formatter:(id<GTMLogFormatter>)formatter
                filter:(id<GTMLogFilter>)filter {
  return [[[self alloc] initWithWriter:writer
                             formatter:formatter
                                filter:filter] autorelease];
}

+ (id)logger {
  return [[[self alloc] init] autorelease];
}

- (id)init {
  return [self initWithWriter:nil formatter:nil filter:nil];
}

- (id)initWithWriter:(id<GTMLogWriter>)writer
           formatter:(id<GTMLogFormatter>)formatter
              filter:(id<GTMLogFilter>)filter {
  if ((self = [super init])) {
    [self setWriter:writer];
    [self setFormatter:formatter];
    [self setFilter:filter];
    GTMLOGGER_ASSERT(formatter_ != nil);
    GTMLOGGER_ASSERT(filter_ != nil);
    GTMLOGGER_ASSERT(writer_ != nil);
  }
  return self;
}

- (void)dealloc {
  GTMLOGGER_ASSERT(writer_ != nil);
  GTMLOGGER_ASSERT(formatter_ != nil);
  GTMLOGGER_ASSERT(filter_ != nil);
  [writer_ release];
  [formatter_ release];
  [filter_ release];
  [super dealloc];
}

- (id<GTMLogWriter>)writer {
  GTMLOGGER_ASSERT(writer_ != nil);
  return [[writer_ retain] autorelease];
}

- (void)setWriter:(id<GTMLogWriter>)writer {
  @synchronized(self) {
    [writer_ autorelease];
    if (writer == nil)
      writer_ = [[NSFileHandle fileHandleWithStandardOutput] retain];
    else
      writer_ = [writer retain];
  }
  GTMLOGGER_ASSERT(writer_ != nil);
}

- (id<GTMLogFormatter>)formatter {
  GTMLOGGER_ASSERT(formatter_ != nil);
  return [[formatter_ retain] autorelease];
}

- (void)setFormatter:(id<GTMLogFormatter>)formatter {
  @synchronized(self) {
    [formatter_ autorelease];
    if (formatter == nil)
      formatter_ = [[GTMLogBasicFormatter alloc] init];
    else
      formatter_ = [formatter retain];
  }
  GTMLOGGER_ASSERT(formatter_ != nil);
}

- (id<GTMLogFilter>)filter {
  GTMLOGGER_ASSERT(filter_ != nil);
  return [[filter_ retain] autorelease];
}

- (void)setFilter:(id<GTMLogFilter>)filter {
  @synchronized(self) {
    [filter_ autorelease];
    if (filter == nil)
      filter_ = [[GTMLogNoFilter alloc] init];
    else
      filter_ = [filter retain];
  }
  GTMLOGGER_ASSERT(filter_ != nil);
}

- (void)logDebug:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:NULL format:fmt valist:args level:kGTMLoggerLevelDebug];
  va_end(args);
}

- (void)logInfo:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:NULL format:fmt valist:args level:kGTMLoggerLevelInfo];
  va_end(args);
}

- (void)logError:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:NULL format:fmt valist:args level:kGTMLoggerLevelError];
  va_end(args);
}

- (void)logAssert:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:NULL format:fmt valist:args level:kGTMLoggerLevelAssert];
  va_end(args);
}

@end  // GTMLogger


@implementation GTMLogger (GTMLoggerMacroHelpers)

- (void)logFuncDebug:(const char *)func msg:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:func format:fmt valist:args level:kGTMLoggerLevelDebug];
  va_end(args);
}

- (void)logFuncInfo:(const char *)func msg:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:func format:fmt valist:args level:kGTMLoggerLevelInfo];
  va_end(args);
}

- (void)logFuncError:(const char *)func msg:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:func format:fmt valist:args level:kGTMLoggerLevelError];
  va_end(args);
}

- (void)logFuncAssert:(const char *)func msg:(NSString *)fmt, ... {
  va_list args;
  va_start(args, fmt);
  [self logInternalFunc:func format:fmt valist:args level:kGTMLoggerLevelAssert];
  va_end(args);
}

@end  // GTMLoggerMacroHelpers


@implementation GTMLogger (PrivateMethods)

- (void)logInternalFunc:(const char *)func
                 format:(NSString *)fmt
                 valist:(va_list)args 
                  level:(GTMLoggerLevel)level {
  GTMLOGGER_ASSERT(formatter_ != nil);
  GTMLOGGER_ASSERT(filter_ != nil);
  GTMLOGGER_ASSERT(writer_ != nil);
  
  NSString *fname = func ? [NSString stringWithUTF8String:func] : nil;
  NSString *msg = [formatter_ stringForFunc:fname
                                 withFormat:fmt
                                     valist:args 
                                      level:level];
  if (msg && [filter_ filterAllowsMessage:msg level:level])
    [writer_ logMessage:msg level:level];
}

@end  // PrivateMethods


@implementation NSFileHandle (GTMFileHandleLogWriter)

+ (id)fileHandleForLoggingAtPath:(NSString *)path mode:(mode_t)mode {
  int fd = -1;
  if (path) {
    int flags = O_WRONLY | O_APPEND | O_CREAT;
    fd = open([path fileSystemRepresentation], flags, mode);
  }
  if (fd == -1) return nil;
  return [[[self alloc] initWithFileDescriptor:fd
                                closeOnDealloc:YES] autorelease];
}

- (void)logMessage:(NSString *)msg level:(GTMLoggerLevel)level {
  @synchronized(self) {
    NSString *line = [NSString stringWithFormat:@"%@\n", msg];
    [self writeData:[line dataUsingEncoding:NSUTF8StringEncoding]];
  }
}

@end  // GTMFileHandleLogWriter


@implementation NSArray (GTMArrayCompositeLogWriter)

- (void)logMessage:(NSString *)msg level:(GTMLoggerLevel)level {
  @synchronized(self) {
    id<GTMLogWriter> child = nil;
    GTM_FOREACH_OBJECT(child, self) {
      if ([child conformsToProtocol:@protocol(GTMLogWriter)])
        [child logMessage:msg level:level];
    }
  }
}

@end  // GTMArrayCompositeLogWriter


@implementation GTMLogger (GTMLoggerLogWriter)

- (void)logMessage:(NSString *)msg level:(GTMLoggerLevel)level {
  switch (level) {
    case kGTMLoggerLevelDebug:
      [self logDebug:@"%@", msg]; 
      break;
    case kGTMLoggerLevelInfo:
      [self logInfo:@"%@", msg];
      break;
    case kGTMLoggerLevelError:   
      [self logError:@"%@", msg];
      break;
    case kGTMLoggerLevelAssert:
      [self logAssert:@"%@", msg];
      break;
    default: 
      // Ignore the message.
      break;
  }
}

@end  // GTMLoggerLogWriter


@implementation GTMLogBasicFormatter

- (NSString *)stringForFunc:(NSString *)func
                 withFormat:(NSString *)fmt
                     valist:(va_list)args 
                      level:(GTMLoggerLevel)level {
  // Performance note: since we always have to create a new NSString from the 
  // returned CFStringRef, we may want to do a quick check here to see if |fmt|
  // contains a '%', and if not, simply return 'fmt'. 
  CFStringRef cfmsg = NULL;  
  cfmsg = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, 
                                               NULL,  // format options
                                               (CFStringRef)fmt, 
                                               args);
  return GTMCFAutorelease(cfmsg);
}

@end  // GTMLogBasicFormatter


@implementation GTMLogStandardFormatter

- (id)init {
  if ((self = [super init])) {
    dateFormatter_ = [[NSDateFormatter alloc] init];
    [dateFormatter_ setFormatterBehavior:NSDateFormatterBehavior10_4];
    [dateFormatter_ setDateFormat:@"yyyy-MM-dd HH:mm:ss.SSS"];
    pname_ = [[[NSProcessInfo processInfo] processName] copy];
    pid_ = [[NSProcessInfo processInfo] processIdentifier];
  }
  return self;
}

- (void)dealloc {
  [dateFormatter_ release];
  [pname_ release];
  [super dealloc];
}

- (NSString *)stringForFunc:(NSString *)func
                 withFormat:(NSString *)fmt
                     valist:(va_list)args 
                      level:(GTMLoggerLevel)level {
  GTMLOGGER_ASSERT(dateFormatter_ != nil);
  NSString *tstamp = nil;
  @synchronized (dateFormatter_) {
    tstamp = [dateFormatter_ stringFromDate:[NSDate date]];
  }
  return [NSString stringWithFormat:@"%@ %@[%d/%p] [lvl=%d] %@ %@",
          tstamp, pname_, pid_, pthread_self(),
          level, (func ? func : @"(no func)"),
          [super stringForFunc:func withFormat:fmt valist:args level:level]];
}

@end  // GTMLogStandardFormatter


@implementation GTMLogLevelFilter

// Check the environment and the user preferences for the GTMVerboseLogging key
// to see if verbose logging has been enabled. The environment variable will
// override the defaults setting, so check the environment first.
// COV_NF_START
static BOOL IsVerboseLoggingEnabled(void) {
  static NSString *const kVerboseLoggingKey = @"GTMVerboseLogging";
  static char *env = NULL;
  if (env == NULL)
    env = getenv([kVerboseLoggingKey UTF8String]);
  
  if (env && env[0]) {
    return (strtol(env, NULL, 10) != 0);
  }

  return [[NSUserDefaults standardUserDefaults] boolForKey:kVerboseLoggingKey];
}
// COV_NF_END

// In DEBUG builds, log everything. If we're not in a debug build we'll assume
// that we're in a Release build.
- (BOOL)filterAllowsMessage:(NSString *)msg level:(GTMLoggerLevel)level {
#if DEBUG
  return YES;
#endif
    
  BOOL allow = YES;
  
  switch (level) {
    case kGTMLoggerLevelDebug:
      allow = NO;
      break;
    case kGTMLoggerLevelInfo:
      allow = (IsVerboseLoggingEnabled() == YES);
      break;
    case kGTMLoggerLevelError:
      allow = YES;
      break;
    case kGTMLoggerLevelAssert:
      allow = YES;
      break;
    default:
      allow = YES;
      break;
  }

  return allow;
}

@end  // GTMLogLevelFilter


@implementation GTMLogNoFilter

- (BOOL)filterAllowsMessage:(NSString *)msg level:(GTMLoggerLevel)level {
  return YES;  // Allow everything through
}

@end  // GTMLogNoFilter
