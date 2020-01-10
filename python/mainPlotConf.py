#!/bin/usr/env python
import os, sys
import time
import ROOT as r
r.TTree.__init__._creates = False
r.TFile.__init__._creates = False
from copy import copy, deepcopy
from collections import defaultdict
from analysis_DSIDs import DSID_GROUPS
from IFFTruthClassifierDefs import IFF_Type

################################################################################
# Globals (not imported)
_work_dir = '/data/uclhc/uci/user/armstro1/Analysis_Stop2L/SusyNt_AB_21_2_79'
_plot_save_dir = '%s/run/plotting/plots/' % _work_dir

_apply_sf = False
_sf_samples = False # False -> DF samples
_fake_factor_looper = False
_truth_sel = False # For when getting yields and plots for the numerator region with data fake estimates

_only1516 = False
_only17 = False
_only18 = False
_only_dilep_trig = False
assert _only1516 + _only17 + _only18 <= 1

# Globals (imported)
EVENT_LIST_DIR = '%s/run/plotting/teventlists' % _work_dir
YIELD_TBL_DIR = '%s/run/plotting/yields' % _work_dir
SAMPLES = []
REGIONS = []
PLOTS = []
YLD_TABLE = None
if _fake_factor_looper:
    YIELD_TBL = None
    NUM_STR = 'num'
    DEN_STR = 'den'
    PLOT_DIR = _plot_save_dir
    NONCLOSURE_SYS = 0.3 # relative uncertainty +/-

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
    _sample_path_base = '%s/run/flatNts/outputs/zjets3l' % _work_dir
    
if _sf_samples:
    _sample_path = '%s/run/flatNts/outputs/baseline_SF' % _work_dir
else:
    #_sample_path = '%s/run/flatNts/outputs/baseline_DF' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/baseline_DF_den' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/zjets2l_inc' % _work_dir
    _sample_path = '%s/run/flatNts/outputs/zjets3l' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/zjets3l_den' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/baseline_SS' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/baseline_SS_den' % _work_dir
    #_sample_path = '%s/run/flatNts/outputs/NEW_baseline_SS_den' % _work_dir

_samples_to_use = [
    'data',
    'ttbar',
    'singletop',
    'ttX',
    #'wjets',
    #'zjets', # Combination: zjets_ee + zjets_mumu + zjets_tautau
    'zjets_ee',
    'zjets_mumu',
    #'zjets_tautau',
    'zgamma',
    #'wgamma',
    #'drellyan',
    'diboson',
    #'triboson',
    #'higgs',
    #'higgs_ggH', 'higgs_VBF', 'higgs_VH', 'higgs_ttH',
    #'fnp', # Fake non-prompt 
    #'other', # Combination: defined below
    #'fnp_fakefactor',
    ############################## 
    #'missing_truth',
    #'multifake',
    #'prompt',
    #'LF',
    #'HF',
    #'conversion',
    #'chargeflip',
    #'other_fake',
    #'mistagged', # only Z+jets region
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
    'zjets3l_CR_num_ll_el', 
    #'zjets3l_CR_den_ll_el',
    'zjets3l_CR_num_ll_mu', 
    #'zjets3l_CR_den_ll_mu',
    #'zjets3l_CR_num_elel_el', 
    #'zjets3l_CR_den_elel_el',
    #'zjets3l_CR_num_mumu_el', 
    #'zjets3l_CR_den_mumu_el',
    #'zjets3l_CR_num_elel_mu', 
    #'zjets3l_CR_den_elel_mu',
    #'zjets3l_CR_num_mumu_mu', 
    #'zjets3l_CR_den_mumu_mu',

    #'VV3l_CR_num', 
    #'VV3l_CR_den',
    #'VV3l_CR_num_ll_el', 
    #'VV3l_CR_den_ll_el',
    #'VV3l_CR_num_ll_mu', 
    #'VV3l_CR_den_ll_mu',
    
    #'fnp_VR_SS_num',
    #'fnp_VR_SS_num_elel',
    #'fnp_VR_SS_num_mumu',
    #'fnp_VR_SS_num_elmu',
    #'fnp_VR_SS_num_muel',
    #'fnp_VR_SS_den',
    #'fnp_VR_SS_den_elel',
    #'fnp_VR_SS_den_mumu',
    #'fnp_VR_SS_den_elmu',
    #'fnp_VR_SS_den_muel',

]
_region_cfs_to_use = [
    #('ttbar_CR_elmu', 'ttbar_CR_muel')
]
_vars_to_plot = [
    # Event weights and SFs
    #'runNumber',
    #'eventweight_single',
    #'eventweight_multi',
    #'pupw_multi',
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
    #'recoLepOrderType',
    #'trigLepOrderType',
    #'fakeweight',
    #'firedTrig',
    #'trigMatchedToInvLep',

    # Dilepton
    #'lepPt[0]',
    #'fabs(lepClusEtaBE[0])',
    #'lepd0sigBSCorr[0]',
    #'fabs(lepd0sigBSCorr[0])',
    #'lepz0SinTheta[0]',
    #'fabs(lepz0SinTheta[0])',
    #'dR_lep_lep[0]',
    #'dR_jet_lep[0]',
    #'dEta_lep_lep[0]',
    #'dEta_jet_lep[0]',
    #'dPhi_lep_lep[0]',
    #'dPhi_jet_lep[0]',
    #'dPhi_met_lep[0]',
    #'lepTruthType[0]',
    #'lepTruthOrigin[0]',
    #'lepTruthIFFClass[0]',
    
    #'lepPt[1]',
    #'fabs(lepClusEtaBE[1])',
    #'lepd0sigBSCorr[1]',
    #'fabs(lepd0sigBSCorr[1])',
    #'lepz0SinTheta[1]',
    #'fabs(lepz0SinTheta[1])',
    #'dR_lep_lep[1]',
    #'dR_jet_lep[1]',
    #'dPhi_met_lep[1]',
    #'lepTruthIFFClass[1]',

    #'mll',
    #'fabs(mll-91.2)',
    #'met',
    #'metrel',
    #'dpTll',
    #'lepmT[0]',
    #'lepmT[1]',
    #'pTll',
    #'jet1Pt',

    # Zjets region
    #'ZLepPt[0]',
    #'ZLepd0sigBSCorr[0]',
    #'ZLepz0SinTheta[0]',
    #'dPhi_met_ZLep[0]',
    #'dR_jet_ZLep[0]',
    #'ZLepTruthType[0]',
    #'ZLepTruthOrigin[0]',
    #'ZLepTruthIFFClass[0]',
    #'ZLepPt[1]',
    #'ZLepd0sigBSCorr[1]',
    #'ZLepz0SinTheta[1]',
    #'dPhi_met_ZLep[1]',
    #'dR_jet_ZLep[1]',
    #'ZLepTruthType[1]',
    #'ZLepTruthOrigin[1]',
    #'ZLepTruthIFFClass[1]',
    #'probeLepPt[0]',
    #'fabs(probeLepClusEtaBE[0])',
    #'probeLepPhi[0]',
    #'probeLepd0sigBSCorr[0]',
    #'probeLepz0SinTheta[0]',
    #'dR_lep_probeLep[0]',
    #'dR_jet_probeLep[0]',
    #'dR_bjet_probeLep[0]',
    #'dR_nonbjet_probeLep[0]',
    #'dEta_lep_probeLep[0]',
    #'dEta_jet_probeLep[0]',
    #'dEta_bjet_probeLep[0]',
    #'dEta_nonbjet_probeLep[0]',
    #'dPhi_lep_probeLep[0]',
    #'dPhi_jet_probeLep[0]',
    #'dPhi_bjet_probeLep[0]',
    #'dPhi_nonbjet_probeLep[0]',
    #'dPhi_met_probeLep[0]',
    'probeLepTruthIFFClass[0]',
    #'fnpLepTruthIFFClass',
    #'promptLepTruthIFFClass',
    #'ZpT',
    #'Zmass',
    #'dpT_ZLeps',
    #'dEta_ZLeps',
    #'dPhi_ZLeps',
    #'dR_ZLeps',
    

    # Complex kinematics
    #'MT2'

    ## Truth
    #'truthLepOrderType',
    #'lepTruthIFFClass[0]',
    #'lepTruthIFFClass[1]',
    #'lepTruthIFFClass[2]',
    #'probeLepTruthIFFClass[0]',

    # Fake Factor
    #'probeLepPt[0]',
    #'fabs(probeLepClusEtaBE[0])',
    #'probeLepmT[0]',
    #'Z2_mll',
    #'mlll',
    #'dR_ZLep1_probeLep1',
    #'dR_ZLep2_probeLep1',
    #'dR_Z_probeLep1',
    #'n_invLeps',
    #'fabs(probeLepClusEtaBE[0]):probeLepPt[0]',
    #'lepIsTrigMatched[probeLepIdx[0]]:fabs(probeLepClusEtaBE[0]):probeLepPt[0]',

    ## Angles
    #'dR_ll',
    #'dPhi_ll', 
    #'dEta_ll',
    
    ## Multiplicites
    #'nLightJets',
    #'nBJets',
    #'nForwardJets',
    #'nNonBJets',
    #'n_sigLeps',
    #'n_invLeps',
    #'n_leps',

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
    #'dPhi_met_lep1:lep1mT',
    #'dPhi_met_lep1:lep2mT',
    #'dPhi_met_lep1:pTll',
    #'dPhi_met_lep2:lep1mT',
    #'dPhi_met_lep2:lep2mT',
    #'dPhi_met_lep2:pTll',

    #'dR_ll:lep1mT' 
    #'probeLepPt[0]:probeLepmT[0]',
    #'probeLepPt[0]:met',
]
assert _vars_to_plot
################################################################################
# Make samples

