describe("Function.prototype", function () {
    it("should have bind() in phantom context", function () {
        expect(typeof Function.prototype.bind).toEqual("function");
    });

    it("should have bind() in WebPage context", function () {
        var s = require("webserver").create();
        s.listen(12345, function(request, response) {
            setTimeout(function() {
                response.statusCode = 200;
                response.write("<html><body>Loaded!</body></html>");
                response.close();
            }, 500);
        });

        var bind = -1;

        var p = require("webpage").create();
        p.open("http//localhost:12345", function (status) {
            bind = p.evaluate(function () {
                return typeof Function.prototype.bind;
            });
            p.close();
        });

        waitsFor(function () {
            return -1 !== bind;
        }, "Page was never opened", 3000);

        runs(function () {
            s.close();
            expect(bind).toEqual("function");
        });
    });
});
