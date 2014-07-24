/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

// requires jQuery

const kTestSuiteVersion = '20101001';
const kTestSuiteHome = '../' + kTestSuiteVersion + '/';
const kTestInfoDataFile = 'testinfo.data';

const kChapterData = [
  {
    'file' : 'about.html',
    'title' : 'About the CSS 2.1 Specification',
  },
  {
    'file' : 'intro.html',
    'title' : 'Introduction to CSS 2.1',
  },
  {
    'file' : 'conform.html',
    'title' : 'Conformance: Requirements and Recommendations',
  },
  {
    'file' : "syndata.html",
    'title' : 'Syntax and basic data types',
  },
  {
    'file' : 'selector.html' ,
    'title' : 'Selectors',
  },
  {
    'file' : 'cascade.html',
    'title' : 'Assigning property values, Cascading, and Inheritance',
  },
  {
    'file' : 'media.html',
    'title' : 'Media types',
  },
  {
    'file' : 'box.html' ,
    'title' : 'Box model',
  },
  {
    'file' : 'visuren.html',
    'title' : 'Visual formatting model',
  },
  {
    'file' :'visudet.html',
    'title' : 'Visual formatting model details',
  },
  {
    'file' : 'visufx.html',
    'title' : 'Visual effects',
  },
  {
    'file' : 'generate.html',
    'title' : 'Generated content, automatic numbering, and lists',
  },
  {
    'file' : 'page.html',
    'title' : 'Paged media',
  },
  {
    'file' : 'colors.html',
    'title' : 'Colors and Backgrounds',
  },
  {
    'file' : 'fonts.html',
    'title' : 'Fonts',
  },
  {
    'file' : 'text.html',
    'title' : 'Text',
  },
  {
    'file' : 'tables.html',
    'title' : 'Tables',
  },
  {
    'file' : 'ui.html',
    'title' : 'User interface',
  },
  {
    'file' : 'aural.html',
    'title' : 'Appendix A. Aural style sheets',
  },
  {
    'file' : 'refs.html',
    'title' : 'Appendix B. Bibliography',
  },
  {
    'file' : 'changes.html',
    'title' : 'Appendix C. Changes',
  },
  {
    'file' : 'sample.html',
    'title' : 'Appendix D. Default style sheet for HTML 4',
  },
  {
    'file' : 'zindex.html',
    'title' : 'Appendix E. Elaborate description of Stacking Contexts',
  },
  {
    'file' : 'propidx.html',
    'title' : 'Appendix F. Full property table',
  },
  {
    'file' : 'grammar.html',
    'title' : 'Appendix G. Grammar of CSS',
  },
  {
    'file' : 'other.html',
    'title' : 'Other',
  },
];


const kHTML4Data = {
  'path' : 'html4',
  'suffix' : '.htm'
};

const kXHTML1Data = {
  'path' : 'xhtml1',
  'suffix' : '.xht'
};

// Results popup
const kResultsSelector = [
  {
    'name': 'All Tests',
    'handler' : function(self) { self.showResultsForAllTests(); },
    'exporter' : function(self) { self.exportResultsForAllTests(); }
  },
  {
    'name': 'Completed Tests',
    'handler' : function(self) { self.showResultsForCompletedTests(); },
    'exporter' : function(self) { self.exportResultsForCompletedTests(); }
  },
  {
    'name': 'Passing Tests',
    'handler' : function(self) { self.showResultsForTestsWithStatus('pass'); },
    'exporter' : function(self) { self.exportResultsForTestsWithStatus('pass'); }
  },
  {
    'name': 'Failing Tests',
    'handler' : function(self) { self.showResultsForTestsWithStatus('fail'); },
    'exporter' : function(self) { self.exportResultsForTestsWithStatus('fail'); }
  },
  {
    'name': 'Skipped Tests',
    'handler' : function(self) { self.showResultsForTestsWithStatus('skipped'); },
    'exporter' : function(self) { self.exportResultsForTestsWithStatus('skipped'); }
  },
  {
    'name': 'Invalid Tests',
    'handler' : function(self) { self.showResultsForTestsWithStatus('invalid'); },
    'exporter' : function(self) { self.exportResultsForTestsWithStatus('invalid'); }
  },
  {
    'name': 'Tests where HTML4 and XHTML1 results differ',
    'handler' : function(self) { self.showResultsForTestsWithMismatchedResults(); },
    'exporter' : function(self) { self.exportResultsForTestsWithMismatchedResults(); }
  },
  {
    'name': 'Tests Not Run',
    'handler' : function(self) { self.showResultsForTestsNotRun(); },
    'exporter' : function(self) { self.exportResultsForTestsNotRun(); }
  }
];

function Test(testInfoLine)
{
  var fields = testInfoLine.split('\t');
  
  this.id = fields[0];
  this.reference = fields[1];
  this.title = fields[2];
  this.flags = fields[3];
  this.links = fields[4];
  this.assertion = fields[5];

  this.paged = false;
  this.testHTML = true;
  this.testXHTML = true;

  if (this.flags) {
    this.paged = this.flags.indexOf('paged') != -1;
  
    if (this.flags.indexOf('nonHTML') != -1)
      this.testHTML = false;

    if (this.flags.indexOf('HTMLonly') != -1)
      this.testXHTML = false;
  }
  
  this.completedHTML = false; // true if this test has a result (pass, fail or skip)
  this.completedXHTML = false; // true if this test has a result (pass, fail or skip)

  this.statusHTML = '';
  this.statusXHTML = '';
  
  if (!this.links)
    this.links = "other.html"
}

Test.prototype.runForFormat = function(format)
{
  if (format == 'html4')
    return this.testHTML;

  if (format == 'xhtml1')
    return this.testXHTML;

  return true;
}

Test.prototype.completedForFormat = function(format)
{
  if (format == 'html4')
    return this.completedHTML;

  if (format == 'xhtml1')
    return this.completedXHTML;

  return true;
}

Test.prototype.statusForFormat = function(format)
{
  if (format == 'html4')
    return this.statusHTML;

  if (format == 'xhtml1')
    return this.statusXHTML;

  return true;
}