# Initialize
from PlotTools.sample import Sample, Data, MCsample, MCBackground, DataBackground, Signal, color_palette
Sample.input_file_treename = 'superNt'

if _only1516 or _only17 or _only18:
    # Single campaign w/ PRW
    MCsample.weight_str = 'eventweight_single'

    # Single campaign w/o PRW
    #MCsample.weight_str = 'eventweight_single / pupw_single)'
else:
    # Multi-campaign w/ PRW
    MCsample.weight_str = 'eventweight_multi' 

    # Multi-campaign w/o PRW
    #MCsample.weight_str = 'eventweight_multi / pupw_multi'

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
DSID_GROUPS['MC'] = (
        DSID_GROUPS['zjets']
      + DSID_GROUPS['ttbar']
      + DSID_GROUPS['singletop']
      + DSID_GROUPS['ttX']
      + DSID_GROUPS['diboson'] 
      #+ DSID_GROUPS['higgs']
      + DSID_GROUPS['zgamma']
      #+ DSID_GROUPS['wgamma']
      #+ DSID_GROUPS['drellyan']
      #+ DSID_GROUPS['wjets']
        )
DSID_GROUPS['fnp_fakefactor'] = DSID_GROUPS['data'] + DSID_GROUPS['MC']
DSID_GROUPS['missing_truth'] = DSID_GROUPS['MC']
DSID_GROUPS['multifake'] = DSID_GROUPS['MC']
DSID_GROUPS['prompt'] = DSID_GROUPS['MC']
DSID_GROUPS['LF'] = DSID_GROUPS['MC']
DSID_GROUPS['HF'] = DSID_GROUPS['MC']
DSID_GROUPS['conversion'] = DSID_GROUPS['MC']
DSID_GROUPS['chargeflip'] = DSID_GROUPS['MC']
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

ZLeps_are_prompt = "(lepIsPrompt[ZLepIdx[0]] && lepIsPrompt[ZLepIdx[1]])"
mistagged_sel = "!(%s)" % ZLeps_are_prompt
missing_truth_sel = "(lepTruthIsUnknown[0] || lepTruthIsUnknown[1] || lepTruthIsUnknown[2])"

#mistagged_sel = "0"
#missing_truth_sel = "(lepTruthIsUnknown[0] || lepTruthIsUnknown[1])"

multifake_sel = "n_fnpLeps >= 2"
common_truth_sel = "!(%s || %s || %s)" % (missing_truth_sel, multifake_sel, mistagged_sel)
def fake_sel_string(base_sel, *args):
    type_sel = ["lepTruthIFFClass[fnpLepIdx[0]] == %d" % t for t in args]
    return "%s && (%s)" % (base_sel, " || ".join(type_sel))

missing_truth = MCBackground('missing_truth', "Missing truth info")
missing_truth.color = color_palette['gray'][0]
missing_truth.cut = missing_truth_sel
SAMPLES.append(missing_truth)

multifake = MCBackground('multifake', "Multiple fakes")
multifake.color = color_palette['orange'][0]
multifake.cut = "!(%s) && %s" % (missing_truth_sel, multifake_sel)
SAMPLES.append(multifake)

mistagged = MCBackground('mistagged', "Mistagged")
mistagged.color = color_palette['orange'][1]
mistagged.cut = "!(%s || %s) && %s" % (missing_truth_sel, multifake_sel, mistagged_sel) 
SAMPLES.append(mistagged)

prompt = MCBackground('prompt', "Prompt")
prompt.color = color_palette['green'][0]
prompt.cut = common_truth_sel + " && n_leps == n_promptLeps"
SAMPLES.append(prompt)

LF = MCBackground('LF', "Light flavor")
LF.color = color_palette['blue'][0]
LF.cut = fake_sel_string(common_truth_sel, IFF_Type['LightFlavorDecay'])
SAMPLES.append(LF)

HF = MCBackground('HF', "Heavy flavor")
HF.color = color_palette['red'][0]
HF.cut = fake_sel_string(common_truth_sel, IFF_Type['BHadronDecay'], IFF_Type["CHadronDecay"])
SAMPLES.append(HF)

conversion = MCBackground('conversion', "Conversion")
conversion.color = color_palette['yellow'][0]
conversion.cut = fake_sel_string(common_truth_sel, IFF_Type['NonPromptPhotonConv'])
SAMPLES.append(conversion)

chargeflip = MCBackground('chargeflip', "Charge flip")
chargeflip.color = color_palette['yellow'][1]
chargeflip.cut = fake_sel_string(common_truth_sel, IFF_Type['ChargeFlipPromptElectron'])
SAMPLES.append(chargeflip)

other_fake = MCBackground('other_fake', "Other")
other_fake.color = color_palette['cyan'][0]
other_fake.cut = fake_sel_string(common_truth_sel,
                                  IFF_Type['PromptPhotonConversion'],
                                  IFF_Type['ElectronFromMuon'],
                                  IFF_Type['TauDecay'])
SAMPLES.append(other_fake)


# Other
fnp = MCBackground('fnp','FNP (Wjets MC)')
fnp.color = r.TColor.GetColor("#FF8F02")
SAMPLES.append(fnp)

other = MCBackground('other','Other (VVV+DY+Higgs)')
other.color = r.TColor.GetColor("#009933")
SAMPLES.append(other)

