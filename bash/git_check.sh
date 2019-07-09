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

    #echo -e "\n\n"
    echo "Getting git summary"
    git_record=
    #update_git_record "source/RestFrames" in detached head state
    update_git_record "source/jigsawcalculator"
    update_git_record "source/susynt-submit"
    update_git_record "source/AddFakeFactorToFlatNts"
    update_git_record "source/IFFTruthClassifier"
    update_git_record "source/SusyNtCutflowLooper"
    update_git_record "source/superflow"
    #update_git_record "source/athena" in detached head state
    update_git_record "source/SusyCommon"
    update_git_record "source/SusyNtuple"
    update_git_record "source/LexStop2LAnalysis"
    update_git_record "${HOME}/LexEnv"
    update_git_record "${HOME}/LexTools"
    update_git_record "${HOME}/PlotTools"

    echo -e "\nGit status summary"
    echo -e "\t++ package (branch) :: remote (commits ahead and behind); [# unstaged changes]++"
    echo -e "\t+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
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
    local result=""
    for remote in $(git remote); do
        # Will get error if checking for branch that doesn't exist on remote
        # So pipe error to dev/null to clean up script output
        local tmp="$(git rev-list --left-right --count "$(get_branch)"...${remote}/"$(get_branch)" 2> /dev/null)"
        if [ ! -z "$tmp" ]; then
            # tmp looks like "X\tY" where X and Y are numbers
            # change to "+X -Y"
            result="${result}${remote} (+${tmp//$'\t'/ -}); "
        fi
    done
    echo "$result"
}
function get_unstaged_changes() {
    echo "$(git diff-index HEAD -- | wc -l)"
}
function update_git_record() {
    path=${1}
    dir=$(dirname $path)
    package=$(basename $path)
    old_dir=$PWD
    cd ${dir}/${package}
    echo "Syncing ${package}..."
    git fetch --all -t -p
    git_record="\t- ${package} ($(get_branch)) \t:: $(get_diff_summary) [$(get_unstaged_changes)]\n${git_record}"
    cd $old_dir
}

main
