/**
 *  File Name:          switch-003.js
 *  ECMA Section:
 *  Description:        The switch Statement
 *
 *  Attempt to verify that case statements are evaluated in source order
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 *
 */
    var SECTION = "switch-003";
    var VERSION = "ECMA_2";
    var TITLE   = "The switch statement";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    SwitchTest( "a", "abc" );
    SwitchTest( "b", "bc" );
    SwitchTest( "c", "c" );
    SwitchTest( "d", "*abc" );
    SwitchTest( "v", "*abc" );
    SwitchTest( "w", "w*abc" );
    SwitchTest( "x", "xw*abc" );
    SwitchTest( "y", "yxw*abc" );
    SwitchTest( "z", "zyxw*abc" );
//    SwitchTest( new java.lang.String("z"), "*abc" );

    test();

    function SwitchTest( input, expect ) {
        var result = "";

        switch ( input ) {
            case "z": result += "z";
            case "y": result += "y";
            case "x": result += "x";
            case "w": result += "w";
            default: result += "*";
            case "a": result += "a";
            case "b": result += "b";
            case "c": result += "c";
        }

        testcases[tc++] = new TestCase(
            SECTION,
            "switch with no breaks:  input is " + input,
            expect,
            result );
    }