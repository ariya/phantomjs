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
	Filename:     general1.js
	Description:  'This tests out some of the functionality on methods on the Array objects'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'String:push,unshift,shift';

	writeHeaderToLog('Executing script: general1.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();

	var array1 = [];

	array1.push(123);            //array1 = [123]
	array1.push("dog");          //array1 = [123,dog]
	array1.push(-99);            //array1 = [123,dog,-99]
	array1.push("cat");          //array1 = [123,dog,-99,cat]
	testcases[count++] = new TestCase( SECTION, "array1.pop()", array1.pop(),'cat');
	                             //array1 = [123,dog,-99]
	array1.push("mouse");        //array1 = [123,dog,-99,mouse]
	testcases[count++] = new TestCase( SECTION, "array1.shift()", array1.shift(),123);
	                             //array1 = [dog,-99,mouse]
	array1.unshift(96);          //array1 = [96,dog,-99,mouse]
	testcases[count++] = new TestCase( SECTION, "state of array", String([96,"dog",-99,"mouse"]), String(array1));
	testcases[count++] = new TestCase( SECTION, "array1.length", array1.length,4);
	array1.shift();              //array1 = [dog,-99,mouse]
	array1.shift();              //array1 = [-99,mouse]
	array1.shift();              //array1 = [mouse]
	testcases[count++] = new TestCase( SECTION, "array1.shift()", array1.shift(),"mouse");
	testcases[count++] = new TestCase( SECTION, "array1.shift()", "undefined", String(array1.shift()));

	test();

