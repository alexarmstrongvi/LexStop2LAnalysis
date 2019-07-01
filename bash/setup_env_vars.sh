################################################################################
# Common directories
################################################################################
# Full path to cloned repository of susynt-read
export SUSYNT_READ_PARENT_DIR='/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master'
export SUSYNT_READ_DIR="${SUSYNT_READ_PARENT_DIR}/susynt-read"

# Source code directory
export SOURCE_DIR="${SUSYNT_READ_DIR}/source"
export SUPERFLOW_DIR="${SOURCE_DIR}/Superflow"
export SUSYNTUPLE_DIR="${SOURCE_DIR}/SusyNtuple"

# Directory for storing the outputs from running source code
export RUN_DIR="${SUSYNT_READ_DIR}/run"

# Directory for storing build results
export BUILD_DIR="${SUSYNT_READ_DIR}/build"

################################################################################
# Used in condor submission scripts
################################################################################
# Directory containing all files that may be tarred and sent with job  
export TAR_DIR=$SUSYNT_READ_DIR

# File path to store tar file that is sent with jobs
#export TAR_FILE="${SUSYNT_READ_PARENT_DIR}/area.tgz"
export TAR_FILE="${RUN_DIR}/batch/condor_output/area.tgz"


# Executables
export SF_EXEC="SuperflowAnaStop2L"
export CONDOR_EXEC="${SF_EXEC}"

# Sum of weights file for use in multi-period processing
#export SUMW_FILE="${SUSYNT_READ_DIR}/data/sumw_file.root"
export SUMW_FILE="${RUN_DIR}/sumw_files/sumw_file_mc16ade.root"

# Directory for dumping the output of batch jobs
export BATCH_OUTPUT_DIR="${RUN_DIR}/batch/condor_output"

