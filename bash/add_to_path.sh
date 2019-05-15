#!/bin/bash
if [ $0 != "-bash" ]; then
    echo "ERROR :: Must source file"
else
    full_path="$(cd "$(dirname "$(dirname "$BASH_SOURCE")")" && pwd -P)/python"
    export PYTHONPATH="$PYTHONPATH:${full_path}"
    echo "$full_path added to PYTHONPATH"
fi
