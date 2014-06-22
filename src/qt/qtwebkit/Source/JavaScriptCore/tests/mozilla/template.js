/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): 
*/

/**
 *  File Name:          template.js
 *  Reference:          ** replace with bugzilla URL or document reference **
 *  Description:        ** replace with description of test **
 *  Author:             ** replace with your e-mail address **
 */

    var SECTION = "";       // provide a document reference (ie, ECMA section)
    var VERSION = "ECMA_2"; // Version of JavaScript or ECMA
    var TITLE   = "";       // Provide ECMA section title or a description
    var BUGNUMBER = "";     // Provide URL to bugsplat or bugzilla report

    startTest();               // leave this alone

    /*
     * Calls to AddTestCase here. AddTestCase is a function that is defined
     * in shell.js and takes three arguments:
     * - a string representation of what is being tested
     * - the expected result
     * - the actual result
     *
     * For example, a test might look like this:
     *
     * var zip = /[\d]{5}$/;
     *
     * AddTestCase(
     * "zip = /[\d]{5}$/; \"PO Box 12345 Boston, MA 02134\".match(zip)",   // description of the test
     *  "02134",                                                           // expected result
     *  "PO Box 12345 Boston, MA 02134".match(zip) );                      // actual result
     *
     */

    test();       // leave this alone.  this executes the test cases and
                  // displays results.
