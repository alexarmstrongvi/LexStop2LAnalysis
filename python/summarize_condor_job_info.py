#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re
from datetime import datetime, date, timedelta

################################################################################
# Configuration
################################################################################
_work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
_input_files_dir = '%s/run/lists/file_lists_prefixed' % _work_dir
_pass_phrase = "SuperflowAnaStop2L    Done." # For SuperflowAnaStop2L
# Example time pattern: 01/15 16:13:06
_time_pattern = " [0-9][0-9]\/[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] "
_log_file_dir = '%s/run/batch/SuperflowAnaStop2l_output' % (_work_dir)
_ofile_name = "%s/condor_summary_ranking.txt" % _log_file_dir
_csv_ofile_name = "%s/condor_summary.csv" % _log_file_dir 

################################################################################
# Main Function 
################################################################################
def main():
    log_glob_cmd = "%s/*log" % os.path.relpath(_log_file_dir, os.getcwd())
    log_files = sorted(glob.glob(log_glob_cmd))
    out_glob_cmd = "%s/*out" % os.path.relpath(_log_file_dir, os.getcwd())
    out_files = sorted(glob.glob(out_glob_cmd))
    input_glob_cmd = '%s/*/*' % _input_files_dir
    input_files = sorted(glob.glob(input_glob_cmd))

    durations = {}
    looper_times = {}
    looper_rates = {}
    memory_usage = {}
    assert len(out_files) == len(log_files)
    print "INFO :: Looping over %d output files" % len(out_files)
    for ii, (out_file, log_file) in enumerate(zip(out_files, log_files)):
        if ii%100 == 0:
            print "INFO :: Processing %d of %d" % (ii, len(out_files))
        if _pass_phrase not in open(out_file, 'r').read(): continue
        
        file_name = parse_file_name_from_log_name(log_file)
        #print "TESTING :: Processing file:", log_file
        
        time_stamps = get_time_stamps(log_file)
        #for t in time_stamps:
        #    print t
        
        times = extract_times(time_stamps)
        #for t, v in times:
        #    print t, ":", v
        
        check_times(time_stamps, times)
        #print "All good"
        
        durations[file_name] = determine_durations(times)
        #for k, v in durations[file_name].items():
        #    print k, " -> ", v
        
        looper_times[file_name] = extract_looper_time(out_file)
        looper_rates[file_name] = extract_looper_rate(out_file)

        memory_usage[file_name] = determine_mem_usage(log_file)
        #print memory_usage[file_name]
    print "INFO :: Processed all files :)"

    print_summary_info(durations, looper_times, looper_rates)
    write_out_rankings(durations, looper_times, looper_rates, memory_usage)
    write_out_csv(durations, looper_times, looper_rates, memory_usage)
 
################################################################################
# Format sensative functions
################################################################################
def parse_file_name_from_log_name(log_file_name):
    # Example input: path/to_file/log_group.phys-susy.data15_13TeV.00276329.physics_Main.SusyNt.p3637_n0306b_13.log
    # Desired output: group.phys-susy.data15_13TeV.00276329.physics_Main.SusyNt.p3637_n0306b_13
    return os.path.basename(log_file_name).replace("log_","").replace(".log","")

def time_str_to_datetime(time_str):
    # Example time_str: 004 (856768.001.000) 01/15 16:26:21 Job was evicted.
    s = re.search(_time_pattern, time_str).group().strip()
    # s = '01/15 16:26:21'
    # idx: 01234567890123
    year = date.today().year
    month = int(s[0:2])
    day = int(s[3:5])
    hour = int(s[6:8])
    minute = int(s[9:11])
    second = int(s[12:14])
    return datetime(year, month, day, hour, minute, second)

def extract_looper_rate(out_file):
    pattern = 'Analysis speed [kHz]:'
    with open(out_file) as f:
        for line in f:
            l = line.strip()
            if pattern in l:
                start_idx = l.find(pattern)+len(pattern) + 1
                return float(l[start_idx:])*1000 #kHz -> Hz

