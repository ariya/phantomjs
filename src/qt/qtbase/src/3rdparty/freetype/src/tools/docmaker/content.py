#  Content (c) 2002, 2004, 2006, 2007, 2008, 2009
#    David Turner <david@freetype.org>
#
#  This file contains routines used to parse the content of documentation
#  comment blocks and build more structured objects out of them.
#

from sources import *
from utils import *
import string, re


# this regular expression is used to detect code sequences. these
# are simply code fragments embedded in '{' and '}' like in:
#
#  {
#    x = y + z;
#    if ( zookoo == 2 )
#    {
#      foobar();
#    }
#  }
#
# note that indentation of the starting and ending accolades must be
# exactly the same. the code sequence can contain accolades at greater
# indentation
#
re_code_start = re.compile( r"(\s*){\s*$" )
re_code_end   = re.compile( r"(\s*)}\s*$" )


# this regular expression is used to isolate identifiers from
# other text
#
re_identifier = re.compile( r'(\w*)' )


# we collect macros ending in `_H'; while outputting the object data, we use
# this info together with the object's file location to emit the appropriate
# header file macro and name before the object itself
#
re_header_macro = re.compile( r'^#define\s{1,}(\w{1,}_H)\s{1,}<(.*)>' )


#############################################################################
#
# The DocCode class is used to store source code lines.
#
#   'self.lines' contains a set of source code lines that will be dumped as
#   HTML in a <PRE> tag.
#
#   The object is filled line by line by the parser; it strips the leading
#   "margin" space from each input line before storing it in 'self.lines'.
#
class  DocCode:

    def  __init__( self, margin, lines ):
        self.lines = []
        self.words = None

        # remove margin spaces
        for l in lines:
            if string.strip( l[:margin] ) == "":
                l = l[margin:]
            self.lines.append( l )

    def  dump( self, prefix = "", width = 60 ):
        lines = self.dump_lines( 0, width )
        for l in lines:
            print prefix + l

    def  dump_lines( self, margin = 0, width = 60 ):
        result = []
        for l in self.lines:
            result.append( " " * margin + l )
        return result



#############################################################################
#
# The DocPara class is used to store "normal" text paragraph.
#
#   'self.words' contains the list of words that make up the paragraph
#
class  DocPara:

    def  __init__( self, lines ):
        self.lines = None
        self.words = []
        for l in lines:
            l = string.strip( l )
            self.words.extend( string.split( l ) )

    def  dump( self, prefix = "", width = 60 ):
        lines = self.dump_lines( 0, width )
        for l in lines:
            print prefix + l

    def  dump_lines( self, margin = 0, width = 60 ):
        cur    = ""  # current line
        col    = 0   # current width
        result = []

        for word in self.words:
            ln = len( word )
            if col > 0:
                ln = ln + 1

            if col + ln > width:
                result.append( " " * margin + cur )
                cur = word
                col = len( word )
            else:
                if col > 0:
                    cur = cur + " "
                cur = cur + word
                col = col + ln

        if col > 0:
            result.append( " " * margin + cur )

        return result



#############################################################################
#
#  The DocField class is used to store a list containing either DocPara or
#  DocCode objects. Each DocField also has an optional "name" which is used
#  when the object corresponds to a field or value definition
#
class  DocField:

    def  __init__( self, name, lines ):
        self.name  = name  # can be None for normal paragraphs/sources
        self.items = []    # list of items

        mode_none  = 0     # start parsing mode
        mode_code  = 1     # parsing code sequences
        mode_para  = 3     # parsing normal paragraph

        margin     = -1    # current code sequence indentation
        cur_lines  = []

        # now analyze the markup lines to see if they contain paragraphs,
        # code sequences or fields definitions
        #
        start = 0
        mode  = mode_none

        for l in lines:
            # are we parsing a code sequence ?
            if mode == mode_code:
                m = re_code_end.match( l )
                if m and len( m.group( 1 ) ) <= margin:
                    # that's it, we finished the code sequence
                    code = DocCode( 0, cur_lines )
                    self.items.append( code )
                    margin    = -1
                    cur_lines = []
                    mode      = mode_none
                else:
                    # nope, continue the code sequence
                    cur_lines.append( l[margin:] )
            else:
                # start of code sequence ?
                m = re_code_start.match( l )
                if m:
                    # save current lines
                    if cur_lines:
                        para = DocPara( cur_lines )
                        self.items.append( para )
                        cur_lines = []

                    # switch to code extraction mode
                    margin = len( m.group( 1 ) )
                    mode   = mode_code
                else:
                    if not string.split( l ) and cur_lines:
                        # if the line is empty, we end the current paragraph,
                        # if any
                        para = DocPara( cur_lines )
                        self.items.append( para )
                        cur_lines = []
                    else:
                        # otherwise, simply add the line to the current
                        # paragraph
                        cur_lines.append( l )

        if mode == mode_code:
            # unexpected end of code sequence
            code = DocCode( margin, cur_lines )
            self.items.append( code )
        elif cur_lines:
            para = DocPara( cur_lines )
            self.items.append( para )

    def  dump( self, prefix = "" ):
        if self.field:
            print prefix + self.field + " ::"
            prefix = prefix + "----"

        first = 1
        for p in self.items:
            if not first:
                print ""
            p.dump( prefix )
            first = 0

    def  dump_lines( self, margin = 0, width = 60 ):
        result = []
        nl     = None

        for p in self.items:
            if nl:
                result.append( "" )

            result.extend( p.dump_lines( margin, width ) )
            nl = 1

        return result



