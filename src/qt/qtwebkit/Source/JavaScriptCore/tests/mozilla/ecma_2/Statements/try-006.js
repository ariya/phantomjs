/**
 *  File Name:          try-006.js
 *  ECMA Section:
 *  Description:        The try statement
 *
 *  Throw an exception from within a With block in a try block.  Verify
 *  that any expected exceptions are caught.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "try-006";
    var VERSION = "ECMA_2";
    var TITLE   = "The try statement";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    /**
     *  This is the "check" function for test objects that will
     *  throw an exception.
     */
    function throwException() {
        throw EXCEPTION_STRING +": " + this.valueOf();
    }
    var EXCEPTION_STRING = "Exception thrown:";

    /**
     *  This is the "check" function for test objects that do not
     *  throw an exception
     */
    function noException() {
        return this.valueOf();
    }

    /**
     *  Add test cases here
     */
    TryWith( new TryObject( "hello", throwException, true ));
    TryWith( new TryObject( "hola",  noException, false ));

    /**
     *  Run the test.
     */

    test();

    /**
     *  This is the object that will be the "this" in a with block.
     */
    function TryObject( value, fun, exception ) {
        this.value = value;
        this.exception = exception;

        this.valueOf = new Function ( "return this.value" );
        this.check = fun;
    }

    /**
     *  This function has the try block that has a with block within it.
     *  Test cases are added in this function.  Within the with block, the
     *  object's "check" function is called.  If the test object's exception
     *  property is true, we expect the result to be the exception value.
     *  If exception is false, then we expect the result to be the value of
     *  the object.
     */
    function TryWith( object ) {
        try {
            with ( object ) {
                result = check();
            }
        } catch ( e ) {
            result = e;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "TryWith( " + object.value +" )",
            (object.exception ? EXCEPTION_STRING +": " + object.valueOf() : object.valueOf()),
            result );
    }
