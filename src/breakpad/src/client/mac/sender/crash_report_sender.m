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

#import "client/mac/sender/crash_report_sender.h"

#import <Cocoa/Cocoa.h>
#import <pwd.h>
#import <sys/stat.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <unistd.h>

#import "client/apple/Framework/BreakpadDefines.h"
#import "common/mac/GTMLogger.h"
#import "common/mac/HTTPMultipartUpload.h"


#define kLastSubmission @"LastSubmission"
const int kUserCommentsMaxLength = 1500;
const int kEmailMaxLength = 64;

#define kApplePrefsSyncExcludeAllKey \
  @"com.apple.PreferenceSync.ExcludeAllSyncKeys"

#pragma mark -

@interface NSView (ResizabilityExtentions)
// Shifts the view vertically by the given amount.
- (void)breakpad_shiftVertically:(CGFloat)offset;

// Shifts the view horizontally by the given amount.
- (void)breakpad_shiftHorizontally:(CGFloat)offset;
@end

@implementation NSView (ResizabilityExtentions)
- (void)breakpad_shiftVertically:(CGFloat)offset {
  NSPoint origin = [self frame].origin;
  origin.y += offset;
  [self setFrameOrigin:origin];
}

- (void)breakpad_shiftHorizontally:(CGFloat)offset {
  NSPoint origin = [self frame].origin;
  origin.x += offset;
  [self setFrameOrigin:origin];
}
@end

@interface NSWindow (ResizabilityExtentions)
// Adjusts the window height by heightDelta relative to its current height,
// keeping all the content at the same size.
- (void)breakpad_adjustHeight:(CGFloat)heightDelta;
@end

@implementation NSWindow (ResizabilityExtentions)
- (void)breakpad_adjustHeight:(CGFloat)heightDelta {
  [[self contentView] setAutoresizesSubviews:NO];

  NSRect windowFrame = [self frame];
  windowFrame.size.height += heightDelta;
  [self setFrame:windowFrame display:YES];
  // For some reason the content view is resizing, but not adjusting its origin,
  // so correct it manually.
  [[self contentView] setFrameOrigin:NSMakePoint(0, 0)];

  [[self contentView] setAutoresizesSubviews:YES];
}
@end

@interface NSTextField (ResizabilityExtentions)
// Grows or shrinks the height of the field to the minimum required to show the
// current text, preserving the existing width and origin.
// Returns the change in height.
- (CGFloat)breakpad_adjustHeightToFit;

// Grows or shrinks the width of the field to the minimum required to show the
// current text, preserving the existing height and origin.
// Returns the change in width.
- (CGFloat)breakpad_adjustWidthToFit;
@end

@implementation NSTextField (ResizabilityExtentions)
- (CGFloat)breakpad_adjustHeightToFit {
  NSRect oldFrame = [self frame];
  // Starting with the 10.5 SDK, height won't grow, so make it huge to start.
  NSRect presizeFrame = oldFrame;
  presizeFrame.size.height = MAXFLOAT;
  // sizeToFit will blow out the width rather than making the field taller, so
  // we do it manually.
  NSSize newSize = [[self cell] cellSizeForBounds:presizeFrame];
  NSRect newFrame = NSMakeRect(oldFrame.origin.x, oldFrame.origin.y,
                               NSWidth(oldFrame), newSize.height);
  [self setFrame:newFrame];

  return newSize.height - NSHeight(oldFrame);
}

- (CGFloat)breakpad_adjustWidthToFit {
  NSRect oldFrame = [self frame];
  [self sizeToFit];
  return NSWidth([self frame]) - NSWidth(oldFrame);
}
@end

@interface NSButton (ResizabilityExtentions)
// Resizes to fit the label using IB-style size-to-fit metrics and enforcing a
// minimum width of 70, while preserving the right edge location.
// Returns the change in width.
- (CGFloat)breakpad_smartSizeToFit;
@end

@implementation NSButton (ResizabilityExtentions)
- (CGFloat)breakpad_smartSizeToFit {
  NSRect oldFrame = [self frame];
  [self sizeToFit];
  NSRect newFrame = [self frame];
  // sizeToFit gives much worse results that IB's Size to Fit option. This is
  // the amount of padding IB adds over a sizeToFit, empirically determined.
  const float kExtraPaddingAmount = 12;
  const float kMinButtonWidth = 70; // The default button size in IB.
  newFrame.size.width = NSWidth(newFrame) + kExtraPaddingAmount;
  if (NSWidth(newFrame) < kMinButtonWidth)
    newFrame.size.width = kMinButtonWidth;
  // Preserve the right edge location.
  newFrame.origin.x = NSMaxX(oldFrame) - NSWidth(newFrame);
  [self setFrame:newFrame];
  return NSWidth(newFrame) - NSWidth(oldFrame);
}
@end

