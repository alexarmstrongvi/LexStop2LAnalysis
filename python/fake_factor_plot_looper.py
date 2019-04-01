#!/usr/bin/env python
"""
================================================================================
Make fake factor files and plots
Examples
    make_fake_factor_plots.py -c config_file.conf

Description:
    Required inputs from config file
        - SAMPLES: two sets of sample objects with either "den" and "num" in the name
        - REGIONS: event selections to apply with truth selections defined
        - PLOTS: plots to make from samples
        - YIELD_TBL: default yield table
        - DEN_STR/NUM_STR: 

Author:
    Alex Armstrong <alarmstr@cern.ch>
Licence:
    Copyright: (C) <June 1st, 2018>; University of California, Irvine
================================================================================
"""

# General python
import sys, os, traceback, argparse
import time
from re import sub
from array import array
from copy import copy, deepcopy
from collections import defaultdict
from importlib import import_module
import subprocess
from contextlib import contextmanager
from math import sqrt


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

# Local classes for plotting
import PlotTools.plot_utils as pu
from PlotTools.YieldTable import UncFloat
from PlotTools.hist import make_plot1D_axis

@contextmanager
def open_root(f_name, f_mode):
    ofile = r.TFile(f_name, f_mode)
    try:
        yield ofile
    finally:
        ofile.Write()
        ofile.Close()

class KeyManager(object) :
    bkg_mc_str = 'MC_probe_lep_prompt'
    fake_mc_str = 'MC_probe_lep_fake'
    fake_mc_sys_scale_str = 'MC_probe_lep_fake_scaled'
    prompt_mc_sys_up_str = 'MC_probe_lep_prompt_shifted_up'
    prompt_mc_sys_dn_str = 'MC_probe_lep_prompt_shifted_dn'

    def __init__(self):
        self.mc_stack = None
        self.mc_stack_hists = {}
        self.mc_hist = None
        self.mc_truth_bkg_stack = None
        self.mc_truth_bkg_stack_hists = {}
        self.mc_truth_bkg_hist = None
        self.mc_truth_bkg_sys_up_hist = None
        self.mc_truth_bkg_sys_dn_hist = None
        self.mc_truth_fake_stack = None
        self.mc_truth_fake_stack_hists = {}
        self.mc_truth_fake_hist = None
        self.mc_truth_fake_sys_stack = None
        self.mc_truth_fake_sys_stack_hists = {}
        self.mc_truth_fake_sys_hist = None
        self.data_hist = None
        self.data_corr_hist = None
        self.data_comp_sys_hist = None
        self.data_prompt_sys_up_hist = None
        self.data_prompt_sys_dn_hist = None
        self.fake_factor_keys = {}

    ############################################################################
    @property
    def data_fake_factor(self):
        return self.fake_factor_keys[self.data_hist]

    @property
    def data_corr_fake_factor(self):
        return self.fake_factor_keys[self.data_corr_hist]

    @property
    def mc_fake_factor(self):
        return self.fake_factor_keys[self.mc_truth_fake_hist]

    @property
    def mc_comp_sys_fake_factor(self):
        return self.fake_factor_keys[self.mc_truth_fake_sys_hist]

    @property
    def data_comp_sys_fake_factor(self):
        return self.fake_factor_keys[self.data_comp_sys_hist]

    @property
    def data_prompt_sys_up_fake_factor(self):
        return self.fake_factor_keys[self.data_prompt_sys_up_hist]

    @property
    def data_prompt_sys_dn_fake_factor(self):
        return self.fake_factor_keys[self.data_prompt_sys_dn_hist]
    ############################################################################
    def generate_hist_key(self, sample, region, cut):
        sample_name = remove_num_den(sample.name)
        num_or_den = get_fake_channel(region.name)
        if region.truth_fake_sel in cut and get_comp_sys_weight(num_or_den) in cut:
            key = self.fake_mc_sys_scale_str + "_" + sample_name
            self.mc_truth_fake_sys_stack_hists[sample_name] = key
        elif region.truth_fake_sel in cut:
            key = self.fake_mc_str + "_" + sample_name
            self.mc_truth_fake_stack_hists[sample_name] = key
        elif region.truth_bkg_sel in cut:
            key = self.bkg_mc_str + "_" + sample_name
            self.mc_truth_bkg_stack_hists[sample_name] = key
        elif sample.isMC:
            key = sample_name
            self.mc_stack_hists[sample_name] = key
        else:
            key = sample_name
            self.data_hist = key
        return key

    def generate_stack_key(self, stack_hist_key):
        if self.is_fake_comp_sys_mc(stack_hist_key):
            key = "stack_%s"%self.fake_mc_sys_scale_str
            self.mc_truth_fake_sys_stack = key
        elif self.is_bkg_mc(stack_hist_key):
            key = "stack_%s"%self.bkg_mc_str
            self.mc_truth_bkg_stack = key
        elif self.is_fake_mc(stack_hist_key):
            key = "stack_%s"%self.fake_mc_str
            self.mc_truth_fake_stack = key
        else:
            key = "stack_MC"
            self.mc_stack = key
        return key

    def generate_mc_hist_key(self, hist_key):
        print "TESTING :: fake mc keys =", self.get_truth_fake_keys()
        if self.is_fake_comp_sys_mc(hist_key):
            key = "%s_hist"%self.fake_mc_sys_scale_str
            self.mc_truth_fake_sys_hist = key
        elif self.is_bkg_sys_up_mc(hist_key):
            key = "%s_hist"%self.prompt_mc_sys_up_str
            self.mc_bkgd_sys_up_hist = key
        elif self.is_bkg_sys_dn_mc(hist_key):
            key = "%s_hist"%self.prompt_mc_sys_dn_str
            self.mc_bkgd_sys_dn_hist = key
        elif self.is_bkg_mc(hist_key):
            key = "%s_hist"%self.bkg_mc_str
            self.mc_truth_bkg_hist = key
        elif self.is_fake_mc(hist_key):
            key = "%s_hist"%self.fake_mc_str
            self.mc_truth_fake_hist = key
        else:
            key = "mc_hist"
            self.mc_hist = key
        print "TESTING :: Key generated:", key
        return key

    def generate_data_corr_key(self):
        key = "%s_bkgd_subtracted"%self.data_hist
        self.data_corr_hist = key
        return key

    def generate_data_comp_sys_key(self):
        key = "%s_comp_sys"%self.data_hist
        self.data_comp_sys_hist = key
        return key

    def generate_prompt_sys_up_key(self, hist_key):
        key = "%s_prompt_sys_up"% hist_key
        if self.is_data(hist_key):
            self.data_prompt_sys_up_hist = key
        elif self.is_bkg_sys_up_mc(hist_key): 
            self.mc_prompt_sys_up_key = key
        return key

    def generate_prompt_sys_dn_key(self, hist_key):
        key = "%s_prompt_sys_dn"% hist_key
        if self.is_data(hist_key):
            self.data_prompt_sys_dn_hist = key
        elif self.is_bkg_sys_dn_mc(hist_key): 
            self.mc_prompt_sys_dn_key = key
        return key

    def generate_fake_factor_key(self, hist_key):
        key = "fake_factor_" + hist_key
        self.fake_factor_keys[hist_key] = key
        return key

    ############################################################################
    def get_data_keys(self):
        keys = [self.data_hist, self.data_corr_hist, self.data_comp_sys_hist, self.data_prompt_sys_up_hist, self.data_prompt_sys_dn_hist]
        keys = [k for k in keys if k]
        return keys

    def get_stack_keys(self):
        keys = [self.mc_stack, self.mc_truth_bkg_stack, self.mc_truth_fake_stack, self.mc_truth_fake_sys_stack]
        keys = [k for k in keys if k]
        return keys

    def get_total_mc_hist_keys(self):
        keys = [self.mc_hist, self.mc_truth_bkg_hist, self.mc_truth_fake_hist, self.mc_truth_fake_sys_hist]
        keys = [k for k in keys if k]
        return keys
    
    def get_raw_mc_keys(self):
        keys = self.mc_stack_hists.values()
        keys += [self.mc_stack, self.mc_hist]
        keys = [k for k in keys if k]
        return keys

    def get_truth_bkg_keys(self):
        keys = self.mc_truth_bkg_stack_hists.values()
        keys += [self.mc_truth_bkg_stack, self.mc_truth_bkg_hist]
        keys = [k for k in keys if k]
        return keys

    def get_truth_fake_keys(self):
        keys = self.mc_truth_fake_stack_hists.values()
        keys += [self.mc_truth_fake_stack, self.mc_truth_fake_hist]
        keys = [k for k in keys if k]
        return keys
    
    def get_truth_fake_comp_sys_keys(self):
        keys = self.mc_truth_fake_sys_stack_hists.values()
        keys += [self.mc_truth_fake_sys_stack, self.mc_truth_fake_sys_hist]
        keys = [k for k in keys if k]
        return keys

    def get_fake_factor_input_keys(self):
        keys = self.get_data_keys() + [self.mc_truth_fake_hist, self.mc_truth_fake_sys_hist] 
        keys = [k for k in keys if k]
        return keys


    ############################################################################
    def is_data(self, hist_key):
        return hist_key in self.get_data_keys()

    def is_stack(self, hist_key):
        return hist_key in self.get_stack_keys()

    def is_raw_mc(self, hist_key):
        return hist_key in self.get_raw_mc_keys()

    def is_fake_mc(self, hist_key):
        return hist_key in self.get_truth_fake_keys()
    
    def is_fake_comp_sys_mc(self, hist_key):
        return hist_key in self.get_truth_fake_comp_sys_keys()

    def is_bkg_mc(self, hist_key):
        return hist_key in self.get_truth_bkg_keys()

    def is_bkg_sys_up_mc(self, hist_key):
        return hist_key == self.mc_truth_bkg_sys_up_hist 
    def is_bkg_sys_dn_mc(self, hist_key):
        return hist_key == self.mc_truth_bkg_sys_dn_hist 

