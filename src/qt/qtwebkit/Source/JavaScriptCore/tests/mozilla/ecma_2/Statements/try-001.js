/**
 *  File Name:          try-001.js
 *  ECMA Section:
 *  Description:        The try statement
 *
 *  This test contains try, catch, and finally blocks.  An exception is
 *  sometimes thrown by a function called from within the try block.
 *
 *  This test doesn't actually make any LiveConnect calls.
 *
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "";
    var VERSION = "ECMA_2";
    var TITLE   = "The try statement";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var INVALID_JAVA_INTEGER_VALUE = "Invalid value for java.lang.Integer constructor";

    TryNewJavaInteger( "3.14159", INVALID_JAVA_INTEGER_VALUE );
    TryNewJavaInteger( NaN, INVALID_JAVA_INTEGER_VALUE );
    TryNewJavaInteger( 0,  0 );
    TryNewJavaInteger( -1, -1 );
    TryNewJavaInteger( 1,  1 );
    TryNewJavaInteger( Infinity, Infinity );

    test();

    /**
     *  Check to see if the input is valid for java.lang.Integer. If it is
     *  not valid, throw INVALID_JAVA_INTEGER_VALUE.  If input is valid,
     *  return Number( v )
     *
     */

    function newJavaInteger( v ) {
        value = Number( v );
        if ( Math.floor(value) != value || isNaN(value) ) {
            throw ( INVALID_JAVA_INTEGER_VALUE );
        } else {
            return value;
        }
    }

    /**
     *  Call newJavaInteger( value ) from within a try block.  Catch any
     *  exception, and store it in result.  Verify that we got the right
     *  return value from newJavaInteger in cases in which we do not expect
     *  exceptions, and that we got the exception in cases where an exception
     *  was expected.
     */
    function TryNewJavaInteger( value, expect ) {
        var finalTest = false;

        try {
            result = newJavaInteger( value );
        } catch ( e ) {
            result = String( e );
        } finally {
            finalTest = true;
        }
            testcases[tc++] = new TestCase(
                SECTION,
                "newJavaValue( " + value +" )",
                expect,
                result);

            testcases[tc++] = new TestCase(
                SECTION,
                "newJavaValue( " + value +" ) hit finally block",
                true,
                finalTest);

    }