fnp_fakefactor = DataBackground('fnp_fakefactor','FNP (Data & MC)', 'FNP (Data \& MC)')
fnp_fakefactor.color = color_palette['gray'][0]
fnp_fakefactor.weight_str = "(((!isMC) + (isMC * %s * %f)) * fakeweight)" % (MCsample.weight_str, MCsample.scale_factor)
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

    if _truth_sel:
        all_leps_prompt = '(!isMC || (truthLepOrderType == 1 || truthLepOrderType == 5))'
        if s.cut:
            s.cut += " && %s" % all_leps_prompt
        else:
            s.cut = all_leps_prompt

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
            #sample_path = '%s/run/flatNts/outputs/baseline_DF_den' % _work_dir
            #sample_path = '%s/run/flatNts/outputs/zjets3l_den' % _work_dir
            #sample_path = '%s/run/flatNts/outputs/baseline_SS_den' % _work_dir
            sample_path = '%s/run/flatNts/outputs/NEW_baseline_SS_den' % _work_dir
            if _only1516:
                exclude_str = ['mc16d','mc16e']
            elif _only17:
                exclude_str = ['mc16a','mc16e']
            elif _only18:
                exclude_str = ['mc16a','mc16d']
            else:
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

if _only_dilep_trig:
    trigger_sel = 'passDilepTrigs'
elif False: # not defined yet
    trigger_sel = 'passSingleLepTrigs'
else:
    trigger_sel = 'passLepTrigs'
    #trigger_sel = 'firedTrig>0'

def add_DF_channels(reg, REGs):
    elmu_channel = reg.build_channel('elmu', 'e#mu', '$e\\mu$', cuts=elmu)
    reg.compare_regions.append(elmu_channel)
    REGs.append(elmu_channel)

    muel_channel = reg.build_channel('muel', '#mue', '$\\mu e$', cuts=muel)
    reg.compare_regions.append(muel_channel)
    REGs.append(muel_channel)

def add_SF_channels(reg, REGs, useZLeps = False):
    ee_cut = "ZisElEl" if useZLeps else elel
    elel_channel = reg.build_channel('elel', 'ee', '$ee$', cuts=ee_cut)
    reg.compare_regions.append(elel_channel)
    REGs.append(elel_channel)

    mm_cut = "ZisMuMu" if useZLeps else mumu
    mumu_channel = reg.build_channel('mumu', '#mu#mu', '$\\mu \\mu$', cuts=mm_cut)
    reg.compare_regions.append(mumu_channel)
    REGs.append(mumu_channel)


baseline_truth_num = "n_fnpLeps == 0"
baseline_truth_den = "n_fnpLeps >= 1"
zjets_truth_num = "%s && n_fnpLeps == 0" % ZLeps_are_prompt
zjets_truth_den = "%s && n_fnpLeps >= 1" % ZLeps_are_prompt

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
region.tcut += ' && lepPt[1] > 20'
REGIONS.append(region)

region = Region('presel_SF','SF Preselection')
region.tcut = trigger_sel
region.tcut += ' && ' + SF
region.tcut += ' && lepPt[1] > 20'
REGIONS.append(region)

# Loose control regions
region = Region('ttbar_CR_loose', 'Loose t#bar{t} CR', 'Loose $t\\bar{t}$ CR')
region.tcut = trigger_sel
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
region.tcut += ' && (20 < lepPt[0] && lepPt[0] < 50.0)' # Select Z->tautau
region.tcut += ' && dR_ll > 2.0' # Select Z->tautau
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('zjets_CR_loose', 'Loose Z/#gamma*(#rightarrowll)+jets','Loose $Z/\gamma^{*}(\\rightarrow \ell\ell)$+jets')
region.tcut = trigger_sel
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && nNonBJets > 0' # Remove W+jets
region.tcut += ' && (fabs(Zmass - 91.2) < 10)' # Select Z->ee and Z->mumu
REGIONS.append(region)
add_SF_channels(region, REGIONS, useZLeps=True)

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
base_selection += ' && lepPt[0] > 25'
base_selection += ' && lepPt[1] > 20'

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

fnp_VR_cut = base_selection
fnp_VR_cut += ' && ' + SS 
#fnp_VR_cut += ' && RPT > 0.7'  # Same as SR
#fnp_VR_cut += ' && gamInvRp1 > 0.7'  # Same as SR
#fnp_VR_cut += ' && DPB_vSS > 0.9 * abs_costheta_b + 1.6' # Same as SR
fnp_VR_cut += ' && nBJets == 0' # Same as SR
#fnp_VR_cut += ' && MDR < 40'
fnp_VR_cut += ' && (isDF || (fabs(mll - 91.2) > 20 && fabs(dR_ll - 3.14) >= 0))'

region = Region('fnp_VR_SS_num', 'FNP VR SS (2 ID)', "FNP VR SS (2 $\\ell_\\text{ID}$)")
region.tcut = fnp_VR_cut
REGIONS.append(region)
add_SF_channels(region, REGIONS)
add_DF_channels(region, REGIONS)

region = Region('fnp_VR_SS_den', 'FNP VR SS (#geq 1 #bar{ID})', "FNP VR SS ($\ge 1 $\\ell_{\\bar{\\text{ID}}}$ )")
region.tcut = fnp_VR_cut + " && n_invLeps > 0"
REGIONS.append(region)
add_SF_channels(region, REGIONS)
add_DF_channels(region, REGIONS)

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
region = Region('zjets2l_inc', 'Z+jets (#geq2 lep)','$Z$+jets (\ge2 $\\ell_\\text{ID}$)')
region.tcut = trigger_sel
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && lepPt[0] > 25'
region.tcut += ' && lepPt[1] > 20'
region.tcut += ' && (fabs(mll - 91.2) < 10)'
#region.tcut += ' && met < 40'
REGIONS.append(region)

# Fake factor regions
zjets3l_sel = trigger_sel
zjets3l_sel += ' && (ZLepIsEle[0] == ZLepIsEle[1])' # Same flavor
zjets3l_sel += ' && (ZLepq[0] * ZLepq[1] < 0)'  # Opposite sign
zjets3l_sel += ' && ZLepPt[0] > 25'
zjets3l_sel += ' && ZLepPt[1] > 20'
zjets3l_sel += ' && (fabs(Zmass - 91.2) < 10)'
zjets3l_sel += ' && dR_lep_probeLep[0] > 0.2'
zjets3l_sel += ' && probeLepmT[0] < 40'
zjets3l_sel += ' && (probeLepPt[0] < 16 || met < 50)'
zjets3l_sel += ' && !lepIsTrigMatched[probeLepIdx[0]]'

#zjets3l_sel += ' && (!probeLepIsEle[0] || probeLepPt[0] > 15) && (probeLepIsEle[0] || probeLepPt[0] > 10)'
if "fnp_fakefactor" in _samples_to_use:
    zjets3l_sel += ' && (!isMC || %s)' % zjets_truth_num
    zjets3l_sel_num = zjets3l_sel
    zjets3l_sel_den = zjets3l_sel
else:
    zjets3l_sel_num = zjets3l_sel + " && n_sigLeps == 3 && n_invLeps == 0"
    zjets3l_sel_den = zjets3l_sel + " && n_sigLeps == 2 && n_invLeps == 1"

def add_zjets3L_channels(reg, REGs):
    # Define selections
    ll_el_sel = 'probeLepIsEle[0] == 1'
    ll_mu_sel = 'probeLepIsEle[0] == 0'
    elel_el_sel = 'ZLepIsEle[0] == 1 && ZLepIsEle[1] == 1 && probeLepIsEle[0] == 1'
    mumu_el_sel = 'ZLepIsEle[0] == 0 && ZLepIsEle[1] == 0 && probeLepIsEle[0] == 1'
    elel_mu_sel = 'ZLepIsEle[0] == 1 && ZLepIsEle[1] == 1 && probeLepIsEle[0] == 0'
    mumu_mu_sel = 'ZLepIsEle[0] == 0 && ZLepIsEle[1] == 0 && probeLepIsEle[0] == 0'
    
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

