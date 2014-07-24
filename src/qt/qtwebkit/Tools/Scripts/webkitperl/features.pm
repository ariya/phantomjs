# Copyright (C) 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved
# Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
# Copyright (C) 2010 Andras Becsi (abecsi@inf.u-szeged.hu), University of Szeged
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

# Module to share code to detect the existance of features in built binaries.

use strict;
use warnings;

use FindBin;
use lib $FindBin::Bin;
use webkitdirs;

BEGIN {
   use Exporter   ();
   our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);
   $VERSION     = 1.00;
   @ISA         = qw(Exporter);
   @EXPORT      = qw(&checkWebCoreFeatureSupport);
   %EXPORT_TAGS = ( );
   @EXPORT_OK   = ();
}

sub libraryContainsSymbol($$)
{
    my ($path, $symbol) = @_;

    if (isCygwin() or isWindows()) {
        # FIXME: Implement this for Windows.
        return 0;
    }

    my $foundSymbol = 0;
    if (-e $path) {
        open NM, "-|", nmPath(), $path or die;
        while (<NM>) {
            $foundSymbol = 1 if /$symbol/; # FIXME: This should probably check for word boundaries before/after the symbol name.
        }
        close NM;
    }
    return $foundSymbol;
}

sub hasFeature($$)
{
    my ($featureName, $path) = @_;
    my %symbolForFeature = (
        "MathML" => "MathMLElement",
        "SVG" => "SVGDefsElement", # We used to look for SVGElement but isSVGElement exists (and would match) in --no-svg builds.
        "Accelerated Compositing" => "GraphicsLayer",
        "3D Rendering" => "WebCoreHas3DRendering",
        "3D Canvas" => "WebGLShader",
        "MHTML" => "MHTMLArchive"
    );
    my $symbolName = $symbolForFeature{$featureName};
    die "Unknown feature: $featureName" unless $symbolName;
    return libraryContainsSymbol($path, $symbolName);
}

sub checkWebCoreFeatureSupport($$)
{
    my ($feature, $required) = @_;
    my $libraryName = "WebCore";
    my $path = builtDylibPathForName($libraryName);
    my $hasFeature = hasFeature($feature, $path);
    if ($required && !$hasFeature) {
        die "$libraryName at \"$path\" does not include $hasFeature support.  See build-webkit --help\n";
    }
    return $hasFeature;
}

1;
