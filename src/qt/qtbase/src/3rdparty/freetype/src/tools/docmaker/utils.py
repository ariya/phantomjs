#  Utils (c) 2002, 2004, 2007, 2008  David Turner <david@freetype.org>
#

import string, sys, os, glob

# current output directory
#
output_dir = None


# This function is used to sort the index.  It is a simple lexicographical
# sort, except that it places capital letters before lowercase ones.
#
def  index_sort( s1, s2 ):
    if not s1:
        return -1

    if not s2:
        return 1

    l1 = len( s1 )
    l2 = len( s2 )
    m1 = string.lower( s1 )
    m2 = string.lower( s2 )

    for i in range( l1 ):
        if i >= l2 or m1[i] > m2[i]:
            return 1

        if m1[i] < m2[i]:
            return -1

        if s1[i] < s2[i]:
            return -1

        if s1[i] > s2[i]:
            return 1

    if l2 > l1:
        return -1

    return 0


# Sort input_list, placing the elements of order_list in front.
#
def  sort_order_list( input_list, order_list ):
    new_list = order_list[:]
    for id in input_list:
        if not id in order_list:
            new_list.append( id )
    return new_list


# Open the standard output to a given project documentation file.  Use
# "output_dir" to determine the filename location if necessary and save the
# old stdout in a tuple that is returned by this function.
#
def  open_output( filename ):
    global output_dir

    if output_dir and output_dir != "":
        filename = output_dir + os.sep + filename

    old_stdout = sys.stdout
    new_file   = open( filename, "w" )
    sys.stdout = new_file

    return ( new_file, old_stdout )


# Close the output that was returned by "close_output".
#
def  close_output( output ):
    output[0].close()
    sys.stdout = output[1]


# Check output directory.
#
def  check_output():
    global output_dir
    if output_dir:
        if output_dir != "":
            if not os.path.isdir( output_dir ):
                sys.stderr.write( "argument" + " '" + output_dir + "' " + \
                                  "is not a valid directory" )
                sys.exit( 2 )
        else:
            output_dir = None


def  file_exists( pathname ):
    """checks that a given file exists"""
    result = 1
    try:
        file = open( pathname, "r" )
        file.close()
    except:
        result = None
        sys.stderr.write( pathname + " couldn't be accessed\n" )

    return result


def  make_file_list( args = None ):
    """builds a list of input files from command-line arguments"""
    file_list = []
    # sys.stderr.write( repr( sys.argv[1 :] ) + '\n' )

    if not args:
        args = sys.argv[1 :]

    for pathname in args:
        if string.find( pathname, '*' ) >= 0:
            newpath = glob.glob( pathname )
            newpath.sort()  # sort files -- this is important because
                            # of the order of files
        else:
            newpath = [pathname]

        file_list.extend( newpath )

    if len( file_list ) == 0:
        file_list = None
    else:
        # now filter the file list to remove non-existing ones
        file_list = filter( file_exists, file_list )

    return file_list

# eof
