#!/bin/bash

################################################################################
# Makes SusyNts from data15-17 and mc16a/d/e samples, comparing the output logs
# messages to those from previous tests.
# TODO: Added script for comparing the files themselves, not just logs
################################################################################

if [ "$(basename $PWD)" != "run" ]; then
    echo "Script must be run inside of susynt-write/run directory"
    return 1
fi
################################################################################
# Globals
DAOD_DIR='./test_daod_samples'
STORE_DIR='./old_test_results' # dont add trailing backslash
NEVTS='1000'

################################################################################
# Test commands for various files
function strip_file_of_timestamps() {
    echo "Stripping $1 of timestamps"
    sed -i.bu '/\ 0x/d' $1 # remove printed pointers
    sed -i '/Analysis\ time/d' $1
    sed -i '/Analysis\ speed/d' $1
}
function run_ntmaker() {
    susynt_dir=$1
    sample=$2
    ofile=$3 # no extension
    nevts=$4
    printf '\n'
    echo NtMaker --mctype mc16a --prw-auto -f $susynt_dir -i $sample --outfilename ${ofile}.root -n $4
    NtMaker --mctype mc16a --prw-auto -f $susynt_dir -i $sample --outfilename ${ofile}.root -n $nevts 2>&1 | tee ${ofile}.log
    
    grep "WARNING\|ERROR" ${ofile}.log > ${ofile}_warnings.log
    echo    
    strip_file_of_timestamps ${ofile}.log
    echo
}

################################################################################
# Test Samples
SAMPLES=
#MC16a (r9364)
SAMPLES="$SAMPLES mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652"
#MC16d ()
#SAMPLES="$SAMPLES mc16_13TeV."
#MC16e ()
#SAMPLES="$SAMPLES mc16_13TeV"
#Data15
SAMPLES="$SAMPLES data15_13TeV.00279515.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
#Data16
SAMPLES="$SAMPLES data16_13TeV.00298595.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
#Data17
SAMPLES="$SAMPLES data17_13TeV.00326439.physics_Main.deriv.DAOD_SUSY2.r10250_p3399_p3637"
##MC16a HIGG4D1
#SAMPLES="$SAMPLES mc16_13TeV.303778.Pythia8EvtGen_A14NNPDF23LO_Ztaumu_LeptonFilter.deriv.DAOD_HIGG4D1.e4245_s3126_r9364_p3563"

for SAMPLE in $SAMPLES; do
    SUSYNT_DIR="${DAOD_DIR}/${SAMPLE}/"
    OFILE="${SAMPLE}_susynt_new" #extentions added by run_ntmaker
    run_ntmaker $SUSYNT_DIR $SAMPLE $OFILE $NEVTS
done

printf "\n\n ========== Performing Checks ========== \n\n"

for new_log in *.log; do
    old_log=$STORE_DIR/`echo $new_log | sed 's/new/old/g'`
    if [ ! -e $old_log ]; then
        echo "No old log file found for $new_log"
        continue
    fi
    if cmp -s $new_log $old_log; then
        echo "PASS :: $new_log and $old_log samples are identical"
    else
        echo "FAIL :: $new_log and $old_log samples are NOT identical"
        echo "Check with -> vimdiff $new_log $old_log"
    fi
done

printf "\n\n ========== Checks Complete ========== \n\n"

printf "\nDo you want to overwrite the old files? (Y/N): "
read overwrite
until [ "$overwrite" == "Y" ] || [ "$overwrite" == "N" ]; do
    echo "Unacceptable Answer: ${overwrite}"
    printf "\nDo you want to overwrite the old files? (Y/N): "
    read overwrite
done
if [ "$overwrite" == "Y" ]; then
    echo "Overwriting files in ${STORE_DIR}"
    rename _new _old *_new*
    mv *_old* ${STORE_DIR}/
elif [ "$overwrite" == "N" ]; then
    echo "New files saved without overwriting old files"
fi
