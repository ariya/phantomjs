#  ToHTML (c) 2002, 2003, 2005, 2006, 2007, 2008
#    David Turner <david@freetype.org>

from sources import *
from content import *
from formatter import *

import time


# The following defines the HTML header used by all generated pages.
html_header_1 = """\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>\
"""

html_header_2 = """\
 API Reference</title>
<style type="text/css">
  body { font-family: Verdana, Geneva, Arial, Helvetica, serif;
         color: #000000;
         background: #FFFFFF; }

  p { text-align: justify; }
  h1 { text-align: center; }
  li { text-align: justify; }
  td { padding: 0 0.5em 0 0.5em; }
  td.left { padding: 0 0.5em 0 0.5em;
            text-align: left; }

  a:link { color: #0000EF; }
  a:visited { color: #51188E; }
  a:hover { color: #FF0000; }

  span.keyword { font-family: monospace;
                 text-align: left;
                 white-space: pre;
                 color: darkblue; }

  pre.colored { color: blue; }

  ul.empty { list-style-type: none; }
</style>
</head>
<body>
"""

html_header_3 = """
<table align=center><tr><td><font size=-1>[<a href="\
"""

html_header_3i = """
<table align=center><tr><td width="100%"></td>
<td><font size=-1>[<a href="\
"""

html_header_4 = """\
">Index</a>]</font></td>
<td width="100%"></td>
<td><font size=-1>[<a href="\
"""

html_header_5 = """\
">TOC</a>]</font></td></tr></table>
<center><h1>\
"""

html_header_5t = """\
">Index</a>]</font></td>
<td width="100%"></td></tr></table>
<center><h1>\
"""

html_header_6 = """\
 API Reference</h1></center>
"""


# The HTML footer used by all generated pages.
html_footer = """\
</body>
</html>\
"""

# The header and footer used for each section.
section_title_header = "<center><h1>"
section_title_footer = "</h1></center>"

# The header and footer used for code segments.
code_header = '<pre class="colored">'
code_footer = '</pre>'

# Paragraph header and footer.
para_header = "<p>"
para_footer = "</p>"

# Block header and footer.
block_header        = '<table align=center width="75%"><tr><td>'
block_footer_start  = """\
</td></tr></table>
<hr width="75%">
<table align=center width="75%"><tr><td><font size=-2>[<a href="\
"""
block_footer_middle = """\
">Index</a>]</font></td>
<td width="100%"></td>
<td><font size=-2>[<a href="\
"""
block_footer_end    = """\
">TOC</a>]</font></td></tr></table>
"""

# Description header/footer.
description_header = '<table align=center width="87%"><tr><td>'
description_footer = "</td></tr></table><br>"

# Marker header/inter/footer combination.
marker_header = '<table align=center width="87%" cellpadding=5><tr bgcolor="#EEEEFF"><td><em><b>'
marker_inter  = "</b></em></td></tr><tr><td>"
marker_footer = "</td></tr></table>"

# Header location header/footer.
header_location_header = '<table align=center width="87%"><tr><td>'
header_location_footer = "</td></tr></table><br>"

# Source code extracts header/footer.
source_header = '<table align=center width="87%"><tr bgcolor="#D6E8FF"><td><pre>\n'
source_footer = "\n</pre></table><br>"

# Chapter header/inter/footer.
chapter_header = '<br><table align=center width="75%"><tr><td><h2>'
chapter_inter  = '</h2><ul class="empty"><li>'
chapter_footer = '</li></ul></td></tr></table>'

# Index footer.
index_footer_start = """\
<hr>
<table><tr><td width="100%"></td>
<td><font size=-2>[<a href="\
"""
index_footer_end = """\
">TOC</a>]</font></td></tr></table>
"""

# TOC footer.
toc_footer_start = """\
<hr>
<table><tr><td><font size=-2>[<a href="\
"""
toc_footer_end = """\
">Index</a>]</font></td>
<td width="100%"></td>
</tr></table>
"""


# source language keyword coloration/styling
keyword_prefix = '<span class="keyword">'
keyword_suffix = '</span>'

