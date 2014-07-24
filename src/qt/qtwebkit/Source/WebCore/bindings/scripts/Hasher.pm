# Copyright (C) 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
# Copyright (C) 2006 Anders Carlsson <andersca@mac.com>
# Copyright (C) 2006, 2007 Samuel Weinig <sam@webkit.org>
# Copyright (C) 2006 Alexey Proskuryakov <ap@webkit.org>
# Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
# Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
# Copyright (C) Research In Motion Limited 2010. All rights reserved.
# Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
# Copyright (C) 2011 Patrick Gansterer <paroga@webkit.org>
# Copyright (C) 2012 Ericsson AB. All rights reserved.
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

package Hasher;

use strict;

sub leftShift($$) {
    my ($value, $distance) = @_;
    return (($value << $distance) & 0xFFFFFFFF);
}

# Paul Hsieh's SuperFastHash
# http://www.azillionmonkeys.com/qed/hash.html
sub GenerateHashValue
{
    my @chars = split(/ */, $_[0]);

    # This hash is designed to work on 16-bit chunks at a time. But since the normal case
    # (above) is to hash UTF-16 characters, we just treat the 8-bit chars as if they
    # were 16-bit chunks, which should give matching results
    
    my $EXP2_32 = 4294967296;
    
    my $hash = 0x9e3779b9;
    my $l    = scalar @chars; #I wish this was in Ruby --- Maks
    my $rem  = $l & 1;
    $l = $l >> 1;
    
    my $s = 0;
    
    # Main loop
    for (; $l > 0; $l--) {
        $hash   += ord($chars[$s]);
        my $tmp = leftShift(ord($chars[$s+1]), 11) ^ $hash;
        $hash   = (leftShift($hash, 16)% $EXP2_32) ^ $tmp;
        $s += 2;
        $hash += $hash >> 11;
        $hash %= $EXP2_32;
    }
    
    # Handle end case
    if ($rem != 0) {
        $hash += ord($chars[$s]);
        $hash ^= (leftShift($hash, 11)% $EXP2_32);
        $hash += $hash >> 17;
    }
    
    # Force "avalanching" of final 127 bits
    $hash ^= leftShift($hash, 3);
    $hash += ($hash >> 5);
    $hash = ($hash% $EXP2_32);
    $hash ^= (leftShift($hash, 2)% $EXP2_32);
    $hash += ($hash >> 15);
    $hash = $hash% $EXP2_32;
    $hash ^= (leftShift($hash, 10)% $EXP2_32);
    
    # Save 8 bits for StringImpl to use as flags.
    $hash &= 0xffffff;
    
    # This avoids ever returning a hash code of 0, since that is used to
    # signal "hash not computed yet". Setting the high bit maintains
    # reasonable fidelity to a hash code of 0 because it is likely to yield
    # exactly 0 when hash lookup masks out the high bits.
    $hash = (0x80000000 >> 8) if ($hash == 0);
    
    return $hash;
}

1;
