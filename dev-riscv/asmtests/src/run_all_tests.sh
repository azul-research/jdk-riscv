#!/bin/bash

set -e

"./build/tester" -l | while read line; do ./src/run_test.sh "$line"; done