section_synopsis_header = '<h2>Synopsis</h2>'
section_synopsis_footer = ''


# Translate a single line of source to HTML.  This will convert
# a "<" into "&lt.", ">" into "&gt.", etc.
def  html_quote( line ):
    result = string.replace( line, "&", "&amp;" )
    result = string.replace( result, "<", "&lt;" )
    result = string.replace( result, ">", "&gt;" )
    return result


# same as 'html_quote', but ignores left and right brackets
def  html_quote0( line ):
    return string.replace( line, "&", "&amp;" )


def  dump_html_code( lines, prefix = "" ):
    # clean the last empty lines
    l = len( self.lines )
    while l > 0 and string.strip( self.lines[l - 1] ) == "":
        l = l - 1

    # The code footer should be directly appended to the last code
    # line to avoid an additional blank line.
    print prefix + code_header,
    for line in self.lines[0 : l + 1]:
        print '\n' + prefix + html_quote( line ),
    print prefix + code_footer,



class  HtmlFormatter( Formatter ):

    def  __init__( self, processor, project_title, file_prefix ):
        Formatter.__init__( self, processor )

        global html_header_1, html_header_2, html_header_3
        global html_header_4, html_header_5, html_footer

        if file_prefix:
            file_prefix = file_prefix + "-"
        else:
            file_prefix = ""

        self.headers           = processor.headers
        self.project_title     = project_title
        self.file_prefix       = file_prefix
        self.html_header       = html_header_1 + project_title +              \
                                 html_header_2 +                              \
                                 html_header_3 + file_prefix + "index.html" + \
                                 html_header_4 + file_prefix + "toc.html" +   \
                                 html_header_5 + project_title +              \
                                 html_header_6

        self.html_index_header = html_header_1 + project_title +             \
                                 html_header_2 +                             \
                                 html_header_3i + file_prefix + "toc.html" + \
                                 html_header_5 + project_title +             \
                                 html_header_6

        self.html_toc_header   = html_header_1 + project_title +              \
                                 html_header_2 +                              \
                                 html_header_3 + file_prefix + "index.html" + \
                                 html_header_5t + project_title +             \
                                 html_header_6

        self.html_footer       = "<center><font size=""-2"">generated on " +     \
                                 time.asctime( time.localtime( time.time() ) ) + \
                                 "</font></center>" + html_footer

        self.columns = 3

    def  make_section_url( self, section ):
        return self.file_prefix + section.name + ".html"

    def  make_block_url( self, block ):
        return self.make_section_url( block.section ) + "#" + block.name

    def  make_html_words( self, words ):
        """ convert a series of simple words into some HTML text """
        line = ""
        if words:
            line = html_quote( words[0] )
            for w in words[1:]:
                line = line + " " + html_quote( w )

        return line

    def  make_html_word( self, word ):
        """analyze a simple word to detect cross-references and styling"""
        # look for cross-references
        m = re_crossref.match( word )
        if m:
            try:
                name = m.group( 1 )
                rest = m.group( 2 )
                block = self.identifiers[name]
                url   = self.make_block_url( block )
                return '<a href="' + url + '">' + name + '</a>' + rest
            except:
                # we detected a cross-reference to an unknown item
                sys.stderr.write( \
                   "WARNING: undefined cross reference '" + name + "'.\n" )
                return '?' + name + '?' + rest

        # look for italics and bolds
        m = re_italic.match( word )
        if m:
            name = m.group( 1 )
            rest = m.group( 3 )
            return '<i>' + name + '</i>' + rest

        m = re_bold.match( word )
        if m:
            name = m.group( 1 )
            rest = m.group( 3 )
            return '<b>' + name + '</b>' + rest

        return html_quote( word )

    def  make_html_para( self, words ):
        """ convert words of a paragraph into tagged HTML text, handle xrefs """
        line = ""
        if words:
            line = self.make_html_word( words[0] )
            for word in words[1:]:
                line = line + " " + self.make_html_word( word )
            # convert `...' quotations into real left and right single quotes
            line = re.sub( r"(^|\W)`(.*?)'(\W|$)",  \
                           r'\1&lsquo;\2&rsquo;\3', \
                           line )
            # convert tilde into non-breakable space
            line = string.replace( line, "~", "&nbsp;" )

        return para_header + line + para_footer

    def  make_html_code( self, lines ):
        """ convert a code sequence to HTML """
        line = code_header + '\n'
        for l in lines:
            line = line + html_quote( l ) + '\n'

        return line + code_footer

    def  make_html_items( self, items ):
        """ convert a field's content into some valid HTML """
        lines = []
        for item in items:
            if item.lines:
                lines.append( self.make_html_code( item.lines ) )
            else:
                lines.append( self.make_html_para( item.words ) )

        return string.join( lines, '\n' )

    def  print_html_items( self, items ):
        print self.make_html_items( items )

    def  print_html_field( self, field ):
        if field.name:
            print "<table><tr valign=top><td><b>" + field.name + "</b></td><td>"

        print self.make_html_items( field.items )

        if field.name:
            print "</td></tr></table>"

    def  html_source_quote( self, line, block_name = None ):
        result = ""
        while line:
            m = re_source_crossref.match( line )
            if m:
                name   = m.group( 2 )
                prefix = html_quote( m.group( 1 ) )
                length = len( m.group( 0 ) )

                if name == block_name:
                    # this is the current block name, if any
                    result = result + prefix + '<b>' + name + '</b>'
                elif re_source_keywords.match( name ):
                    # this is a C keyword
                    result = result + prefix + keyword_prefix + name + keyword_suffix
                elif self.identifiers.has_key( name ):
                    # this is a known identifier
                    block = self.identifiers[name]
                    result = result + prefix + '<a href="' + \
                             self.make_block_url( block ) + '">' + name + '</a>'
                else:
                    result = result + html_quote( line[:length] )

                line = line[length:]
            else:
                result = result + html_quote( line )
                line   = []

        return result

    def  print_html_field_list( self, fields ):
        print "<p></p>"
        print "<table cellpadding=3 border=0>"
        for field in fields:
            if len( field.name ) > 22:
              print "<tr valign=top><td colspan=0><b>" + field.name + "</b></td></tr>"
              print "<tr valign=top><td></td><td>"
            else:
              print "<tr valign=top><td><b>" + field.name + "</b></td><td>"

            self.print_html_items( field.items )
            print "</td></tr>"
        print "</table>"

    def  print_html_markup( self, markup ):
        table_fields = []
        for field in markup.fields:
            if field.name:
                # we begin a new series of field or value definitions, we
                # will record them in the 'table_fields' list before outputting
                # all of them as a single table
                #
                table_fields.append( field )
            else:
                if table_fields:
                    self.print_html_field_list( table_fields )
                    table_fields = []

                self.print_html_items( field.items )

        if table_fields:
            self.print_html_field_list( table_fields )

    #
    #  Formatting the index
    #
    def  index_enter( self ):
        print self.html_index_header
        self.index_items = {}

    def  index_name_enter( self, name ):
        block = self.identifiers[name]
        url   = self.make_block_url( block )
        self.index_items[name] = url

    def  index_exit( self ):
        # block_index already contains the sorted list of index names
        count = len( self.block_index )
        rows  = ( count + self.columns - 1 ) / self.columns

        print "<table align=center border=0 cellpadding=0 cellspacing=0>"
        for r in range( rows ):
            line = "<tr>"
            for c in range( self.columns ):
                i = r + c * rows
                if i < count:
                    bname = self.block_index[r + c * rows]
                    url   = self.index_items[bname]
                    line = line + '<td><a href="' + url + '">' + bname + '</a></td>'
                else:
                    line = line + '<td></td>'
            line = line + "</tr>"
            print line

        print "</table>"

        print index_footer_start +            \
              self.file_prefix + "toc.html" + \
              index_footer_end

        print self.html_footer

        self.index_items = {}

    def  index_dump( self, index_filename = None ):
        if index_filename == None:
            index_filename = self.file_prefix + "index.html"

        Formatter.index_dump( self, index_filename )

    #
    #  Formatting the table of content
    #
    def  toc_enter( self ):
        print self.html_toc_header
        print "<center><h1>Table of Contents</h1></center>"

    def  toc_chapter_enter( self, chapter ):
        print  chapter_header + string.join( chapter.title ) + chapter_inter
        print "<table cellpadding=5>"

    def  toc_section_enter( self, section ):
        print '<tr valign=top><td class="left">'
        print '<a href="' + self.make_section_url( section ) + '">' + \
               section.title + '</a></td><td>'

        print self.make_html_para( section.abstract )

    def  toc_section_exit( self, section ):
        print "</td></tr>"

    def  toc_chapter_exit( self, chapter ):
        print "</table>"
        print chapter_footer

    def  toc_index( self, index_filename ):
        print chapter_header +                                      \
              '<a href="' + index_filename + '">Global Index</a>' + \
              chapter_inter + chapter_footer

    def  toc_exit( self ):
        print toc_footer_start +                \
              self.file_prefix + "index.html" + \
              toc_footer_end

        print self.html_footer

    def  toc_dump( self, toc_filename = None, index_filename = None ):
        if toc_filename == None:
            toc_filename = self.file_prefix + "toc.html"

        if index_filename == None:
            index_filename = self.file_prefix + "index.html"

        Formatter.toc_dump( self, toc_filename, index_filename )

    #
    #  Formatting sections
    #
    def  section_enter( self, section ):
        print self.html_header

        print section_title_header
        print section.title
        print section_title_footer

        maxwidth = 0
        for b in section.blocks.values():
            if len( b.name ) > maxwidth:
                maxwidth = len( b.name )

        width = 70  # XXX magic number
        if maxwidth <> 0:
            # print section synopsis
            print section_synopsis_header
            print "<table align=center cellspacing=5 cellpadding=0 border=0>"

            columns = width / maxwidth
            if columns < 1:
                columns = 1

            count = len( section.block_names )
            rows  = ( count + columns - 1 ) / columns

            for r in range( rows ):
                line = "<tr>"
                for c in range( columns ):
                    i = r + c * rows
                    line = line + '<td></td><td>'
                    if i < count:
                        name = section.block_names[i]
                        line = line + '<a href="#' + name + '">' + name + '</a>'

                    line = line + '</td>'
                line = line + "</tr>"
                print line

            print "</table><br><br>"
            print section_synopsis_footer

        print description_header
        print self.make_html_items( section.description )
        print description_footer

    def  block_enter( self, block ):
        print block_header

        # place html anchor if needed
        if block.name:
            print '<h4><a name="' + block.name + '">' + block.name + '</a></h4>'

        # dump the block C source lines now
        if block.code:
            header = ''
            for f in self.headers.keys():
                if block.source.filename.find( f ) >= 0:
                    header = self.headers[f] + ' (' + f + ')'
                    break;
                
#           if not header:
#               sys.stderr.write( \
#                 'WARNING: No header macro for ' + block.source.filename + '.\n' )

            if header:
                print header_location_header
                print 'Defined in ' + header + '.'
                print header_location_footer

            print source_header
            for l in block.code:
                print self.html_source_quote( l, block.name )
            print source_footer

    def  markup_enter( self, markup, block ):
        if markup.tag == "description":
            print description_header
        else:
            print marker_header + markup.tag + marker_inter

        self.print_html_markup( markup )

    def  markup_exit( self, markup, block ):
        if markup.tag == "description":
            print description_footer
        else:
            print marker_footer

    def  block_exit( self, block ):
        print block_footer_start + self.file_prefix + "index.html" + \
              block_footer_middle + self.file_prefix + "toc.html" +  \
              block_footer_end

    def  section_exit( self, section ):
        print html_footer

    def  section_dump_all( self ):
        for section in self.sections:
            self.section_dump( section, self.file_prefix + section.name + '.html' )

# eof
