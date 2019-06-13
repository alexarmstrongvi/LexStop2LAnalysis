#!/bin/usr/env python

import os, sys
import time
import ROOT as r
r.TTree.__init__._creates = False
r.TFile.__init__._creates = False
from copy import deepcopy
from collections import defaultdict
from analysis_DSIDs import DSID_GROUPS

################################################################################
# Globals (not imported)
_work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
_plot_save_dir = '%s/run/plots/' % _work_dir

_apply_sf = False
_sf_samples = False # False -> DF samples
_fake_factor_looper = False

_only1516 = False
_only17 = False
_only18 = False
_only_dilep_trig = False
assert _only1516 + _only17 + _only18 <= 1

# Globals (imported)
EVENT_LIST_DIR = '%s/run/lists/teventlists' % _work_dir
YIELD_TBL_DIR = '%s/run/yields' % _work_dir
SAMPLES = []
REGIONS = []
PLOTS = []
YLD_TABLE = None
if _fake_factor_looper:
    YIELD_TBL = None
    NUM_STR = 'num'
    DEN_STR = 'den'
    PLOTS_DIR = _plot_save_dir

# Lumi values from
if _only1516:
#             2015      2016
    _lumi = (3219.56 + 32988.1) / 1000.0 # inverse fb
elif _only17:
#             2017
    _lumi = 44307.4 / 1000.0 # inverse fb
elif _only18:
#             2018
    _lumi = 59937.2 / 1000.0 # inverse fb
else:
#             2015      2016      2017      2018
    _lumi = (3219.56 + 32988.1 + 44307.4 + 59937.2) / 1000.0 # inverse fb


if _fake_factor_looper:
    _sample_path_base = '%s/run/batch/output/SuperflowAnaStop2l_zjets3l' % _work_dir
    
if _sf_samples:
    _sample_path = '%s/run/batch/output/SuperflowAnaStop2l_SF' % _work_dir
else:
    _sample_path = '%s/run/batch/output/SuperflowAnaStop2l_DF' % _work_dir
    #_sample_path = '%s/run/batch/output/SuperflowAnaStop2l_DF_den' % _work_dir
    #_sample_path = '%s/run/batch/output/SuperflowAnaStop2l_zjets2l_inc' % _work_dir
    #_sample_path = '%s/run/batch/output/SuperflowAnaStop2l_zjets3l' % _work_dir
    #_sample_path = '%s/run/batch/output/SuperflowAnaStop2l_zjets3l_den' % _work_dir

_samples_to_use = [
    'data',
    #'ttbar',
    #'singletop',
    #'ttX',
    #'wjets',
    #'zjets', # Combination: zjets_ee + zjets_mumu + zjets_tautau
    #'zjets_ee',
    #'zjets_mumu',
    #'zjets_tautau',
    #'zgamma',
    #'wgamma',
    #'drellyan',
    #'diboson',
    #'triboson',
    #'higgs',
    #'higgs_ggH', 'higgs_VBF', 'higgs_VH', 'higgs_ttH',
    #'fnp', # Fake non-prompt 
    #'other', # Combination: defined below
    'fnp_fakefactor',
    ############################## 
    #'no_truth',
    'prompt',
    #'LF',
    #'HF',
    #'conversion',
    #'other_fake',
    #'mistagged',
    ############################## 
    # Signal Samples
    #'stop2l_350_185',
    #'stop2l_375_210',
    #'stop2l_425_335',
    #'stop2l_425_275',
    #'stop2l_425_260',
    #'stop2l_450_360',
    #'stop2l_450_330',
    #'stop2l_450_300',
    #'stop2l_450_285',
    #'stop2l_475_385', # Extra Stats
    #'stop2l_475_355', # Extra Stats
    #'stop2l_475_325', # Extra Stats
    #'stop2l_475_310',
    #'stop2l_500_410',
    #'stop2l_500_380',
    #'stop2l_500_350',
    #'stop2l_500_335',
    #'stop2l_550_460',
    #'stop2l_550_430',
    #'stop2l_550_400',
    #'stop2l_550_385',
    #'stop2l_600_510',
    #'stop2l_600_480',
    #'stop2l_600_450',
    #'stop2l_600_435',
]

_regions_to_use = [
    #'no_sel',
    #'trig_only',
    #'presel_DF',
    #'presel_SF',
    
    #'ttbar_CR_loose',
    #'ttbar_CR_loose_elmu', 
    #'ttbar_CR_loose_muel',
    
    #'VV_CR_DF_loose',
    #'VV_CR_DF_loose_elmu',
    #'VV_CR_DF_loose_muel',

    #'VV_CR_SF_loose',
    #'VV_CR_SF_loose_elel',
    #'VV_CR_SF_loose_mumu',

    #'ztautau_CR_loose',
    #'ztautau_CR_loose_elmu', 
    #'ztautau_CR_loose_muel',

    #'zjets_CR_loose', 
    #'zjets_CR_loose_elel',
    #'zjets_CR_loose_mumu',
    
    #'wjets_CR_loose',
    #'wjets_CR_loose_elmu',
    #'wjets_CR_loose_muel',

    #'ttbar_CR',
    #'ttbar_CR_elmu', 
    #'ttbar_CR_muel',
    
    #'VV_CR_DF',
    #'VV_CR_DF_elmu',
    #'VV_CR_DF_muel',

    #'VV_CR_SF',
    #'VV_CR_SF_elel',
    #'VV_CR_SF_mumu',
    
    #'ttbar_VR',
    #'ttbar_VR_elmu', 
    #'ttbar_VR_muel',
    
    #'VV_VR_DF',
    #'VV_VR_DF_elmu',
    #'VV_VR_DF_muel',

    #'VV_VR_SF',
    #'VV_VR_SF_elel',
    #'VV_VR_SF_mumu',
    
    'fnp_VR_DF',
    #'fnp_VR_DF_elmu',
    #'fnp_VR_DF_muel',

    #'mW_DF_pre',
    
    #'mT_DF_pre',
    
    #'mW_SF_pre',
    
    #'mT_SF_pre',

    #'mW_SR',
    #'mW_SR_elmu',
    #'mW_SR_muel',

    #'mT_SR',
    #'mT_SR_elmu',
    #'mT_SR_muel',

    # For fake factor
    #'zjets2l_inc', 
    #'zjets3l_CR_num', 
    #'zjets3l_CR_den',
    #'zjets3l_CR_num_ll_el', 
    #'zjets3l_CR_den_ll_el',
    #'zjets3l_CR_num_ll_mu', 
    #'zjets3l_CR_den_ll_mu',
    #'zjets3l_CR_num_elel_el', 
    #'zjets3l_CR_den_elel_el',
    #'zjets3l_CR_num_elel_mu', 
    #'zjets3l_CR_den_elel_mu',
    #'zjets3l_CR_num_mumu_el', 
    #'zjets3l_CR_den_mumu_el',
    #'zjets3l_CR_num_mumu_mu', 
    #'zjets3l_CR_den_mumu_mu',


]
_region_cfs_to_use = [
    #('ttbar_CR_elmu', 'ttbar_CR_muel')
]
_vars_to_plot = [
    # Event weights and SFs
    #'runNumber',
    #'eventweight',
    #'eventweight_multi',
    #'pupw',
    #'lepSf',
    #'trigSf',
    #'btagSf',
    #'jvtSf',
    #'period_weight',
    #'avgMu',
    #'avgMuDataSF',
    #'nVtx',
    #'actualMu',
    #'actualMuDataSF',
    #'avgMuDataSF/avgMu',
    #'actualMuDataSF/actualMu',
    #'actualMu/avgMu',
    #'actualMuDataSF/avgMuDataSF',
    #'avgMuDataSF/nVtx',

    ## Basic Kinematics
    'lep1Pt',
    #'lep2Pt',
    #'jet1Pt',
    #'jet2Pt',
    #'mll',
    #'fabs(mll-91.2)',
    'met',
    #'metrel',
    #'dpTll',
    #'lep1mT',
    #'lep2mT',
    #'pTll',

    ## Complex kinematics
    #'MT2'

    ## Truth
    #'lep1TruthClass',
    #'lep2TruthClass',
    #'probeLep1TruthClass',

    ## Fake Factor
    'probeLep1Pt',
    'fabs(probeLep1Eta)',
    #'probeLep1Eta',
    #'probeLep1mT',
    #'Z2_mll',
    #'mlll',
    #'dR_ZLep1_probeLep1',
    #'dR_ZLep2_probeLep1',
    #'dR_Z_probeLep1',
    #'probeLep1_d0sigBSCorr',
    #'probeLep1_z0SinTheta',
    #'nInvLeps',
    #'fabs(probeLep1Eta):probeLep1Pt',

    ## Angles
    #'dR_ll',
    #'dphi_ll', 
    #'deta_ll',
    #'deltaPhi_met_lep1',
    #'deltaPhi_met_lep2',
    
    #'lep1_d0sigBSCorr',
    #'lep2_d0sigBSCorr',
    #'lep1_z0SinTheta',
    #'lep2_z0SinTheta',

    ## Multiplicites
    #'nLightJets',
    'nBJets',
    #'nForwardJets',
    'nNonBJets',
    #'nSigLeps',

    # Multi-object
    #'max_HT_pTV_reco',

    ## Super-razor
    #'shat',
    #'pTT_T',
    #'MDR',
    #'RPT',
    #'gamInvRp1',
    #'DPB_vSS',
    #'abs_costheta_b',
    #'DPB_vSS - 0.9*abs_costheta_b',
    #'(DPB_vSS - 1.6) / abs_costheta_b',

    ## 2-Dimensional (y:x)
    #'DPB_vSS:costheta_b'
    #'met:lep1mT',
    #'met:lep2mT',
    #'met:pTll',
    #'dR_ll:lep1mT',
    #'dR_ll:lep2mT',
    #'dR_ll:pTll',
    #'deltaPhi_met_lep1:lep1mT',
    #'deltaPhi_met_lep1:lep2mT',
    #'deltaPhi_met_lep1:pTll',
    #'deltaPhi_met_lep2:lep1mT',
    #'deltaPhi_met_lep2:lep2mT',
    #'deltaPhi_met_lep2:pTll',

    #'dR_ll:lep1mT' 
    #'probeLep1Pt:probeLep1mT',
    #'probeLep1Pt:met',
]
################################################################################
# Make samples

