#!/bin/usr/env python

import sys, os, traceback, argparse
import time
import importlib
import ROOT as r
r.PyConfig.IgnoreCommandLineOptions = True # don't let root steal cmd-line options
import atlasrootstyle.AtlasStyle
#r.SetAtlasStyle()
from copy import deepcopy
import PlotTools.plot_utils as pu

################################################################################
# Main plot looper
def main():
    import PlotTools.hist as Hist
    
    for reg in REGIONS:
        # Get plots for region
        plots_with_reg = [p for p in PLOTS if p.region == reg.name]
        if not len(plots_with_reg): continue
        print '\n', 20*'-', "Plots for %s region"%reg.name, 20*'-', '\n'

        ########################################################################
        print "Setting EventLists for %s"%reg.name 
        for sample in SAMPLES:
            list_name = "list_" + reg.name + "_" + sample.name
            sample.set_event_list(reg.tcut, list_name, EVENT_LIST_DIR)
        
        ########################################################################
        # Loop over each plot and save image
        n_plots = len(plots_with_reg)
        for ii, plot in enumerate(plots_with_reg, 1):
            print "[%d/%d] Plotting %s"%(ii, n_plots, plot.name), 40*'-'
            backgrounds = [s for s in SAMPLES if s.isMC and not s.isSignal]
            backgrounds += [s for s in SAMPLES if not s.isMC and s.isDataBkg]
            signals = [s for s in SAMPLES if s.isMC and s.isSignal]
            try:
                data = next(s for s in SAMPLES if not s.isMC and not s.isDataBkg)
            except StopIteration:
                data = None
                
            if args.plot_op == 'overlay':
                with Hist.SampleCompare1D(plot, reg, SAMPLES) as hists:
                    plot.make_overlay_plot(reg.displayname, hists)
            elif args.plot_op == 'mcstack':
                print "ERROR :: Not fully implemented"
                continue
                with Hist.StackHist1D(plot, reg, backgrounds) as main_hist:
                    plot.make_stack_plot(reg.displayname, main_hist)
            elif args.plot_op == 'datamcstack':
                with Hist.DataMCStackHist1D(plot, reg, data=data, bkgds=backgrounds, sigs=signals) as main_hist:
                    plot.make_data_mc_stack_plot(reg.displayname, main_hist)
            elif args.plot_op == 'datamcratio':
                assert data
                with Hist.DataMCStackHist1D(plot, reg, data=data, bkgds=backgrounds, sigs=signals) as main_hist:
                    with Hist.DataMCRatioHist1D(plot, reg, main_hist) as ratio_hist:
                        plot.make_data_mc_stack_with_ratio_plot(reg.displayname, main_hist, ratio_hist)
            elif args.plot_op == 'cutscan':        
                with Hist.CutScan1D(plot, reg, signals, backgrounds, plot.xcut_is_max) as main_hist:
                    plot.make_cutscan1d_plot(main_hist, reg.displayname)
            elif args.plot_op == 'overlayratio': 
                with Hist.SampleCompare1D(plot, reg, SAMPLES) as hists:
                    num = hists.hists[0]
                    den = hists.hists[1]
                    with Hist.RatioHist1D(plot, num, den, ymax = 2, ymin = 0) as ratio_hist:
                        ratio_label = "%s / %s" % (SAMPLES[0].displayname, SAMPLES[1].displayname)
                        plot.make_overlay_with_ratio_plot(reg.displayname, ratio_label, hists, ratio_hist)
            elif args.plot_op == 'simple_2d':
                for s in SAMPLES:
                    #TODO : Fix memory leaks from all hists being created with plot.name
                    with Hist.Hist2D(plot, reg, [s]) as main_hists:
                        #print "TESTING :: ", pu.print_hist(main_hists.hist)
                        plot.make_2d_hist(main_hists, s.name)
            elif args.plot_op == 'cutscan_2d': 
                with Hist.CutScan2D(plot, reg, signals, backgrounds, xcut_is_max=True, ycut_is_max=True, and_cuts=True, bkgd_rej=False) as hists:
                    plot.make_cutscan2d_plot(hists, reg.name)
            else:
                print "ERROR :: Unrecognized plot option:", args.plot_op


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
    if args.verbose:
        print "Loaded plots:"
        for plot in PLOTS :
            plot.Print()
    print " ============================"

def check_for_consistency() :
    '''
    Make sure that the plots are not asking for undefined region

    param:
        plots : list(plot class)
            plots defined in config file
        regions : list(region class)
            regions defined in config file
    '''
    region_names = [r.name for r in REGIONS]
    bad_regions = set([p.region for p in PLOTS if p.region not in region_names])
    if len(bad_regions) > 0 :
        print 'check_for_consistency ERROR    '\
        'You have configured a plot for a region that is not defined. '\
        'Here is the list of "bad regions":'
        print bad_regions
        print 'check_for_consistency ERROR    The regions that are defined in the configuration ("%s") are:'%args.plotConfig
        print region_names
        print "check_for_consistency ERROR    Exiting."
        sys.exit()

################################################################################
# Run main
if __name__ == "__main__":
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
                                help='Suffix to append to output plot names')
        parser.add_argument("-o", "--outdir",
                                default="./",
                                help='name of the output directory to save plots.')
        parser.add_argument("--plot-op",
                                default="overlay",
                                help="overlay, cutscan, datamcstack, mcstack, simple_2d, cutscan_2d")
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
        PLOTS = conf.PLOTS
        EVENT_LIST_DIR = conf.EVENT_LIST_DIR
        YLD_TABLE = conf.YLD_TABLE

        check_for_consistency()
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

