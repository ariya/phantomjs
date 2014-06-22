/**
 *  File Name:          while-004
 *  ECMA Section:
 *  Description:        while statement
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "while-004";
    var VERSION = "ECMA_2";
    var TITLE   = "while statement";
    var BUGNUMBER="316725";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    DoWhile_1();
    DoWhile_2();
    DoWhile_3();
    DoWhile_4();
    DoWhile_5();

     test();

    /**
     *  Break out of a while by calling return.
     *
     *  Tests:  12.6.2 step 6.
     */
     function dowhile() {
        result = "pass";

        while (true) {
            return result;
            result = "fail: hit code after return statement";
            break;
        }
    }

   function DoWhile_1() {
        description = "return statement in a while block";

        result = dowhile();

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_1" + description,
            "pass",
            result );
    }

    /**
     *  While with a labeled continue statement.  Verify that statements
     *  after the continue statement are not evaluated.
     *
     *  Tests: 12.6.2 step 8.
     *
     */
    function DoWhile_2() {
        var description = "while with a labeled continue statement";
        var result1 = "pass";
        var result2 = "fail: did not execute code after loop, but inside label";
        var i = 0;
        var j = 0;

        theloop:
            while( i++ < 10  ) {
                j++;
                continue theloop;
                result1 = "failed:  hit code after continue statement";
            }
        result2 = "pass";

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_2:  " +description + " - code inside the loop, before the continue should be executed ("+j+")",
            true,
            j == 10 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_2:  " +description +" - code after labeled continue should not be executed",
            "pass",
            result1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_2:  " +description +" - code after loop but inside label should be executed",
            "pass",
            result2 );
    }

    /**
     *  While with a labeled break.
     *
     */
    function DoWhile_3() {
        var description = "while with a labeled break statement";
        var result1 = "pass";
        var result2 = "pass";
        var result3 = "fail: did not get to code after label";

        woohoo: {
            while( true ) {
                break woohoo;
                result1 = "fail: got to code after a break";
            }
            result2 = "fail: got to code outside of loop but inside label";
        }

        result3 = "pass";

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_3: " +description +" - verify break out of loop",
            "pass",
            result1 );


        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_3: " +description +" - verify break out of label",
            "pass",
            result2 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_3: " +description + " - verify correct exit from label",
            "pass",
            result3 );
    }


    /**
     *  Labled while with an unlabeled break
     *
     */
    function DoWhile_4() {
        var description = "labeled while with an unlabeled break";
        var result1 = "pass";
        var result2 = "pass";
        var result3 = "fail: did not evaluate statement after label";

        woohooboy: {
            while( true ) {
                break woohooboy;
                result1 = "fail: got to code after the break";
            }
            result2 = "fail: broke out of while, but not out of label";
        }
        result3 = "pass";

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_4: " +description +" - verify break out of while loop",
            "pass",
            result1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_4: " +description + " - verify break out of label",
            "pass",
            result2 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_4: " +description +" - verify that statements after label are evaluated",
            "pass",
            result3 );
    }

    /**
     *  in this case, should behave the same way as
     *
     *
     */
    function DoWhile_5() {
        var description = "while with a labeled continue statement";
        var result1 = "pass";
        var result2 = "fail: did not execute code after loop, but inside label";
        var i = 0;
        var j = 0;

        theloop: {
            j++;
            while( i++ < 10  ) {
                continue;
                result1 = "failed:  hit code after continue statement";
            }
            result2 = "pass";
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_5: " +description + " - continue should not execute statements above the loop",
            true,
            ( j == 1 ) );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_5: " +description +" - code after labeled continue should not be executed",
            "pass",
            result1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "DoWhile_5: " +description +" - code after loop but inside label should be executed",
            "pass",
            result2 );
    }