################################################################################
def main ():
    """ Main Function """

    global args

    # Create hist container
    # Organization : dict["channel"]["sample"]["num/den"] = TH1D
    hists = defaultdict(lambda: defaultdict(lambda: defaultdict(r.TH1D)))
    for reg in REGIONS:
        # Get plots for this region
        plots_with_region = [p for p in PLOTS if p.region == reg.name]
        if not len(plots_with_region): continue
        print '\n', 20*'-', "Plots for %s region"%reg.displayname, 20*'-', '\n'

        ########################################################################
        print "Setting EventLists for %s"%reg.name
        cut = r.TCut(reg.tcut)
        num_or_den = get_fake_channel(reg.name)
        samples = [s for s in SAMPLES if num_or_den in s.name]
        for sample in samples:
            list_name = "list_" + reg.name + "_" + sample.name
            sample.set_event_list(cut, list_name, EVENT_LIST_DIR)

        ########################################################################
        # Make hists
        for plot in plots_with_region:
            if args.suffix:
                if plot.suffix:
                    plot.suffix += "_" + args.suffix
                else:
                    plot.suffix = args.suffix
            
            print "Running on %s plot"%plot.name
            add_ff_hist_primitives(plot, hists, reg)
    print "Identified channels:", hists.keys()
    print "Samples in each channel:",sorted(hists[hists.keys()[0]][conf.DEN_STR].keys(), key=len)
    print "\nFormatting histograms"
    format_and_combine_hists(hists)
    print "Making fake factor histograms"
    ff_hists = get_fake_factor_hists(hists)
    for channel_name, ff_hist_dict in ff_hists.iteritems():
        print "FF values for", channel_name
        ff_hist = ff_hist_dict[KEYS.data_corr_fake_factor]
        print pu.print_hist(ff_hist)

        suffix = "_" + args.suffix if args.suffix else ""
        name = channel_name + "_FFbins" + suffix + ".tex"
        save_path = os.path.join(YIELD_TBL_DIR, name)
        with open(save_path, 'w') as ofile:
            print "Saving FF table to", save_path
            ofile.write(pu.print_hist(ff_hist, tablefmt='latex'))

    print "Making plots"
    save_and_write_hists(ff_hists, hists)

    print "Yields found during plotting...\n"
    for yld_tbl in YIELD_TABLES:
        yld_tbl.Print()
        print '\n'

    print 30*'-', 'PLOTS COMPLETED', 30*'-','\n'
