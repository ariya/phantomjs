#!/usr/bin/perl -w

use strict;

open MACHINE, "<" . $ARGV[0];
open OUTPUT, ">" . $ARGV[1];

my @undocumented = ();

print OUTPUT "<!-- Generated from Interpreter.cpp by make-bytecode-docs.pl. -->\n";
print OUTPUT "<style>p code \{ font-size: 14px; \}</style>\n";

while (<MACHINE>) {
    if (/^ *DEFINE_OPCODE/) {
        chomp;
        s/^ *DEFINE_OPCODE\(op_//;
        s/\).*$//;
        my $opcode = $_;
        $_ = <MACHINE>;
        chomp;
        if (m|/\* |) {
            my $format = $_;
            $format =~ s|.* /\* ||;
            my $doc = "";
            while (<MACHINE>) {
                if (m|\*/|) {
                    last;
                }
                $doc .= $_ . " ";
            }

            print OUTPUT "<h2><code>${opcode}</code></h2>\n<p><b>Format: </b><code>\n${format}\n</code></p>\n<p>\n${doc}\n</p>\n";
        } else {
            push @undocumented, $opcode;
        }
    }
}

close OUTPUT;

for my $undoc (@undocumented) {
    print "UNDOCUMENTED: ${undoc}\n";
}
