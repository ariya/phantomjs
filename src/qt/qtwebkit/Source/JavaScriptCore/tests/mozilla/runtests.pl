#!/tools/ns/bin/perl5
#
# simple script that executes JavaScript tests.  you have to build the
# stand-alone, js shell executable (which is not the same as the dll that gets
# built for mozilla).  see the readme at
# http://lxr.mozilla.org/mozilla/source/js/src/README.html for instructions on
# how to build the jsshell.
#
# this is just a quick-n-dirty script.  for full reporting, you need to run
# the test driver, which requires java and is currently not available on
# mozilla.org.
#
# this test looks for an executable JavaScript shell in
# %MOZ_SRC/mozilla/js/src/[platform]-[platform-version]-OPT.OBJ/js,
# which is the default build location when you build using the instructions
# at http://lxr.mozilla.org/mozilla/source/js/src/README.html
#
#
# christine@netscape.com
#

&parse_args;
&setup_env;
&main_test_loop;
&cleanup_env;

#
# given a main directory, assume that there is a file called 'shell.js'
# in it.  then, open all the subdirectories, and look for js files.
# for each test.js that is found, execute the shell, and pass shell.js
# and the test.js as file arguments.  redirect all process output to a
# file.
#
sub main_test_loop {
    foreach $suite ( &get_subdirs( $test_dir )) {
        foreach $subdir (&get_subdirs( $suite, $test_dir )) {
            @jsfiles = &get_js_files($subdir);
            execute_js_tests(@jsfiles);
        }
    }
}

