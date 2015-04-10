Object.defineProperty(Array.prototype, "lastValue",
{
    get: function()
    {
        if (!this.length)
            return undefined;
        return this[this.length - 1];
    }
});
