{% raw %}
/*{% include "../src/instantiate.sql" %}*/
{% endraw %}
select instantiate(schema => '{{ cookiecutter.schema }}');
