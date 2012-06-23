describe("Files and Directories API", function() {
	var TEST_DIR = "testdir",
		TEST_FILE = "testfile",
		START_CWD = fs.workingDirectory;

	it("should create a new temporary directory and change the Current Working Directory to it", function() {
		expect(fs.makeDirectory(TEST_DIR)).toBeTruthy();
		expect(fs.changeWorkingDirectory(TEST_DIR)).toBeTruthy();
	});

	it("should create a file in the Current Working Directory and check it's absolute path", function() {
		fs.write(TEST_FILE, TEST_FILE, "w");
		var suffix = fs.separator + TEST_DIR + fs.separator +  TEST_FILE,
			abs = fs.absolute(".." + suffix),
			lastIndex = abs.lastIndexOf(suffix);
		expect(lastIndex).toNotEqual(-1);
		expect(lastIndex + suffix.length === abs.length);
	});

	it("should return to previous Current Working Directory and remove temporary directory", function() {
		expect(fs.changeWorkingDirectory(START_CWD)).toBeTruthy();
		fs.removeTree(TEST_DIR);
	});

	//it("should copy Content of the '/test/' Directory in a temporary directory, compare with the original and then remove", function() {
		//fs.copyTree(phantom.libraryPath, TEST_DIR);
		//expect(fs.list(phantom.libraryPath).length).toEqual(fs.list(TEST_DIR).length);
		//fs.removeTree(TEST_DIR);
	//});

    // TODO: test the actual functionality once we can create symlink.
    it("should have readLink function", function() {
        expect(typeof fs.readLink).toEqual('function');
    });
});
