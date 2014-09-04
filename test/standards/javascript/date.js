var assert = require('../../assert');

// construct date in mm-dd-yyyy format
var date = new Date('2012-09-07');
assert.isTrue(date.toString() != 'Invalid Date');
assert.equal(date.getDate(), 6);
assert.equal(date.getMonth(), 8);
assert.equal(date.getYear(), 112);

// parse date in ISO8601 format (yyyy-mm-dd)
var date = Date.parse("2012-01-01");
assert.equal(date, 1325376000000);