# Fake factor VV CRs
VV3l_sel = trigger_sel
VV3l_sel += ' && (ZLepIsEle[0] == ZLepIsEle[1])' # Same flavor
VV3l_sel += ' && (ZLepq[0] * ZLepq[1] < 0)'  # Opposite sign
VV3l_sel += ' && ZLepPt[0] > 25'
VV3l_sel += ' && ZLepPt[1] > 20'
VV3l_sel += ' && (fabs(Zmass - 91.2) < 10)'
VV3l_sel += ' && dR_lep_probeLep[0] > 0.2'
VV3l_sel += ' && probeLepmT[0] > 40'
VV3l_sel += ' && (!probeLepIsEle[0] || probeLepPt[0] > 15) && (probeLepIsEle[0] || probeLepPt[0] > 10)'

VV3l_sel_num = VV3l_sel + " && n_sigLeps == 3 && n_invLeps == 0"
VV3l_sel_den = VV3l_sel + " && n_sigLeps == 2 && n_invLeps == 1"

region = Region('VV3l_CR_num', 'VV 3lep CR (3 ID)','$VV\\rightarrow 3\\ell$ CR (3 $\\ell_\\text{ID}$)')
region.tcut = VV3l_sel_num
REGIONS.append(region)
add_zjets3L_channels(region, REGIONS)

region = Region('VV3l_CR_den', 'VV 3lep CR (2 ID + 1 #bar{ID})','$VV\\rightarrow 3\\ell$ CR (2 $\\ell_\\text{ID}$ + 1 $\\ell_{\\bar{\\text{ID}}}$)')
region.tcut = VV3l_sel_den
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
        if _truth_sel:
            reg.yield_table.add_row_formula(name='data_minus_mc', displayname='Data-MC', formula='data - MC')
            reg.yield_table.add_row_formula(name='data_minus_mc_div_data', displayname='(Data-MC)/Data', formula='(data - MC)/data')
        else:
            reg.yield_table.add_row_formula(name='vv_contamination', displayname='VV/MC', formula='diboson/MC')
        #reg.yield_table.add_row_formula(name='vv_contamination2', displayname='VV/Data', formula='diboson/data')


################################################################################
# Make Plots
print "INFO :: Building plots"
from PlotTools.plot import PlotBase, Plot1D, Plot2D, Plot3D, Types
PlotBase.output_format = 'pdf'
PlotBase.save_dir = _plot_save_dir 
Plot1D.doLogY = False
Plot1D.doNorm = False
Plot1D.auto_set_ylimits = True
PlotBase.atlas_status = 'Internal'
PlotBase.atlas_lumi = '#sqrt{s} = 13 TeV, %d fb^{-1}' % _lumi

#print "HACK :: Using biased binning"
#eta_ff_bins_el = [0.0, 2.47] 
#pt_ff_bins_el = [4.5, 20, 30, 50] 
#eta_ff_bins_mu = [0.0, 2.7] 
#pt_ff_bins_mu = [4, 20, 50]
print "HACK :: Using unbiased binning"
eta_ff_bins_el = [0.0, 0.6, 0.8, 1.15, 1.37, 1.52, 1.81, 2.01, 2.37, 2.47] 
pt_ff_bins_el = [4.5, 7, 8, 15, 50] 
eta_ff_bins_mu = [0.0, 0.1, 1.05, 1.5, 2.0, 2.5, 2.7] 
pt_ff_bins_mu = [4, 4.5, 5, 6, 7, 8, 15, 50]

pt_ff_bins_el_forFF = copy(pt_ff_bins_el)
pt_ff_bins_el_forFF[-1] = 1000
pt_ff_bins_mu_forFF = copy(pt_ff_bins_mu)
pt_ff_bins_mu_forFF[-1] = 1000

