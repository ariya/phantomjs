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
* Date: 09 May 2001
*
* SUMMARY: Regression test: we shouldn't crash on this code
* See http://bugzilla.mozilla.org/show_bug.cgi?id=72773
*
* See ECMA-262 Edition 3 13-Oct-1999, Section 8.6.2 re [[Class]] property.
*
* Same as class-001.js - but testing user-defined types here, not native types.
* Therefore we expect the [[Class]] property to equal 'Object' in each case -
*
* The getJSClass() function we use is in a utility file, e.g. "shell.js"
*/
//-------------------------------------------------------------------------------------------------
var bug = 72773;
var summary = "Regression test: we shouldn't crash on this code";
var status = '';
var actual = '';
var expect = '';
var sToEval = '';

/*
 * This code should produce an error, but not a crash.
 *  'TypeError: Function.prototype.toString called on incompatible object'
 */
sToEval += 'function Cow(name){this.name = name;}'
sToEval += 'function Calf(str){this.name = str;}'
sToEval += 'Calf.prototype = Cow;'
sToEval += 'new Calf().toString();'

status = 'Trying to catch an expected error';
try
{
  eval(sToEval);
}
catch(e)
{
  actual = getJSClass(e);
  expect = 'Error';
}


//----------------------------------------------------------------------------------------------
test();
//----------------------------------------------------------------------------------------------


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  reportCompare(expect, actual, status);

  exitFunc ('test');
}