#pragma mark -

@interface Reporter(PrivateMethods)
- (id)initWithConfigFile:(const char *)configFile;

// Returns YES if it has been long enough since the last report that we should
// submit a report for this crash.
- (BOOL)reportIntervalElapsed;

// Returns YES if we should send the report without asking the user first.
- (BOOL)shouldSubmitSilently;

// Returns YES if the minidump was generated on demand.
- (BOOL)isOnDemand;

// Returns YES if we should ask the user to provide comments.
- (BOOL)shouldRequestComments;

// Returns YES if we should ask the user to provide an email address.
- (BOOL)shouldRequestEmail;

// Shows UI to the user to ask for permission to send and any extra information
// we've been instructed to request. Returns YES if the user allows the report
// to be sent.
- (BOOL)askUserPermissionToSend;

// Returns the short description of the crash, suitable for use as a dialog
// title (e.g., "The application Foo has quit unexpectedly").
- (NSString*)shortDialogMessage;

// Return explanatory text about the crash and the reporter, suitable for the
// body text of a dialog.
- (NSString*)explanatoryDialogText;

// Returns the amount of time the UI should be shown before timing out.
- (NSTimeInterval)messageTimeout;

// Preps the comment-prompting alert window for display:
// * localizes all the elements
// * resizes and adjusts layout as necessary for localization
// * removes the email section if includeEmail is NO
- (void)configureAlertWindowIncludingEmail:(BOOL)includeEmail;

// Rmevoes the email section of the dialog, adjusting the rest of the window
// as necessary.
- (void)removeEmailPrompt;

// Run an alert window with the given timeout. Returns
// NSRunStoppedResponse if the timeout is exceeded. A timeout of 0
// queues the message immediately in the modal run loop.
- (NSInteger)runModalWindow:(NSWindow*)window 
                withTimeout:(NSTimeInterval)timeout;

// This method is used to periodically update the UI with how many
// seconds are left in the dialog display.
- (void)updateSecondsLeftInDialogDisplay:(NSTimer*)theTimer;

// When we receive this notification, it means that the user has
// begun editing the email address or comments field, and we disable
// the timers so that the user has as long as they want to type
// in their comments/email.
- (void)controlTextDidBeginEditing:(NSNotification *)aNotification;

- (void)report;

@end

@implementation Reporter
//=============================================================================
- (id)initWithConfigFile:(const char *)configFile {
  if ((self = [super init])) {
    remainingDialogTime_ = 0;
    uploader_ = [[Uploader alloc] initWithConfigFile:configFile];
    if (!uploader_) {
      [self release];
      return nil;
    }
  }
  return self;
}

//=============================================================================
- (BOOL)askUserPermissionToSend {
  // Initialize Cocoa, needed to display the alert
  NSApplicationLoad();

  // Get the timeout value for the notification.
  NSTimeInterval timeout = [self messageTimeout];

  NSInteger buttonPressed = NSAlertAlternateReturn;
  // Determine whether we should create a text box for user feedback.
  if ([self shouldRequestComments]) {
    BOOL didLoadNib = [NSBundle loadNibNamed:@"Breakpad" owner:self];
    if (!didLoadNib) {
      return NO;
    }

    [self configureAlertWindowIncludingEmail:[self shouldRequestEmail]];

    buttonPressed = [self runModalWindow:alertWindow_ withTimeout:timeout];

    // Extract info from the user into the uploader_.
    if ([self commentsValue]) {
      [[uploader_ parameters] setObject:[self commentsValue]
                                 forKey:@BREAKPAD_COMMENTS];
    }
    if ([self emailValue]) {
      [[uploader_ parameters] setObject:[self emailValue]
                                 forKey:@BREAKPAD_EMAIL];
    }
  } else {
    // Create an alert panel to tell the user something happened
    NSPanel* alert = NSGetAlertPanel([self shortDialogMessage],
                                     [self explanatoryDialogText],
                                     NSLocalizedString(@"sendReportButton", @""),
                                     NSLocalizedString(@"cancelButton", @""),
                                     nil);

    // Pop the alert with an automatic timeout, and wait for the response
    buttonPressed = [self runModalWindow:alert withTimeout:timeout];

    // Release the panel memory
    NSReleaseAlertPanel(alert);
  }
  return buttonPressed == NSAlertDefaultReturn;
}

