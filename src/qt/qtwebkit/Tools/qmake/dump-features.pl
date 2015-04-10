#!/usr/bin/perl -w

use strict;
use warnings;

use FeatureList;

foreach my $feature (getFeatureOptionList())
{
    next if $feature->{'option'} eq 'system-malloc';

    my $desc = $feature->{'desc'};
    $desc =~ s/^Toggle\s//;
    $desc =~ s/\ssupport$//;
    $desc = ucfirst($desc);

    $desc .= ' ' . '.' x (30 - length($desc));
    $desc =~ s/\s/_/g;

    my $define = $feature->{'define'};
    $define =~ s/^ENABLE_//;
    $define = lc($define);

    $desc .= "=$define";

    print "$desc "
}
