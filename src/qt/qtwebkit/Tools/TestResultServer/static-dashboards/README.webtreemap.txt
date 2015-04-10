# webtreemap

A simple treemap implementation using web technologies (DOM nodes, CSS
styling and transitions) rather than a big canvas/svg/plugin.

Play with a [demo][].

[demo]: http://martine.github.com/webtreemap/demo/demo.html

## Creating your own

1. Create a page with a DOM node (i.e. a `<div>`) that will contain
   your treemap.
2. Add the treemap to the node via something like

        appendTreemap(document.getElementById('mynode'), mydata);
3. Style the treemap using CSS.

### Input format

The input data (`mydata` in the overview snippet) is a tree of nodes,
likely imported via a separate JSON file.  Each node (including the
root) should contain data in the following format.

    {
      name: (HTML that is displayed via .innerHTML on the caption),
      data: {
        "$area": (a number, in arbitrary units)
      },
      children: (list of child tree nodes)
    }

(This strange format for data comes from the the [JavaScript InfoVis
Toolkit][thejit].  I might change it in the future.)

The `$area` of a node should be the sum of the `$area` of all of its
`children`.

(At runtime, tree nodes will dynamically will gain two extra
attributes, `parent` and `dom`; this is only worth pointing out so
that you don't accidentally conflict with them.)

### CSS styling

The treemap is constructed with one `div` per region with a separate
`div` for the caption.  Each div is styleable via the
`webtreemap-node` CSS class.  The captions are stylable as
`webtreemap-caption`.

Each level of the tree also gets a per-level CSS class,
`webtreemap-level0` through `webtreemap-level4`.  These can be
adjusted to e.g. made different levels different colors.  To control
the caption on a per-level basis, use a CSS selector like
`.webtreemap-level2 > .webtreemap-caption`.

Your best bet is to modify the included `webtreemap.css`, which
contains comments about required and optional CSS attributes.

## Related projects

* [JavaScript InfoVis Toolkit][thejit]

[thejit]: http://thejit.org/