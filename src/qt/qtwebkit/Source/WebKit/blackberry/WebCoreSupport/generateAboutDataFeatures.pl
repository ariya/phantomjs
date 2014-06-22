#!/usr/bin/perl -w

# Copyright (C) 2011 Research In Motion Limited. All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

use strict;

my $macro = shift;
my $inputFileName = shift;
my $outputFileName = shift;
my $input;
my $output;

open($input, '<', $inputFileName) or die "Can't open file for read: $inputFileName $!";
chop(my @features = <$input>);
close($input);

open($output, '>', $outputFileName) or die "Can't open file for write: $outputFileName $!";

print $output "// This is a generated fragment of about features for AboutData.cpp of the BlackBerry porting.\n";
print $output "// Don't edit me manually.\n\n";

foreach my $feature(@features) {
    print $output "#if " . $macro . "(" . $feature . ")\n";
    print $output "    trueList.append(\n";
    print $output "#else\n";
    print $output "    falseList.append(\n";
    print $output "#endif\n";
    print $output "    " . "\"" . $feature . "\");\n\n";
}

close($output);

