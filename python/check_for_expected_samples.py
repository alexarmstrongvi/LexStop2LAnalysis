#!/bin/bash/env python

from pyTools import get_cmd_output
import glob
import os
import re
import sys
import subprocess
from analysis_DSIDs import DSID_GROUPS

################################################################################
# Configuration
################################################################################
work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
input_files_dir = '%s/run/lists/file_lists_prefixed' % work_dir
log_file_dir = '%s/run/batch/condor_output' % (work_dir)

def is_match(campaign, group):
    if 'data' in group and campaign in group:
        return True
    elif 'data' not in group and 'mc16' in campaign:
        return True
    else:
        return False
################################################################################
# Where the magic happens
################################################################################
root_glob_cmd = "%s/*root" % os.path.relpath(log_file_dir, os.getcwd())
file_list = glob.glob(root_glob_cmd)

print "INFO :: Found %d root files" % len(file_list)
def extract_info(name):
    dsid = re.search("[1-9][0-9]{5}", name).group()
    campaign = re.search('mc16[ade]|(?<=_20)1[5678]|(?<=data)1[5678]', name).group()
    match = re.search('(?<=_)[0-9]*$', name.replace(dsid,"X"))
    suffix = match.group() if match else ""
    return dsid, campaign, suffix

from collections import defaultdict
root_files = defaultdict(set)
for f in file_list:
    dsid, campaign, _ = extract_info(os.path.basename(f))
    root_files[campaign].add(dsid)

print "INFO :: Looking for missing root files..."
missing_files = defaultdict(list)
for campaign, lst in root_files.iteritems():
    for group, dsid_lst in DSID_GROUPS.iteritems():
        if not is_match(campaign, group): continue
        for dsid in dsid_lst:
            if dsid not in lst:
                key = "%20s : %s" % (group, dsid)
                missing_files[key].append(campaign)
for k in sorted(missing_files.keys()):
    campaigns = missing_files[k]
    cs = "All" if len(campaigns) == 3 else ', '.join(sorted(campaigns)) 
    print "%s : %s" % (k, cs)
