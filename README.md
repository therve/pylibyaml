# PyLibYAML

PyLibYAML is a Python library for parsing YAML, using LibYAML.
It only cares about a subset of YAML:

- It only parses data to string (no numbers, null, or booleans).
- It ignores tags, anchors and refs.
- It only implements the parsing in C, not duplicating the parsing in several places.

It's a bit inspired by [StrictYAML](https://github.com/crdoconnor/strictyaml), except
more focused on performance and even more stripped down.
