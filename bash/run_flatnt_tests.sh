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
SUSYNT_DIR='./local_samples'

################################################################################
# Test commands for various files
function strip_file_of_timestamps() {
    echo "Stripping $1 of timestamps"
    sed -i.bu '/\ 0x/d' $1 # remove printed pointers
    sed -i '/Analysis\ time/d' $1
    sed -i '/Analysis\ speed/d' $1
}

function run_SuperflowAnaStop2l() {
    susynt=$1
    selection=$2
    ofile=$3
    logfile=$4
    echo "SuperflowAnaStop2L -i $susynt -c -s $selection -n 10000"
    SuperflowAnaStop2L -i $susynt -c -s $selection -n 10000 2>&1 | tee $logfile
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

SELECTION="baseline_DF"
SAMPLES=
#SAMPLES="$SAMPLES group.phys-susy.mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.SusyNt.mc16a.p3703_n0307_nt"
#SAMPLES="$SAMPLES group.phys-susy.mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.SusyNt.mc16d.p3703_n0307_nt"
#SAMPLES="$SAMPLES group.phys-susy.mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.SusyNt.mc16e.p3703_n0307_nt"
#SAMPLES="$SAMPLES group.phys-susy.data15_13TeV.00284285.physics_Main.SusyNt.p3704_n0307_nt"
#SAMPLES="$SAMPLES group.phys-susy.data16_13TeV.00299584.physics_Main.SusyNt.p3704_n0307_nt"
SAMPLES="$SAMPLES group.phys-susy.data17_13TeV.00338377.physics_Main.SusyNt.p3704_n0307_nt"
#SAMPLES="$SAMPLES group.phys-susy.data18_13TeV.00359872.physics_Main.SusyNt.p3704_n0307_nt"
#MC16a HIGG4D1
#SAMPLES="$SAMPLES mc16_13TeV.303778.Pythia8EvtGen_A14NNPDF23LO_Ztaumu_LeptonFilter.deriv.DAOD_HIGG4D1.e4245_s3126_r9364_p3563"

SAMPLE="group.phys-susy.mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.SusyNt.mc16a.p3703_n0307_nt" 
SELECTIONS=
# baseline_DF run above
#SELECTIONS="$SELECTIONS baseline_SF"
#SELECTIONS="$SELECTIONS zjets2l_inc"
#SELECTIONS="$SELECTIONS zjets3l"
#SELECTIONS="$SELECTIONS fake_baseline_DF"
#SELECTIONS="$SELECTIONS fake_baseline_SF"
#SELECTIONS="$SELECTIONS fake_zjets3l"

for sample in $SAMPLES; do
    SUSYNT="${SUSYNT_DIR}/${sample}/" 
    OFILE="${sample}_${SELECTION}_flatnt_new.root"
    LOGFILE="${sample}_${SELECTION}_flatnt_new.log"
    run_SuperflowAnaStop2l $SUSYNT $SELECTION $OFILE $LOGFILE

    #Test grabSumw
    if [[ $sample == "mc16"* ]]; then
        SUMW_LOGFILE="${sample}_sumw_new.log"
        #run_grabSumw $SUSYNT $SUMW_LOGFILE
    fi
done

for sel in $SELECTIONS; do
    SUSYNT="${SUSYNT_DIR}/${SAMPLE}/" 
    OFILE="${SAMPLE}_${sel}_flatnt_new.root"
    LOGFILE="${SAMPLE}_${sel}_flatnt_new.log"
    run_SuperflowAnaStop2l $SUSYNT $sel $OFILE $LOGFILE
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



