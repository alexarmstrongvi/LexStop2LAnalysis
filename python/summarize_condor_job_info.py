#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re
from datetime import datetime, date, timedelta
from collections import defaultdict, Counter

################################################################################
# Configuration
################################################################################
_work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
_input_files_dir = '%s/run/lists/file_lists_prefixed' % _work_dir
_pass_phrase = "SuperflowAnaStop2L    Done." # For SuperflowAnaStop2L
_condor_event_pattern = '"^[0-9]{3}( )"'
# Example time pattern: 01/15 16:13:06
_time_pattern = " [0-9][0-9]\/[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9] "
#_log_file_dir = '%s/run/batch/condor_output' % (_work_dir)
_log_file_dir = '%s/run/batch/output/test' % (_work_dir)
_ofile_name = "%s/condor_summary_ranking.txt" % _log_file_dir
_csv_ofile_name = "%s/condor_summary.csv" % _log_file_dir 

################################################################################
# Globals 
################################################################################
EVENT_TYPES = {
    # See Condor Manual 2.6.7 Managing a Job: In the Job Event Log File
    # http://research.cs.wisc.edu/htcondor/manual/v8.6/2_6Managing_Job.html
    '000' : 'Job submitted',
    '001' : 'Job executing',
    '002' : 'Error in executable',
    '003' : 'Job was checkpointed',
    '004' : 'Job evicted from machine',
    '005' : 'Job terminated',
    '006' : 'Image size of job updated',
    '007' : 'Shadow exception',
    '008' : 'Generic log event',
    '009' : 'Job aborted',
    '010' : 'Job was suspended',
    '011' : 'Job was unsuspended',
    '012' : 'Job was held',
    '013' : 'Job was released',
    '014' : 'Parallel node executed',
    '015' : 'Parallel node terminated',
    '016' : 'POST script terminated',
    '017' : 'Job submitted to Globus',
    '018' : 'Globus submit failed',
    '019' : 'Globus resource up',
    '020' : 'Detected Down Globus Resource',
    '021' : 'Remote error',
    '022' : 'Remote system call socket lost',  # Job disconnected
    '023' : 'Remote system call socket reestablished',  # Job reconnected
    '024' : 'Remote system call reconnect failure',  # Job reconnection failed
    '025' : 'Grid Resource Back Up',
    '026' : 'Detected Down Grid Resource',
    '027' : 'Job submitted to grid resource',
    '028' : 'Job ad information event triggered',
    '029' : 'The job\'s remote status is unknown',
    '030' : 'The job\'s remote status is known again',
    '031' : 'Job stage in',
    '032' : 'Job stage out',
    '033' : 'Job ClassAd attribute update',
    '034' : 'Pre Skip event'
}

# These events tend to be updates without really indicating a change of state
# or they indicate a return to some other state that we care about
IGNORE_EVENTS = ['006','028']

# These events are synonymous
def REMAPPED_TYPES(evt_num):
    return {
            'Remote system call socket reestablished' : 'Job executing',
    }.get(evt_num, evt_num)

# Determined empirically. Unknown states encountered during run time are printed
# at the end and should be added here.
KNOWN_TRANSITIONS = [
    'Job submitted -> Job executing',
    'Job submitted -> Remote system call socket lost',
    'Job submitted -> Shadow exception',
    #
    'Job executing -> Remote system call socket lost',
    'Job executing -> Job evicted from machine',
    'Job executing -> Job terminated',
    'Job executing -> Shadow exception',
    'Remote system call socket lost -> Remote system call socket reestablished',
    'Remote system call socket lost -> Remote system call reconnect failure',
    'Shadow exception -> Job executing',
    #
    'Remote system call socket reestablished -> Job terminated',
    'Remote system call socket reestablished -> Remote system call socket lost',
    'Remote system call socket reestablished -> Shadow exception',
    'Job evicted from machine -> Job executing',
    'Remote system call reconnect failure -> Remote system call socket lost',
    'Remote system call reconnect failure -> Job executing',
    'Remote system call socket reestablished -> Job evicted from machine',
    'Remote system call reconnect failure -> Shadow exception',
    #
    'Shadow exception -> Shadow exception',
]
KNOWN_STATES = set()
for x in KNOWN_TRANSITIONS:
    x1, x2 = x.split(' -> ')
    KNOWN_STATES.add(x1) 
    KNOWN_STATES.add(x2) 

