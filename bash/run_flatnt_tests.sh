#!/bin/bash

################################################################################
# Make flat ntuples of data15-17 and mc16a/d/e SusyNts, comparing the output
# logs and branches to those from previous tests
################################################################################

if [ "$(basename $PWD)" != "run" ]; then
    echo "Script must be run inside of susynt-read/run directory"
    return 1
fi

################################################################################
# Globals
STORE_DIR='./old_test_results' # dont add trailing backslash
SUSYNT_DIR='../../susynt-write/run/old_test_results'

################################################################################
# Test commands for various files
function strip_file_of_timestamps() {
    echo "Stripping $1 of timestamps"
    sed -i.bu '/Analysis\ time/d' $1
    sed -i '/Analysis\ speed/d' $1
}

function run_flatnt_maker() {
    susynt=$1
    region=$2
    ofile=$3
    logfile=$4
    echo "makeFlatNtuples -i $susynt $region"
    makeFlatNtuples -i $susynt $region 2>&1 | tee $logfile
    mv CENTRAL*root $ofile
    
    echo
    strip_file_of_timestamps $logfile
    echo
}

SAMPLES=
#MC16a (r9364)
SAMPLES="$SAMPLES mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652"
##MC16d ()
##SAMPLES="$SAMPLES mc16_13TeV."
##MC16e ()
##SAMPLES="$SAMPLES mc16_13TeV"
##Data15
#SAMPLES="$SAMPLES data15_13TeV.00279515.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
##Data16
#SAMPLES="$SAMPLES data16_13TeV.00298595.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
##Data17
#SAMPLES="$SAMPLES data17_13TeV.00326439.physics_Main.deriv.DAOD_SUSY2.r10250_p3399_p3637"
###MC16a HIGG4D1
##SAMPLES="$SAMPLES mc16_13TeV.303778.Pythia8EvtGen_A14NNPDF23LO_Ztaumu_LeptonFilter.deriv.DAOD_HIGG4D1.e4245_s3126_r9364_p3563"

REGIONS=""
REGIONS="$REGIONS --baseline_sel"

for SAMPLE in $SAMPLES; do
for REGION in $REGIONS; do
    SUSYNT="${SUSYNT_DIR}/${SAMPLE}_susynt_old.root" # Assumed formatting for SusyNts 
    OFILE="${SAMPLE}_flatnt_new.root"
    LOGFILE="${SAMPLE}_flatnt_new.log"
    run_flatnt_maker $SUSYNT $REGION $OFILE $LOGFILE
done
done

printf "\n\n ========== Performing Checks ========== \n\n"
echo ">> Comparing log files"
for new_log in *.log; do
    old_log=$STORE_DIR/`echo $new_log | sed 's/new/old/g'`
    if [ ! -e $old_log ]; then
        echo "No old log file found for $new_log"
        continue
    fi
    
    if cmp -s $new_log $old_log; then
        printf "\tPASS :: $new_log and $old_log logs are identical\n"
    else
        printf "\tFAIL :: $new_log and $old_log logs are NOT identical\n"
        printf "\tCheck with -> vimdiff $new_log $old_log\n"
    fi
done

printf "\n>> Comparing root files\n"
for new_root in *new*root; do
    old_root=$STORE_DIR/`echo $new_root | sed 's/new/old/g'`
    if [ ! -e $old_root ]; then
        echo "No old root file found for $new_log"
        continue
    fi
    
    stripped_name="${new_root%.*}" # remove file extension
    stripped_name=`echo $stripped_name | sed 's/_new//g'`
    log_file="cf_flatnt_${stripped_name}_new.log"
    ops="$old_root $new_root -t superNt -s --verbose"
    python ../../PlotTools/compare_flat_ntuples.py $ops > $log_file 

    files_are_same_pattern="Different: 0"
    if grep -q "$files_are_same_pattern" $log_file; then 
        printf "\tPASS :: $new_root and $old_root files are identical\n"
    else
        printf "\tFAIL :: $new_root and $old_root files are NOT identical\n"
        printf "\tCheck with -> vim $log_file" 
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
    rename _new _old *root
    rename _new _old *log
    rename _new _old *log.bu
    mv *_old* ${STORE_DIR}/
elif [ "$overwrite" == "N" ]; then
    echo "New files saved without overwriting old files"
fi