# Initialize
from PlotTools.sample import Sample, Data, MCsample, MCBackground, DataBackground, Signal, color_palette
Sample.input_file_treename = 'superNt'
#HACK correct PRW values 
#     DSID   : MC16A : MC16D : MC16E
_pupw_corr_vals = [
    #("345121", "0.66", "1.00", "0.66"),
    #("345122", "0.67", "1.00", "0.67"),
    #("364105", "1.34", "0.27", "1.34"),
    #("364116", "1.16", "0.95", "0.95"),
    #("364117", "1.01", "1.00", "1.00"),
    #("364123", "1.07", "0.85", "1.07"),
    #("364134", "1.01", "1.00", "1.00"),
    #("364139", "1.01", "1.00", "1.00"),
    #("364156", "1.02", "0.96", "1.02"),
    #("364157", "1.34", "0.27", "1.34"),
    #("364160", "0.41", "1.21", "1.21"),
    #("364161", "1.00", "0.75", "0.75"),
    #("364162", "1.34", "0.27", "1.34"),
    #("364163", "0.77", "0.19", "1.75"),
    #("364164", "1.00", "0.99", "1.00"),
    #("364165", "1.01", "0.30", "1.52"),
    #("364166", "1.18", "0.23", "1.47"),
    #("364167", "1.02", "0.99", "0.99"),
    #("364168", "1.34", "0.27", "1.34"),
    #("364169", "1.34", "0.27", "1.34"),
    #("364170", "0.90", "0.90", "1.13"),
    #("364171", "1.35", "0.27", "1.34"),
    #("364172", "1.21", "0.55", "1.21"),
    #("364174", "1.09", "0.81", "1.09"),
    #("364175", "1.32", "0.66", "1.07"),
    #("364176", "1.34", "0.27", "1.34"),
    #("364177", "0.84", "1.06", "1.05"),
    #("364179", "0.90", "0.90", "1.13"),
    #("364180", "1.34", "0.27", "1.34"),
    #("364181", "1.06", "0.86", "1.08"),
    #("364182", "1.18", "0.24", "1.47"),
    #("364183", "1.25", "0.70", "1.25"),
    #("364185", "1.01", "1.00", "1.00"),
    #("364189", "1.06", "0.84", "1.06"),
    #("364190", "2.00", "0.20", "1.00"),
    #("364191", "1.72", "0.23", "1.14"),
    #("364192", "1.08", "0.85", "1.06"),
    #("364193", "1.33", "0.89", "0.89"),
    #("364194", "1.22", "0.81", "1.01"),
    #("364196", "1.35", "0.27", "1.34"),
    #("345706", "1.46", "0.73", "0.73"),
    #("364200", "1.01", "1.00", "1.00"),
    #("364208", "1.27", "0.64", "1.27"),
    #("364210", "0.86", "1.04", "1.06"),
    #("364211", "0.99", "1.00", "1.00"),
    #("364255", "1.27", "0.64", "1.27"),
    #("346345", "0.25", "1.00", "1.00"),
    #("364286", "1.47", "0.73", "0.73"),
    ("410472", "1.10", "0.61", "1.10"),
]
pupw_corr = []
for dsid, ca, cd, ce in _pupw_corr_vals:
    pairs = [
            ('(treatAsYear == 2015 || treatAsYear == 2016)', ca),
            ('treatAsYear == 2017', cd),
            ('treatAsYear == 2018', ce),
            ]
    corrections = []
    for year_sel, corr in pairs:
        s = '(isMC && dsid == %s && %s)' % (dsid, year_sel)
        c = '((%s * %s) + !%s)' % (corr, s, s)
        corrections.append(c)
    pupw_corr.append(" * ".join(corrections))
pupw_corr = "(%s)" % (" * ".join(pupw_corr))

if _only1516 or _only17 or _only18:
    # Single campaign w/ PRW
    MCsample.weight_str = 'eventweight_single'

    # Single campaign w/o PRW
    #MCsample.weight_str = 'eventweight / pupw' 
else:
    # Multi-campaign w/ PRW
    MCsample.weight_str = 'eventweight_multi * %s' % pupw_corr

    # Multi-campaign w/o PRW
    #MCsample.weight_str = 'eventweight_multi / pupw'

MCsample.scale_factor = _lumi

# Build combined DSID groups
if _only1516:
    DSID_GROUPS['data'] = DSID_GROUPS['data15'] + DSID_GROUPS['data16']
elif _only17:
    DSID_GROUPS['data'] = DSID_GROUPS['data17']
elif _only18:
    DSID_GROUPS['data'] = DSID_GROUPS['data18']
else:
    DSID_GROUPS['data'] = (
            DSID_GROUPS['data15']
          + DSID_GROUPS['data16']
          + DSID_GROUPS['data17']
          + DSID_GROUPS['data18']
          )

DSID_GROUPS['ttbar'] = (
        #DSID_GROUPS['ttbar_dilep']
        DSID_GROUPS['ttbar_nonallhad']
      )
DSID_GROUPS['Wt'] = (
        #DSID_GROUPS['Wt_dilep']
        DSID_GROUPS['Wt_incl']
      )
DSID_GROUPS['singletop'] = (
        DSID_GROUPS['st-channel']
      + DSID_GROUPS['tZ']
      + DSID_GROUPS['Wt']
      )
DSID_GROUPS['ttX'] = (
        DSID_GROUPS['ttgamma']
      + DSID_GROUPS['ttVV']
      + DSID_GROUPS['ttV']
      + DSID_GROUPS['4topSM']
      )
DSID_GROUPS['diboson'] = (
        DSID_GROUPS['diboson_1l']
      + DSID_GROUPS['diboson_2l']
      + DSID_GROUPS['diboson_3l']
      + DSID_GROUPS['diboson_4l']
      )
DSID_GROUPS['zjets'] = (
        DSID_GROUPS['zjets_tautau']
      + DSID_GROUPS['zjets_mumu']
      + DSID_GROUPS['zjets_ee']
      )
DSID_GROUPS['higgs'] = (
        DSID_GROUPS['higgs_ggH']
      + DSID_GROUPS['higgs_VBF']
      + DSID_GROUPS['higgs_ttH']
      + DSID_GROUPS['higgs_VH']
      )
DSID_GROUPS['other'] = (
        DSID_GROUPS['triboson']
      + DSID_GROUPS['drellyan']
      + DSID_GROUPS['higgs']
      )
DSID_GROUPS['fnp'] = DSID_GROUPS['wjets']
DSID_GROUPS['Wt'] = DSID_GROUPS['Wt_dilep']
DSID_GROUPS['MC'] = (
        DSID_GROUPS['zjets']
      + DSID_GROUPS['ttbar']
      #+ DSID_GROUPS['Wt']
      #+ DSID_GROUPS['singletop']
      #+ DSID_GROUPS['ttX']
      + DSID_GROUPS['diboson'] 
      #+ DSID_GROUPS['higgs']
      + DSID_GROUPS['zgamma']
        )
DSID_GROUPS['fnp_fakefactor'] = DSID_GROUPS['data'] + DSID_GROUPS['MC']
DSID_GROUPS['no_truth'] = DSID_GROUPS['MC']
DSID_GROUPS['prompt'] = DSID_GROUPS['MC']
DSID_GROUPS['LF'] = DSID_GROUPS['MC']
DSID_GROUPS['HF'] = DSID_GROUPS['MC']
DSID_GROUPS['conversion'] = DSID_GROUPS['MC']
DSID_GROUPS['other_fake'] = DSID_GROUPS['MC']
DSID_GROUPS['mistagged'] = DSID_GROUPS['MC']

# Setup samples
data = Data('data','Data')
data.color = r.kBlack
SAMPLES.append(data)

# Top Samples
ttbar = MCBackground('ttbar',"t#bar{t}","$t\\bar{t}$")
#ttbar.color = r.TColor.GetColor("#FFFF04")
ttbar.color = color_palette['red'][0] 
#sf = 0.91 if _only1516 else 0.94 if _only17 else 0.94 if _only18 else 0.94
sf = 0.89 if _only1516 else 1.00 if _only17 else 1.00 if _only18 else 0.91
if _apply_sf:
    ttbar.scale_factor *= sf
SAMPLES.append(ttbar)

#Wt = MCBackground('Wt')
#Wt.color = r.TColor.GetColor("#FF0201")
#SAMPLES.append(Wt)

singletop = MCBackground('singletop', "Single top")
singletop.color = color_palette['red'][1] 
SAMPLES.append(singletop)

#ttV = MCBackground('ttV','t#bar{t}+V','$t\\bar{t}+V$')
#ttV.color = r.TColor.GetColor("#0400CC")
#SAMPLES.append(ttV)

ttX = MCBackground('ttX','t#bar{t}+X','$t\\bar{t}+X$')
ttX.color = color_palette['red'][2] 
SAMPLES.append(ttX)

# V+jets
wjets = MCBackground('wjets','W+jets')
wjets.color = color_palette['yellow'][4]
SAMPLES.append(wjets)

