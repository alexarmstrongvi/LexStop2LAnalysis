#!/bin/usr/env python

import os
import ROOT as r
import atlasrootstyle.AtlasStyle
r.SetAtlasStyle()
from copy import deepcopy
from analysis_DSIDs import DSID_GROUPS
import glob

################################################################################
# Globals (not imported)
_work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
_sample_path = '%s/run/batch/SuperflowAnaStop2l_output' % _work_dir
_sample_path_tmp = '%s/run/batch/test_SuperflowAnaStop2L' % _work_dir #TMP
_plot_save_dir = '%s/run/plots/' % _work_dir

# Globals (imported)
EVENT_LIST_DIR = '%s/run/lists/teventlists' % _work_dir
YIELD_TBL_DIR = '%s/run/yields' % _work_dir
SAMPLES = []
REGIONS = []
PLOTS = []
YLD_TABLE = None

#         2015      2016      2017      2018
_lumi = (3219.56 + 32988.1 + 44307.4 + 59937.2) / 1000.0 # inverse fb

_samples_to_use = [
    'data',
    'ttbar',
    #'Wt',
]

_regions_to_use = [
    #'no_sel',
    'ttbar_CR',
]

_vars_to_plot = [
    #'eventweight',
    #'eventweight_multi',
    #'pupw',
    #'lepSf', # bug with trigSf overwritten
    #'btagSf',
    #'jvtSf',
    #'period_weight',
    'lept1Pt',
]
################################################################################
# Make samples

# Initialize
from PlotTools.sample import Sample, Data, MCsample
Sample.input_file_treename = 'superNt'
MCsample.weight_str = 'eventweight_multi / pupw'
MCsample.scale_factor = _lumi

# Setup samples and store if requested
data_dsids = (DSID_GROUPS['data15']
            + DSID_GROUPS['data16']
            + DSID_GROUPS['data17']
            + DSID_GROUPS['data18'])
data = Data('data','Data')
data.color = r.kBlack
if data.name in _samples_to_use:
    data.set_chain_from_dsid_list(data_dsids, _sample_path_tmp)
    SAMPLES.append(data)

ttbar = MCsample('ttbar',"t#bar{t}","t\bar{t}")
ttbar.color = r.kRed
if ttbar.name in _samples_to_use:
    ttbar.set_chain_from_dsid_list(DSID_GROUPS['ttbar'], _sample_path_tmp)
    SAMPLES.append(ttbar)

Wt = MCsample('Wt')
Wt.color = r.kRed
if Wt.name in _samples_to_use:
    Wt.set_chain_from_dsid_list(DSID_GROUPS['WtPP8'], _sample_path)
    SAMPLES.append(Wt)

# Remove samples not properly setup
SAMPLES = [s for s in SAMPLES if s.is_setup()]

# Check for at least 1 sample
assert SAMPLES, "ERROR :: No samples are setup"

################################################################################
# Make Regions
from PlotTools.region import Region

# Define common selections
elel = '(lept1Flav == 0 && lept2Flav == 0)' 
elmu = '(lept1Flav == 0 && lept2Flav == 1)' 
muel = '(lept1Flav == 1 && lept2Flav == 0)' 
mumu = '(lept1Flav == 1 && lept2Flav == 1)' 
DF = '(lept1Flav != lept2Flav)'
SF = '(lept1Flav == lept2Flav)'
OS = '(lept1q != lept2q)'
SS = '(lept1q == lept2q)'

e15_trig    = '(HLT_e24_lhmedium_L1EM20VH || HLT_e60_lhmedium || HLT_e120_lhloose)'
mu15_trig   = '(HLT_mu20_iloose_L1MU15 || HLT_mu40)'
e16_trig    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
mu16_trig   = '(HLT_mu26_ivarmedium || HLT_mu50)'
singlep_trig = "(%s)" % (' || '.join([e15_trig, mu15_trig, e16_trig, mu16_trig]))

# No selection
region = Region('no_sel','No Selection')
region.tcut = '1'
REGIONS.append(region)

# Preselection
preselection = singlep_trig
preselection += ' && ' + DF + ' && ' + OS

# Baseline

# Control regions
region = Region('ttbar_CR', 't#bar{t} CR', 't\bar{t} CR')
region.tcut = preselection
region.tcut += ' && nBJets == 2'
REGIONS.append(region)

# Check all requested regions correspond to one defined region
all_reg_names = [r.name for r in REGIONS]
if len(all_reg_names) > len(set(all_reg_names)):
    print "ERROR :: Duplicate region names:", all_reg_names
for reg_name in _regions_to_use:
    if reg_name not in all_reg_names:
        print 'ERROR :: Region %s not defined: %s' % (reg_name, all_reg_names)
REGIONS = [reg for reg in REGIONS if reg.name in _regions_to_use]

################################################################################
# Yield Table
from PlotTools.YieldTable import YieldTbl
YieldTbl.write_to_latex = True
YLD_TABLE = YieldTbl()
YLD_TABLE.add_row_formula(name="mc_total",displayname="MC Total", formula="MC")
YLD_TABLE.add_row_formula(name="data_mc_ratio",displayname="Data/MC", formula="data/MC")

################################################################################
# Make Plots
from PlotTools.plot import PlotBase, Plot1D, Types
PlotBase.output_format = 'pdf'
PlotBase.save_dir = _plot_save_dir 
Plot1D.doLogY = False
Plot1D.auto_set_ylimits = True
Plot1D.type_default = Types.stack 
PlotBase.atlas_status = 'Internal'

plots_defaults = {
    'eventweight' : Plot1D(bin_range=[-0.001, 0.01], nbins=100, xlabel='Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'eventweight_multi' : Plot1D(bin_range=[-0.002, 0.003], nbins=100, xlabel='Multi-period Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'pupw' : Plot1D(bin_range=[0, 4], nbins=100, xlabel='Pilup Weight', add_underflow=True, doLogY=True, doNorm=True),
    'lepSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Lepton SF', add_underflow=True, doLogY=True, doNorm=True),
    'trigSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Lepton SF', add_underflow=True, doLogY=True, doNorm=True),
    'btagSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='B-tag SF', add_underflow=True, doLogY=True, doNorm=True),
    'jvtSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='JVT SF', add_underflow=True, doLogY=True, doNorm=True),
    'period_weight' : Plot1D(bin_range=[0, 3], nbins=100, xlabel='Period weight', add_underflow=True, doLogY=True, doNorm=True),
    'lept1Pt' : Plot1D(bin_range=[0,200], bin_width = 5, xlabel='p_{T}^{leading lep}', xunits='GeV')
}


for reg in REGIONS:
    reg.yield_table = deepcopy(YLD_TABLE)
    for var in _vars_to_plot:
        p = deepcopy(plots_defaults[var])
        
        # Set common plot properties
        p.update(reg.name, var)  # sets other plot variables (e.g. name)
        if p.ptype == Types.ratio: 
            p.setRatioPads(p.name)
        elif p.ptype == Types.stack:
            p.setStackPads(p.name)
        elif p.ptype == Types.default:
            p.setRatioPads(p.name)
        else:
            print "WARNING :: %s plots are not yet setup"%p.ptype.name
            continue

        PLOTS.append(p)
print "INFO :: Configuration file loaded\n"
print "="*80