def extract_looper_time(out_file):
    pattern = 'Analysis time: Real'
    end_pattern = ', CPU'
    time = 0
    with open(out_file) as f:
        for line in f:
            l = line.strip()
            if pattern in l:
                start_idx = l.find(pattern)+len(pattern) + 1
                end_idx = l.find(end_pattern)
                time = l[start_idx:end_idx]
                break
    time = time.split(":")
    return timedelta(hours=int(time[0]), minutes=int(time[1]), seconds=int(time[2]))

def is_submit_line(line):
    return 'Job submitted from host' in line
def is_execute_line(line):
    return 'Job executing on host' in line
def is_evict_line(line):
    return 'Job was evicted' in line
def is_abort_line(line):
    return 'Job was aborted by the user' in line
def is_terminate_line(line):
    return 'Job terminated' in line
def is_add_info_line(line):
    return 'Job ad information event triggered' in line
def is_update_size_line(line):
    return 'Image size of job updated:' in line

################################################################################
# Supporting functions
################################################################################
def ignore_time_line(line):
    return is_add_info_line(line) or is_update_size_line(line)

def get_time_stamps(log_file):
    bash_cmd = "grep -E \"%s\" %s"% (_time_pattern, log_file)
    time_stamps = get_cmd_output(bash_cmd)
    return [s.strip() for s in time_stamps if not ignore_time_line(s)]

def extract_times(time_stamps):
    '''
    args:
        time_stamps (list[str]) - list of lines from log file with important time stamps
    returns:
        (dict[str]=list[datetime]) - dictionary with lists of date-time objects keyed by condor operations (e.g. submit, execute, etc.)
    '''
    times = [] 
    for t in time_stamps:
        if is_submit_line(t): type_str = 'submit'
        elif is_execute_line(t): type_str = 'execute'
        elif is_evict_line(t): type_str = 'evict'
        elif is_abort_line(t): type_str = 'abort'
        elif is_terminate_line(t): type_str = 'terminate'
        ntup = (type_str, time_str_to_datetime(t))
        times.append(ntup)
    return times

def check_times(time_stamps, times):
    # Check ordering is as expected
    # Normal: Submit -> Execute -> Evict -> Abort -> Resubmit -> (repeat) -> Terminate
    # Other: Submit -> Execute -> Evict -> Re-execute -> (repeat) -> Terminate
    # Other: Submit -> Abort -> Resubmit -> (repeat) -> Execute -> ...
    # Other: Submit -> Execute -> Terminate -> Resubmit -> (repeat) -> Terminate
    prev_line = None
    for line in time_stamps:
        if is_submit_line(line):
            assert not prev_line or is_abort_line(prev_line) or is_terminate_line(prev_line) 
        elif is_execute_line(line):
            assert is_submit_line(prev_line) or is_evict_line(prev_line)
        elif is_evict_line(line):
            assert is_execute_line(prev_line)
        elif is_abort_line(line):
            assert is_evict_line(prev_line) or is_submit_line(prev_line)
        elif is_terminate_line(line):
            assert is_execute_line(prev_line)
        else:
            print "ERROR :: Unrecognized time stamp line:", line
        prev_line = line

    prev_time = times[0]
    for t in times[1:]:
        assert t[1] > prev_time[1], (prev_time, t)
        prev_time = t

def determine_durations(times):
    durations = {}
    durations['user'] = timedelta()
    durations['system_submit'] = timedelta() 
    durations['system_other'] = timedelta() 
    durations['total'] = times[-1][1] - times[0][1]

    prev_time = times[0]
    for t in times[1:]:
        if prev_time[0] == 'execute':
            durations['user'] += t[1] - prev_time[1]
        elif prev_time[0] == 'submit':
            durations['system_submit'] += t[1] - prev_time[1]
        else:
            durations['system_other'] += t[1] - prev_time[1]
        prev_time = t

    assert durations['system_submit'] + durations['user'] + durations['system_other'] == durations['total']
    return durations

