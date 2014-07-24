#!/usr/bin/perl -w
#
# Copyright (C) 2011 Google Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

# This script tests the parser of prepare-ChangeLog (i.e. get_function_line_ranges_for_XXXX()).
# This script runs the unittests specified in @testFiles.

use strict;
use warnings;

use Data::Dumper;
use File::Basename;
use File::Spec;
use File::Temp qw(tempfile);
use FindBin;
use Getopt::Long;
use Test::More;
use lib File::Spec->catdir($FindBin::Bin, "..");
use LoadAsModule qw(PrepareChangeLog prepare-ChangeLog);

sub captureOutput($);
sub convertAbsolutepathToWebKitPath($);

my %testFiles = ("perl_unittests.pl" => "get_function_line_ranges_for_perl",
                 "python_unittests.py" => "get_function_line_ranges_for_python",
                 "java_unittests.java" => "get_function_line_ranges_for_java",
                 "cpp_unittests.cpp" => "get_function_line_ranges_for_cpp",
                 "javascript_unittests.js" => "get_function_line_ranges_for_javascript",
                 "css_unittests.css" => "get_selector_line_ranges_for_css",
                 "css_unittests_warning.css" => "get_selector_line_ranges_for_css",
                );

my $resetResults;
GetOptions('reset-results' => \$resetResults);

my @testSet;
foreach my $testFile (sort keys %testFiles) {
    my $basename = $testFile;
    $basename = $1 if $basename =~ /^(.*)\.[^\.]*$/;
    push @testSet, {method => $testFiles{$testFile},
                    inputFile => File::Spec->catdir($FindBin::Bin, "resources", $testFile),
                    expectedFile => File::Spec->catdir($FindBin::Bin, "resources", $basename . "-expected.txt")};
}

plan(tests => scalar @testSet);
foreach my $test (@testSet) {
    open FH, "< $test->{inputFile}" or die "Cannot open $test->{inputFile}: $!";
    my $parser = eval "\\&PrepareChangeLog::$test->{method}";
    my @ranges;
    my ($stdout, $stderr) = captureOutput(sub { @ranges = $parser->(\*FH, $test->{inputFile}); });
    close FH;
    $stdout = convertAbsolutepathToWebKitPath($stdout);
    $stderr = convertAbsolutepathToWebKitPath($stderr);

    my %actualOutput = (ranges => \@ranges, stdout => $stdout, stderr => $stderr);
    if ($resetResults) {
        open FH, "> $test->{expectedFile}" or die "Cannot open $test->{expectedFile}: $!";
        print FH Data::Dumper->new([\%actualOutput])->Terse(1)->Indent(1)->Dump();
        close FH;
        next;
    }

    open FH, "< $test->{expectedFile}" or die "Cannot open $test->{expectedFile}: $!";
    local $/ = undef;
    my $expectedOutput = eval <FH>;
    close FH;

    is_deeply(\%actualOutput, $expectedOutput, "Tests $test->{inputFile}");
}

sub captureOutput($)
{
    my ($targetMethod) = @_;

    my ($stdoutFH, $stdoutFileName) = tempfile();
    my ($stderrFH, $stderrFileName) = tempfile();

    open OLDSTDOUT, ">&", \*STDOUT or die "Cannot dup STDOUT: $!";
    open OLDSTDERR, ">&", \*STDERR or die "Cannot dup STDERR: $!";

    open STDOUT, ">&", $stdoutFH or die "Cannot redirect STDOUT: $!";
    open STDERR, ">&", $stderrFH or die "Cannot redirect STDERR: $!";

    &$targetMethod();

    close STDOUT;
    close STDERR;

    open STDOUT, ">&OLDSTDOUT" or die "Cannot dup OLDSTDOUT: $!";
    open STDERR, ">&OLDSTDERR" or die "Cannot dup OLDSTDERR: $!";

    close OLDSTDOUT;
    close OLDSTDERR;

    seek $stdoutFH, 0, 0;
    seek $stderrFH, 0, 0;
    local $/ = undef;
    my $stdout = <$stdoutFH>;
    my $stderr = <$stderrFH>;

    close $stdoutFH;
    close $stderrFH;

    unlink $stdoutFileName or die "Cannot unlink $stdoutFileName: $!";
    unlink $stderrFileName or die "Cannot unlink $stderrFileName: $!";
    return ($stdout, $stderr);
}

sub convertAbsolutepathToWebKitPath($)
{
    my $string = shift;
    my $sourceDir = LoadAsModule::sourceDir();
    $sourceDir .= "/" unless $sourceDir =~ m-/$-;
    $string =~ s/$sourceDir//g;
    return $string;
}
