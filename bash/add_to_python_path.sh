if [ "$BASH_SOURCE" == "$(basename "$BASH_SOURCE")" ]; then
    # sourcing file from parent directory
    full_path="$(pwd -P)/$BASH_SOURCE"
else
    # sourcing file from outside parent directory
    full_path="$(cd "$(dirname "$BASH_SOURCE")" && pwd -P)/$(basename "$BASH_SOURCE")"
fi

parent_dir="$(dirname "$full_path")"
gparent_dir="$(dirname "$parent_dir")"
ggparent_dir="$(dirname "$gparent_dir")"

#echo "full_path = $full_path"
#echo "parent_dir = $parent_dir"
#echo "gparent_dir = $gparent_dir"
#echo "ggparent_dir = $ggparent_dir"
dir_to_add="${gparent_dir}/python"

export PYTHONPATH="$PYTHONPATH:${dir_to_add}"
echo "$dir_to_add added to PYTHONPATH"
echo "Now you can run \"import file.py\" for files in LexStop2LAnalysis/python/"
