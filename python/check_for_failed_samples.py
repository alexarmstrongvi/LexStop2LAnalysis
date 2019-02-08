#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re
import sys
from collections import defaultdict

################################################################################
# Configuration
################################################################################
work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
input_files_dir = '%s/run/lists/file_lists_prefixed' % work_dir
superflow = False; sumw = True
dumb_run = False

if superflow:
    print "INFO :: Setting up to check Superflow output"
    pass_phrase = "SuperflowAnaStop2L    Done." # For SuperflowAnaStop2L
    log_file_dir = '%s/run/batch/SuperflowAnaStop2l_output' % (work_dir)
elif sumw:
    print "INFO :: Setting up to check SumW output"
    pass_phrase = "Sumw job done" # For grabSumw
    log_file_dir = '%s/run/batch/sumw_output' % (work_dir)

finished_phrase = "Normal termination"

# This command might help clear old log information from previous submissions
#bash_cmd = 'sed -i "1,/TotalReceivedBytes/d" `grep -ro "TotalReceivedBytes" %s | uniq -c | grep -oP "(?<=[2-9] ).*log"`' % log_glob_cmd


################################################################################
# Where the magic happens
################################################################################
out_glob_cmd = "%s/*out" % os.path.relpath(log_file_dir, os.getcwd())
log_glob_cmd = "%s/*log" % os.path.relpath(log_file_dir, os.getcwd())
err_glob_cmd = "%s/*err" % os.path.relpath(log_file_dir, os.getcwd())
root_glob_cmd = "%s/*root" % os.path.relpath(log_file_dir, os.getcwd())

condor_submit_file = "%s/submit.condor" % os.path.relpath(log_file_dir, os.getcwd())
condor_resubmit_file = "%s/resubmit.condor" % os.path.relpath(log_file_dir, os.getcwd())

def start_of_job_queue_line(line): return line.startswith("arguments")
def end_of_job_queue_line(line): return line == 'queue\n'

n_root_files = len(glob.glob(root_glob_cmd))
all_files = set(x.replace('.log','') for x in glob.glob(log_glob_cmd))
all_files2 = set(x.replace('.out','') for x in glob.glob(out_glob_cmd))
all_files3 = set(x.replace('.err','') for x in glob.glob(err_glob_cmd))
assert len(all_files) == len(all_files2) == len(all_files3), (
    "ERROR :: .log, .out, and .err files differ: %d, %d, and %d respectively." % (len(all_files), len(all_files2), len(all_files3)),
    all_files - all_files2
)

print "INFO :: Processing %d batch job output files" % len(all_files)

print "INFO :: Looking for completed files"
bash_cmd = "grep -l \"%s\" %s" % (pass_phrase, out_glob_cmd)
print "INFO :: >>", bash_cmd, '\n' 
complete_files = get_cmd_output(bash_cmd)
complete_files = set(f.strip().replace(".out","") for f in complete_files)

if dumb_run:
    print "WARNING :: DUMB RUN: Resubmit all files without successful run"
    resubmit_files = set(f for f in all_files if f not in complete_files)

