describe("Basic Files API (read, write, remove, ...)", function() {
    var FILENAME = "temp-01.test",
        ABSENT = "absent-01.test";
    
    it("should be able to create and write a file", function() {
        try{
            var f = fs.open(FILENAME, "w");
            
            f.write("hello");
            f.writeLine("");
            f.writeLine("world");
            f.close();
        } catch (e) { }
        expect(fs.exists(FILENAME)).toBeTruthy();
    });
    
    it("should be able to read content from a file", function() {
        var content = "";
        try{
            var f = fs.open(FILENAME, "r");
            
            content = f.read();    
            f.close();
        } catch (e) { }
        expect(content).toEqual("hello\nworld\n");
    });
    
    it("should be able to remove a file", function() {
        expect(fs.exists(FILENAME)).toBeTruthy();
        fs.remove(FILENAME);
        expect(fs.exists(FILENAME)).toBeFalsy();
    });
    
    it("should throw an exception when trying to open for read a non existing file", function(){
        expect(function(){
            fs.open(ABSENT, "r");
        }).toThrow("Unable to open file '"+ ABSENT +"'");
    });
});