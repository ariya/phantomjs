/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" 
* basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 14 Mar 2001
*
* SUMMARY: Testing the internal [[Class]] property of objects
* See ECMA-262 Edition 3 13-Oct-1999, Section 8.6.2
*
* The getJSClass() function we use is in a utility file, e.g. "shell.js".
*/
//-------------------------------------------------------------------------------------------------
var i = 0;
var UBound = 0;
var bug = '(none)';
var summary = 'Testing the internal [[Class]] property of objects';
var statprefix = 'Current object is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];


status = 'the global object';
actual = getJSClass(this);
expect = 'global';
addThis();

status = 'new Object()';
actual = getJSClass(new Object());
expect = 'Object';
addThis();

status = 'new Function()';
actual = getJSClass(new Function());
expect = 'Function';
addThis();

status = 'new Array()';
actual = getJSClass(new Array());
expect = 'Array';
addThis();

status = 'new String()';
actual = getJSClass(new String());
expect = 'String';
addThis();

status = 'new Boolean()';
actual = getJSClass(new Boolean());
expect = 'Boolean';
addThis();

status = 'new Number()';
actual = getJSClass(new Number());
expect = 'Number';
addThis();

status = 'Math';
actual = getJSClass(Math);  // can't use 'new' with the Math object (EMCA3, 15.8)
expect = 'Math';
addThis();

status = 'new Date()';
actual = getJSClass(new Date());
expect = 'Date';
addThis();

status = 'new RegExp()';
actual = getJSClass(new RegExp());
expect = 'RegExp';
addThis();

status = 'new Error()';
actual = getJSClass(new Error());
expect = 'Error';
addThis();



//---------------------------------------------------------------------------------
test();
//---------------------------------------------------------------------------------



function addThis()
{
  statusList[UBound] = status;
  actualvalue[UBound] = actual;
  expectedvalue[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
 
  for (i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalue[i], actualvalue[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statprefix + statusList[i];
}
