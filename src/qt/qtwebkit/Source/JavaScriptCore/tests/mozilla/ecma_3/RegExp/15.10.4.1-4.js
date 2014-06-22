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
*SUMMARY: Passing a RegExp object to a RegExp() constructor. 
*This test arose from Bugzilla bug 61266. The ECMA3 section is:       
*
*  15.10.4.1 new RegExp(pattern, flags)
*
*  If pattern is an object R whose [[Class]] property is "RegExp" and 
*  flags is undefined, then let P be the pattern used to construct R 
*  and let F be the flags used to construct R. If pattern is an object R
*  whose [[Class]] property is "RegExp" and flags is not undefined, 
*  then throw a TypeError exception. Otherwise, let P be the empty string  
*  if pattern is undefined and ToString(pattern) otherwise, and let F be 
*  the empty string if flags is undefined and ToString(flags) otherwise.
*
*
*The current test will check the first scenario outlined above:
*
*   "pattern" is itself a RegExp object R
*   "flags"  is undefined 
*
* We check that a new RegExp object obj2 defined from these parameters
* is morally the same as the original RegExp object obj1. Of course, they 
* can't be equal as objects - so we check their enumerable properties...
*
* In this test, the initial RegExp object obj1 will include a flag. This test is 
* identical to test 15.10.4.1-3.js, except that here we use this syntax:
*
*                  obj2 = new RegExp(obj1, undefined);
*
* instead of:
*
*                  obj2 = new RegExp(obj1); 
*/
//-------------------------------------------------------------------------------------------------
var bug = '61266';
var summary = 'Passing a RegExp object to a RegExp() constructor';
var statprefix = 'Applying RegExp() twice to pattern ';
var statmiddle = ' and flag ';
var statsuffix =  '; testing property ';
var singlequote = "'";
var i = -1; var j = -1; var s = '';
var obj1 = {}; var obj2 = {};
var status = ''; var actual = ''; var expect = ''; var msg = '';
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
      obj1 = new RegExp(s, f); 
      obj2 = new RegExp(obj1, undefined);  // see introduction to bug 
  
      for (prop in obj2)
      { 
        msg  = status +  quote(prop); 
        actual = obj2[prop]; 
        expect = obj1[prop];
        reportCompare (expect, actual, msg);
      }
    }
  }

  exitFunc ('test');
}


function getStatus(regexp, flag)
{ 
  return (statprefix  +  quote(regexp) +  statmiddle  +  flag  +  statsuffix); 
}


function quote(text)
{
  return (singlequote  +  text  + singlequote);
}