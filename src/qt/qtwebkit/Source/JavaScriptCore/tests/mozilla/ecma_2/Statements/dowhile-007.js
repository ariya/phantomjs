/**
 *  File Name:          dowhile-007
 *  ECMA Section:
 *  Description:        do...while statements
 *
 *  A general do...while test.
 *
 *  Author:             christine@netscape.com
 *  Date:               26 August 1998
 */
    var SECTION = "dowhile-007";
    var VERSION = "ECMA_2";
    var TITLE   = "do...while";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    DoWhile( new DoWhileObject( false, false, false, false ));
    DoWhile( new DoWhileObject( true, false, false, false ));
    DoWhile( new DoWhileObject( true, true, false, false ));
    DoWhile( new DoWhileObject( true, true, true, false ));
    DoWhile( new DoWhileObject( true, true, true, true ));
    DoWhile( new DoWhileObject( false, false, false, true ));
    DoWhile( new DoWhileObject( false, false, true, true ));
    DoWhile( new DoWhileObject( false, true, true, true ));
    DoWhile( new DoWhileObject( false, false, true, false ));

    test();

function DoWhileObject( out1, out2, out3, in1 ) {
    this.breakOutOne = out1;
    this.breakOutTwo = out2;
    this.breakOutThree = out3;
    this.breakIn = in1;
}
function DoWhile( object ) {
    result1 = false;
    result2 = false;
    result3 = false;
    result4 = false;

    outie:
        do {
            if ( object.breakOutOne ) {
                break outie;
            }
            result1 = true;

            innie:
                do {
                    if ( object.breakOutTwo ) {
                        break outie;
                    }
                    result2 = true;

                    if ( object.breakIn ) {
                        break innie;
                    }
                    result3 = true;

                } while ( false );
                    if ( object.breakOutThree ) {
                        break outie;
                    }
                    result4 = true;
        } while ( false );

        testcases[tc++] = new TestCase(
            SECTION,
            "break one: ",
            (object.breakOutOne) ? false : true,
            result1 );

        testcases[tc++] = new TestCase(
            SECTION,
            "break two: ",
            (object.breakOutOne||object.breakOutTwo) ? false : true,
            result2 );

        testcases[tc++] = new TestCase(
            SECTION,
            "break three: ",
            (object.breakOutOne||object.breakOutTwo||object.breakIn) ? false : true,
            result3 );

        testcases[tc++] = new TestCase(
            SECTION,
            "break four: ",
            (object.breakOutOne||object.breakOutTwo||object.breakOutThree) ? false: true,
            result4 );
}
