<ul>

{% assign category_items = category_docs %}
{% assign kind = 'doc' %}
{% assign kind_key = 'D' %}
{% include nav-list-group.html %}

{% assign category_items = category_properties %}
{% assign kind = 'property' %}
{% assign kind_key = 'P' %}
{% include nav-list-group.html %}

{% assign category_items = category_methods %}
{% assign kind = 'method' %}
{% assign kind_key = 'M' %}
{% include nav-list-group.html %}

{% assign category_items = category_handlers %}
{% assign kind = 'handler' %}
{% assign kind_key = 'H' %}
{% include nav-list-group.html %}

</ul>