################################################################################
# Fake factor functions
def get_comp_sys_weight(num_or_den):
    isEl = 'l_flav[2] == 0'
    isMu = 'l_flav[2] == 1'
   
    is_noOriginInfo = 'l_truthClass[2]==-3'; 
    is_noTruthInfo = 'l_truthClass[2]==-1'; 
    is_promptEl_from_FSR = 'l_truthClass[2]==4';
    is_hadDecay = 'l_truthClass[2]==5';
    is_Mu_as_e = 'l_truthClass[2]==6';
    is_HF_tau = 'l_truthClass[2]==7';
    is_HF_B = 'l_truthClass[2]==8'; 
    is_HF_C = 'l_truthClass[2]==9';
    is_bkgEl_from_phoConv = 'l_truthClass[2]==10';
   
    # Numbers were determined manually by plotting l_truthClass variables
    den_comp_weight =     "(%f * (%s && %s))" % (0.0, isEl, is_noOriginInfo)
    num_comp_weight =     "(%f * (%s && %s))" % (0.0, isEl, is_noOriginInfo)
    den_comp_weight += " + (%f * (%s && %s))" % (2.0, isMu, is_noOriginInfo)
    num_comp_weight += " + (%f * (%s && %s))" % (5.8, isMu, is_noOriginInfo)
    den_comp_weight += " + (%f * (%s && %s))" % (0.0, isEl, is_noTruthInfo)
    num_comp_weight += " + (%f * (%s && %s))" % (0.0, isEl, is_noTruthInfo)
    den_comp_weight += " + (%f * (%s && %s))" % (0.1, isMu, is_noTruthInfo)
    num_comp_weight += " + (%f * (%s && %s))" % (0.4, isMu, is_noTruthInfo)
    den_comp_weight += " + (%f * (%s && %s))" % (0.0, isEl, is_promptEl_from_FSR)
    num_comp_weight += " + (%f * (%s && %s))" % (1.2, isEl, is_promptEl_from_FSR)
    den_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_promptEl_from_FSR)
    num_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_promptEl_from_FSR)
    den_comp_weight += " + (%f * (%s && %s))" % (0.6, isEl, is_hadDecay)
    num_comp_weight += " + (%f * (%s && %s))" % (1.3, isEl, is_hadDecay)
    den_comp_weight += " + (%f * (%s && %s))" % (1.1, isMu, is_hadDecay)
    num_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_hadDecay)
    den_comp_weight += " + (%f * (%s && %s))" % (259.8, isEl, is_Mu_as_e)
    num_comp_weight += " + (%f * (%s && %s))" % (54.0, isEl, is_Mu_as_e)
    den_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_Mu_as_e)
    num_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_Mu_as_e)
    den_comp_weight += " + (%f * (%s && %s))" % (0.3, isEl, is_HF_tau)
    num_comp_weight += " + (%f * (%s && %s))" % (2.4, isEl, is_HF_tau)
    den_comp_weight += " + (%f * (%s && %s))" % (1.8, isMu, is_HF_tau)
    num_comp_weight += " + (%f * (%s && %s))" % (6.8, isMu, is_HF_tau)
    den_comp_weight += " + (%f * (%s && %s))" % (0.7, isEl, is_HF_B)
    num_comp_weight += " + (%f * (%s && %s))" % (0.7, isEl, is_HF_B)
    den_comp_weight += " + (%f * (%s && %s))" % (0.7, isMu, is_HF_B)
    num_comp_weight += " + (%f * (%s && %s))" % (0.4, isMu, is_HF_B)
    den_comp_weight += " + (%f * (%s && %s))" % (4.6, isEl, is_HF_C)
    num_comp_weight += " + (%f * (%s && %s))" % (3.9, isEl, is_HF_C)
    den_comp_weight += " + (%f * (%s && %s))" % (2.6, isMu, is_HF_C)
    num_comp_weight += " + (%f * (%s && %s))" % (5.2, isMu, is_HF_C)
    den_comp_weight += " + (%f * (%s && %s))" % (0.5, isEl, is_bkgEl_from_phoConv)
    num_comp_weight += " + (%f * (%s && %s))" % (5.7, isEl, is_bkgEl_from_phoConv)
    den_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_bkgEl_from_phoConv)
    num_comp_weight += " + (%f * (%s && %s))" % (0.0, isMu, is_bkgEl_from_phoConv)

    if num_or_den == conf.NUM_STR:
        return "(" + num_comp_weight + ")"
    else:
        return "(" + den_comp_weight + ")"
   
def add_ff_hist_primitives(plot, hists, reg):
    channel_name = remove_num_den(reg.name)
    num_or_den = get_fake_channel(reg.name)
    samples = [s for s in SAMPLES if num_or_den in s.name]

    yld_tbls = defaultdict(lambda: deepcopy(YIELD_TBL))
    for sample in samples:
        # Get histogram(s) for each plot-sample pair
        # For MC samples, make additional hists for truth matched selections

        # Get cuts
        cuts = []
        if sample.isMC:
            weight = "%s * %s"%(sample.weight_str, sample.scale_factor)
            cuts.append("(%s) * %s"%(reg.tcut, weight))
            cuts.append("(%s && %s) * %s"%(reg.tcut, reg.truth_fake_sel, weight))
            cuts.append("(%s && %s) * %s"%(reg.tcut, reg.truth_bkg_sel, weight))
            if args.systematics:
                cuts.append("(%s && %s) * %s * (%s)"%(reg.tcut, reg.truth_fake_sel, weight, get_comp_sys_weight(num_or_den)))
        else: # Data
            cuts.append("(" + reg.tcut + ")")

        # Apply each cut to the sample and fill hist
        for cut in cuts:
            hist_key = KEYS.generate_hist_key(sample, reg, cut)
            # histogram name must be unique relative to all hists made by script
            if plot.is3D:
                var = plot.zvariable + ":" + plot.yvariable + ":" + plot.xvariable 
            elif plot.is2D:
                var = plot.yvariable + ":" + plot.xvariable
            else:
                var = plot.variable

            var = sub(r'[:\-(){}[\]]+','', var)
            h_name = "h_"+reg.name+'_'+hist_key+"_"+ var
            hist = build_hist(h_name, plot, sample, cut)
            hist.displayname = sample.displayname
            hist.plot = plot
            hist.color = sample.color
            hists[channel_name][num_or_den][hist_key] = hist
            
            if KEYS.is_fake_mc(hist_key):
                yld_region = "(2 Prompt + 1 Fake)"
                num_or_den_sel = 'den_sel'
            elif KEYS.is_bkg_mc(hist_key):
                yld_region = "(3 Prompt)"
                num_or_den_sel = 'num_sel'
            else:
                yld_region = reg.displayname 
                num_or_den_sel = 'base_sel'

            yld_tbls[num_or_den_sel].region = yld_region
            yld_tbls[num_or_den_sel].variable = var
            stat_err = r.Double(0.0)
            if plot.is3D:
                integral = hist.IntegralAndError(0,-1,0,-1,0,-1,stat_err)
            elif plot.is2D:
                integral = hist.IntegralAndError(0,-1,0,-1,stat_err)
            else:
                integral = hist.IntegralAndError(0,-1,stat_err)
            print "Base histograms (Sample: %s, Yield: %.2f): %s created"%(
                    sample.name, integral, h_name)
            if sample.isMC:
                yld_tbls[num_or_den_sel].mc[sample.name] = UncFloat(integral, stat_err) 
            else: 
                yld_tbls['den_sel'].data[sample.name] = UncFloat(0, 0) 
                yld_tbls['num_sel'].data[sample.name] = UncFloat(0, 0) 
                yld_tbls['base_sel'].data[sample.name] = UncFloat(integral, stat_err)

    yld_tbls['base_sel'].partitions.append(yld_tbls['den_sel'])
    yld_tbls['base_sel'].partitions.append(yld_tbls['num_sel'])
    YIELD_TABLES.append(yld_tbls['base_sel'])
     
