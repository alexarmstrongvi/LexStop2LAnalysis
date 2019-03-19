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
condor_user = 'alarmstr'

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('-i','--ignore-id', nargs='*', help="Process IDs to ignore")
parser.add_argument('-d','--directory', help="Directory with all condor output files", default='%s/run/batch/condor_output' % (work_dir))
parser.add_argument('--dumb-run', action='store_true', help="Resubmit all files without pass phrase, no other checks")
args = parser.parse_args()

exclude_proc = map(int, args.ignore_id) if args.ignore_id else []
log_file_dir = args.directory

if superflow:
    print "INFO :: Setting up to check Superflow output"
    pass_phrase = "SuperflowAnaStop2L    Done." # For SuperflowAnaStop2L
elif sumw:
    print "INFO :: Setting up to check SumW output"
    pass_phrase = "Sumw job done" # For grabSumw

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
def print_jobsite_ranking(log_files, max_sites = 0):
    if not log_files: 
        print "INFO :: No log files to parse"
        return
    file_str = " ".join(log_files)
    bash_cmd = 'fgrep -h JOB_Site %s | fgrep -v GLIDEIN_Site:Unknown | sort | uniq -c | sort -rn' % file_str
    if max_sites > 0:
       bash_cmd += " | head -n " + str(max_sites)
    subprocess.call(bash_cmd, shell=True)

root_files = set(x.replace('.root','') for x in glob.glob(root_glob_cmd))
all_files = set(x.replace('.log','') for x in glob.glob(log_glob_cmd))
all_files2 = set(x.replace('.out','') for x in glob.glob(out_glob_cmd))
all_files3 = set(x.replace('.err','') for x in glob.glob(err_glob_cmd))
if len(all_files) > len(all_files2):
    print "WARNING :: There is/are %d missing .out file(s)" % (len(all_files) - len(all_files2))
    print "INFO :: Treating these as if they were empty .out files"
add_to_empty = all_files - all_files2
all_files2 = all_files2 | all_files
    #print '\n', "="*80
    #print "INFO :: There is/are %d missing .out file(s)" % (len(all_files) - len(all_files2))
    #if (len(all_files) - len(all_files2)) < 5:
    #    for f in (all_files-all_files2):
    #        print "\t%s.out" % f
    #
    #usr_msg = "Should empty .out files be created to restore balance to the force?\n"
    #usr_msg += "Input your answer [Y/N]: "
    #user_op = raw_input(usr_msg)

    #while user_op not in ["Y","N"]:
    #    usr_msg = "Unacceptable answer: %s\n" % user_op
    #    usr_msg += "Input your answer [Y/N]: "
    #    user_op = raw_input(usr_msg)

    #if user_op == "Y":
    #    for f in (all_files-all_files2):
    #        bash_cmd = 'touch %s.out' % f
    #        subprocess.call(bash_cmd, shell=True)
    #    all_files2 = set(x.replace('.out','') for x in glob.glob(out_glob_cmd))
    #if user_op == "N":
    #    print "\nINFO :: Oh. Fine then. Have it your way :(\n"


assert len(all_files) == len(all_files2) == len(all_files3), (
    "ERROR :: .log, .out, and .err files differ: %d, %d, and %d respectively." % (len(all_files), len(all_files2), len(all_files3)),
    all_files - all_files3
)

print "INFO :: Processing %d batch job output files" % len(all_files)

print '\n', "="*80
print "INFO :: Looking for completed files"
bash_cmd = "grep -l \"%s\" %s" % (pass_phrase, out_glob_cmd)
print "INFO :: >>", bash_cmd, '\n' 
complete_files = get_cmd_output(bash_cmd)
complete_files = set(f.strip().replace(".out","") for f in complete_files)

if args.dumb_run:
    print "WARNING :: DUMB RUN: Resubmit all files without successful run"
    resubmit_files = set(f for f in all_files if f not in complete_files)