################################################################################
# Main Function 
################################################################################
def main():
    log_glob_cmd = "%s/*log" % os.path.relpath(_log_file_dir, os.getcwd())
    out_glob_cmd = "%s/*out" % os.path.relpath(_log_file_dir, os.getcwd())
    input_glob_cmd = '%s/*/*' % _input_files_dir
    
    log_files = sorted(glob.glob(log_glob_cmd))
    out_files = sorted(glob.glob(out_glob_cmd))
    input_files = sorted(glob.glob(input_glob_cmd))

    durations = {}
    looper_info = defaultdict(dict)
    memory_usage = {}
    assert len(out_files) == len(log_files)
    print "INFO :: Looping over %d output files" % len(out_files)
    n_skipped = 0
    transitions = Counter()
    for ii, (out_file, log_file) in enumerate(zip(out_files, log_files)):
        if ii%100 == 0:
            print "INFO :: Processing %d of %d" % (ii, len(out_files))
        if _pass_phrase not in open(out_file, 'r').read():
            n_skipped += 1
            continue
        
        file_name = parse_file_name_from_log_name(log_file)
        #print "TESTING :: [%d/%d] Processing file: %s" % (ii, len(out_files), log_file)
        
        event_st = get_event_statements(log_file)
        #for t in event_st:
        #    print t
        
        times = extract_times(event_st)
        #for t, v in times:
        #    print t, ":", v
        
        tr = summarize_transitions(times)
        transitions.update(tr)
        
        durations[file_name] = determine_durations(times)
        #for k, v in durations[file_name].items():
        #    print k, " -> ", v
        
        nevts, rate, time = extract_looper_info(out_file)
        looper_info[file_name]['events'] = nevts
        looper_info[file_name]['rates'] = rate
        looper_info[file_name]['times'] = time

        memory_usage[file_name] = determine_mem_usage(log_file)
        #print memory_usage[file_name]
    if n_skipped:
        print "INFO :: Processed all but %d files" % n_skipped
    else:
        print "INFO :: Processed all files :)"

    print "Looking for unseen transitions..."
    for f in transitions.most_common(): 
        if f[0] in KNOWN_TRANSITIONS: continue
        print f
    print_summary_info(durations, looper_info)
    write_out_rankings(durations, looper_info, memory_usage)
    write_out_csv(durations, looper_info, memory_usage)
 
################################################################################
# Format sensative functions
################################################################################
def parse_file_name_from_log_name(log_file_name):
    # Example input: path/to_file/log_group.phys-susy.data15_13TeV.00276329.physics_Main.SusyNt.p3637_n0306b_13.log
    # Desired output: group.phys-susy.data15_13TeV.00276329.physics_Main.SusyNt.p3637_n0306b_13
    return os.path.basename(log_file_name).replace("log_","").replace(".log","")

def event_line_to_datetime(time_str):
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

def extract_looper_info(out_file):
    time = rate = events = 0
    with open(out_file) as f:
        for line in f:
            l = line.strip()
            pattern = 'Number of events processed:'
            if pattern in l:
                start_idx = l.find(pattern)+len(pattern) + 1
                events = int(l[start_idx:])
            
            pattern = 'Analysis speed [kHz]:'
            if pattern in l:
                start_idx = l.find(pattern)+len(pattern) + 1
                rate = float(l[start_idx:])*1000 #kHz -> Hz
            
            pattern = 'Analysis time: Real'
            end_pattern = ', CPU'
            if pattern in l:
                start_idx = l.find(pattern)+len(pattern) + 1
                end_idx = l.find(end_pattern)
                time = l[start_idx:end_idx]
    time = time.split(":")
    time = timedelta(hours=int(time[0]), minutes=int(time[1]), seconds=int(time[2]))
    return events, rate, time
        
################################################################################
# Supporting functions
################################################################################
def get_event_statements(log_file):
    bash_cmd = "egrep %s %s"% (_condor_event_pattern, log_file)
    event_statement = get_cmd_output(bash_cmd)
    return [s.strip() for s in event_statement if not any(s.startswith(x) for x in IGNORE_EVENTS)]

def extract_times(event_statements):
    '''
    args:
        time_stamps (list[str]) - list of lines from log file with important time stamps
    returns:
        (list[tuple(str, datetime)] - lists of tuples with the event number and time 
    '''
    times = [] 
    for s in event_statements:
        # Ex: s = '004 (856768.001.000) 01/15 16:26:21 Job was evicted.' 
        evt_num = s[0:3]
        ntup = (evt_num, event_line_to_datetime(s))
        times.append(ntup)
    return times

