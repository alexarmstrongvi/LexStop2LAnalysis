#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re

################################################################################
# Configuration
################################################################################
work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
input_files_dir = '%s/run/lists/file_lists_prefixed' % work_dir
susynt_tag = 'n0306'
superflow = True; sumw = False
if superflow:
    print "INFO :: Setting up to check Superflow output"
    pass_phrase = "SuperflowAnaStop2L    Done." # For SuperflowAnaStop2L
    log_file_dir = '%s/run/batch/SuperflowAnaStop2l_output' % (work_dir)
    ofile_name = "%s/rerun_these_files.txt" % log_file_dir
    rerun_cmd = "python submit_to_condor.py `cat %s` -o SuperflowAnaStop2l_output/ --overwrite" % os.path.relpath(ofile_name, os.getcwd())
elif sumw:
    print "INFO :: Setting up to check SumW output"
    pass_phrase = "Sumw job done" # For grabSumw
    log_file_dir = '%s/run/batch/sumw_output' % (work_dir)
    ofile_name = "%s/rerun_these_files.txt" % log_file_dir
    rerun_cmd = "python submit_to_condor.py -e grabSumw `cat %s` -o ./sumw_output/ --overwrite" % os.path.relpath(ofile_name, os.getcwd())

################################################################################
# Where the magic happens
################################################################################
log_glob_cmd = "%s/*out" % os.path.relpath(log_file_dir, os.getcwd())
log_files = glob.glob(log_glob_cmd)
input_glob_cmd = '%s/*/*' % input_files_dir
input_files = glob.glob(input_glob_cmd)

print "INFO :: Processing %d batch job output files\n" % len(log_files)
bash_cmd = "grep -L \"%s\" %s" % (pass_phrase, log_glob_cmd)
print "INFO :: Looking for incomplete files"
print "INFO :: >> grep -L \"%s\" %s" % (pass_phrase, log_glob_cmd)
not_complete_files = get_cmd_output(bash_cmd)
bash_cmd = "find %s -type f -empty" % (log_glob_cmd)
print "INFO :: Looking for empty files"
print "INFO :: >>", bash_cmd, '\n' 
empty_files = get_cmd_output(bash_cmd)
get_input_files = [f for f in not_complete_files if f not in empty_files]

if len(get_input_files):
    print "INFO :: Searching through %d input files to build rerun list" % len(input_files)
    files_to_rerun = []
    for f in get_input_files:
        pattern = "%s[a-z]?.out" % susynt_tag
        if re.search(pattern, f): # Not a split sample
            for ifile in input_files:
                basename = os.path.basename(ifile).replace(".txt","")
                if basename in f:
                    files_to_rerun.append(ifile.strip())
                    break
            else:
                print "WARNING :: Unable to find input file for", f.strip()

        else:
            # Split file was incomplete grabbing only the incomplete xrootd link"
            for line in open(f.strip(),'r'):
                if "root://" in line and "susyNt.root" in line:
                    start = line.find("root://")
                    end = line.find("susyNt.root") + len("susyNt.root")
                    xrootd_link = line[start:end]
                    files_to_rerun.append(xrootd_link)
                    break
            else:
                print "WARNING :: Unable to find input xrootd file", f.strip()

if len(not_complete_files):
    print "INFO :: %d files are incomplete" % len(not_complete_files)
    print "INFO :: %d of those are empty and likely still running" % len(empty_files)
    print "INFO :: %d likely failed and should be rerun" % len(files_to_rerun)
    with open(ofile_name,'w') as ofile:
        ofile.write('\n'.join(files_to_rerun))
    print "INFO :: Files to rerun were written to %s" % os.path.relpath(ofile_name, os.getcwd())
    print rerun_cmd
else:
    print "INFO :: All files passed :)"

