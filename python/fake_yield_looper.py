#!/usr/bin/env python
import sys, os, traceback, argparse
import time
import importlib
from PlotTools.YieldTable import YieldTbl, UncFloat
UncFloat.precision = 2
from math import sqrt
from copy import deepcopy

import pdb #TESTING

# Root data analysis framework
import ROOT as r
r.PyConfig.IgnoreCommandLineOptions = True # don't let root steal cmd-line options
r.gROOT.SetBatch(True)
r.gStyle.SetOptStat(False)

# Turn off root ownership when creating classes
r.TEventList.__init__._creates = False
r.TH1F.__init__._creates = False
r.TGraphErrors.__init__._creates = False
r.TGraphAsymmErrors.__init__._creates = False
r.TLegend.__init__._creates = False
r.THStack.__init__._creates = False

def main():
    global args
    lep1fake = '(lep1TruthClass < 1 || 2 < lep1TruthClass)'
    lep2fake = '(lep2TruthClass < 1 || 2 < lep2TruthClass)'
    lep1prompt = '(lep1TruthClass == 1 || lep1TruthClass == 2)'
    lep2prompt = '(lep2TruthClass == 1 || lep2TruthClass == 2)'
    fake_event = '(%s || %s)' % (lep1fake, lep2fake)
    sglfake_event = '(%s != %s)' % (lep1fake, lep2fake)
    leadfake_event = '(%s && %s)' % (lep1fake, lep2prompt)
    sleadfake_event = '(%s && %s)' % (lep1prompt, lep2fake)
    dblfake_event = '(%s && %s)' % (lep1fake, lep2fake)
    fake_sels = [
        ('TotalYld' , ''),
        ('FakeEvents' , fake_event),
        ('LeadingFakeLep' , leadfake_event),
        ('SubleadingFakeLep' , sleadfake_event),
        ('DoubleFakeLep' , dblfake_event),
    ]
    for reg in REGIONS:
        print '\n', 20*'-', "Yields for %s region"%reg.displayname, 20*'-', '\n'
        final_print_str = ''
        for name, truth_cut in fake_sels:

            ########################################################################
            print "Setting EventLists for %s + %s"% (reg.name, name)
            total_yld = UncFloat()
            sample_yld = {}
            faketype1_yld = {}
            faketype2_yld = {}
            for sample in SAMPLES :
                if not sample.isMC: continue
                weight_var = sample.weight_str

                scale_factor = sample.scale_factor if sample.isMC else 1
                list_name = "list_" + reg.name + "_" + sample.name
                cut = reg.tcut + " && " + truth_cut if truth_cut else reg.tcut
                sample.set_event_list(cut, list_name, EVENT_LIST_DIR)
                yld, error = get_yield_and_error(sample.tree, weight_var, scale_factor)
                
                result = UncFloat(yld, error)
                total_yld += result
                sample_yld[sample.name] = result

                h_truth1 = None
                h_truth2 = None
                if name == 'DoubleFakeLep':
                    h_truth1 = get_truth_hist(sample.tree, weight_var, scale_factor, 'lep1TruthClass')
                    h_truth2 = get_truth_hist(sample.tree, weight_var, scale_factor, 'lep2TruthClass')
                elif name == 'LeadingFakeLep':
                    h_truth1 = get_truth_hist(sample.tree, weight_var, scale_factor, 'lep1TruthClass')
                elif name == 'SubleadingFakeLep':
                    h_truth2 = get_truth_hist(sample.tree, weight_var, scale_factor, 'lep2TruthClass')

                if h_truth1:
                    for ftype, yld in get_faketype_ylds(h_truth1).items():
                        if ftype not in faketype1_yld:
                            faketype1_yld[ftype] = UncFloat()
                        faketype1_yld[ftype] += yld
                if h_truth2:
                    for ftype, yld in get_faketype_ylds(h_truth2).items():
                        if ftype not in faketype2_yld:
                            faketype2_yld[ftype] = UncFloat()
                        faketype2_yld[ftype] += yld

                if h_truth1: h_truth1.Delete()
                if h_truth2: h_truth2.Delete()
                
                # Done looping over samples
            print_str = "Breakdown for %s\n" % name
            print_str += "Total Yield : %s\n" % str(total_yld)
            print_str += "\tFake processes: \n%s" % rank_ylds_str(sample_yld, tabs='\t')
            if faketype1_yld:
                print_str += "\tLeading lepton fake types: \n%s" % rank_ylds_str(faketype1_yld, tabs='\t')
            if faketype2_yld:
                print_str += "\tSubleading lepton fake types: \n%s" % rank_ylds_str(faketype2_yld, tabs='\t')

            final_print_str += print_str
        print final_print_str

def get_yield_and_error(ttree, weight_var="", scale=1, dummy_var="isMC"):
    error = r.Double(0.0)
    weight_str = "%s * %f" % (weight_var, scale) if weight_var else "1"
    h_tmp = r.TH1D('h_get_yields_tmp','',1,0,-1) #Beware of saturation errors when using TH1F instead of TH1D. Samples be getting huge these days
    draw_cmd = "%s >> %s" % (dummy_var, h_tmp.GetName())
    ttree.Draw(draw_cmd, weight_str)
    yld = h_tmp.IntegralAndError(0,-1, error)
    h_tmp.Delete()

    return yld, error
        
