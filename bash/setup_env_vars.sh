################################################################################
# Common directories
################################################################################
# Full path to cloned repository of susynt-read
export SUSYNT_DIR='/data/uclhc/uci/user/armstro1/Analysis_Stop2L/SusyNt_AB_21_2_79'

# Source code directory
export SOURCE_DIR="${SUSYNT_DIR}/source"
export SUPERFLOW_DIR="${SOURCE_DIR}/Superflow"
export SUSYNTUPLE_DIR="${SOURCE_DIR}/SusyNtuple"

# Directory for storing the outputs from running source code
export RUN_DIR="${SUSYNT_DIR}/run"

# Directory for storing build results
export BUILD_DIR="${SUSYNT_DIR}/build"

################################################################################
# Used in condor submission scripts
################################################################################
# Directory containing all files that may be tarred and sent with job  
export TAR_DIR=$SUSYNT_DIR

# File path to store tar file that is sent with jobs

# Executables
export SF_EXEC="SuperflowAnaStop2L"
export CONDOR_EXEC="${SF_EXEC}"

# Sum of weights file for use in multi-period processing
#export SUMW_FILE="${SUSYNT_DIR}/data/sumw_file.root"
export SUMW_FILE="${RUN_DIR}/sumw_files/sumw_file_mc16ade.root"

# Directory for dumping the output of batch jobs
export BATCH_OUTPUT_DIR="${RUN_DIR}/batch/condor_output"

