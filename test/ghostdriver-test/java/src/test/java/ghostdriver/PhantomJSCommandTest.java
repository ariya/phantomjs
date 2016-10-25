package ghostdriver;

import org.junit.Test;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.phantomjs.PhantomJSDriver;

import static junit.framework.TestCase.assertEquals;

public class PhantomJSCommandTest extends BaseTest {
    @Test
    public void executePhantomJS() {
        WebDriver d = getDriver();
        if (!(d instanceof PhantomJSDriver)) {
            // Skip this test if not using PhantomJS.
            // The command under test is only available when using PhantomJS
            return;
        }

        PhantomJSDriver phantom = (PhantomJSDriver)d;

        // Do we get results back?
        Object result = phantom.executePhantomJS("return 1 + 1");
        assertEquals(new Long(2), (Long)result);

        // Can we read arguments?
        result = phantom.executePhantomJS("return arguments[0] + arguments[0]", new Long(1));
        assertEquals(new Long(2), (Long)result);

        // Can we override some browser JavaScript functions in the page context?
        result = phantom.executePhantomJS("var page = this;" +
           "page.onInitialized = function () { " +
                "page.evaluate(function () { " +
                    "Math.random = function() { return 42 / 100 } " +
                "})" +
            "}");

        phantom.get("http://ariya.github.com/js/random/");

        WebElement numbers = phantom.findElement(By.id("numbers"));
        boolean foundAtLeastOne = false;
        for(String number : numbers.getText().split(" ")) {
            foundAtLeastOne = true;
            assertEquals("42", number);
        }
        assert(foundAtLeastOne);
    }
}
