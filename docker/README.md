# PhantomJS Docker Image

## Supported tags and respective `Dockerfile` links

  * [`3`, `3-debian`, `3.0`, `3.0-debian`, `3.0.0`, `3.0.0-debian`, `latest`](https://github.com/fabiocicerchia/phantomjs/blob/master/docker/Dockerfile.debian)

  * [`3`, `3-alpine`, `3.0`, `3.0-alpine`, `3.0.0`, `3.0.0-alpine`, `latest`](https://github.com/fabiocicerchia/phantomjs/blob/master/docker/Dockerfile.alpine)
  * [`3-debian`, `3.0-debian`, `3.0.0-debian`](https://github.com/fabiocicerchia/phantomjs/blob/master/docker/Dockerfile.debian)
  * [`3-ubuntu`, `3.0-ubuntu`, `3.0.0-ubuntu`](https://github.com/fabiocicerchia/phantomjs/blob/master/docker/Dockerfile.ubuntu)
  * [`3-fedora`, `3.0-fedora`, `3.0.0-fedora`](https://github.com/fabiocicerchia/phantomjs/blob/master/docker/Dockerfile.fedora)

## What is PhantomJS

[PhantomJS][phantomjs] is a headless WebKit scriptable with JavaScript. Often used via [WebDriver][webdriver] for web system testing.

### Use Cases

- **Headless web testing**. Lightning-fast testing without the browser is now possible!
- **Page automation**. [Access and manipulate](http://phantomjs.org/page-automation.html) web pages with the standard DOM API, or with usual libraries like jQuery.
- **Screen capture**. Programmatically [capture web contents](http://phantomjs.org/screen-capture.html), including CSS, SVG and Canvas. Build server-side web graphics apps, from a screenshot service to a vector chart rasterizer.
- **Network monitoring**. Automate performance analysis, track [page loading](http://phantomjs.org/network-monitoring.html) and export as standard HAR format.


## Features of this image

This [Dockerized][docker] version of PhantomJS is:

 * **Small**: Using [Debian image][debian], and removing packages used during build.
 * **Simple**: Exposes default port, easy to extend.
 * **Secure**: Runs as non-root UID/GID `19534` (selected randomly to avoid mapping to an existing user) and uses [dumb-init](https://github.com/Yelp/dumb-init) to reap zombie processes.


## Usage

### JavaScript interactive shell
 
Start PhantomJS in [REPL](http://phantomjs.org/repl.html):

    $ docker run -it --rm fabiocicerchia/phantomjs
    >

### Remote WebDriver

Start as 'Remote WebDriver mode' (embedded [GhostDriver](https://github.com/detro/ghostdriver)):

    $ docker run -d -p 8910:8910 fabiocicerchia/phantomjs phantomjs --webdriver=8910

To connect to it (some examples per language):

  * Java:

        WebDriver driver = new RemoteWebDriver(
            new URL("http://127.0.0.1:8910"),
            DesiredCapabilities.phantomjs());

  * Python (after running [`$ pip install selenium`](https://pypi.python.org/pypi/selenium/)):
  
        from selenium import webdriver
        from selenium.webdriver.common.desired_capabilities import DesiredCapabilities

        driver = webdriver.Remote(
            command_executor='http://127.0.0.1:8910',
            desired_capabilities=DesiredCapabilities.PHANTOMJS)

        driver.get('http://example.com')
        driver.find_element_by_css_selector('a[title="hello"]').click()
        
        driver.quit()

## Feedbacks

Improvement ideas and pull requests are welcome via
[Github Issue Tracker](https://github.com/wernight/docker-phantomjs/issues).

[phantomjs]:        http://phantomjs.org/
[docker]:           https://www.docker.io/
[debian]:           https://registry.hub.docker.com/_/debian/
[webdriver]:        http://www.seleniumhq.org/projects/webdriver/