def get_truth_hist(ttree, weight_var="", scale=1, var='lep1TruthClass'):
    weight_str = "%s * %f" % (weight_var, scale) if weight_var else "1"
    h_name = 'h_%s_%s' % (ttree.GetName(), var)
    h_tmp = r.TH1D(h_name,'',16, -4.5, 11.5)
    draw_cmd = "%s >> %s" % (var, h_tmp.GetName())
    ttree.Draw(draw_cmd, weight_str)
    return h_tmp

def get_faketype_ylds(h_fake):
    truthClass_labels = [
        '',                   # Bin 1
        'no origin info',     # Bin 2
        'no mother info',     # Bin 3
        'no truth info',      # Bin 4
        'uncategorized',      # Bin 5
        'prompt El',          # Bin 6
        'prompt Mu',          # Bin 7
        'prompt Pho',         # Bin 8
        'prompt El from FSR', # Bin 9
        'hadron',             # Bin 10
        'Mu as e',            # Bin 11
        'HF tau',             # Bin 12
        'HF B',               # Bin 13
        'HF C',               # Bin 14
        'unknown pho conv',   # Bin 15
        ' '                   # Bin 16  
    ]
    fake_dict = {}
    for ibin, label in enumerate(truthClass_labels, 1):
        if not label.strip(): continue
        yld = h_fake.GetBinContent(ibin)
        err = h_fake.GetBinError(ibin)
        result = UncFloat(yld, err)
        fake_dict[label] = result
    return fake_dict

def rank_ylds_str(yld_dict, tabs='', min_yld_perc = 0.0):
    rank_str = ''
    total = sum(yld_dict.values())
    if total == UncFloat():
        return ''
    other = UncFloat()
    for k, yld in sorted(yld_dict.items(), key=lambda (k,v): v, reverse=True):
        perc = yld / total
        if perc.value > min_yld_perc:
            rank_str += '%s%20s (%-15s)[%-15s%%]\n' % (tabs, k, yld, perc * 100)
        else:
            other += yld
    if other > UncFloat():
        rank_str += '%s%20s (%-15s)[%-15s%%]; ' % (tabs, 'Remainder', other, (other/total) * 100)
    return rank_str
    

################################################################################
# SETUP FUNCTIONS
def check_args(args):
    """ Check the input arguments are as expected """
    configuration_file = os.path.normpath(args.plotConfig)
    if not os.path.exists(configuration_file):
        print "ERROR :: Cannot find config file:", configuration_file
        sys.exit()

def check_environment():
    """ Check if the shell environment is setup as expected """
    python_ver = sys.version_info[0] + 0.1*sys.version_info[1]
    if python_ver < 2.7:
        print "ERROR :: Running old version of python\n", sys.version
        sys.exit()

def print_inputs(args):
    """ Print the program inputs """
    full_path = os.path.abspath(__file__)
    prog_name = os.path.basename(full_path)
    prog_dir = os.path.dirname(full_path)

    print " ==================================================================\n"
    print " Program : %s "%prog_name
    print " Run from: %s "%prog_dir
    print ""
    print " Options:"
    print "     plot config      :  %s "%args.plotConfig
    print "     suffix           :  %s "%args.suffix
    print "     output directory :  %s "%args.outdir
    print "     verbose          :  %s "%args.verbose
    print ""
    print "===================================================================\n"

    # print out the loaded samples and plots
    print " ============================"
    if SAMPLES :
        print "Loaded samples:    "
        for sample in SAMPLES :
            print '\t',
            sample.Print()
    print " ============================"

################################################################################
# Run main when not imported
if __name__ == '__main__':
    try:
        start_time = time.time()
        parser = argparse.ArgumentParser(
                description=__doc__,
                formatter_class=argparse.RawDescriptionHelpFormatter)
        parser.add_argument("-c", "--plotConfig",
                                default="",
                                help='name of the config file')
        parser.add_argument("-s", "--suffix",
                                default="",
                                help='Suffix to append to output yield tables')
        parser.add_argument("-o", "--outdir",
                                default="./",
                                help='name of the output directory to save plots.')
        parser.add_argument("-v", "--verbose",
                                action="store_true",
                                help='set verbosity mode')
        args = parser.parse_args()

        if args.verbose:
            print '>'*40
            print 'Running {}...'.format(os.path.basename(__file__))
            print time.asctime()

        check_args(args)
        check_environment()

        # Import configuration file
        import_conf = args.plotConfig.replace(".py","")
        conf = importlib.import_module(import_conf)

        SAMPLES = conf.SAMPLES
        REGIONS = conf.REGIONS
        EVENT_LIST_DIR = conf.EVENT_LIST_DIR
        YIELD_TBL_DIR = conf.YIELD_TBL_DIR
        YLD_TABLE = conf.YLD_TABLE

        print_inputs(args)

        main()

        if args.verbose:
            print time.asctime()
            time = (time.time() - start_time)
            print 'TOTAL TIME: %fs'%time,
            print ''
            print '<'*40
    except KeyboardInterrupt, e: # Ctrl-C
        print 'Program ended by keyboard interruption'
        raise e
    except SystemExit, e: # sys.exit()
        print 'Program ended by system exit'
        raise e
    except Exception, e:
        print 'ERROR, UNEXPECTED EXCEPTION'
        print str(e)
        traceback.print_exc()
        os._exit(1)

