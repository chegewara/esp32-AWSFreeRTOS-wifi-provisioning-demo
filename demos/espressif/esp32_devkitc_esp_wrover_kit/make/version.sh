#!/bin/bash
echo -n `git describe --tags --long --match "*.*" | sed 's,\(.*\)-\(.*\)-g\(.*\),\1.\2-\3,'`
