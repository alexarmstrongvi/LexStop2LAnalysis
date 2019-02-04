#!/bin/bash

################################################################################
# Make flat ntuples of data15-17 and mc16a/d/e SusyNts, comparing the output
# logs and branches to those from previous tests
# Also make sumw files with grabSumw and compare log outputs
################################################################################

if [ "$(basename $PWD)" != "run" ]; then
    echo "Script must be run inside of susynt-read/run directory"
    return 1
fi

################################################################################
# Globals
STORE_DIR='./old_test_results/SuperflowAnaStop2L_results' # dont add trailing backslash
SUSYNT_DIR='../../susynt-write/run/old_test_results'

################################################################################
# Test commands for various files
function strip_file_of_timestamps() {
    echo "Stripping $1 of timestamps"
    sed -i.bu '/\ 0x/d' $1 # remove printed pointers
    sed -i '/Analysis\ time/d' $1
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

function run_SuperflowAnaStop2l() {
    susynt=$1
    ofile=$2
    logfile=$3
    echo "SuperflowAnaStop2L -i $susynt -c"
    SuperflowAnaStop2L -i $susynt -c 2>&1 | tee $logfile
    mv CENTRAL*root $ofile
    
    echo
    strip_file_of_timestamps $logfile
    echo
}
function run_grabSumw() {
    susynt=$1
    logfile=$2

    echo "grabSumw -i $susynt"
    grabSumw -i $susynt 2>&1 | tee $logfile
    rm sumw_file*.root # Log file is enough for checks

    echo
    strip_file_of_timestamps $logfile
    echo
}

SAMPLES=
#MC16a (r9364)
SAMPLES="$SAMPLES mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r9364_p3652"
#MC16d (r10201)
SAMPLES="$SAMPLES mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r10201_p3627"
#MC16e (r10724)
SAMPLES="$SAMPLES mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.deriv.DAOD_SUSY2.e6348_s3126_r10724_p3627"
#Data15
SAMPLES="$SAMPLES data15_13TeV.00279515.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
#Data16
SAMPLES="$SAMPLES data16_13TeV.00298595.physics_Main.deriv.DAOD_SUSY2.r9264_p3083_p3637"
#Data17
SAMPLES="$SAMPLES data17_13TeV.00326439.physics_Main.deriv.DAOD_SUSY2.r10250_p3399_p3637"
#Data18
SAMPLES="$SAMPLES data18_13TeV.00359310.physics_Main.deriv.DAOD_SUSY2.f964_m2020_p3653"
#MC16a HIGG4D1
#SAMPLES="$SAMPLES mc16_13TeV.303778.Pythia8EvtGen_A14NNPDF23LO_Ztaumu_LeptonFilter.deriv.DAOD_HIGG4D1.e4245_s3126_r9364_p3563"

REGIONS=""
REGIONS="$REGIONS --baseline_sel"

for SAMPLE in $SAMPLES; do
    SUSYNT="${SUSYNT_DIR}/${SAMPLE}_susynt_old.root" # Assumed formatting for SusyNts 
    OFILE="${SAMPLE}_flatnt_new.root"
    LOGFILE="${SAMPLE}_flatnt_new.log"
    run_SuperflowAnaStop2l $SUSYNT $OFILE $LOGFILE

    # Test grabSumw
    #if [[ $SAMPLE == "mc16"* ]]; then
    #    SUMW_LOGFILE="${SAMPLE}_sumw_new.log"
    #    run_grabSumw $SUSYNT $SUMW_LOGFILE
    #fi

    # No longer using old ntuple maker
    #for REGION in $REGIONS; do
    #    run_flatnt_maker $SUSYNT $REGION $OFILE $LOGFILE
    #done
done

printf "\n\n ========== Performing Checks ========== \n\n"
echo ">> Comparing log files"
for new_log in *.log; do
    if [ $new_log == '*log' ]; then
        echo "No log files to compare"
        continue
    fi
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

printf "\n>> Comparing shared branches in root files\n"
for new_root in *new*root; do
    if [ $new_root == '*new*root' ]; then
        echo "No root files to compare"
        continue
    fi
    old_root=$STORE_DIR/`echo $new_root | sed 's/new/old/g'`
    if [ ! -e $old_root ]; then
        echo "No old root file found for $new_log"
        continue
    fi
    
    stripped_name="${new_root%.*}" # remove file extension
    stripped_name=`echo $stripped_name | sed 's/_new//g'`
    log_file="cf_flatnt_${stripped_name}_new.log"
    ops="$old_root $new_root -t superNt --verbose"
    python ../../PlotTools/compare_flat_ntuples.py $ops > $log_file 

    files_are_same_pattern="Different: 0"
    if grep -q "$files_are_same_pattern" $log_file; then 
        printf "\tPASS :: $new_root and $old_root files are identical\n"
    else
        printf "\tFAIL :: $new_root and $old_root files are NOT identical\n"
        printf "\tINFO :: Check with -> vim $log_file\n" 
        printf "\tINFO :: Make plots with python ../../PlotTools/compare_flat_ntuples.py $ops -s\n"
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