zjets = MCBackground('zjets','Z/#gamma*+jets','$Z/\gamma^{*}$+jets')
#zjets.color = r.TColor.GetColor("#33FF03")
zjets.color = color_palette['blue'][0]
#zjets.isSignal = True
SAMPLES.append(zjets)

zjets_ee = MCBackground('zjets_ee','Z/#gamma*(#rightarrowee)+jets','$Z/\gamma^{*}(\\rightarrow ee)$+jets')
zjets_ee.color = color_palette['blue'][0]
SAMPLES.append(zjets_ee)

zjets_mumu = MCBackground('zjets_mumu','Z/#gamma*(#rightarrow#mu#mu)+jets','$Z/\gamma^{*}(\\rightarrow \mu\mu)$+jets')
zjets_mumu.color = color_palette['blue'][1]
SAMPLES.append(zjets_mumu)

zjets_tautau = MCBackground('zjets_tautau','Z/#gamma*(#rightarrow#tau#tau)+jets','$Z/\gamma^{*}(\\rightarrow \\tau\\tau)$+jets')
zjets_tautau.color = color_palette['blue'][2]
SAMPLES.append(zjets_tautau)

zgamma = MCBackground('zgamma','Z+#gamma','$Z+\gamma$')
zgamma.color = color_palette['cyan'][0]
SAMPLES.append(zgamma)

wgamma = MCBackground('wgamma','W+#gamma','$W+\gamma$')
wgamma.color = color_palette['cyan'][1]
SAMPLES.append(wgamma)

DY = MCBackground('drellyan','Drell-Yan')
DY.color = color_palette['blue'][3]
SAMPLES.append(DY)

# Multi-boson
diboson = MCBackground('diboson','VV')
#diboson.color = r.TColor.GetColor("#33CCFF")
diboson.color = color_palette['green'][0]

diboson_1l = MCBackground('diboson_1l','VV (1l)', '$VV (1\ell)$')
diboson_1l.color = color_palette['green'][0]
diboson_2l = MCBackground('diboson_2l','VV (2l)', '$VV (2\ell)$')
diboson_2l.color = color_palette['green'][1]
diboson_3l = MCBackground('diboson_3l','VV (3l)', '$VV (3\ell)$')
diboson_3l.color = color_palette['green'][2]
diboson_4l = MCBackground('diboson_4l','VV (4l)', '$VV (4\ell)$')
diboson_4l.color = color_palette['green'][3]

if _sf_samples:
    sf = 1 if _only1516 else 1.13 if _only17 else 0.84 if _only18 else 1.06
else:
    sf = 1.26 if _only1516 else 1 if _only17 else 1 if _only18 else 1.21
if _apply_sf:
    diboson.scale_factor *= sf
    diboson_1l.scale_factor *= sf
    diboson_2l.scale_factor *= sf
    diboson_3l.scale_factor *= sf
    diboson_4l.scale_factor *= sf
SAMPLES.append(diboson)
SAMPLES.append(diboson_1l)
SAMPLES.append(diboson_2l)
SAMPLES.append(diboson_3l)
SAMPLES.append(diboson_4l)

triboson = MCBackground('triboson','VVV')
triboson.color = color_palette['green'][4]
SAMPLES.append(triboson)

# Higgs
higgs = MCBackground('higgs', 'Higgs')
#higgs.color = r.TColor.GetColor("#009933")
higgs.color = color_palette['yellow'][0]
SAMPLES.append(higgs)

higgs_ggH = MCBackground('higgs_ggH', 'ggH')
higgs_ggH.color = color_palette['yellow'][0]
SAMPLES.append(higgs_ggH)

higgs_VBF = MCBackground('higgs_VBF', 'Higgs VBF')
higgs_VBF.color = color_palette['yellow'][1]
SAMPLES.append(higgs_VBF)

higgs_ttH = MCBackground('higgs_ttH', 'Higgs ttH')
higgs_ttH.color = color_palette['yellow'][2]
SAMPLES.append(higgs_ttH)

higgs_VH = MCBackground('higgs_VH', 'Higgs VH')
higgs_VH.color = color_palette['yellow'][3]
SAMPLES.append(higgs_VH)

# MC truth fake samples
truth_lep1_prompt = '(lep1TruthClass == 1 || lep1TruthClass == 2)'
truth_lep1_fake = '(lep1TruthClass < 1 || 2 < lep1TruthClass)'
truth_lep2_prompt = '(lep2TruthClass == 1 || lep2TruthClass == 2)'
truth_lep2_fake = '(lep2TruthClass < 1 || 2 < lep2TruthClass)'
truth_probe1_prompt = '(probeLep1TruthClass == 1 || probeLep1TruthClass == 2)'
truth_probe1_fake = '(probeLep1TruthClass < 1 || 2 < probeLep1TruthClass)'

probe_is_no_truth = 'probeLep1TruthClass < 1'
probe_is_prompt = truth_probe1_prompt
probe_is_LF = 'probeLep1TruthClass == 5'
probe_is_HF = '(probeLep1TruthClass == 8 || probeLep1TruthClass == 9)'
probe_is_conversion = '(probeLep1TruthClass == 4 || probeLep1TruthClass == 10)'
probe_is_other_fake = '(probeLep1TruthClass == 3 || probeLep1TruthClass == 6 || probeLep1TruthClass == 7)'
if any('zjets3l' in r for r in _regions_to_use):
    tag_is_prompt = truth_lep1_prompt + " && " + truth_lep2_prompt
else:
    tag_is_prompt = truth_lep1_prompt

no_truth = MCBackground('no_truth', "No truth info")
no_truth.color = color_palette['gray'][0]
no_truth.cut = tag_is_prompt + " && " + probe_is_no_truth
SAMPLES.append(no_truth)

prompt = MCBackground('prompt', "Prompt")
prompt.color = color_palette['green'][0]
prompt.cut = tag_is_prompt + " && " + probe_is_prompt
SAMPLES.append(prompt)

LF = MCBackground('LF', "Light flavor")
LF.color = color_palette['blue'][0]
LF.cut = tag_is_prompt + " && " + probe_is_LF
SAMPLES.append(LF)

HF = MCBackground('HF', "Heavy flavor")
HF.color = color_palette['red'][0]
HF.cut = tag_is_prompt + " && " + probe_is_HF
SAMPLES.append(HF)

conversion = MCBackground('conversion', "Conversion")
conversion.color = color_palette['yellow'][0]
conversion.cut = tag_is_prompt + " && " + probe_is_conversion
SAMPLES.append(conversion)

other_fake = MCBackground('other_fake', "Other")
other_fake.color = color_palette['cyan'][0]
other_fake.cut = tag_is_prompt + " && " + probe_is_other_fake
SAMPLES.append(other_fake)

mistagged = MCBackground('mistagged', "Mistagged")
mistagged.color = color_palette['orange'][0]
mistagged.cut = "!(%s)" % tag_is_prompt 
SAMPLES.append(mistagged)

# Other
fnp = MCBackground('fnp','FNP (Wjets MC)')
fnp.color = r.TColor.GetColor("#FF8F02")
SAMPLES.append(fnp)

other = MCBackground('other','Other (VVV+DY+Higgs)')
other.color = r.TColor.GetColor("#009933")
SAMPLES.append(other)

fnp_fakefactor = DataBackground('fnp_fakefactor','FNP (Data)')
fnp_fakefactor.color = color_palette['gray'][0]
fnp_fakefactor.weight_str = "(((!isMC) + (isMC * %s * %f)) * fakeFactorInfo.fakeweight)" % (MCsample.weight_str, MCsample.scale_factor)
SAMPLES.append(fnp_fakefactor)

# Signal
sig_samples = [(k, v[0]) for k, v in DSID_GROUPS.items() if 'stop2l' in k and k in _samples_to_use]
sig_samples = sorted(sig_samples, key = lambda x : x[1])
for ii, (name, dsid) in enumerate(sig_samples, 1):
    m_stop = name.split('_')[1]
    m_chi = name.split('_')[2]
    signal = Signal(name, 
                '#tilde{t}_{1}#tilde{t}_{1}, m(#tilde{t}_{1},#tilde{#chi}_{1}^{0}) = (%s, %s)GeV' % (m_stop, m_chi),
                '$\\tilde{t}_{1}\\tilde{t}_{1}, m(\\tilde{t}_{1},\\tilde{\chi}_{1}^{0}) = (%s, %s)$GeV' % (m_stop, m_chi)
    )
    step_size = (16**2)/len(sig_samples)
    start = (16**2)%len(sig_samples) - 1
    hex_col = format(start + ii*step_size, 'X')
    hex_col = "#%s0000" % hex_col.zfill(2)
    signal.color = r.TColor.GetColor(hex_col)
    signal.scale_factor *= 1
    # Dynamic class members
    signal.mass_x = m_stop
    signal.mass_y = m_chi
    SAMPLES.append(signal)

# Setup the samples
tmp_list = []
for s in SAMPLES:
    if s.name not in _samples_to_use: continue
    # Assume sample name is the same as the DSID group key
    if s.isMC:
        if _only1516:
            checklist = ['mc16a']
            search_str = ['mc16a']
            exclude_str = []
        elif _only17:
            checklist = ['mc16d']
            search_str = ['mc16d']
            exclude_str = []
        elif _only18:
            checklist = ['mc16e']
            search_str = ['mc16e']
            exclude_str = []
        else:
            checklist = ['mc16a','mc16d','mc16e']
            search_str = ['mc16']
            exclude_str = []
    else:
        checklist = []
        search_str = []
        exclude_str = ['mc16']

    if _fake_factor_looper:
        for n_d in [NUM_STR, DEN_STR]:
            if n_d == NUM_STR:
                sample_path = _sample_path_base
            elif n_d == DEN_STR:
                sample_path = _sample_path_base + '_' + DEN_STR
            s_tmp = deepcopy(s)
            s_tmp.name += "_" + n_d
            s_tmp.displayname += " (%s)" % n_d
            s_tmp.latexname += " (%s)" % n_d
            s_tmp.set_chain_from_dsid_list(DSID_GROUPS[s.name], sample_path, checklist, search_str)
            tmp_list.append(s_tmp)
    else:
        if s.name == 'fnp_fakefactor':
            print "TMP :: Hack for FNP samples"
            sample_path = '%s/run/batch/output/SuperflowAnaStop2l_DF_den' % _work_dir
            exclude_str = []
            s.set_chain_from_dsid_list(DSID_GROUPS[s.name], sample_path, checklist, search_str, exclude_str)
        else:
            s.set_chain_from_dsid_list(DSID_GROUPS[s.name], _sample_path, checklist, search_str)
        tmp_list.append(s)
