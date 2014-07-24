#!/usr/bin/perl -w

# Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

# Used for helping remove extra blank lines from files when processing.
# see split-class for an example usage (or other scripts in bugzilla)

BEGIN {
   use Exporter   ();
   our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
   $VERSION     = 1.00;
   @ISA         = qw(Exporter);
   @EXPORT      = qw(&resetSpacingHeuristics &isOnlyWhiteSpace &applySpacingHeuristicsAndPrint &setPreviousAllowedLine &setPreviousAllowedLine &printPendingEmptyLines &ignoringLine);
   %EXPORT_TAGS = ();
   @EXPORT_OK   = ();
}

our @EXPORT_OK;

my $justFoundEmptyLine = 0;
my $previousLineWasDisallowed = 0;
my $previousAllowedLine = "";
my $pendingEmptyLines = "";

sub resetSpacingHeuristics
{
    $justFoundEmptyLine = 0;
    $previousLineWasDisallowed = 0;
    $previousAllowedLine = "";
    $pendingEmptyLines = "";
}

sub isOnlyWhiteSpace
{
    my $line = shift;
    my $isOnlyWhiteSpace = ($line =~ m/^\s+$/);
    $pendingEmptyLines .= $line if ($isOnlyWhiteSpace);
    return $isOnlyWhiteSpace;
}

sub applySpacingHeuristicsAndPrint
{
    my ($out, $line) = @_;
    
    printPendingEmptyLines($out, $line);
    $previousLineWasDisallowed = 0;
    print $out $line;
}

sub setPreviousAllowedLine
{
    my $line = shift;
    $previousAllowedLine = $line;
}

sub printPendingEmptyLines
{
    my $out = shift;
    my $line = shift;
    if ($previousLineWasDisallowed) {
        if (!($pendingEmptyLines eq "") && !($previousAllowedLine =~ m/{\s*$/) && !($line =~ m/^\s*}/)) {
            $pendingEmptyLines = "\n";
        } else {
            $pendingEmptyLines = "";
        }
    }
    print $out $pendingEmptyLines;
    $pendingEmptyLines = "";
}

sub ignoringLine
{
    # my $line = shift; # ignoring input argument
    $previousLineWasDisallowed = 1;
}

1;