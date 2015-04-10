#  Sources (c) 2002, 2003, 2004, 2006, 2007, 2008, 2009
#    David Turner <david@freetype.org>
#
#
# this file contains definitions of classes needed to decompose
# C sources files into a series of multi-line "blocks". There are
# two kinds of blocks:
#
#   - normal blocks, which contain source code or ordinary comments
#
#   - documentation blocks, which have restricted formatting, and
#     whose text always start with a documentation markup tag like
#     "<Function>", "<Type>", etc..
#
# the routines used to process the content of documentation blocks
# are not contained here, but in "content.py"
#
# the classes and methods found here only deal with text parsing
# and basic documentation block extraction
#

import fileinput, re, sys, os, string



################################################################
##
##  BLOCK FORMAT PATTERN
##
##   A simple class containing compiled regular expressions used
##   to detect potential documentation format block comments within
##   C source code
##
##   note that the 'column' pattern must contain a group that will
##   be used to "unbox" the content of documentation comment blocks
##
class  SourceBlockFormat:

    def  __init__( self, id, start, column, end ):
        """create a block pattern, used to recognize special documentation blocks"""
        self.id     = id
        self.start  = re.compile( start, re.VERBOSE )
        self.column = re.compile( column, re.VERBOSE )
        self.end    = re.compile( end, re.VERBOSE )



#
# format 1 documentation comment blocks look like the following:
#
#    /************************************/
#    /*                                  */
#    /*                                  */
#    /*                                  */
#    /************************************/
#
# we define a few regular expressions here to detect them
#

start = r'''
  \s*      # any number of whitespace
  /\*{2,}/ # followed by '/' and at least two asterisks then '/'
  \s*$     # probably followed by whitespace
'''

column = r'''
  \s*      # any number of whitespace
  /\*{1}   # followed by '/' and precisely one asterisk
  ([^*].*) # followed by anything (group 1)
  \*{1}/   # followed by one asterisk and a '/'
  \s*$     # probably followed by whitespace
'''

re_source_block_format1 = SourceBlockFormat( 1, start, column, start )


#
# format 2 documentation comment blocks look like the following:
#
#    /************************************ (at least 2 asterisks)
#     *
#     *
#     *
#     *
#     **/       (1 or more asterisks at the end)
#
# we define a few regular expressions here to detect them
#
start = r'''
  \s*     # any number of whitespace
  /\*{2,} # followed by '/' and at least two asterisks
  \s*$    # probably followed by whitespace
'''

column = r'''
  \s*        # any number of whitespace
  \*{1}(?!/) # followed by precisely one asterisk not followed by `/'
  (.*)       # then anything (group1)
'''

end = r'''
  \s*  # any number of whitespace
  \*+/ # followed by at least one asterisk, then '/'
'''

re_source_block_format2 = SourceBlockFormat( 2, start, column, end )


#
# the list of supported documentation block formats, we could add new ones
# relatively easily
#
re_source_block_formats = [re_source_block_format1, re_source_block_format2]


#
# the following regular expressions corresponds to markup tags
# within the documentation comment blocks. they're equivalent
# despite their different syntax
#
# notice how each markup tag _must_ begin a new line
#
re_markup_tag1 = re.compile( r'''\s*<(\w*)>''' )  # <xxxx> format
re_markup_tag2 = re.compile( r'''\s*@(\w*):''' )  # @xxxx: format

#
# the list of supported markup tags, we could add new ones relatively
# easily
#
re_markup_tags = [re_markup_tag1, re_markup_tag2]

#
# used to detect a cross-reference, after markup tags have been stripped
#
re_crossref = re.compile( r'@(\w*)(.*)' )

#
# used to detect italic and bold styles in paragraph text
#
re_italic = re.compile( r"_(\w(\w|')*)_(.*)" )     #  _italic_
re_bold   = re.compile( r"\*(\w(\w|')*)\*(.*)" )   #  *bold*

#
# used to detect the end of commented source lines
#
re_source_sep = re.compile( r'\s*/\*\s*\*/' )

#
# used to perform cross-reference within source output
#
re_source_crossref = re.compile( r'(\W*)(\w*)' )

