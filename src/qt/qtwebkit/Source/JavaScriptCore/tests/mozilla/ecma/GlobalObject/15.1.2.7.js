/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          15.1.2.7.js
    ECMA Section:       15.1.2.7 isFinite(number)

    Description:        Applies ToNumber to its argument, then returns false if
                        the result is NaN, Infinity, or -Infinity, and otherwise
                        returns true.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/
    var SECTION = "15.1.2.7";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "isFinite( x )";

    var BUGNUMBER= "77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION, "isFinite.length",      1,                  isFinite.length );
    array[item++] = new TestCase( SECTION, "isFinite.length = null; isFinite.length",   1,      eval("isFinite.length=null; isFinite.length") );
    array[item++] = new TestCase( SECTION, "delete isFinite.length",                    false,  delete isFinite.length );
    array[item++] = new TestCase( SECTION, "delete isFinite.length; isFinite.length",   1,      eval("delete isFinite.length; isFinite.length") );
    array[item++] = new TestCase( SECTION, "var MYPROPS=''; for ( p in isFinite ) { MYPROPS+= p }; MYPROPS",    "", eval("var MYPROPS=''; for ( p in isFinite ) { MYPROPS += p }; MYPROPS") );

    array[item++] = new TestCase( SECTION,  "isFinite()",           false,              isFinite() );
    array[item++] = new TestCase( SECTION, "isFinite( null )",      true,              isFinite(null) );
    array[item++] = new TestCase( SECTION, "isFinite( void 0 )",    false,             isFinite(void 0) );
    array[item++] = new TestCase( SECTION, "isFinite( false )",     true,              isFinite(false) );
    array[item++] = new TestCase( SECTION, "isFinite( true)",       true,              isFinite(true) );
    array[item++] = new TestCase( SECTION, "isFinite( ' ' )",       true,              isFinite( " " ) );

    array[item++] = new TestCase( SECTION, "isFinite( new Boolean(true) )",     true,   isFinite(new Boolean(true)) );
    array[item++] = new TestCase( SECTION, "isFinite( new Boolean(false) )",    true,   isFinite(new Boolean(false)) );

    array[item++] = new TestCase( SECTION, "isFinite( 0 )",        true,              isFinite(0) );
    array[item++] = new TestCase( SECTION, "isFinite( 1 )",        true,              isFinite(1) );
    array[item++] = new TestCase( SECTION, "isFinite( 2 )",        true,              isFinite(2) );
    array[item++] = new TestCase( SECTION, "isFinite( 3 )",        true,              isFinite(3) );
    array[item++] = new TestCase( SECTION, "isFinite( 4 )",        true,              isFinite(4) );
    array[item++] = new TestCase( SECTION, "isFinite( 5 )",        true,              isFinite(5) );
    array[item++] = new TestCase( SECTION, "isFinite( 6 )",        true,              isFinite(6) );
    array[item++] = new TestCase( SECTION, "isFinite( 7 )",        true,              isFinite(7) );
    array[item++] = new TestCase( SECTION, "isFinite( 8 )",        true,              isFinite(8) );
    array[item++] = new TestCase( SECTION, "isFinite( 9 )",        true,              isFinite(9) );

    array[item++] = new TestCase( SECTION, "isFinite( '0' )",        true,              isFinite('0') );
    array[item++] = new TestCase( SECTION, "isFinite( '1' )",        true,              isFinite('1') );
    array[item++] = new TestCase( SECTION, "isFinite( '2' )",        true,              isFinite('2') );
    array[item++] = new TestCase( SECTION, "isFinite( '3' )",        true,              isFinite('3') );
    array[item++] = new TestCase( SECTION, "isFinite( '4' )",        true,              isFinite('4') );
    array[item++] = new TestCase( SECTION, "isFinite( '5' )",        true,              isFinite('5') );
    array[item++] = new TestCase( SECTION, "isFinite( '6' )",        true,              isFinite('6') );
    array[item++] = new TestCase( SECTION, "isFinite( '7' )",        true,              isFinite('7') );
    array[item++] = new TestCase( SECTION, "isFinite( '8' )",        true,              isFinite('8') );
    array[item++] = new TestCase( SECTION, "isFinite( '9' )",        true,              isFinite('9') );

    array[item++] = new TestCase( SECTION, "isFinite( 0x0a )",    true,                 isFinite( 0x0a ) );
    array[item++] = new TestCase( SECTION, "isFinite( 0xaa )",    true,                 isFinite( 0xaa ) );
    array[item++] = new TestCase( SECTION, "isFinite( 0x0A )",    true,                 isFinite( 0x0A ) );
    array[item++] = new TestCase( SECTION, "isFinite( 0xAA )",    true,                 isFinite( 0xAA ) );

    array[item++] = new TestCase( SECTION, "isFinite( '0x0a' )",    true,               isFinite( "0x0a" ) );
    array[item++] = new TestCase( SECTION, "isFinite( '0xaa' )",    true,               isFinite( "0xaa" ) );
    array[item++] = new TestCase( SECTION, "isFinite( '0x0A' )",    true,               isFinite( "0x0A" ) );
    array[item++] = new TestCase( SECTION, "isFinite( '0xAA' )",    true,               isFinite( "0xAA" ) );

    array[item++] = new TestCase( SECTION, "isFinite( 077 )",       true,               isFinite( 077 ) );
    array[item++] = new TestCase( SECTION, "isFinite( '077' )",     true,               isFinite( "077" ) );

    array[item++] = new TestCase( SECTION, "isFinite( new String('Infinity') )",        false,      isFinite(new String("Infinity")) );
    array[item++] = new TestCase( SECTION, "isFinite( new String('-Infinity') )",       false,      isFinite(new String("-Infinity")) );

    array[item++] = new TestCase( SECTION, "isFinite( 'Infinity' )",        false,      isFinite("Infinity") );
    array[item++] = new TestCase( SECTION, "isFinite( '-Infinity' )",       false,      isFinite("-Infinity") );
    array[item++] = new TestCase( SECTION, "isFinite( Number.POSITIVE_INFINITY )",  false,  isFinite(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "isFinite( Number.NEGATIVE_INFINITY )",  false,  isFinite(Number.NEGATIVE_INFINITY) );
    array[item++] = new TestCase( SECTION, "isFinite( Number.NaN )",                false,  isFinite(Number.NaN) );

    array[item++] = new TestCase( SECTION, "isFinite( Infinity )",  false,  isFinite(Infinity) );
    array[item++] = new TestCase( SECTION, "isFinite( -Infinity )",  false,  isFinite(-Infinity) );
    array[item++] = new TestCase( SECTION, "isFinite( NaN )",                false,  isFinite(NaN) );


    array[item++] = new TestCase( SECTION, "isFinite( Number.MAX_VALUE )",          true,  isFinite(Number.MAX_VALUE) );
    array[item++] = new TestCase( SECTION, "isFinite( Number.MIN_VALUE )",          true,  isFinite(Number.MIN_VALUE) );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
