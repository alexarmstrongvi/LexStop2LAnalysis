#!/bin/bash/env python
"""
Program make_dsid_list.py
Author: Alex Armstrong <alarmstr@cern.ch>
Copyright: (C) Jan 6th, 2018; University of California, Irvine
"""
import sys
import os
from argparse import ArgumentParser
import analysis_DSIDs

def main():
    """ Main function """
    print "\nMaking DSID list..."

    parser = ArgumentParser()
    parser.add_argument('-o', '--output',
                        default='.',
                        help='output dir')
    parser.add_argument('--all',
                        action='store_true',
                        help='output all DSIDs in analysis_DSIDs')
    parser.add_argument('--data',
                        action='store_true',
                        help='output only data DSIDs')
    parser.add_argument('--mc',
                        action='store_true',
                        help='output only MC DSIDs')
    args = parser.parse_args()
    output = args.output

    # Checks
    if sum([args.all, args.data, args.mc]) > 1:
        print 'ERROR :: More than one DSID type specified.'\
              'Specify at most one of data, mc, and all.'
        sys.exit()

    groups_list = []
    if args.data or args.all:
        groups_list.append(('data15', analysis_DSIDs.get_data_groups(year=15)))
        groups_list.append(('data16', analysis_DSIDs.get_data_groups(year=16)))
        groups_list.append(('data17', analysis_DSIDs.get_data_groups(year=17)))
        groups_list.append(('data18', analysis_DSIDs.get_data_groups(year=18)))
    
    if args.mc or args.all:
        groups_list.append(('mc16',analysis_DSIDs.get_mc_groups()))

    # Write to output
    for label, groups in groups_list:
        ofile_name = "%s/desired_%s_DSIDs.txt" % (args.output, label)
        with open(ofile_name,'w') as ofile:
            written_dsids = set()
            for group in sorted(groups, key=lambda k:len(groups[k])):
                lst = groups[group]
                if set(lst) <= written_dsids: continue # Don't write groups that are subsets of others
                ofile.write('===== %s (%d) ===== \n'%(group, len(lst)))
                for dsid in lst:
                    ofile.write('%s\n'%dsid)
                    written_dsids.add(dsid)
                ofile.write('\n')
        print 'Output saved at %s' % ofile_name

if __name__ == '__main__':
    # Do not execute main() when script is imported as a module
    main()
