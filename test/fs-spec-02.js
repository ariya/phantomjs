describe("Attributes Files API", function() {
    var FILENAME = "temp-02.test",
        CONTENT = "This is a test for PhantomJS, an awesome headless browser to do all sort of stuff :) ",
        CONTENT_MULTIPLIER = 1024,
        ABSENT = "absent-02.test";

    it("should throw an exception when trying to read the size of a non existing file", function(){
        expect(function(){
            fs.size(ABSENT);
        }).toThrow("Unable to read file '"+ ABSENT +"' size");
    });

    it("should return a null Date object when trying to read the last modified date of a non existing file", function(){
        expect(fs.lastModified(ABSENT)).toBeNull();
    });

    it("should create temporary file '"+ FILENAME +"' and writes some content in it", function(){
        try{
            var f = fs.open(FILENAME, "w");

            expect(f).toBeDefined();
            for (var i = 1; i <= CONTENT_MULTIPLIER; ++i) {
                f.write(CONTENT);
            }
            f.close();
        } catch (e) { }
    });

    it("should be able to read the size of a temporary file '"+ FILENAME +"'", function() {
        expect(fs.size(FILENAME)).toEqual(CONTENT.length * CONTENT_MULTIPLIER);
    });

    it("should be able to read the Date on which a temporary file '"+ FILENAME +"' was last modified", function() {
        var flm = fs.lastModified(FILENAME),
            now = new Date();

        expect(now.getDay()).toEqual(flm.getDay());
        expect(now.getMonth()).toEqual(flm.getMonth());
        expect(now.getFullYear()).toEqual(flm.getFullYear());
        expect(now.getMilliseconds()).toNotEqual(flm.getMilliseconds());
    });

    it("should remove temporary file '"+ FILENAME +"'", function(){
        fs.remove(FILENAME);
    });
});
