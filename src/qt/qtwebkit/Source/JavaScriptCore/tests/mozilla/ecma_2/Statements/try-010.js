/**
 *  File Name:          try-010.js
 *  ECMA Section:
 *  Description:        The try statement
 *
 *  This has a try block nested in the try block.  Verify that the
 *  exception is caught by the right try block, and all finally blocks
 *  are executed.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "try-010";
    var VERSION = "ECMA_2";
    var TITLE   = "The try statement: try in a tryblock";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var EXCEPTION_STRING = "Exception thrown: ";
    var NO_EXCEPTION_STRING = "No exception thrown:  ";


    NestedTry( new TryObject( "No Exceptions Thrown",  NoException, NoException, 43 ) );
    NestedTry( new TryObject( "Throw Exception in Outer Try", ThrowException, NoException, 48 ));
    NestedTry( new TryObject( "Throw Exception in Inner Try", NoException, ThrowException, 45 ));
    NestedTry( new TryObject( "Throw Exception in Both Trys", ThrowException, ThrowException, 48 ));

    test();

    function TryObject( description, tryOne, tryTwo, result ) {
        this.description = description;
        this.tryOne = tryOne;
        this.tryTwo = tryTwo;
        this.result = result;
    }
    function ThrowException() {
        throw EXCEPTION_STRING + this.value;
    }
    function NoException() {
        return NO_EXCEPTION_STRING + this.value;
    }
    function NestedTry( object ) {
        result = 0;
        try {
            object.tryOne();
            result += 1;
            try {
                object.tryTwo();
                result += 2;
            } catch ( e ) {
                result +=4;
            } finally {
                result += 8;
            }
        } catch ( e ) {
            result += 16;
        } finally {
            result += 32;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            object.description,
            object.result,
            result );
    }