def format_and_combine_hists(hists):
    '''
    Make all the combined histograms.

    1) THStack plots for all sets of MC samples
    2) Total SM plots for all sets of MC samples
    3) Data with non-fake background subtracted

    args:
        hists (dict[dict[dict[TH1D]]] : histograms organized by channel,
        sample or combined samples, and then by numerator or denominator
        selections

    '''
    # First loop to make MC stack histograms
    for channel_name, ch_dict in hists.items():
        for num_or_den, sample_dict in ch_dict.items():
            for hist_key, hist in sample_dict.items():
                if KEYS.is_data(hist_key): continue
                stack_key = KEYS.generate_stack_key(hist_key)
                if stack_key not in sample_dict:
                    print "Stack histograms (Channel: %s [%s]): %s initialized"%(
                            channel_name, num_or_den, stack_key)
                    stack_name = stack_key+'_'+num_or_den
                    if hist.plot.is3D:
                        #HACK for 3D plots
                        sample_dict[stack_key] = hist.Clone(stack_name)
                        sample_dict[stack_key].Reset()
                    else:
                        sample_dict[stack_key] = r.THStack(stack_name, "")
                    sample_dict[stack_key].plot = hist.plot

                sample_dict[stack_key].Add(hist)

        # Second loop to make MC total histograms
        for num_or_den, sample_dict in ch_dict.items():
            for hist_key, hist in sample_dict.items():
                #TODO: Use KeyManager to grab correct hist
                if not KEYS.is_stack(hist_key): continue
                mc_hist_key = KEYS.generate_mc_hist_key(hist_key)
                hist_name = mc_hist_key+'_'+num_or_den
                if hist.plot.is3D:
                    #HACK for 3D plots
                    mc_total_hist = hist.Clone(hist_name)
                else:
                    mc_total_hist = hist.GetStack().Last().Clone(hist_name)
                mc_total_hist.plot = hist.plot
                if KEYS.is_bkg_mc(hist_key):
                    mc_total_hist.displayname = KEYS.bkg_mc_str.replace("_"," ")
                elif KEYS.is_fake_mc(hist_key):
                    mc_total_hist.displayname = KEYS.fake_mc_str.replace("_"," ")
                elif KEYS.is_fake_comp_sys_mc(hist_key):
                    mc_total_hist.displayname = KEYS.fake_mc_str.replace("_"," ")
                else:
                    mc_total_hist.displayname = 'Total MC'
                if not hist.plot.is2D and not hist.plot.is3D:
                    mc_total_hist.SetLineWidth(3)
                    mc_total_hist.SetLineStyle(1)
                    mc_total_hist.SetFillStyle(0)
                    mc_total_hist.SetLineWidth(3)
                mc_total_hist.is_total = True

                sample_dict[mc_hist_key] = mc_total_hist
                print "Total MC histogram  (Channel: %s [%s]): %s created"%(
                       channel_name, num_or_den, mc_hist_key)

            # Grab hists
            data_hist = sample_dict[KEYS.data_hist]
            mc_background_hist = sample_dict[KEYS.mc_truth_bkg_hist]
            mc_truth_fake_hist = sample_dict[KEYS.mc_truth_fake_hist]
            mc_truth_fake_sys_hist = sample_dict[KEYS.mc_truth_fake_sys_hist]
            mc_truth_fake_sys_stack = sample_dict[KEYS.mc_truth_fake_sys_stack]
            data_corr_hist_key = KEYS.generate_data_corr_key()

            # Rescale composition scaled MC hists to have same yield as unscaled
            # - this forces the composition systematic to effect the shape but no the yield
            if args.systematics:
                norm_factor = mc_truth_fake_hist.Integral(0,-1) / mc_truth_fake_sys_hist.Integral(0,-1)
                #norm_factor = 1
                mc_truth_fake_sys_hist.Scale(norm_factor)
                pu.scale_thstack(mc_truth_fake_sys_stack, norm_factor)

            # Create and store background-subtracted data histogram
            data_corrected_name = "%s_%s"%(data_corr_hist_key, num_or_den)
            data_corrected_hist = data_hist.Clone(data_corrected_name)
            data_corrected_hist.Add(mc_background_hist, -1)
            data_corrected_hist.displayname = "Data (bkgd subtracted)"
            data_corrected_hist.plot = data_hist.plot
            sample_dict[data_corr_hist_key] = data_corrected_hist
            print "Data corrected histogram (Channel: %s [%s]): %s created"%(
                    channel_name, num_or_den, data_corr_hist_key)

            # Create and store composition adjusted data histograms
            if args.systematics:
                data_comp_sys_key = KEYS.generate_data_comp_sys_key()
                data_comp_sys_name = "%s_%s" % (data_comp_sys_key, num_or_den)
                data_comp_sys_hist = data_corrected_hist.Clone(data_comp_sys_name)
                comp_sys_scale_factor = mc_truth_fake_sys_hist.Clone()
                #comp_sys_scale_factor.Divide(mc_truth_fake_hist)
                #data_comp_sys_hist.Multiply(comp_sys_scale_factor)
                comp_sys_scale_factor.Add(mc_truth_fake_hist, -1)
                data_comp_sys_hist.Add(comp_sys_scale_factor)
                data_comp_sys_hist.displayname = "Data (composition systematic)"
                data_comp_sys_hist.plot = data_corrected_hist.plot
                sample_dict[data_comp_sys_key] = data_comp_sys_hist
                print "Data composition systematic  histogram (Channel: %s [%s]): %s created"%(
                        channel_name, num_or_den, data_comp_sys_key)

                # Create and store prompt background adjusted up data histograms
                mc_prompt_sys_up_key = KEYS.generate_prompt_sys_up_key(KEYS.mc_truth_bkg_hist)
                mc_prompt_sys_up_name = "%s_%s" % (mc_prompt_sys_up_key, num_or_den)
                mc_bkgd_sys_up_hist = mc_background_hist.Clone(mc_prompt_sys_up_name)
                pu.shift_hist_by_unc(mc_bkgd_sys_up_hist, up=True)
                data_prompt_sys_up_key = KEYS.generate_prompt_sys_up_key(KEYS.data_hist)
                data_prompt_sys_up_name = "%s_%s" % (data_prompt_sys_up_key, num_or_den)
                data_prompt_sys_up_hist = data_hist.Clone(data_prompt_sys_up_name)
                data_prompt_sys_up_hist.Add(mc_bkgd_sys_up_hist, -1)

                mc_bkgd_sys_up_hist.displayname = "MC real background [SYS_UP]"
                mc_bkgd_sys_up_hist.plot = mc_background_hist.plot
                sample_dict[mc_prompt_sys_up_key] = mc_bkgd_sys_up_hist
                data_prompt_sys_up_hist.displayname = "Data (bkgd subtracted [SYS_UP])"
                data_prompt_sys_up_hist.plot = data_hist.plot
                sample_dict[data_prompt_sys_up_key] = data_prompt_sys_up_hist
                
                # Create and store prompt background adjusted up data histograms
                mc_prompt_sys_dn_key = KEYS.generate_prompt_sys_dn_key(KEYS.mc_truth_bkg_hist)
                mc_prompt_sys_dn_name = "%s_%s" % (mc_prompt_sys_dn_key, num_or_den)
                mc_bkgd_sys_dn_hist = mc_background_hist.Clone(mc_prompt_sys_dn_name)
                pu.shift_hist_by_unc(mc_bkgd_sys_dn_hist, up=False)
                data_prompt_sys_dn_key = KEYS.generate_prompt_sys_dn_key(KEYS.data_hist)
                data_prompt_sys_dn_name = "%s_%s" % (data_prompt_sys_dn_key, num_or_den)
                data_prompt_sys_dn_hist = data_hist.Clone(data_prompt_sys_dn_name)
                data_prompt_sys_dn_hist.Add(mc_bkgd_sys_dn_hist, -1)

                mc_bkgd_sys_dn_hist.displayname = "MC real background [SYS_DN]"
                mc_bkgd_sys_dn_hist.plot = mc_background_hist.plot
                sample_dict[mc_prompt_sys_dn_key] = mc_bkgd_sys_dn_hist
                data_prompt_sys_dn_hist.displayname = "Data (bkgd subtracted [SYS_DN])"
                data_prompt_sys_dn_hist.plot = data_hist.plot
                sample_dict[data_prompt_sys_dn_key] = data_prompt_sys_dn_hist


