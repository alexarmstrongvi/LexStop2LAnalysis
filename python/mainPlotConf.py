#!/bin/usr/env python

import os
import ROOT as r
import atlasrootstyle.AtlasStyle
r.SetAtlasStyle()
from copy import deepcopy
from collections import defaultdict
from analysis_DSIDs import DSID_GROUPS
import glob

################################################################################
# Globals (not imported)
_work_dir = '/data/uclhc/uci/user/armstro1/SusyNt/Stop2l/SusyNt_master/susynt-read'
_sample_path = '%s/run/batch/SuperflowAnaStop2l_output/' % _work_dir
_plot_save_dir = '%s/run/plots/' % _work_dir

_only2015_16 = True
_only_dilep_trig = True

# Globals (imported)
EVENT_LIST_DIR = '%s/run/lists/teventlists' % _work_dir
YIELD_TBL_DIR = '%s/run/yields' % _work_dir
SAMPLES = []
REGIONS = []
PLOTS = []
YLD_TABLE = None

# Lumi values from
if _only2015_16:
#             2015      2016
    _lumi = (3219.56 + 32988.1) / 1000.0 # inverse fb
else:
#             2015      2016      2017      2018
    _lumi = (3219.56 + 32988.1 + 44307.4 + 59937.2) / 1000.0 # inverse fb

_samples_to_use = [
    'data',
    'ttbar',
    'Wt',
    'ttV',
    #'wjets_sherpa',
    'zjets', # Combination: zjets_ee + zjets_mumu + zjets_tautau
    #'zjets_ee_sherpa',
    #'zjets_mumu_sherpa',
    #'zjets_tautau_sherpa',
    #'drellyan_sherpa',
    'diboson',
    #'triboson',
    #'higgs',
    'fnp', # Fake non-prompt 
    'other', # Combination: defined below
]

_regions_to_use = [
    #'no_sel',
    
    #'ttbar_CR_loose',
    #'ttbar_CR_loose_elmu', 
    #'ttbar_CR_loose_muel',
    
    #'zjets_CR_loose', 
    #'zjets_CR_loose_elel',
    #'zjets_CR_loose_mumu',
    
    #'ztautau_CR_loose',
    #'ztautau_CR_loose_elmu', 
    #'ztautau_CR_loose_muel',

    #'wjets_CR_loose',
    #'wjets_CR_loose_elmu',
    #'wjets_CR_loose_muel',

    'ttbar_CR',
    #'ttbar_CR_elmu', 
    #'ttbar_CR_muel',
    
    'VV_CR_DF',
    #'VV_CR_DF_elmu',
    #'VV_CR_DF_muel',

    #'VV_CR_SF',
    #'VV_CR_SF_elmu',
    #'VV_CR_SF_muel',
    
    'ttbar_VR',
    #'ttbar_VR_elmu', 
    #'ttbar_VR_muel',
    
    'VV_VR_DF',
    #'VV_VR_DF_elmu',
    #'VV_VR_DF_muel',

    #'VV_VR_SF',
    #'VV_VR_SF_elel',
    #'VV_VR_SF_mumu',

    #'mW_SR',
    #'mW_SR_elmu',
    #'mW_SR_muel',

    #'mT_SR',
    #'mT_SR_elmu',
    #'mT_SR_muel',

]

_vars_to_plot = [
    # Event weights and SFs
    #'eventweight',
    #'eventweight_multi',
    #'pupw',
    #'lepSf',
    #'trigSf',
    #'btagSf',
    #'jvtSf',
    #'period_weight',
    #TODO: add pileup or average pileup

    ## Basic Kinematics
    'lept1Pt',
    #'lept2Pt',
    #'jet1Pt',
    #'jet2Pt',
    #'mll',
    #'met',
    #'dpTll',
    #'lept1mT',
    # TODO: add pTll

    ## Angles
    #'dR_ll',

    ## Multiplicites
    #'nLightJets',
    'nBJets',
    #'nForwardJets',
    'nLightJets+nForwardJets',
    # TODO: nSigJets

    ## Super-razor
    'MDR',
    'RPT',
    'gamInvRp1',
    'DPB_vSS',
    'fabs(costheta_b)',

    ## 2-Dimensional (y:x)
    #'DPB_vSS:costheta_b'
]
################################################################################
# Make samples