else:
    print "INFO :: Looking for files of jobs that terminated normally"
    bash_cmd = "grep -l \"%s\" %s" % (finished_phrase, log_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    norm_term_files = get_cmd_output(bash_cmd)
    norm_term_files = set(f.split()[0].replace(".log","") for f in norm_term_files)

    # Extra checks
    import htcondor
    print "INFO :: Getting number of active jobs"
    coll = htcondor.Collector()
    schedd_ad = coll.locate(htcondor.DaemonTypes.Schedd)
    schedd = htcondor.Schedd(schedd_ad)
    req = 'Owner == "%s"' % condor_user
    for x in exclude_proc:
        req += ' && ClusterId != %d' % x
    job_iterlist = schedd.xquery(requirements = req, projection = ['Out'])
    active_ofiles = [x['Out'] for x in job_iterlist]
    active_files = set(os.path.basename(x).replace('.out','') for x in active_ofiles)
    
    bash_cmd = "condor_q | tail -n1 | grep -Po \"[0-9]*(?= jobs)\""
    print "INFO :: >>", bash_cmd, '\n'
    n_total_jobs = int(get_cmd_output(bash_cmd)[0])
    #bash_cmd = "condor_q | tail -n1 | grep -Po \"[0-9]*(?= removed)\""
    #print "INFO :: >>", bash_cmd, '\n' 
    #n_removed_jobs = int(get_cmd_output(bash_cmd)[0])


    print "INFO :: Looking for empty files"
    bash_cmd = "find %s -type f -empty" % (out_glob_cmd)
    print "INFO :: >>", bash_cmd, '\n' 
    empty_files = get_cmd_output(bash_cmd)
    empty_files = set(f.strip().replace(".out","") for f in empty_files)
    empty_files = empty_files | add_to_empty

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
    resubmit_files = failed_files | aborted_files
    finished_files = failed_files | aborted_files | complete_files

    # Checks
    assert len(active_files) == len(all_files - norm_term_files - aborted_files), (
        "Active Files: %d != %d" % (len(active_files), len(all_files - norm_term_files - aborted_files))
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
    assert len(aborted_files) + len(active_files) >= len(empty_files),( # active files might dump partially into .out
        "ERROR :: There are %d aborted and %d active file but %d empty files" % (len(aborted_files), len(active_files), len(empty_files)),
        empty_files - active_files - aborted_files
    )
    assert len(failed_files) + len(aborted_files) == len(resubmit_files) 
    assert len(complete_files) + len(failed_files) + len(aborted_files) == len(finished_files)
    if len(complete_files) != len(root_files):
        print '\n', "="*80
        print "ERROR :: %d complete files but only %d root files" % (len(complete_files), len(root_files))
        rf = set(extract_info(x) for x in root_files)
        cf = set(extract_info(x) for x in complete_files)
        ntf = set(extract_info(x) for x in norm_term_files)
        questionable_f = rf ^ cf
        if questionable_f:
            if len(rf - cf):
                print "ERROR :: %d jobs have root files but the .out file doesn't say they are complete" % (len(rf - cf))
                print "INFO :: These are the files\n\t", rf - cf
                
                usr_msg = "Should the root files be deleted and log files modified?\n"
                usr_msg += "Input your answer [Y/N]: "
                user_op = raw_input(usr_msg)

                while user_op not in ["Y","N"]:
                    usr_msg = "Unacceptable answer: %s\n" % user_op
                    usr_msg += "Input your answer [Y/N]: "
                    user_op = raw_input(usr_msg)

                if user_op == "Y":
                    del_rfiles = ["%s.root"%f for f in root_files if extract_info(f) in questionable_f]
                    fill_files = [f for f in all_files if extract_info(f) in questionable_f]
                    del_cmd = ''
                    for ii, f in enumerate(del_rfiles, 1):
                        del_cmd += "rm %s; " % f
                        if ii%100==0: #Don't let delete bash cmd get too long
                            subprocess.call(del_cmd, shell=True)
                            del_cmd = ''
                    subprocess.call(del_cmd, shell=True)
                    
                    fill_cmd = ''
                    for f in fill_files:
                        fill_cmd += 'echo \"%s (manually added)\" >> %s.log; ' % (finished_phrase, f)
                        fill_cmd += 'echo \" Job failed for unknown reasons\" >> %s.out; ' % (f)
                    subprocess.call(fill_cmd, shell=True)
                    print "\nINFO :: Done. Rerun script"
                    sys.exit()
                if user_op == "N":
                    print "\nINFO :: Okay. Exiting program"
                    sys.exit()
            elif len(cf - rf):
                print "ERROR :: %d jobs have no root files but the .out file does say they are complete" % (len(cf - rf))
                print "INFO :: These are the files\n\t",
            print '\n\t'.join(["%s.out" % f for f in all_files if extract_info(f) in questionable_f])
            print "INFO :: Job site information"
            print_jobsite_ranking(["%s.log" % f for f in all_files if extract_info(f) in questionable_f])
        if (rf - cf) & ntf:
            print "INFO :: Note that %d of those files have log files saying they finished normally" % (len((rf - cf) & ntf))
        

    assert len(complete_files) == len(root_files), (
        "ERROR :: %d complete files but %d root files exist" % (len(complete_files), len(root_files))
    )

    if len(active_files) or len(resubmit_files):
        # Error monitoring
        err_files = ["%s.err" % f for f in resubmit_files]
        if err_files:
            print '\n', "="*80
            print "INFO :: Summarizing top 10 errors occurrances for failed jobs"
            bash_cmd = 'fgrep -ih error %s | grep -v "tar:" | fgrep -v cling::AutoloadingVisitor::InsertIntoAutoloadingState | sort | uniq -c | sort -rn | head -n 10' % (" ".join(err_files))
            output = get_cmd_output(bash_cmd)
            for ii, line in enumerate(output, 1):
                err = line.strip().split(' ',1)[1]
                bash_cmd = 'fgrep -l \"%s\" %s' % (err, " ".join(err_files))
                files_with_err = get_cmd_output(bash_cmd)
                log_files = [f.strip().replace('.err','.log') for f in files_with_err]

                print "%2d) %s" % (ii, line.strip())
                if len(log_files) > 1000: 
                    print "\t%d/%d files have this error" % (len(log_files), len(all_files))
                    continue 
                print_jobsite_ranking(log_files, max_sites=2)

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
usr_msg = "Would you like to resubmit %d jobs?\n" % (len(resubmit_files))
usr_msg += "NOTE: This will clear the .log, .out, and .err files of jobs being resubmitted\n"
usr_msg += "Input your answer [Y/N]: "
user_op = raw_input(usr_msg)

while user_op not in ["Y","N"]:
    usr_msg = "Unacceptable answer: %s\n" % user_op
    usr_msg += "Would you like to resubmit jobs? [Y/N] "
    user_op = raw_input(usr_msg)

if user_op == "Y":
    print "INFO :: Clearing files for resubmission..."
    clear_cmd = ''
    for ii, f in enumerate(resubmit_files, 1):
        clear_cmd += "> %s.log;" % f
        clear_cmd += "> %s.out;" % f
        clear_cmd += "> %s.err;" % f
        if ii%10 == 0 or ii == len(resubmit_files):
            # cmd can get too large to run all at once at the end
            subprocess.call(clear_cmd, shell=True)
            clear_cmd = ''

    print "INFO :: Resubmitting files"
    directory = os.path.dirname(condor_resubmit_file)
    sub_file = os.path.basename(condor_resubmit_file)
    cmd = 'cd %s; condor_submit %s; cd -' % (directory, sub_file)
    subprocess.call(cmd, shell=True)
    print '\n', "="*80
else:
    print "We hope you enjoyed your experience. Come back soon. :)"
    print '\n', "="*80
