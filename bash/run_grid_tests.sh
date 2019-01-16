#!/bin/bash

################################################################################
# Make flat ntuples and sum of weights files for data15-17 and mc16a/d/e SusyNts
# on the grid. Compare output and error files as well as the flat ntuples
#
# TODO: Add test to submit same job to different sites to check that they all work
# TODO: Add test to run on files stored at different sites
# TODO: Add functionality to parse outputs and log information related to how
#       long jobs are taking. Log information should include things like where
#       the job was run, how many events, which sample, event processing rate,
#       total time of job, etc...
#
################################################################################

if [ "$(basename $PWD)" != "run" ]; then
    echo "Script must be run inside of susynt-read/run directory"
    return 1
fi

################################################################################
# Globals
STORE_DIR='./old_test_results/grid_tests' # dont add trailing backslash
FILELIST_DIR='./lists/file_lists_prefixed'
TEST_FILELIST_DIR='./lists/test_filelists'

################################################################################
function remove_grid_log_timestamps() {
    sed -i.bu '/\ 0x/d' $1 # remove printed pointers
    sed -i '/[0-9][0-9]\ [0-9][0-9]:[0-9][0-9]/d' $1
    sed -i '/[0-9][0-9]h:[0-9][0-9]m:[0-9][0-9]s/d' $1
    sed -i '/condor\/execute/d' $1 # remove listings of condor directories
}

function submit_flatnt_maker() {

    #remove_grid_log_timestamps $logfile

}

function submit_grabSumw() {

    #remove_grid_log_timestamps $logfile

}

################################################################################
# Filelists to loop over
SAMPLES=
#MC16a (r9364)
SAMPLES="$SAMPLES ${FILELIST_DIR}/mc16a/*410472.PhPy8EG_A14_ttbar_hdamp258p75_dil*txt"
#MC16d (r10201)
SAMPLES="$SAMPLES ${FILELIST_DIR}/mc16d/*410472.PhPy8EG_A14_ttbar_hdamp258p75_dil*txt"
#MC16e (r10724)
SAMPLES="$SAMPLES ${FILELIST_DIR}/mc16e/*410472.PhPy8EG_A14_ttbar_hdamp258p75_dil*txt"
#Data15
SAMPLES="$SAMPLES ${FILELIST_DIR}/data15/*279515*txt"
#Data16
SAMPLES="$SAMPLES ${FILELIST_DIR}/data16/*298595*txt"
#Data17
SAMPLES="$SAMPLES ${FILELIST_DIR}/data17/*326439*txt"
#Data18
SAMPLES="$SAMPLES ${FILELIST_DIR}/data18/*359310*txt"

################################################################################
# Copy full filelists over to test directory with only some samples
# to make the jobs go faster

################################################################################
# Submit grid jobs
for SAMPLE in $SAMPLES; do
    echo "TESTING :: submitting flat ntuple maker $SAMPLE"
    #submit_flatnt_maker $SAMPLE $ODIR

    if [[ $SAMPLE == *"mc16"* ]]; then
        echo "TESTING :: submitting grabSumw maker $SAMPLE"
        #submit_grabSumw $SAMPLE $ODIR
    fi
done

################################################################################
# Wait for jobs to finish

################################################################################
# Check outputs
