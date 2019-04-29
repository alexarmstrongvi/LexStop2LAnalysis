#!/bin/bash
#echo "TESTING :: BASH_SOURCE = ${BASH_SOURCE}"
if [ $0 != "-bash" ]; then
    echo "ERROR :: Must source directory"
else
    full_path="$(cd "$(dirname "$(dirname "$BASH_SOURCE")")" && pwd -P)/python"
    export PYTHONPATH="$PYTHONPATH:${full_path}"
    echo "$full_path added to PYTHONPATH"
    #echo 
    #echo "See for yourself..."
    #echo "Running: echo \$PYTHONPATH | tr \":\" \"\\n\" | tail -n 3"
    #echo $PYTHONPATH | tr ":" "\n" | tail -n 3
fi
