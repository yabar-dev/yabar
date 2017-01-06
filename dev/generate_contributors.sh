#!/bin/bash
# Script to create contributors file from git log

set -e;

CONTRIBUTORSFILE=${FILE:-CONTRIBUTORS}

echo "# yabar contributors (as of $(date +%F))" > "$CONTRIBUTORSFILE"

git log --format='%aN <%aE>' | sort -u >> "$CONTRIBUTORSFILE"
