#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re
import sys
import subprocess

################################################################################
# Configuration
################################################################################
work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
input_files_dir = '%s/run/lists/file_lists_prefixed' % work_dir
superflow = True; sumw = False
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
abort_phrase = 'via condor_rm'

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
def extract_info(name):
    dsid = re.search("[1-9][0-9]{5}", name).group()
    campaign = re.search('mc16[ade]|(?<=_20)1[5678]|(?<=data)1[5678]', name).group()
    match = re.search('(?<=_)[0-9]*$', name.replace(dsid,"X"))
    suffix = match.group() if match else ""
    return dsid, campaign, suffix

root_files = set(x.replace('.root','') for x in glob.glob(root_glob_cmd))
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
    bash_cmd = "condor_q | tail -n1 | grep -Po \"[0-9]*(?= jobs)\""
    print "INFO :: >>", bash_cmd
    n_total_jobs = int(get_cmd_output(bash_cmd)[0])
    bash_cmd = "condor_q | tail -n1 | grep -Po \"[0-9]*(?= removed)\""
    print "INFO :: >>", bash_cmd, '\n' 
    n_removed_jobs = int(get_cmd_output(bash_cmd)[0])
    n_active_files = n_total_jobs - n_removed_jobs


    print "INFO :: Looking for empty files"
    bash_cmd = "find %s -type f -empty" % (out_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    empty_files = get_cmd_output(bash_cmd)
    empty_files = set(f.strip().replace(".out","") for f in empty_files)

    print "INFO :: Looking for aborted files"
    bash_cmd = 'grep -l "%s" %s' % (abort_phrase, log_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    aborted_files = get_cmd_output(bash_cmd)
    aborted_files = set(f.strip().replace(".log","") for f in aborted_files)

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
    
    failed_files = norm_term_files - complete_files
    active_files = all_files - norm_term_files - aborted_files
    resubmit_files = failed_files | aborted_files
    finished_files = failed_files | aborted_files | complete_files

    # Checks
    if len(complete_files) != len(root_files):
        rf = set(extract_info(x) for x in root_files)
        cf = set(extract_info(x) for x in complete_files)
        print "TESTING"

        for x in rf - cf:
            print x
    assert len(complete_files) == len(root_files), (
        "ERROR :: %d complete files but %d root files exist" % (len(complete_files), len(root_files))
    )
    assert not (failed_files & aborted_files), (
        "Failed and aborted files:", failed_files & aborted_files
    )
    assert not (failed_files & complete_files), (
        "Failed and complete files:", failed_files & complete_files
    )
    assert len(active_files) + len(finished_files) == len(all_files), (
        "ERROR :: %d active files + %d finished != %d total files" % (len(active_files), len(finished_files), len(all_files)),
        all_files - active_files - finished_files 
    )
    assert n_active_files == len(active_files), (
        "ERROR :: condor_q gives %d active files but %d do not have \"Normal Termination\" in them" % (n_active_files, len(active_files))
    )
    assert len(aborted_files) + len(active_files) >= len(empty_files) # active files might dump partially into .out
    assert len(failed_files) + len(aborted_files) == len(resubmit_files) 
    assert len(complete_files) + len(failed_files) + len(aborted_files) == len(finished_files)

    if len(active_files) or len(resubmit_files):
        # Error monitoring
        err_files = ["%s.err" % f for f in resubmit_files]
        if err_files:
            print '\n', "="*80
            print "INFO :: Summarizing errors occurrances for failed jobs"
            bash_cmd = 'grep -ih error %s | grep -v "tar:" | sort | uniq -c | sort -rn' % (" ".join(err_files))

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

matched_resub = set()
with open(condor_resubmit_file, 'w') as ofile:
    ofile_str = ''
    resub_files = set(os.path.basename(f) for f in resubmit_files)
    with open(condor_submit_file, 'r') as ifile:
        gathering_job_cmd = False
        tmp_job_queue_cmd = ''
        # At first add all lines from header
        # Once the first job queue cmd is found, collect the full cmd
        # without adding it the resubmit file. Check if is supposed to 
        # be resubmitted and, if so, add it the resubmit file
        for line in ifile:
            if not line.strip(): continue
            if start_of_job_queue_line(line):
                gathering_job_cmd = True

            if gathering_job_cmd:
                tmp_job_queue_cmd += line
            else:
                ofile_str += line
            
            if gathering_job_cmd and end_of_job_queue_line(line):
                #if any("%s.log" % f in tmp_job_queue_cmd for f in resub_files):
                for f in resub_files:
                    if not "%s.log" % f in tmp_job_queue_cmd: continue
                    ofile_str += '\n' + tmp_job_queue_cmd
                    matched_resub.add(f)
                tmp_job_queue_cmd = ''
    ofile.write(ofile_str)

if resub_files - matched_resub:
    print '\n', "="*80
    print "WARNING :: The following jobs for resubmission were not found ",
    print "in the original condor submission script:"
    print resub_files - matched_resub

print '\n', "="*80
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
    print '\n', "="*80
else:
    print "We hope you enjoyed your experience. Come back soon. :)"
    print '\n', "="*80