- (void)configureAlertWindowIncludingEmail:(BOOL)includeEmail {
  // Swap in localized values, making size adjustments to impacted elements as
  // we go. Remember that the origin is in the bottom left, so elements above
  // "fall" as text areas are shrunk from their overly-large IB sizes.

  // Localize the header. No resizing needed, as it has plenty of room.
  [dialogTitle_ setStringValue:[self shortDialogMessage]];

  // Localize the explanatory text field.
  [commentMessage_ setStringValue:[NSString stringWithFormat:@"%@\n\n%@",
                                   [self explanatoryDialogText],
                                   NSLocalizedString(@"commentsMsg", @"")]];
  CGFloat commentHeightDelta = [commentMessage_ breakpad_adjustHeightToFit];
  [headerBox_ breakpad_shiftVertically:commentHeightDelta];
  [alertWindow_ breakpad_adjustHeight:commentHeightDelta];

  // Either localize the email explanation field or remove the whole email
  // section depending on whether or not we are asking for email.
  if (includeEmail) {
    [emailMessage_ setStringValue:NSLocalizedString(@"emailMsg", @"")];
    CGFloat emailHeightDelta = [emailMessage_ breakpad_adjustHeightToFit];
    [preEmailBox_ breakpad_shiftVertically:emailHeightDelta];
    [alertWindow_ breakpad_adjustHeight:emailHeightDelta];
  } else {
    [self removeEmailPrompt];  // Handles necessary resizing.
  }

  // Localize the email label, and shift the associated text field.
  [emailLabel_ setStringValue:NSLocalizedString(@"emailLabel", @"")];
  CGFloat emailLabelWidthDelta = [emailLabel_ breakpad_adjustWidthToFit];
  [emailEntryField_ breakpad_shiftHorizontally:emailLabelWidthDelta];

  // Localize the privacy policy label, and keep it right-aligned to the arrow.
  [privacyLinkLabel_ setStringValue:NSLocalizedString(@"privacyLabel", @"")];
  CGFloat privacyLabelWidthDelta =
      [privacyLinkLabel_ breakpad_adjustWidthToFit];
  [privacyLinkLabel_ breakpad_shiftHorizontally:(-privacyLabelWidthDelta)];

  // Ensure that the email field and the privacy policy link don't overlap.
  CGFloat kMinControlPadding = 8;
  CGFloat maxEmailFieldWidth = NSMinX([privacyLinkLabel_ frame]) -
                               NSMinX([emailEntryField_ frame]) -
                               kMinControlPadding;
  if (NSWidth([emailEntryField_ bounds]) > maxEmailFieldWidth &&
      maxEmailFieldWidth > 0) {
    NSSize emailSize = [emailEntryField_ frame].size;
    emailSize.width = maxEmailFieldWidth;
    [emailEntryField_ setFrameSize:emailSize];
  }

  // Localize the placeholder text.
  [[commentsEntryField_ cell]
      setPlaceholderString:NSLocalizedString(@"commentsPlaceholder", @"")];
  [[emailEntryField_ cell]
      setPlaceholderString:NSLocalizedString(@"emailPlaceholder", @"")];

  // Localize the buttons, and keep the cancel button at the right distance.
  [sendButton_ setTitle:NSLocalizedString(@"sendReportButton", @"")];
  CGFloat sendButtonWidthDelta = [sendButton_ breakpad_smartSizeToFit];
  [cancelButton_ breakpad_shiftHorizontally:(-sendButtonWidthDelta)];
  [cancelButton_ setTitle:NSLocalizedString(@"cancelButton", @"")];
  [cancelButton_ breakpad_smartSizeToFit];
}

- (void)removeEmailPrompt {
  [emailSectionBox_ setHidden:YES];
  CGFloat emailSectionHeight = NSHeight([emailSectionBox_ frame]);
  [preEmailBox_ breakpad_shiftVertically:(-emailSectionHeight)];
  [alertWindow_ breakpad_adjustHeight:(-emailSectionHeight)];
}

