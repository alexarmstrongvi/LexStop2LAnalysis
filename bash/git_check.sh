project_dir="SusyNt_AB_21_2_79"

function main() {
    # Check script is being sourced in correct directory
    if [ "$(basename "$PWD")" != "$project_dir" ]; then
        echo "ERROR :: Must run from inside project directory: $project_dir"
        return 1 
    elif [ ! -d source ] || [ ! -d build ] || [ ! -d run ]; then
        echo "ERROR :: Must run from top level directory containing source/, build/, and run/"
        return 1
    fi

    #echo "Setting up ATLAS software environment"
    #setupATLAS

    #echo -e "\n\n"
    #echo "Setting up recent git version"
    #lsetup git

    echo -e "\n\n"
    echo "Getting git summary"
    git_record=
    #update_git_record "RestFrames" in detached head state
    update_git_record "jigsawcalculator"
    update_git_record "susynt-submit"
    update_git_record "AddFakeFactorToFlatNts"
    update_git_record "IFFTruthClassifier"
    update_git_record "SusyNtCutflowLooper"
    update_git_record "superflow"
    #update_git_record "athena" in detached head state
    update_git_record "SusyCommon"
    update_git_record "SusyNtuple"
    update_git_record "LexStop2LAnalysis"

    echo -e "\nGit status summary"
    echo -e "\t- package (branch) : commits ahead and behind"
    echo -e $git_record
    
    echo ""
    echo "Environment is setup"
    echo ""
    return 0
}

function get_branch() {
    echo "$(git branch | sed -n -e 's/^\* \(.*\)/\1/p')"
}
function get_diff_summary() {
    echo "$(git rev-list --left-right --count "$(get_branch)"...origin/"$(get_branch)")"
}
function update_git_record() {
    package=${1}
    cd source/${package}
    echo "Syncing ${package}..."
    git fetch origin
    git_record="\t- ${package} ($(get_branch)) : $(get_diff_summary)\n${git_record}"
    cd ../..
}

main
