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
* Contributor(s): pschwartau@netscape.com  
* Date: 26 November 2000
*
*
* SUMMARY: Passing (RegExp object, flag) to RegExp() function. 
* This test arose from Bugzilla bug 61266. The ECMA3 section is:       
*
* 15.10.3 The RegExp Constructor Called as a Function
*
*   15.10.3.1 RegExp(pattern, flags)
* 
*   If pattern is an object R whose [[Class]] property is "RegExp"
*   and flags is undefined, then return R unchanged.  Otherwise  
*   call the RegExp constructor (section 15.10.4.1),  passing it the
*   pattern and flags arguments and return  the object constructed
*   by that constructor.
*
*
* The current test will check the first scenario outlined above:
*
*   "pattern" is itself a RegExp object R
*   "flags" is undefined
*
* This test is identical to test 15.10.3.1-1.js, except here we do:
*
*                     RegExp(R, undefined);
*
* instead of:
*
*                     RegExp(R); 
*
*
* We check that RegExp(R, undefined) returns R  -
*/
//-------------------------------------------------------------------------------------------------
var bug = '61266';
var summary = 'Passing (RegExp object,flag) to RegExp() function';
var statprefix = 'RegExp(new RegExp(';
var comma =  ', '; var singlequote = "'"; var closeparens = '))';
var cnSUCCESS = 'RegExp() returned the supplied RegExp object';
var cnFAILURE =  'RegExp() did NOT return the supplied RegExp object';
var i = -1; var j = -1; var s = ''; var f = '';
var obj = {};
var status = ''; var actual = ''; var expect = '';
var patterns = new Array();
var flags = new Array();  


// various regular expressions to try -
patterns[0] = '';
patterns[1] = 'abc';
patterns[2] = '(.*)(3-1)\s\w';
patterns[3] = '(.*)(...)\\s\\w';
patterns[4] = '[^A-Za-z0-9_]';
patterns[5] = '[^\f\n\r\t\v](123.5)([4 - 8]$)';

// various flags to try -
flags[0] = 'i';
flags[1] = 'g';
flags[2] = 'm';
flags[3] = undefined;



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function test() 
{
  enterFunc ('test'); 
  printBugNumber (bug);
  printStatus (summary);

  for (i in patterns)
  {
    s = patterns[i];

    for (j in flags)
    {
      f = flags[j];
      status = getStatus(s, f);
      obj = new RegExp(s, f);   

      actual = (obj == RegExp(obj, undefined))? cnSUCCESS : cnFAILURE ;
      expect = cnSUCCESS;
      reportCompare (expect, actual, status);
    }
  }
  
  exitFunc ('test');
}


function getStatus(regexp, flag)
{ 
  return (statprefix  +  quote(regexp) +  comma  +   flag  +  closeparens); 
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}