SAMPLES = tmp_list

# Remove samples not properly setup
print "INFO :: Checking which files are setup"
tmp_list = []
for s in SAMPLES:
    start = time.time()
    print "INFO :: \tChecking if %s is setup..." % s.name,
    sys.stdout.flush()
    if s.is_setup():
        tmp_list.append(s)
        print "Pass",
    else:
        print "Fail",
    end = time.time()
    dur = end - start
    print " (%.2fsec)" % dur
SAMPLES = tmp_list

# Check for at least 1 sample
assert SAMPLES, "ERROR :: No samples are setup"

################################################################################
# Make Regions
print "INFO :: Building regions"
from PlotTools.region import Region

########################################
# Define common selections
elel = 'isElEl' 
elmu = 'isElMu' 
muel = 'isMuEl' 
mumu = 'isMuMu' 
DF = 'isDF'
SF = '!isDF'
OS = 'isOS'
SS = '!isOS'

triggers = defaultdict(dict)
triggers['15']['e']    = '(HLT_e24_lhmedium_L1EM20VH || HLT_e60_lhmedium || HLT_e120_lhloose)'
triggers['15']['ee']   = '(HLT_2e12_lhloose_L12EM10VH)'
triggers['15']['mu']   = '(HLT_mu20_iloose_L1MU15 || HLT_mu40)'
triggers['15']['mumu'] = '(HLT_mu18_mu8noL1)'
triggers['15']['emu']  = '(HLT_e17_lhloose_mu14)'
triggers['15']['mue']  = '(HLT_e7_lhmedium_mu24)'

triggers['16']['e']    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
triggers['16']['ee']   = '(HLT_2e17_lhvloose_nod0)'
triggers['16']['mu']   = '(HLT_mu26_ivarmedium || HLT_mu50)'
triggers['16']['mumu'] = '(HLT_mu22_mu8noL1)'# || HLT_2mu14)'
triggers['16']['emu']  = '(HLT_e17_lhloose_nod0_mu14)'# || HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1)'
triggers['16']['mue']  = '(HLT_e7_lhmedium_nod0_mu24)'

triggers['17']['e']    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
triggers['17']['ee']   = '(HLT_2e24_lhvloose_nod0)'
triggers['17']['mu']   = '(HLT_mu26_ivarmedium || HLT_mu50)'
triggers['17']['mumu'] = '(HLT_mu22_mu8noL1)'# || HLT_2mu14)'
triggers['17']['emu']  = '(HLT_e17_lhloose_nod0_mu14 || HLT_e26_lhmedium_nod0_mu8noL1)'
triggers['17']['mue']  = '(HLT_e7_lhmedium_nod0_mu24)'

triggers['18']['e']    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
triggers['18']['ee']   = '(HLT_2e24_lhvloose_nod0 || HLT_2e17_lhvloose_nod0_L12EM15VHI)'
triggers['18']['mu']   = '(HLT_mu26_ivarmedium || HLT_mu50)'
triggers['18']['mumu'] = '(HLT_mu22_mu8noL1)'# || HLT_2mu14)'
triggers['18']['emu']  = '(HLT_e17_lhloose_nod0_mu14 || HLT_e26_lhmedium_nod0_mu8noL1)'
triggers['18']['mue']  = '(HLT_e7_lhmedium_nod0_mu24)'

def join_trig(*trigs):
    return '(%s)' % (' || '.join(trigs))

# Build final trigger strategy for each year
for year, tr in triggers.iteritems():
    triggers[year]['onelep'] = join_trig(tr['e'], tr['mu'])
    triggers[year]['dilep'] = join_trig(tr['ee'], tr['mumu'], tr['emu'], tr['mue'])
    if _only_dilep_trig:
        triggers[year]['final'] = triggers[year]['dilep']
    else:
        triggers[year]['final'] = join_trig(triggers[year]['onelep'], triggers[year]['dilep'])
    triggers[year]['final'] = '(treatAsYear == 20%s && %s)' % (year, triggers[year]['final'])
    
# Combine trigger strategy for each year
if _only1516:
    trigger_sel = join_trig(triggers['15']['final'],triggers['16']['final'])
elif _only17:
    trigger_sel = triggers['17']['final']
elif _only18:
    trigger_sel = triggers['18']['final']
else:
    trigger_sel = join_trig(triggers['15']['final'],triggers['16']['final'],
                            triggers['17']['final'],triggers['18']['final'])

#TESTING - overwrite old trigger option
#if _only_dilep_trig:
#    trigger_sel = 'passDilepTrigs'
#elif false: # not defined yet
#    trigger_sel = 'passSingleLepTrigs'
#else:
#    trigger_sel = 'passLepTrigs'

def add_DF_channels(reg, REGs):
    elmu_channel = reg.build_channel('elmu', 'e#mu', '$e\\mu$', cuts=elmu)
    reg.compare_regions.append(elmu_channel)
    REGs.append(elmu_channel)

    muel_channel = reg.build_channel('muel', '#mue', '$\\mu e$', cuts=muel)
    reg.compare_regions.append(muel_channel)
    REGs.append(muel_channel)

def add_SF_channels(reg, REGs):
    elel_channel = reg.build_channel('elel', 'ee', '$ee$', cuts=elel)
    reg.compare_regions.append(elel_channel)
    REGs.append(elel_channel)

    mumu_channel = reg.build_channel('mumu', '#mu#mu', '$\\mu \\mu$', cuts=mumu)
    reg.compare_regions.append(mumu_channel)
    REGs.append(mumu_channel)


baseline_truth_num = "%s && %s" % (truth_lep1_prompt, truth_probe1_prompt)
baseline_truth_den = "%s && %s" % (truth_lep1_prompt, truth_probe1_fake)
zjets_truth_num = "%s && %s && %s" % (truth_lep1_prompt, truth_lep2_prompt, truth_probe1_prompt)
zjets_truth_den = "%s && %s && %s" % (truth_lep1_prompt, truth_lep2_prompt, truth_probe1_fake)

########################################
# No selection
region = Region('no_sel','No Selection')
region.tcut = '1'
REGIONS.append(region)

region = Region('trig_only','Trigger Only')
region.tcut = trigger_sel
REGIONS.append(region)

region = Region('presel_DF','DF Preselection')
region.tcut = trigger_sel
region.tcut += ' && ' + DF
region.tcut += ' && lep2Pt > 20'
REGIONS.append(region)

region = Region('presel_SF','SF Preselection')
region.tcut = trigger_sel
region.tcut += ' && ' + SF
region.tcut += ' && lep2Pt > 20'
REGIONS.append(region)

# Loose control regions
region = Region('ttbar_CR_loose', 'Loose t#bar{t} CR', 'Loose $t\\bar{t}$ CR')
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 2'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_DF_loose', 'Loose VV-DF CR')
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0'
region.tcut += ' && nNonBJets < 2'
region.tcut += ' && dR_ll < 2.8'
region.tcut += ' && lep1mT > 50'
region.tcut += ' && lep2mT > 30'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_SF_loose', 'Loose VV-SF CR')
region.tcut = trigger_sel
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && (fabs(mll - 91.2) > 10)' # Remove Z->ee and Z->mumu
region.tcut += ' && nBJets == 0'
region.tcut += ' && nNonBJets < 2'
region.tcut += ' && metrel > 60'
region.tcut += ' && pTll > 60'
REGIONS.append(region)
add_SF_channels(region, REGIONS)

region = Region('ztautau_CR_loose', "Loose Z/#gamma*(#rightarrow#tau#tau)+jets CR", "Loose $Z/\gamma^{*}(\\rightarrow \\tau\\tau)$+jets CR")
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && (30 < mll && mll < 60)' # Remove Z->ee and Z->mumu
region.tcut += ' && nLightJets > 0' # Remove W+jets
region.tcut += ' && 20 < met && met < 80' # Select Z->tautau
region.tcut += ' && (20 < lep1Pt && lep1Pt < 50.0)' # Select Z->tautau
region.tcut += ' && dR_ll > 2.0' # Select Z->tautau
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('zjets_CR_loose', 'Loose Z/#gamma*(#rightarrowll)+jets','Loose $Z/\gamma^{*}(\\rightarrow \ell\ell)$+jets')
region.tcut = trigger_sel
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && nLightJets > 0' # Remove W+jets
region.tcut += ' && (fabs(mll - 91.2) < 10)' # Select Z->ee and Z->mumu
REGIONS.append(region)
add_SF_channels(region, REGIONS)

region = Region('wjets_CR_loose', 'Loose W+jets CR')
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && (mll < 60 || 120 < mll)' # Remove Z+jets
region.tcut += ' && lep1mT > 50' # Select lep1 from W decay
REGIONS.append(region)
add_DF_channels(region, REGIONS)

########################################
# Baseline
base_selection = trigger_sel
base_selection += ' && lep1Pt > 25'
base_selection += ' && lep2Pt > 20'

