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
* SUMMARY: Testing the [[Class]] property of native constructors.
* See ECMA-262 Edition 3 13-Oct-1999, Section 8.6.2 re [[Class]] property.
*
* Same as class-001.js - but testing the constructors here, not object instances.
* Therefore we expect the [[Class]] property to equal 'Function' in each case. 
*
* The getJSClass() function we use is in a utility file, e.g. "shell.js"
*/
//-------------------------------------------------------------------------------------------------
var i = 0;
var UBound = 0;
var bug = '(none)';
var summary = 'Testing the internal [[Class]] property of native constructors';
var statprefix = 'Current constructor is: ';
var status = ''; var statusList = [ ];
var actual = ''; var actualvalue = [ ];
var expect= ''; var expectedvalue = [ ];

/*
 * We set the expect variable each time only for readability.
 * We expect 'Function' every time; see discussion above -
 */
status = 'Object';
actual = getJSClass(Object);
expect = 'Function';
addThis();

status = 'Function';
actual = getJSClass(Function);
expect = 'Function';
addThis();

status = 'Array';
actual = getJSClass(Array);
expect = 'Function';
addThis();

status = 'String';
actual = getJSClass(String);
expect = 'Function';
addThis();

status = 'Boolean';
actual = getJSClass(Boolean);
expect = 'Function';
addThis();

status = 'Number';
actual = getJSClass(Number);
expect = 'Function';
addThis();

status = 'Date';
actual = getJSClass(Date);
expect = 'Function';
addThis();

status = 'RegExp';
actual = getJSClass(RegExp);
expect = 'Function';
addThis();

status = 'Error';
actual = getJSClass(Error);
expect = 'Function';
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