else:
    print "INFO :: Looking for files of jobs that terminated normally"
    bash_cmd = "grep -l \"%s\" %s" % (finished_phrase, log_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    norm_term_files = get_cmd_output(bash_cmd)
    norm_term_files = set(f.split()[0].replace(".log","") for f in norm_term_files)

    # Extra checks
    print "INFO :: Getting number of active jobs"
    bash_cmd = "condor_q | tail -n1 | grep -Eo \"^[0-9]*\""
    print "INFO :: >>", bash_cmd, '\n' 
    n_active_files = int(get_cmd_output(bash_cmd)[0])

    print "INFO :: Looking for empty files"
    bash_cmd = "find %s -type f -empty" % (out_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    empty_files = get_cmd_output(bash_cmd)
    empty_files = set(f.strip().replace(".out","") for f in empty_files)

    #if n_active_files != len(empty_files):
    #    print "INFO :: condor_q returns %d running jobs but %s output files are empty" % (n_active_files, len(empty_files))
    #    print "INFO :: Cannot assume empty files are all still active (some were likely aborted)"
    #    print "INFO :: Using separate method to determine active files (this can take 10-60 seconds)"
    #    bash_cmd = "for f in %s; do echo $f \"$(condor_check_userlogs $f |& tail -n 1)\" | grep \"Log(s) have error(s)\"; done" % (log_glob_cmd)
    #    print "INFO :: >>", bash_cmd, '\n' 
    #    active_files2 = get_cmd_output(bash_cmd)
    #    active_files2 = set(f.split()[0].replace(".log","") for f in active_files2)
    #    assert n_active_files == len(active_files2), (
    #        "ERROR :: The second method also disagrees, showing %d active jobs" % len(active_files2)
    #    )
    #    active_files = active_files2
    #else:
    #    active_files = empty_files
    active_files = empty_files - norm_term_files
    failed_files = norm_term_files - complete_files
    aborted_files = empty_files & norm_term_files
    resubmit_files = failed_files | aborted_files
    finished_files = failed_files | aborted_files | complete_files

    # Checks
    assert len(complete_files) == n_root_files
    assert not (failed_files & aborted_files), (
        "Failed and aborted files:", failed_files & aborted_files
    )
    assert not (failed_files & complete_files), (
        "Failed and complete files:", failed_files & complete_files
    )
    assert len(active_files) + len(finished_files) == len(all_files), (
        "ERROR :: %d active files + %d finished != %d total files" % (len(active_files), len(finished_files), len(all_files)),
        #"INFO :: Overlapping samples:\n", active_files & finished_files & all_files
    )
    assert n_active_files == len(active_files), (
        "ERROR :: condor_q gives %d active files but %d do not have \"Normal Termination\" in them" % (n_active_files, len(active_files))
    )
    assert len(aborted_files) + len(active_files) == len(empty_files)
    assert len(failed_files) + len(aborted_files) == len(resubmit_files) 
    assert len(complete_files) + len(failed_files) + len(aborted_files) == len(finished_files)

    if len(active_files) or len(resubmit_files):
        # Error monitoring
        err_files = ["%s.err" % f for f in resubmit_files]
        if err_files:
            print '\n', "="*80
            print "INFO :: Summarizing errors occurrances for failed jobs"
            bash_cmd = 'grep -ih error %s | grep -v "tar:" | sort | uniq -c | sort -rn' % (" ".join(err_files))

            import subprocess
            subprocess.call(bash_cmd, shell=True)

        # Summarize results 
        print '\n', "="*80
        print "INFO :: Job Status"
        print "\t%d active jobs" % len(active_files)
        print "\t%d finished jobs" % len(finished_files)
        print "\t\t%d are complete" % len(complete_files)
        print "\t\t%d failed" % len(failed_files)
        print "\t\t%d were stopped early by the user" % len(aborted_files)
        print "%d jobs should be resubmitted\n" % len(resubmit_files)
    else:
        print "All files finished successfully"
        sys.exit()

if not len(resubmit_files):
    print "All files are complete or still running" 
    sys.exit()

with open(condor_resubmit_file, 'w') as ofile:
    ofile_str = ''
    resub_files = set(os.path.basename(f) for f in resubmit_files)
    with open(condor_submit_file, 'r') as ifile:
        gathering_job_cmd = False
        tmp_job_queue_cmd = ''
        for line in ifile:
            if not line.strip(): continue
            if start_of_job_queue_line(line):
                gathering_job_cmd = True

            if gathering_job_cmd:
                tmp_job_queue_cmd += line
            else:
                ofile_str += line
            
            if gathering_job_cmd and end_of_job_queue_line(line):
                if any("%s.log" % f in tmp_job_queue_cmd for f in resub_files):
                    ofile_str += '\n' + tmp_job_queue_cmd
                tmp_job_queue_cmd = ''
    ofile.write(ofile_str)

usr_msg = "Would you like to resubmit jobs?\n"
usr_msg += "NOTE: This will clear the .log, .out, and .err files of jobs being resubmitted\n"
usr_msg += "Input your answer [Y/N]: "
user_op = raw_input(usr_msg)

while user_op not in ["Y","N"]:
    usr_msg = "Unacceptable answer: %s\n" % user_op
    usr_msg += "Would you like to resubmit jobs? [Y/N] "
    user_op = raw_input(usr_msg)

if user_op == "Y":
    print "INFO :: Clearing files for resubmission..."
    for f in resubmit_files:
        clear_cmd = ''
        clear_cmd += "> %s.log;" % f
        clear_cmd += "> %s.out;" % f
        clear_cmd += "> %s.err;" % f
        subprocess.call(clear_cmd, shell=True)

    print "INFO :: Resubmitting files"
    directory = os.path.dirname(condor_resubmit_file)
    sub_file = os.path.basename(condor_resubmit_file)
    cmd = 'cd %s; condor_submit %s; cd -' % (directory, sub_file)
    subprocess.call(cmd, shell=True)
    print "\n"
else:
    print "We hope you enjoyed your experience. Come back soon. :)\n"


# OLD CODE
#split_dsid_ops = defaultdict(list)
#files_to_rerun = set()
#if len(get_input_files):
#    print "INFO :: Searching through %d input files to build rerun list" % len(input_files)
#    for f in get_input_files:
#        for ifile in input_files:
#            basename = os.path.basename(ifile).replace(".txt","")
#            if basename in f:
#                files_to_rerun.add(ifile.strip())
#                break
#        else:
#            print "WARNING :: Unable to find input file for", f.strip()
#
#        pattern = "%s[a-z]?$" % susynt_tag
#        if not re.search(pattern, f): # Split sample
#            # Split file was incomplete grabbing only the incomplete xrootd link"
#            dsid = re.search(r'[1-9][0-9]{5}(?=\.)', f).group()
#            campaign = re.search(r'mc16[ade]|data1[5678]', f).group()
#            empty_file = True
#            for line in open(f.strip()+'.out','r'):
#                empty_file = False
#                if "root://" in line and "susyNt.root" in line:
#                    start = line.find("root://")
#                    end = line.find("susyNt.root") + len("susyNt.root")
#                    xrootd_link = line[start:end]
#                    split_dsid_ops[(dsid, campaign)].append(xrootd_link) 
#                    break
#            else:
#                if not empty_file: print "WARNING :: Unable to find input xrootd file", f.strip()
#
## Build rerun command
#split_dsids_cmd = ''
#n_split_files = 0
#for (dsid, campaign), file_names in split_dsid_ops.iteritems():
#    split_dsids_cmd += ' --split-dsids %s %s' % (dsid, campaign)
#    n_split_files += len(file_names) - 1
#    for f in file_names:
#        split_dsids_cmd += ' %s' % f
#rerun_cmd += split_dsids_cmd
#
#if len(not_complete_files):
#    print "INFO :: %d files are incomplete" % len(not_complete_files)
#    print "INFO :: %d of those are empty" % len(empty_files)
#    print "INFO :: \t%d of those are active" % len(active_files)
#    print "INFO :: \t%d of those are aborted" % len(aborted_files)
#    print "INFO :: %d of those (%d samples) selected to be rerun" % (len(files_to_rerun) + n_split_files, len(files_to_rerun))
#    with open(ofile_name,'w') as ofile:
#        ofile.write('\n'.join(files_to_rerun))
#    print "INFO :: Files to rerun were written to %s" % os.path.relpath(ofile_name, os.getcwd())
#    print rerun_cmd
#else:
#    print "INFO :: All files passed :)"