# Initialize
from PlotTools.sample import Sample, Data, MCsample, color_palette
Sample.input_file_treename = 'superNt'
# 
# HACK :: only remove pupw when ttbar mc16e sample
ttbar_mc16e = '(dsid==410472)'
pupw_fix = '(%s * pupw + (1 - %s))' % (ttbar_mc16e, ttbar_mc16e)
if _only2015_16:
    # Single campaign w/ PRW
    MCsample.weight_str = 'eventweight / period_weight'

    # Single campaign w/o PRW
    #MCsample.weight_str = 'eventweight / pupw' 
else:
    # Multi-campaign w/ PRW
    MCsample.weight_str = 'eventweight_multi / %s ' % pupw_fix

    # Multi-campaign w/o PRW
    #MCsample.weight_str = 'eventweight_multi / pupw'


MCsample.scale_factor = _lumi

# Build combined DSID groups
if _only2015_16:
    DSID_GROUPS['data'] = (
            DSID_GROUPS['data15']
          + DSID_GROUPS['data16']
          )
else:
    DSID_GROUPS['data'] = (
            DSID_GROUPS['data15']
          + DSID_GROUPS['data16']
          + DSID_GROUPS['data17']
          + DSID_GROUPS['data18']
          )

DSID_GROUPS['diboson'] = (
        DSID_GROUPS['1l_diboson_sherpa']
      + DSID_GROUPS['2l_diboson_sherpa']
      + DSID_GROUPS['3l_diboson_sherpa']
      + DSID_GROUPS['4l_diboson_sherpa']
      )
DSID_GROUPS['zjets'] = (
        DSID_GROUPS['zjets_tautau_sherpa']
      + DSID_GROUPS['zjets_mumu_sherpa']
      + DSID_GROUPS['zjets_ee_sherpa']
      )
DSID_GROUPS['higgs'] = (
        DSID_GROUPS['higgs_ggH']
      + DSID_GROUPS['higgs_VBF']
      + DSID_GROUPS['higgs_ttH']
      + DSID_GROUPS['higgs_VH']
      )
DSID_GROUPS['other'] = (
        DSID_GROUPS['triboson_sherpa']
      + DSID_GROUPS['drellyan_sherpa']
      )
DSID_GROUPS['fnp'] = DSID_GROUPS['wjets_sherpa']
DSID_GROUPS['Wt'] = DSID_GROUPS['WtPP8']

# Setup samples
data = Data('data','Data')
data.color = r.kBlack
SAMPLES.append(data)

# Top Samples
ttbar = MCsample('ttbar',"t#bar{t}","$t\\bar{t}$")
ttbar.color = r.TColor.GetColor("#FFFF04")
SAMPLES.append(ttbar)

Wt = MCsample('Wt')
Wt.color = r.TColor.GetColor("#FF0201")
SAMPLES.append(Wt)

ttV = MCsample('ttV','t#bar{t}+V','$t\\bar{t}+V$')
ttV.color = r.TColor.GetColor("#0400CC")
SAMPLES.append(ttV)

# V+jets
wjets = MCsample('wjets_sherpa','W+jets')
wjets.color = color_palette['yellow'][0]
SAMPLES.append(wjets)

zjets = MCsample('zjets','Z/#gamma*+jets','$Z/\gamma^{*}$+jets')
zjets.color = r.TColor.GetColor("#33FF03")
SAMPLES.append(zjets)

zjets_ee = MCsample('zjets_ee_sherpa','Z/#gamma*(#rightarrowee)+jets','$Z/\gamma^{*}(\\rightarrow ee)$+jets')
zjets_ee.color = color_palette['blue'][0]
SAMPLES.append(zjets_ee)

