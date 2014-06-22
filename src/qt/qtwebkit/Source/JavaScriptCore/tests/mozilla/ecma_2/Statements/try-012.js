/**
 *  File Name:          try-012.js
 *  ECMA Section:
 *  Description:        The try statement
 *
 *  This test has a try with no catch, and a finally.  This is like try-003,
 *  but throws from a finally block, not the try block.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "try-012";
    var VERSION = "ECMA_2";
    var TITLE   = "The try statement";
    var BUGNUMBER="336872";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    // Tests start here.

    TrySomething( "x = \"hi\"", true );
    TrySomething( "throw \"boo\"", true );
    TrySomething( "throw 3", true );

    test();

    /**
     *  This function contains a try block with no catch block,
     *  but it does have a finally block.  Try to evaluate expressions
     *  that do and do not throw exceptions.
     *
     * The productioni TryStatement Block Finally is evaluated as follows:
     * 1. Evaluate Block
     * 2. Evaluate Finally
     * 3. If Result(2).type is normal return result 1 (in the test case, result 1 has
     *    the completion type throw)
     * 4. return result 2 (does not get hit in this case)
     *
     */

    function TrySomething( expression, throwing ) {
        innerFinally = "FAIL: DID NOT HIT INNER FINALLY BLOCK";
        if (throwing) {
            outerCatch = "FAILED: NO EXCEPTION CAUGHT";
        } else {
            outerCatch = "PASS";
        }
        outerFinally = "FAIL: DID NOT HIT OUTER FINALLY BLOCK";


        // If the inner finally does not throw an exception, the result
        // of the try block should be returned.  (Type of inner return
        // value should be throw if finally executes correctly

        try {
            try {
                throw 0;
            } finally {
                innerFinally = "PASS";
                eval( expression );
            }
        } catch ( e  ) {
            if (throwing) {
                outerCatch = "PASS";
            } else {
                outerCatch = "FAIL: HIT OUTER CATCH BLOCK";
            }
        } finally {
            outerFinally = "PASS";
        }


        testcases[tc++] = new TestCase(
                SECTION,
                "eval( " + expression +" ): evaluated inner finally block",
                "PASS",
                innerFinally );
        testcases[tc++] = new TestCase(
                SECTION,
                "eval( " + expression +" ): evaluated outer catch block ",
                "PASS",
                outerCatch );
        testcases[tc++] = new TestCase(
                SECTION,
                "eval( " + expression +" ):  evaluated outer finally block",
                "PASS",
                outerFinally );
    }
