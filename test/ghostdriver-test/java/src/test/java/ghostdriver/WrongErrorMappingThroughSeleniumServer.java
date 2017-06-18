package ghostdriver;

import org.junit.Test;
import org.openqa.selenium.*;

public class WrongErrorMappingThroughSeleniumServer extends BaseTest {

    @Test(expected = NoSuchElementException.class)
    public void shouldThrowNoSuchElementException() {
        WebDriver driver = getDriver();

        String google = "http://www.google.com/ncr";
        driver.get(google);
        driver.findElement(By.name("q"));
        driver.findElement(By.name("nonameexists"));
    }

    @Test(expected = StaleElementReferenceException.class)
    public void shouldThrowStaleElementReference() {
        WebDriver driver = getDriver();

        String google = "http://www.google.com/ncr";
        driver.get(google);
        WebElement queryInput = driver.findElement(By.name("q"));

        String bbc = "http://www.bbc.co.uk/";
        driver.get(bbc);
        queryInput.getAttribute("outerHTML");
    }

}