function ChapterSection(link)
{
  var result= link.match(/^([.\w]+)(#.+)?$/);
  if (result != null) {
    this.file = result[1];
    this.anchor = result[2];
  }

  this.testCountHTML = 0;
  this.testCountXHTML = 0;

  this.tests = [];
}

ChapterSection.prototype.countTests = function()
{
  this.testCountHTML = 0;
  this.testCountXHTML = 0;

  for (var i = 0; i < this.tests.length; ++i) {
    var currTest = this.tests[i];

    if (currTest.testHTML)
      ++this.testCountHTML;

    if (currTest.testXHTML)
      ++this.testCountXHTML;
  }
}

function Chapter(chapterInfo)
{
  this.file = chapterInfo.file;
  this.title = chapterInfo.title;
  this.testCountHTML = 0;
  this.testCountXHTML = 0;
  this.sections = []; // array of ChapterSection
}

Chapter.prototype.description = function(format)
{
  
  
  return this.title + ' (' + this.testCount(format) + ' tests, ' + this.untestedCount(format) + ' untested)';
}

Chapter.prototype.countTests = function()
{
  this.testCountHTML = 0;
  this.testCountXHTML = 0;

  for (var i = 0; i < this.sections.length; ++i) {
    var currSection = this.sections[i];

    currSection.countTests();

    this.testCountHTML += currSection.testCountHTML;
    this.testCountXHTML += currSection.testCountXHTML;
  }
}

Chapter.prototype.testCount = function(format)
{
  if (format == 'html4')
    return this.testCountHTML;

  if (format == 'xhtml1')
    return this.testCountXHTML;

  return 0;
}

Chapter.prototype.untestedCount = function(format)
{
  var completedProperty = format == 'html4' ? 'completedHTML' : 'completedXHTML';
  
  var count = 0;
  for (var i = 0; i < this.sections.length; ++i) {
    var currSection = this.sections[i];
    for (var j = 0; j < currSection.tests.length; ++j) {
      count += currSection.tests[j].completedForFormat(format) ? 0 : 1;
    }
  }
  return count;
  
}

// Utils
String.prototype.trim = function() { return this.replace(/^\s+|\s+$/g, ''); }

function TestSuite()
{
  this.chapterSections = {}; // map of links to ChapterSections
  this.tests = {}; // map of test id to test info
  
  this.chapters = {}; // map of file name to chapter
  this.currentChapter = null;
  
  this.currentChapterTests = []; // array of tests for the current chapter.
  this.currChapterTestIndex = -1; // index of test in the current chapter
  
  this.format = '';
  this.formatChanged('html4');
  
  this.testInfoLoaded = false;

  this.populatingDatabase = false;
  
  var testInfoPath = kTestSuiteHome + kTestInfoDataFile;
  this.loadTestInfo(testInfoPath);
}

TestSuite.prototype.loadTestInfo = function(testInfoPath)
{
  var _self = this;
  this.asyncLoad(testInfoPath, 'data', function(data, status) {
    _self.testInfoDataLoaded(data, status);
  });
}

TestSuite.prototype.testInfoDataLoaded = function(data, status)
{
  if (status != 'success') {
    alert("Failed to load testinfo.data. Database of tests will not be initialized.");
    return;
  }

  this.parseTests(data);
  this.buildChapters();

  this.testInfoLoaded = true;
  
  this.fillChapterPopup();

  this.initializeControls();

  this.openDatabase();
}

TestSuite.prototype.parseTests = function(data)
{
  var lines = data.split('\n');
  
  // First line is column labels
  for (var i = 1; i < lines.length; ++i) {
    var test = new Test(lines[i]);
    if (test.id.length > 0)
      this.tests[test.id] = test;
  }
}

TestSuite.prototype.buildChapters = function()
{
  for (var testID in this.tests) {
    var currTest = this.tests[testID];

    // FIXME: tests with more than one link will be presented to the user
    // twice. Be smarter about avoiding this.
    var testLinks = currTest.links.split(',');
    for (var i = 0; i < testLinks.length; ++i) {
      var link = testLinks[i];
      var section = this.chapterSections[link];
      if (!section) {
        section = new ChapterSection(link);
        this.chapterSections[link] = section;
      }
      
      section.tests.push(currTest);
    }
  }
  
  for (var i = 0; i < kChapterData.length; ++i) {
    var chapter = new Chapter(kChapterData[i]);
    chapter.index = i;
    this.chapters[chapter.file] = chapter;
  }
  
  for (var sectionName in this.chapterSections) {
    var section = this.chapterSections[sectionName];

    var file = section.file;
    var chapter = this.chapters[file];
    if (!chapter)
      window.console.log('failed to find chapter ' + file + ' in chapter data.');
    chapter.sections.push(section);
  }
  
  for (var chapterName in this.chapters) {
    var currChapter = this.chapters[chapterName];
    currChapter.sections.sort();
    currChapter.countTests();
  }
}

TestSuite.prototype.indexOfChapter = function(chapter)
{
  for (var i = 0; i < kChapterData.length; ++i) {
    if (kChapterData[i].file == chapter.file)
      return i;
  }
  
  window.console.log('indexOfChapter for ' + chapter.file + ' failed');
  return -1;
}

TestSuite.prototype.chapterAtIndex = function(index)
{
  if (index < 0 || index >= kChapterData.length)
    return null;

  return this.chapters[kChapterData[index].file];
}

TestSuite.prototype.fillChapterPopup = function()
{
  var select = document.getElementById('chapters')
  select.innerHTML = ''; // Remove all children.
  
  for (var i = 0; i < kChapterData.length; ++i) {
    var chapterData = kChapterData[i];
    var chapter = this.chapters[chapterData.file];
    
    var option = document.createElement('option');
    option.innerText = chapter.description(this.format);
    option._chapter = chapter;
    
    select.appendChild(option);
  }
}

TestSuite.prototype.updateChapterPopup = function()
{
  var select = document.getElementById('chapters')
  var currOption = select.firstChild;
  
  for (var i = 0; i < kChapterData.length; ++i) {
    var chapterData = kChapterData[i];
    var chapter = this.chapters[chapterData.file];
    if (!chapter)
      continue;
    currOption.innerText = chapter.description(this.format);
    currOption = currOption.nextSibling;
  }
}

TestSuite.prototype.buildTestListForChapter = function(chapter)
{
  this.currentChapterTests = this.testListForChapter(chapter);
}

TestSuite.prototype.testListForChapter = function(chapter)
{
  var testList = [];
  
  for (var i in chapter.sections) {
    var currSection = chapter.sections[i];
    
    for (var j = 0; j < currSection.tests.length; ++j) {
      var currTest = currSection.tests[j];
      if (currTest.runForFormat(this.format))
        testList.push(currTest);
    }
  }
  
  // FIXME: test may occur more than once.
  testList.sort(function(a, b) {
    return a.id.localeCompare(b.id);
  });
  
  return testList;
}

TestSuite.prototype.initializeControls = function()
{
  var chaptersPopup = document.getElementById('chapters');

  var _self = this;
  chaptersPopup.addEventListener('change', function() {
    _self.chapterPopupChanged();
  }, false);

  this.chapterPopupChanged();
  
  // Results popup
  var resultsPopup = document.getElementById('results-popup');
  resultsPopup.innerHTML = '';
  
  for (var i = 0; i < kResultsSelector.length; ++i) {
    var option = document.createElement('option');
    option.innerText =  kResultsSelector[i].name;
    
    resultsPopup.appendChild(option);
  }
}

TestSuite.prototype.chapterPopupChanged = function()
{
  var chaptersPopup = document.getElementById('chapters');
  var selectedChapter = chaptersPopup.options[chaptersPopup.selectedIndex]._chapter;

  this.setSelectedChapter(selectedChapter);
}

TestSuite.prototype.fillTestList = function()
{
  var statusProperty = this.format == 'html4' ? 'statusHTML' : 'statusXHTML';

  var testList = document.getElementById('test-list');
  testList.innerHTML = '';
  
  for (var i = 0; i < this.currentChapterTests.length; ++i) {
    var currTest = this.currentChapterTests[i];

    var option = document.createElement('option');
    option.innerText = currTest.id;
    option.className = currTest[statusProperty];
    option._test = currTest;
    testList.appendChild(option);
  }
}

TestSuite.prototype.updateTestList = function()
{
  var statusProperty = this.format == 'html4' ? 'statusHTML' : 'statusXHTML';
  var testList = document.getElementById('test-list');
  
  var options = testList.getElementsByTagName('option');
  for (var i = 0; i < options.length; ++i) {
    var currOption = options[i];
    currOption.className = currOption._test[statusProperty];
  }
}

TestSuite.prototype.setSelectedChapter = function(chapter)
{
  this.currentChapter = chapter;
  this.buildTestListForChapter(this.currentChapter);
  this.currChapterTestIndex = -1;
  
  this.fillTestList();
  this.goToTestIndex(0);
  
  var chaptersPopup = document.getElementById('chapters');
  chaptersPopup.selectedIndex = this.indexOfChapter(chapter);
}

/* ------------------------------------------------------- */

TestSuite.prototype.passTest = function()
{
  this.recordResult(this.currentTestName(), 'pass');
  this.nextTest();
}

TestSuite.prototype.failTest = function()
{
  this.recordResult(this.currentTestName(), 'fail');
  this.nextTest();
}

TestSuite.prototype.invalidTest = function()
{
  this.recordResult(this.currentTestName(), 'invalid');
  this.nextTest();
}

TestSuite.prototype.skipTest = function(reason)
{
  this.recordResult(this.currentTestName(), 'skipped', reason);
  this.nextTest();
}

TestSuite.prototype.nextTest = function()
{
  if (this.currChapterTestIndex < this.currentChapterTests.length - 1)
    this.goToTestIndex(this.currChapterTestIndex + 1);
  else {
    var currChapterIndex = this.indexOfChapter(this.currentChapter);
    this.goToChapterIndex(currChapterIndex + 1);
  }
}

TestSuite.prototype.previousTest = function()
{
  if (this.currChapterTestIndex > 0)
    this.goToTestIndex(this.currChapterTestIndex - 1);
  else {
    var currChapterIndex = this.indexOfChapter(this.currentChapter);
    if (currChapterIndex > 0)
      this.goToChapterIndex(currChapterIndex - 1);
  }
}

TestSuite.prototype.goToNextIncompleteTest = function()
{
  var completedProperty = this.format == 'html4' ? 'completedHTML' : 'completedXHTML';

  // Look to the end of this chapter.
  for (var i = this.currChapterTestIndex + 1; i < this.currentChapterTests.length; ++i) {
    if (!this.currentChapterTests[i][completedProperty]) {
      this.goToTestIndex(i);
      return;
    }
  }

  // Start looking through later chapter
  var currChapterIndex = this.indexOfChapter(this.currentChapter);
  for (var c = currChapterIndex + 1; c < kChapterData.length; ++c) {
    var chapterData = this.chapterAtIndex(c);
    
    var testIndex = this.firstIncompleteTestIndex(chapterData);
    if (testIndex != -1) {
      this.goToChapterIndex(c);
      this.goToTestIndex(testIndex);
      break;
    }
  }
}

TestSuite.prototype.firstIncompleteTestIndex = function(chapter)
{
  var completedProperty = this.format == 'html4' ? 'completedHTML' : 'completedXHTML';

  var chapterTests = this.testListForChapter(chapter);
  for (var i = 0; i < chapterTests.length; ++i) {
    if (!chapterTests[i][completedProperty])
      return i;
  }
  
  return -1;
}

/* ------------------------------------------------------- */

TestSuite.prototype.goToTestByName = function(testName)
{
  var match = testName.match(/^(?:(html4|xhtml1)\/)?([\w-_]+)(\.xht|\.htm)?/);
  if (!match)
    return false;

  var prefix = match[1];
  var testId = match[2];
  var extension = match[3];
  
  var format = this.format;
  if (prefix)
    format = prefix;
  else if (extension) {
    if (extension == kXHTML1Data.suffix)
      format = kXHTML1Data.path;
    else if (extension == kHTML4Data.suffix)
      format = kHTML4Data.path;
  }
  
  this.switchToFormat(format);
  
  var test = this.tests[testId];
  if (!test)
    return false;

  // Find the first chapter.
  var links = test.links.split(',');
  if (links.length == 0) {
    window.console.log('test ' + test.id + 'had no links.');
    return false;
  }

  var firstLink = links[0];
  var result = firstLink.match(/^([.\w]+)(#.+)?$/);
  if (result)
    firstLink = result[1];

  // Find the chapter and index of the test.
  for (var i = 0; i < kChapterData.length; ++i) {
    var chapterData = kChapterData[i];
    if (chapterData.file == firstLink) {

      this.goToChapterIndex(i);
      
      for (var j = 0; j < this.currentChapterTests.length; ++j) {
        var currTest = this.currentChapterTests[j];
        if (currTest.id == testId) {
          this.goToTestIndex(j);
          return true;
        }
      }
    }
  }

  return false;
}

TestSuite.prototype.goToTestIndex = function(index)
{
  if (index >= 0 && index < this.currentChapterTests.length) {
    this.currChapterTestIndex = index;
    this.loadCurrentTest();
  }
}

TestSuite.prototype.goToChapterIndex = function(chapterIndex)
{
  if (chapterIndex >= 0 && chapterIndex < kChapterData.length) {
    var chapterFile = kChapterData[chapterIndex].file;
    this.setSelectedChapter(this.chapters[chapterFile]);
  }
}

TestSuite.prototype.currentTestName = function()
{
  if (this.currChapterTestIndex < 0 || this.currChapterTestIndex >= this.currentChapterTests.length)
    return undefined;

  return this.currentChapterTests[this.currChapterTestIndex].id;
}

TestSuite.prototype.loadCurrentTest = function()
{
  var theTest = this.currentChapterTests[this.currChapterTestIndex];
  if (!theTest) {
    this.configureForManualTest();
    this.clearTest();
    return;
  }

  if (theTest.reference) {
    this.configureForRefTest();
    this.loadRef(theTest);
  } else {
    this.configureForManualTest();
  }

  this.loadTest(theTest);

  this.updateProgressLabel();
  
  document.getElementById('test-list').selectedIndex = this.currChapterTestIndex;
}

TestSuite.prototype.updateProgressLabel = function()
{
  document.getElementById('test-index').innerText = this.currChapterTestIndex + 1;
  document.getElementById('chapter-test-count').innerText = this.currentChapterTests.length;
}

TestSuite.prototype.configureForRefTest = function()
{
  $('#test-content').addClass('with-ref');
}

TestSuite.prototype.configureForManualTest = function()
{
  $('#test-content').removeClass('with-ref');
}

TestSuite.prototype.loadTest = function(test)
{
  var iframe = document.getElementById('test-frame');
  iframe.src = 'about:blank';
  
  var url = this.urlForTest(test.id);
  window.setTimeout(function() {
    iframe.src = url;
  }, 0);
  
  document.getElementById('test-title').innerText = test.title;
  document.getElementById('test-url').innerText = this.pathForTest(test.id);
  document.getElementById('test-assertion').innerText = test.assertion;
  document.getElementById('test-flags').innerText = test.flags;
  
  this.processFlags(test);
}

TestSuite.prototype.processFlags = function(test)
{ 
  if (test.paged)
    $('#test-content').addClass('print');
  else
    $('#test-content').removeClass('print');

  var showWarning = false;
  var warning = '';
  if (test.flags.indexOf('font') != -1)
    warning = 'Requires a specific font to be installed.';
  
  if (test.flags.indexOf('http') != -1) {
    if (warning != '')
      warning += ' ';
    warning += 'Must be tested over HTTP, with custom HTTP headers.';
  }
  
  if (test.paged) {
    if (warning != '')
      warning += ' ';
    warning += 'Test via the browser\'s Print Preview.';
  }

  document.getElementById('warning').innerText = warning;

  if (warning.length > 0)
    $('#test-content').addClass('warn');
  else
    $('#test-content').removeClass('warn');

}

TestSuite.prototype.clearTest = function()
{
  var iframe = document.getElementById('test-frame');
  iframe.src = 'about:blank';
  
  document.getElementById('test-title').innerText = '';
  document.getElementById('test-url').innerText = '';
  document.getElementById('test-assertion').innerText = '';
  document.getElementById('test-flags').innerText = '';

  $('#test-content').removeClass('print');
  $('#test-content').removeClass('warn');
  document.getElementById('warning').innerText = '';
}

TestSuite.prototype.loadRef = function(test)
{
  // Suites 20101001 and earlier used .xht refs, even for HTML tests, so strip off
  // the extension and use the same format as the test.
  var ref = test.reference.replace(/(\.xht)?$/, '');
  
  var iframe = document.getElementById('ref-frame');
  iframe.src = this.urlForTest(ref);
}

TestSuite.prototype.pathForTest = function(testName)
{
  var prefix = this.formatInfo.path;
  var suffix = this.formatInfo.suffix;
  
  return prefix + '/' + testName + suffix;
}

TestSuite.prototype.urlForTest = function(testName)
{
  return kTestSuiteHome + this.pathForTest(testName);
}

/* ------------------------------------------------------- */

TestSuite.prototype.recordResult = function(testName, resolution, comment)
{
  if (!testName)
    return;

  this.beginAppendingOutput();
  this.appendResultToOutput(this.formatInfo, testName, resolution, comment);
  this.endAppendingOutput();
  
  if (comment == undefined)
    comment = '';
  
  this.storeTestResult(testName, this.format, resolution, comment, navigator.userAgent);
  
  var htmlStatus = null;
  var xhtmlStatus = null;
  if (this.format == 'html4')
    htmlStatus = resolution;
  if (this.format == 'xhtml1')
    xhtmlStatus = resolution;

  this.markTestCompleted(testName, htmlStatus, xhtmlStatus);
  this.updateTestList();

  this.updateSummaryData();
  this.updateChapterPopup();
}

TestSuite.prototype.beginAppendingOutput = function()
{
}

TestSuite.prototype.endAppendingOutput = function()
{
  var output = document.getElementById('output');
  output.scrollTop = output.scrollHeight;
}

TestSuite.prototype.appendResultToOutput = function(formatData, testName, resolution, comment)
{
  var output = document.getElementById('output');
  
  var result = formatData.path + '/' + testName + formatData.suffix + '\t' + resolution;
  if (comment)
    result += '\t(' + comment + ')';

  var line = document.createElement('p');
  line.className = resolution;
  line.appendChild(document.createTextNode(result));
  output.appendChild(line);
}

TestSuite.prototype.clearOutput = function()
{
  document.getElementById('output').innerHTML = '';
}

/* ------------------------------------------------------- */

TestSuite.prototype.switchToFormat = function(formatString)
{
  if (formatString == 'html4')
    document.harness.format.html4.checked = true;
  else
    document.harness.format.xhtml1.checked = true;

  this.formatChanged(formatString);
}

TestSuite.prototype.formatChanged = function(formatString)
{
  if (this.format == formatString)
    return;
  
  this.format = formatString;

  if (formatString == 'html4')
    this.formatInfo = kHTML4Data;
  else
    this.formatInfo = kXHTML1Data;

  // try to keep the current test selected
  var selectedTestName;
  if (this.currChapterTestIndex >= 0 && this.currChapterTestIndex < this.currentChapterTests.length)
    selectedTestName = this.currentChapterTests[this.currChapterTestIndex].id;
  
  if (this.currentChapter) {
    this.buildTestListForChapter(this.currentChapter);
    this.fillTestList();
    this.goToTestByName(selectedTestName);
  }

  this.updateChapterPopup();
  this.updateTestList();
  this.updateProgressLabel();
}

/* ------------------------------------------------------- */

TestSuite.prototype.asyncLoad = function(url, type, handler)
{
  $.get(url, handler, type);
}

/* ------------------------------------------------------- */

TestSuite.prototype.exportResults = function(resultTypeIndex)
{
  var resultInfo = kResultsSelector[resultTypeIndex];
  if (!resultInfo)
    return;

  resultInfo.exporter(this);
}

TestSuite.prototype.exportHeader = function()
{
  var result = '# Safari 5.0.2' + ' ' + navigator.platform + '\n';
  result += '# ' + navigator.userAgent + '\n';
  result += '# http://test.csswg.org/suites/css2.1/' + kTestSuiteVersion + '/\n';
  result += 'testname\tresult\n';

  return result;
}

TestSuite.prototype.createExportLine = function(formatData, testName, resolution, comment)
{
  var result = formatData.path + '/' + testName + '\t' + resolution;
  if (comment)
    result += '\t(' + comment + ')';
  return result;
}

TestSuite.prototype.exportQueryComplete = function(data)
{
  window.open("data:text/plain," + escape(data))
}

TestSuite.prototype.resultsPopupChanged = function(index)
{
  var resultInfo = kResultsSelector[index];
  if (!resultInfo)
    return;

  this.clearOutput();
  resultInfo.handler(this);
  
  var enableExport = resultInfo.exporter != undefined;
  document.getElementById('export-button').disabled = !enableExport;
}

/* ------------------------- Import ------------------------------- */
/*
  Import format is the same as the export format, namely:
  
  testname<tab>result

  with optional trailing <tab>comment.

html4/absolute-non-replaced-height-002<tab>pass
xhtml1/absolute-non-replaced-height-002<tab>?
  
  Lines starting with # are ignored.
  The "testname<tab>result" line is ignored.
*/
TestSuite.prototype.importResults = function(data)
{
  var testsToImport = [];

  var lines = data.split('\n');
  for (var i = 0; i < lines.length; ++i) {
    var currLine = lines[i];
    if (currLine.length == 0 || currLine.charAt(0) == '#')
      continue;

    var match = currLine.match(/^(html4|xhtml1)\/([\w-_]+)\t([\w?]+)\t?(.+)?$/);
    if (match) {
      var test = { 'id' : match[2] };
      test.format =  match[1];
      test.result = match[3];
      test.comment = match[4];
      
      if (test.result != '?')
        testsToImport.push(test);
    } else {
      window.console.log('failed to match line \'' + currLine + '\'');
    }
  }

  this.importTestResults(testsToImport);
  
  this.resetTestStatus();
  this.updateSummaryData();
}



/* --------------------- Clear Results --------------------------- */
/*
  Clear results format is either same as the export format, or
  a list of bare test IDs (e.g. absolute-non-replaced-height-001)
  in which case both HTML4 and XHTML1 results are cleared.
*/
TestSuite.prototype.clearResults = function(data)
{
  var testsToClear = [];

  var lines = data.split('\n');
  for (var i = 0; i < lines.length; ++i) {
    var currLine = lines[i];
    if (currLine.length == 0 || currLine.charAt(0) == '#')
      continue;

    // Look for format/test with possible extension
    var result = currLine.match(/^((html4|xhtml1)?)\/?([\w-_]+)/);
    if (result) {
      var testId = result[3];
      var format = result[1];
      
      var clearHTML = format.length == 0 || format == 'html4';
      var clearXHTML = format.length == 0 || format == 'xhtml1';
      
      var result = { 'id' : testId };
      result.clearHTML = clearHTML;
      result.clearXHTML = clearXHTML;

      testsToClear.push(result);
    } else {
      window.console.log('failed to match line ' + currLine);
    }
  }
  
  this.clearTestResults(testsToClear);
  
  this.resetTestStatus();
  this.updateSummaryData();
}

/* -------------------------------------------------------- */

TestSuite.prototype.exportResultsCompletion = function(exportTests)
{
  // Lame workaround for ORDER BY not working
  exportTests.sort(function(a, b) {
      return a.test.localeCompare(b.test);
  });
  
  var exportLines = [];
  for (var i = 0; i < exportTests.length; ++i) {
    var currTest = exportTests[i];
    if (currTest.html4 != '')
      exportLines.push(currTest.html4);
    if (currTest.xhtml1 != '')
      exportLines.push(currTest.xhtml1);
  }
  
  var exportString = this.exportHeader() + exportLines.join('\n');
  this.exportQueryComplete(exportString);
}

/* -------------------------------------------------------- */

TestSuite.prototype.showResultsForCompletedTests = function()
{
  this.beginAppendingOutput();
  
  var _self = this;
  this.queryDatabaseForCompletedTests(
      function(item) {
        if (item.hstatus)
          _self.appendResultToOutput(kHTML4Data, item.test, item.hstatus, item.hcomment);

        if (item.xstatus)
          _self.appendResultToOutput(kXHTML1Data, item.test, item.xstatus, item.xcomment);
      },
      function() {
        _self.endAppendingOutput();
      }
    );
}

TestSuite.prototype.exportResultsForCompletedTests = function()
{
  var exportTests = []; // each test will have html and xhtml items on it

  var _self = this;
  this.queryDatabaseForCompletedTests(
      function(item) {
        var htmlLine = '';
        if (item.hstatus)
          htmlLine= _self.createExportLine(kHTML4Data, item.test, item.hstatus, item.hcomment);

        var xhtmlLine = '';
        if (item.xstatus)
          xhtmlLine = _self.createExportLine(kXHTML1Data, item.test, item.xstatus, item.xcomment);

        exportTests.push({
          'test' : item.test,
          'html4' : htmlLine,
          'xhtml1' : xhtmlLine });
      },
      function() {
        _self.exportResultsCompletion(exportTests);
      }
    );
}


/* -------------------------------------------------------- */

TestSuite.prototype.showResultsForAllTests = function()
{
  this.beginAppendingOutput();

  var _self = this;
  this.queryDatabaseForAllTests('test',
    function(item) {
      _self.appendResultToOutput(kHTML4Data, item.test, item.hstatus, item.hcomment);
      _self.appendResultToOutput(kXHTML1Data, item.test, item.xstatus, item.xcomment);
    },
    function() {
      _self.endAppendingOutput();
    });
}

TestSuite.prototype.exportResultsForAllTests = function()
{
  var exportTests = [];

  var _self = this;
  this.queryDatabaseForAllTests('test',
      function(item) {
        var htmlLine= _self.createExportLine(kHTML4Data, item.test, item.hstatus ? item.hstatus : '?', item.hcomment);
        var xhtmlLine = _self.createExportLine(kXHTML1Data, item.test, item.xstatus ? item.xstatus : '?', item.xcomment);
        exportTests.push({
          'test' : item.test,
          'html4' : htmlLine,
          'xhtml1' : xhtmlLine });
      },
      function() {
        _self.exportResultsCompletion(exportTests);
      }
    );
}

/* -------------------------------------------------------- */

TestSuite.prototype.showResultsForTestsNotRun = function()
{
  this.beginAppendingOutput();

  var _self = this;
  this.queryDatabaseForTestsNotRun(
      function(item) {
        if (!item.hstatus)
          _self.appendResultToOutput(kHTML4Data, item.test, '?', item.hcomment);
        if (!item.xstatus)
          _self.appendResultToOutput(kXHTML1Data, item.test, '?', item.xcomment);
      },
      function() {
        _self.endAppendingOutput();
      }
    );
}

TestSuite.prototype.exportResultsForTestsNotRun = function()
{
  var exportTests = [];

  var _self = this;
  this.queryDatabaseForTestsNotRun(
      function(item) {
        var htmlLine = '';
        if (!item.hstatus)
          htmlLine= _self.createExportLine(kHTML4Data, item.test, '?', item.hcomment);

        var xhtmlLine = '';
        if (!item.xstatus)
          xhtmlLine = _self.createExportLine(kXHTML1Data, item.test, '?', item.xcomment);

        exportTests.push({
          'test' : item.test,
          'html4' : htmlLine,
          'xhtml1' : xhtmlLine });
      },
      function() {
        _self.exportResultsCompletion(exportTests);
      }
    );
}

/* -------------------------------------------------------- */

TestSuite.prototype.showResultsForTestsWithStatus = function(status)
{
  this.beginAppendingOutput();

  var _self = this;
  this.queryDatabaseForTestsWithStatus(status,
      function(item) {
        if (item.hstatus == status)
          _self.appendResultToOutput(kHTML4Data, item.test, item.hstatus, item.hcomment);
        if (item.xstatus == status)
          _self.appendResultToOutput(kXHTML1Data, item.test, item.xstatus, item.xcomment);
      },
      function() {
        _self.endAppendingOutput();
      }
    );
}

TestSuite.prototype.exportResultsForTestsWithStatus = function(status)
{
  var exportTests = [];

  var _self = this;
  this.queryDatabaseForTestsWithStatus(status,
      function(item) {
        var htmlLine = '';
        if (item.hstatus == status)
          htmlLine= _self.createExportLine(kHTML4Data, item.test, item.hstatus, item.hcomment);

        var xhtmlLine = '';
        if (item.xstatus == status)
          xhtmlLine = _self.createExportLine(kXHTML1Data, item.test, item.xstatus, item.xcomment);

        exportTests.push({
          'test' : item.test,
          'html4' : htmlLine,
          'xhtml1' : xhtmlLine });
      },
      function() {
        _self.exportResultsCompletion(exportTests);
      }
    );
}

/* -------------------------------------------------------- */

TestSuite.prototype.showResultsForTestsWithMismatchedResults = function()
{
  this.beginAppendingOutput();

  var _self = this;
  this.queryDatabaseForTestsWithMixedStatus(
      function(item) {
        _self.appendResultToOutput(kHTML4Data, item.test, item.hstatus, item.hcomment);
        _self.appendResultToOutput(kXHTML1Data, item.test, item.xstatus, item.xcomment);
      },
      function() {
        _self.endAppendingOutput();
      }
    );
}

TestSuite.prototype.exportResultsForTestsWithMismatchedResults = function()
{
  var exportTests = [];

  var _self = this;
  this.queryDatabaseForTestsWithMixedStatus(
      function(item) {
        var htmlLine= _self.createExportLine(kHTML4Data, item.test, item.hstatus ? item.hstatus : '?', item.hcomment);
        var xhtmlLine = _self.createExportLine(kXHTML1Data, item.test, item.xstatus ? item.xstatus : '?', item.xcomment);
        exportTests.push({
          'test' : item.test,
          'html4' : htmlLine,
          'xhtml1' : xhtmlLine });
      },
      function() {
        _self.exportResultsCompletion(exportTests);
      }
    );
}

/* -------------------------------------------------------- */

TestSuite.prototype.markTestCompleted = function(testID, htmlStatus, xhtmlStatus)
{
  var test = this.tests[testID];
  if (!test) {
    window.console.log('markTestCompleted failed to find test ' + testID);
    return;
  }

  if (htmlStatus) {
    test.completedHTML = true;
    test.statusHTML = htmlStatus;
  }
  if (xhtmlStatus) {
    test.completedXHTML = true;
    test.statusXHTML = xhtmlStatus;
  }
}

TestSuite.prototype.testCompletionStateChanged = function()
{
  this.updateTestList();
  this.updateChapterPopup();
}

TestSuite.prototype.loadTestStatus = function()
{
  var _self = this;
  this.queryDatabaseForCompletedTests(
      function(item) {
      _self.markTestCompleted(item.test, item.hstatus, item.xstatus);
      },
      function() {
        _self.testCompletionStateChanged();
      }
    );
    
    this.updateChapterPopup();
}

TestSuite.prototype.resetTestStatus = function()
{
  for (var testID in this.tests) {
    var currTest = this.tests[testID];
    currTest.completedHTML = false;
    currTest.completedXHTML = false;
  }
  this.loadTestStatus();
}

/* -------------------------------------------------------- */

TestSuite.prototype.updateSummaryData = function()
{
  this.queryDatabaseForSummary(
      function(results) {
        
        var hTotal, xTotal;
        var hDone, xDone;
        
        for (var i = 0; i < results.length; ++i) {
          var result = results[i];
          
          switch (result.name) {
            case 'h-total': hTotal = result.count; break;
            case 'x-total': xTotal = result.count; break;
            case 'h-tested': hDone = result.count; break;
            case 'x-tested': xDone = result.count; break;
          }

          document.getElementById(result.name).innerText = result.count;
        }
        
        // We should get these all together.
        if (hTotal) {
          document.getElementById('h-percent').innerText = Math.round(100.0 * hDone / hTotal);
          document.getElementById('x-percent').innerText = Math.round(100.0 * xDone / xTotal);
        }
      }
    );
}

/* ------------------------------------------------------- */
// Database stuff

function errorHandler(transaction, error)
{
  alert('Database error: ' + error.message);
  window.console.log('Database error: ' + error.message);
}

TestSuite.prototype.openDatabase = function()
{
  if (!'openDatabase' in window) {
    alert('Your browser does not support client-side SQL databases, so results will not be stored.');
    return;
  }
  
  var _self = this;
  this.db = window.openDatabase('css21testsuite', '', 'CSS 2.1 test suite results', 10 * 1024 * 1024);

  // Migration handling. We assume migration will happen whenever the suite version changes,
  // so that we can check for new or obsoleted tests.
  function creation(tx) {
    _self.databaseCreated(tx);
  }

  function migration1_0To1_1(tx) {
    window.console.log('updating 1.0 to 1.1');
    // We'll use the 'seen' column to cross-check with testinfo.data.
    tx.executeSql('ALTER TABLE tests ADD COLUMN seen BOOLEAN DEFAULT \"FALSE\"', null, function() {
      _self.syncDatabaseWithTestInfoData();
    }, errorHandler);
  }

  if (this.db.version == '') {
    _self.db.changeVersion('', '1.0', creation, null, function() {
      _self.db.changeVersion('1.0', '1.1', migration1_0To1_1, null, function() {
        _self.databaseReady();
      }, errorHandler);
    }, errorHandler);

    return;
  }

  if (this.db.version == '1.0') {
    _self.db.changeVersion('1.0', '1.1', migration1_0To1_1, null, function() {
      window.console.log('ready')
      _self.databaseReady();
    }, errorHandler);
    return;
  }

  this.databaseReady();
}

TestSuite.prototype.databaseCreated = function(tx)
{
  window.console.log('databaseCreated');
  this.populatingDatabase = true;

  // hstatus: HTML4 result
  // xstatus: XHTML1 result
  var _self = this;
  tx.executeSql('CREATE TABLE tests (test PRIMARY KEY UNIQUE, ref, title, flags, links, assertion, hstatus, hcomment, xstatus, xcomment)', null,
    function(tx, results) {
      _self.populateDatabaseFromTestInfoData();
    }, errorHandler);
}

TestSuite.prototype.databaseReady = function()
{
  this.updateSummaryData();
  this.loadTestStatus();
}

TestSuite.prototype.storeTestResult = function(test, format, result, comment, useragent)
{
  if (!this.db)
    return;

  this.db.transaction(function (tx) {
    if (format == 'html4')
      tx.executeSql('UPDATE tests SET hstatus=?, hcomment=? WHERE test=?\n', [result, comment, test], null, errorHandler);
    else if (format == 'xhtml1')
      tx.executeSql('UPDATE tests SET xstatus=?, xcomment=? WHERE test=?\n', [result, comment, test], null, errorHandler);
  });
}

TestSuite.prototype.importTestResults = function(results)
{
  if (!this.db)
    return;

  this.db.transaction(function (tx) {

    for (var i = 0; i < results.length; ++i) {
      var currResult = results[i];

      var query;
      if (currResult.format == 'html4')
        query = 'UPDATE tests SET hstatus=?, hcomment=? WHERE test=?\n';
      else if (currResult.format == 'xhtml1')
        query = 'UPDATE tests SET xstatus=?, xcomment=? WHERE test=?\n';

      tx.executeSql(query, [currResult.result, currResult.comment, currResult.id], null, errorHandler);
    }
  });
}

TestSuite.prototype.clearTestResults = function(results)
{
  if (!this.db)
    return;

  this.db.transaction(function (tx) {
    
    for (var i = 0; i < results.length; ++i) {
      var currResult = results[i];

      if (currResult.clearHTML)
        tx.executeSql('UPDATE tests SET hstatus=NULL, hcomment=NULL WHERE test=?\n', [currResult.id], null, errorHandler);

      if (currResult.clearXHTML)
        tx.executeSql('UPDATE tests SET xstatus=NULL, xcomment=NULL WHERE test=?\n', [currResult.id], null, errorHandler);
      
    }
  });
}

TestSuite.prototype.populateDatabaseFromTestInfoData = function()
{
  if (!this.testInfoLoaded) {
    window.console.log('Tring to populate database before testinfo.data has been loaded');
    return;
  }
  
  window.console.log('populateDatabaseFromTestInfoData')
  var _self = this;
  this.db.transaction(function (tx) {
    for (var testID in _self.tests) {
      var test = _self.tests[testID];
      // Version 1.0, so no 'seen' column.
      tx.executeSql('INSERT INTO tests (test, ref, title, flags, links, assertion) VALUES (?, ?, ?, ?, ?, ?)',
        [test.id, test.reference, test.title, test.flags, test.links, test.assertion], null, errorHandler);
    }
    _self.populatingDatabase = false;
  });

}

TestSuite.prototype.insertTest = function(tx, test)
{
  tx.executeSql('INSERT INTO tests (test, ref, title, flags, links, assertion, seen) VALUES (?, ?, ?, ?, ?, ?, ?)',
    [test.id, test.reference, test.title, test.flags, test.links, test.assertion, 'TRUE'], null, errorHandler);
}

// Deal with removed/renamed tests in a new version of the suite.
// self.tests is canonical; the database may contain stale entries.
TestSuite.prototype.syncDatabaseWithTestInfoData = function()
{
  if (!this.testInfoLoaded) {
    window.console.log('Trying to sync database before testinfo.data has been loaded');
    return;
  }

  // Make an object with all tests that we'll use to track new tests.
  var testsToInsert = {};
  for (var testId in this.tests) {
    var currTest = this.tests[testId];
    testsToInsert[currTest.id] = currTest;
  }
  
  var _self = this;
  this.db.transaction(function (tx) {
    // Find tests that are not in the database yet.
    // (Wasn't able to get INSERT ... IF NOT working.)
    tx.executeSql('SELECT * FROM tests', [], function(tx, results) {
      var len = results.rows.length;
      for (var i = 0; i < len; ++i) {
        var item = results.rows.item(i);
        delete testsToInsert[item.test];
      }
    }, errorHandler);
  });

  this.db.transaction(function (tx) {
    for (var testId in testsToInsert) {
      var currTest = testsToInsert[testId];
      window.console.log(currTest.id + ' is new; inserting');
      _self.insertTest(tx, currTest);
    }
  });
    
  this.db.transaction(function (tx) {
    for (var testID in _self.tests)
      tx.executeSql('UPDATE tests SET seen=\"TRUE\" WHERE test=?\n', [testID], null, errorHandler);

    tx.executeSql('SELECT * FROM tests WHERE seen=\"FALSE\"', [], function(tx, results) {
      var len = results.rows.length;
      for (var i = 0; i < len; ++i) {
        var item = results.rows.item(i);
        window.console.log('Test ' + item.test + ' was in the database but is no longer in the suite; deleting.');
      }
    }, errorHandler);

    // Delete rows for disappeared tests.
    tx.executeSql('DELETE FROM tests WHERE seen=\"FALSE\"', [], function(tx, results) {
      _self.populatingDatabase = false;
      _self.databaseReady();
    }, errorHandler);
  });
}

TestSuite.prototype.queryDatabaseForAllTests = function(sortKey, perRowHandler, completionHandler)
{
  if (this.populatingDatabase)
    return;

  var _self = this;
  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;
    var query;
    var args = [];
    if (sortKey != '') {
      query = 'SELECT * FROM tests ORDER BY ? ASC';  // ORDER BY doesn't seem to work
      args.push(sortKey);
    }
    else
      query = 'SELECT * FROM tests';

    tx.executeSql(query, args, function(tx, results) {

      var len = results.rows.length;
      for (var i = 0; i < len; ++i)
        perRowHandler(results.rows.item(i));
      
      completionHandler();
    }, errorHandler);
  });  
}

TestSuite.prototype.queryDatabaseForTestsWithStatus = function(status, perRowHandler, completionHandler)
{
  if (this.populatingDatabase)
    return;

  var _self = this;
  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;
    tx.executeSql('SELECT * FROM tests WHERE hstatus=? OR xstatus=?', [status, status], function(tx, results) {

      var len = results.rows.length;
      for (var i = 0; i < len; ++i)
        perRowHandler(results.rows.item(i));
      
      completionHandler();
    }, errorHandler);
  });  
}

