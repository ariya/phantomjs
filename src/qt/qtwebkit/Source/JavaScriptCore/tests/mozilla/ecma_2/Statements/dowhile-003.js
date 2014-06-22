/**
 *  File Name:          dowhile-003
 *  ECMA Section:
 *  Description:        do...while statements
 *
 *  Test do while, when the while expression is a JavaScript Number object.
 *
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "dowhile-003";
    var VERSION = "ECMA_2";
    var TITLE   = "do...while with a labeled continue statement";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    DoWhile( new DoWhileObject( 1, 1, 0 ));
    DoWhile( new DoWhileObject( 1000, 1000, 0 ));
    DoWhile( new DoWhileObject( 1001, 1001, 0 ));
    DoWhile( new DoWhileObject( 1002, 1001, 1 ));
    DoWhile( new DoWhileObject( -1, 1001, -1002 ));

    test();

function DoWhileObject( value, iterations, endvalue ) {
    this.value = value;
    this.iterations = iterations;
    this.endvalue = endvalue;
}

function DoWhile( object ) {
    var i = 0;

    do {
        object.value =  --object.value;
        i++;
        if ( i > 1000 )
            break;
   } while( object.value );

   testcases[tc++] = new TestCase(
        SECTION,
        "loop iterations",
        object.iterations,
        i
    );

   testcases[tc++] = new TestCase(
        SECTION,
        "object.value",
        object.endvalue,
        Number( object.value )
    );

}