def get_fake_factor_hists(hists):
    ff_hists = defaultdict(lambda: defaultdict(r.TH1D))
    for channel_name, ch_dict in hists.iteritems():
        for hist_key in KEYS.get_fake_factor_input_keys():
            fake_factor_key = KEYS.generate_fake_factor_key(hist_key)
            fake_factor_name = channel_name + "_" + fake_factor_key
            num_hist = ch_dict[conf.NUM_STR][hist_key]
            ff_hist = num_hist.Clone(fake_factor_name)
            ff_hist.Divide(ch_dict[conf.DEN_STR][hist_key])
            ff_hist.SetMaximum(5)

            # Append some information   
            ff_hist.displayname = hist_key.replace("_"," ")
            ff_hist.plot = num_hist.plot
            ff_hist.plot = copy(num_hist.plot)

            # Format the hists
            if not ff_hist.plot.is2D and not ff_hist.plot.is3D:
                ff_hist.plot.update(doLogY = False, doNorm = True) #doNorm only affects axis

            ff_hists[channel_name][fake_factor_key] = ff_hist

    return ff_hists

def save_and_write_hists(ff_hists_dict, hists):
    # Writing fake factor hists to root file
    for channel_name, ff_hists in ff_hists_dict.iteritems():
        ff_hist = ff_hists[KEYS.data_corr_fake_factor]
        if args.systematics:
            ''' 
            Get uncertainty from each systematic and then add them in qaudrature with the statistical uncertainty
            '''
            # Get nbins in fake factor
            nbins = ff_hist.GetNbinsX()+2
            if ff_hist.plot.is2D:
                nbins *= ff_hist.GetNbinsY()+2
            if ff_hist.plot.is3D:
                nbins *= ff_hist.GetNbinsY()+2
                nbins *= ff_hist.GetNbinsZ()+2
            
            # Non closure systematics
            ff_nonclosure_unc = ff_hist.Clone(ff_hist.GetName() + "_nonclosure_Syst")
            ff_nonclosure_unc.Reset()
            for ibin in range(0, nbins+1):
                ff_nonclosure_unc.SetBinContent(ibin, conf.NONCLOSURE_SYS)

            # Composition systematics
            ff_composition_unc = ff_hists[KEYS.data_comp_sys_fake_factor]
            mc_comp_sys_ff_hist = ff_hists[KEYS.mc_comp_sys_fake_factor] 
            mc_ff_hist = ff_hists[KEYS.mc_fake_factor]

            # Background subtraction systematics
            ff_bkgd_subtraction_unc_up = ff_hists[KEYS.data_prompt_sys_up_fake_factor]
            ff_bkgd_subtraction_unc_dn = ff_hists[KEYS.data_prompt_sys_dn_fake_factor]

            # Combine systematics
            ff_sys_unc = ff_hist.Clone(ff_hist.GetName() + "_Syst") 
            ff_sys_unc.Reset()
            for ibin in range(0, nbins):
                value = ff_hist.GetBinContent(ibin)
               
                stat_err = ff_hist.GetBinError(ibin)
                
                #comp_err = abs(value - ff_composition_unc.GetBinContent(ibin))
                mc_ff_val = mc_ff_hist.GetBinContent(ibin)
                mc_comp_sys_ff_val = mc_comp_sys_ff_hist.GetBinContent(ibin)
                mc_comp_sys_rel_err = abs(mc_ff_val - mc_comp_sys_ff_val) / mc_ff_val if mc_ff_val else 0
                comp_err = value * mc_comp_sys_rel_err
                
                subtraction_err_up = abs(value - ff_bkgd_subtraction_unc_up.GetBinContent(ibin))
                subtraction_err_dn = abs(value - ff_bkgd_subtraction_unc_dn.GetBinContent(ibin))
                
                nonclo_err  = value * ff_nonclosure_unc.GetBinContent(ibin)
                
                ff_syst_err = sqrt(stat_err**2 + comp_err**2 + nonclo_err**2 + subtraction_err_up**2 + subtraction_err_dn**2)
                print "INFO :: Sys Error = sum_quad(stat, comp, nonclo, prompt) = sum_quad(%.2f, %.2f, %.2f, %.2f, %.2f) = %.3f" % (stat_err, comp_err, nonclo_err, subtraction_err_up, subtraction_err_dn, ff_syst_err)
                ff_sys_unc.SetBinContent(ibin, ff_syst_err)
                ff_hist.SetBinError(ibin, ff_syst_err)

            if args.ofile_name:
                with open_root(args.ofile_name,"RECREATE") as ofile:
                    ff_hist.Write()
                    ff_sys_unc.Write()
                    
            ff_nonclosure_unc.Delete()
            ff_sys_unc.Delete()

        # Only try to paint 1D histograms
        if ff_hist.plot.is2D or ff_hist.plot.is3D:
            return

        # Saving plots of fake factor hists
        data_corr_ff_hist = ff_hists[KEYS.data_corr_fake_factor]
        data_corr_ff_hist.displayname = "Data (Corrected)"
        

        #for ibin in range(0, data_corr_ff_hist.GetNbinsX()+2):
        #    print "Final error = %.4f" % data_corr_ff_hist.GetBinError(ibin)
        
        #data_ff_hist = ff_hists[KEYS.data_fake_factor]
        mc_ff_hist = ff_hists[KEYS.mc_fake_factor]
        mc_ff_hist.color = r.kBlue+2
        mc_ff_hist.displayname = "MC"
        #data_ff_hist.color = r.kBlack
        data_corr_ff_hist.color = r.kRed
        plot_title = 'Fake Factor (Ch: %s)'%channel_name
        #hists_to_plot = [mc_ff_hist, data_ff_hist, data_corr_ff_hist]
        hists_to_plot = [mc_ff_hist, data_corr_ff_hist]
        plot = data_corr_ff_hist.plot
        plot.ylabel = "Fake Factor"
        reg_name = "Z+jets (Mu)" if channel_name.endswith('m') else "Z+jets (El)"
        save_hist(plot_title, plot, reg_name, hists_to_plot)

        if args.systematics:
            ff_composition_data_unc = ff_hists[KEYS.data_comp_sys_fake_factor] 
            ff_composition_data_unc.displayname = "Composition Sys (Data Shift)"
            ff_composition_data_unc.color = r.kYellow
            mc_comp_sys_ff_hist = ff_hists[KEYS.mc_comp_sys_fake_factor] 
            mc_comp_sys_ff_hist.displayname = "Composition Sys (MC Shift)"
            mc_comp_sys_ff_hist.color = r.kSpring
            ff_bkgd_subtraction_unc_up = ff_hists[KEYS.data_prompt_sys_up_fake_factor]
            ff_bkgd_subtraction_unc_up.displayname = "Background Subtraction SYS_UP"
            ff_bkgd_subtraction_unc_up.color = r.kGreen+2
            ff_bkgd_subtraction_unc_dn = ff_hists[KEYS.data_prompt_sys_dn_fake_factor]
            ff_bkgd_subtraction_unc_dn.displayname = "Background Subtraction SYS_DN"
            ff_bkgd_subtraction_unc_dn.color = r.kGreen+1
            plot_title = "Fake Factor Systematic Variation" 
            #hists_to_plot = [ff_composition_data_unc, data_corr_ff_hist, ff_bkgd_subtraction_unc_up, ff_bkgd_subtraction_unc_dn]
            #hists_to_plot = [mc_ff_hist, mc_comp_sys_ff_hist, data_corr_ff_hist, ff_bkgd_subtraction_unc_up, ff_bkgd_subtraction_unc_dn]
            hists_to_plot = [mc_ff_hist, mc_comp_sys_ff_hist, ff_composition_data_unc, data_corr_ff_hist]
            save_hist(plot_title, plot, reg_name, hists_to_plot)


    # Save all other desired plots
    for channel_name, ch_dict in hists.iteritems():
        for num_or_den, sample_dict in ch_dict.iteritems():
            # Save MC Stacks
            data_hist = sample_dict[KEYS.data_hist]
            mc_stack = sample_dict[KEYS.mc_stack]
            mc_hist = sample_dict[KEYS.mc_hist]
            data_hist.color = r.kBlack
            mc_hist.color = r.kBlack
            plot_title = 'MC Backgrounds'
            plot_title += ' (%s)'%num_or_den
            hists_to_plot = [mc_stack, mc_hist, data_hist]
            plot = mc_stack.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            mc_truth_bkg_stack = sample_dict[KEYS.mc_truth_bkg_stack]
            mc_truth_bkg_hist = sample_dict[KEYS.mc_truth_bkg_hist]
            data_hist.color = r.kBlack
            mc_truth_bkg_hist.color = r.kBlack
            plot_title = 'MC Backgrounds with 3 ID truth-matched prompt leptons'
            plot_title += ' (%s)'%num_or_den
            hists_to_plot = [mc_truth_bkg_stack, mc_truth_bkg_hist, data_hist]
            plot = mc_truth_bkg_stack.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            mc_truth_fake_stack = sample_dict[KEYS.mc_truth_fake_stack]
            mc_truth_fake_hist = sample_dict[KEYS.mc_truth_fake_hist]
            data_hist.color = r.kBlack
            mc_truth_fake_hist.color = r.kBlack
            plot_title = 'MC Backgrounds with anti-ID truth-matched fake lepton'
            plot_title += ' (%s)'%num_or_den
            hists_to_plot = [mc_truth_fake_stack, mc_truth_fake_hist, data_hist]
            plot = mc_truth_fake_stack.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            if args.systematics:
                mc_truth_fake_stack = sample_dict[KEYS.mc_truth_fake_sys_stack]
                mc_truth_fake_sys_hist = sample_dict[KEYS.mc_truth_fake_sys_hist]
                data_hist.color = r.kBlack
                mc_truth_fake_sys_hist.color = r.kBlack
                plot_title = 'Scaled MC Backgrounds with anti-ID truth-matched fake lepton'
                plot_title += ' (%s)'%num_or_den
                hists_to_plot = [mc_truth_fake_stack, mc_truth_fake_sys_hist, data_hist]
                plot = mc_truth_fake_stack.plot
                save_hist(plot_title, plot, channel_name, hists_to_plot)

            # Overlay of data before and after correction with MC truth
            # background stack
            data_corr_hist = sample_dict[KEYS.data_corr_hist]
            data_hist.color = r.kBlack
            data_corr_hist.color = r.kRed
            mc_truth_bkg_hist.color = r.kBlue - 1
            plot_title = 'Data before and after background substraction'
            plot_title += ' (%s)'%num_or_den
            hists_to_plot = [mc_truth_bkg_hist, data_hist, data_corr_hist]
            plot = data_hist.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            # Overlay of fake MC with data after MC truth background
            # subtraction
            plot_title = 'Resulting fake estimates in Data and MC'
            plot_title += ' (%s)'%num_or_den
            data_corr_hist.color = r.kBlack
            hists_to_plot = [mc_truth_fake_stack, data_corr_hist]
            plot = mc_truth_fake_stack.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            # Overlay of data with stack of total MC truth background and total
            # MC truth fake
            stack = r.THStack("bkg_and_fake_mc","")
            mc_truth_bkg_hist.color = r.kBlue - 1
            mc_truth_fake_hist.color = r.kGray
            data_hist.color = r.kBlack
            mc_truth_fake_hist.displayname = "MC fake background"
            mc_truth_bkg_hist.displayname = "MC real background"
            stack.Add(mc_truth_fake_hist)
            stack.Add(mc_truth_bkg_hist)
            plot_title = 'MC breakdown of fake and non-fake backgrounds'
            plot_title += ' (%s)'%num_or_den
            hists_to_plot = [stack, data_hist]
            plot = data_hist.plot
            save_hist(plot_title, plot, channel_name, hists_to_plot)

            # Overlay of data with stack of total MC truth background and total
            # MC truth fake with composition rescaling
            if args.systematics:
                stack = r.THStack("bkg_and_scaled_fake_mc","")
                mc_truth_bkg_hist.color = r.kBlue - 1
                mc_truth_fake_sys_hist.color = r.kGray
                data_hist.color = r.kBlack
                mc_truth_fake_sys_hist.displayname = "MC rescaled fake background"
                stack.Add(mc_truth_fake_sys_hist)
                stack.Add(mc_truth_bkg_hist)
                plot_title = 'MC breakdown of scaled fake and non-fake backgrounds'
                plot_title += ' (%s)'%num_or_den
                hists_to_plot = [stack, data_hist]
                plot = data_hist.plot
                save_hist(plot_title, plot, channel_name, hists_to_plot)

