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
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): brendan@mozilla.org, pschwartau@netscape.com
* Date: 09 October 2001
*
* SUMMARY: Regression test for Bugzilla bug 99663
* See http://bugzilla.mozilla.org/show_bug.cgi?id=99663
*
*******************************************************************************
*******************************************************************************
* ESSENTIAL!: this test should contain, or be loaded after, a call to
*
*                         version(120);
*
* Only JS version 1.2 or less has the behavior we're expecting here -
*
* Brendan: "The JS_SetVersion stickiness is necessary for tests such as
* this one to work properly. I think the existing js/tests have been lucky
* in dodging the buggy way that JS_SetVersion's effect can be undone by
* function return."
*
* Note: it is the function statements for f1(), etc. that MUST be compiled
* in JS version 1.2 or less for the test to pass -
*
*******************************************************************************
*******************************************************************************
*
*
* NOTE: the test uses the |it| object of SpiderMonkey; don't run it in Rhino -
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 99663;
var summary = 'Regression test for Bugzilla bug 99663';
/*
 * This testcase expects error messages containing
 * the phrase 'read-only' or something similar -
 */
var READONLY = /read\s*-?\s*only/;
var READONLY_TRUE = 'a "read-only" error';
var READONLY_FALSE = 'Error: ';
var FAILURE = 'NO ERROR WAS GENERATED!';
var status = '';
var actual = '';
var expect= '';
var statusitems = [];
var expectedvalues = [];
var actualvalues = [];


/*
 * These MUST be compiled in JS1.2 or less for the test to work - see above
 */
function f1()
{
  with (it)
  {
    for (rdonly in this);
  }
}


function f2()
{
  for (it.rdonly in this);
}


function f3(s)
{
  for (it[s] in this);
}



/*
 * Begin testing by capturing actual vs. expected values.
 * Initialize to FAILURE; this will get reset if all goes well -
 */
actual = FAILURE;
try
{
  f1();
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 1 of test - got ' + actual;
addThis();


actual = FAILURE;
try
{
  f2();
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 2 of test - got ' + actual;
addThis();


actual = FAILURE;
try
{
  f3('rdonly');
}
catch(e)
{
  actual = readOnly(e.message);
}
expect= READONLY_TRUE;
status = 'Section 3 of test - got ' + actual;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function readOnly(msg)
{
  if (msg.match(READONLY))
    return READONLY_TRUE;
  return READONLY_FALSE + msg;
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  writeLineToLog ('Bug Number ' + bug);
  writeLineToLog ('STATUS: ' + summary);

  for (var i=0; i<UBound; i++)
  {
    writeTestCaseResult(expectedvalues[i], actualvalues[i], statusitems[i]);
  }
}