plots_defaults = {
    'runNumber' : Plot1D(bin_range=[284000, 311000], bin_width=1000, xlabel='Run Number', add_underflow=True, doLogY=True),
    'eventweight_single' : Plot1D(bin_range=[-0.1, 1], nbins=100, xlabel='Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'eventweight_multi' : Plot1D(bin_range=[-0.1, 1], nbins=100, xlabel='Multi-period Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'pupw_multi' : Plot1D(bin_range=[0, 4], nbins=100, xlabel='Pilup Weight', add_underflow=True, doLogY=True, doNorm=True),
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
    'recoLepOrderType' : Plot1D(bin_range=[-0.5,13.5], bin_width=1, xlabel="Enum Lep Reco Type", doNorm=True),
    'trigLepOrderType' : Plot1D(bin_range=[-0.5,10.5], bin_width=1, xlabel="Enum Lep Trig Match Type", doNorm=True),
    'fakeweight' : Plot1D(bin_range=[-2, 2], nbins=100, xlabel='fakeweight', add_underflow=True, doLogY=False, doNorm=True),
    'firedTrig' : Plot1D(bin_edges=[-0.5,0.5,6.5,10.5,18.5,24.5,25.5], xlabel="Fired trigger", doNorm=True),
    #'firedTrig' : Plot1D(bin_range=[-0.5, 25.5], bin_width=1, xlabel="Fired trigger", doNorm=True),
    'trigMatchedToInvLep' : Plot1D(bin_range=[-0.5,1.5], nbins=2, xlabel="Trig matched to Anti-ID", doNorm=False),
    
    # Dilepton
    'lepPt[0]' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{lep0}', xunits='GeV', xcut_is_max=False),
    'fabs(lepClusEtaBE[0])' : Plot1D(bin_edges=eta_ff_bins_el, xlabel='#eta^{lep0}', xunits='', add_underflow=True),
    'lepd0sigBSCorr[0]'   : Plot1D( bin_range=[-5.75, 5.75], bin_width=0.5, add_underflow=True, doNorm=False, doLogY=True, xlabel='lep0 d_{0}/#sigma_{d_{0}} BSCorr'),
    'lepz0SinTheta[0]'    : Plot1D( bin_range=[-0.575, 0.575], bin_width=0.05, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='lep0 z_{0}sin(#theta)'),
    'lepmT[0]'            : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{l0}', xunits='GeV'),
    'dR_lep_lep[0]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep0, nearest lep)', xunits='GeV', add_overflow=False),
    'dR_jet_lep[0]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep0, nearest jet)', xunits='GeV', add_overflow=False),
    'dR_bjet_lep[0]'      : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep0, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dR_nonbjet_lep[0]'   : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep0, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dEta_lep_lep[0]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep0, nearest lep)', xunits='GeV', add_overflow=False),
    'dEta_jet_lep[0]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep0, nearest jet)', xunits='GeV', add_overflow=False),
    'dEta_bjet_lep[0]'    : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep0, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dEta_nonbjet_lep[0]' : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep0, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_lep_lep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep0, nearest lep)', xunits='GeV', add_overflow=False),
    'dPhi_jet_lep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep0, nearest jet)', xunits='GeV', add_overflow=False),
    'dPhi_bjet_lep[0]'    : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep0, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_nonbjet_lep[0]' : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep0, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_met_lep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep0, MET)', xunits='GeV'),
    'lepTruthIFFClass[0]' : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class lead ZLep', xunits='GeV'),
    
    'lepPt[1]' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{lep1}', xunits='GeV', xcut_is_max=False),
    'fabs(lepClusEtaBE[1])' : Plot1D(bin_edges=eta_ff_bins_el, xlabel='#eta^{lep1}', xunits='', add_underflow=True),
    'lepd0sigBSCorr[1]'   : Plot1D( bin_range=[-5.75, 5.75], bin_width=0.5, add_underflow=True, doNorm=False, doLogY=True, xlabel='lep1 d_{0}/#sigma_{d_{0}} BSCorr'),
    'lepz0SinTheta[1]'    : Plot1D( bin_range=[-0.575, 0.575], bin_width=0.05, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='lep1 z_{0}sin(#theta)'),
    'lepmT[1]'            : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{l1}', xunits='GeV'),
    'dR_lep_lep[1]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep1, nearest lep)', xunits='GeV', add_overflow=False),
    'dR_jet_lep[1]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep1, nearest jet)', xunits='GeV', add_overflow=False),
    'dR_bjet_lep[1]'      : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep1, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dR_nonbjet_lep[1]'   : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lep1, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dEta_lep_lep[1]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep1, nearest lep)', xunits='GeV', add_overflow=False),
    'dEta_jet_lep[1]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep1, nearest jet)', xunits='GeV', add_overflow=False),
    'dEta_bjet_lep[1]'    : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep1, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dEta_nonbjet_lep[1]' : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(lep1, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_lep_lep[1]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep1, nearest lep)', xunits='GeV', add_overflow=False),
    'dPhi_jet_lep[1]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep1, nearest jet)', xunits='GeV', add_overflow=False),
    'dPhi_bjet_lep[1]'    : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep1, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_nonbjet_lep[1]' : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep1, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_met_lep[1]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(lep1, MET)', xunits='GeV'),
    'lepTruthIFFClass[1]' : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class lead ZLep', xunits='GeV'),

    'jet1Pt'  : Plot1D(bin_range=[0,350], bin_width = 10, xlabel='p_{T}^{leading jet}', xunits='GeV'),
    'jet2Pt'  : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading jet}', xunits='GeV'),
    'mll'     : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='M_{ll}', xunits='GeV', xcut_is_max=False),
    'fabs(mll-91.2)'     : Plot1D(bin_range=[0, 50], bin_width=1, xlabel='|M_{ll} - 91.2|', xunits='GeV', xcut_is_max=False),
    'met'     : Plot1D(bin_range=[0, 200], bin_width=10, xlabel='E_{T}^{miss}', xunits='GeV', xcut_is_max=True),
    'metrel'  : Plot1D(bin_range=[0, 200], bin_width=5, xlabel='E_{T}^{miss}_{rel}', xunits='GeV', xcut_is_max=True),
    'dpTll'   : Plot1D(bin_range=[0, 100], bin_width=5, xlabel='#Deltap_{T}(l_{1},l_{2})', xunits='GeV', xcut_is_max=False),
    'pTll'    : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='p_{T}^{ll}', xunits='GeV', xcut_is_max=False),
    'max_HT_pTV_reco'    : Plot1D(bin_range=[0, 300], bin_width=10, xlabel='max(HT,pTV)', xunits='GeV'),
    'fabs(lepd0sigBSCorr[0])'   : Plot1D( bin_range=[0, 15], bin_width=.15, doNorm=True, doLogY=True, xcut_is_max=True, xlabel='l^{lep0} d_{0}/#sigma_{d_{0}} BSCorr'),
    'fabs(lepz0SinTheta[0])'    : Plot1D( bin_range=[0, 1.5], bin_width=0.015, doNorm=True, doLogY=True, xunits='mm', xlabel='l^{lep0} z_{0}sin(#theta)'),
    'fabs(lepd0sigBSCorr[1])'   : Plot1D( bin_range=[0, 15], bin_width=.15, doNorm=True, doLogY=True, xlabel='l^{lep1} d_{0}/#sigma_{d_{0}} BSCorr'),
    'fabs(lepz0SinTheta[1])'    : Plot1D( bin_range=[0, 1.5], bin_width=0.015, doNorm=True, doLogY=True, xunits='mm', xlabel='l^{lep1} z_{0}sin(#theta)'),

    # Z+jets kinematics
    'ZLepPt[0]'            : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{leading Z lep}', xunits='GeV'),
    'ZLepd0sigBSCorr[0]'   : Plot1D( bin_range=[-15.75, 15.75], bin_width=1.5, add_underflow=True, doNorm=False, doLogY=True, xlabel='l^{Zlep0} d_{0}/#sigma_{d_{0}} BSCorr'),
    'ZLepz0SinTheta[0]'    : Plot1D( bin_range=[-1.575, 1.575], bin_width=0.15, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='l^{Zlep0} z_{0}sin(#theta)'),
    'dPhi_met_ZLep[0]'     : Plot1D(bin_range=[0, 3.2], bin_width = 0.2, xlabel='#Delta#phi(MET, lead ZLep)', xunits='GeV'),
    'dR_jet_ZLep[0]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(lead ZLep, nearest jet)', xunits='GeV'),
    'ZLepTruthType[0]'     : Plot1D(bin_range=[-1.5,39.5], bin_width = 1, xlabel='truth type lead ZLep', xunits='GeV'),
    'ZLepTruthOrigin[0]'   : Plot1D(bin_range=[-1.5,45.5], bin_width = 1, xlabel='truth origin lead ZLep', xunits='GeV'),
    'ZLepTruthIFFClass[0]' : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class lead ZLep', xunits='GeV'),
    'ZLepPt[1]'            : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading Z lep}', xunits='GeV'),
    'ZLepd0sigBSCorr[1]'   : Plot1D( bin_range=[-15.75, 15.75], bin_width=1.5, add_underflow=True, doNorm=False, doLogY=True, xlabel='l^{Zlep1} d_{0}/#sigma_{d_{0}} BSCorr'),
    'ZLepz0SinTheta[1]'    : Plot1D( bin_range=[-1.575, 1.575], bin_width=0.15, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='l^{Zlep1} z_{0}sin(#theta)'),
    'dPhi_met_ZLep[1]'     : Plot1D(bin_range=[0, 3.2], bin_width = 0.2, xlabel='#Delta#phi(MET, sublead ZLep)', xunits='GeV'),
    'dR_jet_ZLep[1]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(sublead ZLep, nearest jet)', xunits='GeV'),
    'ZLepTruthType[1]'     : Plot1D(bin_range=[-1.5,39.5], bin_width = 1, xlabel='truth type lead ZLep', xunits='GeV'),
    'ZLepTruthOrigin[1]'   : Plot1D(bin_range=[-1.5,45.5], bin_width = 1, xlabel='truth origin sublead ZLep', xunits='GeV'),
    'ZLepTruthIFFClass[1]' : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class sublead ZLep', xunits='GeV'),
    'probeLepPt[0]'        : Plot1D(bin_edges=pt_ff_bins_el, xlabel='p_{T}^{probe lep}', xunits='GeV', doLogY=False),
    #'probeLepPt[0]'        : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{probe lep}', xunits='GeV'),
    'fabs(probeLepClusEtaBE[0])' : Plot1D(bin_edges=eta_ff_bins_el, xlabel='#eta^{probe lep}', xunits='', add_underflow=True),
    'probeLepEta[0]'           : Plot1D(bin_range=[-3,3], bin_width = 0.2, xlabel='#eta^{probe lep}', xunits='GeV', add_underflow=True),
    'probeLepPhi[0]'           : Plot1D(bin_range=[-3.2,3.2], bin_width = 0.2, xlabel='#phi^{probe lep}', xunits='GeV', add_underflow=True),
    'probeLepd0sigBSCorr[0]'   : Plot1D( bin_range=[-5.75, 5.75], bin_width=0.5, add_underflow=True, doNorm=False, doLogY=True, xlabel='l^{probe} d_{0}/#sigma_{d_{0}} BSCorr'),
    'probeLepz0SinTheta[0]'    : Plot1D( bin_range=[-0.575, 0.575], bin_width=0.05, add_underflow=True, doNorm=False, doLogY=True, xunits='mm', xlabel='l^{probe} z_{0}sin(#theta)'),
    'dR_lep_probeLep[0]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(probe lep, nearest lep)', xunits='GeV', add_overflow=False),
    'dR_jet_probeLep[0]'       : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(probe lep, nearest jet)', xunits='GeV', add_overflow=False),
    'dR_bjet_probeLep[0]'      : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(probe lep, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dR_nonbjet_probeLep[0]'   : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#DeltaR(probe lep, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dEta_lep_probeLep[0]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(probe lep, nearest lep)', xunits='GeV', add_overflow=False),
    'dEta_jet_probeLep[0]'     : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(probe lep, nearest jet)', xunits='GeV', add_overflow=False),
    'dEta_bjet_probeLep[0]'    : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(probe lep, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dEta_nonbjet_probeLep[0]' : Plot1D(bin_range=[0,6], bin_width = 0.2, xlabel='#Delta#eta(probe lep, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_lep_probeLep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(probe lep, nearest lep)', xunits='GeV', add_overflow=False),
    'dPhi_jet_probeLep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(probe lep, nearest jet)', xunits='GeV', add_overflow=False),
    'dPhi_bjet_probeLep[0]'    : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(probe lep, nearest b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_nonbjet_probeLep[0]' : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(probe lep, nearest non b-jet)', xunits='GeV', add_overflow=False),
    'dPhi_met_probeLep[0]'     : Plot1D(bin_range=[0,3.2], bin_width = 0.2, xlabel='#Delta#phi(probe lep, MET)', xunits='GeV'),
    'probeLepTruthIFFClass[0]' : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class probe lep', xunits='GeV', doNorm=True, doLogY=True),
    'fnpLepTruthIFFClass'      : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class fake leps', xunits='GeV', doNorm=True, doLogY=True),
    'promptLepTruthIFFClass'   : Plot1D(bin_range=[-1.5,12.5], bin_width = 1, xlabel='truth class prompt leps', xunits='GeV', doNorm=True, doLogY=True),
    'ZpT'        : Plot1D(bin_range=[0, 150], bin_width = 5, xlabel='p_{T,ll} Z-tagged leps', xunits='GeV'),
    'Zmass'      : Plot1D(bin_range=[75, 105], bin_width=1, xlabel='M_{ll} Z-tagged leps', xunits='GeV'),
    'dpT_ZLeps'  : Plot1D(bin_range=[0, 100], bin_width=5, xlabel='#Deltap_{T}(ZLeps)', xunits='GeV', xcut_is_max=False),
    'dPhi_ZLeps' : Plot1D(bin_range=[0, 3.2], bin_width=0.1, xlabel='#Delta#phi(ZLeps)'),
    'dEta_ZLeps' : Plot1D(bin_range=[0, 3], bin_width=0.2, xlabel='#Delta#eta(ZLeps)'),
    'dR_ZLeps'   : Plot1D(bin_range=[0, 6], bin_width = 0.2, xlabel='#DeltaR(ZLeps)', xunits='GeV'),

    #Truth
    'lepTruthIFFClass[0]'      : Plot1D( bin_range=[-1.5, 12.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Leading lepton truth classification'),
    'lepTruthIFFClass[1]'      : Plot1D( bin_range=[-1.5, 12.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Subleading lepton truth classification'),
    'lepTruthIFFClass[2]'      : Plot1D( bin_range=[-1.5, 12.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Third-leading lepton truth classification'),
    'probeLepTruthIFFClass[0]' : Plot1D( bin_range=[-1.5, 12.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Probe lepton truth classification'),
    'truthLepOrderType' : Plot1D( bin_range=[-1.5, 15.5],  bin_width=1, doNorm=True, doLogY=False, xlabel='Order type of truth leptons'),

    # Fake Factor
    'probeLepmT[0]'  : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{probe lep}', xunits='GeV', xcut_is_max=True),
    'mlll'         : Plot1D(bin_range=[70, 150], bin_width=5, xlabel='M_{lll}', xunits='GeV', add_underflow=True),
    'Z2_mll'       : Plot1D(bin_range=[50, 140], bin_width=5, xlabel='Z2 M_{ll}', xunits='GeV', add_underflow=True),
    'dR_ZLep1_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z1})'),
    'dR_ZLep2_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z2})', add_overflow=False, doLogY=True),
    'dR_Z_probeLep1' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l^{probe}_{1},l_{Z})'),
    'n_invLeps' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Anti-ID leps}'),

    # Angles
    'dR_ll' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#DeltaR(l_{1},l_{2})', xcut_is_max=False),
    'dPhi_ll' : Plot1D(bin_range=[0, 3.2], bin_width=0.1, xlabel='#Delta#phi(l_{1},l_{2})'),
    'dEta_ll' : Plot1D(bin_range=[0, 6], bin_width=0.2, xlabel='#Delta#eta(l_{1},l_{2})'),
    'dPhi_met_lep1' : Plot1D(bin_range=[0, 3.2], bin_width=0.2, xlabel='#Delta#phi(E_{T}^{miss},l_{1})', xcut_is_max=True),
    'dPhi_met_lep2' : Plot1D(bin_range=[0, 3.2], bin_width=0.2, xlabel='#Delta#phi(E_{T}^{miss},l_{2})', xcut_is_max=False),

    # Multiplicites
    'nLightJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{light jets}'),
    'nBJets'     : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{B jets}', doLogY=True),
    'nForwardJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{forward jets}'),
    'nNonBJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{non-B jets}', doLogY=True, xcut_is_max=True),
    'n_sigLeps' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Signal leps}'),
    'n_leps' : Plot1D(bin_range=[-1.5, 5.5], bin_width=1, xlabel='N_{leps}'),

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
    'dPhi_met_lep1:lep1mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep1mT', ylabel='dPhi_met_lep1'), 
    'dPhi_met_lep1:lep2mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep2mT', ylabel='dPhi_met_lep1'), 
    'dPhi_met_lep1:pTll'    : Plot2D(bin_range=[0, 300, 0, 3.2], xbin_width = 20, ybin_width = 0.2, xlabel='pTll', ylabel='dPhi_met_lep1'), 
    'dPhi_met_lep2:lep1mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep1mT', ylabel='dPhi_met_lep2'), 
    'dPhi_met_lep2:lep2mT' : Plot2D(bin_range=[0, 150, 0, 3.2], xbin_width = 10, ybin_width = 0.2, xlabel='lep2mT', ylabel='dPhi_met_lep2'), 
    'dPhi_met_lep2:pTll'    : Plot2D(bin_range=[0, 300, 0, 3.2], xbin_width = 20, ybin_width = 0.2, xlabel='pTll', ylabel='dPhi_met_lep2'), 
    'fabs(probeLepClusEtaBE[0]):probeLepPt[0]'  : Plot2D(xbin_edges=pt_ff_bins_el_forFF, xlabel='p_{T}^{probe lep}', ybin_edges=eta_ff_bins_el, ylabel='|#eta^{probe lep}|'), 
    'lepIsTrigMatched[probeLepIdx[0]]:fabs(probeLepClusEtaBE[0]):probeLepPt[0]'  : Plot3D(xbin_edges=pt_ff_bins_el_forFF, xlabel='p_{T}^{probe lep}', ybin_edges=eta_ff_bins_el, ylabel='|#eta^{probe lep}|', zbin_edges=[-0.5,0.5,1.5], zlabel='Trig fired on anti-ID'), 
    'probeLepPt[0]:probeLepmT[0]'    : Plot2D(bin_range=[0, 150, 0, 150], xbin_width = 10, ybin_width = 10, xlabel='m_{T}^{probe lep}', ylabel='p_{T}^{probe lep}'), 
    'probeLepPt[0]:met'    : Plot2D(bin_range=[0, 150, 0, 150], xbin_width = 10, ybin_width = 10, xlabel='E_{T}^{miss}', ylabel='p_{T}^{probe lep}', ), 
}
l_truthClass_labels = ['','Unkn','KnUnkn','PrEl','ChFlip','NPrConv','PrMu','PrConv','MuAsEl','HadTau','HF B','HF C', 'LF', '']
plots_defaults['lepTruthIFFClass[0]'].bin_labels = l_truthClass_labels
plots_defaults['lepTruthIFFClass[1]'].bin_labels = l_truthClass_labels
plots_defaults['lepTruthIFFClass[2]'].bin_labels = l_truthClass_labels
plots_defaults['ZLepTruthIFFClass[0]'].bin_labels = l_truthClass_labels
plots_defaults['ZLepTruthIFFClass[1]'].bin_labels = l_truthClass_labels
plots_defaults['probeLepTruthIFFClass[0]'].bin_labels = l_truthClass_labels
plots_defaults['recoLepOrderType'].bin_labels = ['','II','IA','AI','AA','III','IIA','IAI','AII','IAA','AIA','AAI','AAA', '']
plots_defaults['trigLepOrderType'].bin_labels = ['','10','01','11','100','010','001','110','101','011','']
#plots_defaults['firedTrig'].bin_labels = ['',
#  'HLT_e120_lhloose', 
#  'HLT_e60_lhmedium', 
#  'HLT_e24_lhmedium_L1EM20VH', 
#  'HLT_e140_lhloose_nod0', 
#  'HLT_e60_lhmedium_nod0', 
#  'HLT_e26_lhtight_nod0_ivarloose', 
#  'HLT_mu40', 
#  'HLT_mu20_iloose_L1MU15', 
#  'HLT_mu50', 
#  'HLT_mu26_ivarmedium', 
#  'HLT_2e12_lhloose_L12EM10VH', 
#  'HLT_2e17_lhvloose_nod0', 
#  'HLT_2e24_lhvloose_nod0', 
#  'HLT_2e17_lhvloose_nod0_L12EM15VHI', 
#  'HLT_mu18_mu8noL1', 
#  'HLT_2mu10', 
#  'HLT_mu22_mu8noL1', 
#  'HLT_2mu14', 
#  'HLT_e17_lhloose_mu14', 
#  'HLT_e7_lhmedium_mu24', 
#  'HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1', 
#  'HLT_e17_lhloose_nod0_mu14', 
#  'HLT_e7_lhmedium_nod0_mu24', 
#  'HLT_e26_lhmedium_nod0_mu8noL1',
#  '']
plots_defaults['firedTrig'].bin_labels = ['','Single El', 'Single Mu', 'Dilepton SF', 'Dilepton DF', '']

