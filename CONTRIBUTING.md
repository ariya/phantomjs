# Documentation Contribution Guide

This page describes how to contribute changes to PhantomJS **Documentation**, to contribute to the core project, please see the [core PhantomJS contribution guide](https://github.com/ariya/phantomjs/blob/master/CONTRIBUTING.md).

## 1) From the Documentation itself

The easiest way to contribute is to click the "improve this document" link from any page in the docs, these links take you to the Edit page on GitHub to edit and Pull Request changes immediately.

## 2) With Jekyll

The phantomjs.org site and documentation is written with [Jekyll](http://jekyllrb.com), it can be installed using `gem install jekyll`.

The [Jekyll Docs](http://jekyllrb.com/docs/home/) explain it best, but in summary;

* Once installed, running `jekyll build -w` watches your clone of the project and regenerates the _site directory whenever anything changes.
* Running `jekyll serve` will serve your clone of the site locally at http://localhost:4000.
* All content is managed in the _posts directory.
* Files must be named in the format YYYY-MM-DD-*.md for Jekyll to notice them.
* The _layouts directory contains the overall design templates.
  * post.html is the layout used for the majority of the site.
  * post.html extends document.html, which is the overall site design, head element etc.
  * default.html is the home page.
* The _includes directory contains small snippets of reused HTML, mainly the left hand navigation elements used in the API docs.

## Liquid Templates

Jekyll uses [Liquid Templates](http://wiki.shopify.com/Liquid) for interpolation, conditionals etc.

## Front Matter

Every file has a small header area which is called [Jekyll Front Matter](http://jekyllrb.com/docs/frontmatter/). It's basically meta data about the file. We use it to define the permalink for each page, as well as the categories/tags it should be listed under for being listed in various menus and categories and the URL it should be found at.

    ---
    layout: post                         # which "design" to use from the _layouts directory
    title: Web Server Module             # the h1 and title element values
    categories: api webserver            # tags to assign to this page for inclusion in navs
    permalink: api/webserver/index.html  # the url this page should reside at
    ---

## GitHub

GitHub integrates very well with Jekyll, all you need to do is push your changes and GitHub will automatically do a `jekyll build` on the server site to update your gh-pages generated website.

## Important _config.yml

When developing locally or publishing to GitHub comment in/out these two URL values accordingly so that links resolve properly in your environment.

	# When pushing to GitHub
	# url: http://ariya.github.io/phantomjs

	# When Developing Locally
	url: http://localhost:4000
