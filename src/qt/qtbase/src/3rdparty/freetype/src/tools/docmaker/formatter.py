#  Formatter (c) 2002, 2004, 2007, 2008 David Turner <david@freetype.org>
#

from sources import *
from content import *
from utils   import *

# This is the base Formatter class.  Its purpose is to convert
# a content processor's data into specific documents (i.e., table of
# contents, global index, and individual API reference indices).
#
# You need to sub-class it to output anything sensible.  For example,
# the file tohtml.py contains the definition of the HtmlFormatter sub-class
# used to output -- you guessed it -- HTML.
#

class  Formatter:

    def  __init__( self, processor ):
        self.processor   = processor
        self.identifiers = {}
        self.chapters    = processor.chapters
        self.sections    = processor.sections.values()
        self.block_index = []

        # store all blocks in a dictionary
        self.blocks = []
        for section in self.sections:
            for block in section.blocks.values():
                self.add_identifier( block.name, block )

                # add enumeration values to the index, since this is useful
                for markup in block.markups:
                    if markup.tag == 'values':
                        for field in markup.fields:
                            self.add_identifier( field.name, block )

        self.block_index = self.identifiers.keys()
        self.block_index.sort( index_sort )

    def  add_identifier( self, name, block ):
        if self.identifiers.has_key( name ):
            # duplicate name!
            sys.stderr.write(                                           \
               "WARNING: duplicate definition for '" + name + "' in " + \
               block.location() + ", previous definition in " +         \
               self.identifiers[name].location() + "\n" )
        else:
            self.identifiers[name] = block

    #
    #  Formatting the table of contents
    #
    def  toc_enter( self ):
        pass

    def  toc_chapter_enter( self, chapter ):
        pass

    def  toc_section_enter( self, section ):
        pass

    def  toc_section_exit( self, section ):
        pass

    def  toc_chapter_exit( self, chapter ):
        pass

    def  toc_index( self, index_filename ):
        pass

    def  toc_exit( self ):
        pass

    def  toc_dump( self, toc_filename = None, index_filename = None ):
        output = None
        if toc_filename:
            output = open_output( toc_filename )

        self.toc_enter()

        for chap in self.processor.chapters:

            self.toc_chapter_enter( chap )

            for section in chap.sections:
                self.toc_section_enter( section )
                self.toc_section_exit( section )

            self.toc_chapter_exit( chap )

        self.toc_index( index_filename )

        self.toc_exit()

        if output:
            close_output( output )

    #
    #  Formatting the index
    #
    def  index_enter( self ):
        pass

    def  index_name_enter( self, name ):
        pass

    def  index_name_exit( self, name ):
        pass

    def  index_exit( self ):
        pass

    def  index_dump( self, index_filename = None ):
        output = None
        if index_filename:
            output = open_output( index_filename )

        self.index_enter()

        for name in self.block_index:
            self.index_name_enter( name )
            self.index_name_exit( name )

        self.index_exit()

        if output:
            close_output( output )

    #
    #  Formatting a section
    #
    def  section_enter( self, section ):
        pass

    def  block_enter( self, block ):
        pass

    def  markup_enter( self, markup, block = None ):
        pass

    def  field_enter( self, field, markup = None, block = None ):
        pass

    def  field_exit( self, field, markup = None, block = None ):
        pass

    def  markup_exit( self, markup, block = None ):
        pass

    def  block_exit( self, block ):
        pass

    def  section_exit( self, section ):
        pass

    def  section_dump( self, section, section_filename = None ):
        output = None
        if section_filename:
            output = open_output( section_filename )

        self.section_enter( section )

        for name in section.block_names:
            block = self.identifiers[name]
            self.block_enter( block )

            for markup in block.markups[1:]:   # always ignore first markup!
                self.markup_enter( markup, block )

                for field in markup.fields:
                    self.field_enter( field, markup, block )
                    self.field_exit( field, markup, block )

                self.markup_exit( markup, block )

            self.block_exit( block )

        self.section_exit( section )

        if output:
            close_output( output )

    def  section_dump_all( self ):
        for section in self.sections:
            self.section_dump( section )

# eof
