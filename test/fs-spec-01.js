describe("Basic Files API (read, write, remove, ...)", function() {
    var FILENAME = "temp-01.test",
		FILENAME_COPY = FILENAME + ".copy",
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
    
	it("should copy a file", function() {
		expect(fs.exists(FILENAME_COPY)).toBeFalsy();
		fs.copy(FILENAME, FILENAME_COPY);
		expect(fs.exists(FILENAME_COPY)).toBeTruthy();
		expect(fs.read(FILENAME)).toEqual(fs.read(FILENAME_COPY));
	});

    it("should be able to remove a file", function() {
		expect(fs.exists(FILENAME)).toBeTruthy();
		fs.remove(FILENAME);
        expect(fs.exists(FILENAME)).toBeFalsy();
    });

 	it("should be able to remove a (copied) file", function() {
        expect(fs.exists(FILENAME_COPY)).toBeTruthy();
        fs.remove(FILENAME_COPY);
        expect(fs.exists(FILENAME_COPY)).toBeFalsy();
    });
    
    it("should throw an exception when trying to open for read a non existing file", function(){
        expect(function(){
            fs.open(ABSENT, "r");
        }).toThrow("Unable to open file '"+ ABSENT +"'");
    });

	it("should throw an exception when trying to copy a non existing file", function() {
		expect(function(){
			fs.copy(ABSENT, FILENAME_COPY);
		}).toThrow("Unable to copy file '" + ABSENT + "' at '" + FILENAME_COPY + "'");
	});
});