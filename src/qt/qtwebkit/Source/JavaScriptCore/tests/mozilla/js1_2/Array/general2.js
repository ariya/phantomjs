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
	Filename:     general2.js
	Description:  'This tests out some of the functionality on methods on the Array objects'

	Author:       Nick Lerissa
	Date:         Fri Feb 13 09:58:28 PST 1998
*/

	var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
	var VERSION = 'no version';
    startTest();
	var TITLE = 'String:push,splice,concat,unshift,sort';

	writeHeaderToLog('Executing script: general2.js');
	writeHeaderToLog( SECTION + " "+ TITLE);

	var count = 0;
	var testcases = new Array();


	array1 = new Array();
	array2 = [];
	size   = 10;

	// this for loop populates array1 and array2 as follows:
	// array1 = [0,1,2,3,4,....,size - 2,size - 1]
	// array2 = [size - 1, size - 2,...,4,3,2,1,0]
	for (var i = 0; i < size; i++)
	{
		array1.push(i);
		array2.push(size - 1 - i);
	}

	// the following for loop reverses the order of array1 so
	// that it should be similarly ordered to array2
	for (i = array1.length; i > 0; i--)
	{
		array3 = array1.slice(1,i);
		array1.splice(1,i-1);
		array1 = array3.concat(array1);
	}

	// the following for loop reverses the order of array1
	// and array2
	for (i = 0; i < size; i++)
	{
	    array1.push(array1.shift());
	    array2.unshift(array2.pop());
	}

	testcases[count++] = new TestCase( SECTION, "Array.push,pop,shift,unshift,slice,splice", true,String(array1) == String(array2));
	array1.sort();
	array2.sort();
	testcases[count++] = new TestCase( SECTION, "Array.sort", true,String(array1) == String(array2));

	test();