def save_hist(title, plot, reg_name, hist_list):
    
    if plot.is3D:
        var = plot.zvariable + ":" + plot.yvariable + ":" + plot.xvariable 
    elif plot.is2D:
        var = plot.yvariable + ":" + plot.xvariable
    else:
        var = plot.variable
    suffix = "_" + plot.suffix if plot.suffix else ""
    outname = reg_name+ '_' + var+ '_' + title + suffix + ".pdf"
    outname = outname.replace(" ","_")
    outname = sub(r'[:\-(){}[\]+]+','', outname)
    
    plot.setStackPads(outname)
    can = plot.pads.canvas
    can.cd()
    can.SetTitle(title)
    if not (plot.is2D or plot.is3D) and plot.doLogY : can.SetLogy(True)

    # Make Axis
    axis = make_plot1D_axis(plot)
    if plot.auto_set_ylimits:
        print "TESTING :: Autoformatting axis"
        reformat_axis(plot, axis, hist_list)

    # Format Primitives
    # TODO: Sort THStack by integral

    # Format primitives and fill legend
    legend = pu.default_legend(xl=0.55,yl=0.71,xh=0.93,yh=0.90)
    legend.SetNColumns(1)
   
    stack_flag = False
    for hist in hist_list:
        if isinstance(hist, r.THStack):
            stack_flag = True
            for stack_hist in hist.GetHists():
                stack_hist.SetFillColor(stack_hist.color)
                stack_hist.SetLineColor(stack_hist.color)
                stack_hist.SetFillStyle(1001)
                legend.AddEntry(stack_hist, stack_hist.displayname, "f")
            hist.Modified()
        else:
            hist.SetLineWidth(3)
            if hasattr(hist, 'is_total') and not stack_flag:
                hist.SetFillStyle(1001)
                hist.SetFillColor(hist.color)
                hist.SetLineColor(hist.color)
                leg_type = 'f'
            elif hasattr(hist, 'is_total') and stack_flag:
                hist.SetFillStyle(0)
                hist.SetLineColor(hist.color)
                leg_type = 'l'
            else:
                hist.SetFillStyle(0)
                hist.SetMarkerStyle(r.kFullCircle)
                hist.SetMarkerSize(1.5)
                hist.SetMarkerColor(hist.color)
                #hist.SetLineColor(hist.color)
                leg_type = 'p'
            legend.AddEntry(hist, hist.displayname, leg_type)

    # Draw primitives to canvas
    axis.Draw()
    for hist in hist_list:
        if isinstance(hist, r.THStack):
            hist.Draw("HIST SAME")
        elif hasattr(hist, 'is_total'):
            hist.Draw("HIST SAME")
        else:
            hist.Draw("pE1 same")
            #hist.Draw("HIST SAME")

    legend.Draw()
    pu.draw_atlas_label(plot.atlas_status,plot.atlas_lumi, reg_name)

    # Finalize
    can.RedrawAxis()
    can.SetTickx()
    can.SetTicky()
    can.Update()

    # Save
    save_path = os.path.join(PLOTS_DIR, args.dir_name, outname)
    save_path = os.path.normpath(save_path)
    can.SaveAs(save_path)
    axis.Delete()
    can.Clear()

def reformat_axis(plot, axis, hist_list):
    ''' Reformat axis to fit content and labels'''
    # Get maximum histogram y-value
    maxs = []
    mins = []
    for hist in hist_list:
        if isinstance(hist, r.TGraph):
            maxs.append(pu.get_tgraph_max(hist))
            mins.append(pu.get_tgraph_min(hist))
        else:
            maxs.append(hist.GetMaximum())
            mins.append(hist.GetMinimum())
    maxy, miny = max(maxs), min(mins)
    assert maxy >= 0

    # Get default y-axis max and min limits
    logy = plot.doLogY
    if logy:
        ymax = 10**(pu.get_order_of_mag(maxy))
        if miny > 0:
            ymin = 10**(pu.get_order_of_mag(miny))
        else:
            ymin = 10**(pu.get_order_of_mag(maxy) - 7)
    else:
        ymax = maxy
        ymin = 0

    # Get y-axis max multiplier to fit labels
    max_mult = 1e4 if logy else 1.8

    # reformat the axis
    axis.SetMaximum(max_mult*maxy)
    axis.SetMinimum(ymin)
    print "TESTING :: plot %s has maxy = 1.8 * %.2f = %.2f" % (plot.variable, maxy, max_mult*maxy)


################################################################################
# Fake factor functions
def build_hist(h_name, plot, sample, cut):
    cut = r.TCut(cut)
    if plot.is3D:
        hist = r.TH3D(h_name, "", plot.nxbins, plot.xbin_edges, plot.nybins, plot.ybin_edges, plot.nzbins, plot.zbin_edges)
        draw_cmd = "%s>>%s"%(plot.zvariable+":"+plot.yvariable+":"+plot.xvariable, hist.GetName())
        sample.tree.Draw(draw_cmd, cut, "goff")
    elif plot.is2D:
        hist = r.TH2D(h_name, "", plot.nxbins, plot.xbin_edges, plot.nybins, plot.ybin_edges)
        draw_cmd = "%s>>%s"%(plot.yvariable+":"+plot.xvariable, hist.GetName())
        sample.tree.Draw(draw_cmd, cut, "goff")
    else:
        labels = ";%s;%s" % (plot.xlabel, plot.ylabel)
        hist = r.TH1D(h_name, labels, plot.nbins, plot.bin_edges)
        hist.Sumw2
        hist.SetLineColor(sample.color)
        draw_cmd = "%s>>+%s"%(plot.variable, hist.GetName())
        sample.tree.Draw(draw_cmd, cut, "goff")

        if plot.add_overflow:
            pu.add_overflow_to_lastbin(hist)
        if plot.add_underflow:
            pu.add_underflow_to_firstbin(hist)

    return hist

def remove_num_den(name):
    name = name.replace(conf.NUM_STR,"")
    name = name.replace(conf.DEN_STR,"")
    name = name.replace("__","_")
    if name.endswith("_"): name = name[:-1]
    if name.startswith("_"): name = name[1:]
    return name


def get_fake_channel(reg_name):
    return conf.DEN_STR if conf.DEN_STR in reg_name else conf.NUM_STR

################################################################################
# Check functions
def check_environment():
    """ Check if the shell environment is setup as expected """
    assert os.environ['USER'], "USER variable not set"

    python_ver = sys.version_info[0] + 0.1*sys.version_info[1]
    assert python_ver >= 2.7, ("Running old version of python\n", sys.version)

def check_input_args():
    """
    Check that user inputs are as expected
    """
    if not os.path.isfile(args.config):
        print "ERROR :: configuration file not found: %s"%(args.config)
        sys.exit()

    if not os.path.exists(args.dir_name):
        print "ERROR :: output directory not found: %s"%(args.dir_name)
        sys.exit()

    if args.ofile_name:
        of = os.path.join(args.dir_name, args.ofile_name)
        if os.path.exists(of):
            if not os.path.exists("%s.bu"%of):
                print "Renaming old output file %s -> %s.bu"%(of, of)
                mv_cmd = 'mv %s %s.bu'%(of, of)
                subprocess.call(mv_cmd, shell=True)
            else:
                print "WARNING :: Output file already exists: %s"%of
                print "\tConsider deleting it or its backup (%s.bu)"%of
                sys.exit()

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
    bad_regions = [p.region for p in PLOTS if p.region not in region_names]
    if len(bad_regions) > 0 :
        print 'check_for_consistency ERROR    '\
        'You have configured a plot for a region that is not defined. '\
        'Here is the list of "bad regions":'
        print bad_regions
        print 'check_for_consistency ERROR    The regions that are defined in the configuration ("%s") are:'%g_plotConfig
        print region_names
        print "check_for_consistency ERROR    Exiting."
        sys.exit()

def check_import_globals():

    #TODO: Allow plotting of multiple variables
    reg_names = [p.region for p in conf.PLOTS]
    
    assert all(conf.NUM_STR in n or conf.DEN_STR in n for n in reg_names),(
        "ERROR :: Non fake factor region defined")
    
    num_regions = [n for n in reg_names if conf.NUM_STR in n]
    den_regions = [n for n in reg_names if conf.DEN_STR in n]
    assert len(num_regions) == len(den_regions), (
        "ERROR :: Number of numerator and denominator regions do not match")
    
    num_regions = [n.replace(conf.NUM_STR, "") for n in num_regions]
    den_regions = [n.replace(conf.DEN_STR, "") for n in den_regions]
    assert sorted(num_regions) == sorted(den_regions), (
        "ERROR :: numerator and denominator regions do not matched")


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
    print "     plot config      :  %s "%args.config
    print "     output file      :  %s "%args.ofile_name
    print "     output directory :  %s "%args.dir_name
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

################################################################################
# Run main when not imported
if __name__ == '__main__':
    try:
        start_time = time.time()
        parser = argparse.ArgumentParser(
                description=__doc__,
                formatter_class=argparse.RawDescriptionHelpFormatter)
        parser.add_argument("-c", "--config",
                            default="",
                            help='path to config file')
        parser.add_argument('-o', '--ofile_name',
                            help="Create fake factor root files")
        parser.add_argument("-s", "--suffix",
                            default="",
                            help='Suffix to append to output plot names')
        parser.add_argument('-d', '--dir_name',
                            default="./",
                            help="Output directory")
        parser.add_argument('--systematics',
                            action='store_true', default=False,
                            help='Run with systematic variation')
        parser.add_argument('-v', '--verbose',
                            action='store_true', default=False,
                            help='verbose output')
        args = parser.parse_args()

        check_environment()
        check_input_args()

        import_conf = args.config.replace(".py","")
        conf = import_module(import_conf)
        check_import_globals()

        SAMPLES = conf.SAMPLES
        REGIONS = conf.REGIONS
        PLOTS = conf.PLOTS
        YIELD_TBL = conf.YIELD_TBL
        YIELD_TABLES = []
        EVENT_LIST_DIR = conf.EVENT_LIST_DIR
        YIELD_TBL_DIR = conf.YIELD_TBL_DIR
        PLOTS_DIR = conf.PLOTS_DIR
        KEYS = KeyManager()

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