- (NSInteger)runModalWindow:(NSWindow*)window 
                withTimeout:(NSTimeInterval)timeout {
  // Queue a |stopModal| message to be performed in |timeout| seconds.
  if (timeout > 0.001) {
    remainingDialogTime_ = timeout;
    SEL updateSelector = @selector(updateSecondsLeftInDialogDisplay:);
    messageTimer_ = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                     target:self
                                                   selector:updateSelector
                                                   userInfo:nil
                                                    repeats:YES];
  }

  // Run the window modally and wait for either a |stopModal| message or a
  // button click.
  [NSApp activateIgnoringOtherApps:YES];
  NSInteger returnMethod = [NSApp runModalForWindow:window];

  return returnMethod;
}

- (IBAction)sendReport:(id)sender {
  // Force the text fields to end editing so text for the currently focused
  // field will be commited.
  [alertWindow_ makeFirstResponder:alertWindow_];

  [alertWindow_ orderOut:self];
  // Use NSAlertDefaultReturn so that the return value of |runModalWithWindow|
  // matches the AppKit function NSRunAlertPanel()
  [NSApp stopModalWithCode:NSAlertDefaultReturn];
}

// UI Button Actions
//=============================================================================
- (IBAction)cancel:(id)sender {
  [alertWindow_ orderOut:self];
  // Use NSAlertDefaultReturn so that the return value of |runModalWithWindow|
  // matches the AppKit function NSRunAlertPanel()
  [NSApp stopModalWithCode:NSAlertAlternateReturn];
}

- (IBAction)showPrivacyPolicy:(id)sender {
  // Get the localized privacy policy URL and open it in the default browser.
  NSURL* privacyPolicyURL =
      [NSURL URLWithString:NSLocalizedString(@"privacyPolicyURL", @"")];
  [[NSWorkspace sharedWorkspace] openURL:privacyPolicyURL];
}

// Text Field Delegate Methods
//=============================================================================
- (BOOL)    control:(NSControl*)control
           textView:(NSTextView*)textView
doCommandBySelector:(SEL)commandSelector {
  BOOL result = NO;
  // If the user has entered text on the comment field, don't end
  // editing on "return".
  if (control == commentsEntryField_ &&
      commandSelector == @selector(insertNewline:)
      && [[textView string] length] > 0) {
    [textView insertNewlineIgnoringFieldEditor:self];
    result = YES;
  }
  return result;
}

- (void)controlTextDidBeginEditing:(NSNotification *)aNotification {
  [messageTimer_ invalidate];
  [self setCountdownMessage:@""];
}

- (void)updateSecondsLeftInDialogDisplay:(NSTimer*)theTimer {
  remainingDialogTime_ -= 1;

  NSString *countdownMessage;
  NSString *formatString;

  int displayedTimeLeft; // This can be either minutes or seconds.
  
  if (remainingDialogTime_ > 59) {
    // calculate minutes remaining for UI purposes
    displayedTimeLeft = (int)(remainingDialogTime_ / 60);
    
    if (displayedTimeLeft == 1) {
      formatString = NSLocalizedString(@"countdownMsgMinuteSingular", @"");
    } else {
      formatString = NSLocalizedString(@"countdownMsgMinutesPlural", @"");
    }
  } else {
    displayedTimeLeft = (int)remainingDialogTime_;
    if (displayedTimeLeft == 1) {
      formatString = NSLocalizedString(@"countdownMsgSecondSingular", @"");
    } else {
      formatString = NSLocalizedString(@"countdownMsgSecondsPlural", @"");
    }
  }
  countdownMessage = [NSString stringWithFormat:formatString,
                               displayedTimeLeft];
  if (remainingDialogTime_ <= 30) {
    [countdownLabel_ setTextColor:[NSColor redColor]];
  }
  [self setCountdownMessage:countdownMessage];
  if (remainingDialogTime_ <= 0) {
    [messageTimer_ invalidate];
    [NSApp stopModal];
  }
}



#pragma mark Accessors
#pragma mark -
//=============================================================================

- (NSString *)commentsValue {
  return [[commentsValue_ retain] autorelease];
}

- (void)setCommentsValue:(NSString *)value {
  if (commentsValue_ != value) {
    [commentsValue_ release];
    commentsValue_ = [value copy];
  }
}

- (NSString *)emailValue {
  return [[emailValue_ retain] autorelease];
}

- (void)setEmailValue:(NSString *)value {
  if (emailValue_ != value) {
    [emailValue_ release];
    emailValue_ = [value copy];
  }
}

- (NSString *)countdownMessage {
  return [[countdownMessage_ retain] autorelease];
}

- (void)setCountdownMessage:(NSString *)value {
  if (countdownMessage_ != value) {
    [countdownMessage_ release];
    countdownMessage_ = [value copy];
  }
}

