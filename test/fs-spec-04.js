describe("Tests Files API", function() {
    var ABSENT_DIR = "absentdir04",
        ABSENT_FILE = "absentfile04",
        TEST_DIR = "testdir04",
        TEST_FILE = "testfile04",
        TEST_FILE_PATH = fs.join(TEST_DIR, TEST_FILE),
        TEST_CONTENT = "test content",
        START_CWD = null;

    it("should create some temporary file and directory", function(){
        fs.makeDirectory(TEST_DIR);
        fs.write(TEST_FILE_PATH, TEST_CONTENT, "w");
    });

    it("should confirm that test file and test dir exist, while the absent ones don't", function(){
        expect(fs.exists(TEST_FILE_PATH)).toBeTruthy();
        expect(fs.exists(TEST_DIR)).toBeTruthy();
        expect(fs.exists(ABSENT_FILE)).toBeFalsy();
        expect(fs.exists(ABSENT_DIR)).toBeFalsy();
    });

    it("should confirm that the temporary directory is infact a directory, while the absent one doesn't", function(){
        expect(fs.isDirectory(TEST_DIR)).toBeTruthy();
        expect(fs.isDirectory(ABSENT_DIR)).toBeFalsy();
    });

    it("should confirm that the temporary file is infact a file, while the absent one doesn't", function(){
        expect(fs.isFile(TEST_FILE_PATH)).toBeTruthy();
        expect(fs.isFile(ABSENT_FILE)).toBeFalsy();
    });

    it("should confirm that a relative path is not absolute, while an absolute one is", function(){
        var absPath = fs.absolute(TEST_FILE_PATH);

        expect(fs.isAbsolute(TEST_FILE_PATH)).toBeFalsy();
        expect(fs.isAbsolute(absPath)).toBeTruthy();
    });

    it("should confirm that temporary file is readable, writable and non-executable, while absent file is none of those", function(){
        expect(fs.isReadable(TEST_FILE_PATH)).toBeTruthy();
        expect(fs.isWritable(TEST_FILE_PATH)).toBeTruthy();
        expect(fs.isExecutable(TEST_FILE_PATH)).toBeFalsy();

        expect(fs.isReadable(ABSENT_FILE)).toBeFalsy();
        expect(fs.isWritable(ABSENT_FILE)).toBeFalsy();
        expect(fs.isExecutable(ABSENT_FILE)).toBeFalsy();
    });

    it("should confirm that temporary directory is readable, writable and executable, while absent dir is none of those", function(){
        expect(fs.isReadable(TEST_DIR)).toBeTruthy();
        expect(fs.isWritable(TEST_DIR)).toBeTruthy();
        expect(fs.isExecutable(TEST_DIR)).toBeTruthy();

        expect(fs.isReadable(ABSENT_DIR)).toBeFalsy();
        expect(fs.isWritable(ABSENT_DIR)).toBeFalsy();
        expect(fs.isExecutable(ABSENT_DIR)).toBeFalsy();
    });

    it("should confirm that neither temporary file/dir or absent file/dir are links", function(){
        expect(fs.isLink(TEST_DIR)).toBeFalsy();
        expect(fs.isLink(TEST_FILE_PATH)).toBeFalsy();
        expect(fs.isLink(ABSENT_DIR)).toBeFalsy();
        expect(fs.isLink(ABSENT_FILE)).toBeFalsy();
    });

    it("should delete the temporary directory and file", function(){
        fs.removeTree(TEST_DIR);
    });

});
