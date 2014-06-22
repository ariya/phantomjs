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
 *  File Name:          function-002.js
 *  Description:
 *
 * http://scopus.mcom.com/bugsplat/show_bug.cgi?id=330462
 * js> function f(a){var a,b;}
 *
 * causes an an assert on a null 'sprop' in the 'Variables' function in
 * jsparse.c This will crash non-debug build.
 *
 *  Author:      christine@netscape.com
 *  Date:           11 August 1998
 *  REVISED:   04 February 2001
 *  (changed  the comma expressions from trivial to  non-trivial)
 *  Author:      pschwartau@netscape.com
 *
 * Brendan: "The test seemed to require something that ECMA does not
 * guarantee, and that JS1.4 didn't either. For example, given
 *
 *             dec2 = "function f2(){1,2}";
 *
 * the engine is free to decompile a function object compiled from this source,
 * via Function.prototype.toString(), into some other string that compiles to
 * an equivalent function. The engine now eliminates the useless comma expression
 * 1,2, giving function f2(){}. This should be legal by the testsuite's lights."
 *
 */
    var SECTION = "function-002.js";
    var VERSION = "JS1_4";
    var TITLE   = "Regression test case for 325843";
    var BUGNUMBER="330462";

    startTest();

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    dec1 = "function f1(x,y){++x, --y}";
    dec2 = "function f2(){var y; f1(1,2); y=new Date(); print(y.toString())}";

    eval(dec1);
    eval(dec2);

    testcases[tc++] = new TestCase(
        SECTION,
         "typeof f1",
        "function",
        typeof f1 );


    // force a function decompilation
    testcases[tc++] = new TestCase(
        SECTION,
        "f1.toString() == dec1",
        true,
        StripSpaces(f1.toString()) == StripSpaces(dec1));

    testcases[tc++] = new TestCase(
        SECTION,
        "typeof f2",
        "function",
        typeof f2 );

    // force a function decompilation

    testcases[tc++] = new TestCase(
        SECTION,
        "f2.toString() == dec2",
        true,
        StripSpaces(f2.toString()) == StripSpaces(dec2));

    test();

    function StripSpaces( s ) {
        var strippedString = "";
        for ( var currentChar = 0; currentChar < s.length; currentChar++ ) {
            if (!IsWhiteSpace(s.charAt(currentChar))) {
                strippedString += s.charAt(currentChar);
            }
        }
        return strippedString;
    }

    function IsWhiteSpace( string ) {
        var cc = string.charCodeAt(0);

        switch (cc) {
            case (0x0009):
            case (0x000B):
            case (0x000C):
            case (0x0020):
            case (0x000A):
            case (0x000D):
            case ( 59 ): // let's strip out semicolons, too
                return true;
                break;
            default:
                return false;
        }
    }