TestSuite.prototype.queryDatabaseForTestsWithMixedStatus = function(perRowHandler, completionHandler)
{
  if (this.populatingDatabase)
    return;

  var _self = this;
  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;
    tx.executeSql('SELECT * FROM tests WHERE hstatus IS NOT NULL AND xstatus IS NOT NULL AND hstatus <> xstatus', [], function(tx, results) {

      var len = results.rows.length;
      for (var i = 0; i < len; ++i)
        perRowHandler(results.rows.item(i));
      
      completionHandler();
    }, errorHandler);
  });  
}

TestSuite.prototype.queryDatabaseForCompletedTests = function(perRowHandler, completionHandler)
{
  if (this.populatingDatabase)
    return;

  var _self = this;
  this.db.transaction(function (tx) {
    
    if (_self.populatingDatabase)
      return;

    tx.executeSql('SELECT * FROM tests WHERE hstatus IS NOT NULL OR xstatus IS NOT NULL', [], function(tx, results) {
      var len = results.rows.length;
      for (var i = 0; i < len; ++i)
        perRowHandler(results.rows.item(i));
      
      completionHandler();
    }, errorHandler);
  });  
}

TestSuite.prototype.queryDatabaseForTestsNotRun = function(perRowHandler, completionHandler)
{
  if (this.populatingDatabase)
    return;

  var _self = this;
  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;

    tx.executeSql('SELECT * FROM tests WHERE hstatus IS NULL OR xstatus IS NULL', [], function(tx, results) {

      var len = results.rows.length;
      for (var i = 0; i < len; ++i)
        perRowHandler(results.rows.item(i));

      completionHandler();
    }, errorHandler);
  });  
}