def determine_mem_usage(log_file):
    bash_cmd = "grep \"Normal termination\" -A 12 %s | tail -n 2"% (log_file)
    mem_lines = get_cmd_output(bash_cmd)
    # Example results:
    # Disk (KB)            :   389513        1  40181230
    # Memory (MB)          :      293     1024      1024
    
    memory = {}
    memory['disk_usage'] = int(round(int(mem_lines[0].strip().split()[3])/1000.0)) # KB -> MB 
    memory['disk_allocated'] = int(round(int(mem_lines[0].strip().split()[5])/1000.0)) 
    memory['memory_usage'] = int(mem_lines[1].strip().split()[3])
    memory['memory_allocated'] = int(mem_lines[1].strip().split()[5]) 

    return memory

def print_summary_info(durations, looper_times, looper_rates):
    def print_info(header, times):
        avg_dur = sum(times, timedelta())/len(times)
        max_dur = max(times)
        min_dur = min(times) 
        print header
        print "\t Avg:", avg_dur
        print "\t Min:", min_dur
        print "\t Max:", max_dur
    # Exec time
    print_info("Looper Time", looper_times.values())
    total_times = [t['total'] for k, t in durations.iteritems()]
    print_info("Total Time", total_times)

def write_out_rankings(durations, looper_times, looper_rates, memory_usage):
    def make_rank_str(header, dic, reverse=True, nresults_to_plot=10):
        dic_gen = sorted(dic.iteritems(), key=lambda (k,v) : (v,k), reverse=reverse)
        out_str = "Ranking for %s\n" % header
        for ii, (k, v) in enumerate(dic_gen):
            if ii > nresults_to_plot: break
            out_str += '%4d) %10s : %s\n' % (ii, v, k)
        out_str += '\n'
        return out_str

    out_str = ''
    total = {k : v['total'] for (k, v) in durations.iteritems()}
    out_str += make_rank_str("Total Time", total)
    system_other = {k : v['system_other'] for (k, v) in durations.iteritems()}
    out_str += make_rank_str("System Other Time", system_other)
    system_submit = {k : v['system_submit'] for (k, v) in durations.iteritems()}
    out_str += make_rank_str("System Submit Time", system_submit)
    user = {k : v['user'] for (k, v) in durations.iteritems()}
    out_str += make_rank_str("User Time", user)
    out_str += make_rank_str("Looper Time", looper_times)
    out_str += make_rank_str("Looper Rate [Events/s]", looper_rates)
    disk_usage = {k : v['disk_usage'] for (k, v) in memory_usage.iteritems()}
    out_str += make_rank_str("Disk Usage [MB]", disk_usage)
    disk_allocated = {k : v['disk_allocated'] for (k, v) in memory_usage.iteritems()}
    out_str += make_rank_str("Disk Allocated [MB]", disk_allocated)
    mem_usage = {k : v['memory_usage'] for (k, v) in memory_usage.iteritems()}
    out_str += make_rank_str("Memory Usage [MB]", mem_usage)
    memory_allocated = {k : v['memory_allocated'] for (k, v) in memory_usage.iteritems()}
    out_str += make_rank_str("Memory Allocated [MB]", memory_allocated)
    
    with open(_ofile_name, 'w') as ofile:
        ofile.write(out_str)
    print "INFO :: Ranked info written to", os.path.relpath(_ofile_name, os.getcwd())

def write_out_csv(durations, looper_times, looper_rates, memory_usage):
    my_dict = {}
    assert len(durations) == len(looper_times) == len(looper_rates) == len(memory_usage)
    headers = []
    for f in durations:
        tmp_dict = {}
        tmp_dict.update(memory_usage[f])
        tmp_dict['looper_time'] = looper_times[f]
        tmp_dict['looper_rate'] = looper_rates[f]
        tmp_dict.update(durations[f])
        my_dict[f] = tmp_dict
    else:
        headers = tmp_dict.keys()

    import csv
    with open(_csv_ofile_name, 'w') as ofile:
        w = csv.DictWriter(ofile, ["Sample"] + headers)
        w.writeheader()
        for f, d in my_dict.iteritems():
            row = {"Sample" : f}
            for k, v in d.iteritems():
                if type(v) == timedelta:
                    value = round(v.total_seconds()/3600.0,3) # sec -> hrs
                else:
                    value = v
                row[k] = value
            w.writerow(row)
        print "INFO :: Summary table written to", os.path.relpath(_csv_ofile_name, os.getcwd())

   

if __name__ == '__main__':
    main()
