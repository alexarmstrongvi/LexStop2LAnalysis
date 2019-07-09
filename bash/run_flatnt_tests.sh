#!/bin/bash
################################################################################
# Make flat ntuples of data15-17 and mc16a/d/e SusyNts, comparing the output
# logs and branches to those from previous tests
# Also make sumw files with grabSumw and compare log outputs
################################################################################

#let script exit if a command fails
set -o errexit 

function main() {
    if is_true $VERBOSE; then
        echo "VERBOSE (main) :: Running main"
    fi
    
    SAMPLES=""
    set_samples $SAMPLE_OP
    process_samples "$SAMPLES"

    if is_not_empty $STORE_DIR; then
        if [ "$(ls -A $STORE_DIR)" ]; then
            # Output directory is not empty
            compare_results $STORE_DIR
        else
            mv_results $STORE_DIR
        fi
    else
        # No output directory provided
        request_rm_results
    fi
    
    if is_true $VERBOSE; then
        echo "VERBOSE (main) :: Finishing main"
    fi
}

################################################################################
# Declare globals
################################################################################
RUN_DIR=$PWD
PROG_NAME="$(basename $0)"
PROG_DIR="$(dirname $0)"
PROG_PATH="$(cd "$PROG_DIR"; pwd)"
ARGS="$@"

SUSYNT_DIR='./test_susyNt_samples'
STORE_DIR=""
NEVTS="10000"

########################################
# Samples
#MC16 (a=r9364, d=r10201, e=r10724)
SAMPLE_WJETS="user.alarmstr.mc16_13TeV.364161.Sherpa221_Wmunu_70_140_BFilt.SusyNt.mc16a.p3703_n0308_nt"
SAMPLE_TTBAR="user.alarmstr.mc16_13TeV.410472.PhPy8EG_A14_ttbar_hdamp258p75_dil.SusyNt.mc16d.p3703_n0308_nt"
SAMPLE_VV="user.alarmstr.mc16_13TeV.364253.Sherpa_222_lllv.SusyNt.mc16e.p3703_n0308_nt"

SAMPLE_WJETS_NAME="mc16a_364161_Wmunu_Sherpa_221_MAXHTPTV70_140_BFilter.SUSY2.p3875_n0308"
SAMPLE_TTBAR_NAME="mc16d_410472_ttbar_dil_PhPy8EG.SUSY2.p3875_n0308"
SAMPLE_VV_NAME="mc16e_364253_VV_lllv_Sherpa_222.SUSY2.p3875_n0308"

SAMPLE_MC16A=$SAMPLE_WJETS
SAMPLE_MC16A_NAME=$SAMPLE_WJETS_NAME
SAMPLE_MC16D=$SAMPLE_TTBAR
SAMPLE_MC16D_NAME=$SAMPLE_TTBAR_NAME
SAMPLE_MC16E=$SAMPLE_VV
SAMPLE_MC16E_NAME=$SAMPLE_VV_NAME

#Data
SAMPLE_DATA15="user.alarmstr.data15_13TeV.00280950.physics_Main.SusyNt.p3704_n0308_nt"
SAMPLE_DATA16="user.alarmstr.data16_13TeV.00302872.physics_Main.SusyNt.p3704_n0308_nt"
SAMPLE_DATA17="user.alarmstr.data17_13TeV.00337705.physics_Main.SusyNt.p3704_n0308_nt"
SAMPLE_DATA18="user.alarmstr.data18_13TeV.00350880.physics_Main.SusyNt.p3704_n0308_nt"

SAMPLE_DATA15_NAME="data15_run280950.SUSY2.p3704_n0308"
SAMPLE_DATA16_NAME="data16_run302872.SUSY2.p3704_n0308"
SAMPLE_DATA17_NAME="data17_run337705.SUSY2.p3704_n0308"
SAMPLE_DATA18_NAME="data18_run350880.SUSY2.p3704_n0308"