zjets_mumu = MCsample('zjets_mumu_sherpa','Z/#gamma*(#rightarrow#mu#mu)+jets','$Z/\gamma^{*}(\\rightarrow \mu\mu)$+jets')
zjets_mumu.color = color_palette['blue'][1]
SAMPLES.append(zjets_mumu)

zjets_tautau = MCsample('zjets_tautau_sherpa','Z/#gamma*(#rightarrow#tau#tau)+jets','$Z/\gamma^{*}(\\rightarrow \\tau\\tau)$+jets')
zjets_tautau.color = color_palette['blue'][2]
SAMPLES.append(zjets_tautau)

DY = MCsample('drellyan_sherpa','Drell-Yan')
DY.color = color_palette['blue'][3]
SAMPLES.append(DY)

# Multi-boson
diboson = MCsample('diboson','VV')
diboson.color = r.TColor.GetColor("#33CCFF")
SAMPLES.append(diboson)

triboson = MCsample('triboson','VVV')
triboson.color = color_palette['green'][1]
SAMPLES.append(triboson)

# Higgs
higgs = MCsample('higgs', 'Higgs')
higgs.color = r.TColor.GetColor("#009933")
SAMPLES.append(higgs)

# Other
fnp = MCsample('fnp','FNP (Wjets MC)')
fnp.color = r.TColor.GetColor("#FF8F02")
SAMPLES.append(fnp)

other = MCsample('other','Other (VVV+DY)')
other.color = r.TColor.GetColor("#009933")
SAMPLES.append(other)

for s in SAMPLES:
    if s.name not in _samples_to_use: continue
    # Assume sample name is the same as the DSID group key
    if _only2015_16:
        if s.isMC:
            checklist = ['mc16a']
            search_str = ['mc16a']
            exclude_str = []
        else:
            checklist = search_str = []
            exclude_str = ['mc16']
    else:
        if s.isMC:
            checklist = ['mc16a','mc16d','mc16e'] if s.isMC else []
            search_str = ['mc16']
            exclude_str = []
        else:
            checklist = search_str = []
            exclude_str = ['mc16']

    s.set_chain_from_dsid_list(DSID_GROUPS[s.name], _sample_path, checklist, search_str)

# Remove samples not properly setup
SAMPLES = [s for s in SAMPLES if s.is_setup()]

# Check for at least 1 sample
assert SAMPLES, "ERROR :: No samples are setup"

################################################################################
# Make Regions
from PlotTools.region import Region

########################################
# Define common selections
elel = '(lept1Flav == 0 && lept2Flav == 0)' 
elmu = '(lept1Flav == 0 && lept2Flav == 1)' 
muel = '(lept1Flav == 1 && lept2Flav == 0)' 
mumu = '(lept1Flav == 1 && lept2Flav == 1)' 
DF = '(lept1Flav != lept2Flav)'
SF = '(lept1Flav == lept2Flav)'
OS = '(lept1q != lept2q)'
SS = '(lept1q == lept2q)'

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
triggers['16']['mumu'] = '(HLT_mu22_mu8noL1)'
triggers['16']['emu']  = '(HLT_e17_lhloose_nod0_mu14)'# || HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1)'
triggers['16']['mue']  = '(HLT_e7_lhmedium_nod0_mu24)'

triggers['17']['e']    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
triggers['17']['ee']   = '(HLT_2e24_lhvloose_nod0)'
triggers['17']['mu']   = '(HLT_mu26_ivarmedium || HLT_mu50)'
triggers['17']['mumu'] = '(HLT_mu22_mu8noL1)'
triggers['17']['emu']  = '(HLT_e17_lhloose_nod0_mu14 || HLT_e26_lhmedium_nod0_mu8noL1)'
triggers['17']['mue']  = '(HLT_e7_lhmedium_nod0_mu24)'

