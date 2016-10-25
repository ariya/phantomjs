package ghostdriver;

import org.junit.Test;
import org.openqa.selenium.WebDriver;

public class DriverBasicTest extends BaseTest {
    @Test
    public void useDriverButDontQuit() {
        WebDriver d = getDriver();
        disableAutoQuitDriver();

        d.get("http://www.google.com/");
        d.quit();
    }

//    @Test
//    public void shouldSurviveExecutingManyTimesTheSameCommand() {
//        WebDriver d = getDriver();
//        d.get("http://www.google.com");
//        for (int j = 0; j < 100; j++) {
//            try {
//                d.findElement(By.linkText(org.apache.commons.lang3.RandomStringUtils.randomAlphabetic(4))).isDisplayed();
//            } catch (NoSuchElementException nsee) {
//                // swallow exceptions: we don't care about the result
//            }
//        }
//    }
//
//    @Test
//    public void shouldSurviveExecutingManyTimesTheSameTest() throws Exception {
//        for (int i = 0; i < 100; ++i) {
//            prepareDriver();
//            shouldSurviveExecutingManyTimesTheSameCommand();
//            quitDriver();
//        }
//    }
}
