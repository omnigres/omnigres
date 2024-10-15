#!/bin/bash
dot -Tsvg -orule_dependencies.svg rule_dependencies.dot
dot -Tpng -orule_dependencies.png rule_dependencies.dot
