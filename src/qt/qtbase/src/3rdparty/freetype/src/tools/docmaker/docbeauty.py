#!/usr/bin/env python
#
#  DocBeauty (c) 2003, 2004, 2008 David Turner <david@freetype.org>
#
# This program is used to beautify the documentation comments used
# in the FreeType 2 public headers.
#

from sources import *
from content import *
from utils   import *

import utils

import sys, os, time, string, getopt


content_processor = ContentProcessor()


def  beautify_block( block ):
    if block.content:
        content_processor.reset()

        markups = content_processor.process_content( block.content )
        text    = []
        first   = 1

        for markup in markups:
            text.extend( markup.beautify( first ) )
            first = 0

        # now beautify the documentation "borders" themselves
        lines = [" /*************************************************************************"]
        for l in text:
            lines.append( "  *" + l )
        lines.append( "  */" )

        block.lines = lines


def  usage():
    print "\nDocBeauty 0.1 Usage information\n"
    print "  docbeauty [options] file1 [file2 ...]\n"
    print "using the following options:\n"
    print "  -h : print this page"
    print "  -b : backup original files with the 'orig' extension"
    print ""
    print "  --backup : same as -b"


def  main( argv ):
    """main program loop"""

    global output_dir

    try:
        opts, args = getopt.getopt( sys.argv[1:], \
                                    "hb",         \
                                    ["help", "backup"] )
    except getopt.GetoptError:
        usage()
        sys.exit( 2 )

    if args == []:
        usage()
        sys.exit( 1 )

    # process options
    #
    output_dir = None
    do_backup  = None

    for opt in opts:
        if opt[0] in ( "-h", "--help" ):
            usage()
            sys.exit( 0 )

        if opt[0] in ( "-b", "--backup" ):
            do_backup = 1

    # create context and processor
    source_processor = SourceProcessor()

    # retrieve the list of files to process
    file_list = make_file_list( args )
    for filename in file_list:
        source_processor.parse_file( filename )

        for block in source_processor.blocks:
            beautify_block( block )

        new_name = filename + ".new"
        ok       = None

        try:
            file = open( new_name, "wt" )
            for block in source_processor.blocks:
                for line in block.lines:
                    file.write( line )
                    file.write( "\n" )
            file.close()
        except:
            ok = 0


# if called from the command line
#
if __name__ == '__main__':
    main( sys.argv )


# eof
