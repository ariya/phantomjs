# Copyright (C) 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc.  All rights reserved.
# Copyright (C) 2009, 2010 Chris Jerdonek (chris.jerdonek@gmail.com)
# Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
# Copyright (C) 2012 Daniel Bates (dbates@intudata.com)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Module to share code to work with various version control systems.
package VCSUtils;

use strict;
use warnings;

use Cwd qw();  # "qw()" prevents warnings about redefining getcwd() with "use POSIX;"
use English; # for $POSTMATCH, etc.
use File::Basename;
use File::Spec;
use POSIX;
use Term::ANSIColor qw(colored);

BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(
        &applyGitBinaryPatchDelta
        &callSilently
        &canonicalizePath
        &changeLogEmailAddress
        &changeLogFileName
        &changeLogName
        &chdirReturningRelativePath
        &decodeGitBinaryChunk
        &decodeGitBinaryPatch
        &determineSVNRoot
        &determineVCSRoot
        &escapeSubversionPath
        &exitStatus
        &fixChangeLogPatch
        &gitBranch
        &gitdiff2svndiff
        &isGit
        &isGitSVN
        &isGitBranchBuild
        &isGitDirectory
        &isSVN
        &isSVNDirectory
        &isSVNVersion16OrNewer
        &makeFilePathRelative
        &mergeChangeLogs
        &normalizePath
        &parseChunkRange
        &parseFirstEOL
        &parsePatch
        &pathRelativeToSVNRepositoryRootForPath
        &possiblyColored
        &prepareParsedPatch
        &removeEOL
        &runCommand
        &runPatchCommand
        &scmMoveOrRenameFile
        &scmToggleExecutableBit
        &setChangeLogDateAndReviewer
        &svnRevisionForDirectory
        &svnStatus
        &toWindowsLineEndings
        &gitCommitForSVNRevision
        &listOfChangedFilesBetweenRevisions
    );
    %EXPORT_TAGS = ( );
    @EXPORT_OK   = ();
}

our @EXPORT_OK;

my $gitBranch;
my $gitRoot;
my $isGit;
my $isGitSVN;
my $isGitBranchBuild;
my $isSVN;
my $svnVersion;

# Project time zone for Cupertino, CA, US
my $changeLogTimeZone = "PST8PDT";

my $gitDiffStartRegEx = qr#^diff --git [^\r\n]+#;
my $gitDiffStartWithPrefixRegEx = qr#^diff --git \w/(.+) \w/([^\r\n]+)#; # We suppose that --src-prefix and --dst-prefix don't contain a non-word character (\W) and end with '/'.
my $gitDiffStartWithoutPrefixNoSpaceRegEx = qr#^diff --git (\S+) (\S+)$#;
my $svnDiffStartRegEx = qr#^Index: ([^\r\n]+)#;
my $gitDiffStartWithoutPrefixSourceDirectoryPrefixRegExp = qr#^diff --git ([^/]+/)#;
my $svnPropertiesStartRegEx = qr#^Property changes on: ([^\r\n]+)#; # $1 is normally the same as the index path.
my $svnPropertyStartRegEx = qr#^(Modified|Name|Added|Deleted): ([^\r\n]+)#; # $2 is the name of the property.
my $svnPropertyValueStartRegEx = qr#^\s*(\+|-|Merged|Reverse-merged)\s*([^\r\n]+)#; # $2 is the start of the property's value (which may span multiple lines).
my $svnPropertyValueNoNewlineRegEx = qr#\ No newline at end of property#;

# This method is for portability. Return the system-appropriate exit
# status of a child process.
#
# Args: pass the child error status returned by the last pipe close,
#       for example "$?".
sub exitStatus($)
{
    my ($returnvalue) = @_;
    if ($^O eq "MSWin32") {
        return $returnvalue >> 8;
    }
    if (!WIFEXITED($returnvalue)) {
        return 254;
    }
    return WEXITSTATUS($returnvalue);
}

# Call a function while suppressing STDERR, and return the return values
# as an array.
sub callSilently($@) {
    my ($func, @args) = @_;

    # The following pattern was taken from here:
    #   http://www.sdsc.edu/~moreland/courses/IntroPerl/docs/manual/pod/perlfunc/open.html
    #
    # Also see this Perl documentation (search for "open OLDERR"):
    #   http://perldoc.perl.org/functions/open.html
    open(OLDERR, ">&STDERR");
    close(STDERR);
    my @returnValue = &$func(@args);
    open(STDERR, ">&OLDERR");
    close(OLDERR);

    return @returnValue;
}

sub toWindowsLineEndings
{
    my ($text) = @_;
    $text =~ s/\n/\r\n/g;
    return $text;
}

# Note, this method will not error if the file corresponding to the $source path does not exist.
sub scmMoveOrRenameFile
{
    my ($source, $destination) = @_;
    return if ! -e $source;
    if (isSVN()) {
        my $escapedDestination = escapeSubversionPath($destination);
        my $escapedSource = escapeSubversionPath($source);
        system("svn", "move", $escapedSource, $escapedDestination);
    } elsif (isGit()) {
        system("git", "mv", $source, $destination);
    }
}

# Note, this method will not error if the file corresponding to the path does not exist.
sub scmToggleExecutableBit
{
    my ($path, $executableBitDelta) = @_;
    return if ! -e $path;
    if ($executableBitDelta == 1) {
        scmAddExecutableBit($path);
    } elsif ($executableBitDelta == -1) {
        scmRemoveExecutableBit($path);
    }
}

sub scmAddExecutableBit($)
{
    my ($path) = @_;

    if (isSVN()) {
        my $escapedPath = escapeSubversionPath($path);
        system("svn", "propset", "svn:executable", "on", $escapedPath) == 0 or die "Failed to run 'svn propset svn:executable on $escapedPath'.";
    } elsif (isGit()) {
        chmod(0755, $path);
    }
}

sub scmRemoveExecutableBit($)
{
    my ($path) = @_;

    if (isSVN()) {
        my $escapedPath = escapeSubversionPath($path);
        system("svn", "propdel", "svn:executable", $escapedPath) == 0 or die "Failed to run 'svn propdel svn:executable $escapedPath'.";
    } elsif (isGit()) {
        chmod(0664, $path);
    }
}

sub isGitDirectory($)
{
    my ($dir) = @_;
    return system("cd $dir && git rev-parse > " . File::Spec->devnull() . " 2>&1") == 0;
}

sub isGit()
{
    return $isGit if defined $isGit;

    $isGit = isGitDirectory(".");
    return $isGit;
}

sub isGitSVN()
{
    return $isGitSVN if defined $isGitSVN;

    # There doesn't seem to be an officially documented way to determine
    # if you're in a git-svn checkout. The best suggestions seen so far
    # all use something like the following:
    my $output = `git config --get svn-remote.svn.fetch 2>& 1`;
    $isGitSVN = $output ne '';
    return $isGitSVN;
}

sub gitDirectory()
{
    chomp(my $result = `git rev-parse --git-dir`);
    return $result;
}

sub gitBisectStartBranch()
{
    my $bisectStartFile = File::Spec->catfile(gitDirectory(), "BISECT_START");
    if (!-f $bisectStartFile) {
        return "";
    }
    open(BISECT_START, $bisectStartFile) or die "Failed to open $bisectStartFile: $!";
    chomp(my $result = <BISECT_START>);
    close(BISECT_START);
    return $result;
}

sub gitBranch()
{
    unless (defined $gitBranch) {
        chomp($gitBranch = `git symbolic-ref -q HEAD`);
        my $hasDetachedHead = exitStatus($?);
        if ($hasDetachedHead) {
            # We may be in a git bisect session.
            $gitBranch = gitBisectStartBranch();
        }
        $gitBranch =~ s#^refs/heads/##;
        $gitBranch = "" if $gitBranch eq "master";
    }

    return $gitBranch;
}

sub isGitBranchBuild()
{
    my $branch = gitBranch();
    chomp(my $override = `git config --bool branch.$branch.webKitBranchBuild`);
    return 1 if $override eq "true";
    return 0 if $override eq "false";

    unless (defined $isGitBranchBuild) {
        chomp(my $gitBranchBuild = `git config --bool core.webKitBranchBuild`);
        $isGitBranchBuild = $gitBranchBuild eq "true";
    }

    return $isGitBranchBuild;
}

sub isSVNDirectory($)
{
    my ($dir) = @_;
    return system("cd $dir && svn info > " . File::Spec->devnull() . " 2>&1") == 0;
}

sub isSVN()
{
    return $isSVN if defined $isSVN;

    $isSVN = isSVNDirectory(".");
    return $isSVN;
}

sub svnVersion()
{
    return $svnVersion if defined $svnVersion;

    if (!isSVN()) {
        $svnVersion = 0;
    } else {
        chomp($svnVersion = `svn --version --quiet`);
    }
    return $svnVersion;
}

sub isSVNVersion16OrNewer()
{
    my $version = svnVersion();
    return eval "v$version" ge v1.6;
}

sub chdirReturningRelativePath($)
{
    my ($directory) = @_;
    my $previousDirectory = Cwd::getcwd();
    chdir $directory;
    my $newDirectory = Cwd::getcwd();
    return "." if $newDirectory eq $previousDirectory;
    return File::Spec->abs2rel($previousDirectory, $newDirectory);
}

sub determineSVNRoot()
{
    my $last = '';
    my $path = '.';
    my $parent = '..';
    my $repositoryRoot;
    my $repositoryUUID;
    while (1) {
        my $thisRoot;
        my $thisUUID;
        my $escapedPath = escapeSubversionPath($path);
        # Ignore error messages in case we've run past the root of the checkout.
        open INFO, "svn info '$escapedPath' 2> " . File::Spec->devnull() . " |" or die;
        while (<INFO>) {
            if (/^Repository Root: (.+)/) {
                $thisRoot = $1;
            }
            if (/^Repository UUID: (.+)/) {
                $thisUUID = $1;
            }
            if ($thisRoot && $thisUUID) {
                local $/ = undef;
                <INFO>; # Consume the rest of the input.
            }
        }
        close INFO;

        # It's possible (e.g. for developers of some ports) to have a WebKit
        # checkout in a subdirectory of another checkout.  So abort if the
        # repository root or the repository UUID suddenly changes.
        last if !$thisUUID;
        $repositoryUUID = $thisUUID if !$repositoryUUID;
        last if $thisUUID ne $repositoryUUID;

        last if !$thisRoot;
        $repositoryRoot = $thisRoot if !$repositoryRoot;
        last if $thisRoot ne $repositoryRoot;

        $last = $path;
        $path = File::Spec->catdir($parent, $path);
    }

    return File::Spec->rel2abs($last);
}