#
# a list of reserved source keywords
#
re_source_keywords = re.compile( '''\\b ( typedef   |
                                          struct    |
                                          enum      |
                                          union     |
                                          const     |
                                          char      |
                                          int       |
                                          short     |
                                          long      |
                                          void      |
                                          signed    |
                                          unsigned  |
                                          \#include |
                                          \#define  |
                                          \#undef   |
                                          \#if      |
                                          \#ifdef   |
                                          \#ifndef  |
                                          \#else    |
                                          \#endif   ) \\b''', re.VERBOSE )


################################################################
##
##  SOURCE BLOCK CLASS
##
##   A SourceProcessor is in charge of reading a C source file
##   and decomposing it into a series of different "SourceBlocks".
##   each one of these blocks can be made of the following data:
##
##   - A documentation comment block that starts with "/**" and
##     whose exact format will be discussed later
##
##   - normal sources lines, including comments
##
##   the important fields in a text block are the following ones:
##
##     self.lines   : a list of text lines for the corresponding block
##
##     self.content : for documentation comment blocks only, this is the
##                    block content that has been "unboxed" from its
##                    decoration. This is None for all other blocks
##                    (i.e. sources or ordinary comments with no starting
##                     markup tag)
##
class  SourceBlock:

    def  __init__( self, processor, filename, lineno, lines ):
        self.processor = processor
        self.filename  = filename
        self.lineno    = lineno
        self.lines     = lines[:]
        self.format    = processor.format
        self.content   = []

        if self.format == None:
            return

        words = []

        # extract comment lines
        lines = []

        for line0 in self.lines:
            m = self.format.column.match( line0 )
            if m:
                lines.append( m.group( 1 ) )

        # now, look for a markup tag
        for l in lines:
            l = string.strip( l )
            if len( l ) > 0:
                for tag in re_markup_tags:
                    if tag.match( l ):
                        self.content = lines
                        return

    def  location( self ):
        return "(" + self.filename + ":" + repr( self.lineno ) + ")"

    # debugging only - not used in normal operations
    def  dump( self ):
        if self.content:
            print "{{{content start---"
            for l in self.content:
                print l
            print "---content end}}}"
            return

        fmt = ""
        if self.format:
            fmt = repr( self.format.id ) + " "

        for line in self.lines:
            print line



################################################################
##
##  SOURCE PROCESSOR CLASS
##
##   The SourceProcessor is in charge of reading a C source file
##   and decomposing it into a series of different "SourceBlock"
##   objects.
##
##   each one of these blocks can be made of the following data:
##
##   - A documentation comment block that starts with "/**" and
##     whose exact format will be discussed later
##
##   - normal sources lines, include comments
##
##
class  SourceProcessor:

    def  __init__( self ):
        """initialize a source processor"""
        self.blocks   = []
        self.filename = None
        self.format   = None
        self.lines    = []

    def  reset( self ):
        """reset a block processor, clean all its blocks"""
        self.blocks = []
        self.format = None

    def  parse_file( self, filename ):
        """parse a C source file, and add its blocks to the processor's list"""
        self.reset()

        self.filename = filename

        fileinput.close()
        self.format = None
        self.lineno = 0
        self.lines  = []

        for line in fileinput.input( filename ):
            # strip trailing newlines, important on Windows machines!
            if line[-1] == '\012':
                line = line[0:-1]

            if self.format == None:
                self.process_normal_line( line )
            else:
                if self.format.end.match( line ):
                    # that's a normal block end, add it to 'lines' and
                    # create a new block
                    self.lines.append( line )
                    self.add_block_lines()
                elif self.format.column.match( line ):
                    # that's a normal column line, add it to 'lines'
                    self.lines.append( line )
                else:
                    # humm.. this is an unexpected block end,
                    # create a new block, but don't process the line
                    self.add_block_lines()

                    # we need to process the line again
                    self.process_normal_line( line )

        # record the last lines
        self.add_block_lines()

    def  process_normal_line( self, line ):
        """process a normal line and check whether it is the start of a new block"""
        for f in re_source_block_formats:
            if f.start.match( line ):
                self.add_block_lines()
                self.format = f
                self.lineno = fileinput.filelineno()

        self.lines.append( line )

    def  add_block_lines( self ):
        """add the current accumulated lines and create a new block"""
        if self.lines != []:
            block = SourceBlock( self, self.filename, self.lineno, self.lines )

            self.blocks.append( block )
            self.format = None
            self.lines  = []

    # debugging only, not used in normal operations
    def  dump( self ):
        """print all blocks in a processor"""
        for b in self.blocks:
            b.dump()

# eof