#pragma mark -
//=============================================================================
- (BOOL)reportIntervalElapsed {
  float interval = [[[uploader_ parameters]
      objectForKey:@BREAKPAD_REPORT_INTERVAL] floatValue];
  NSString *program = [[uploader_ parameters] objectForKey:@BREAKPAD_PRODUCT];
  NSUserDefaults *ud = [NSUserDefaults standardUserDefaults];
  NSMutableDictionary *programDict =
    [NSMutableDictionary dictionaryWithDictionary:[ud dictionaryForKey:program]];
  NSNumber *lastTimeNum = [programDict objectForKey:kLastSubmission];
  NSTimeInterval lastTime = lastTimeNum ? [lastTimeNum floatValue] : 0;
  NSTimeInterval now = CFAbsoluteTimeGetCurrent();
  NSTimeInterval spanSeconds = (now - lastTime);

  [programDict setObject:[NSNumber numberWithDouble:now] 
                  forKey:kLastSubmission];
  [ud setObject:programDict forKey:program];
  [ud synchronize];

  // If we've specified an interval and we're within that time, don't ask the
  // user if we should report
  GTMLoggerDebug(@"Reporter Interval: %f", interval);
  if (interval > spanSeconds) {
    GTMLoggerDebug(@"Within throttling interval, not sending report");
    return NO;
  }
  return YES;
}

- (BOOL)isOnDemand {
  return [[[uploader_ parameters] objectForKey:@BREAKPAD_ON_DEMAND]
	   isEqualToString:@"YES"];
}

- (BOOL)shouldSubmitSilently {
  return [[[uploader_ parameters] objectForKey:@BREAKPAD_SKIP_CONFIRM]
            isEqualToString:@"YES"];
}

- (BOOL)shouldRequestComments {
  return [[[uploader_ parameters] objectForKey:@BREAKPAD_REQUEST_COMMENTS]
            isEqualToString:@"YES"];
}

- (BOOL)shouldRequestEmail {
  return [[[uploader_ parameters] objectForKey:@BREAKPAD_REQUEST_EMAIL]
            isEqualToString:@"YES"];
}

- (NSString*)shortDialogMessage {
  NSString *displayName =
      [[uploader_ parameters] objectForKey:@BREAKPAD_PRODUCT_DISPLAY];
  if (![displayName length])
    displayName = [[uploader_ parameters] objectForKey:@BREAKPAD_PRODUCT];

  if ([self isOnDemand]) {
    return [NSString
             stringWithFormat:NSLocalizedString(@"noCrashDialogHeader", @""),
             displayName];
  } else {
    return [NSString 
             stringWithFormat:NSLocalizedString(@"crashDialogHeader", @""),
             displayName];
  }
}

- (NSString*)explanatoryDialogText {
  NSString *displayName =
      [[uploader_ parameters] objectForKey:@BREAKPAD_PRODUCT_DISPLAY];
  if (![displayName length])
    displayName = [[uploader_ parameters] objectForKey:@BREAKPAD_PRODUCT];

  NSString *vendor = [[uploader_ parameters] objectForKey:@BREAKPAD_VENDOR];
  if (![vendor length])
    vendor = @"unknown vendor";

  if ([self isOnDemand]) {
    return [NSString
             stringWithFormat:NSLocalizedString(@"noCrashDialogMsg", @""),
             vendor, displayName];
  } else {
    return [NSString
             stringWithFormat:NSLocalizedString(@"crashDialogMsg", @""),
             vendor];
  }
}

- (NSTimeInterval)messageTimeout {
  // Get the timeout value for the notification.
  NSTimeInterval timeout = [[[uploader_ parameters]
      objectForKey:@BREAKPAD_CONFIRM_TIMEOUT] floatValue];
  // Require a timeout of at least a minute (except 0, which means no timeout).
  if (timeout > 0.001 && timeout < 60.0) {
    timeout = 60.0;
  }
  return timeout;
}

- (void)report {
  [uploader_ report];
}

//=============================================================================
- (void)dealloc {
  [uploader_ release];
  [super dealloc];
}

- (void)awakeFromNib {
  [emailEntryField_ setMaximumLength:kEmailMaxLength];
  [commentsEntryField_ setMaximumLength:kUserCommentsMaxLength];
}

@end

//=============================================================================
@implementation LengthLimitingTextField

- (void)setMaximumLength:(NSUInteger)maxLength {
  maximumLength_ = maxLength;
}