########################################
# Control regions
region = Region('ttbar_CR', 't#bar{t} CR', '$t\\bar{t}$ CR')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets > 0'
region.tcut += ' && MDR > 80'
region.tcut += ' && RPT > 0.7'
region.tcut += ' && (DPB_vSS < 0.9 * abs_costheta_b + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_DF', 'Diboson CR (DF)', 'Diboson CR (DF)')
region.tcut = base_selection
region.tcut += ' && nBJets == 0'
region.tcut += ' && MDR > 50'
region.tcut += ' && RPT < 0.5'
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS < 0.9 * abs_costheta_b + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_SF', 'Diboson CR (SF)', 'Diboson CR (SF)')
region.tcut = base_selection
region.tcut += ' && fabs(mll - 91.2) > 10'
region.tcut += ' && nBJets == 0'
region.tcut += ' && MDR > 70'
region.tcut += ' && RPT < 0.5'
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS < (0.9 * abs_costheta_b + 1.6))'
REGIONS.append(region)
add_SF_channels(region, REGIONS)

########################################
# Validation regions
region = Region('ttbar_VR', 't#bar{t} VR', '$t\\bar{t}$ VR')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0' # orthogonal to CR
region.tcut += ' && MDR > 80'
region.tcut += ' && RPT < 0.7' # closer to SR
region.tcut += ' && (DPB_vSS > 0.9 * abs_costheta_b + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_VR_DF', 'Diboson VR (DF)', 'Diboson VR (DF)')
region.tcut = base_selection
region.tcut += ' && nBJets == 0'
region.tcut += ' && 50 < MDR && MDR < 95' # tighter than CR
region.tcut += ' && RPT < 0.7' # looser than CR
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS > 0.9 * abs_costheta_b + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_VR_SF', 'Diboson VR (SF)', 'Diboson VR (SF)')
region.tcut = base_selection
region.tcut += ' && fabs(mll - 91.2) > 10'
region.tcut += ' && nBJets == 0'
region.tcut += ' && 60 < MDR && MDR < 95' #tighter than CR
region.tcut += ' && RPT < 0.4' # tighter than CR
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS > 0.9 * abs_costheta_b + 1.6)'
REGIONS.append(region)
add_SF_channels(region, REGIONS)

region = Region('fnp_VR_DF', 'FNP VR (DF)')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && RPT > 0.7'  # Same as SR
region.tcut += ' && gamInvRp1 > 0.7'  # Same as SR
#region.tcut += ' && DPB_vSS > 0.9 * abs_costheta_b + 1.6' # Same as SR
region.tcut += ' && nBJets == 0' # Same as SR
region.tcut += ' && MDR < 40'
REGIONS.append(region)
#add_DF_channels(region, REGIONS)

########################################
# Signal region preselection
region = Region('mW_DF_pre','DF Preselection + b-veto')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0'
REGIONS.append(region)

region = Region('mT_DF_pre','DF Preselection + >0 b-jets')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets > 0'
REGIONS.append(region)

region = Region('mW_SF_pre','SF Preselection + b-veto')
region.tcut = base_selection
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && nBJets == 0'
region.tcut += ' && fabs(mll - 91.2) > 10'
REGIONS.append(region)

region = Region('mT_SF_pre','SF Preselection + >0 b-jets')
region.tcut = base_selection
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && nBJets > 0'
region.tcut += ' && fabs(mll - 91.2) > 10'
REGIONS.append(region)

########################################
# Signal regions
signal_selection = base_selection
signal_selection += ' && RPT > 0.7'
signal_selection += ' && gamInvRp1 > 0.7'
signal_selection += ' && DPB_vSS > 0.9 * abs_costheta_b + 1.6'
if _sf_samples:
    signal_selection += ' && fabs(mll - 91.2) > 20'
signal_selection += ' && (!' + truth_lep1_prompt + " || !" + truth_lep2_prompt + ')' 

region = Region('mW_SR', 'SR (#Deltam ~ m_{W})', 'SR ($\Delta m \sim m_{W}$)')
region.isSR = True
region.tcut = signal_selection
region.tcut += ' && nBJets == 0'
region.tcut += ' && MDR > 95'

REGIONS.append(region)
if _sf_samples:
    add_SF_channels(region, REGIONS)
else:
    add_DF_channels(region, REGIONS)

region = Region('mT_SR', 'SR (#Deltam ~ m_{T})', 'SR ($\Delta m \sim m_{T}$)')
region.isSR = True
region.tcut = signal_selection
region.tcut += ' && nBJets > 0'
region.tcut += ' && MDR > 110'
REGIONS.append(region)
if _sf_samples:
    add_SF_channels(region, REGIONS)
else:
    add_DF_channels(region, REGIONS)

########################################
region = Region('zjets2l_inc', 'Z+jets (#ge2 lep)','$Z$+jets (\ge2 $\\ell_\\text{ID}$)')
region.tcut = trigger_sel
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && lep1Pt > 25'
region.tcut += ' && lep2Pt > 20'
region.tcut += ' && (fabs(mll - 91.2) < 10)'
#region.tcut += ' && met < 40'
REGIONS.append(region)

# Fake factor regions
zjets3l_sel = trigger_sel
zjets3l_sel += ' && ' + SF + ' && ' + OS
zjets3l_sel += ' && lep1Pt > 25'
zjets3l_sel += ' && lep2Pt > 20'
zjets3l_sel += ' && (fabs(mll - 91.2) < 10)'
zjets3l_sel += ' && probeLep1mT < 40'
zjets3l_sel += ' && (probeLep1Pt < 16 || met < 50)'
#zjets3l_sel += ' && probeLep1Pt < 8'
#zjets3l_sel += ' && (fabs(Z2_mll - 91.2) > 10)'
zjets3l_sel += ' && dR_ZLep1_probeLep1 > 0.2 && dR_ZLep2_probeLep1 > 0.2' 
if "fnp_fakefactor" in _samples_to_use:
    zjets3l_sel += ' && (!isMC || %s)' % zjets_truth_num
    zjets3l_sel_num = zjets3l_sel
    zjets3l_sel_den = zjets3l_sel
else:
    zjets3l_sel_num = zjets3l_sel + " && nSigLeps == 3 && nInvLeps == 0"
    zjets3l_sel_den = zjets3l_sel + " && nSigLeps == 2 && nInvLeps == 1"

def add_zjets3L_channels(reg, REGs):
    # Define selections
    ll_el_sel = 'probeLep1Flav == 0'
    ll_mu_sel = 'probeLep1Flav == 1'
    elel_el_sel = 'lep1Flav == 0 && lep2Flav == 0 && probeLep1Flav == 0'
    mumu_el_sel = 'lep1Flav == 1 && lep2Flav == 1 && probeLep1Flav == 0'
    elel_mu_sel = 'lep1Flav == 0 && lep2Flav == 0 && probeLep1Flav == 1'
    mumu_mu_sel = 'lep1Flav == 1 && lep2Flav == 1 && probeLep1Flav == 1'
    
    # Create channels
    ch = reg.build_channel('ll_el', 'll+e', '$\\ell\\ell+e$',cuts=ll_el_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)
    ch = reg.build_channel('ll_mu', 'll+#mu', '$\ell\ell+\mu$',cuts=ll_mu_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)
    ch = reg.build_channel('elel_el', 'ee+e', '$ee+e$',cuts=elel_el_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)
    ch = reg.build_channel('mumu_el', '#mu#mu+e', '$\mu\mu+e$',cuts=mumu_el_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)
    ch = reg.build_channel('elel_mu', 'ee+#mu', '$ee+\mu$',cuts=elel_mu_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)
    ch = reg.build_channel('mumu_mu', '#mu#mu+#mu', '$\mu\mu+\mu$',cuts=mumu_mu_sel)
    reg.compare_regions.append(ch)
    REGs.append(ch)


region = Region('zjets3l_CR_num', 'Z+jets CR (3 ID)','$Z$+jets CR (3 $\\ell_\\text{ID}$)')
region.tcut = zjets3l_sel_num
REGIONS.append(region)
add_zjets3L_channels(region, REGIONS)

region = Region('zjets3l_CR_den', 'Z+jets CR (2 ID + 1 #bar{ID})','$Z$+jets CR (2 $\\ell_\\text{ID}$ + 1 $\\ell_{\\bar{\\text{ID}}}$)')
region.tcut = zjets3l_sel_den
REGIONS.append(region)
add_zjets3L_channels(region, REGIONS)

########################################
# Check all requested regions correspond to one defined region
all_reg_names = [r.name for r in REGIONS]
if len(all_reg_names) > len(set(all_reg_names)):
    print "ERROR :: Duplicate region names:", all_reg_names

for reg_name in _regions_to_use:
    if reg_name not in all_reg_names:
        print 'ERROR :: Region %s not defined: %s' % (reg_name, all_reg_names)

# Build final list of regions for looper
tmp_list = []
for reg in REGIONS:
    if reg.name not in _regions_to_use: continue
    if 'zjets3l' in reg.name:
        reg.truth_fake_sel = zjets_truth_den
        reg.truth_bkg_sel = zjets_truth_num
    else:
        reg.truth_fake_sel = baseline_truth_den
        reg.truth_bkg_sel = baseline_truth_num
    tmp_list.append(reg)
        
REGIONS = tmp_list

# Define regions to be compared in region comparison looper
for reg_name in {x for y in _region_cfs_to_use for x in y}:
    if reg_name not in all_reg_names:
        print 'ERROR :: Region %s not defined: %s' % (reg_name, all_reg_names)
REGION_TUPLES = []
for reg_tup in _region_cfs_to_use:
    tmp = tuple(reg for reg in REGIONS if reg.name in reg_tup)
    REGION_TUPLES.append(tmp)

################################################################################
# Yield Table
print "INFO :: Yield tables"