triggers['18']['e']    = '(HLT_e26_lhtight_nod0_ivarloose || HLT_e60_lhmedium_nod0 || HLT_e140_lhloose_nod0)'
triggers['18']['ee']   = '(HLT_2e24_lhvloose_nod0 || HLT_2e17_lhvloose_nod0_L12EM15VHI)'
triggers['18']['mu']   = '(HLT_mu26_ivarmedium || HLT_mu50)'
triggers['18']['mumu'] = '(HLT_mu22_mu8noL1)'
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
    triggers[year]['final'] = '(treatAsYear != 20%s || %s)' % (year, triggers[year]['final'])
    
# Combine trigger strategy for each year
if _only2015_16:
    trigger_sel = join_trig(triggers['15']['final'],triggers['16']['final'])
else:
    trigger_sel = join_trig(triggers['15']['final'],triggers['16']['final'],
                            triggers['17']['final'],triggers['18']['final'])

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

def if_cut(iif,then):
    return "( !(%s) || (%s) )" % (iif, then)

########################################
# No selection
region = Region('no_sel','No Selection')
region.tcut = '1'
REGIONS.append(region)

# Loose control regions
region = Region('ttbar_CR_loose', 'Loose t#bar{t} CR', 'Loose $t\\bar{t}$ CR')
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 2'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('zll_CR_loose', 'Loose Z/#gamma*(#rightarrowll)+jets','Loose $Z/\gamma^{*}(\\rightarrow \ell\ell)$+jets')
region.tcut = trigger_sel
region.tcut += ' && ' + SF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && nLightJets > 0' # Remove W+jets
region.tcut += ' && (fabs(mll - 91.2) < 10)' # Select Z->ee and Z->mumu
REGIONS.append(region)
add_SF_channels(region, REGIONS)

region = Region('ztautau_CR_loose', "Loose Z/#gamma*(#rightarrow#tau#tau)+jets CR", "Loose $Z/\gamma^{*}(\\rightarrow \\tau\\tau)$+jets CR")
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && (30 < mll && mll < 60)' # Remove Z->ee and Z->mumu
region.tcut += ' && nLightJets > 0' # Remove W+jets
region.tcut += ' && 20 < met && met < 80' # Select Z->tautau
region.tcut += ' && (20 < lept1Pt && lept1Pt < 50.0)' # Select Z->tautau
region.tcut += ' && dR_ll > 2.0' # Select Z->tautau
#region.tcut += ' && dpTll < 30' # Select Z->tautau
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('wjets_CR_loose', 'Loose W+jets CR')
region.tcut = trigger_sel
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets == 0' # Remove Top
region.tcut += ' && (mll < 60 || 120 < mll)' # Remove Z+jets
region.tcut += ' && lept1mT > 50' # Select lep1 from W decay
#region.tcut += ' && dR_ll > 2.0' # Select back-to-back leptons
REGIONS.append(region)
add_DF_channels(region, REGIONS)

########################################
# Baseline
base_selection = trigger_sel
base_selection += ' && lept1Pt > 25' 
base_selection += ' && lept2Pt > 20'

########################################
# Control regions
region = Region('ttbar_CR', 't#bar{t} CR', '$t\\bar{t}$ CR')
region.tcut = base_selection
region.tcut += ' && ' + DF + ' && ' + OS
region.tcut += ' && nBJets > 0'
region.tcut += ' && MDR > 80'
region.tcut += ' && RPT > 0.7'
region.tcut += ' && (DPB_vSS < 0.9 * fabs(costheta_b) + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_DF', 'Diboson CR (DF)', 'Diboson CR (DF)')
region.tcut = base_selection
region.tcut += ' && nBJets == 0'
region.tcut += ' && MDR > 50'
region.tcut += ' && RPT < 0.5'
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS < 0.9 * fabs(costheta_b) + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_CR_SF', 'Diboson CR (SF)', 'Diboson CR (SF)')
region.tcut = base_selection
region.tcut += ' && fabs(mll - 91.2) > 10'
region.tcut += ' && nBJets == 0'
region.tcut += ' && MDR > 70'
region.tcut += ' && RPT < 0.5'
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS < 0.9 * fabs(costheta_b) + 1.6)'
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
region.tcut += ' && (DPB_vSS > 0.9 * fabs(costheta_b) + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_VR_DF', 'Diboson VR (DF)', 'Diboson VR (DF)')
region.tcut = base_selection
region.tcut += ' && nBJets == 0'
region.tcut += ' && 50 < MDR && MDR < 95' # tighter than CR
region.tcut += ' && RPT < 0.7' # looser than CR
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS > 0.9 * fabs(costheta_b) + 1.6)'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('VV_VR_SF', 'Diboson VR (SF)', 'Diboson VR (SF)')
region.tcut = base_selection
region.tcut += ' && fabs(mll - 91.2) > 10'
region.tcut += ' && nBJets == 0'
region.tcut += ' && 60 < MDR && MDR < 95' #tighter than CR
region.tcut += ' && RPT < 0.4' # tighter than CR
region.tcut += ' && gamInvRp1 > 0.7'
region.tcut += ' && (DPB_vSS > 0.9 * fabs(costheta_b) + 1.6)'
REGIONS.append(region)
add_SF_channels(region, REGIONS)