# this regular expression is used to detect field definitions
#
re_field = re.compile( r"\s*(\w*|\w(\w|\.)*\w)\s*::" )



class  DocMarkup:

    def  __init__( self, tag, lines ):
        self.tag    = string.lower( tag )
        self.fields = []

        cur_lines = []
        field     = None
        mode      = 0

        for l in lines:
            m = re_field.match( l )
            if m:
                # we detected the start of a new field definition

                # first, save the current one
                if cur_lines:
                    f = DocField( field, cur_lines )
                    self.fields.append( f )
                    cur_lines = []
                    field     = None

                field     = m.group( 1 )   # record field name
                ln        = len( m.group( 0 ) )
                l         = " " * ln + l[ln:]
                cur_lines = [l]
            else:
                cur_lines.append( l )

        if field or cur_lines:
            f = DocField( field, cur_lines )
            self.fields.append( f )

    def  get_name( self ):
        try:
            return self.fields[0].items[0].words[0]
        except:
            return None

    def  get_start( self ):
        try:
            result = ""
            for word in self.fields[0].items[0].words:
                result = result + " " + word
            return result[1:]
        except:
            return "ERROR"

    def  dump( self, margin ):
        print " " * margin + "<" + self.tag + ">"
        for f in self.fields:
            f.dump( "  " )
        print " " * margin + "</" + self.tag + ">"



class  DocChapter:

    def  __init__( self, block ):
        self.block    = block
        self.sections = []
        if block:
            self.name  = block.name
            self.title = block.get_markup_words( "title" )
            self.order = block.get_markup_words( "sections" )
        else:
            self.name  = "Other"
            self.title = string.split( "Miscellaneous" )
            self.order = []



class  DocSection:

    def  __init__( self, name = "Other" ):
        self.name        = name
        self.blocks      = {}
        self.block_names = []  # ordered block names in section
        self.defs        = []
        self.abstract    = ""
        self.description = ""
        self.order       = []
        self.title       = "ERROR"
        self.chapter     = None

    def  add_def( self, block ):
        self.defs.append( block )

    def  add_block( self, block ):
        self.block_names.append( block.name )
        self.blocks[block.name] = block

    def  process( self ):
        # look up one block that contains a valid section description
        for block in self.defs:
            title = block.get_markup_text( "title" )
            if title:
                self.title       = title
                self.abstract    = block.get_markup_words( "abstract" )
                self.description = block.get_markup_items( "description" )
                self.order       = block.get_markup_words( "order" )
                return

    def  reorder( self ):
        self.block_names = sort_order_list( self.block_names, self.order )