if _fake_factor_looper:
    from PlotTools.YieldTable import YieldTable
    YIELD_TBL = YieldTable()

from PlotTools.YieldTable import YieldTbl
YieldTbl.write_to_latex = True
YLD_TABLE = YieldTbl()
YLD_TABLE.add_row_formula(name="mc_total",displayname="MC Total", formula="MC")
YLD_TABLE.add_row_formula(name="data_mc_ratio",displayname="Data/MC", formula="data/MC")

for reg in REGIONS:
    reg.yield_table = deepcopy(YLD_TABLE)
    if 'ttbar' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='ttbar/MC')
        if 'CR' in reg.name:
            reg.yield_table.add_row_formula(name='normf', displayname='Norm Factor', formula='(data - (MC - ttbar))/ttbar')
    elif 'zll' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='zjets/MC')
    elif 'ztautau' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='zjets_tautau/MC')
    elif 'wjets' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='wjets/MC')
    elif 'VV' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='diboson/MC')
        if 'CR' in reg.name:
            reg.yield_table.add_row_formula(name='normf', displayname='Norm Factor', formula='(data - (MC - diboson))/diboson')
    elif 'SR' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='signal/MC')
        reg.yield_table.add_row_formula(name='ttbar_contamination', displayname='t#bar{t}/MC', latexname='$t\\bar{t}$/MC', formula='ttbar/MC')
        reg.yield_table.add_row_formula(name='vv_contamination', displayname='VV/MC', formula='diboson/MC')
    elif 'zjets3l' in reg.name:
        reg.yield_table.add_row_formula(name='vv_contamination', displayname='VV/MC', formula='diboson/MC')
        #reg.yield_table.add_row_formula(name='vv_contamination2', displayname='VV/Data', formula='diboson/data')


################################################################################
# Make Plots
print "INFO :: Building plots"
from PlotTools.plot import PlotBase, Plot1D, Plot2D, Types
PlotBase.output_format = 'pdf'
PlotBase.save_dir = _plot_save_dir 
Plot1D.doLogY = False
Plot1D.doNorm = False
Plot1D.auto_set_ylimits = True
PlotBase.atlas_status = 'Internal'
PlotBase.atlas_lumi = '#sqrt{s} = 13 TeV, %d fb^{-1}' % _lumi

plots_defaults = {
    'runNumber' : Plot1D(bin_range=[284000, 311000], bin_width=1000, xlabel='Run Number', add_underflow=True, doLogY=True),
    'eventweight' : Plot1D(bin_range=[-0.001, 0.01], nbins=100, xlabel='Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'eventweight_multi' : Plot1D(bin_range=[-0.002, 0.003], nbins=100, xlabel='Multi-period Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'pupw' : Plot1D(bin_range=[0, 4], nbins=100, xlabel='Pilup Weight', add_underflow=True, doLogY=True, doNorm=True),
    'lepSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Lepton SF', add_underflow=True, doLogY=True, doNorm=True),
    'trigSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Trig SF', add_underflow=True, doLogY=True, doNorm=True),
    'btagSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='B-tag SF', add_underflow=True, doLogY=True, doNorm=True),
    'jvtSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='JVT SF', add_underflow=True, doLogY=True, doNorm=True),
    'period_weight' : Plot1D(bin_range=[0, 3], nbins=100, xlabel='Period weight', add_underflow=True, doLogY=True, doNorm=True),
    'avgMu' : Plot1D(bin_range=[0, 80], bin_width=1, xlabel='Average pileup', add_underflow=True),
    'avgMuDataSF' : Plot1D(bin_range=[0, 80], bin_width=1, xlabel='Average pileup (Data SF)', add_underflow=True),
    'nVtx' : Plot1D(bin_range=[0, 80], bin_width=1, xlabel='N_{vertices}', add_underflow=True),
    'actualMu' : Plot1D(bin_range=[0, 80], bin_width=1, xlabel='Actual pileup', add_underflow=True),
    'actualMuDataSF' : Plot1D(bin_range=[0, 80], bin_width=1, xlabel='Actual pileup (Data SF)', add_underflow=True),
    'avgMuDataSF/avgMu' : Plot1D(bin_range=[0,2], nbins=100, xlabel='Average Pileup: DataSF/noDataSF', add_underflow=True),
    'actualMuDataSF/actualMu' : Plot1D(bin_range=[0,2], nbins=100, xlabel='Actual Pileup: DataSF/noDataSF', add_underflow=True),
    'actualMu/avgMu' : Plot1D(bin_range=[0,2], nbins=100, xlabel='Pileup: actual/average', add_underflow=True),
    'actualMuDataSF/avgMuDataSF' : Plot1D(bin_range=[0,2], nbins=100, xlabel='Pileup (Data SF): actual/average', add_underflow=True),
    'avgMuDataSF/nVtx' : Plot1D(bin_range=[0,10], nbins=100, xlabel='Actual pileup / nVertices', add_underflow=True),
    
    # Kinematics
    'lep1Pt' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{leading lep}', xunits='GeV', xcut_is_max=False),
    'lep2Pt' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading lep}', xunits='GeV', xcut_is_max=False),
    'jet1Pt'  : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{leading jet}', xunits='GeV'),
    'jet2Pt'  : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading jet}', xunits='GeV'),
    'mll'     : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='M_{ll}', xunits='GeV', xcut_is_max=False),
    'fabs(mll-91.2)'     : Plot1D(bin_range=[0, 50], bin_width=1, xlabel='|M_{ll} - 91.2|', xunits='GeV', xcut_is_max=False),
    'met'     : Plot1D(bin_range=[0, 200], bin_width=10, xlabel='E_{T}^{miss}', xunits='GeV', xcut_is_max=True),
    'metrel'  : Plot1D(bin_range=[0, 200], bin_width=5, xlabel='E_{T}^{miss}_{rel}', xunits='GeV', xcut_is_max=True),
    'dpTll'   : Plot1D(bin_range=[0, 100], bin_width=5, xlabel='#Deltap_{T}(l_{1},l_{2})', xunits='GeV', xcut_is_max=False),
    'pTll'    : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='p_{T}^{ll}', xunits='GeV', xcut_is_max=False),
    'max_HT_pTV_reco'    : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='max(HT,pTV)', xunits='GeV'),
    'lep1mT' : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{l_{1}}', xunits='GeV', xcut_is_max=True),
    'lep2mT' : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{l_{2}}', xunits='GeV', xcut_is_max=False),

    #Truth
    'lep1TruthClass'      : Plot1D( bin_range=[-4.5, 11.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Leading lepton truth classification'),
    'lep2TruthClass'      : Plot1D( bin_range=[-4.5, 11.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Subleading lepton truth classification'),
    'probeLep1TruthClass' : Plot1D( bin_range=[-4.5, 11.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Probe lepton truth classification'),

    # Fake Factor
    'probeLep1Pt' : Plot1D(bin_edges=[0, 3, 3.5, 4, 4.5, 5, 6, 7, 8, 9, 10, 11, 13, 16, 20, 30, 50], xlabel='p_{T}^{probe lep}', xunits='GeV', doLogY=False),
    #'probeLep1Pt' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{probe lep}', xunits='GeV'),
    'fabs(probeLep1Eta)' : Plot1D(bin_edges=[0.0, 0.6, 0.8, 1.15, 1.37, 1.52, 1.81, 2.01, 2.37, 2.5, 3.0], xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    #'fabs(probeLep1Eta)' : Plot1D(bin_edges=[0.0, 0.1, 1.05, 1.5, 2.0, 2.5, 2.7, 3.0], xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    #'probeLep1Eta' : Plot1D(bin_edges=[-3.0, -2.7, -1.05, 0, 1.05, 2.7, 3.0], xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    #'probeLep1Eta' : Plot1D(bin_edges=[-3.0, -2.5, -2.0, -1.45, -1.0, -0.5, 0, 0.5, 1.0, 1.45, 2.0, 2.5, 3.0], xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    'probeLep1Eta' : Plot1D(bin_range=[-3.0, 3.0], bin_width=0.25, xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    'probeLep1mT'  : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{probe lep}', xunits='GeV', xcut_is_max=True),
    'mlll'         : Plot1D(bin_range=[70, 150], bin_width=5, xlabel='M_{lll}', xunits='GeV', add_underflow=True),
    'Z2_mll'       : Plot1D(bin_range=[50, 140], bin_width=5, xlabel='Z2 M_{ll}', xunits='GeV', add_underflow=True),
    'dR_ZLep1_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z1})'),
    'dR_ZLep2_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z2})'),
    'dR_Z_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z})'),
    'probeLep1_d0sigBSCorr'   : Plot1D( bin_range=[-20, 20],     bin_width=0.8, add_underflow=True, doNorm=False, doLogY=True, xlabel='l^{probe}_{1} d_{0}/#sigma_{d_{0}} BSCorr'),
    'probeLep1_z0SinTheta'    : Plot1D( bin_range=[-0.5, 0.5],     bin_width=0.02, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='l^{probe}_{1} z_{0}sin(#theta)'),
    'nInvLeps' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Anti-ID leps}'),

    # Angles
    'dR_ll' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l_{1},l_{2})', xcut_is_max=False),
    'dphi_ll' : Plot1D(bin_range=[0, 3.2], bin_width=0.1, xlabel='#Delta#phi(l_{1},l_{2})'),
    'deta_ll' : Plot1D(bin_range=[0, 3], bin_width=0.2, xlabel='#Delta#eta(l_{1},l_{2})'),
    'deltaPhi_met_lep1' : Plot1D(bin_range=[0, 3.2], bin_width=0.2, xlabel='#Delta#phi(E_{T}^{miss},l_{1})', xcut_is_max=True),
    'deltaPhi_met_lep2' : Plot1D(bin_range=[0, 3.2], bin_width=0.2, xlabel='#Delta#phi(E_{T}^{miss},l_{2})', xcut_is_max=False),
    'lep1_d0sigBSCorr'   : Plot1D( bin_range=[-5, 5],     bin_width=0.2, add_underflow=True, doNorm=False, doLogY=True, xlabel='Lep1 d_{0}/#sigma_{d_{0}} BSCorr'),
    'lep2_d0sigBSCorr'   : Plot1D( bin_range=[-5, 5],     bin_width=0.2, add_underflow=True, doNorm=False, doLogY=True, xlabel='Lep2 d_{0}/#sigma_{d_{0}} BSCorr'),
    'lep1_z0SinTheta'    : Plot1D( bin_range=[-0.5, 0.5],     bin_width=0.02, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='Lep1 z_{0}sin(#theta)'),
    'lep2_z0SinTheta'    : Plot1D( bin_range=[-0.5, 0.5],     bin_width=0.02, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='Lep2 z_{0}sin(#theta)'),

    # Multiplicites
    'nLightJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{light jets}'),
    'nBJets'     : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{B jets}'),
    'nForwardJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{forward jets}'),
    'nNonBJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Signal jets}', xcut_is_max=True),
    'nSigLeps' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Signal leps}'),

    # Super-razor
    'shat' : Plot1D(bin_range=[0, 1000], bin_width=40, xlabel='m_{PP} or #sqrt{#hat{s}_{R}}', xunits='GeV'),
    'pTT_T' : Plot1D(bin_range=[0, 400], nbins=33, xlabel='|#vec{p}^{PP}_{T}|', xunits='GeV'),
    'MDR' : Plot1D(bin_range=[0, 200], nbins=25, xlabel='E_{V}^{P} or M_{#Delta}^{R}', xunits='GeV', xcut_is_max=True),
    'RPT' : Plot1D(bin_range=[0, 1], bin_width=0.05, xlabel='R_{p_{T}}'),
    'gamInvRp1' : Plot1D(bin_range=[0, 1], bin_width=0.1, xlabel='1/#gamma_{R+1}'),
    'DPB_vSS'   : Plot1D(bin_range=[0, 3.2], bin_width=0.1, xlabel='#Delta#phi(#vec{#beta}_{PP}^{LAB},#vec{p}_{V}^{PP}) or #Delta#phi_{#beta}^{R}'),
    'abs_costheta_b': Plot1D(bin_range=[0, 1.0], bin_width=0.1, xlabel='|cos#theta_{b}|'),
    'DPB_vSS - 0.9*abs_costheta_b': Plot1D(bin_range=[-1.5, 4], bin_width=0.5, xlabel='#Delta#phi_{#beta}^{R} - 0.9#times|cos#theta_{b}|', doLogY=False),
    '(DPB_vSS - 1.6) / abs_costheta_b': Plot1D(bin_edges=[-10,-1,0.9,3,10], xlabel='(#Delta#phi_{#beta}^{R} - 1.6) / |cos#theta_{b}|', add_underflow=True, doLogY=False),
    
    # 2-Dimensional (y:x)
    'DPB_vSS:costheta_b' : Plot2D(bin_range=[-1, 1, 0, 3.2], xbin_width = 0.1, ybin_width = 0.1, xlabel='cos#theta_{b}', ylabel='#Delta#phi(#vec{#Beta}_{PP}^{LAB},#vec{p}_{V}^{PP})'), 
    'dR_ll:dpTll' : Plot2D(bin_range=[0, 100, 0, 6], xbin_width = 1, ybin_width = 0.1, xlabel='#Deltap_{T}(l_{1},l_{2})', ylabel='#DeltaR(l_{1},l_{2})'), 
    'met:lep1mT' : Plot2D(bin_range=[0, 150, 0, 200], xbin_width = 10, ybin_width = 10, xlabel='lep1mT', ylabel='met'), 
    'met:lep2mT' : Plot2D(bin_range=[0, 150, 0, 200], xbin_width = 10, ybin_width = 10, xlabel='lep2mT', ylabel='met'), 
    'met:pTll'    : Plot2D(bin_range=[0, 300, 0, 200], xbin_width = 20, ybin_width = 10, xlabel='pTll', ylabel='met'), 
    'dR_ll:lep1mT' : Plot2D(bin_range=[0, 150, 0, 6], xbin_width = 10, ybin_width = 0.2, xlabel='lep1mT', ylabel='dR_ll'), 
    'dR_ll:lep2mT' : Plot2D(bin_range=[0, 150, 0, 6], xbin_width = 10, ybin_width = 0.2, xlabel='lep2mT', ylabel='dR_ll'), 
    'dR_ll:pTll'    : Plot2D(bin_range=[0, 300, 0, 6], xbin_width = 20, ybin_width = 0.2, xlabel='pTll', ylabel='dR_ll'), 
    'deltaPhi_met_lep1:lep1mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep1mT', ylabel='deltaPhi_met_lep1'), 
    'deltaPhi_met_lep1:lep2mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep2mT', ylabel='deltaPhi_met_lep1'), 
    'deltaPhi_met_lep1:pTll'    : Plot2D(bin_range=[0, 300, 0, 3.2], xbin_width = 20, ybin_width = 0.2, xlabel='pTll', ylabel='deltaPhi_met_lep1'), 
    'deltaPhi_met_lep2:lep1mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep1mT', ylabel='deltaPhi_met_lep2'), 
    'deltaPhi_met_lep2:lep2mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep2mT', ylabel='deltaPhi_met_lep2'), 
    'deltaPhi_met_lep2:pTll'    : Plot2D(bin_range=[0, 300, 0, 3.2], xbin_width = 20, ybin_width = 0.2, xlabel='pTll', ylabel='deltaPhi_met_lep2'), 
    'fabs(probeLep1Eta):probeLep1Pt'  : Plot2D(xbin_edges=[0, 3, 3.5, 4, 4.5, 5, 6, 7, 8, 9, 10, 11, 13, 16, 20, 30, 1000], xlabel='p_{T}^{probe lep}',
                                               ybin_edges=[0.0, 0.6, 0.8, 1.15, 1.37, 1.52, 1.81, 2.01, 2.37, 2.5, 3.0], ylabel='|#eta^{probe lep}|'), 
    'probeLep1Pt:probeLep1mT'    : Plot2D(bin_range=[0, 150, 0, 150], xbin_width = 10, ybin_width = 10, xlabel='m_{T}^{probe lep}', ylabel='p_{T}^{probe lep}'), 
    'probeLep1Pt:met'    : Plot2D(bin_range=[0, 150, 0, 150], xbin_width = 10, ybin_width = 10, xlabel='E_{T}^{miss}', ylabel='p_{T}^{probe lep}', ), 
}
l_truthClass_labels = ['','no Origin','no Mother','no Truth','Uncategorized','El','Mu','Pho','El from FSR','LF','Mu as El','Tau','HF B','HF C', 'pho conv (noM)','']
plots_defaults['lep1TruthClass'].bin_labels = l_truthClass_labels
plots_defaults['lep2TruthClass'].bin_labels = l_truthClass_labels
plots_defaults['probeLep1TruthClass'].bin_labels = l_truthClass_labels

