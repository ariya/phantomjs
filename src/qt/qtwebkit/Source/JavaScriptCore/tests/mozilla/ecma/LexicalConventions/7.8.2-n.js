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
    File Name:          7.8.2.js
    ECMA Section:       7.8.2 Examples of Automatic Semicolon Insertion
    Description:        compare some specific examples of the automatic
                        insertion rules in the EMCA specification.
    Author:             christine@netscape.com
    Date:               15 september 1997
*/

    var SECTION="7.8.2";
    var VERSION="ECMA_1"
    startTest();
    writeHeaderToLog(SECTION+" "+"Examples of Semicolon Insertion");

    testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

//    array[item++] = new TestCase( "7.8.2",  "{ 1 \n 2 } 3",      3,         "{ 1 \n 2 } 3" );
    array[item++] = new TestCase( "7.8.2",  "{ 1 2 } 3",         "error",   eval("{1 2 } 3")     );

    return ( array );
}

