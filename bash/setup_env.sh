################################################################################
# Setup the environment for the intended task
# 1) Making SusyNts
# 2) Making flat ntuples
# 3) Making plots
################################################################################
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

    echo "Setting up ATLAS software environment"
    setupATLAS

    echo -e "\n\n"
    echo "Setting up AnalysisBase 21.2.79"
    lsetup "asetup AnalysisBase,21.2.79"

    echo -e "\n\n"
    echo "Setting up recent git version"
    lsetup git

    echo -e "\n\n"
    echo "Updating python path"
    source ${HOME}/PlotTools/bash/add_to_python_path.sh
    source ${HOME}/atlasrootstyle/add_to_python_path.sh
    source source/LexStop2LAnalysis/bash/add_to_python_path.sh
    
    echo -e "\n\n"
    echo "Setting needed environment variables"
    source source/LexStop2LAnalysis/bash/setup_env_vars.sh

    echo -e "\n\n"
    echo "Setting up rest frames"
    source source/jigsawcalculator/bash/setup.sh --restframes-dir source/ 

    if [ -f build/x86*/setup.sh ]; then 
        echo -e "\n\n"
        echo "Sourcing build setup"
        source build/x86*/setup.sh
    else 
        echo "WARNING :: build files not generated yet"
    fi
    
    echo ""
    echo "Environment is setup"
    echo ""
    return 0
}
main