function set_samples() {
    local sample_op="$1"
    if is_true $VERBOSE; then
        echo "VERBOSE (set_samples) :: sample option = ${sample_op}"
    fi

    if [ $sample_op == "data15" ]; then
        SAMPLES=$SAMPLE_DATA15
    elif [ $sample_op == "data16" ]; then
        SAMPLES=$SAMPLE_DATA16
    elif [ $sample_op == "data17" ]; then
        SAMPLES=$SAMPLE_DATA17
    elif [ $sample_op == "data18" ]; then
        SAMPLES=$SAMPLE_DATA18
    elif [ $sample_op == "mc16a" ]; then
        SAMPLES=$SAMPLE_MC16A
    elif [ $sample_op == "mc16d" ]; then
        SAMPLES=$SAMPLE_MC16D
    elif [ $sample_op == "mc16e" ]; then
        SAMPLES=$SAMPLE_MC16E
    elif [ $sample_op == "wjets" ]; then
        SAMPLES=$SAMPLE_WJETS
    elif [ $sample_op == "ttbar" ]; then
        SAMPLES=$SAMPLE_TTBAR
    elif [ $sample_op == "vv" ]; then
        SAMPLES=$SAMPLE_VV
    elif [ $sample_op == "all" ]; then
        SAMPLES="$SAMPLE_DATA15"
        SAMPLES="$SAMPLES $SAMPLE_DATA16"
        SAMPLES="$SAMPLES $SAMPLE_MC16A"
        SAMPLES="$SAMPLES $SAMPLE_DATA17"
        SAMPLES="$SAMPLES $SAMPLE_MC16D"
        SAMPLES="$SAMPLES $SAMPLE_DATA18"
        SAMPLES="$SAMPLES $SAMPLE_MC16E"
    else
        echo "ERROR :: Unrecognized sample option: ${sample_op}"
        exit 1
    fi
}

function get_sample_name() {
    local sample_path=${1}
    if [ "$sample_path" == "$SAMPLE_DATA15" ]; then
        echo $SAMPLE_DATA15_NAME
    elif [ "$sample_path" == "$SAMPLE_DATA16" ]; then
        echo $SAMPLE_DATA16_NAME
    elif [ "$sample_path" == "$SAMPLE_DATA17" ]; then
        echo $SAMPLE_DATA17_NAME
    elif [ "$sample_path" == "$SAMPLE_DATA18" ]; then
        echo $SAMPLE_DATA18_NAME
    elif [ "$sample_path" = "$SAMPLE_MC16A" ]; then
        echo $SAMPLE_MC16A_NAME
    elif [ "$sample_path" = "$SAMPLE_MC16D" ]; then
        echo $SAMPLE_MC16D_NAME
    elif [ "$sample_path" = "$SAMPLE_MC16E" ]; then
        echo $SAMPLE_MC16E_NAME
    elif [ "$sample_path" = "$SAMPLE_WJETS" ]; then
        echo $SAMPLE_WJETS_NAME
    elif [ "$sample_path" = "$SAMPLE_TTBAR" ]; then
        echo $SAMPLE_TTBAR_NAME
    elif [ "$sample_path" = "$SAMPLE_VV" ]; then
        echo $SAMPLE_VV_NAME
    else
        echo "$sample_path"
    fi
}

################################################################################
# Usage information
function usage() {
        # NOTE: cat <<-EOF only works if tabs are used for the following lines 
        #       of text. Spaces will cause problems
    cat <<- EOF
	############################################################################
	#    ${PROG_NAME}
	#
	#    Script to facilitate running repeatable tests producing FlatNts
	#    from a set of fixed input SusyNt samples for the Stop2L analysis
	#
	#
	#    OPTIONS:
	#       Required:
	#       -s    sample option (e.g. all, data18, ttbar, mc16d, ...)
	#
	#       Optional:
	#       -o    output directory for logs
	#       -n    number of events to process [${NEVTS}]
	#       -v    run in verbose mode
	#       -d    run with 'set -x', printing all executed commands
	#       -h    show this help
	#
	#    Examples:
	#       Run quick test on less events without saving log files
	#       >> $PROG_NAME -s data18
	#
	#       Run default benchmark test on specific sample and save log files
	#       to a new directory
	#       >> $PROG_NAME -s mc16a -o empty_results_dir
	#
	#       Run default benchmark test on all samples, saving log files into 
	#       directory with previous results. New results will be diff'd with
	#       old results before prompting the user to decide to overwrite the old
	#       results or not. New results are stored in current directory if the
	#       user decides not to overwrite the old results.
	#       >> $PROG_NAME -s all -o prev_results_dir
	############################################################################
	EOF
}