########################################
# Signal regions
signal_selection = base_selection
signal_selection += ' && RPT > 0.7'
signal_selection += ' && gamInvRp1 > 0.7'
signal_selection += ' && DPB_vSS > 0.9 * fabs(costheta_b) + 1.6'

region = Region('mW_SR', 'SR (#Deltam ~ m_{W})', 'SR ($\Delta m ~ m_{W}$)')
region.tcut = signal_selection
region.tcut = ' && nBJets == 0'
region.tcut = ' && MDR > 95'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

region = Region('mT_SR', 'SR (#Deltam ~ m_{t})', 'SR ($\Delta m ~ m_{t}$)')
region.tcut = signal_selection
region.tcut = ' && nBJets > 0'
region.tcut = ' && MDR > 110'
REGIONS.append(region)
add_DF_channels(region, REGIONS)

########################################
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

for reg in REGIONS:
    reg.yield_table = deepcopy(YLD_TABLE)
    if 'ttbar' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='ttbar/MC')
        reg.yield_table.add_row_formula(name='normf', displayname='Norm Factor', formula='(data - (MC - ttbar))/ttbar')
    elif 'zll' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='(zjets_ee_sherpa+zjets_mumu_sherpa)/diboson')
    elif 'ztautau' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='zjets_tautau_sherpa/MC')
    elif 'wjets' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='wjets_sherpa/MC')
    elif 'VV' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='diboson/MC')
        reg.yield_table.add_row_formula(name='normf', displayname='Norm Factor', formula='(data - (MC - diboson))/diboson')
    elif 'SR' in reg.name:
        reg.yield_table.add_row_formula(name='purity', displayname='Purity', formula='signal/MC')
        reg.yield_table.add_row_formula(name='ttbar_contamination', displayname='t#bar{t}', formula='ttbar/MC')
        reg.yield_table.add_row_formula(name='vv_contamination', displayname='VV', formula='diboson/MC')


################################################################################
# Make Plots
from PlotTools.plot import PlotBase, Plot1D, Plot2D, Types
PlotBase.output_format = 'pdf'
PlotBase.save_dir = _plot_save_dir 
Plot1D.doLogY = False
Plot1D.auto_set_ylimits = True
Plot1D.type_default = Types.stack 
PlotBase.atlas_status = 'Internal'
PlotBase.atlas_lumi = '#sqrt{s} = 13 TeV, %d fb^{-1}' % _lumi

