describe("WebKit", function() {
  it("should construct date in mm-dd-yyyy format", function() {
    var date = new Date('2012-09-07');    
    expect(date.toString()).toNotEqual('Invalid Date');
  });
  
  it("should parse date in ISO8601 format (yyyy-mm-dd)", function() {
    var date = Date.parse("2012-01-01");
    expect(date).toEqual(1325376000000);
  });
});