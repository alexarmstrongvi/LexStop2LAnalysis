////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) <2018> by Alex Armstrong
///
/// @file makeFlatNtuple.h
/// @author Alex Armstrong <alarmstr@cern.ch>
/// @date <May 27th, 2018>
///
////////////////////////////////////////////////////////////////////////////////

#pragma once

// general cpp
#include <iostream>
#include <string>
#include <cmath>
#include <limits.h>
#include <algorithm>

// ROOT Data Analysis Framework
#include "TChain.h"
#include "TLorentzVector.h"

// analysis include(s)
#include "Superflow/Superflow.h"
#include "Superflow/Superlink.h"

#include "SusyNtuple/ChainHelper.h"
#include "SusyNtuple/string_utils.h"
#include "SusyNtuple/KinematicTools.h"
#include "SusyNtuple/MCTruthClassifierDefs.h"
//#include "SusyNtuple/SusyNt.h"
//#include "SusyNtuple/SusyDefs.h"
//#include "SusyNtuple/SusyNtObject.h"
//#include "SusyNtuple/SusyNtTools.h"


/// @brief Available run selections
enum Sel {BASELINE_SEL, ZLL_CR};

using std::string;
using sflow::Superflow;
using sflow::SuperflowRunMode;
using sflow::Superlink;

void setup_chain(TChain* chain, string iname);
void run_superflow(Sel sel_type);
Superflow* get_cutflow(TChain* chain, Sel sel_type);
string determine_suffix(string user_suffix, Sel sel_type);
Superflow* initialize_superflow(TChain *chain, string name_suffix);

void add_shortcut_variables(Superflow* superflow, Sel /*sel_type*/);
void add_cleaning_cuts(Superflow* superflow);
void add_baseline_lepton_cuts(Superflow* superflow);
void add_zll_cr_lepton_cuts(Superflow* superflow);
void add_final_cuts(Superflow* superflow);
void add_event_variables(Superflow* superflow);
void add_trigger_variables(Superflow* superflow);
void add_prelepton_variables(Superflow* superflow);
void add_baselepton_variables(Superflow* superflow);
void add_signallepton_variables(Superflow* superflow);
void add_tau_variables(Superflow* superflow);
void add_other_lepton_variables(Superflow* superflow);
void add_met_variables(Superflow* superflow);
void add_relative_met_variables(Superflow* superflow);
void add_jet_variables(Superflow* superflow);
void add_shortcut_variables_reset(Superflow* superflow);


bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons);
void add_SFOS_lepton_cut(Superflow* superflow);
void add_DFOS_lepton_cut(Superflow* superflow);
int get_lepton_truth_class(Susy::Lepton* lepton);
template<class T>
bool isIn(T element, vector<T> container) {
    if (find(container.begin(), container.end(), element) == container.end()) {
        return false;
    } else {
        return true;
    }
}
void print_weight_info(Superlink* sl);

// Important globals
TChain* m_chain = new TChain("susyNt");

bool m_denominator_selection = false;

// All globals must be initialized here and reset in the source file
int m_cutflags = 0;  ///< Cutflags used for cleaning cuts

TLorentzVector m_MET;

LeptonVector m_selectLeptons;
LeptonVector m_triggerLeptons;

TLorentzVector m_lepton0;
TLorentzVector m_lepton1;
TLorentzVector m_dileptonP4;
Susy::Electron* m_el0 = nullptr;
Susy::Electron* m_el1 = nullptr;
Susy::Muon* m_mu0 = nullptr;
Susy::Muon* m_mu1 = nullptr;

JetVector m_lightJets;
JetVector m_BJets;
JetVector m_forwardJets;
TLorentzVector m_Dijet_TLV, m_Jet1_TLV, m_Jet0_TLV;

////////////////////////////////////////////////////////////////////////////////
// Configuration settings
////////////////////////////////////////////////////////////////////////////////

SuperflowRunMode m_run_mode = SuperflowRunMode::nominal;  ///< The mode in which
            ///< Superflow will process events (e.g. compute weights or not)