plots_defaults = {
    'eventweight' : Plot1D(bin_range=[-0.001, 0.01], nbins=100, xlabel='Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'eventweight_multi' : Plot1D(bin_range=[-0.002, 0.003], nbins=100, xlabel='Multi-period Event Weight', add_underflow=True, doLogY=True, doNorm=True),
    'pupw' : Plot1D(bin_range=[0, 4], nbins=100, xlabel='Pilup Weight', add_underflow=True, doLogY=True, doNorm=True),
    'lepSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Lepton SF', add_underflow=True, doLogY=True, doNorm=True),
    'trigSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='Trig SF', add_underflow=True, doLogY=True, doNorm=True),
    'btagSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='B-tag SF', add_underflow=True, doLogY=True, doNorm=True),
    'jvtSf' : Plot1D(bin_range=[0, 2], nbins=100, xlabel='JVT SF', add_underflow=True, doLogY=True, doNorm=True),
    'period_weight' : Plot1D(bin_range=[0, 3], nbins=100, xlabel='Period weight', add_underflow=True, doLogY=True, doNorm=True),
    
    # Kinematics
    'lept1Pt' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{leading lep}', xunits='GeV'),
    'lept2Pt' : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading lep}', xunits='GeV'),
    'jet1Pt'  : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{leading jet}', xunits='GeV'),
    'jet2Pt'  : Plot1D(bin_range=[0,150], bin_width = 5, xlabel='p_{T}^{subleading jet}', xunits='GeV'),
    'mll'     : Plot1D(bin_range=[0, 300], bin_width=5, xlabel='M_{ll}', xunits='GeV'),
    'met'     : Plot1D(bin_range=[0, 200], bin_width=5, xlabel='E_{T}^{miss}', xunits='GeV'),
    'dpTll'  : Plot1D(bin_range=[0, 100], bin_width=5, xlabel='#Deltap_{T}(l_{1},l_{2})', xunits='GeV'),
    'lept1mT' : Plot1D(bin_range=[0, 150], bin_width=5, xlabel='m_{T}^{l_{1}}', xunits='GeV'),

    # Angles
    'dR_ll' : Plot1D(bin_range=[0, 6], bin_width=0.1, xlabel='#DeltaR(l_{1},l_{2})'),

    # Multiplicites
    'nLightJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{light jets}'),
    'nBJets'     : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{B jets}'),
    'nForwardJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{forward jets}'),
    'nLightJets+nForwardJets' : Plot1D(bin_range=[-1.5, 10.5], bin_width=1, xlabel='N_{Signal jets}'),

    # Super-razor
    'MDR' : Plot1D(bin_range=[0, 200], bin_width=5, xlabel='E_{V}^{P} or M_{#Delta}^{R}', xunits='GeV'),
    'RPT' : Plot1D(bin_range=[0, 1], bin_width=0.05, xlabel='R_{p_{T}}'),
    'gamInvRp1' : Plot1D(bin_range=[0, 1], bin_width=0.05, xlabel='1/#gamma_{R+1}'),
    'DPB_vSS'   : Plot1D(bin_range=[0, 3.2], bin_width=0.1, xlabel='#Delta#phi(#vec{#beta}_{PP}^{LAB},#vec{p}_{V}^{PP})'),
    'fabs(costheta_b)': Plot1D(bin_range=[0, 1.1], bin_width=0.1, xlabel='|cos#theta_{b}|'),
    
    # 2-Dimensional (y:x)
    'DPB_vSS:costheta_b' : Plot2D(bin_range=[-1, 1, 0, 3.2], xbin_width = 0.1, ybin_width = 0.1, xlabel='cos#theta_{b}', ylabel='#Delta#phi(#vec{#Beta}_{PP}^{LAB},#vec{p}_{V}^{PP})'), 
}
p = plots_defaults['lept1Pt']
region_plots = {}
region_plots['ttbar_CR'] = {
  'lept1Pt'                 : deepcopy(plots_defaults['lept1Pt']),
  'nBJets'                  : deepcopy(plots_defaults['nBJets']),
  'nLightJets+nForwardJets' : deepcopy(plots_defaults['nLightJets+nForwardJets']),
  'MDR'                     : deepcopy(plots_defaults['MDR']),
  'RPT'                     : deepcopy(plots_defaults['RPT']),
  'gamInvRp1'               : deepcopy(plots_defaults['gamInvRp1']),
  'DPB_vSS'                 : deepcopy(plots_defaults['DPB_vSS']),
  'fabs(costheta_b)'        : deepcopy(plots_defaults['fabs(costheta_b)']),
}
region_plots['ttbar_VR'] = deepcopy(region_plots['ttbar_CR'])
region_plots['VV_CR_DF'] = deepcopy(region_plots['ttbar_CR']) 
region_plots['VV_VR_DF'] = deepcopy(region_plots['ttbar_CR']) 

