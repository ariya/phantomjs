describe("WebPage CJK support", function () {
    var texts = [
        new Text("Shift_JIS", "g3SDQIOTg2eDgA==", "ファントム")
    ,   new Text("EUC-JP", "pdWloaXzpcil4A0K", "ファントム")
    ,   new Text("ISO-2022-JP", "GyRCJVUlISVzJUglYBsoQg0K", "ファントム")
    ,   new Text("Big5", "pNu2SA0K", "幻象")
    ,   new Text("GBK", "u8PP8w0K", "幻象")
    ,   new Text("EUC-KR", "yK+/tQ==", "환영")
    ];

    texts.forEach(function (t) {
        it(t.codec, function() {
            var decodedText = -1;
            var page = new WebPage();

            page.open(t.dataUrl(), function(status) {
                decodedText = page.evaluate(function() {
                    return document.getElementsByTagName("pre")[0].innerText;
                });
                page.close();
            });

            waitsFor(function () {
                return -1 !== decodedText;
            }, "Text not decoded within three seconds", 3000);

            runs(function () {
                expect(t.check(decodedText)).toBeTruthy();
            });
        });
    });

    function Text(codec, base64, reference) {
        this.codec = codec;
        this.base64 = base64;
        this.reference = reference;
    }

    Text.prototype.dataUrl = function () {
        return "data:text/plain;charset=" + this.codec + ";base64," + this.base64;
    };

    Text.prototype.check = function (decodedText) {
        return decodedText.match("^" + this.reference) == this.reference;
    };
});

// vim:ts=4:sw=4:sts=4:et:
