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
#

sub func1
{
}

sub func2
{
    return 123;
}

sub func3
{
    return 123;
    return 456;
}

sub func4()
{
}

sub func5($$$$)
{
}

sub func6(\@\@\$\$\$\$)
{
}

sub func7
{
    $str =<< EOF;

EOF
}

sub func8
{
    $str =<< "EOF";

EOF
}

sub func9
{
    $str =<< 'EOF';

EOF
}

sub func10
{
    $str =<< EOF;
sub funcInHereDocument1
{
}
EOF
}

sub func11
{
    $str =<< EOF;
sub funcInHereDocument2
{
}
sub funcInHereDocument3
{
}
EOF
}

sub func12
{
    $str =<< EOF;
{
{
{
}
}
}
EOF
}

sub func13
{
    $str =<< EOF;

$str << DUMMY_EOF

DUMMY_EOF

EOF
}

sub func14
{
    push(@array, << EOF);

EOF
}

sub func15
{
    print << EOF;

EOF
}

sub func16 {
}

sub prototypeDeclaration1;
sub prototypeDeclaration2();
sub prototypeDeclaration3(\@$$);

if (1) {
}

for (@array) {
}

{}

{
}
