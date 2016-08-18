{% if category_path %}

### {{ category_title }}

<ul>
  {% for post in category_path reversed sort_by:title %}
  <li><a href="{{ site.url }}{{ post.url }}">{{ post.title }}</a></li>
  {% endfor %}
</ul>

{% endif %}