def summarize_transitions(times):
    transitions = Counter()
    for idx in range(1,len(times)):
        evt1 = EVENT_TYPES[times[idx-1][0]]
        evt2 = EVENT_TYPES[times[idx][0]]
        relation = "%s -> %s" % (evt1, evt2)
        if relation not in KNOWN_TRANSITIONS:
            print "DEBUG :: Unknown relation:", relation
        transitions.update({relation:1})
    return transitions


    
def determine_durations(times):
    durations = {}
    durations['total'] = times[-1][1] - times[0][1]
    
    for s in KNOWN_STATES:
        if s == 'Job terminated': continue
        s = REMAPPED_TYPES(s)
        durations[s] = timedelta()

    for idx in range(1, len(times)):
        prev_n, prev_t = times[idx-1]
        now_n, now_t = times[idx]
        key = EVENT_TYPES[prev_n]
        key = REMAPPED_TYPES(key)

        if key not in durations:
            durations[key] = timedelta()
        durations[key] += (now_t - prev_t)
    
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

def print_summary_info(durations, looper_info):
    def print_info(header, times, time=True):
        if time:
            avg_dur = sum(times, timedelta())/len(times)
        else:
            avg_dur = sum(times)/len(times)
        max_dur = max(times)
        min_dur = min(times) 
        print header
        print "\t Avg:", avg_dur
        print "\t Min:", min_dur
        print "\t Max:", max_dur
    # Exec time
    looper_times = [t['times'] for k, t in looper_info.iteritems()]
    print_info("Looper Time", looper_times)
    looper_rates = [t['rates'] for k, t in looper_info.iteritems()]
    print_info("Looper Rate", looper_rates, time=False)
    looper_events = [t['events'] for k, t in looper_info.iteritems()]
    print_info("Looper Events", looper_events, time=False)

    inv_dict = {}
    for f, d in durations.iteritems():
        for k, t in d.iteritems():
            if k not in inv_dict:
                inv_dict[k] = []
            inv_dict[k].append(t)
    for k, l in inv_dict.iteritems():
        print_info(k,l)

def write_out_rankings(durations, looper_info, memory_usage):
    def make_rank_str(header, dic, reverse=True, nresults_to_plot=30):
        dic_gen = sorted(dic.iteritems(), key=lambda (k,v) : (v,k), reverse=reverse)
        out_str = "Ranking for %s\n" % header
        for ii, (k, v) in enumerate(dic_gen):
            if ii > nresults_to_plot: break
            out_str += '%4d) %10s : %s\n' % (ii, v, k)
        out_str += '\n'
        return out_str

    def simplify_dict(dic, key):
        return {k : v[key] for (k, v) in dic.iteritems()}

    out_str = ''
    total = simplify_dict(durations,'total')
    out_str += make_rank_str("Total Time", total)

    skip_states = ['Job terminated']
    for s in KNOWN_STATES:
        s = REMAPPED_TYPES(s)
        if s in skip_states: continue
        else: skip_states.append(s)
        d = simplify_dict(durations, s)
        out_str += make_rank_str(s, d)

    looper_rates = simplify_dict(looper_info, 'rates')
    out_str += make_rank_str("Looper Rate [Events/s]", looper_rates, reverse=False)
    
    looper_times = simplify_dict(looper_info, 'times')
    out_str += make_rank_str("Looper Time", looper_times)
    
    looper_events = simplify_dict(looper_info, 'events')
    out_str += make_rank_str("Looper Events", looper_events)
    
    disk_usage = simplify_dict(memory_usage, 'disk_usage')
    out_str += make_rank_str("Disk Usage [MB]", disk_usage)
    
    disk_allocated = simplify_dict(memory_usage, 'disk_allocated')
    out_str += make_rank_str("Disk Allocated [MB]", disk_allocated)
    
    mem_usage = simplify_dict(memory_usage, 'memory_usage')
    out_str += make_rank_str("Memory Usage [MB]", mem_usage)
    
    memory_allocated = simplify_dict(memory_usage, 'memory_allocated')
    out_str += make_rank_str("Memory Allocated [MB]", memory_allocated)
    
    with open(_ofile_name, 'w') as ofile:
        ofile.write(out_str)
    print "INFO :: Ranked info written to", os.path.relpath(_ofile_name, os.getcwd())

def write_out_csv(durations, looper_info, memory_usage):
    my_dict = {}
    assert len(durations) == len(looper_info) == len(memory_usage)
    headers = []
    for f in durations:
        tmp_dict = {}
        tmp_dict.update(memory_usage[f])
        tmp_dict.update(looper_info[f])
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