function print_configuration() {
    cat <<- EOF
	########################################################################
	#   ${PROG_NAME} Configuration
	#
	#   Run cmd: ${PROG_DIR}/${PROG_NAME} $ARGS
	#   Run from: $RUN_DIR
	#
	#   Sample option:  $SAMPLE_OP
	#   Output directory: $STORE_DIR
	#   # Events: $NEVTS
	#   Verbose: $VERBOSE
	#   Debug mode: $DEBUG
	#
	########################################################################
	EOF
}
################################################################################
# Comamand line argument parsing
function cmdline()
{
    while getopts "s:o:n:vdh" OPTION; do
        case $OPTION in
            s) SAMPLE_OP=$OPTARG ;;
            o) STORE_DIR=$OPTARG ;;
            n) NEVTS=$OPTARG ;;
            v) VERBOSE=true ;;
            d)
                DEBUG='-x'
                set -x
                ;;
            h)
                usage
                exit 0
                ;;
            \?)
            echo "Invalid option: -$OPTARG"
            exit 1;
            ;;
        esac
    done

    ############################################################################
    # Set defaults
    if is_empty $SAMPLE_OP; then
        echo "ERROR :: sample option not set"
        exit 0
    fi

    # Check arguments if not running tests
    if is_not_empty ${STORE_DIR}; then 
        if is_not_dir ${STORE_DIR}; then
            echo "ERROR :: No input file provided"
            exit 1
        fi
    fi

    ## Print configuration
    if is_true $VERBOSE; then
        print_configuration
    fi
}

################################################################################
# Helper functions
################################################################################
function is_true() {
    local bool=$1
    [ "$bool" == true ]
}

function is_false() {
    local bool=$1
    [ "$bool" == false ]
}

function is_empty() {
    local var=$1
    [[ -z $var ]]
}
function is_not_empty() {
    local var=$1
    [[ -n $var ]]
}
function is_not_dir() {
    local dir=$1
    [[ ! -d $dir ]]
}

################################################################################
# Main supporting functions
################################################################################
function process_samples() {
    local samples="$1"
    if is_true $VERBOSE; then
        echo "VERBOSE (process_samples) :: SAMPLES = ${samples}"
    fi
    for sample in $samples; do
        sample_name=$(get_sample_name $sample)
        susynt_dir="${SUSYNT_DIR}/${sample}"
        ofile="${sample_name}_FlatNt" #extensions added by run_ntmaker
        selection="baseline_DF"
        run_SuperflowAnaStop2l $susynt_dir $selection $ofile $NEVTS
    done
}

function run_SuperflowAnaStop2l() {
    susynt=$1
    selection=$2
    ofile=$3
    nevts=$4

    ########################################
    cmd="SuperflowAnaStop2L -i ${susynt}/ -c -s $selection -n $nevts"
    ########################################
    
    echo ">> ${cmd}"
    if is_empty $STORE_DIR; then
        $cmd
    else
        $cmd 2>&1 | tee ${ofile}.log
        grep "WARNING\|ERROR" ${ofile}.log > ${ofile}_warnings.log
        echo    
        strip_file_of_timestamps ${ofile}.log
        echo
    fi
    mv CENTRAL*root "${ofile}.root"
}

function strip_file_of_timestamps() {
    echo "Stripping $1 of timestamps"
    sed -i.bu '/\ 0x/d' $1 # remove printed pointers
    sed -i '/Analysis\ time/d' $1
    sed -i '/Analysis\ speed/d' $1
}

