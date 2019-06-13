#!/bin/bash
################################################################################
# Tests for running addFakeFactor from command line
################################################################################

TEST_DIR="/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read/source/LexStop2LAnalysis/test"
MACRO_PATH="/data/home/alarmstr/LexTools/RootMacros"
PASS_TESTS=0

run_success_test () {
    # Args
    local cmd="$1"

    # Run test
    echo "Running $cmd"
    $cmd > /dev/null 
    if [ $? -eq 0 ]; then
        echo "Test succeeded"
        return 0
    else
        echo "Test failed"
        PASS_TESTS=1
        return 1
    fi
}
run_fail_test () {
    # Args
    local cmd="$1"
    local fail_code=$2

    # Run test
    echo "Running $cmd"
    $cmd > /dev/null 
    if [ $? -eq $fail_code ]; then
        echo "Test succeeded"
        return 0
    else
        echo "Test failed"
        PASS_TESTS=1
        return 1
    fi
}

main () {

    # Real files
    local empty_file="${TEST_DIR}/data/emptyRootFile.root"
    local fake_file="${TEST_DIR}/data/dummyFakeFactorFile.root"
    local ntuple_file="${TEST_DIR}/data/dummyFlatNtupleForAddFakeFactor.root"
    local ntuple_file_solution="${TEST_DIR}/data/solutionFlatNtupleForAddFakeFactor.root"
    
    echo -e "\nChecking if help information successfully prints"
    run_fail_test "addFakeFactor -h" 3

    echo -e "\nChecking for failure when given non-existent files"
    run_fail_test "addFakeFactor -f ${empty_file} -i fileThatDoesntExist.root -t superNt -s 2T" 3
    run_fail_test "addFakeFactor -f fileThatDoesntExist.root -i ${empty_file} -t superNt -s 2T" 3
    
    echo -e "\nMaking dummy fake factor and input files"
    root -l -q ''"${MACRO_PATH}"'/makeDummyFakeFactor.cxx("'"${fake_file}"'", 0.25, 0.50)' > /dev/null
    root -l -q ''"${MACRO_PATH}"'/makeDummyFlatNtupleForAddFakeFactor.cxx("'"${ntuple_file}"'")' > /dev/null
    cp ${ntuple_file} ${ntuple_file}.bu
    
    echo -e "\nChecking for success when given correctly formatted inputs in test mode"
    run_success_test "addFakeFactor -f ${fake_file} -i ${ntuple_file} -t superNt --test -s 2T"
    
    echo -e "\nChecking that flat ntuple was not modifed when running in test mode"
    run_success_test "${MACRO_PATH}/rootFilesAreIdentical.py ${ntuple_file} ${ntuple_file}.bu"
    
    echo -e "\nChecking for success when adding fake factor for the first time"
    run_success_test "addFakeFactor -f ${fake_file} -i ${ntuple_file} -t superNt -s 2T"
    
    echo -e "\nChecking that flat ntuple was modifed"
    run_fail_test "${MACRO_PATH}/rootFilesAreIdentical.py ${ntuple_file} ${ntuple_file}.bu" 3 
    
    root -l -q ''"${MACRO_PATH}"'/makeDummyFakeFactor.cxx("'"${fake_file}"'", 0.50, 0.25)' > /dev/null
    cp ${ntuple_file} ${ntuple_file}.bu
    
    echo -e "\nChecking for success when overwriting fake factor"
    run_success_test "addFakeFactor -f ${fake_file} -i ${ntuple_file} -t superNt -s 2T"
    
    echo -e "\nChecking that flat ntuple was modifed"
    run_fail_test "${MACRO_PATH}/rootFilesAreIdentical.py ${ntuple_file} ${ntuple_file}.bu" 3 

    echo -e "\nChecking for agreement with solution ntuple" 
    run_success_test "${MACRO_PATH}/rootFilesAreIdentical.py ${ntuple_file} ${ntuple_file_solution}" 

    if [ $PASS_TESTS -eq 0 ]; then
        echo -e "\nAll tests passed"
    else
        echo -e "\nFailed tests"
    fi
    return $PASS_TESTS
}

main