region_plots['ttbar_CR']['lept1Pt'].update(bin_range = [25, 600, 1E-1, 1E8], nbins=19, doLogY=True)
region_plots['ttbar_CR']['nBJets'].update(bin_range = [0, 10, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['nLightJets+nForwardJets'].update(bin_range = [0, 12, 1E-1, 1E9], bin_width=1, doLogY=True)
region_plots['ttbar_CR']['MDR'].update(bin_range = [80, 200, 1E-1, 1E9], bin_width=5, doLogY=True)
region_plots['ttbar_CR']['RPT'].update(bin_range = [0.7, 1, 1E-1, 1E8], bin_width=0.02, doLogY=True)
region_plots['ttbar_CR']['gamInvRp1'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.1, doLogY=True)
region_plots['ttbar_CR']['DPB_vSS'].update(bin_range = [0, 2.8, 1E-1, 1E8], bin_width=0.2, doLogY=True)
region_plots['ttbar_CR']['fabs(costheta_b)'].update(bin_range = [0, 1, 1E-1, 1E9], bin_width=0.05, doLogY=True)

region_plots['ttbar_VR']['lept1Pt'].update(bin_range = [25, 300], nbins=13)
region_plots['ttbar_VR']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['ttbar_VR']['nLightJets+nForwardJets'].update(bin_range = [0, 10], bin_width=1)
region_plots['ttbar_VR']['MDR'].update(bin_range = [80, 135], bin_width=5)
region_plots['ttbar_VR']['RPT'].update(bin_range = [0, 0.7], bin_width=0.05)
region_plots['ttbar_VR']['gamInvRp1'].update(bin_range = [0, 1], bin_width=0.1)
region_plots['ttbar_VR']['DPB_vSS'].update(bin_range = [1.8, 3.2], bin_width=0.2)
region_plots['ttbar_VR']['fabs(costheta_b)'].update(bin_range = [0, 1], bin_width=0.1)

region_plots['VV_CR_DF']['lept1Pt'].update(bin_range = [25, 100], nbins=7)
region_plots['VV_CR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_CR_DF']['nLightJets+nForwardJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_CR_DF']['MDR'].update(bin_range = [50, 140], bin_width=10)
region_plots['VV_CR_DF']['RPT'].update(bin_range = [0, 0.5], bin_width=0.05)
region_plots['VV_CR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], bin_width=0.02)
region_plots['VV_CR_DF']['DPB_vSS'].update(bin_range = [0, 2.8], nbins=6)
region_plots['VV_CR_DF']['fabs(costheta_b)'].update(bin_range = [0, 1], nbins=7)

region_plots['VV_VR_DF']['lept1Pt'].update(bin_range = [25, 70], bin_width=5)
region_plots['VV_VR_DF']['nBJets'].update(bin_range = [0, 3], bin_width=1)
region_plots['VV_VR_DF']['nLightJets+nForwardJets'].update(bin_range = [0, 7], bin_width=1)
region_plots['VV_VR_DF']['MDR'].update(bin_range = [50, 95], nbins=5)
region_plots['VV_VR_DF']['RPT'].update(bin_range = [0, 0.7], bin_width=0.1)
region_plots['VV_VR_DF']['gamInvRp1'].update(bin_range = [0.7, 1], nbins=6)
region_plots['VV_VR_DF']['DPB_vSS'].update(bin_range = [1.8, 3.2], nbins=7)
region_plots['VV_VR_DF']['fabs(costheta_b)'].update(bin_range = [0, 1], bin_width=0.2)

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

################################################################################
print "INFO :: Configuration file loaded\n"
print "="*80