sub determineVCSRoot()
{
    if (isGit()) {
        return dirname(gitDirectory());
    }

    if (!isSVN()) {
        # Some users have a workflow where svn-create-patch, svn-apply and
        # svn-unapply are used outside of multiple svn working directores,
        # so warn the user and assume Subversion is being used in this case.
        warn "Unable to determine VCS root for '" . Cwd::getcwd() . "'; assuming Subversion";
        $isSVN = 1;
    }

    return determineSVNRoot();
}

sub isWindows()
{
    return ($^O eq "MSWin32") || 0;
}

sub svnRevisionForDirectory($)
{
    my ($dir) = @_;
    my $revision;

    if (isSVNDirectory($dir)) {
        my $escapedDir = escapeSubversionPath($dir);
        my $command = "svn info $escapedDir | grep Revision:";
        $command = "LC_ALL=C $command" if !isWindows();
        my $svnInfo = `$command`;
        ($revision) = ($svnInfo =~ m/Revision: (\d+).*/g);
    } elsif (isGitDirectory($dir)) {
        my $command = "git log --grep=\"git-svn-id: \" -n 1 | grep git-svn-id:";
        $command = "LC_ALL=C $command" if !isWindows();
        $command = "cd $dir && $command";
        my $gitLog = `$command`;
        ($revision) = ($gitLog =~ m/ +git-svn-id: .+@(\d+) /g);
    }
    if (!defined($revision)) {
        $revision = "unknown";
        warn "Unable to determine current SVN revision in $dir";
    }
    return $revision;
}

sub pathRelativeToSVNRepositoryRootForPath($)
{
    my ($file) = @_;
    my $relativePath = File::Spec->abs2rel($file);

    my $svnInfo;
    if (isSVN()) {
        my $escapedRelativePath = escapeSubversionPath($relativePath);
        my $command = "svn info $escapedRelativePath";
        $command = "LC_ALL=C $command" if !isWindows();
        $svnInfo = `$command`;
    } elsif (isGit()) {
        my $command = "git svn info $relativePath";
        $command = "LC_ALL=C $command" if !isWindows();
        $svnInfo = `$command`;
    }

    $svnInfo =~ /.*^URL: (.*?)$/m;
    my $svnURL = $1;

    $svnInfo =~ /.*^Repository Root: (.*?)$/m;
    my $repositoryRoot = $1;

    $svnURL =~ s/$repositoryRoot\///;
    return $svnURL;
}

sub makeFilePathRelative($)
{
    my ($path) = @_;
    return $path unless isGit();

    unless (defined $gitRoot) {
        chomp($gitRoot = `git rev-parse --show-cdup`);
    }
    return $gitRoot . $path;
}

sub normalizePath($)
{
    my ($path) = @_;
    $path =~ s/\\/\//g;
    return $path;
}

sub possiblyColored($$)
{
    my ($colors, $string) = @_;

    if (-t STDOUT) {
        return colored([$colors], $string);
    } else {
        return $string;
    }
}

sub adjustPathForRecentRenamings($) 
{ 
    my ($fullPath) = @_; 
 
    $fullPath =~ s|WebCore/webaudio|WebCore/Modules/webaudio|g;
    $fullPath =~ s|JavaScriptCore/wtf|WTF/wtf|g;
    $fullPath =~ s|test_expectations.txt|TestExpectations|g;

    return $fullPath; 
} 