class  ContentProcessor:

    def  __init__( self ):
        """initialize a block content processor"""
        self.reset()

        self.sections = {}    # dictionary of documentation sections
        self.section  = None  # current documentation section

        self.chapters = []    # list of chapters

        self.headers  = {}    # dictionary of header macros

    def  set_section( self, section_name ):
        """set current section during parsing"""
        if not self.sections.has_key( section_name ):
            section = DocSection( section_name )
            self.sections[section_name] = section
            self.section                = section
        else:
            self.section = self.sections[section_name]

    def  add_chapter( self, block ):
        chapter = DocChapter( block )
        self.chapters.append( chapter )


    def  reset( self ):
        """reset the content processor for a new block"""
        self.markups      = []
        self.markup       = None
        self.markup_lines = []

    def  add_markup( self ):
        """add a new markup section"""
        if self.markup and self.markup_lines:

            # get rid of last line of markup if it's empty
            marks = self.markup_lines
            if len( marks ) > 0 and not string.strip( marks[-1] ):
                self.markup_lines = marks[:-1]

            m = DocMarkup( self.markup, self.markup_lines )

            self.markups.append( m )

            self.markup       = None
            self.markup_lines = []

    def  process_content( self, content ):
        """process a block content and return a list of DocMarkup objects
           corresponding to it"""
        markup       = None
        markup_lines = []
        first        = 1

        for line in content:
            found = None
            for t in re_markup_tags:
                m = t.match( line )
                if m:
                    found  = string.lower( m.group( 1 ) )
                    prefix = len( m.group( 0 ) )
                    line   = " " * prefix + line[prefix:]   # remove markup from line
                    break

            # is it the start of a new markup section ?
            if found:
                first = 0
                self.add_markup()  # add current markup content
                self.markup = found
                if len( string.strip( line ) ) > 0:
                    self.markup_lines.append( line )
            elif first == 0:
                self.markup_lines.append( line )

        self.add_markup()

        return self.markups

    def  parse_sources( self, source_processor ):
        blocks = source_processor.blocks
        count  = len( blocks )

        for n in range( count ):
            source = blocks[n]
            if source.content:
                # this is a documentation comment, we need to catch
                # all following normal blocks in the "follow" list
                #
                follow = []
                m = n + 1
                while m < count and not blocks[m].content:
                    follow.append( blocks[m] )
                    m = m + 1

                doc_block = DocBlock( source, follow, self )

    def  finish( self ):
        # process all sections to extract their abstract, description
        # and ordered list of items
        #
        for sec in self.sections.values():
            sec.process()

        # process chapters to check that all sections are correctly
        # listed there
        for chap in self.chapters:
            for sec in chap.order:
                if self.sections.has_key( sec ):
                    section = self.sections[sec]
                    section.chapter = chap
                    section.reorder()
                    chap.sections.append( section )
                else:
                    sys.stderr.write( "WARNING: chapter '" +          \
                        chap.name + "' in " + chap.block.location() + \
                        " lists unknown section '" + sec + "'\n" )

        # check that all sections are in a chapter
        #
        others = []
        for sec in self.sections.values():
            if not sec.chapter:
                others.append( sec )

        # create a new special chapter for all remaining sections
        # when necessary
        #
        if others:
            chap = DocChapter( None )
            chap.sections = others
            self.chapters.append( chap )



class  DocBlock:

    def  __init__( self, source, follow, processor ):
        processor.reset()

        self.source  = source
        self.code    = []
        self.type    = "ERRTYPE"
        self.name    = "ERRNAME"
        self.section = processor.section
        self.markups = processor.process_content( source.content )

        # compute block type from first markup tag
        try:
            self.type = self.markups[0].tag
        except:
            pass

        # compute block name from first markup paragraph
        try:
            markup = self.markups[0]
            para   = markup.fields[0].items[0]
            name   = para.words[0]
            m = re_identifier.match( name )
            if m:
                name = m.group( 1 )
            self.name = name
        except:
            pass

        if self.type == "section":
            # detect new section starts
            processor.set_section( self.name )
            processor.section.add_def( self )
        elif self.type == "chapter":
            # detect new chapter
            processor.add_chapter( self )
        else:
            processor.section.add_block( self )

        # now, compute the source lines relevant to this documentation
        # block. We keep normal comments in for obvious reasons (??)
        source = []
        for b in follow:
            if b.format:
                break
            for l in b.lines:
                # collect header macro definitions
                m = re_header_macro.match( l )
                if m:
                    processor.headers[m.group( 2 )] = m.group( 1 );

                # we use "/* */" as a separator
                if re_source_sep.match( l ):
                    break
                source.append( l )

        # now strip the leading and trailing empty lines from the sources
        start = 0
        end   = len( source ) - 1

        while start < end and not string.strip( source[start] ):
            start = start + 1

        while start < end and not string.strip( source[end] ):
            end = end - 1

        if start == end and not string.strip( source[start] ):
            self.code = []
        else:
            self.code = source[start:end + 1]

    def  location( self ):
        return self.source.location()

    def  get_markup( self, tag_name ):
        """return the DocMarkup corresponding to a given tag in a block"""
        for m in self.markups:
            if m.tag == string.lower( tag_name ):
                return m
        return None

    def  get_markup_name( self, tag_name ):
        """return the name of a given primary markup in a block"""
        try:
            m = self.get_markup( tag_name )
            return m.get_name()
        except:
            return None

    def  get_markup_words( self, tag_name ):
        try:
            m = self.get_markup( tag_name )
            return m.fields[0].items[0].words
        except:
            return []

    def  get_markup_text( self, tag_name ):
        result = self.get_markup_words( tag_name )
        return string.join( result )

    def  get_markup_items( self, tag_name ):
        try:
            m = self.get_markup( tag_name )
            return m.fields[0].items
        except:
            return None

# eof
