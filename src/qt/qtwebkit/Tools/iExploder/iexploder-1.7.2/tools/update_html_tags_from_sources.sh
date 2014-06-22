#!/bin/sh
#
# This script imports HTML and CSS tags from source trees. Supported browsers:
#
# * WebKit
# * Firefox
# * dillo
# * gtkhtml

src_dir=$1
tools_dir=`dirname $0`
dest_dir="$tools_dir/../src"
tmp_prefix="/tmp/$$"

if [ -z "$src_dir" ]; then
  echo "You must define a source directory to examine."
fi

if [ ! -d "$dest_dir" ]; then
  echo "Unable to find htdocs directory. Tried $dest_dir"
  exit 1
fi


if [ -d "$src_dir/WebKit" ]; then
  # Tested as of WebKit-r55454
  source_name="webkit"
  grep -v "^#" $src_dir/Source/WebCore/css/CSSPropertyNames.in > ${tmp_prefix}.css-properties
  grep -v "^#" $src_dir/Source/WebCore/css/CSSValueKeywords.in > ${tmp_prefix}.css-values
  grep -v "^#" $src_dir/Source/WebCore/html/HTMLAttributeNames.in | cut -d" " -f1  | \
    egrep -v "^namespace\w*=" > ${tmp_prefix}.html-attrs
  grep -v "^#" $src_dir/Source/WebCore/html/HTMLTagNames.in | cut -d" " -f1 | \
    egrep -v "^namespace\w*=" > ${tmp_prefix}.html-tags
  egrep "equalIgnoringCase" $src_dir/Source/WebCore/html/HTML*.cpp | \
    ruby -e '$stdin.readlines.join("").scan(/\"([\w-]+)"/) { |tag| puts tag }' > ${tmp_prefix}.html-values
  grep -r "protocolIs" $src_dir/Source/WebCore/* | ruby -e '$stdin.readlines.join("").scan(/\"([\w-]+)"/) { |tag| puts "#{tag}:" }' > ${tmp_prefix}.protocols
  grep "map->add" $src_dir/Source/WebCore/html/HTMLInputElement.cpp | cut -d\" -f2 >> ${tmp_prefix}.html-values
  grep "AtomicString,.*Header, (" $src_dir/Source/WebCore/platform/network/ResourceResponseBase.cpp | cut -d\" -f2 > ${tmp_prefix}.headers
  grep -o -r 'httpHeaderField(".*"' $src_dir  | cut -d\" -f2 >> ${tmp_prefix}.headers
  egrep -r '"[-\+a-z]+/[-\+a-z]+"' $src_dir/Source/WebCore | ruby -e '$stdin.readlines.join("").scan(/\"([afimtvwx][\w\+-]+\/[\w\+-]+)"/) { puts $1 }'  > ${tmp_prefix}.mime-types
  grep DEFINE_STATIC $src_dir/Source/WebCore/css/CSSSelector.cpp  | cut -d\" -f2 \
    > ${tmp_prefix}.css-pseudo
  egrep -o '"@.*?\"' $src_dir/Source/WebCore/css/CSSParser.cpp | cut -d\" -f2 | cut -d"{" -f1 | \
    sed s/" "//  > ${tmp_prefix}.css-atrules
elif [ -d "$src_dir/xpcom" ]; then
  # Tested as of Sep 1 2010 
  source_name="mozilla"
  grep "^HTML_.*TAG" $src_dir/parser/htmlparser/public/nsHTMLTagList.h \
    | cut -d\( -f2 | cut -d, -f1 | cut -d\) -f1 > ${tmp_prefix}.html-tags
  grep -r "Get.*Attr.*nsGkAtoms" $src_dir | perl -ne 'if (/nsGkAtoms::(\w+)/) { print "$1\n" } '\
    | xargs -n1 -I{} grep "({}," $src_dir/content/base/src/nsGkAtomList.h \
    | cut -d\" -f2 > ${tmp_prefix}.html-attrs
  grep "nsHtml5AttributeName.*nsHtml5Atoms::" $src_dir/parser/html/nsHtml5AttributeName.cpp  \
    | cut -d: -f3  | cut -d\) -f1 | cut -d, -f1  | xargs -n1 -I{} grep "({}," $src_dir/parser/html/nsHtml5AtomList.h \
    | cut -d\" -f2 >> ${tmp_prefix}.html-attrs
  egrep "^    [a-z-]+," $src_dir/layout/style/nsCSSPropList.h | cut -d, -f1 \
    | awk '{ print $1 }' > ${tmp_prefix}.css-properties
  grep 'CSS_KEY(' $src_dir/layout/style/nsCSSKeywordList.h | cut -d"(" -f2 \
    | cut -d, -f1 > ${tmp_prefix}.css-values
  egrep '{ "[a-z]+:' $src_dir/docshell/build/nsDocShellModule.cpp | cut -d\" -f2 \
    > ${tmp_prefix}.protocols
  grep -r 'aURI->SchemeIs("' $src_dir/* | cut -d\" -f2 | perl -ne 'chomp; print "$_:\n";' >> ${tmp_prefix}.protocols
  grep -r 'uri->SchemeIs("' $src_dir/* | cut -d\" -f2 | perl -ne 'chomp; print "$_:\n";' >> ${tmp_prefix}.protocols
  grep "{ \"" $src_dir/docshell/base/nsAboutRedirector.cpp | cut -d\" -f2 \
    | xargs -I{} echo "about:{}" >> ${tmp_prefix}.protocols
  grep targetScheme.EqualsLiteral $src_dir/netwerk/base/public/nsNetUtil.h \
    | cut -d\" -f2 | sed s/$/:/g>> ${tmp_prefix}.protocols
  grep "name.LowerCaseEqualsLiteral" $src_dir/docshell/base/nsDocShell.cpp | cut -d\" -f2 >> ${tmp_prefix}.html-values
  egrep '  { "[a-z]+' $src_dir/content/html/content/src/nsGenericHTMLElement.cpp | cut -d\" -f2 >> ${tmp_prefix}.html-values
  grep ' { "' $src_dir/content/html/content/src/nsHTMLInputElement.cpp | cut -d\" -f2 >> ${tmp_prefix}.html-values
  grep -r value.LowerCaseEqualsLiteral $src_dir/content/base/src/* | cut -d\" -f2 >> ${tmp_prefix}.html-values
  grep "^HTTP_ATOM" $src_dir/netwerk/protocol/http/nsHttpAtomList.h  | cut -d\" -f2 \
    | grep '[a-z]'>> ${tmp_prefix}.headers
  egrep -r '"[-\+a-z]+/[-\+a-z]+"' $src_dir/browser/base $src_dir/browser/components $src_dir/uriloader $src_dir/netwerk/mime $src_dir/content/html \
   | ruby -e '$stdin.readlines.join("").scan(/\"([afimtvwx][\w\+-]+\/[\w\+-]+)"/) { puts $1 }' > ${tmp_prefix}.mime-types
  egrep -o '":(.*?)"' $src_dir/layout/style/nsCSSPseudoClassList.h  | cut -d\" -f2 \
    | sed s/^:// > ${tmp_prefix}.css-pseudo
  grep AssignLiteral $src_dir/layout/style/nsCSSRules.cpp | egrep -o '"@.*?"' \
    | cut -d\" -f2 | cut -d" " -f1 > ${tmp_prefix}.css-atrules
elif [ -f "$src_dir/dillorc" ]; then
  # Tested as of dillo 2.2
  source_name="dillo"
  grep '{"' $src_dir/src/cssparser.cc  | cut -d\" -f2 > ${tmp_prefix}.css-properties
  egrep '^ +\"[a-z-]+\", ' $src_dir/src/cssparser.cc | \
    ruby -e '$stdin.readlines.join("").scan(/\"(.*?)\"/) { |tag| puts tag }' > ${tmp_prefix}.css-values
  grep "_get_attr(html" $src_dir/src/html.cc | grep '"' | cut -d\" -f2 > ${tmp_prefix}.html-attrs
  grep 'a_Html_get_attr(html.*"' $src_dir/src/*.cc | cut -d\" -f2 >> ${tmp_prefix}.html-attrs
  grep Html_tag_open_ $src_dir/src/html.cc  | grep "^ {" | cut -d\" -f2 > ${tmp_prefix}.html-tags
  grep dStrcasecmp $src_dir/src/form.cc $src_dir/src/html.cc $src_dir/src/table.cc | \
    ruby -e '$stdin.readlines.join("").scan(/\"([-a-z]+)\"/) { |tag| puts tag }' > ${tmp_prefix}.html-values
  grep -r 'URL_SCHEME.*"[a-z]' $src_dir | cut -d \" -f2 | perl -ne 'chomp; print "$_:\n";' > ${tmp_prefix}.protocols
  grep -r 'header, "' $src_dir/src/cache.c  | cut -d\" -f2 > ${tmp_prefix}.headers
  egrep -r "[-\+a-z]+/[-\+a-z]+" $src_dir/dpi $src_dir/src | \
    ruby -e '$stdin.readlines.join("").scan(/\"([\w\+-]+\/[\w\+-]+)"/) { puts $1 }' > ${tmp_prefix}.mime-types
elif [ -d "$src_dir/gtkhtml" ]; then
  # tested as of gtkhtml-3.29.91
  source_name="gtkhtml"
  grep -r "#define ID_" $src_dir/gtkhtml/htmlengine.c | cut -d\" -f2 | egrep '^[a-z]' > ${tmp_prefix}.html-tags
  grep "html_element_get_attr" $src_dir/gtkhtml/*.c | cut -d\" -f2 > ${tmp_prefix}.html-attrs
  grep -r "g_ascii_strncasecmp" $src_dir/gtkhtml/*.c | cut -d\" -f2 | grep -v ":" | cut -d"=" -f1 | grep "^[a-z]"  > ${tmp_prefix}.html-attrs
  grep "g_ascii_strncasecmp" $src_dir/gtkhtml/htmlstyle.c | cut -d\" -f2 | cut -d" " -f1 | sed s/://g > ${tmp_prefix}.css-properties
  grep "g_ascii_strcasecmp" $src_dir/gtkhtml/htmlstyle.c | cut -d\" -f2 > ${tmp_prefix}.css-values
  grep g_ascii_strcasecmp $src_dir/gtkhtml/htmlengine.c |  ruby -e '$stdin.readlines.join("").scan(/\"([\/\w-]+)"/) { |tag| puts tag }' > ${tmp_prefix}.html-values
fi

if [ "$source_name" ]; then
  echo "Updating $source_name"
  # We always append, never remove.
  types="css-properties css-values html-attrs html-tags html-values protocols headers mime-types css-pseudo css-atrules" 
  for type in $types
  do
    if [ -f "${tmp_prefix}.${type}" ]; then
      if [ -s "${tmp_prefix}.${type}" ]; then
        echo "- $type"
        cat $dest_dir/$type/$source_name ${tmp_prefix}.${type} | sort -u > $dest_dir/$type/$source_name
      else
        echo "- Unable to parse ${type}, source code is incompatible (skipping)"
      fi
      rm -f "${tmp_prefix}.${type}"
    fi
  done
else
  echo "Could not identify the correct source type for $src_dir"
fi

