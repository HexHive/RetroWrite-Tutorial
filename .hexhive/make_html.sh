#!/bin/sh
python -m markdown -x fenced_code README.md > README.html
python -m markdown -x fenced_code 01-native_fuzzing/README.md > Phase01.html
python -m markdown -x fenced_code 02-coverage_fuzzing/README.md > Phase02.html
python -m markdown -x fenced_code 03-coverage_sanitized_fuzzing/README.md > Phase03.html

