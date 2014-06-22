#!/ns/tools/bin/perl5

# mkhtml.pl cruises through your $MOZ_SRC/mozilla/js/tests/ subdirectories,
# and for any .js file it finds, creates an HTML file that includes:
# $MOZ_SRC/mozilla/js/tests/$suite/shell.js, $
# MOZ_SRC/mozilla/js/tests/$suite/browser.js,
# and the test.js file.
#
#

$moz_src = $ENV{"MOZ_SRC"} ||
    die ("You need to set your MOZ_SRC environment variable.\n");

$test_home = $moz_src ."/js/tests/";

opendir (TEST_HOME, $test_home);
@__suites = readdir (TEST_HOME);
closedir TEST_HOME;

foreach (@__suites ) {
    if ( -d $_ && $_ !~ /\./ && $_ !~ 'CVS' ) {
        $suites[$#suites+1] = $_;
    }
}
if ( ! $ARGV[0]  ) {
    die ( "Specify a directory: ". join(" ", @suites) ."\n" );
}

$js_test_dir = $moz_src .  "/js/tests/" . $ARGV[0] ."/";

print "Generating html files for the tests in $js_test_dir\n";

$shell_js = $js_test_dir . "shell.js";
$browser_js = $js_test_dir . "browser.js";

# cd to the test directory
chdir $js_test_dir ||
    die "Couldn't chdir to js_test_dir, which is $js_test_dir\n";

print ( "js_test_dir is $js_test_dir\n" );

# read the test directory
opendir ( JS_TEST_DIR, $js_test_dir );
#    || die "Couldn't open js_test_dir, which is $js_test_dir\n";
@js_test_dir_items = readdir( JS_TEST_DIR );
#   || die "Couldn't read js_test_dir, which is $js_test_dir\n";
closedir( JS_TEST_DIR );

print ("The js_test_dir_items are: " . join( ",", @js_test_dir_items ) . "\n");

# figure out which of the items are directories
foreach $js_test_subdir ( @js_test_dir_items ) {
    if ( -d $js_test_subdir ) {

        $js_test_subdir = $js_test_dir ."/" . $js_test_subdir;
        chdir $js_test_subdir
            || die "Couldn't chdir to js_test_subdir $js_test_subdir\n";
        print "Just chdir'd to $js_test_subdir \n";

        opendir( JS_TEST_SUBDIR, $js_test_subdir );
        @subdir_tests = readdir( JS_TEST_SUBDIR );
        closedir( JS_TEST_SUBDIR );

        foreach ( @subdir_tests ) {
            $js_test = $_;

            if ( $_ =~ /\.js$/ ) {
                s/\.js$/\.html/;
                print $_ ."\n";
                open( HTML_TEST, "> $_")
                    || die "Can't open html file $test_html\n";
                print HTML_TEST
                    '<script src=./../shell.js></script>';
                print HTML_TEST
                    '<script src=./../browser.js></script>';
                print HTML_TEST
                    '<script src=./' . $js_test. '></script>';
                close HTML_TEST;
            }
        }
    }
    chdir $js_test_dir;
}