#
# given a directory, return an array of all subdirectories
#
sub get_subdirs{
    local ($dir, $path)  = @_;
    local @subdirs;

    local $dir_path = $path . $dir;
    chdir $dir_path;

    opendir ( DIR, ${dir_path} );
    local @testdir_contents = readdir( DIR );
    closedir( DIR );

    foreach (@testdir_contents) {
        if ( (-d $_) && ($_ !~ 'CVS') && ( $_ ne '.') && ($_ ne '..')) {
            @subdirs[$#subdirs+1] = $_;
        }
    }
    chdir $path;
    return @subdirs;
}

#
# given a directory, return an array of all the js files that are in it.
#
sub get_js_files {
    ( $test_subdir ) = @_;
    local @js_file_array;

    $current_test_dir = $test_dir  ."/". $suite . "/" .$test_subdir;
    chdir $current_test_dir;

    opendir ( TEST_SUBDIR, ${current_test_dir} );
    @subdir_files = readdir( TEST_SUBDIR );
    closedir( TOP_LEVEL_BUILD_DIR );

    foreach ( @subdir_files ) {
        if ( $_ =~ /\.js$/ ) {
            $js_file_array[$#js_file_array+1] = $_;
        }
    }

    return @js_file_array;
}

#
# given an array of test.js files, execute the shell command and pass
# the shell.js and test.js files as file arguments.  redirect process
# output to a file.  if $js_verbose is set (not recommended), write all
# testcase output to the output file.  if $js_quiet is set, only write
# failed test case information to the output file.  the default setting
# is to write a line for each test file, and whether each file passed
# or failed.
#
sub execute_js_tests {
    (@js_file_array) = @_;

    $js_printed_suitename = 0;
    if ( !$js_quiet ) {
        &js_print_suitename;
    }

    foreach $js_test (@js_file_array) {
        $js_printed_filename = 0;
        $js_test_bugnumber = 0;
        $runtime_error = "";

        local $passed = -1;

        # create the test command
        $test_command =
            $shell_command .
            " -f $test_dir/$suite/shell.js " .
            " -f $test_dir/$suite/$subdir/$js_test";

        if ( !$js_quiet ) {
            &js_print_filename;
        } else {
            print '.';
        }

        $test_path = $test_dir ."/" . $suite ."/". $test_subdir ."/". $js_test;


        if ( !-e $test_path ) {
            &js_print( " FAILED! file not found\n",
                "<font color=#990000>", "</font><br>\n");
        } else {
            open( RUNNING_TEST,  "$test_command" . ' 2>&1 |');


			# this is where we want the tests to provide a lot more information
			# that this script must parse so that we can  

        	while( <RUNNING_TEST> ){
    	        if ( $js_verbose && !$js_quiet ) {
    	            &js_print ($_ ."\n", "", "<br>\n");
                }
                if ( $_ =~ /BUGNUMBER/ ) {
                    $js_test_bugnumber = $_;
                }
				if ( $_ =~ /PASSED/ && $passed == -1 ) {
					$passed = 1;
				}
                if ( $_ =~ /FAILED/ && $_ =~ /expected/) {
                    &js_print_suitename;
                    &js_print_filename;
                    &js_print_bugnumber;

                    local @msg = split ( "FAILED", $_ );
                    &js_print ( $passed ? "\n" : "" );
                    &js_print( "    " . $msg[0], "&nbsp;&nbsp;<tt>" );
                    &js_print( "FAILED", "<font color=#990000>", "</font>");
                    &js_print( $msg[1], "", "</tt><br>\n" );
                    $passed = 0;
                }
                if ( $_ =~ /$js_test/ ) {
                    $runtime_error .= $_;
                }
    	    }
    	    close( RUNNING_TEST );

            #
            # figure out whether the test passed or failed.  print out an
            # appropriate level of output based on the value of $js_quiet
            #
            if ( $js_test =~ /-n\.js$/ ) {
                if ( $runtime_error ) {
                    if ( !$js_quiet ) {
                        &js_print( " PASSED!\n ",
                            "<font color=#009900>&nbsp;&nbsp",
                            "</font><br>" );
                        if ( $js_errors ) {
                            &js_print( $runtime_error, "<pre>", "</pre>");
                        }
                    }
                } else {
                    &js_print_suitename;
                    &js_print_filename;
                    &js_print_bugnumber;
                    &js_print( " FAILED! ", "&nbsp;&nbsp;<font color=#990000>",
                        "</font>");
                    &js_print( " Should have resulted in an error\n",
                        "","<br>" );
                }
            } else {
                if ( $passed == 1 && !$js_quiet) {
                    &js_print( " PASSED!\n " , "&nbsp;&nbsp;<font color=#009900>",
                        "</font><br>" );
                } else {
					if ($passed == -1) {
						&js_print_suitename;
						&js_print_filename;
						&js_print_bugnumber;
						&js_print( " FAILED!\n " , "&nbsp;&nbsp;<font color=#990000>",
						"</font><br>" );
						&js_print( " Missing 'PASSED' in output\n", "","<br>" );
						&js_print( $log, "output:<br><pre>", "</pre>" );
                     }
				}						

            }
        }
    }
}

#
# figure out what os we're on, the default name of the object directory
#
sub setup_env {
    # MOZ_SRC must be set, so we can figure out where the
    # JavaScript executable is
    $moz_src = $ENV{"MOZ_SRC"}
        || die( "You need to set your MOZ_SRC environment variable.\n" );
    $src_dir = $moz_src . '/mozilla/js/src/';

    # JS_TEST_DIR must be set so we can figure out where the tests are.
    $test_dir = $ENV{"JS_TEST_DIR"};

    # if it's not set, look for it relative to $moz_src
    if ( !$test_dir ) {
        $test_dir = $moz_src . '/mozilla/js/tests/';
    }

    # make sure that the test dir exists
    if ( ! -e $test_dir ) {
        die "The JavaScript Test Library could not be found at $test_dir.\n" .
            "Check the tests out from /mozilla/js/tests or\n" .
            "Set the value of your JS_TEST_DIR environment variable\n " .
            "to the location of the test library.\n";
    }

    # make sure that the test dir ends with a trailing slash
    $test_dir .= '/';

    chdir $src_dir;

    # figure out which platform we're on, and figure out where the object
    # directory is

    $machine_os = `uname -s`;

    if ( $machine_os =~ /WIN/ ) {
        $machine_os = 'WIN';
        $object_dir = ($js_debug) ? 'Debug' : 'Release';
        $js_exe = 'jsshell.exe';
    } else {
        chop $machine_os;
        $js_exe = 'js';

        # figure out what the object directory is.  on all platforms,
        # it's the directory that ends in OBJ.  if $js_debug is set,
        # look the directory that ends with or DBG.OBJ; otherwise
        # look for the directory that ends with OPT.OBJ

        opendir ( SRC_DIR_FILES, $src_dir );
        @src_dir_files = readdir( SRC_DIR_FILES );
        closedir ( SRC_DIR_FILES );

        $object_pattern = $js_debug ? 'DBG.OBJ' : 'OPT.OBJ';

        foreach (@src_dir_files) {
            if ( $_ =~ /$object_pattern/ && $_ =~ $machine_os) {
                $object_dir = $_;
            }
        }
    }
    if ( ! $object_dir ) {
        die( "Couldn't find an object directory in $src_dir.\n" );
    }

    # figure out what the name of the javascript executable should be, and
    # make sure it's there.  if it's not there, give a helpful message so
    # the user can figure out what they need to do next.


    if ( ! $js_exe_full_path ) {
        $shell_command = $src_dir . $object_dir .'/'. $js_exe;
    } else {
        $shell_command = $js_exe_full_path;
    }

    if ( !-e $shell_command ) {
        die ("Could not find JavaScript shell executable $shell_command.\n" .
            "Check the value of your MOZ_SRC environment variable.\n" .
            "Currently, MOZ_SRC is set to $ENV{\"MOZ_SRC\"}\n".
            "See the readme at http://lxr.mozilla.org/mozilla/src/js/src/ " .
            "for instructions on building the JavaScript shell.\n" );
    }

    # set the output file name.  let's base its name on the date and platform,
    # and give it a sequence number.

    if ( $get_output ) {
        $js_output = &get_output;
    }
    if ($js_output) {
        print( "Writing results to $js_output\n" );
        chdir $test_dir;
        open( JS_OUTPUT, "> ${js_output}" ) ||
            die "Can't open log file $js_output\n";
        close JS_OUTPUT;
    }

    # get the start time
    $start_time = time;

    # print out some nice stuff
    $start_date = &get_date;
    &js_print( "JavaScript tests started: " . $start_date, "<p><tt>", "</tt></p>" );

    &js_print ("Executing all the tests under $test_dir\n against " .
        "$shell_command\n", "<p><tt>", "</tt></p>" );
}

#
# parse arguments.  see usage for what arguments are expected.
#
sub parse_args {
    $i = 0;
    while( $i < @ARGV ){
        if ( $ARGV[$i] eq '--threaded' ) {
            $js_threaded = 1;
        } elsif ( $ARGV[$i] eq '--d' ) {
            $js_debug = 1;
        } elsif ( $ARGV[$i] eq '--14' ) {
            $js_version = '14';
        } elsif ( $ARGV[$i] eq '--v' ) {
            $js_verbose = 1;
        } elsif ( $ARGV[$i] eq '-f' ) {
            $js_output = $ARGV[++$i];
        } elsif ( $ARGV[$i] eq '--o' ) {
            $get_output = 1;
        } elsif ($ARGV[$i] eq '--e' ) {
            $js_errors = 1;
        } elsif ($ARGV[$i] eq '--q' ) {
            $js_quiet = 1;
        } elsif ($ARGV[$i] eq '--h' ) {
            die &usage;
        } elsif ( $ARGV[$i] eq '-E' ) {
            $js_exe_full_path = $ARGV[$i+1];
            $i++;
        } else {
            die &usage;
        }
        $i++;
    }

    #
    # if no output options are provided, show some output and write to file
    #
    if ( !$js_verbose && !$js_output && !$get_output ) {
        $get_output = 1;
    }
}

#
# print the arguments that this script expects
#
sub usage {
    die ("usage: $0\n" .
        "--q       Quiet mode -- only show information for tests that failed\n".
        "--e       Show runtime error messages for negative tests\n" .
        "--v       Verbose output -- show all test cases (not recommended)\n" .
        "--o       Send output to file whose generated name is based on date\n".
        "--d       Look for a debug JavaScript executable (default is optimized)\n" .
        "-f <file> Redirect output to file named <file>\n"
        );
}

#
# if $js_output is set, print to file as well as stdout
#
sub js_print {
    ($string, $start_tag, $end_tag) = @_;

    if ($js_output) {
        open( JS_OUTPUT, ">> ${js_output}" ) ||
            die "Can't open log file $js_output\n";

        print JS_OUTPUT "$start_tag $string $end_tag";
        close JS_OUTPUT;
    }
    print $string;
}

#
# close open files
#
sub cleanup_env {
    # print out some nice stuff
    $end_date = &get_date;
    &js_print( "\nTests complete at $end_date", "<hr><tt>", "</tt>" );

    # print out how long it took to complete
    $end_time = time;

    $test_seconds = ( $end_time - $start_time );

    &js_print( "Start Date: $start_date\n", "<tt><br>" );
    &js_print( "End Date:   $end_date\n", "<br>" );
    &js_print( "Test Time:  $test_seconds seconds\n", "<br>" );

    if ($js_output ) {
        if ( !$js_verbose) {
            &js_print( "Results were written to " . $js_output ."\n",
                "<br>", "</tt>" );
        }
        close JS_OUTPUT;
    }
}


#
# get the current date and time
#
sub get_date {
    &get_localtime;
    $now = $year ."/". $mon ."/". $mday ." ". $hour .":".
        $min .":". $sec ."\n";
    return $now;

}
sub get_localtime {
    ( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst ) =
        localtime;
    $mon++;
    $mon = &zero_pad($mon);
    $year= ($year < 2000) ? "19" . $year : $year;
    $mday= &zero_pad($mday);
    $sec = &zero_pad($sec);
    $min = &zero_pad($min);
    $hour = &zero_pad($hour);
}
sub zero_pad {
    local ($string) = @_;
    $string = ($string < 10) ? "0" . $string : $string;
    return $string;
}

#
# generate an output file name based on the date
#
sub get_output {
    &get_localtime;

    chdir $test_dir;

    $js_output = $test_dir ."/". $year .'-'. $mon .'-'. $mday ."\.1.html";

    $output_file_found = 0;

    while ( !$output_file_found ) {
        if ( -e $js_output ) {
        # get the last sequence number - everything after the dot
            @seq_no = split( /\./, $js_output, 2 );
            $js_output = $seq_no[0] .".". (++$seq_no[1]) . "\.html";
        } else {
            $output_file_found = 1;
        }
    }
    return $js_output;
}

sub js_print_suitename {
    if ( !$js_printed_suitename ) {
        &js_print( "$suite\\$subdir\n", "<hr><font size+=1><b>",
            "</font></b><br>" );
    }
    $js_printed_suitename = 1;
}

sub js_print_filename {
    if ( !$js_printed_filename ) {
        &js_print( "$js_test\n", "<b>", "</b><br>" );
        $js_printed_filename = 1;
    }
}

sub js_print_bugnumber {
    if ( !$js_printed_bugnumber ) {
        if ( $js_bugnumber =~ /^http/ ) {
            &js_print( "$js_bugnumber", "<a href=$js_bugnumber>", "</a>" );
        } else {
            &js_print( "$js_bugnumber",
                "<a href=http://scopus.mcom.com/bugsplat/show_bug.cgi?id=" .
                    $js_bugnumber .">",
                "</a>" );
        }
        $js_printed_bugnumber = 1;
    }
}
