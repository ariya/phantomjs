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
 *  File Name:          function-003.js
 *  Description:
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=104766
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "toString-001.js";
    var VERSION = "JS1_4";
    var TITLE   = "Regression test case for 104766";
    var BUGNUMBER="310514";

    startTest();

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase(
        SECTION,
        "StripSpaces(Array.prototype.concat.toString()).substring(0,17)",
        "functionconcat(){",
        StripSpaces(Array.prototype.concat.toString()).substring(0,17));

    test();

   function StripSpaces( s ) {
        for ( var currentChar = 0, strippedString="";
                currentChar < s.length; currentChar++ )
        {
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

