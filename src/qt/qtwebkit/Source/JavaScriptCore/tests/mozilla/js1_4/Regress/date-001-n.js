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
 *  File Name:          date-001-n.js
 *  Description:
 *
 *  http://scopus.mcom.com/bugsplat/show_bug.cgi?id=299903
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "date-001-n.js";
    var VERSION = "JS1_4";
    var TITLE   = "Regression test case for 299903";
    var BUGNUMBER="299903";

    startTest();

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    function MyDate() {
        this.foo = "bar";
    }
    MyDate.prototype = new Date();

    testcases[tc++] = new TestCase(
        SECTION,
        "function MyDate() { this.foo = \"bar\"; }; "+
        "MyDate.prototype = new Date(); " +
        "new MyDate().toString()",
        "error",
        new MyDate().toString() );

    test();
