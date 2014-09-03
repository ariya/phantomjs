describe("phantom global object", function() {

    it("should be able to get the error signal handler that is currently set on it", function() {
        phantom.onError = undefined;
        expect(phantom.onError).toBeUndefined();
        var onErrorFunc1 = function() { return !"x"; };
        phantom.onError = onErrorFunc1;
        expect(phantom.onError).toEqual(onErrorFunc1);
        var onErrorFunc2 = function() { return !!"y"; };
        phantom.onError = onErrorFunc2;
        expect(phantom.onError).toEqual(onErrorFunc2);
        expect(phantom.onError).toNotEqual(onErrorFunc1);
        phantom.onError = null;
        // Will only allow setting to a function value, so setting it to `null` returns `undefined`
        expect(phantom.onError).toBeUndefined();
        phantom.onError = undefined;
        expect(phantom.onError).toBeUndefined();
    });

    it("reports parse time error source and line in stack", function() {
        var stack;
        phantom.onError = function(message, s) { stack = s; };

        var helperFile = "./fixtures/parse-error-helper.js";
        phantom.injectJs(helperFile);

        waits(0);

        runs(function() {
            expect(stack[0].file).toEqual(helperFile);
            expect(stack[0].line).toEqual(2);
            phantom.onError = phantom.defaultErrorHandler;
        });
    });
});
