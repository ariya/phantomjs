#! /usr/bin/sed -nf

s/^#define  *FREETYPE_MAJOR  *\([^ ][^ ]*\).*$/freetype_major="\1" ;/p
s/^#define  *FREETYPE_MINOR  *\([^ ][^ ]*\).*$/freetype_minor=".\1" ;/p
s/^#define  *FREETYPE_PATCH  *\([^ ][^ ]*\).*$/freetype_patch=".\1" ;/p
