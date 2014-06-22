/**
 *  File Name:          switch-001.js
 *  ECMA Section:
 *  Description:        The switch Statement
 *
 *  A simple switch test with no abrupt completions.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 *
 */
    var SECTION = "switch-001";
    var VERSION = "ECMA_2";
    var TITLE   = "The switch statement";

    var BUGNUMBER="315767";



    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    SwitchTest( 0, 126 );
    SwitchTest( 1, 124 );
    SwitchTest( 2, 120 );
    SwitchTest( 3, 112 );
    SwitchTest( 4, 64 );
    SwitchTest( 5, 96 );
    SwitchTest( true, 96 );
    SwitchTest( false, 96 );
    SwitchTest( null, 96 );
    SwitchTest( void 0, 96 );
    SwitchTest( "0", 96 );

    test();

    function SwitchTest( input, expect ) {
        var result = 0;

        switch ( input ) {
            case 0:
                result += 2;
            case 1:
                result += 4;
            case 2:
                result += 8;
            case 3:
                result += 16;
            default:
                result += 32;
            case 4:
                result +=64;
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "switch with no breaks, case expressions are numbers.  input is "+
            input,
            expect,
            result );
    }