// This is the method we're overriding in NSTextField, which lets us
// limit the user's input if it makes the string too long.
- (BOOL)       textView:(NSTextView *)textView
shouldChangeTextInRange:(NSRange)affectedCharRange
      replacementString:(NSString *)replacementString {

  // Sometimes the range comes in invalid, so reject if we can't
  // figure out if the replacement text is too long.
  if (affectedCharRange.location == NSNotFound) {
    return NO;
  }
  // Figure out what the new string length would be, taking into
  // account user selections.
  NSUInteger newStringLength =
    [[textView string] length] - affectedCharRange.length +
    [replacementString length];
  if (newStringLength > maximumLength_) {
    return NO;
  } else {
    return YES;
  }
}

// Cut, copy, and paste have to be caught specifically since there is no menu.
- (BOOL)performKeyEquivalent:(NSEvent*)event {
  // Only handle the key equivalent if |self| is the text field with focus.
  NSText* fieldEditor = [self currentEditor];
  if (fieldEditor != nil) {
    // Check for a single "Command" modifier
    NSUInteger modifiers = [event modifierFlags];
    modifiers &= NSDeviceIndependentModifierFlagsMask;
    if (modifiers == NSCommandKeyMask) {
      // Now, check for Select All, Cut, Copy, or Paste key equivalents.
      NSString* characters = [event characters];
      // Select All is Command-A.
      if ([characters isEqualToString:@"a"]) {
        [fieldEditor selectAll:self];
        return YES;
      // Cut is Command-X.
      } else if ([characters isEqualToString:@"x"]) {
        [fieldEditor cut:self];
        return YES;
      // Copy is Command-C.
      } else if ([characters isEqualToString:@"c"]) {
        [fieldEditor copy:self];
        return YES;
      // Paste is Command-V.
      } else if ([characters isEqualToString:@"v"]) {
        [fieldEditor paste:self];
        return YES;
      }
    }
  }
  // Let the super class handle the rest (e.g. Command-Period will cancel).
  return [super performKeyEquivalent:event];
}

@end

//=============================================================================
int main(int argc, const char *argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#if DEBUG
  // Log to stderr in debug builds.
  [GTMLogger setSharedLogger:[GTMLogger standardLoggerWithStderr]];
#endif
  GTMLoggerDebug(@"Reporter Launched, argc=%d", argc);
  // The expectation is that there will be one argument which is the path
  // to the configuration file
  if (argc != 2) {
    exit(1);
  }

  Reporter *reporter = [[Reporter alloc] initWithConfigFile:argv[1]];
  if (!reporter) {
    GTMLoggerDebug(@"reporter initialization failed");
    exit(1);
  }

  // only submit a report if we have not recently crashed in the past
  BOOL shouldSubmitReport = [reporter reportIntervalElapsed];
  BOOL okayToSend = NO;

  // ask user if we should send
  if (shouldSubmitReport) {
    if ([reporter shouldSubmitSilently]) {
      GTMLoggerDebug(@"Skipping confirmation and sending report");
      okayToSend = YES;
    } else {
      okayToSend = [reporter askUserPermissionToSend];
    }
  }

  // If we're running as root, switch over to nobody
  if (getuid() == 0 || geteuid() == 0) {
    struct passwd *pw = getpwnam("nobody");

    // If we can't get a non-root uid, don't send the report
    if (!pw) {
      GTMLoggerDebug(@"!pw - %s", strerror(errno));
      exit(0);
    }

    if (setgid(pw->pw_gid) == -1) {
      GTMLoggerDebug(@"setgid(pw->pw_gid) == -1 - %s", strerror(errno));
      exit(0);
    }

    if (setuid(pw->pw_uid) == -1) {
      GTMLoggerDebug(@"setuid(pw->pw_uid) == -1 - %s", strerror(errno));
      exit(0);
    }
  }
  else {
     GTMLoggerDebug(@"getuid() !=0 || geteuid() != 0");
  }

  if (okayToSend && shouldSubmitReport) {
    GTMLoggerDebug(@"Sending Report");
    [reporter report];
    GTMLoggerDebug(@"Report Sent!");
  } else {
    GTMLoggerDebug(@"Not sending crash report okayToSend=%d, "\
                     "shouldSubmitReport=%d", okayToSend, shouldSubmitReport);
  }

  GTMLoggerDebug(@"Exiting with no errors");
  // Cleanup
  [reporter release];
  [pool release];
  return 0;
}
