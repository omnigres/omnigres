# YAML

omni_yaml provides rudimentary support for YAML in the form of two functions:

* **`omni_yaml.to_json(text)`** returning `json`: converts YAML to JSON.
* **`omni_yaml.to_yaml(json)`** returning `text`: converts JSON to YAML.

!!! tip "Why does this extension not support JSONB?"

    At this time, direct support would involve a lot more work than simply emitting JSON in its textual format.
    Casting to `jsonb` solves the problem for the time being.