sub canonicalizePath($)
{
    my ($file) = @_;

    # Remove extra slashes and '.' directories in path
    $file = File::Spec->canonpath($file);

    # Remove '..' directories in path
    my @dirs = ();
    foreach my $dir (File::Spec->splitdir($file)) {
        if ($dir eq '..' && $#dirs >= 0 && $dirs[$#dirs] ne '..') {
            pop(@dirs);
        } else {
            push(@dirs, $dir);
        }
    }
    return ($#dirs >= 0) ? File::Spec->catdir(@dirs) : ".";
}

sub removeEOL($)
{
    my ($line) = @_;
    return "" unless $line;

    $line =~ s/[\r\n]+$//g;
    return $line;
}

sub parseFirstEOL($)
{
    my ($fileHandle) = @_;

    # Make input record separator the new-line character to simplify regex matching below.
    my $savedInputRecordSeparator = $INPUT_RECORD_SEPARATOR;
    $INPUT_RECORD_SEPARATOR = "\n";
    my $firstLine  = <$fileHandle>;
    $INPUT_RECORD_SEPARATOR = $savedInputRecordSeparator;

    return unless defined($firstLine);

    my $eol;
    if ($firstLine =~ /\r\n/) {
        $eol = "\r\n";
    } elsif ($firstLine =~ /\r/) {
        $eol = "\r";
    } elsif ($firstLine =~ /\n/) {
        $eol = "\n";
    }
    return $eol;
}

sub firstEOLInFile($)
{
    my ($file) = @_;
    my $eol;
    if (open(FILE, $file)) {
        $eol = parseFirstEOL(*FILE);
        close(FILE);
    }
    return $eol;
}

# Parses a chunk range line into its components.
#
# A chunk range line has the form: @@ -L_1,N_1 +L_2,N_2 @@, where the pairs (L_1, N_1),
# (L_2, N_2) are ranges that represent the starting line number and line count in the
# original file and new file, respectively.
#
# Note, some versions of GNU diff may omit the comma and trailing line count (e.g. N_1),
# in which case the omitted line count defaults to 1. For example, GNU diff may output
# @@ -1 +1 @@, which is equivalent to @@ -1,1 +1,1 @@.
#
# This subroutine returns undef if given an invalid or malformed chunk range.
#
# Args:
#   $line: the line to parse.
#   $chunkSentinel: the sentinel that surrounds the chunk range information (defaults to "@@").
#
# Returns $chunkRangeHashRef
#   $chunkRangeHashRef: a hash reference representing the parts of a chunk range, as follows--
#     startingLine: the starting line in the original file.
#     lineCount: the line count in the original file.
#     newStartingLine: the new starting line in the new file.
#     newLineCount: the new line count in the new file.
sub parseChunkRange($;$)
{
    my ($line, $chunkSentinel) = @_;
    $chunkSentinel = "@@" if !$chunkSentinel;
    my $chunkRangeRegEx = qr#^\Q$chunkSentinel\E -(\d+)(,(\d+))? \+(\d+)(,(\d+))? \Q$chunkSentinel\E#;
    if ($line !~ /$chunkRangeRegEx/) {
        return;
    }
    my %chunkRange;
    $chunkRange{startingLine} = $1;
    $chunkRange{lineCount} = defined($2) ? $3 : 1;
    $chunkRange{newStartingLine} = $4;
    $chunkRange{newLineCount} = defined($5) ? $6 : 1;
    return \%chunkRange;
}

sub svnStatus($)
{
    my ($fullPath) = @_;
    my $escapedFullPath = escapeSubversionPath($fullPath);
    my $svnStatus;
    open SVN, "svn status --non-interactive --non-recursive '$escapedFullPath' |" or die;
    if (-d $fullPath) {
        # When running "svn stat" on a directory, we can't assume that only one
        # status will be returned (since any files with a status below the
        # directory will be returned), and we can't assume that the directory will
        # be first (since any files with unknown status will be listed first).
        my $normalizedFullPath = File::Spec->catdir(File::Spec->splitdir($fullPath));
        while (<SVN>) {
            # Input may use a different EOL sequence than $/, so avoid chomp.
            $_ = removeEOL($_);
            my $normalizedStatPath = File::Spec->catdir(File::Spec->splitdir(substr($_, 7)));
            if ($normalizedFullPath eq $normalizedStatPath) {
                $svnStatus = "$_\n";
                last;
            }
        }
        # Read the rest of the svn command output to avoid a broken pipe warning.
        local $/ = undef;
        <SVN>;
    }
    else {
        # Files will have only one status returned.
        $svnStatus = removeEOL(<SVN>) . "\n";
    }
    close SVN;
    return $svnStatus;
}

# Return whether the given file mode is executable in the source control
# sense.  We make this determination based on whether the executable bit
# is set for "others" rather than the stronger condition that it be set
# for the user, group, and others.  This is sufficient for distinguishing
# the default behavior in Git and SVN.
#
# Args:
#   $fileMode: A number or string representing a file mode in octal notation.
sub isExecutable($)
{
    my $fileMode = shift;

    return $fileMode % 2;
}

# Parse the Git diff header start line.
#
# Args:
#   $line: "diff --git" line.
#
# Returns the path of the target file.
sub parseGitDiffStartLine($)
{
    my $line = shift;
    $_ = $line;
    if (/$gitDiffStartWithPrefixRegEx/ || /$gitDiffStartWithoutPrefixNoSpaceRegEx/) {
        return $2;
    }
    # Assume the diff was generated with --no-prefix (e.g. git diff --no-prefix).
    if (!/$gitDiffStartWithoutPrefixSourceDirectoryPrefixRegExp/) {
        # FIXME: Moving top directory file is not supported (e.g diff --git A.txt B.txt).
        die("Could not find '/' in \"diff --git\" line: \"$line\"; only non-prefixed git diffs (i.e. not generated with --no-prefix) that move a top-level directory file are supported.");
    }
    my $pathPrefix = $1;
    if (!/^diff --git \Q$pathPrefix\E.+ (\Q$pathPrefix\E.+)$/) {
        # FIXME: Moving a file through sub directories of top directory is not supported (e.g diff --git A/B.txt C/B.txt).
        die("Could not find '/' in \"diff --git\" line: \"$line\"; only non-prefixed git diffs (i.e. not generated with --no-prefix) that move a file between top-level directories are supported.");
    }
    return $1;
}

# Parse the next Git diff header from the given file handle, and advance
# the handle so the last line read is the first line after the header.
#
# This subroutine dies if given leading junk.
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the header to parse.  This should be a line
#                beginning with "diff --git".
#   $line: the line last read from $fileHandle
#
# Returns ($headerHashRef, $lastReadLine):
#   $headerHashRef: a hash reference representing a diff header, as follows--
#     copiedFromPath: the path from which the file was copied or moved if
#                     the diff is a copy or move.
#     executableBitDelta: the value 1 or -1 if the executable bit was added or
#                         removed, respectively.  New and deleted files have
#                         this value only if the file is executable, in which
#                         case the value is 1 and -1, respectively.
#     indexPath: the path of the target file.
#     isBinary: the value 1 if the diff is for a binary file.
#     isDeletion: the value 1 if the diff is a file deletion.
#     isCopyWithChanges: the value 1 if the file was copied or moved and
#                        the target file was changed in some way after being
#                        copied or moved (e.g. if its contents or executable
#                        bit were changed).
#     isNew: the value 1 if the diff is for a new file.
#     shouldDeleteSource: the value 1 if the file was copied or moved and
#                         the source file was deleted -- i.e. if the copy
#                         was actually a move.
#     svnConvertedText: the header text with some lines converted to SVN
#                       format.  Git-specific lines are preserved.
#   $lastReadLine: the line last read from $fileHandle.
sub parseGitDiffHeader($$)
{
    my ($fileHandle, $line) = @_;

    $_ = $line;

    my $indexPath;
    if (/$gitDiffStartRegEx/) {
        # Use $POSTMATCH to preserve the end-of-line character.
        my $eol = $POSTMATCH;

        # The first and second paths can differ in the case of copies
        # and renames.  We use the second file path because it is the
        # destination path.
        $indexPath = adjustPathForRecentRenamings(parseGitDiffStartLine($_));

        $_ = "Index: $indexPath$eol"; # Convert to SVN format.
    } else {
        die("Could not parse leading \"diff --git\" line: \"$line\".");
    }

    my $copiedFromPath;
    my $foundHeaderEnding;
    my $isBinary;
    my $isDeletion;
    my $isNew;
    my $newExecutableBit = 0;
    my $oldExecutableBit = 0;
    my $shouldDeleteSource = 0;
    my $similarityIndex = 0;
    my $svnConvertedText;
    while (1) {
        # Temporarily strip off any end-of-line characters to simplify
        # regex matching below.
        s/([\n\r]+)$//;
        my $eol = $1;

        if (/^(deleted file|old) mode (\d+)/) {
            $oldExecutableBit = (isExecutable($2) ? 1 : 0);
            $isDeletion = 1 if $1 eq "deleted file";
        } elsif (/^new( file)? mode (\d+)/) {
            $newExecutableBit = (isExecutable($2) ? 1 : 0);
            $isNew = 1 if $1;
        } elsif (/^similarity index (\d+)%/) {
            $similarityIndex = $1;
        } elsif (/^copy from ([^\t\r\n]+)/) {
            $copiedFromPath = $1;
        } elsif (/^rename from ([^\t\r\n]+)/) {
            # FIXME: Record this as a move rather than as a copy-and-delete.
            #        This will simplify adding rename support to svn-unapply.
            #        Otherwise, the hash for a deletion would have to know
            #        everything about the file being deleted in order to
            #        support undoing itself.  Recording as a move will also
            #        permit us to use "svn move" and "git move".
            $copiedFromPath = $1;
            $shouldDeleteSource = 1;
        } elsif (/^--- \S+/) {
            # Convert to SVN format.
            # We emit the suffix "\t(revision 0)" to handle $indexPath which contains a space character.
            # The patch(1) command thinks a file path is characters before a tab.
            # This suffix make our diff more closely match the SVN diff format.
            $_ = "--- $indexPath\t(revision 0)";
        } elsif (/^\+\+\+ \S+/) {
            # Convert to SVN format.
            # We emit the suffix "\t(working copy)" to handle $indexPath which contains a space character.
            # The patch(1) command thinks a file path is characters before a tab.
            # This suffix make our diff more closely match the SVN diff format.
            $_ = "+++ $indexPath\t(working copy)";
            $foundHeaderEnding = 1;
        } elsif (/^GIT binary patch$/ ) {
            $isBinary = 1;
            $foundHeaderEnding = 1;
        # The "git diff" command includes a line of the form "Binary files
        # <path1> and <path2> differ" if the --binary flag is not used.
        } elsif (/^Binary files / ) {
            die("Error: the Git diff contains a binary file without the binary data in ".
                "line: \"$_\".  Be sure to use the --binary flag when invoking \"git diff\" ".
                "with diffs containing binary files.");
        }

        $svnConvertedText .= "$_$eol"; # Also restore end-of-line characters.

        $_ = <$fileHandle>; # Not defined if end-of-file reached.

        last if (!defined($_) || /$gitDiffStartRegEx/ || $foundHeaderEnding);
    }

    my $executableBitDelta = $newExecutableBit - $oldExecutableBit;

    my %header;

    $header{copiedFromPath} = $copiedFromPath if $copiedFromPath;
    $header{executableBitDelta} = $executableBitDelta if $executableBitDelta;
    $header{indexPath} = $indexPath;
    $header{isBinary} = $isBinary if $isBinary;
    $header{isCopyWithChanges} = 1 if ($copiedFromPath && ($similarityIndex != 100 || $executableBitDelta));
    $header{isDeletion} = $isDeletion if $isDeletion;
    $header{isNew} = $isNew if $isNew;
    $header{shouldDeleteSource} = $shouldDeleteSource if $shouldDeleteSource;
    $header{svnConvertedText} = $svnConvertedText;

    return (\%header, $_);
}

# Parse the next SVN diff header from the given file handle, and advance
# the handle so the last line read is the first line after the header.
#
# This subroutine dies if given leading junk or if it could not detect
# the end of the header block.
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the header to parse.  This should be a line
#                beginning with "Index:".
#   $line: the line last read from $fileHandle
#
# Returns ($headerHashRef, $lastReadLine):
#   $headerHashRef: a hash reference representing a diff header, as follows--
#     copiedFromPath: the path from which the file was copied if the diff
#                     is a copy.
#     indexPath: the path of the target file, which is the path found in
#                the "Index:" line.
#     isBinary: the value 1 if the diff is for a binary file.
#     isNew: the value 1 if the diff is for a new file.
#     sourceRevision: the revision number of the source, if it exists.  This
#                     is the same as the revision number the file was copied
#                     from, in the case of a file copy.
#     svnConvertedText: the header text converted to a header with the paths
#                       in some lines corrected.
#   $lastReadLine: the line last read from $fileHandle.
sub parseSvnDiffHeader($$)
{
    my ($fileHandle, $line) = @_;

    $_ = $line;

    my $indexPath;
    if (/$svnDiffStartRegEx/) {
        $indexPath = adjustPathForRecentRenamings($1);
    } else {
        die("First line of SVN diff does not begin with \"Index \": \"$_\"");
    }

    my $copiedFromPath;
    my $foundHeaderEnding;
    my $isBinary;
    my $isNew;
    my $sourceRevision;
    my $svnConvertedText;
    while (1) {
        # Temporarily strip off any end-of-line characters to simplify
        # regex matching below.
        s/([\n\r]+)$//;
        my $eol = $1;

        # Fix paths on "---" and "+++" lines to match the leading
        # index line.
        if (s/^--- [^\t\n\r]+/--- $indexPath/) {
            # ---
            if (/^--- .+\(revision (\d+)\)/) {
                $sourceRevision = $1;
                $isNew = 1 if !$sourceRevision; # if revision 0.
                if (/\(from (\S+):(\d+)\)$/) {
                    # The "from" clause is created by svn-create-patch, in
                    # which case there is always also a "revision" clause.
                    $copiedFromPath = $1;
                    die("Revision number \"$2\" in \"from\" clause does not match " .
                        "source revision number \"$sourceRevision\".") if ($2 != $sourceRevision);
                }
            }
        } elsif (s/^\+\+\+ [^\t\n\r]+/+++ $indexPath/ || $isBinary && /^$/) {
            $foundHeaderEnding = 1;
        } elsif (/^Cannot display: file marked as a binary type.$/) {
            $isBinary = 1;
            # SVN 1.7 has an unusual display format for a binary diff. It repeats the first
            # two lines of the diff header. For example:
            #     Index: test_file.swf
            #     ===================================================================
            #     Cannot display: file marked as a binary type.
            #     svn:mime-type = application/octet-stream
            #     Index: test_file.swf
            #     ===================================================================
            #     --- test_file.swf
            #     +++ test_file.swf
            #
            #     ...
            #     Q1dTBx0AAAB42itg4GlgYJjGwMDDyODMxMDw34GBgQEAJPQDJA==
            # Therefore, we continue reading the diff header until we either encounter a line
            # that begins with "+++" (SVN 1.7 or greater) or an empty line (SVN version less
            # than 1.7).
        }

        $svnConvertedText .= "$_$eol"; # Also restore end-of-line characters.

        $_ = <$fileHandle>; # Not defined if end-of-file reached.

        last if (!defined($_) || !$isBinary && /$svnDiffStartRegEx/ || $foundHeaderEnding);
    }

    if (!$foundHeaderEnding) {
        die("Did not find end of header block corresponding to index path \"$indexPath\".");
    }

    my %header;

    $header{copiedFromPath} = $copiedFromPath if $copiedFromPath;
    $header{indexPath} = $indexPath;
    $header{isBinary} = $isBinary if $isBinary;
    $header{isNew} = $isNew if $isNew;
    $header{sourceRevision} = $sourceRevision if $sourceRevision;
    $header{svnConvertedText} = $svnConvertedText;

    return (\%header, $_);
}

# Parse the next diff header from the given file handle, and advance
# the handle so the last line read is the first line after the header.
#
# This subroutine dies if given leading junk or if it could not detect
# the end of the header block.
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the header to parse.  For SVN-formatted diffs, this
#                is a line beginning with "Index:".  For Git, this is a line
#                beginning with "diff --git".
#   $line: the line last read from $fileHandle
#
# Returns ($headerHashRef, $lastReadLine):
#   $headerHashRef: a hash reference representing a diff header
#     copiedFromPath: the path from which the file was copied if the diff
#                     is a copy.
#     executableBitDelta: the value 1 or -1 if the executable bit was added or
#                         removed, respectively.  New and deleted files have
#                         this value only if the file is executable, in which
#                         case the value is 1 and -1, respectively.
#     indexPath: the path of the target file.
#     isBinary: the value 1 if the diff is for a binary file.
#     isGit: the value 1 if the diff is Git-formatted.
#     isSvn: the value 1 if the diff is SVN-formatted.
#     sourceRevision: the revision number of the source, if it exists.  This
#                     is the same as the revision number the file was copied
#                     from, in the case of a file copy.
#     svnConvertedText: the header text with some lines converted to SVN
#                       format.  Git-specific lines are preserved.
#   $lastReadLine: the line last read from $fileHandle.
sub parseDiffHeader($$)
{
    my ($fileHandle, $line) = @_;

    my $header;  # This is a hash ref.
    my $isGit;
    my $isSvn;
    my $lastReadLine;

    if ($line =~ $svnDiffStartRegEx) {
        $isSvn = 1;
        ($header, $lastReadLine) = parseSvnDiffHeader($fileHandle, $line);
    } elsif ($line =~ $gitDiffStartRegEx) {
        $isGit = 1;
        ($header, $lastReadLine) = parseGitDiffHeader($fileHandle, $line);
    } else {
        die("First line of diff does not begin with \"Index:\" or \"diff --git\": \"$line\"");
    }

    $header->{isGit} = $isGit if $isGit;
    $header->{isSvn} = $isSvn if $isSvn;

    return ($header, $lastReadLine);
}

# FIXME: The %diffHash "object" should not have an svnConvertedText property.
#        Instead, the hash object should store its information in a
#        structured way as properties.  This should be done in a way so
#        that, if necessary, the text of an SVN or Git patch can be
#        reconstructed from the information in those hash properties.
#
# A %diffHash is a hash representing a source control diff of a single
# file operation (e.g. a file modification, copy, or delete).
#
# These hashes appear, for example, in the parseDiff(), parsePatch(),
# and prepareParsedPatch() subroutines of this package.
#
# The corresponding values are--
#
#   copiedFromPath: the path from which the file was copied if the diff
#                   is a copy.
#   executableBitDelta: the value 1 or -1 if the executable bit was added or
#                       removed from the target file, respectively.
#   indexPath: the path of the target file.  For SVN-formatted diffs,
#              this is the same as the path in the "Index:" line.
#   isBinary: the value 1 if the diff is for a binary file.
#   isDeletion: the value 1 if the diff is known from the header to be a deletion.
#   isGit: the value 1 if the diff is Git-formatted.
#   isNew: the value 1 if the dif is known from the header to be a new file.
#   isSvn: the value 1 if the diff is SVN-formatted.
#   sourceRevision: the revision number of the source, if it exists.  This
#                   is the same as the revision number the file was copied
#                   from, in the case of a file copy.
#   svnConvertedText: the diff with some lines converted to SVN format.
#                     Git-specific lines are preserved.

# Parse one diff from a patch file created by svn-create-patch, and
# advance the file handle so the last line read is the first line
# of the next header block.
#
# This subroutine preserves any leading junk encountered before the header.
#
# Composition of an SVN diff
#
# There are three parts to an SVN diff: the header, the property change, and
# the binary contents, in that order. Either the header or the property change
# may be ommitted, but not both. If there are binary changes, then you always
# have all three.
#
# Args:
#   $fileHandle: a file handle advanced to the first line of the next
#                header block. Leading junk is okay.
#   $line: the line last read from $fileHandle.
#   $optionsHashRef: a hash reference representing optional options to use
#                    when processing a diff.
#     shouldNotUseIndexPathEOL: whether to use the line endings in the diff instead
#                               instead of the line endings in the target file; the
#                               value of 1 if svnConvertedText should use the line
#                               endings in the diff.
#
# Returns ($diffHashRefs, $lastReadLine):
#   $diffHashRefs: A reference to an array of references to %diffHash hashes.
#                  See the %diffHash documentation above.
#   $lastReadLine: the line last read from $fileHandle
sub parseDiff($$;$)
{
    # FIXME: Adjust this method so that it dies if the first line does not
    #        match the start of a diff.  This will require a change to
    #        parsePatch() so that parsePatch() skips over leading junk.
    my ($fileHandle, $line, $optionsHashRef) = @_;

    my $headerStartRegEx = $svnDiffStartRegEx; # SVN-style header for the default

    my $headerHashRef; # Last header found, as returned by parseDiffHeader().
    my $svnPropertiesHashRef; # Last SVN properties diff found, as returned by parseSvnDiffProperties().
    my $svnText;
    my $indexPathEOL;
    my $numTextChunks = 0;
    while (defined($line)) {
        if (!$headerHashRef && ($line =~ $gitDiffStartRegEx)) {
            # Then assume all diffs in the patch are Git-formatted. This
            # block was made to be enterable at most once since we assume
            # all diffs in the patch are formatted the same (SVN or Git).
            $headerStartRegEx = $gitDiffStartRegEx;
        }

        if ($line =~ $svnPropertiesStartRegEx) {
            my $propertyPath = $1;
            if ($svnPropertiesHashRef || $headerHashRef && ($propertyPath ne $headerHashRef->{indexPath})) {
                # This is the start of the second diff in the while loop, which happens to
                # be a property diff.  If $svnPropertiesHasRef is defined, then this is the
                # second consecutive property diff, otherwise it's the start of a property
                # diff for a file that only has property changes.
                last;
            }
            ($svnPropertiesHashRef, $line) = parseSvnDiffProperties($fileHandle, $line);
            next;
        }
        if ($line !~ $headerStartRegEx) {
            # Then we are in the body of the diff.
            my $isChunkRange = defined(parseChunkRange($line));
            $numTextChunks += 1 if $isChunkRange;
            my $nextLine = <$fileHandle>;
            my $willAddNewLineAtEndOfFile = defined($nextLine) && $nextLine =~ /^\\ No newline at end of file$/;
            if ($willAddNewLineAtEndOfFile) {
                # Diff(1) always emits a LF character preceeding the line "\ No newline at end of file".
                # We must preserve both the added LF character and the line ending of this sentinel line
                # or patch(1) will complain.
                $svnText .= $line . $nextLine;
                $line = <$fileHandle>;
                next;
            }
            if ($indexPathEOL && !$isChunkRange) {
                # The chunk range is part of the body of the diff, but its line endings should't be
                # modified or patch(1) will complain. So, we only modify non-chunk range lines.
                $line =~ s/\r\n|\r|\n/$indexPathEOL/g;
            }
            $svnText .= $line;
            $line = $nextLine;
            next;
        } # Otherwise, we found a diff header.

        if ($svnPropertiesHashRef || $headerHashRef) {
            # Then either we just processed an SVN property change or this
            # is the start of the second diff header of this while loop.
            last;
        }

        ($headerHashRef, $line) = parseDiffHeader($fileHandle, $line);
        if (!$optionsHashRef || !$optionsHashRef->{shouldNotUseIndexPathEOL}) {
            # FIXME: We shouldn't query the file system (via firstEOLInFile()) to determine the
            #        line endings of the file indexPath. Instead, either the caller to parseDiff()
            #        should provide this information or parseDiff() should take a delegate that it
            #        can use to query for this information.
            $indexPathEOL = firstEOLInFile($headerHashRef->{indexPath}) if !$headerHashRef->{isNew} && !$headerHashRef->{isBinary};
        }

        $svnText .= $headerHashRef->{svnConvertedText};
    }

    my @diffHashRefs;

    if ($headerHashRef->{shouldDeleteSource}) {
        my %deletionHash;
        $deletionHash{indexPath} = $headerHashRef->{copiedFromPath};
        $deletionHash{isDeletion} = 1;
        push @diffHashRefs, \%deletionHash;
    }
    if ($headerHashRef->{copiedFromPath}) {
        my %copyHash;
        $copyHash{copiedFromPath} = $headerHashRef->{copiedFromPath};
        $copyHash{indexPath} = $headerHashRef->{indexPath};
        $copyHash{sourceRevision} = $headerHashRef->{sourceRevision} if $headerHashRef->{sourceRevision};
        if ($headerHashRef->{isSvn}) {
            $copyHash{executableBitDelta} = $svnPropertiesHashRef->{executableBitDelta} if $svnPropertiesHashRef->{executableBitDelta};
        }
        push @diffHashRefs, \%copyHash;
    }

    # Note, the order of evaluation for the following if conditional has been explicitly chosen so that
    # it evaluates to false when there is no headerHashRef (e.g. a property change diff for a file that
    # only has property changes).
    if ($headerHashRef->{isCopyWithChanges} || (%$headerHashRef && !$headerHashRef->{copiedFromPath})) {
        # Then add the usual file modification.
        my %diffHash;
        # FIXME: We should expand this code to support other properties.  In the future,
        #        parseSvnDiffProperties may return a hash whose keys are the properties.
        if ($headerHashRef->{isSvn}) {
            # SVN records the change to the executable bit in a separate property change diff
            # that follows the contents of the diff, except for binary diffs.  For binary
            # diffs, the property change diff follows the diff header.
            $diffHash{executableBitDelta} = $svnPropertiesHashRef->{executableBitDelta} if $svnPropertiesHashRef->{executableBitDelta};
        } elsif ($headerHashRef->{isGit}) {
            # Git records the change to the executable bit in the header of a diff.
            $diffHash{executableBitDelta} = $headerHashRef->{executableBitDelta} if $headerHashRef->{executableBitDelta};
        }
        $diffHash{indexPath} = $headerHashRef->{indexPath};
        $diffHash{isBinary} = $headerHashRef->{isBinary} if $headerHashRef->{isBinary};
        $diffHash{isDeletion} = $headerHashRef->{isDeletion} if $headerHashRef->{isDeletion};
        $diffHash{isGit} = $headerHashRef->{isGit} if $headerHashRef->{isGit};
        $diffHash{isNew} = $headerHashRef->{isNew} if $headerHashRef->{isNew};
        $diffHash{isSvn} = $headerHashRef->{isSvn} if $headerHashRef->{isSvn};
        if (!$headerHashRef->{copiedFromPath}) {
            # If the file was copied, then we have already incorporated the
            # sourceRevision information into the change.
            $diffHash{sourceRevision} = $headerHashRef->{sourceRevision} if $headerHashRef->{sourceRevision};
        }
        # FIXME: Remove the need for svnConvertedText.  See the %diffHash
        #        code comments above for more information.
        #
        # Note, we may not always have SVN converted text since we intend
        # to deprecate it in the future.  For example, a property change
        # diff for a file that only has property changes will not return
        # any SVN converted text.
        $diffHash{svnConvertedText} = $svnText if $svnText;
        $diffHash{numTextChunks} = $numTextChunks if $svnText && !$headerHashRef->{isBinary};
        push @diffHashRefs, \%diffHash;
    }

    if (!%$headerHashRef && $svnPropertiesHashRef) {
        # A property change diff for a file that only has property changes.
        my %propertyChangeHash;
        $propertyChangeHash{executableBitDelta} = $svnPropertiesHashRef->{executableBitDelta} if $svnPropertiesHashRef->{executableBitDelta};
        $propertyChangeHash{indexPath} = $svnPropertiesHashRef->{propertyPath};
        $propertyChangeHash{isSvn} = 1;
        push @diffHashRefs, \%propertyChangeHash;
    }

    return (\@diffHashRefs, $line);
}

# Parse an SVN property change diff from the given file handle, and advance
# the handle so the last line read is the first line after this diff.
#
# For the case of an SVN binary diff, the binary contents will follow the
# the property changes.
#
# This subroutine dies if the first line does not begin with "Property changes on"
# or if the separator line that follows this line is missing.
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the footer to parse.  This line begins with
#                "Property changes on".
#   $line: the line last read from $fileHandle.
#
# Returns ($propertyHashRef, $lastReadLine):
#   $propertyHashRef: a hash reference representing an SVN diff footer.
#     propertyPath: the path of the target file.
#     executableBitDelta: the value 1 or -1 if the executable bit was added or
#                         removed from the target file, respectively.
#   $lastReadLine: the line last read from $fileHandle.
sub parseSvnDiffProperties($$)
{
    my ($fileHandle, $line) = @_;

    $_ = $line;

    my %footer;
    if (/$svnPropertiesStartRegEx/) {
        $footer{propertyPath} = $1;
    } else {
        die("Failed to find start of SVN property change, \"Property changes on \": \"$_\"");
    }

    # We advance $fileHandle two lines so that the next line that
    # we process is $svnPropertyStartRegEx in a well-formed footer.
    # A well-formed footer has the form:
    # Property changes on: FileA
    # ___________________________________________________________________
    # Added: svn:executable
    #    + *
    $_ = <$fileHandle>; # Not defined if end-of-file reached.
    my $separator = "_" x 67;
    if (defined($_) && /^$separator[\r\n]+$/) {
        $_ = <$fileHandle>;
    } else {
        die("Failed to find separator line: \"$_\".");
    }

    # FIXME: We should expand this to support other SVN properties
    #        (e.g. return a hash of property key-values that represents
    #        all properties).
    #
    # Notice, we keep processing until we hit end-of-file or some
    # line that does not resemble $svnPropertyStartRegEx, such as
    # the empty line that precedes the start of the binary contents
    # of a patch, or the start of the next diff (e.g. "Index:").
    my $propertyHashRef;
    while (defined($_) && /$svnPropertyStartRegEx/) {
        ($propertyHashRef, $_) = parseSvnProperty($fileHandle, $_);
        if ($propertyHashRef->{name} eq "svn:executable") {
            # Notice, for SVN properties, propertyChangeDelta is always non-zero
            # because a property can only be added or removed.
            $footer{executableBitDelta} = $propertyHashRef->{propertyChangeDelta};   
        }
    }

    return(\%footer, $_);
}

# Parse the next SVN property from the given file handle, and advance the handle so the last
# line read is the first line after the property.
#
# This subroutine dies if the first line is not a valid start of an SVN property,
# or the property is missing a value, or the property change type (e.g. "Added")
# does not correspond to the property value type (e.g. "+").
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the property to parse.  This should be a line
#                that matches $svnPropertyStartRegEx.
#   $line: the line last read from $fileHandle.
#
# Returns ($propertyHashRef, $lastReadLine):
#   $propertyHashRef: a hash reference representing a SVN property.
#     name: the name of the property.
#     value: the last property value.  For instance, suppose the property is "Modified".
#            Then it has both a '-' and '+' property value in that order.  Therefore,
#            the value of this key is the value of the '+' property by ordering (since
#            it is the last value).
#     propertyChangeDelta: the value 1 or -1 if the property was added or
#                          removed, respectively.
#   $lastReadLine: the line last read from $fileHandle.
sub parseSvnProperty($$)
{
    my ($fileHandle, $line) = @_;

    $_ = $line;

    my $propertyName;
    my $propertyChangeType;
    if (/$svnPropertyStartRegEx/) {
        $propertyChangeType = $1;
        $propertyName = $2;
    } else {
        die("Failed to find SVN property: \"$_\".");
    }

    $_ = <$fileHandle>; # Not defined if end-of-file reached.

    if (defined($_) && defined(parseChunkRange($_, "##"))) {
        # FIXME: We should validate the chunk range line that is part of an SVN 1.7
        #        property diff. For now, we ignore this line.
        $_ = <$fileHandle>;
    }

    # The "svn diff" command neither inserts newline characters between property values
    # nor between successive properties.
    #
    # As of SVN 1.7, "svn diff" may insert "\ No newline at end of property" after a
    # property value that doesn't end in a newline.
    #
    # FIXME: We do not support property values that contain tailing newline characters
    #        as it is difficult to disambiguate these trailing newlines from the empty
    #        line that precedes the contents of a binary patch.
    my $propertyValue;
    my $propertyValueType;
    while (defined($_) && /$svnPropertyValueStartRegEx/) {
        # Note, a '-' property may be followed by a '+' property in the case of a "Modified"
        # or "Name" property.  We only care about the ending value (i.e. the '+' property)
        # in such circumstances.  So, we take the property value for the property to be its
        # last parsed property value.
        #
        # FIXME: We may want to consider strictly enforcing a '-', '+' property ordering or
        #        add error checking to prevent '+', '+', ..., '+' and other invalid combinations.
        $propertyValueType = $1;
        ($propertyValue, $_) = parseSvnPropertyValue($fileHandle, $_);
        $_ = <$fileHandle> if defined($_) && /$svnPropertyValueNoNewlineRegEx/;
    }

    if (!$propertyValue) {
        die("Failed to find the property value for the SVN property \"$propertyName\": \"$_\".");
    }

    my $propertyChangeDelta;
    if ($propertyValueType eq "+" || $propertyValueType eq "Merged") {
        $propertyChangeDelta = 1;
    } elsif ($propertyValueType eq "-" || $propertyValueType eq "Reverse-merged") {
        $propertyChangeDelta = -1;
    } else {
        die("Not reached.");
    }

    # We perform a simple validation that an "Added" or "Deleted" property
    # change type corresponds with a "+" and "-" value type, respectively.
    my $expectedChangeDelta;
    if ($propertyChangeType eq "Added") {
        $expectedChangeDelta = 1;
    } elsif ($propertyChangeType eq "Deleted") {
        $expectedChangeDelta = -1;
    }

    if ($expectedChangeDelta && $propertyChangeDelta != $expectedChangeDelta) {
        die("The final property value type found \"$propertyValueType\" does not " .
            "correspond to the property change type found \"$propertyChangeType\".");
    }

    my %propertyHash;
    $propertyHash{name} = $propertyName;
    $propertyHash{propertyChangeDelta} = $propertyChangeDelta;
    $propertyHash{value} = $propertyValue;
    return (\%propertyHash, $_);
}

# Parse the value of an SVN property from the given file handle, and advance
# the handle so the last line read is the first line after the property value.
#
# This subroutine dies if the first line is an invalid SVN property value line
# (i.e. a line that does not begin with "   +" or "   -").
#
# Args:
#   $fileHandle: advanced so the last line read from the handle is the first
#                line of the property value to parse.  This should be a line
#                beginning with "   +" or "   -".
#   $line: the line last read from $fileHandle.
#
# Returns ($propertyValue, $lastReadLine):
#   $propertyValue: the value of the property.
#   $lastReadLine: the line last read from $fileHandle.
sub parseSvnPropertyValue($$)
{
    my ($fileHandle, $line) = @_;

    $_ = $line;

    my $propertyValue;
    my $eol;
    if (/$svnPropertyValueStartRegEx/) {
        $propertyValue = $2; # Does not include the end-of-line character(s).
        $eol = $POSTMATCH;
    } else {
        die("Failed to find property value beginning with '+', '-', 'Merged', or 'Reverse-merged': \"$_\".");
    }

    while (<$fileHandle>) {
        if (/^[\r\n]+$/ || /$svnPropertyValueStartRegEx/ || /$svnPropertyStartRegEx/ || /$svnPropertyValueNoNewlineRegEx/) {
            # Note, we may encounter an empty line before the contents of a binary patch.
            # Also, we check for $svnPropertyValueStartRegEx because a '-' property may be
            # followed by a '+' property in the case of a "Modified" or "Name" property.
            # We check for $svnPropertyStartRegEx because it indicates the start of the
            # next property to parse.
            last;
        }

        # Temporarily strip off any end-of-line characters. We add the end-of-line characters
        # from the previously processed line to the start of this line so that the last line
        # of the property value does not end in end-of-line characters.
        s/([\n\r]+)$//;
        $propertyValue .= "$eol$_";
        $eol = $1;
    }

    return ($propertyValue, $_);
}

# Parse a patch file created by svn-create-patch.
#
# Args:
#   $fileHandle: A file handle to the patch file that has not yet been
#                read from.
#   $optionsHashRef: a hash reference representing optional options to use
#                    when processing a diff.
#     shouldNotUseIndexPathEOL: whether to use the line endings in the diff instead
#                               instead of the line endings in the target file; the
#                               value of 1 if svnConvertedText should use the line
#                               endings in the diff.
#
# Returns:
#   @diffHashRefs: an array of diff hash references.
#                  See the %diffHash documentation above.
sub parsePatch($;$)
{
    my ($fileHandle, $optionsHashRef) = @_;

    my $newDiffHashRefs;
    my @diffHashRefs; # return value

    my $line = <$fileHandle>;

    while (defined($line)) { # Otherwise, at EOF.

        ($newDiffHashRefs, $line) = parseDiff($fileHandle, $line, $optionsHashRef);

        push @diffHashRefs, @$newDiffHashRefs;
    }

    return @diffHashRefs;
}

# Prepare the results of parsePatch() for use in svn-apply and svn-unapply.
#
# Args:
#   $shouldForce: Whether to continue processing if an unexpected
#                 state occurs.
#   @diffHashRefs: An array of references to %diffHashes.
#                  See the %diffHash documentation above.
#
# Returns $preparedPatchHashRef:
#   copyDiffHashRefs: A reference to an array of the $diffHashRefs in
#                     @diffHashRefs that represent file copies. The original
#                     ordering is preserved.
#   nonCopyDiffHashRefs: A reference to an array of the $diffHashRefs in
#                        @diffHashRefs that do not represent file copies.
#                        The original ordering is preserved.
#   sourceRevisionHash: A reference to a hash of source path to source
#                       revision number.
sub prepareParsedPatch($@)
{
    my ($shouldForce, @diffHashRefs) = @_;

    my %copiedFiles;

    # Return values
    my @copyDiffHashRefs = ();
    my @nonCopyDiffHashRefs = ();
    my %sourceRevisionHash = ();
    for my $diffHashRef (@diffHashRefs) {
        my $copiedFromPath = $diffHashRef->{copiedFromPath};
        my $indexPath = $diffHashRef->{indexPath};
        my $sourceRevision = $diffHashRef->{sourceRevision};
        my $sourcePath;

        if (defined($copiedFromPath)) {
            # Then the diff is a copy operation.
            $sourcePath = $copiedFromPath;

            # FIXME: Consider printing a warning or exiting if
            #        exists($copiedFiles{$indexPath}) is true -- i.e. if
            #        $indexPath appears twice as a copy target.
            $copiedFiles{$indexPath} = $sourcePath;

            push @copyDiffHashRefs, $diffHashRef;
        } else {
            # Then the diff is not a copy operation.
            $sourcePath = $indexPath;

            push @nonCopyDiffHashRefs, $diffHashRef;
        }

        if (defined($sourceRevision)) {
            if (exists($sourceRevisionHash{$sourcePath}) &&
                ($sourceRevisionHash{$sourcePath} != $sourceRevision)) {
                if (!$shouldForce) {
                    die "Two revisions of the same file required as a source:\n".
                        "    $sourcePath:$sourceRevisionHash{$sourcePath}\n".
                        "    $sourcePath:$sourceRevision";
                }
            }
            $sourceRevisionHash{$sourcePath} = $sourceRevision;
        }
    }

    my %preparedPatchHash;

    $preparedPatchHash{copyDiffHashRefs} = \@copyDiffHashRefs;
    $preparedPatchHash{nonCopyDiffHashRefs} = \@nonCopyDiffHashRefs;
    $preparedPatchHash{sourceRevisionHash} = \%sourceRevisionHash;

    return \%preparedPatchHash;
}

# Return localtime() for the project's time zone, given an integer time as
# returned by Perl's time() function.
sub localTimeInProjectTimeZone($)
{
    my $epochTime = shift;

    # Change the time zone temporarily for the localtime() call.
    my $savedTimeZone = $ENV{'TZ'};
    $ENV{'TZ'} = $changeLogTimeZone;
    my @localTime = localtime($epochTime);
    if (defined $savedTimeZone) {
         $ENV{'TZ'} = $savedTimeZone;
    } else {
         delete $ENV{'TZ'};
    }

    return @localTime;
}

# Set the reviewer and date in a ChangeLog patch, and return the new patch.
#
# Args:
#   $patch: a ChangeLog patch as a string.
#   $reviewer: the name of the reviewer, or undef if the reviewer should not be set.
#   $epochTime: an integer time as returned by Perl's time() function.
sub setChangeLogDateAndReviewer($$$)
{
    my ($patch, $reviewer, $epochTime) = @_;

    my @localTime = localTimeInProjectTimeZone($epochTime);
    my $newDate = strftime("%Y-%m-%d", @localTime);

    my $firstChangeLogLineRegEx = qr#(\n\+)\d{4}-[^-]{2}-[^-]{2}(  )#;
    $patch =~ s/$firstChangeLogLineRegEx/$1$newDate$2/;

    if (defined($reviewer)) {
        # We include a leading plus ("+") in the regular expression to make
        # the regular expression less likely to match text in the leading junk
        # for the patch, if the patch has leading junk.
        $patch =~ s/(\n\+.*)NOBODY \(OOPS!\)/$1$reviewer/;
    }

    return $patch;
}

# If possible, returns a ChangeLog patch equivalent to the given one,
# but with the newest ChangeLog entry inserted at the top of the
# file -- i.e. no leading context and all lines starting with "+".
#
# If given a patch string not representable as a patch with the above
# properties, it returns the input back unchanged.
#
# WARNING: This subroutine can return an inequivalent patch string if
# both the beginning of the new ChangeLog file matches the beginning
# of the source ChangeLog, and the source beginning was modified.
# Otherwise, it is guaranteed to return an equivalent patch string,
# if it returns.
#
# Applying this subroutine to ChangeLog patches allows svn-apply to
# insert new ChangeLog entries at the top of the ChangeLog file.
# svn-apply uses patch with --fuzz=3 to do this. We need to apply
# this subroutine because the diff(1) command is greedy when matching
# lines. A new ChangeLog entry with the same date and author as the
# previous will match and cause the diff to have lines of starting
# context.
#
# This subroutine has unit tests in VCSUtils_unittest.pl.
#
# Returns $changeLogHashRef:
#   $changeLogHashRef: a hash reference representing a change log patch.
#     patch: a ChangeLog patch equivalent to the given one, but with the
#            newest ChangeLog entry inserted at the top of the file, if possible.              
sub fixChangeLogPatch($)
{
    my $patch = shift; # $patch will only contain patch fragments for ChangeLog.

    $patch =~ s|test_expectations.txt:|TestExpectations:|g;

    $patch =~ /(\r?\n)/;
    my $lineEnding = $1;
    my @lines = split(/$lineEnding/, $patch);

    my $i = 0; # We reuse the same index throughout.

    # Skip to beginning of first chunk.
    for (; $i < @lines; ++$i) {
        if (substr($lines[$i], 0, 1) eq "@") {
            last;
        }
    }
    my $chunkStartIndex = ++$i;
    my %changeLogHashRef;

    # Optimization: do not process if new lines already begin the chunk.
    if (substr($lines[$i], 0, 1) eq "+") {
        $changeLogHashRef{patch} = $patch;
        return \%changeLogHashRef;
    }

    # Skip to first line of newly added ChangeLog entry.
    # For example, +2009-06-03  Eric Seidel  <eric@webkit.org>
    my $dateStartRegEx = '^\+(\d{4}-\d{2}-\d{2})' # leading "+" and date
                         . '\s+(.+)\s+' # name
                         . '<([^<>]+)>$'; # e-mail address

    for (; $i < @lines; ++$i) {
        my $line = $lines[$i];
        my $firstChar = substr($line, 0, 1);
        if ($line =~ /$dateStartRegEx/) {
            last;
        } elsif ($firstChar eq " " or $firstChar eq "+") {
            next;
        }
        $changeLogHashRef{patch} = $patch; # Do not change if, for example, "-" or "@" found.
        return \%changeLogHashRef;
    }
    if ($i >= @lines) {
        $changeLogHashRef{patch} = $patch; # Do not change if date not found.
        return \%changeLogHashRef;
    }
    my $dateStartIndex = $i;

    # Rewrite overlapping lines to lead with " ".
    my @overlappingLines = (); # These will include a leading "+".
    for (; $i < @lines; ++$i) {
        my $line = $lines[$i];
        if (substr($line, 0, 1) ne "+") {
          last;
        }
        push(@overlappingLines, $line);
        $lines[$i] = " " . substr($line, 1);
    }

    # Remove excess ending context, if necessary.
    my $shouldTrimContext = 1;
    for (; $i < @lines; ++$i) {
        my $firstChar = substr($lines[$i], 0, 1);
        if ($firstChar eq " ") {
            next;
        } elsif ($firstChar eq "@") {
            last;
        }
        $shouldTrimContext = 0; # For example, if "+" or "-" encountered.
        last;
    }
    my $deletedLineCount = 0;
    if ($shouldTrimContext) { # Also occurs if end of file reached.
        splice(@lines, $i - @overlappingLines, @overlappingLines);
        $deletedLineCount = @overlappingLines;
    }

    # Work backwards, shifting overlapping lines towards front
    # while checking that patch stays equivalent.
    for ($i = $dateStartIndex - 1; @overlappingLines && $i >= $chunkStartIndex; --$i) {
        my $line = $lines[$i];
        if (substr($line, 0, 1) ne " ") {
            next;
        }
        my $text = substr($line, 1);
        my $newLine = pop(@overlappingLines);
        if ($text ne substr($newLine, 1)) {
            $changeLogHashRef{patch} = $patch; # Unexpected difference.
            return \%changeLogHashRef;
        }
        $lines[$i] = "+$text";
    }

    # If @overlappingLines > 0, this is where we make use of the
    # assumption that the beginning of the source file was not modified.
    splice(@lines, $chunkStartIndex, 0, @overlappingLines);

    # Update the date start index as it may have changed after shifting
    # the overlapping lines towards the front.
    for ($i = $chunkStartIndex; $i < $dateStartIndex; ++$i) {
        $dateStartIndex = $i if $lines[$i] =~ /$dateStartRegEx/;
    }
    splice(@lines, $chunkStartIndex, $dateStartIndex - $chunkStartIndex); # Remove context of later entry.
    $deletedLineCount += $dateStartIndex - $chunkStartIndex;

    # Update the initial chunk range.
    my $chunkRangeHashRef = parseChunkRange($lines[$chunkStartIndex - 1]);
    if (!$chunkRangeHashRef) {
        # FIXME: Handle errors differently from ChangeLog files that
        # are okay but should not be altered. That way we can find out
        # if improvements to the script ever become necessary.
        $changeLogHashRef{patch} = $patch; # Error: unexpected patch string format.
        return \%changeLogHashRef;
    }
    my $oldSourceLineCount = $chunkRangeHashRef->{lineCount};
    my $oldTargetLineCount = $chunkRangeHashRef->{newLineCount};

    my $sourceLineCount = $oldSourceLineCount + @overlappingLines - $deletedLineCount;
    my $targetLineCount = $oldTargetLineCount + @overlappingLines - $deletedLineCount;
    $lines[$chunkStartIndex - 1] = "@@ -1,$sourceLineCount +1,$targetLineCount @@";

    $changeLogHashRef{patch} = join($lineEnding, @lines) . "\n"; # patch(1) expects an extra trailing newline.
    return \%changeLogHashRef;
}

# This is a supporting method for runPatchCommand.
#
# Arg: the optional $args parameter passed to runPatchCommand (can be undefined).
#
# Returns ($patchCommand, $isForcing).
#
# This subroutine has unit tests in VCSUtils_unittest.pl.
sub generatePatchCommand($)
{
    my ($passedArgsHashRef) = @_;

    my $argsHashRef = { # Defaults
        ensureForce => 0,
        shouldReverse => 0,
        options => []
    };
    
    # Merges hash references. It's okay here if passed hash reference is undefined.
    @{$argsHashRef}{keys %{$passedArgsHashRef}} = values %{$passedArgsHashRef};
    
    my $ensureForce = $argsHashRef->{ensureForce};
    my $shouldReverse = $argsHashRef->{shouldReverse};
    my $options = $argsHashRef->{options};

    if (! $options) {
        $options = [];
    } else {
        $options = [@{$options}]; # Copy to avoid side effects.
    }

    my $isForcing = 0;
    if (grep /^--force$/, @{$options}) {
        $isForcing = 1;
    } elsif ($ensureForce) {
        push @{$options}, "--force";
        $isForcing = 1;
    }

    if ($shouldReverse) { # No check: --reverse should never be passed explicitly.
        push @{$options}, "--reverse";
    }

    @{$options} = sort(@{$options}); # For easier testing.

    my $patchCommand = join(" ", "patch -p0", @{$options});

    return ($patchCommand, $isForcing);
}

# Apply the given patch using the patch(1) command.
#
# On success, return the resulting exit status. Otherwise, exit with the
# exit status. If "--force" is passed as an option, however, then never
# exit and always return the exit status.
#
# Args:
#   $patch: a patch string.
#   $repositoryRootPath: an absolute path to the repository root.
#   $pathRelativeToRoot: the path of the file to be patched, relative to the
#                        repository root. This should normally be the path
#                        found in the patch's "Index:" line. It is passed
#                        explicitly rather than reparsed from the patch
#                        string for optimization purposes.
#                            This is used only for error reporting. The
#                        patch command gleans the actual file to patch
#                        from the patch string.
#   $args: a reference to a hash of optional arguments. The possible
#          keys are --
#            ensureForce: whether to ensure --force is passed (defaults to 0).
#            shouldReverse: whether to pass --reverse (defaults to 0).
#            options: a reference to an array of options to pass to the
#                     patch command. The subroutine passes the -p0 option
#                     no matter what. This should not include --reverse.
#
# This subroutine has unit tests in VCSUtils_unittest.pl.
sub runPatchCommand($$$;$)
{
    my ($patch, $repositoryRootPath, $pathRelativeToRoot, $args) = @_;

    my ($patchCommand, $isForcing) = generatePatchCommand($args);

    # Temporarily change the working directory since the path found
    # in the patch's "Index:" line is relative to the repository root
    # (i.e. the same as $pathRelativeToRoot).
    my $cwd = Cwd::getcwd();
    chdir $repositoryRootPath;

    open PATCH, "| $patchCommand" or die "Could not call \"$patchCommand\" for file \"$pathRelativeToRoot\": $!";
    print PATCH $patch;
    close PATCH;
    my $exitStatus = exitStatus($?);

    chdir $cwd;

    if ($exitStatus && !$isForcing) {
        print "Calling \"$patchCommand\" for file \"$pathRelativeToRoot\" returned " .
              "status $exitStatus.  Pass --force to ignore patch failures.\n";
        exit $exitStatus;
    }

    return $exitStatus;
}

# Merge ChangeLog patches using a three-file approach.
#
# This is used by resolve-ChangeLogs when it's operated as a merge driver
# and when it's used to merge conflicts after a patch is applied or after
# an svn update.
#
# It's also used for traditional rejected patches.
#
# Args:
#   $fileMine:  The merged version of the file.  Also known in git as the
#               other branch's version (%B) or "ours".
#               For traditional patch rejects, this is the *.rej file.
#   $fileOlder: The base version of the file.  Also known in git as the
#               ancestor version (%O) or "base".
#               For traditional patch rejects, this is the *.orig file.
#   $fileNewer: The current version of the file.  Also known in git as the
#               current version (%A) or "theirs".
#               For traditional patch rejects, this is the original-named
#               file.
#
# Returns 1 if merge was successful, else 0.
sub mergeChangeLogs($$$)
{
    my ($fileMine, $fileOlder, $fileNewer) = @_;

    my $traditionalReject = $fileMine =~ /\.rej$/ ? 1 : 0;

    local $/ = undef;

    my $patch;
    if ($traditionalReject) {
        open(DIFF, "<", $fileMine) or die $!;
        $patch = <DIFF>;
        close(DIFF);
        rename($fileMine, "$fileMine.save");
        rename($fileOlder, "$fileOlder.save");
    } else {
        open(DIFF, "diff -u -a --binary \"$fileOlder\" \"$fileMine\" |") or die $!;
        $patch = <DIFF>;
        close(DIFF);
    }

    unlink("${fileNewer}.orig");
    unlink("${fileNewer}.rej");

    open(PATCH, "| patch --force --fuzz=3 --binary \"$fileNewer\" > " . File::Spec->devnull()) or die $!;
    if ($traditionalReject) {
        print PATCH $patch;
    } else {
        my $changeLogHash = fixChangeLogPatch($patch);
        print PATCH $changeLogHash->{patch};
    }
    close(PATCH);

    my $result = !exitStatus($?);

    # Refuse to merge the patch if it did not apply cleanly
    if (-e "${fileNewer}.rej") {
        unlink("${fileNewer}.rej");
        if (-f "${fileNewer}.orig") {
            unlink($fileNewer);
            rename("${fileNewer}.orig", $fileNewer);
        }
    } else {
        unlink("${fileNewer}.orig");
    }

    if ($traditionalReject) {
        rename("$fileMine.save", $fileMine);
        rename("$fileOlder.save", $fileOlder);
    }

    return $result;
}

sub gitConfig($)
{
    return unless $isGit;

    my ($config) = @_;

    my $result = `git config $config`;
    chomp $result;
    return $result;
}

sub changeLogSuffix()
{
    my $rootPath = determineVCSRoot();
    my $changeLogSuffixFile = File::Spec->catfile($rootPath, ".changeLogSuffix");
    return "" if ! -e $changeLogSuffixFile;
    open FILE, $changeLogSuffixFile or die "Could not open $changeLogSuffixFile: $!";
    my $changeLogSuffix = <FILE>;
    chomp $changeLogSuffix;
    close FILE;
    return $changeLogSuffix;
}

sub changeLogFileName()
{
    return "ChangeLog" . changeLogSuffix()
}

sub changeLogNameError($)
{
    my ($message) = @_;
    print STDERR "$message\nEither:\n";
    print STDERR "  set CHANGE_LOG_NAME in your environment\n";
    print STDERR "  OR pass --name= on the command line\n";
    print STDERR "  OR set REAL_NAME in your environment";
    print STDERR "  OR git users can set 'git config user.name'\n";
    exit(1);
}

sub changeLogName()
{
    my $name = $ENV{CHANGE_LOG_NAME} || $ENV{REAL_NAME} || gitConfig("user.name") || (split /\s*,\s*/, (getpwuid $<)[6])[0];

    changeLogNameError("Failed to determine ChangeLog name.") unless $name;
    # getpwuid seems to always succeed on windows, returning the username instead of the full name.  This check will catch that case.
    changeLogNameError("'$name' does not contain a space!  ChangeLogs should contain your full name.") unless ($name =~ /\S\s\S/);

    return $name;
}

sub changeLogEmailAddressError($)
{
    my ($message) = @_;
    print STDERR "$message\nEither:\n";
    print STDERR "  set CHANGE_LOG_EMAIL_ADDRESS in your environment\n";
    print STDERR "  OR pass --email= on the command line\n";
    print STDERR "  OR set EMAIL_ADDRESS in your environment\n";
    print STDERR "  OR git users can set 'git config user.email'\n";
    exit(1);
}

sub changeLogEmailAddress()
{
    my $emailAddress = $ENV{CHANGE_LOG_EMAIL_ADDRESS} || $ENV{EMAIL_ADDRESS} || gitConfig("user.email");

    changeLogEmailAddressError("Failed to determine email address for ChangeLog.") unless $emailAddress;
    changeLogEmailAddressError("Email address '$emailAddress' does not contain '\@' and is likely invalid.") unless ($emailAddress =~ /\@/);

    return $emailAddress;
}

# http://tools.ietf.org/html/rfc1924
sub decodeBase85($)
{
    my ($encoded) = @_;
    my %table;
    my @characters = ('0'..'9', 'A'..'Z', 'a'..'z', '!', '#', '$', '%', '&', '(', ')', '*', '+', '-', ';', '<', '=', '>', '?', '@', '^', '_', '`', '{', '|', '}', '~');
    for (my $i = 0; $i < 85; $i++) {
        $table{$characters[$i]} = $i;
    }

    my $decoded = '';
    my @encodedChars = $encoded =~ /./g;

    for (my $encodedIter = 0; defined($encodedChars[$encodedIter]);) {
        my $digit = 0;
        for (my $i = 0; $i < 5; $i++) {
            $digit *= 85;
            my $char = $encodedChars[$encodedIter];
            $digit += $table{$char};
            $encodedIter++;
        }

        for (my $i = 0; $i < 4; $i++) {
            $decoded .= chr(($digit >> (3 - $i) * 8) & 255);
        }
    }

    return $decoded;
}

sub decodeGitBinaryChunk($$)
{
    my ($contents, $fullPath) = @_;

    # Load this module lazily in case the user don't have this module
    # and won't handle git binary patches.
    require Compress::Zlib;

    my $encoded = "";
    my $compressedSize = 0;
    while ($contents =~ /^([A-Za-z])(.*)$/gm) {
        my $line = $2;
        next if $line eq "";
        die "$fullPath: unexpected size of a line: $&" if length($2) % 5 != 0;
        my $actualSize = length($2) / 5 * 4;
        my $encodedExpectedSize = ord($1);
        my $expectedSize = $encodedExpectedSize <= ord("Z") ? $encodedExpectedSize - ord("A") + 1 : $encodedExpectedSize - ord("a") + 27;

        die "$fullPath: unexpected size of a line: $&" if int(($expectedSize + 3) / 4) * 4 != $actualSize;
        $compressedSize += $expectedSize;
        $encoded .= $line;
    }

    my $compressed = decodeBase85($encoded);
    $compressed = substr($compressed, 0, $compressedSize);
    return Compress::Zlib::uncompress($compressed);
}

sub decodeGitBinaryPatch($$)
{
    my ($contents, $fullPath) = @_;

    # Git binary patch has two chunks. One is for the normal patching
    # and another is for the reverse patching.
    #
    # Each chunk a line which starts from either "literal" or "delta",
    # followed by a number which specifies decoded size of the chunk.
    #
    # Then, content of the chunk comes. To decode the content, we
    # need decode it with base85 first, and then zlib.
    my $gitPatchRegExp = '(literal|delta) ([0-9]+)\n([A-Za-z0-9!#$%&()*+-;<=>?@^_`{|}~\\n]*?)\n\n';
    if ($contents !~ m"\nGIT binary patch\n$gitPatchRegExp$gitPatchRegExp\Z") {
        die "$fullPath: unknown git binary patch format"
    }

    my $binaryChunkType = $1;
    my $binaryChunkExpectedSize = $2;
    my $encodedChunk = $3;
    my $reverseBinaryChunkType = $4;
    my $reverseBinaryChunkExpectedSize = $5;
    my $encodedReverseChunk = $6;

    my $binaryChunk = decodeGitBinaryChunk($encodedChunk, $fullPath);
    my $binaryChunkActualSize = length($binaryChunk);
    my $reverseBinaryChunk = decodeGitBinaryChunk($encodedReverseChunk, $fullPath);
    my $reverseBinaryChunkActualSize = length($reverseBinaryChunk);

    die "$fullPath: unexpected size of the first chunk (expected $binaryChunkExpectedSize but was $binaryChunkActualSize" if ($binaryChunkType eq "literal" and $binaryChunkExpectedSize != $binaryChunkActualSize);
    die "$fullPath: unexpected size of the second chunk (expected $reverseBinaryChunkExpectedSize but was $reverseBinaryChunkActualSize" if ($reverseBinaryChunkType eq "literal" and $reverseBinaryChunkExpectedSize != $reverseBinaryChunkActualSize);

    return ($binaryChunkType, $binaryChunk, $reverseBinaryChunkType, $reverseBinaryChunk);
}

sub readByte($$)
{
    my ($data, $location) = @_;
    
    # Return the byte at $location in $data as a numeric value. 
    return ord(substr($data, $location, 1));
}

# The git binary delta format is undocumented, except in code:
# - https://github.com/git/git/blob/master/delta.h:get_delta_hdr_size is the source
#   of the algorithm in decodeGitBinaryPatchDeltaSize.
# - https://github.com/git/git/blob/master/patch-delta.c:patch_delta is the source
#   of the algorithm in applyGitBinaryPatchDelta.
sub decodeGitBinaryPatchDeltaSize($)
{
    my ($binaryChunk) = @_;
    
    # Source and destination buffer sizes are stored in 7-bit chunks at the
    # start of the binary delta patch data.  The highest bit in each byte
    # except the last is set; the remaining 7 bits provide the next
    # chunk of the size.  The chunks are stored in ascending significance
    # order.
    my $cmd;
    my $size = 0;
    my $shift = 0;
    for (my $i = 0; $i < length($binaryChunk);) {
        $cmd = readByte($binaryChunk, $i++);
        $size |= ($cmd & 0x7f) << $shift;
        $shift += 7;
        if (!($cmd & 0x80)) {
            return ($size, $i);
        }
    }
}

sub applyGitBinaryPatchDelta($$)
{
    my ($binaryChunk, $originalContents) = @_;
    
    # Git delta format consists of two headers indicating source buffer size
    # and result size, then a series of commands.  Each command is either
    # a copy-from-old-version (the 0x80 bit is set) or a copy-from-delta
    # command.  Commands are applied sequentially to generate the result.
    #
    # A copy-from-old-version command encodes an offset and size to copy
    # from in subsequent bits, while a copy-from-delta command consists only
    # of the number of bytes to copy from the delta.

    # We don't use these values, but we need to know how big they are so that
    # we can skip to the diff data.
    my ($size, $bytesUsed) = decodeGitBinaryPatchDeltaSize($binaryChunk);
    $binaryChunk = substr($binaryChunk, $bytesUsed);
    ($size, $bytesUsed) = decodeGitBinaryPatchDeltaSize($binaryChunk);
    $binaryChunk = substr($binaryChunk, $bytesUsed);

    my $out = "";
    for (my $i = 0; $i < length($binaryChunk); ) {
        my $cmd = ord(substr($binaryChunk, $i++, 1));
        if ($cmd & 0x80) {
            # Extract an offset and size from the delta data, then copy
            # $size bytes from $offset in the original data into the output.
            my $offset = 0;
            my $size = 0;
            if ($cmd & 0x01) { $offset = readByte($binaryChunk, $i++); }
            if ($cmd & 0x02) { $offset |= readByte($binaryChunk, $i++) << 8; }
            if ($cmd & 0x04) { $offset |= readByte($binaryChunk, $i++) << 16; }
            if ($cmd & 0x08) { $offset |= readByte($binaryChunk, $i++) << 24; }
            if ($cmd & 0x10) { $size = readByte($binaryChunk, $i++); }
            if ($cmd & 0x20) { $size |= readByte($binaryChunk, $i++) << 8; }
            if ($cmd & 0x40) { $size |= readByte($binaryChunk, $i++) << 16; }
            if ($size == 0) { $size = 0x10000; }
            $out .= substr($originalContents, $offset, $size);
        } elsif ($cmd) {
            # Copy $cmd bytes from the delta data into the output.
            $out .= substr($binaryChunk, $i, $cmd);
            $i += $cmd;
        } else {
            die "unexpected delta opcode 0";
        }
    }

    return $out;
}

sub escapeSubversionPath($)
{
    my ($path) = @_;
    $path .= "@" if $path =~ /@/;
    return $path;
}

sub runCommand(@)
{
    my @args = @_;
    my $pid = open(CHILD, "-|");
    if (!defined($pid)) {
        die "Failed to fork(): $!";
    }
    if ($pid) {
        # Parent process
        my $childStdout;
        while (<CHILD>) {
            $childStdout .= $_;
        }
        close(CHILD);
        my %childOutput;
        $childOutput{exitStatus} = exitStatus($?);
        $childOutput{stdout} = $childStdout if $childStdout;
        return \%childOutput;
    }
    # Child process
    # FIXME: Consider further hardening of this function, including sanitizing the environment.
    exec { $args[0] } @args or die "Failed to exec(): $!";
}

sub gitCommitForSVNRevision
{
    my ($svnRevision) = @_;
    my $command = "git svn find-rev r" . $svnRevision;
    $command = "LC_ALL=C $command" if !isWindows();
    my $gitHash = `$command`;
    if (!defined($gitHash)) {
        $gitHash = "unknown";
        warn "Unable to determine GIT commit from SVN revision";
    } else {
        chop($gitHash);
    }
    return $gitHash;
}

sub listOfChangedFilesBetweenRevisions
{
    my ($sourceDir, $firstRevision, $lastRevision) = @_;
    my $command;

    if ($firstRevision eq "unknown" or $lastRevision eq "unknown") {
        return ();
    }

    # Some VCS functions don't work from within the build dir, so always
    # go to the source dir first.
    my $cwd = Cwd::getcwd();
    chdir $sourceDir;

    if (isGit()) {
        my $firstCommit = gitCommitForSVNRevision($firstRevision);
        my $lastCommit = gitCommitForSVNRevision($lastRevision);
        $command = "git diff --name-status $firstCommit..$lastCommit";
    } elsif (isSVN()) {
        $command = "svn diff --summarize -r $firstRevision:$lastRevision";
    }

    my @result = ();

    if ($command) {
        my $diffOutput = `$command`;
        $diffOutput =~ s/^[A-Z]\s+//gm;
        @result = split(/[\r\n]+/, $diffOutput);
    }

    chdir $cwd;

    return @result;
}


1;
