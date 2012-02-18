#!/usr/bin/perl -w

# Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc. All rights reserved.
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

use strict;

my %aliasesFromCharsetsFile;
my %namesWritten;

my $output = "";

my $error = 0;

sub error ($)
{
    print STDERR @_, "\n";
    $error = 1;
}

sub emit_line
{
    my ($name, $prefix, $encoding, $flags) = @_;
 
    error "$name shows up twice in output" if $namesWritten{$name};
    $namesWritten{$name} = 1;
    
    $output .= "        { \"$name\", $prefix$encoding },\n";
}

sub process_platform_encodings
{
    my ($filename, $PlatformPrefix) = @_;
    my $baseFilename = $filename;
    $baseFilename =~ s|.*/||;
    
    my %seenPlatformNames;
    my %seenIANANames;
    
    open PLATFORM_ENCODINGS, $filename or die;
    
    while (<PLATFORM_ENCODINGS>) {
        chomp;
        s/\#.*$//;
        s/\s+$//;
        if (my ($PlatformName, undef, $flags, $IANANames) = /^(.+?)(, (.+))?: (.+)$/) {
            my %aliases;
            
            my $PlatformNameWithFlags = $PlatformName;
            if ($flags) {
                $PlatformNameWithFlags .= ", " . $flags;
            } else {
                $flags = "NoEncodingFlags";
            }
            error "Platform encoding name $PlatformName is mentioned twice in $baseFilename" if $seenPlatformNames{$PlatformNameWithFlags};
            $seenPlatformNames{$PlatformNameWithFlags} = 1;

            # Build the aliases list.
            # Also check that no two names are part of the same entry in the charsets file.
            my @IANANames = split ", ", $IANANames;
            my $firstName = "";
            my $canonicalFirstName = "";
            my $prevName = "";
            for my $name (@IANANames) {
                if ($firstName eq "") {
                    if ($name !~ /^[-A-Za-z0-9_]+$/) {
                        error "$name, in $baseFilename, has illegal characters in it";
                        next;
                    }
                    $firstName = $name;
                } else {
                    if ($name !~ /^[a-z0-9]+$/) {
                        error "$name, in $baseFilename, has illegal characters in it (must be all lowercase alphanumeric)";
                        next;
                    }
                    if ($name le $prevName) {
                        error "$name comes after $prevName in $baseFilename, but everything must be in alphabetical order";
                    }
                    $prevName = $name;
                }
                
                my $canonicalName = lc $name;
                $canonicalName =~ tr/-_//d;
                
                $canonicalFirstName = $canonicalName if $canonicalFirstName eq "";
                
                error "$name is mentioned twice in $baseFilename" if $seenIANANames{$canonicalName};
                $seenIANANames{$canonicalName} = 1;
                
                $aliases{$canonicalName} = 1;
                next if !$aliasesFromCharsetsFile{$canonicalName};
                for my $alias (@{$aliasesFromCharsetsFile{$canonicalName}}) {
                    $aliases{$alias} = 1;
                }
                for my $otherName (@IANANames) {
                    next if $canonicalName eq $otherName;
                    if ($aliasesFromCharsetsFile{$otherName}
                        && $aliasesFromCharsetsFile{$canonicalName} eq $aliasesFromCharsetsFile{$otherName}
                        && $canonicalName le $otherName) {
                        error "$baseFilename lists both $name and $otherName under $PlatformName, but that aliasing is already specified in character-sets.txt";
                    }
                }
            }
            
            # write out
            emit_line($firstName, $PlatformPrefix, $PlatformName, $flags);
            for my $alias (sort keys %aliases) {
                emit_line($alias, $PlatformPrefix, $PlatformName, $flags) if $alias ne $canonicalFirstName;
            }
        } elsif (/^([a-zA-Z0-9_]+)(, (.+))?$/) {
            my $PlatformName = $1;
            
            error "Platform encoding name $PlatformName is mentioned twice in $baseFilename" if $seenPlatformNames{$PlatformName};
            $seenPlatformNames{$PlatformName} = 1;
        } elsif (/./) {
            error "syntax error in $baseFilename, line $.";
        }
    }
    
    close PLATFORM_ENCODINGS;
}

sub process_iana_charset 
{
    my ($canonical_name, @aliases) = @_;
    
    return if !$canonical_name;
    
    my @names = sort $canonical_name, @aliases;
    
    for my $name (@names) {
        $aliasesFromCharsetsFile{$name} = \@names;
    }
}

sub process_iana_charsets
{
    my ($filename) = @_;
    
    open CHARSETS, $filename or die;
    
    my %seen;
    
    my $canonical_name;
    my @aliases;
    
    my %exceptions = ( isoir91 => 1, isoir92 => 1 );
    
    while (<CHARSETS>) {
        chomp;
        if ((my $new_canonical_name) = /Name: ([^ \t]*).*/) {
            $new_canonical_name = lc $new_canonical_name;
            $new_canonical_name =~ tr/a-z0-9//cd;
            
            error "saw $new_canonical_name twice in character-sets.txt", if $seen{$new_canonical_name};
            $seen{$new_canonical_name} = $new_canonical_name;
            
            process_iana_charset $canonical_name, @aliases;

            $canonical_name = $new_canonical_name;
            @aliases = ();
        } elsif ((my $new_alias) = /Alias: ([^ \t]*).*/) {
            $new_alias = lc $new_alias;
            $new_alias =~ tr/a-z0-9//cd;
            
            # do this after normalizing the alias, sometimes character-sets.txt
            # has weird escape characters, e.g. \b after None
            next if $new_alias eq "none";

            error "saw $new_alias twice in character-sets.txt $seen{$new_alias}, $canonical_name", if $seen{$new_alias} && $seen{$new_alias} ne $canonical_name && !$exceptions{$new_alias};
            push @aliases, $new_alias if !$seen{$new_alias};
            $seen{$new_alias} = $canonical_name;            
        }
    }
    
    process_iana_charset $canonical_name, @aliases;
    
    close CHARSETS;
}

# Program body

process_iana_charsets($ARGV[0]);
process_platform_encodings($ARGV[1], $ARGV[2]);

exit 1 if $error;

print <<EOF
// File generated by make-charset-table.pl. Do not edit!

#include "config.h"
#include "CharsetData.h"

namespace WebCore {

    const CharsetEntry CharsetTable[] = {
$output
        { 0, 0 }
    };

}
EOF