region_plots = {}
region_plots['default'] = {
  'lepPt[0]'                  : deepcopy(plots_defaults['lepPt[0]']),
  'lepPt[1]'                  : deepcopy(plots_defaults['lepPt[1]']),
  'mll'                     : deepcopy(plots_defaults['mll']),
  'nBJets'                  : deepcopy(plots_defaults['nBJets']),
  'nNonBJets'               : deepcopy(plots_defaults['nNonBJets']),
  'MDR'                     : deepcopy(plots_defaults['MDR']),
  'RPT'                     : deepcopy(plots_defaults['RPT']),
  'gamInvRp1'               : deepcopy(plots_defaults['gamInvRp1']),
  'DPB_vSS'                 : deepcopy(plots_defaults['DPB_vSS']),
  'abs_costheta_b'          : deepcopy(plots_defaults['abs_costheta_b']),
  'DPB_vSS - 0.9*abs_costheta_b'        : deepcopy(plots_defaults['DPB_vSS - 0.9*abs_costheta_b']),
  'lepIsTrigMatched[probeLepIdx[0]]:fabs(probeLepClusEtaBE[0]):probeLepPt[0]'        : deepcopy(plots_defaults['lepIsTrigMatched[probeLepIdx[0]]:fabs(probeLepClusEtaBE[0]):probeLepPt[0]']),
  'fabs(probeLepClusEtaBE[0]):probeLepPt[0]'        : deepcopy(plots_defaults['fabs(probeLepClusEtaBE[0]):probeLepPt[0]']),
  'fabs(probeLepClusEtaBE[0])'            : deepcopy(plots_defaults['fabs(probeLepClusEtaBE[0])']),
  'probeLepPt[0]'            : deepcopy(plots_defaults['probeLepPt[0]']),
  'fabs(lepClusEtaBE[0])'   : deepcopy(plots_defaults['fabs(lepClusEtaBE[0])']),
  'fabs(lepClusEtaBE[1])'   : deepcopy(plots_defaults['fabs(lepClusEtaBE[1])']),
  'lepPt[0]'            : deepcopy(plots_defaults['lepPt[0]']),
  'lepPt[1]'            : deepcopy(plots_defaults['lepPt[1]']),
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
region_plots['fnp_VR_SS_num'] = deepcopy(region_plots['default']) 

region_plots['presel_DF']['nBJets'].update(bin_range = [-0.5, 10.5, 1E-4, 1E2], bin_width=1, doLogY=True)
region_plots['presel_DF']['abs_costheta_b'].update(bin_range = [0, 1, 1E-2, 1E0], bin_width=0.1, doLogY=True)
region_plots['presel_DF']['DPB_vSS'].update(bin_range = [0, 3.2, 5E-3, 1E0], bin_width=0.1, doLogY=True)
region_plots['presel_DF']['gamInvRp1'].update(bin_range = [0, 1, 1E-3, 1E1], bin_width=0.05, doLogY=True)

region_plots['mW_DF_pre']['lepPt[0]'].update(bin_range = [25, 500, 1E-1, 1E12], nbins=11, doLogY=True)
region_plots['mW_DF_pre']['lepPt[1]'].update(bin_range = [20, 500, 1E-1, 1E12], bin_width=40, doLogY=True)
region_plots['mW_DF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E12], bin_width=20, doLogY=True)
region_plots['mW_DF_pre']['nBJets'].update(bin_range = [0, 3, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mW_DF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mW_DF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E12], bin_width=0.2, doLogY=True)
region_plots['mW_DF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mW_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E12], bin_width=0.5, doLogY=True)
#region_plots['mW_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mW_SF_pre']['lepPt[0]'].update(bin_range = [25, 400, 1E-1, 1E13], nbins=9, doLogY=True)
region_plots['mW_SF_pre']['lepPt[1]'].update(bin_range = [20, 500, 1E-1, 1E13], bin_width=40, doLogY=True)
region_plots['mW_SF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E13], bin_width=10, doLogY=True)
region_plots['mW_SF_pre']['nBJets'].update(bin_range = [0, 3, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mW_SF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mW_SF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E13], bin_width=0.2, doLogY=True)
region_plots['mW_SF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mW_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E13], bin_width=0.5, doLogY=True)
#region_plots['mW_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mT_DF_pre']['lepPt[0]'].update(bin_range = [25, 500, 1E-1, 1E12], nbins=11, doLogY=True)
region_plots['mT_DF_pre']['lepPt[1]'].update(bin_range = [20, 500, 1E-1, 1E12], bin_width=40, doLogY=True)
region_plots['mT_DF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E12], bin_width=10, doLogY=True)
region_plots['mT_DF_pre']['nBJets'].update(bin_range = [0, 6, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mT_DF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E12], bin_width=1, doLogY=True)
region_plots['mT_DF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E12], bin_width=0.2, doLogY=True)
region_plots['mT_DF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E12], bin_width=0.05, doLogY=True)
region_plots['mT_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E12], bin_width=0.5, doLogY=True)
#region_plots['mT_DF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['mT_SF_pre']['lepPt[0]'].update(bin_range = [25, 500, 1E-1, 1E13], nbins=11, doLogY=True)
region_plots['mT_SF_pre']['lepPt[1]'].update(bin_range = [20, 400, 1E-1, 1E13], nbins=9, doLogY=True)
region_plots['mT_SF_pre']['MDR'].update(bin_range = [0, 200, 1E-1, 1E13], bin_width=10, doLogY=True)
region_plots['mT_SF_pre']['nBJets'].update(bin_range = [0, 6, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mT_SF_pre']['nNonBJets'].update(bin_range = [0, 15, 1E-1, 1E13], bin_width=1, doLogY=True)
region_plots['mT_SF_pre']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['DPB_vSS'].update(bin_range = [0, 3.2, 1E-1, 1E13], bin_width=0.2, doLogY=True)
region_plots['mT_SF_pre']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['RPT'].update(bin_range = [0, 1, 1E-1, 1E13], bin_width=0.05, doLogY=True)
region_plots['mT_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-1, 1E13], bin_width=0.5, doLogY=True)
#region_plots['mT_SF_pre']['DPB_vSS - 0.9*abs_costheta_b'].update(bin_range = [-1.5, 4, 1E-3, 1E1], bin_width=0.5, doLogY=True)

region_plots['ttbar_CR']['lepPt[0]'].update(bin_range = [25, 600, 1E-1, 1E8], nbins=19, doLogY=True)
region_plots['ttbar_CR']['nBJets'].update(bin_range = [0, 10, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['nNonBJets'].update(bin_range = [0, 12, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['MDR'].update(bin_range = [80, 200, 1E-1, 1E9], bin_width=5, doLogY=True)
region_plots['ttbar_CR']['RPT'].update(bin_range = [0.7, 1, 1E-1, 1E8], bin_width=0.02, doLogY=True)
region_plots['ttbar_CR']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.1, doLogY=True)
region_plots['ttbar_CR']['DPB_vSS'].update(bin_range = [0, 2.8, 1E-1, 1E8], bin_width=0.2, doLogY=True)
region_plots['ttbar_CR']['abs_costheta_b'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.05, doLogY=True)
region_plots['ttbar_CR_elmu'] = region_plots['ttbar_CR']
region_plots['ttbar_CR_muel'] = region_plots['ttbar_CR']

region_plots['ttbar_VR']['lepPt[0]'].update(bin_range = [25, 300], nbins=13)
region_plots['ttbar_VR']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['ttbar_VR']['nNonBJets'].update(bin_range = [0, 10], bin_width=1)
region_plots['ttbar_VR']['MDR'].update(bin_range = [80, 135], bin_width=5)
region_plots['ttbar_VR']['RPT'].update(bin_range = [0, 0.7], bin_width=0.05)
region_plots['ttbar_VR']['gamInvRp1'].update(bin_range = [0, 1], bin_width=0.1)
region_plots['ttbar_VR']['DPB_vSS'].update(bin_range = [1.8, 3.2], bin_width=0.2)
region_plots['ttbar_VR']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.1)

region_plots['VV_CR_DF']['lepPt[0]'].update(bin_range = [25, 100], nbins=7)
region_plots['VV_CR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_CR_DF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_CR_DF']['MDR'].update(bin_range = [50, 140], bin_width=10)
region_plots['VV_CR_DF']['RPT'].update(bin_range = [0, 0.5], bin_width=0.05)
region_plots['VV_CR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], bin_width=0.02)
region_plots['VV_CR_DF']['DPB_vSS'].update(bin_range = [0, 2.8], nbins=6)
region_plots['VV_CR_DF']['abs_costheta_b'].update(bin_range = [0, 1], nbins=7)

region_plots['VV_VR_DF']['lepPt[0]'].update(bin_range = [25, 70], bin_width=5)
region_plots['VV_VR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_VR_DF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_VR_DF']['MDR'].update(bin_range = [50, 95], nbins=5)
region_plots['VV_VR_DF']['RPT'].update(bin_range = [0, 0.7], bin_width=0.1)
region_plots['VV_VR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], nbins=6)
region_plots['VV_VR_DF']['DPB_vSS'].update(bin_range = [1.8, 3.2], nbins=7)
region_plots['VV_VR_DF']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.2)

region_plots['VV_CR_SF']['lepPt[0]'].update(bin_range = [25, 105], nbins=5)
region_plots['VV_CR_SF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_CR_SF']['nNonBJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_CR_SF']['MDR'].update(bin_range = [70, 100], bin_width=5)
region_plots['VV_CR_SF']['RPT'].update(bin_range = [0, 0.5], bin_width=0.05)
region_plots['VV_CR_SF']['gamInvRp1'].update(bin_range = [0.7, 1], nbins=8)
region_plots['VV_CR_SF']['DPB_vSS'].update(bin_range = [0, 2.8], nbins=6)
region_plots['VV_CR_SF']['abs_costheta_b'].update(bin_range = [0, 1], bin_width=0.2)

region_plots['VV_VR_SF']['lepPt[0]'].update(bin_range = [25, 70], nbins=4)
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
region_plots['zjets3l_CR_num_ll_mu']['lepIsTrigMatched[probeLepIdx[0]]:fabs(probeLepClusEtaBE[0]):probeLepPt[0]'].update(ybin_edges=eta_ff_bins_mu, xbin_edges=pt_ff_bins_mu_forFF)
region_plots['zjets3l_CR_num_ll_mu']['fabs(probeLepClusEtaBE[0]):probeLepPt[0]'].update(ybin_edges=eta_ff_bins_mu, xbin_edges=pt_ff_bins_mu_forFF)
region_plots['zjets3l_CR_num_ll_mu']['fabs(probeLepClusEtaBE[0])'].update(bin_edges=eta_ff_bins_mu)
region_plots['zjets3l_CR_num_ll_mu']['probeLepPt[0]'].update(bin_edges=pt_ff_bins_mu)
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

region_plots['fnp_VR_SS_num_mumu'] = deepcopy(region_plots['fnp_VR_SS_num'])
region_plots['fnp_VR_SS_num_mumu']['fabs(lepClusEtaBE[0])'].update(bin_edges=eta_ff_bins_mu)
region_plots['fnp_VR_SS_num_mumu']['fabs(lepClusEtaBE[1])'].update(bin_edges=eta_ff_bins_mu)
#region_plots['fnp_VR_SS_num_mumu']['lepPt[0]'].update(bin_edges=pt_ff_bins_mu)
#region_plots['fnp_VR_SS_num_mumu']['lepPt[1]'].update(bin_edges=pt_ff_bins_mu)
region_plots['fnp_VR_SS_num_muel'] = deepcopy(region_plots['fnp_VR_SS_num'])
region_plots['fnp_VR_SS_num_muel']['fabs(lepClusEtaBE[0])'].update(bin_edges=eta_ff_bins_mu)
#region_plots['fnp_VR_SS_num_muel']['lepPt[0]'].update(bin_edges=pt_ff_bins_mu)
region_plots['fnp_VR_SS_num_elmu'] = deepcopy(region_plots['fnp_VR_SS_num'])
region_plots['fnp_VR_SS_num_elmu']['fabs(lepClusEtaBE[1])'].update(bin_edges=eta_ff_bins_mu)
#region_plots['fnp_VR_SS_num_elmu']['lepPt[1]'].update(bin_edges=pt_ff_bins_mu)

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
        elif p.is3D:
            zvar, yvar, xvar = var.split(':')
            p.update(reg.name, xvar, yvar, zvar)
        else:
            p.update(reg.name, var)  # sets other plot variables (e.g. name)
        
        PLOTS.append(p)

################################################################################
print "INFO :: Configuration file loaded\n"
print "="*80