function compare_results() {
    local store_dir="$1"
    if is_true $VERBOSE; then
        echo "VERBOSE (compare_results) :: STORE_DIR = ${store_dir}"
    fi
    
    printf "\n\n ========== Performing Checks ========== \n\n"

    for new_log in *.log; do
        old_log="${store_dir}/${new_log}"
        if [ ! -e $old_log ]; then
            echo "INFO :: No old log file found for $new_log"
            continue
        fi
        if cmp -s $new_log $old_log; then
            echo "PASS :: $new_log and $old_log samples are identical"
        else
            echo "FAIL :: $new_log and $old_log samples are NOT identical"
            echo "Check with -> vimdiff $new_log $old_log"
        fi
    done
    
    printf "\n>> Comparing content of flat ntuples\n"
    for new_root in *root; do
        if [ $new_root == '*root' ]; then
            echo "No root files to compare"
            continue
        fi
        old_root=${store_dir}/${new_root}
        if [ ! -e $old_root ]; then
            echo "No old root file found for $new_log"
            continue
        fi

        stripped_name="${new_root%.*}" # remove file extension
        log_file="${stripped_name}_cf_flatnt.log"
        
        executable=~/LexTools/RootMacros/rootFilesAreIdentical.py
        ops="$old_root $new_root -d INFO"
        $executable $ops > $log_file

        if [ $? -eq 0 ]; then
            printf "PASS :: $new_root and $old_root files are identical\n"
        else
            printf "FAIL :: $new_root and $old_root files are NOT identical\n"
            printf "INFO :: Check with -> vim $log_file\n" 
            ops="$old_root $new_root -t superNt --verbose"
            printf "INFO :: Make plots with python ~/PlotTools/compare_flat_ntuples.py $ops -s\n"
        fi
        
        # Old flat ntuple comparison approach
        #executable="~/PlotTools/compare_flat_ntuples.py"
        #ops="$old_root $new_root -t superNt --verbose"
        #files_are_same_pattern="Different: 0"
        #if grep -q "$files_are_same_pattern" $log_file; then 
        #   printf "\tPASS"
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
        echo "Overwriting files in ${store_dir}"
        mv_results $store_dir
    elif [ "$overwrite" == "N" ]; then
        echo "New logs saved to current directory"
    fi
}

function mv_results() {
    local store_dir="$1"
    if is_true $VERBOSE; then
        echo "VERBOSE (mv_results) :: STORE_DIR = ${store_dir}"
    fi
    for sample in $SAMPLES; do
        sample_name=$(get_sample_name $sample)
        if is_true $VERBOSE; then
            echo "mv ${sample_name}* ./${store_dir}"
        fi
        mv ${sample_name}* ./${store_dir}
    done
}

function request_rm_results() {
    printf "\nDo you want to remove the output files? (Y/N): "
    read overwrite
    until [ "$overwrite" == "Y" ] || [ "$overwrite" == "N" ]; do
        echo "Unacceptable Answer: ${overwrite}"
        printf "\nDo you want to remove the output files? (Y/N): "
        read overwrite
    done
    if [ "$overwrite" == "Y" ]; then
        for sample in $SAMPLES; do
            sample_name=$(get_sample_name $sample)
            if is_true $VERBOSE; then
                echo "rm ${sample_name}*"
            fi
            rm ${sample_name}*
        done
    elif [ "$overwrite" == "N" ]; then
        echo "Outputs left in current directory"
    fi
}

################################################################################
# Run main program
cmdline $ARGS
main

set +x
exit 0

SELECTIONS=
SELECTIONS="$SELECTIONSbaseline_DF"
#SELECTIONS="$SELECTIONS baseline_SF"
#SELECTIONS="$SELECTIONS zjets2l_inc"
#SELECTIONS="$SELECTIONS zjets3l"
#SELECTIONS="$SELECTIONS fake_baseline_DF"
#SELECTIONS="$SELECTIONS fake_baseline_SF"
#SELECTIONS="$SELECTIONS fake_zjets3l"

