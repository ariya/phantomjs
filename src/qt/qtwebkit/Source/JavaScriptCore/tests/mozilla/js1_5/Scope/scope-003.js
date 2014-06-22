/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an
* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): coliver@mminternet.com, pschwartau@netscape.com
* Date: 2001-07-03
*
* SUMMARY:  Testing scope with nested functions
*
* From correspondence with Christopher Oliver <coliver@mminternet.com>:
*
* > Running this test with Rhino produces the following exception:
* >
* > uncaught JavaScript exception: undefined: Cannot find default value for
* > object. (line 3)
* >
* > This is due to a bug in org.mozilla.javascript.NativeCall which doesn't
* > implement toString or valueOf or override getDefaultValue.
* > However, even after I hacked in an implementation of getDefaultValue in
* > NativeCall, Rhino still produces a different result then SpiderMonkey:
* >
* > [object Call]
* > [object Object]
* > [object Call]
*
* Note the results should be:
*
*   [object global]
*   [object Object]
*   [object global]
*
* This is what we are checking for in this testcase -
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing scope with nested functions';
var statprefix = 'Section ';
var statsuffix = ' of test -';
var self = this; // capture a reference to the global object;
var cnGlobal = self.toString();
var cnObject = (new Object).toString();
var statusitems = [];
var actualvalues = [];
var expectedvalues = [];


function a()
{
  function b()
  {
    capture(this.toString());
  }

  this.c = function()
  {
    capture(this.toString());
    b();
  }

  b();
}


var obj = new a();  // captures actualvalues[0]
obj.c();            // captures actualvalues[1], actualvalues[2]


// The values we expect - see introduction above -
expectedvalues[0] = cnGlobal;
expectedvalues[1] = cnObject;
expectedvalues[2] = cnGlobal;



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function capture(val)
{
  actualvalues[UBound] = val;
  statusitems[UBound] = getStatus(UBound);
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
 
  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statprefix + i + statsuffix;
}
