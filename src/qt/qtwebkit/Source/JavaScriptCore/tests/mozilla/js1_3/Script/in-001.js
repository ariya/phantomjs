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
    File Name:          in-001.js
    Section:
    Description:

    http://scopus.mcom.com/bugsplat/show_bug.cgi?id=196109


    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "in-001";
    var VERSION = "JS1_3";
    var TITLE   = "Regression test for 196109";
    var BUGNUMBER="196109";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    o = {};
    o.foo = 'sil';

    testcases[tc++] = new TestCase(
        SECTION,
        "\"foo\" in o",
        true,
        "foo" in o );

    test();