region_plots = {}
region_plots['default'] = {
  'lep1Pt'                  : deepcopy(plots_defaults['lep1Pt']),
  'lep2Pt'                  : deepcopy(plots_defaults['lep2Pt']),
  'mll'                     : deepcopy(plots_defaults['mll']),
  'nBJets'                  : deepcopy(plots_defaults['nBJets']),
  'nNonBJets'               : deepcopy(plots_defaults['nNonBJets']),
  'MDR'                     : deepcopy(plots_defaults['MDR']),
  'RPT'                     : deepcopy(plots_defaults['RPT']),
  'gamInvRp1'               : deepcopy(plots_defaults['gamInvRp1']),
  'DPB_vSS'                 : deepcopy(plots_defaults['DPB_vSS']),
  'abs_costheta_b'          : deepcopy(plots_defaults['abs_costheta_b']),
  'DPB_vSS - 0.9*abs_costheta_b'        : deepcopy(plots_defaults['DPB_vSS - 0.9*abs_costheta_b']),
  'fabs(probeLep1Eta):probeLep1Pt'        : deepcopy(plots_defaults['fabs(probeLep1Eta):probeLep1Pt']),
  'fabs(probeLep1Eta)'            : deepcopy(plots_defaults['fabs(probeLep1Eta)']),
}
region_plots['presel_DF'] = deepcopy(region_plots['default'])
region_plots['ttbar_CR'] = deepcopy(region_plots['default'])
region_plots['ttbar_VR'] = deepcopy(region_plots['default'])
region_plots['VV_CR_DF'] = deepcopy(region_plots['default']) 
region_plots['VV_CR_SF'] = deepcopy(region_plots['default']) 
region_plots['VV_VR_DF'] = deepcopy(region_plots['default']) 
region_plots['VV_VR_SF'] = deepcopy(region_plots['default']) 
region_plots['mW_DF_pre'] = deepcopy(region_plots['default']) 
region_plots['mW_SF_pre'] = deepcopy(region_plots['default']) 
region_plots['mT_DF_pre'] = deepcopy(region_plots['default']) 
region_plots['mT_SF_pre'] = deepcopy(region_plots['default']) 
region_plots['zjets3l_CR_num'] = deepcopy(region_plots['default']) 

region_plots['presel_DF']['nBJets'].update(bin_range = [-0.5, 10.5, 1E-4, 1E2], bin_width=1, doLogY=True)
region_plots['presel_DF']['abs_costheta_b'].update(bin_range = [0, 1, 1E-2, 1E0], bin_width=0.1, doLogY=True)
region_plots['presel_DF']['DPB_vSS'].update(bin_range = [0, 3.2, 5E-3, 1E0], bin_width=0.1, doLogY=True)
region_plots['presel_DF']['gamInvRp1'].update(bin_range = [0, 1, 1E-3, 1E1], bin_width=0.05, doLogY=True)