/*

  completionHandler gets called an array of results,
  which may be some or all of:
  
  data = [
    { 'name' : ,
      'count' : 
    },
  ]
  
  where name is one of:
  
    'h-total'
    'h-tested'
    'h-passed'
    'h-failed'
    'h-skipped'

    'x-total'
    'x-tested'
    'x-passed'
    'x-failed'
    'x-skipped'

 */


TestSuite.prototype.countTestsWithColumnValue = function(tx, completionHandler, column, value, label)
{  
  var allRowsCount = 'COUNT(*)';

  tx.executeSql('SELECT COUNT(*) FROM tests WHERE ' + column + '=?', [value], function(tx, results) {
    var data = [];
    if (results.rows.length > 0)
      data.push({ 'name' : label, 'count' : results.rows.item(0)[allRowsCount] })
    completionHandler(data);
  }, errorHandler);
}

TestSuite.prototype.countTestsWithFlag = function(tx, completionHandler, flag)
{  
  var allRowsCount = 'COUNT(*)';

  tx.executeSql('SELECT COUNT(*) FROM tests WHERE flags LIKE \"%' + flag + '%\"', [], function(tx, results) {
    var rowCount = 0;
    if (results.rows.length > 0)
      rowCount = results.rows.item(0)[allRowsCount];
    completionHandler(rowCount);
  }, errorHandler);
}

