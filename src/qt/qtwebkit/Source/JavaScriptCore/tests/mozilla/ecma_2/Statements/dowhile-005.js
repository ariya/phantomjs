/**
 *  File Name:          dowhile-005
 *  ECMA Section:
 *  Description:        do...while statements
 *
 *  Test a labeled do...while.  Break out of the loop with no label
 *  should break out of the loop, but not out of the label.
 *
 *  Currently causes an infinite loop in the monkey.  Uncomment the
 *  print statement below and it works OK.
 *
 *  Author:             christine@netscape.com
 *  Date:               26 August 1998
 */
    var SECTION = "dowhile-005";
    var VERSION = "ECMA_2";
    var TITLE   = "do...while with a labeled continue statement";
    var BUGNUMBER = "316293";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    NestedLabel();


    test();

    function NestedLabel() {
        i = 0;
        result1 = "pass";
        result2 = "fail: did not hit code after inner loop";
        result3 = "pass";

        outer: {
            do {
                inner: {
//                    print( i );
                    break inner;
                    result1 = "fail: did break out of inner label";
                  }
                result2 = "pass";
                break outer;
                print (i);
            } while ( i++ < 100 );

        }

        result3 = "fail: did not break out of outer label";

        testcases[tc++] = new TestCase(
            SECTION,
            "number of loop iterations",
            0,
            i );

        testcases[tc++] = new TestCase(
            SECTION,
            "break out of inner loop",
            "pass",
            result1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "break out of outer loop",
            "pass",
            result2 );
    }