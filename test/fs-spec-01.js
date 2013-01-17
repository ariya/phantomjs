describe("Basic Files API (read, write, remove, ...)", function() {
    var FILENAME = "temp-01.test",
        FILENAME_COPY = FILENAME + ".copy",
        FILENAME_MOVED = FILENAME + ".moved",
        FILENAME_EMPTY = FILENAME + ".empty",
        FILENAME_ENC = FILENAME + ".enc",
        FILENAME_BIN = FILENAME + ".bin",
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

    it("should be able to create (touch) an empty file", function() {
        expect(fs.exists(FILENAME_EMPTY)).toBeFalsy();
        fs.touch(FILENAME_EMPTY);
        expect(fs.exists(FILENAME_EMPTY)).toBeTruthy();
        expect(fs.size(FILENAME_EMPTY)).toEqual(0);
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

    it("should be able to read specific number of bytes from a specific position in a file", function() {
        var content = "";
        try{
            var f = fs.open(FILENAME, "r");
            f.seek(3);
            content = f.read(5);
            f.close();
        } catch (e) { }
        expect(content).toEqual("lo\nwo");
    });

    it("should be able to read/write/append content from a file", function() {
        var content = "";
        try{
            var f = fs.open(FILENAME, "rw+");
            f.writeLine("asdf");
            content = f.read();
            f.close();
        } catch (e) { }
        expect(content).toEqual("hello\nworld\nasdf\n");
    });

    it("should be able to copy a file", function() {
        expect(fs.exists(FILENAME_COPY)).toBeFalsy();
        fs.copy(FILENAME, FILENAME_COPY);
        expect(fs.exists(FILENAME_COPY)).toBeTruthy();
        expect(fs.read(FILENAME)).toEqual(fs.read(FILENAME_COPY));
    });

    it("should be able to move a file", function() {
        expect(fs.exists(FILENAME)).toBeTruthy();
        var contentBeforeMove = fs.read(FILENAME);
        fs.move(FILENAME, FILENAME_MOVED);
        expect(fs.exists(FILENAME)).toBeFalsy();
        expect(fs.exists(FILENAME_MOVED)).toBeTruthy();
        expect(fs.read(FILENAME_MOVED)).toEqual(contentBeforeMove);
    });

    it("should be able to remove a (moved) file", function() {
        expect(fs.exists(FILENAME_MOVED)).toBeTruthy();
        fs.remove(FILENAME_MOVED);
        expect(fs.exists(FILENAME_MOVED)).toBeFalsy();
    });

    it("should be able to remove a (copied) file", function() {
        expect(fs.exists(FILENAME_COPY)).toBeTruthy();
        fs.remove(FILENAME_COPY);
        expect(fs.exists(FILENAME_COPY)).toBeFalsy();
    });

    it("should be able to remove an empty file", function() {
        expect(fs.exists(FILENAME_EMPTY)).toBeTruthy();
        fs.remove(FILENAME_EMPTY);
        expect(fs.exists(FILENAME_EMPTY)).toBeFalsy();
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

    it("should be read/write utf8 text by default", function() {
        var content, output = "ÄABCÖ";
        try {
            var f = fs.open(FILENAME_ENC, "w");
            f.write(output);
            f.close();

            f = fs.open(FILENAME_ENC, "r");
            content = f.read();
            f.close();

            fs.remove(FILENAME_ENC);
        } catch (e) { }
        expect(content).toEqual(output);
    });

    it("should be read/write binary data", function() {
        var content, output = String.fromCharCode(0, 1, 2, 3, 4, 5);
        try {
            var f = fs.open(FILENAME_BIN, "wb");
            f.write(output);
            f.close();

            f = fs.open(FILENAME_BIN, "rb");
            content = f.read();
            f.close();

            fs.remove(FILENAME_BIN);
        } catch (e) { }
        expect(content).toEqual(output);
    });

    it("should be read/write binary data (shortcuts)", function() {
        var content, output = String.fromCharCode(0, 1, 2, 3, 4, 5);
        try {
            fs.write(FILENAME_BIN, output, "b");

            content = fs.read(FILENAME_BIN, "b");

            fs.remove(FILENAME_BIN);
        } catch (e) { }
        expect(content).toEqual(output);
    });
});