TestSuite.prototype.queryDatabaseForSummary = function(completionHandler)
{
  if (!this.db || this.populatingDatabase)
    return;

  var _self = this;

  var htmlOnlyTestCount = 0;
  var xHtmlOnlyTestCount = 0;

  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;

    var allRowsCount = 'COUNT(*)';
      
    _self.countTestsWithFlag(tx, function(count) {
      htmlOnlyTestCount = count;
    }, 'htmlOnly');

    _self.countTestsWithFlag(tx, function(count) {
      xHtmlOnlyTestCount = count;
    }, 'nonHTML');
  });
  
  this.db.transaction(function (tx) {
    if (_self.populatingDatabase)
      return;

    var allRowsCount = 'COUNT(*)';
    var html4RowsCount = 'COUNT(hstatus)';
    var xhtml1RowsCount = 'COUNT(xstatus)';
    
    tx.executeSql('SELECT COUNT(*), COUNT(hstatus), COUNT(xstatus) FROM tests', [], function(tx, results) {

      var data = [];
      if (results.rows.length > 0) {
        var rowItem = results.rows.item(0);
        data.push({ 'name' : 'h-total' , 'count' : rowItem[allRowsCount] - xHtmlOnlyTestCount })
        data.push({ 'name' : 'x-total' , 'count' : rowItem[allRowsCount] - htmlOnlyTestCount })
        data.push({ 'name' : 'h-tested', 'count' : rowItem[html4RowsCount] })
        data.push({ 'name' : 'x-tested', 'count' : rowItem[xhtml1RowsCount] })
      }
      completionHandler(data);
      
    }, errorHandler);


    _self.countTestsWithColumnValue(tx, completionHandler, 'hstatus', 'pass', 'h-passed');
    _self.countTestsWithColumnValue(tx, completionHandler, 'xstatus', 'pass', 'x-passed');

    _self.countTestsWithColumnValue(tx, completionHandler, 'hstatus', 'fail', 'h-failed');
    _self.countTestsWithColumnValue(tx, completionHandler, 'xstatus', 'fail', 'x-failed');

    _self.countTestsWithColumnValue(tx, completionHandler, 'hstatus', 'skipped', 'h-skipped');
    _self.countTestsWithColumnValue(tx, completionHandler, 'xstatus', 'skipped', 'x-skipped');

    _self.countTestsWithColumnValue(tx, completionHandler, 'hstatus', 'invalid', 'h-invalid');
    _self.countTestsWithColumnValue(tx, completionHandler, 'xstatus', 'invalid', 'x-invalid');
  });
}

