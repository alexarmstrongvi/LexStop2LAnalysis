################################################################################
# Setup the environment for the intended task
# 1) Making SusyNts
# 2) Making flat ntuples
# 3) Making plots
################################################################################

if [ "$(basename $PWD)" == "susynt-read" ]; then
    source bash/setup_release.sh  
    source source/jigsawcalculator/bash/setup.sh
    lsetup git
    echo "Environment is setup"
else
    echo "ERROR :: Only setup to be run from inside susynt-read"
fi
echo ""