region_plots['mW_DF_pre']['lep1Pt'].update(bin_range = [25, 500, 1E-1, 1E12], nbins=11, doLogY=True)
region_plots['mW_DF_pre']['lep2Pt'].update(bin_range = [20, 500, 1E-1, 1E12], bin_width=40, doLogY=True)
region_plots['mW_DF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E12], bin_width=20, doLogY=True)
region_plots['mW_DF_pre']['nBJets'].update(bin_range = [0, 3, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mW_DF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mW_DF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E12], bin_width=0.2, doLogY=True)
region_plots['mW_DF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E12], bin_width=0.5, doLogY=True)
#region_plots['mW_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mW_SF_pre']['lep1Pt'].update(bin_range = [25, 400, 1E-1, 1E13], nbins=9, doLogY=True)
region_plots['mW_SF_pre']['lep2Pt'].update(bin_range = [20, 500, 1E-1, 1E13], bin_width=40, doLogY=True)
region_plots['mW_SF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E13], bin_width=10, doLogY=True)
region_plots['mW_SF_pre']['nBJets'].update(bin_range = [0, 3, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mW_SF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mW_SF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E13], bin_width=0.2, doLogY=True)
region_plots['mW_SF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E13], bin_width=0.5, doLogY=True)
#region_plots['mW_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mT_DF_pre']['lep1Pt'].update(bin_range = [25, 500, 1E-1, 1E12], nbins=11, doLogY=True)
region_plots['mT_DF_pre']['lep2Pt'].update(bin_range = [20, 500, 1E-1, 1E12], bin_width=40, doLogY=True)
region_plots['mT_DF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E12], bin_width=10, doLogY=True)
region_plots['mT_DF_pre']['nBJets'].update(bin_range = [0, 6, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mT_DF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mT_DF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E12], bin_width=0.2, doLogY=True)
region_plots['mT_DF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E12], bin_width=0.5, doLogY=True)
#region_plots['mT_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mT_SF_pre']['lep1Pt'].update(bin_range = [25, 500, 1E-1, 1E13], nbins=11, doLogY=True)
region_plots['mT_SF_pre']['lep2Pt'].update(bin_range = [20, 400, 1E-1, 1E13], nbins=9, doLogY=True)
region_plots['mT_SF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E13], bin_width=10, doLogY=True)
region_plots['mT_SF_pre']['nBJets'].update(bin_range = [0, 6, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mT_SF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mT_SF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E13], bin_width=0.2, doLogY=True)
region_plots['mT_SF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E13], bin_width=0.5, doLogY=True)
#region_plots['mT_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['ttbar_CR']['lep1Pt'].update(bin_range = [25, 600, 1E-1, 1E8], nbins=19, doLogY=True)
region_plots['ttbar_CR']['nBJets'].update(bin_range = [0, 10, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['nNonBJets'].update(bin_range = [0, 12, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['MDR'].update(bin_range = [80, 200, 1E-1, 1E9], bin_width=5, doLogY=True)
region_plots['ttbar_CR']['RPT'].update(bin_range = [0.7, 1, 1E-1, 1E8], bin_width=0.02, doLogY=True)
region_plots['ttbar_CR']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.1, doLogY=True)
region_plots['ttbar_CR']['DPB_vSS'].update(bin_range = [0, 2.8, 1E-1, 1E8], bin_width=0.2, doLogY=True)
region_plots['ttbar_CR']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.05, doLogY=True)
region_plots['ttbar_CR_elmu'] = region_plots['ttbar_CR']
region_plots['ttbar_CR_muel'] = region_plots['ttbar_CR']

region_plots['ttbar_VR']['lep1Pt'].update(bin_range = [25, 300], nbins=13)
region_plots['ttbar_VR']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['ttbar_VR']['nNonBJets'].update(bin_range = [0, 10], bin_width=1)
region_plots['ttbar_VR']['MDR'].update(bin_range = [80, 135], bin_width=5)
region_plots['ttbar_VR']['RPT'].update(bin_range = [0, 0.7], bin_width=0.05)
region_plots['ttbar_VR']['gamInvRp1'].update(bin_range = [0, 1], bin_width=0.1)
region_plots['ttbar_VR']['DPB_vSS'].update(bin_range = [1.8, 3.2], bin_width=0.2)
region_plots['ttbar_VR']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.1)

region_plots['VV_CR_DF']['lep1Pt'].update(bin_range = [25, 100], nbins=7)
region_plots['VV_CR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_CR_DF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_CR_DF']['MDR'].update(bin_range = [50, 140], bin_width=10)
region_plots['VV_CR_DF']['RPT'].update(bin_range = [0, 0.5], bin_width=0.05)
region_plots['VV_CR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], bin_width=0.02)
region_plots['VV_CR_DF']['DPB_vSS'].update(bin_range = [0, 2.8], nbins=6)
region_plots['VV_CR_DF']['abs_costheta_b'].update(bin_range = [0, 1], nbins=7)

region_plots['VV_VR_DF']['lep1Pt'].update(bin_range = [25, 70], bin_width=5)
region_plots['VV_VR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_VR_DF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_VR_DF']['MDR'].update(bin_range = [50, 95], nbins=5)
region_plots['VV_VR_DF']['RPT'].update(bin_range = [0, 0.7], bin_width=0.1)
region_plots['VV_VR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], nbins=6)
region_plots['VV_VR_DF']['DPB_vSS'].update(bin_range = [1.8, 3.2], nbins=7)
region_plots['VV_VR_DF']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.2)

region_plots['VV_CR_SF']['lep1Pt'].update(bin_range = [25, 105], nbins=5)
region_plots['VV_CR_SF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_CR_SF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_CR_SF']['MDR'].update(bin_range = [70, 100], bin_width=5)
region_plots['VV_CR_SF']['RPT'].update(bin_range = [0, 0.5], bin_width=0.05)
region_plots['VV_CR_SF']['gamInvRp1'].update(bin_range = [0.7, 1], nbins=8)
region_plots['VV_CR_SF']['DPB_vSS'].update(bin_range = [0, 2.8], nbins=6)
region_plots['VV_CR_SF']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.2)

region_plots['VV_VR_SF']['lep1Pt'].update(bin_range = [25, 70], nbins=4)
region_plots['VV_VR_SF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_VR_SF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_VR_SF']['MDR'].update(bin_range = [60, 100], bin_width=5)
region_plots['VV_VR_SF']['RPT'].update(bin_range = [0, 0.4], bin_width=0.1)
region_plots['VV_VR_SF']['gamInvRp1'].update(bin_range = [0.7, 1], bin_width=0.05)
region_plots['VV_VR_SF']['DPB_vSS'].update(bin_range = [1.8, 3.2], nbins=7)
region_plots['VV_VR_SF']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.2)

region_plots['zjets3l_CR_num']['mll'].update(bin_range = [80.2, 102.2], bin_width=1)
region_plots['zjets3l_CR_num_ll_el'] = region_plots['zjets3l_CR_num']
region_plots['zjets3l_CR_num_elel_el'] = region_plots['zjets3l_CR_num']
region_plots['zjets3l_CR_num_mumu_el'] = region_plots['zjets3l_CR_num']

region_plots['zjets3l_CR_num_ll_mu'] = deepcopy(region_plots['zjets3l_CR_num'])
region_plots['zjets3l_CR_num_ll_mu']['fabs(probeLep1Eta):probeLep1Pt'].update(ybin_edges=[0.0, 0.1, 1.05, 1.5, 2.0, 2.5, 2.7, 3.0])
region_plots['zjets3l_CR_num_ll_mu']['fabs(probeLep1Eta)'].update(bin_edges=[0.0, 0.1, 1.05, 1.5, 2.0, 2.5, 2.7, 3.0])
region_plots['zjets3l_CR_num_elel_mu'] = region_plots['zjets3l_CR_num_ll_mu']
region_plots['zjets3l_CR_num_mumu_mu'] = region_plots['zjets3l_CR_num_ll_mu']

region_plots['zjets3l_CR_den'] = region_plots['zjets3l_CR_num']
region_plots['zjets3l_CR_den_ll_el'] = region_plots['zjets3l_CR_num_ll_el']
region_plots['zjets3l_CR_den_elel_el'] = region_plots['zjets3l_CR_num_ll_el']
region_plots['zjets3l_CR_den_mumu_el'] = region_plots['zjets3l_CR_num_ll_el']
region_plots['zjets3l_CR_den_ll_mu'] = region_plots['zjets3l_CR_num_ll_mu']
region_plots['zjets3l_CR_den_elel_mu'] = region_plots['zjets3l_CR_num_ll_mu']
region_plots['zjets3l_CR_den_mumu_mu'] = region_plots['zjets3l_CR_num_ll_mu']
region_plots['zjets2l_inc'] = region_plots['zjets3l_CR_num']

# Make a plot for reach region and variable
for reg in REGIONS:
    for var in _vars_to_plot:
        # Get plot settings
        # Use region specific settings if defined
        if reg.name in region_plots and var in region_plots[reg.name]:
            p = deepcopy(region_plots[reg.name][var]) 
        else:
            p = deepcopy(plots_defaults[var])
        
        # Set common plot properties
        if p.is2D:
            yvar, xvar = var.split(':')
            p.update(reg.name, xvar, yvar)
        else:
            p.update(reg.name, var)  # sets other plot variables (e.g. name)
        
        PLOTS.append(p)

################################################################################
print "INFO :: Configuration file loaded\n"
print "="*80
