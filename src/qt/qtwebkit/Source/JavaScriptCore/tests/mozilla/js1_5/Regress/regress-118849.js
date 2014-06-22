/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2001
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    08 Jan 2002
* SUMMARY: Just testing that we don't crash on this code
* See http://bugzilla.mozilla.org/show_bug.cgi?id=118849
*
* http://developer.netscape.com:80/docs/manuals/js/core/jsref/function.htm
* The Function constructor:
* Function ([arg1[, arg2[, ... argN]],] functionBody)
*
* Parameters
* arg1, arg2, ... argN
*   (Optional) Names to be used by the function as formal argument names.
*   Each must be a string that corresponds to a valid JavaScript identifier.
*
* functionBody 
*   A string containing JS statements comprising the function definition.
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 118849;
var summary = 'Should not crash if we provide Function() with bad arguments'
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var cnFAIL_1 = 'LEGAL call to Function() caused an ERROR!!!';
var cnFAIL_2 = 'ILLEGAL call to Function() FAILED to cause an error';
var cnSTRING = 'ASDF';
var cnNUMBER = 123;


/***********************************************************/
/****  THESE ARE LEGITMATE CALLS AND SHOULD ALL SUCCEED  ***/
/***********************************************************/
status = inSection(1);
actual = cnFAIL_1; // initialize to failure
try
{
  Function(cnSTRING);
  Function(cnNUMBER);  // cnNUMBER is a valid functionBody        
  Function(cnSTRING,cnSTRING);
  Function(cnSTRING,cnNUMBER);
  Function(cnSTRING,cnSTRING,cnNUMBER);

  new Function(cnSTRING);
  new Function(cnNUMBER);
  new Function(cnSTRING,cnSTRING);
  new Function(cnSTRING,cnNUMBER);
  new Function(cnSTRING,cnSTRING,cnNUMBER);

  actual = expect;
}
catch(e)
{
}
addThis();



/**********************************************************/
/***  EACH CASE THAT FOLLOWS SHOULD TRIGGER AN ERROR    ***/
/***               (BUT NOT A CRASH)                    ***/
/***  NOTE WE NOW USE cnFAIL_2 INSTEAD OF cnFAIL_1      ***/
/**********************************************************/
status = inSection(2);
actual = cnFAIL_2;
try
{
  Function(cnNUMBER,cnNUMBER); // cnNUMBER is an invalid JS identifier name
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(3);
actual = cnFAIL_2;
try
{
  Function(cnNUMBER,cnSTRING,cnSTRING);
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(4);
actual = cnFAIL_2;
try
{
  new Function(cnNUMBER,cnNUMBER);
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(5);
actual = cnFAIL_2;
try
{
  new Function(cnNUMBER,cnSTRING,cnSTRING);
}
catch(e)
{
  actual = expect;
}
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
 
  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
