////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) <2018> by Alex Armstrong
///
/// @file SuperflowAnaStop2L.cxx
/// @author Alex Armstrong (alarmstr@cern.ch)
/// @date <December 2018>
/// @brief Make flat ntuples from SusyNts
///
///////////////////////////////////////////////////////////////////////////////

// std
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
using std::cout;
#include <string>
using std::string;
#include <getopt.h>
#include <map>
using std::map;
#include <utility>
using std::pair;

// ROOT
#include "TChain.h"
#include "TVectorD.h"
#include "TF1.h"

// xAOD
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "xAODEgamma/Electron.h"
#include "xAODMuon/Muon.h"

// ASG
#include "AsgTools/StatusCode.h"
#include "PathResolver/PathResolver.h"
#include "IFFTruthClassifier/IFFTruthClassifier.h"
#include "IFFTruthClassifier/IFFTruthClassifierDefs.h"
#include "PATInterfaces/SystematicCode.h"
using namespace asg::msgUserCode; // required for ANA_CHECK

// SusyNtuple
#include "SusyNtuple/ChainHelper.h"
#include "SusyNtuple/string_utils.h"
#include "SusyNtuple/SusyNtSys.h"
#include "SusyNtuple/KinematicTools.h"
#include "SusyNtuple/AnalysisType.h"
#include "SusyNtuple/MCTruthClassifierDefs.h"

// Superflow
#include "Superflow/Superflow.h"
#include "Superflow/Superlink.h"
#include "Superflow/Cut.h"
#include "Superflow/StringTools.h"
#include "Superflow/input_options.h"

//Jigsaw
#include "jigsawcalculator/JigsawCalculator.h"

using namespace std;
using namespace sflow;

////////////////////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////////////////////
string m_ana_name = "SuperflowAnaStop2L";
string m_input_ttree_name = "susyNt";
bool m_verbose = true;
bool m_print_weighted_cutflow = true;
int m_lumi = 1000; // inverse picobarns (ipb)
Susy::AnalysisType m_ana_type = Susy::AnalysisType::Ana_Stop2L;
const float ZMASS = 91.2;
const float GeVtoMeV = 1000.0;

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string ttree_name, bool verbose);
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain);
bool set_global_variables(Superflow* sf);
void add_cleaning_cuts(Superflow* sf);
void add_analysis_cuts(Superflow* sf);
void add_4bcutflow_cuts(Superflow* sf);
void add_event_variables(Superflow* sf);
void add_trigger_variables(Superflow* sf);
void add_lepton_variables(Superflow* sf);
void add_mc_lepton_variables(Superflow* sf);
void add_jet_variables(Superflow* sf);
void add_met_variables(Superflow* sf);
void add_dilepton_variables(Superflow* sf);
void add_jigsaw_variables(Superflow* sf);
void add_miscellaneous_variables(Superflow* sf);
void add_Zlepton_variables(Superflow* sf);
void add_Zll_probeLep_variables(Superflow* sf);
void add_multi_object_variables(Superflow* sf);

void add_weight_systematics(Superflow* sf);
void add_shape_systematics(Superflow* sf);


// Selections (set with user input)
// Formatting: m_<region>_<SF/DF>_<den>
bool m_baseline_DF = false;
bool m_baseline_SS = false;
bool m_baseline_SS_den = false;
bool m_zjets_3l = false;
bool m_fake_baseline_DF = false;
bool m_fake_zjets_3l = false;
bool m_zjets2l_inc = false;

// globals for use in superflow cuts and variables
// TODO: Do I need static variables
static int m_cutflags = 0;
static JetVector m_light_jets;
static TLorentzVector m_MET;
// Formatting for lepton vectors: m_<identifier>Leps
// This is assumed in macros so it is required
// Only exception is for the all inclusive m_leps
static LeptonVector m_leps;
static LeptonVector m_sigLeps;
static LeptonVector m_invLeps;
static LeptonVector m_promptLeps;
static LeptonVector m_fnpLeps;
static LeptonVector m_promptSigLeps;
static LeptonVector m_promptInvLeps;
static LeptonVector m_fnpSigLeps;
static LeptonVector m_fnpInvLeps;

static LeptonVector m_ZLeps;
static LeptonVector m_probeLeps;
static int m_ztagged_idx1;
static int m_ztagged_idx2;
static int m_probeLep_idx;
static int m_trigLep_idx0;
static int m_trigLep_idx1;
static string m_firedTrig;
static map< uint, vector<string> > m_single_ele_trigs {
    { 2015, {
        "HLT_e120_lhloose",
        "HLT_e60_lhmedium",
        "HLT_e24_lhmedium_L1EM20VH"
    } },
    { 2016, {
        "HLT_e140_lhloose_nod0",
        "HLT_e60_lhmedium_nod0",
        "HLT_e26_lhtight_nod0_ivarloose"
    } },
    { 2017, {
        "HLT_e140_lhloose_nod0",
        "HLT_e60_lhmedium_nod0",
        "HLT_e26_lhtight_nod0_ivarloose"
    } },
    { 2018, {
        "HLT_e140_lhloose_nod0",
        "HLT_e60_lhmedium_nod0",
        "HLT_e26_lhtight_nod0_ivarloose"
    } }
};
static map< uint, vector<string> > m_single_mu_trigs {
    { 2015, {
        "HLT_mu40",
        "HLT_mu20_iloose_L1MU15"
    } },
    { 2016, {
        "HLT_mu50",
        "HLT_mu26_ivarmedium"
    } },
    { 2017, {
        "HLT_mu50",
        "HLT_mu26_ivarmedium"
    } },
    { 2018, {
        "HLT_mu50",
        "HLT_mu26_ivarmedium",
    } }
};
static map< uint, vector<string> > m_dielectron_trigs {
    { 2015, {
        "HLT_2e12_lhloose_L12EM10VH"
    } },
    { 2016, {
        "HLT_2e17_lhvloose_nod0"
    } },
    { 2017, {
        "HLT_2e24_lhvloose_nod0"
    } },
    { 2018, {
        "HLT_2e24_lhvloose_nod0",
        "HLT_2e17_lhvloose_nod0_L12EM15VHI"
    } }
};
static map< uint, vector<string> > m_dimuon_trigs {
    { 2015, {
        "HLT_2mu10",
        "HLT_mu18_mu8noL1"
    } },
    { 2016, {
        "HLT_2mu14",
        "HLT_mu22_mu8noL1"
    } },
    { 2017, {
        "HLT_2mu14",
        "HLT_mu22_mu8noL1"
    } },
    { 2018, {
        "HLT_2mu14",
        "HLT_mu22_mu8noL1"
    } },
};
static map< uint, vector<string> > m_diff_flav_trigs {
    { 2015, {
        "HLT_e17_lhloose_mu14",
        "HLT_e7_lhmedium_mu24"
    } },
    { 2016, {
        "HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1",
        "HLT_e17_lhloose_nod0_mu14",
        "HLT_e7_lhmedium_nod0_mu24"
    } },
    { 2017, {
        "HLT_e26_lhmedium_nod0_mu8noL1",
        "HLT_e17_lhloose_nod0_mu14",
        "HLT_e7_lhmedium_nod0_mu24"
    } },
    { 2018, {
        "HLT_e26_lhmedium_nod0_mu8noL1",
        "HLT_e17_lhloose_nod0_mu14",
        "HLT_e7_lhmedium_nod0_mu24"
    } },
};
static map<string, float> m_single_lep_pT_thresholds = {
    // Electron
    // 2015
    {"HLT_e24_lhmedium_L1EM20VH", 25},
    {"HLT_e60_lhmedium", 61},
    {"HLT_e120_lhloose", 121},
    // 2016-2018
    {"HLT_e26_lhtight_nod0_ivarloose", 27},
    {"HLT_e60_lhmedium_nod0", 61},
    {"HLT_e140_lhloose_nod0", 141},

    // Muon
    // 2015
    {"HLT_mu20_iloose_L1MU15", 21},
    {"HLT_mu40", 41},
    // 2016-2018
    {"HLT_mu26_ivarmedium", 27},
    {"HLT_mu50", 51},
};
static map<string, pair<float,float>> m_dilepton_pT_thresholds = {
    // Electron-Electron
    // 2015
    {"HLT_2e12_lhloose_L12EM10VH", std::make_pair(13, 13)},
    // 2016
    {"HLT_2e17_lhvloose_nod0", std::make_pair(18, 18)},
    // 2017-2018
    {"HLT_2e24_lhvloose_nod0", std::make_pair(25, 25)},
    // 2018
    {"HLT_2e17_lhvloose_nod0_L12EM15VHI", std::make_pair(18, 18)},

    // Muon-Muon
    // 2015
    {"HLT_2mu10", std::make_pair(11, 11)},
    {"HLT_mu18_mu8noL1", std::make_pair(19, 9)},
    // 2016-2018
    {"HLT_2mu14", std::make_pair(15, 15)},
    {"HLT_mu22_mu8noL1", std::make_pair(23, 9)},

    // Different flavor
    // 2015
    {"HLT_e17_lhloose_mu14", std::make_pair(18, 15)},
    {"HLT_e7_lhmedium_mu24",std::make_pair( 8, 25)},
    // 2016
    {"HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1", std::make_pair(27, 9)},
    // 2016-2018
    {"HLT_e17_lhloose_nod0_mu14", std::make_pair(18, 15)},
    {"HLT_e7_lhmedium_nod0_mu24",std::make_pair( 8, 25)},
    // 2017-2018
    {"HLT_e26_lhmedium_nod0_mu8noL1", std::make_pair(27, 9)},
};
static map<string, uint> m_trig_enum = {
    {"HLT_e120_lhloose",                          1},
    {"HLT_e60_lhmedium",                          2},
    {"HLT_e24_lhmedium_L1EM20VH",                 3},
    {"HLT_e140_lhloose_nod0",                     4},
    {"HLT_e60_lhmedium_nod0",                     5},
    {"HLT_e26_lhtight_nod0_ivarloose",            6},
    {"HLT_mu40",                                  7},
    {"HLT_mu20_iloose_L1MU15",                    8},
    {"HLT_mu50",                                  9},
    {"HLT_mu26_ivarmedium",                      10},
    {"HLT_2e12_lhloose_L12EM10VH",               11},
    {"HLT_2e17_lhvloose_nod0",                   12},
    {"HLT_2e24_lhvloose_nod0",                   13},
    {"HLT_2e17_lhvloose_nod0_L12EM15VHI",        14},
    {"HLT_mu18_mu8noL1",                         15},
    {"HLT_2mu10",                                16},
    {"HLT_mu22_mu8noL1",                         17},
    {"HLT_2mu14",                                18},
    {"HLT_e17_lhloose_mu14",                     19},
    {"HLT_e7_lhmedium_mu24",                     20},
    {"HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1",  21},
    {"HLT_e17_lhloose_nod0_mu14",                22},
    {"HLT_e7_lhmedium_nod0_mu24",                23},
    {"HLT_e26_lhmedium_nod0_mu8noL1",            24}
};


static map<string, bool> m_triggerPass;

static IFFTruthClassifier m_truthClassifier("truthClassifier");

static jigsaw::JigsawCalculator m_calculator;
static std::map< std::string, float> m_jigsaw_vars;

// Helpful functions
bool isSignal(const Susy::Lepton* lep, Superlink* sl);
bool isSignal(const Susy::Lepton* lep);
bool isInverted(const Susy::Lepton* lepton, Superlink* sl);
bool isInverted(const Susy::Lepton* lepton);
bool isPrompt(Susy::Lepton* lepton);
bool isFNP(Susy::Lepton* lepton);
bool isUnknownTruth(Susy::Lepton* lepton);
void add_lepton_property_flags(Superflow* sf);
void add_lepton_property_indexes(Superflow* sf);
void add_mc_lepton_property_flags(Superflow* sf);
void add_mc_lepton_property_indexes(Superflow* sf);
IFF::Type get_IFF_class(Susy::Lepton* lep);
const xAOD::Electron* to_iff_aod_electron(Susy::Electron& ele);
const xAOD::Muon* to_iff_aod_muon(Susy::Muon& muo);
int to_int(IFF::Type t);

bool is_1lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep, float pt_min = 0);
bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2, float pt_min1 = 0, float pt_min2 = 0);
#define ADD_LEP_TRIGGER_VAR(trig_name) { \
    *sf << NewVar(#trig_name" trigger bit"); { \
        *sf << HFTname(#trig_name); \
        *sf << [=](Superlink* /*sl*/, var_bool*) -> bool { \
            return m_triggerPass.at(#trig_name); \
        }; \
        *sf << SaveVar(); \
    } \
}

// Addings a jigsaw variable
#define ADD_JIGSAW_VAR(var_name) { \
    *sf << NewVar(#var_name); { \
        *sf << HFTname(#var_name); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { \
            return m_jigsaw_vars.count(#var_name) ? m_jigsaw_vars.at(#var_name) : -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
}

// Adding main lepton variables
#define ADD_LEPTON_VARS(lep_name) { \
    *sf << NewVar("number of "#lep_name"s"); { \
        *sf << HFTname("n_"#lep_name"s"); \
        *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name##s.size();}; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" isEle"); { \
        *sf << HFTname(#lep_name"IsEle"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->isEle() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" pT"); { \
        *sf << HFTname(#lep_name"Pt"); \
        *sf << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->Pt() ); } \
            return out; \
        }; \
    *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" eta"); { \
        *sf << HFTname(#lep_name"Eta"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->Eta() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" clusBE2 eta"); { \
        *sf << HFTname(#lep_name"ClusEtaBE"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                if (l->isEle()) { \
                    auto el = static_cast<Susy::Electron*>(l); \
                    out.push_back(el->clusEtaBE); \
                } else { \
                    out.push_back(l->Eta()); \
                } \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" phi"); { \
        *sf << HFTname(#lep_name"Phi"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->Phi() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" energy"); { \
        *sf << HFTname(#lep_name"E"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->E() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" mass"); { \
        *sf << HFTname(#lep_name"M"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->M() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" charge"); { \
        *sf << HFTname(#lep_name"q"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->q); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" d0sigBSCorr"); { \
        *sf << HFTname(#lep_name"d0sigBSCorr"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->d0sigBSCorr); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" z0SinTheta"); { \
        *sf << HFTname(#lep_name"z0SinTheta"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( l->z0SinTheta() ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" transverse mass"); { \
        *sf << HFTname(#lep_name"mT"); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dphi = l->DeltaPhi(m_MET); \
                double pT2 = l->Pt()*m_MET.Pt(); \
                double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) )); \
                out.push_back(lep_mT); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("delta Phi of "#lep_name" and met"); { \
        *sf << HFTname("dPhi_met_"#lep_name); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                out.push_back( fabs(l->DeltaPhi(m_MET)) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dR between "#lep_name" and closest lep"); { \
        *sf << HFTname("dR_lep_"#lep_name); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dR = m_leps.size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Lepton* l2 : m_leps) { \
                    if (l2 == l) continue; \
                    float tmp_dR = fabs(l2->DeltaR(*l)); \
                    if (tmp_dR < dR) dR = tmp_dR; \
                } \
                out.push_back( fabs(dR) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dR between "#lep_name" and closest jet"); { \
        *sf << HFTname("dR_jet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dR = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    float tmp_dR = fabs(jet->DeltaR(*l)); \
                    if (tmp_dR < dR) dR = tmp_dR; \
                } \
                out.push_back( fabs(dR) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dR between "#lep_name" and closest b-jet"); { \
        *sf << HFTname("dR_bjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dR = sl->tools->numberOfBJets(*sl->jets) ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (!sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dR = fabs(jet->DeltaR(*l)); \
                    if (tmp_dR < dR) dR = tmp_dR; \
                } \
                out.push_back( fabs(dR) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dR between "#lep_name" and closest non b-jet"); { \
        *sf << HFTname("dR_nonbjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dR = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dR = fabs(jet->DeltaR(*l)); \
                    if (tmp_dR < dR) dR = tmp_dR; \
                } \
                out.push_back( fabs(dR) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dPhi between "#lep_name" and closest lep"); { \
        *sf << HFTname("dPhi_lep_"#lep_name); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dPhi = m_leps.size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Lepton* l2 : m_leps) { \
                    if (l2 == l) continue; \
                    float tmp_dPhi = fabs(l2->DeltaPhi(*l)); \
                    if (tmp_dPhi < dPhi) dPhi = tmp_dPhi; \
                } \
                out.push_back( fabs(dPhi) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dPhi between "#lep_name" and closest jet"); { \
        *sf << HFTname("dPhi_jet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    float tmp_dPhi = fabs(jet->DeltaPhi(*l)); \
                    if (tmp_dPhi < dPhi) dPhi = tmp_dPhi; \
                } \
                out.push_back( fabs(dPhi) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dPhi between "#lep_name" and closest b-jet"); { \
        *sf << HFTname("dPhi_bjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dPhi = sl->tools->numberOfBJets(*sl->jets) ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (!sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dPhi = fabs(jet->DeltaPhi(*l)); \
                    if (tmp_dPhi < dPhi) dPhi = tmp_dPhi; \
                } \
                out.push_back( fabs(dPhi) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dPhi between "#lep_name" and closest non b-jet"); { \
        *sf << HFTname("dPhi_nonbjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dPhi = fabs(jet->DeltaPhi(*l)); \
                    if (tmp_dPhi < dPhi) dPhi = tmp_dPhi; \
                } \
                out.push_back( fabs(dPhi) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dEta between "#lep_name" and closest lep"); { \
        *sf << HFTname("dEta_lep_"#lep_name); \
        *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dEta = m_leps.size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Lepton* l2 : m_leps) { \
                    if (l2 == l) continue; \
                    float tmp_dEta = fabs(l2->Eta() - l->Eta()); \
                    if (tmp_dEta < dEta) dEta = tmp_dEta; \
                } \
                out.push_back( fabs(dEta) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dEta between "#lep_name" and closest jet"); { \
        *sf << HFTname("dEta_jet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dEta = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    float tmp_dEta = fabs(jet->Eta() - l->Eta()); \
                    if (tmp_dEta < dEta) dEta = tmp_dEta; \
                } \
                out.push_back( fabs(dEta) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dEta between "#lep_name" and closest b-jet"); { \
        *sf << HFTname("dEta_bjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dEta = sl->tools->numberOfBJets(*sl->jets) ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (!sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dEta = fabs(jet->Eta() - l->Eta()); \
                    if (tmp_dEta < dEta) dEta = tmp_dEta; \
                } \
                out.push_back( fabs(dEta) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar("dEta between "#lep_name" and closest non b-jet"); { \
        *sf << HFTname("dEta_nonbjet_"#lep_name); \
        *sf << [](Superlink* sl, var_float_array*) -> vector<double> { \
            vector<double> out; \
            for(const auto& l : m_##lep_name##s) { \
                double dEta = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
                for (Susy::Jet* jet : *sl->jets) { \
                    if (sl->tools->jetSelector().isBJet(jet)) continue; \
                    float tmp_dEta = fabs(jet->Eta() - l->Eta()); \
                    if (tmp_dEta < dEta) dEta = tmp_dEta; \
                } \
                out.push_back( fabs(dEta) ); \
            } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth type"); { \
        *sf << HFTname(#lep_name"TruthType"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->mcType); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth origin"); { \
        *sf << HFTname(#lep_name"TruthOrigin"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->mcOrigin); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth mother type"); { \
        *sf << HFTname(#lep_name"TruthMotherType"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->mcFirstEgMotherTruthType); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth mother origin"); { \
        *sf << HFTname(#lep_name"TruthMotherOrigin"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->mcFirstEgMotherTruthOrigin); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth mother PDG ID"); { \
        *sf << HFTname(#lep_name"TruthMotherPDGID"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back(l->mcFirstEgMotherPdgId); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" truth IFF class"); { \
        *sf << HFTname(#lep_name"TruthIFFClass"); \
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
            vector<int> out; \
            for(const auto& l : m_##lep_name##s) { out.push_back( to_int(get_IFF_class(l)) ); } \
            return out; \
        }; \
        *sf << SaveVar(); \
    } \
    \
}
////////////////////////////////////////////////////////////////////////////////
// Main function
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    /////////////////////////////////////////////////////////////////////
    // Read in the command-line options (input file, num events, etc...)
    ////////////////////////////////////////////////////////////////////
    SFOptions options(argc, argv);
    options.ana_name = m_ana_name;
    if(!read_options(options)) {
        exit(1);
    }

    if (options.ana_selection == "baseline_DF") {
        m_baseline_DF = true;
        cout << "INFO :: Running baseline DF selections\n";
    } else if (options.ana_selection == "baseline_SS") {
        m_baseline_SS = true;
        cout << "INFO :: Running baseline SF selections\n";
    } else if (options.ana_selection == "baseline_SS_den") {
        m_baseline_SS_den = true;
        cout << "INFO :: Running baseline SF denominator selections\n";
    } else if (options.ana_selection == "zjets3l") {
        m_zjets_3l = true;
        cout << "INFO :: Running Z+jets selections\n";
    } else if (options.ana_selection == "fake_baseline_DF") {
        m_fake_baseline_DF = true;
        cout << "INFO :: Running baseline DF denominator selections\n";
    } else if (options.ana_selection == "fake_zjets3l") {
        m_fake_zjets_3l = true;
        cout << "INFO :: Running Z+jets denominator selections\n";
    } else if (options.ana_selection == "zjets2l_inc") {
        m_zjets2l_inc = true;
        cout << "INFO :: Running Z+jets 2 lepton inclusive selections\n";
    } else {
        cout << "ERROR :: Unknown analysis selection:" << options.ana_selection << '\n';
        exit(1);
    }
    // New TChain* added to heap, remember to delete later
    TChain* chain = create_new_chain(options.input, m_input_ttree_name, m_verbose);

    Long64_t tot_num_events = chain->GetEntries();
    options.n_events_to_process = (options.n_events_to_process < 0 ? tot_num_events : options.n_events_to_process);

    xAOD::TEvent* tEvent = new xAOD::TEvent(); (void)tEvent;
    xAOD::TStore* tStore = new xAOD::TStore(); (void)tStore;

    ////////////////////////////////////////////////////////////
    // Initialize & configure the analysis
    //  > Superflow inherits from SusyNtAna : TSelector
    ////////////////////////////////////////////////////////////
    Superflow* superflow = create_new_superflow(options, chain);

    cout << options.ana_name << "    Total Entries: " << chain->GetEntries() << endl;
    //if (options.run_mode == SuperflowRunMode::single_event_syst) sf->setSingleEventSyst(nt_sys_);

    // Set variables for use in other cuts/vars. MUST ADD FIRST!
    // TODO: Move to after cleaning cuts and remove globals from cutflow
    set_global_variables(superflow);

    // Event selections
    add_cleaning_cuts(superflow);
    add_analysis_cuts(superflow);
    //add_4bcutflow_cuts(superflow);

    // Output variables
    add_event_variables(superflow);
    add_trigger_variables(superflow);
    add_lepton_variables(superflow);
    add_mc_lepton_variables(superflow);
    add_jet_variables(superflow);
    add_met_variables(superflow);
    add_dilepton_variables(superflow);
    if (m_baseline_DF || m_baseline_SS || m_baseline_SS_den || m_fake_baseline_DF) {
        add_jigsaw_variables(superflow);
    } else if (m_zjets_3l || m_fake_zjets_3l || m_zjets2l_inc) {
        add_Zlepton_variables(superflow);
    }
    if (m_zjets_3l || m_fake_zjets_3l) {
        add_Zll_probeLep_variables(superflow);
    };
    add_miscellaneous_variables(superflow);
    add_multi_object_variables(superflow);

    // Systematics
    add_weight_systematics(superflow);
    add_shape_systematics(superflow);

    // Run Superflow
    chain->Process(superflow, options.input.c_str(), options.n_events_to_process);

    // Clean up
    delete superflow;
    delete chain;

    cout << m_ana_name << "    Done." << endl;
    exit(0);
}

////////////////////////////////////////////////////////////////////////////////
// Function definitions
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string input_ttree_name, bool verbose) {
    TChain* chain = new TChain(input_ttree_name.c_str());
    chain->SetDirectory(0);
    ChainHelper::addInput(chain, input, verbose);
    return chain;
}
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain) {
    Superflow* sf = new Superflow(); // initialize the superflow
    sf->setAnaName(sf_options.ana_name);
    sf->setAnaType(m_ana_type);
    sf->setLumi(m_lumi);
    sf->setSampleName(sf_options.input);
    sf->setRunMode(sf_options.run_mode);
    sf->setCountWeights(m_print_weighted_cutflow);
    sf->setChain(chain);
    sf->setDebug(sf_options.dbg);
    sf->setOutput(sf_options.output_name);
    sf->nttools().initTriggerTool(ChainHelper::firstFile(sf_options.input, 0.0));
    if(sf_options.suffix_name != "") {
        sf->setFileSuffix(sf_options.suffix_name);
    }
    if(sf_options.sumw_file_name != "") {
        cout << sf_options.ana_name
             << "    Reading sumw for sample from file: "
             << sf_options.sumw_file_name << endl;
        sf->setUseSumwFile(sf_options.sumw_file_name);
    }
    return sf;
}
bool set_global_variables(Superflow* sf) {
    // IFFTruthClassifier
    ANA_CHECK( m_truthClassifier.initialize(); )
    // Jigsaw
    m_calculator.initialize("TTMET2LW");

    *sf << CutName("read in") << [](Superlink* sl) -> bool {
        ////////////////////////////////////////////////////////////////////////
        // Reset all globals used in cuts/variables
        m_cutflags = 0;
        m_light_jets.clear();
        m_MET = {};
        m_leps.clear();
        m_sigLeps.clear();
        m_invLeps.clear();
        m_promptLeps.clear();
        m_fnpLeps.clear();
        m_promptSigLeps.clear();
        m_promptInvLeps.clear();
        m_fnpSigLeps.clear();
        m_fnpInvLeps.clear();
        m_ZLeps.clear();
        m_probeLeps.clear();
        m_triggerPass.clear();

        ////////////////////////////////////////////////////////////////////////
        // Set globals
        // Note: No cuts have been applied so add appropriate checks before
        //       dereferencing pointers or accessing vector indices
        m_cutflags = sl->nt->evt()->cutFlags[NtSys::NOM];

        // Light jets: jets that are neither forward nor b-tagged
        for (int i = 0; i < (int)sl->jets->size(); i++) {
            if ( !sl->tools->jetSelector().isBJet(sl->jets->at(i))
              && !sl->tools->jetSelector().isForward(sl->jets->at(i))) {
                m_light_jets.push_back(sl->jets->at(i));
            }
        }

        // Missing transverse momentum
        m_MET.SetPxPyPzE(sl->met->Et * cos(sl->met->phi),
                         sl->met->Et * sin(sl->met->phi),
                         0.,
                         sl->met->Et);

        // Commonly used leptons
        m_leps = *sl->baseLeptons;
        //for (Susy::Lepton* lepton : *sl->baseLeptons) {
        //    if (isSignal(lepton, sl) || isInverted(lepton, sl)) m_leps.push_back(lepton);
        //}
        for (Susy::Lepton* lepton : m_leps) {
            bool isSig = false, isInv = false;
            if (isSignal(lepton, sl)) {
                isSig = true;
                m_sigLeps.push_back(lepton);
            } else if (isInverted(lepton, sl)) {
                isInv = true;
                m_invLeps.push_back(lepton);
            }
            if (sl->isMC) {
                if (isPrompt(lepton)) {
                    m_promptLeps.push_back(lepton);
                    if (isSig) m_promptSigLeps.push_back(lepton);
                    else if (isInv) m_promptInvLeps.push_back(lepton);
                } else if (isFNP(lepton)) {
                    m_fnpLeps.push_back(lepton);
                    if (isSig) m_fnpSigLeps.push_back(lepton);
                    else if (isInv) m_fnpInvLeps.push_back(lepton);
                }
            }
        }
        m_ztagged_idx1 = -1;
        m_ztagged_idx2 = -1;
        if (m_sigLeps.size() >= 2) {
            float Z_diff = FLT_MAX;
            for (uint ii = 0; ii < m_sigLeps.size(); ++ii) {
                Susy::Lepton *lep_ii = m_sigLeps.at(ii);
                for (uint jj = ii+1; jj < m_sigLeps.size(); ++jj) {
                    Susy::Lepton *lep_jj = m_sigLeps.at(jj);
                    bool SF = lep_ii->isEle() == lep_jj->isEle();
                    bool OS = lep_ii->q * lep_jj->q < 0;
                    if (!SF || !OS) continue;
                    float Z_diff_cf = fabs((*lep_ii+*lep_jj).M() - ZMASS);
                    if (Z_diff_cf < Z_diff) {
                        Z_diff = Z_diff_cf;
                        m_ztagged_idx1 = ii;
                        m_ztagged_idx2 = jj;
                    }
                }
            }
        }
        bool ztagged = m_ztagged_idx1 >= 0 && m_ztagged_idx2 >=0;
        LeptonVector prefTrigLeptons;
        LeptonVector allTrigLeptons;
        allTrigLeptons.reserve(m_sigLeps.size() + m_invLeps.size());
        allTrigLeptons.insert( allTrigLeptons.end(), m_sigLeps.begin(), m_sigLeps.end() );
        allTrigLeptons.insert( allTrigLeptons.end(), m_invLeps.begin(), m_invLeps.end() );
        // Define region specific globals
        if ((m_baseline_DF || m_baseline_SS) && m_sigLeps.size() == 2) {
            prefTrigLeptons = m_sigLeps;
        } else if ((m_fake_baseline_DF || m_baseline_SS_den) && m_sigLeps.size() == 1 && m_invLeps.size() == 1) {
            prefTrigLeptons = m_sigLeps;
        } else if (m_zjets_3l && m_sigLeps.size() == 3 && ztagged) {
            m_ZLeps.push_back( m_sigLeps.at(m_ztagged_idx1) );
            m_ZLeps.push_back( m_sigLeps.at(m_ztagged_idx2) );
            m_probeLep_idx = -1;
            if      (m_ztagged_idx1 != 0 && m_ztagged_idx2 != 0) { m_probeLep_idx = 0; }
            else if (m_ztagged_idx1 != 1 && m_ztagged_idx2 != 1) { m_probeLep_idx = 1; }
            else if (m_ztagged_idx1 != 2 && m_ztagged_idx2 != 2) { m_probeLep_idx = 2; }
            m_probeLeps.push_back(m_sigLeps.at(m_probeLep_idx));
            prefTrigLeptons.push_back(m_ZLeps.at(0));
            prefTrigLeptons.push_back(m_ZLeps.at(1));
        } else if (m_fake_zjets_3l && m_sigLeps.size() == 2 && m_invLeps.size() == 1 && ztagged) {
            m_ZLeps = m_sigLeps;
            m_probeLeps.push_back(m_invLeps.at(0));
            m_probeLep_idx = -1;
            if      (m_ztagged_idx1 != 0 && m_ztagged_idx2 != 0) { m_probeLep_idx = 0; }
            else if (m_ztagged_idx1 != 1 && m_ztagged_idx2 != 1) { m_probeLep_idx = 1; }
            else if (m_ztagged_idx1 != 2 && m_ztagged_idx2 != 2) { m_probeLep_idx = 2; }
            prefTrigLeptons.push_back(m_ZLeps.at(0));
            prefTrigLeptons.push_back(m_ZLeps.at(1));
        } else if (m_zjets2l_inc && m_sigLeps.size() >= 2 && ztagged) {
            m_ZLeps.push_back( m_sigLeps.at(m_ztagged_idx1) );
            m_ZLeps.push_back( m_sigLeps.at(m_ztagged_idx2) );
            prefTrigLeptons.push_back(m_ZLeps.at(0));
            prefTrigLeptons.push_back(m_ZLeps.at(1));
        } else {
            // These events should be removed by the cutflow requirements
            // Need to define ZLeps to be apply some selection though
            m_ZLeps = m_sigLeps;
            allTrigLeptons.clear();
        }
        int year = sl->nt->evt()->treatAsYear;
        // Implement trigger strategy
        m_trigLep_idx0 = -1;
        m_trigLep_idx1 = -1;
        m_firedTrig = "";

        for (auto const& it : m_single_lep_pT_thresholds) { m_triggerPass.emplace(it.first, false);}
        for (auto const& it : m_dilepton_pT_thresholds) { m_triggerPass.emplace(it.first, false);}

        for (const LeptonVector& trigLeps : {prefTrigLeptons, allTrigLeptons}) {
            for (uint idx0 = 0; idx0 < m_leps.size(); idx0++) {
            for (uint idx1 = idx0 + 1; idx1 < m_leps.size(); idx1++) {
                Susy::Lepton* lep0 = m_leps.at(idx0);
                if (std::find(trigLeps.begin(), trigLeps.end(), lep0) == trigLeps.end()) continue;
                Susy::Lepton* lep1 = m_leps.at(idx1);
                if (std::find(trigLeps.begin(), trigLeps.end(), lep1) == trigLeps.end()) continue;

                vector<string> dilepton_trigs;
                if (lep0->isEle() == lep1->isEle()) {
                    dilepton_trigs = lep0->isEle() ? m_dielectron_trigs.at(year) : m_dimuon_trigs.at(year);
                } else {
                    dilepton_trigs = m_diff_flav_trigs.at(year);
                    // pT thresholds for DF trigs assume electron is lep0
                    if (lep1->isEle()) { std::swap(lep0, lep1); }
                }
                for (const string& trig_name : dilepton_trigs ) {
                    float pt_thresh0 = m_dilepton_pT_thresholds.at(trig_name).first;
                    float pt_thresh1 = m_dilepton_pT_thresholds.at(trig_name).second;
                    bool pass = is_2lep_trig_matched(sl, trig_name, lep0, lep1, pt_thresh0, pt_thresh1);
                    m_triggerPass.at(trig_name) |= pass;
                    if (m_firedTrig == "" && pass) {
                       m_trigLep_idx0 = idx0;
                       m_trigLep_idx1 = idx1;
                       m_firedTrig = trig_name;
                    }
                }
            }
            }
            for (uint idx = 0; idx < m_leps.size(); idx++) {
                Susy::Lepton* lep = m_leps.at(idx);
                if (std::find(trigLeps.begin(), trigLeps.end(), lep) == trigLeps.end()) continue;
                vector<string> single_lep_trigs = lep->isEle() ? m_single_ele_trigs.at(year) : m_single_mu_trigs.at(year);
                for (const string& trig_name : single_lep_trigs ) {
                    float pt_thresh = m_single_lep_pT_thresholds.at(trig_name);
                    bool pass = is_1lep_trig_matched(sl, trig_name, lep, pt_thresh);
                    m_triggerPass.at(trig_name) |= pass;
                    if (m_firedTrig == "" && pass) {
                       m_trigLep_idx0 = idx;
                       m_firedTrig = trig_name;
                    }
                }
            }
        }
        ////////////////////////////////////////////////////////////////////////////
        // Combined triggers

        bool passSingleLepTrig = false;
        if (year == 2015) {
            passSingleLepTrig |= m_triggerPass.at("HLT_e24_lhmedium_L1EM20VH")
                              || m_triggerPass.at("HLT_e60_lhmedium")
                              || m_triggerPass.at("HLT_e120_lhloose")
                              || m_triggerPass.at("HLT_mu20_iloose_L1MU15")
                              || m_triggerPass.at("HLT_mu40");
        } else if (year == 2016 || year == 2017 || year == 2018) {
            passSingleLepTrig |= m_triggerPass.at("HLT_e26_lhtight_nod0_ivarloose")
                              || m_triggerPass.at("HLT_e60_lhmedium_nod0")
                              || m_triggerPass.at("HLT_e140_lhloose_nod0")
                              || m_triggerPass.at("HLT_mu26_ivarmedium")
                              || m_triggerPass.at("HLT_mu50");
        }
        m_triggerPass.emplace("singleLepTrigs", passSingleLepTrig);

        bool passDilepTrig = false;
        if (year == 2015) {
            passDilepTrig |= m_triggerPass.at("HLT_2e12_lhloose_L12EM10VH")
                          || m_triggerPass.at("HLT_2mu10")
                          || m_triggerPass.at("HLT_mu18_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_mu24");
        } else if (year == 2016) {
            passDilepTrig |= m_triggerPass.at("HLT_2e17_lhvloose_nod0")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          || m_triggerPass.at("HLT_2mu14")
                          || m_triggerPass.at("HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        } else if (year == 2017) {
            passDilepTrig |= m_triggerPass.at("HLT_2e24_lhvloose_nod0")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          || m_triggerPass.at("HLT_2mu14")
                          || m_triggerPass.at("HLT_e26_lhmedium_nod0_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        } else if (year == 2018) {
            passDilepTrig |= m_triggerPass.at("HLT_2e24_lhvloose_nod0")
                          || m_triggerPass.at("HLT_2e17_lhvloose_nod0_L12EM15VHI")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          || m_triggerPass.at("HLT_2mu14")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e26_lhmedium_nod0_mu8noL1")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        }
        m_triggerPass.emplace("dilepTrigs", passDilepTrig);

        bool passTrig = passSingleLepTrig || passDilepTrig;
        m_triggerPass.emplace("lepTrigs", passTrig);

        // Jigsaw variables
        if (m_leps.size() >= 2) {
            // build the object map for the calculator
            // the TTMET2LW calculator expects "leptons" and "met"
            std::map<std::string, std::vector<TLorentzVector>> object_map;
            object_map["leptons"] = { *m_leps.at(0), *m_leps.at(1) };
            object_map["met"] = { m_MET };
            m_calculator.load_event(object_map);
            m_jigsaw_vars = m_calculator.variables();
        }

        ////////////////////////////////////////////////////////////////////////
        return true; // All events pass this cut
    };

    return true;
}
void add_cleaning_cuts(Superflow* sf) {
    *sf << CutName("Pass GRL") << [](Superlink* sl) -> bool {
        return (sl->tools->passGRL(m_cutflags));
    };
    *sf << CutName("Error flags") << [](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(m_cutflags)
                && sl->tools->passTileErr(m_cutflags)
                && sl->tools->passSCTErr(m_cutflags)
                && sl->tools->passTTC(m_cutflags));
    };
    *sf << CutName("pass Good Vertex") << [](Superlink * sl) -> bool {
        return (sl->tools->passGoodVtx(m_cutflags));
    };
    *sf << CutName("pass bad muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passBadMuon(sl->preMuons));
    };
    //*sf << CutName("pass cosmic muon veto") << [](Superlink* sl) -> bool {
    //    return (sl->tools->passCosmicMuon(sl->baseMuons));
    //};
    *sf << CutName("pass jet cleaning") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };
}
void add_analysis_cuts(Superflow* sf) {
    ////////////////////////////////////////////////////////////////////////////
    // Baseline Selections
    if (m_baseline_DF || m_baseline_SS || m_fake_baseline_DF || m_baseline_SS_den) {
        *sf << CutName("2 baseline leptons") << [](Superlink* /*sl*/) -> bool {
            return m_leps.size() == 2;
        };

        if (m_baseline_DF || m_baseline_SS) {
            *sf << CutName("2 signal leptons") << [](Superlink* /*sl*/) -> bool {
                return (m_sigLeps.size() == 2);
            };
        } else if (m_fake_baseline_DF || m_baseline_SS_den) {
            *sf << CutName("1 inverted and signal lepton") << [](Superlink* /*sl*/) -> bool {
                return (m_invLeps.size() == 1 && m_sigLeps.size() == 1);
            };
        }
        if (m_baseline_DF || m_fake_baseline_DF) {
            *sf << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
                return (m_leps.at(0)->q * m_leps.at(1)->q < 0);
            };
            *sf << CutName("dilepton flavor (emu/mue)") << [](Superlink* /*sl*/) -> bool {
                return (m_leps.at(0)->isEle() != m_leps.at(1)->isEle());
            };
        } else if (m_baseline_SS || m_baseline_SS_den) {
            *sf << CutName("same sign") << [](Superlink* /*sl*/) -> bool {
                return (m_leps.at(0)->q * m_leps.at(1)->q > 0);
            };
            *sf << CutName("if SF, then |mll - mZ| > 20") << [](Superlink* /*sl*/) -> bool {
                if (m_leps.at(0)->isEle() == m_leps.at(1)->isEle()) {
                    TLorentzVector dilepP4 = *m_leps.at(0) + *m_leps.at(1);
                    return fabs(dilepP4.M() - ZMASS) > 20.0;
                } else {
                    return true;
                }
            };
        }
        *sf << CutName("m_ll > 20 GeV") << [](Superlink* /*sl*/) -> bool {
            TLorentzVector dilepP4 = *m_leps.at(0) + *m_leps.at(1);
            return dilepP4.M() > 20.0;
        };
    ////////////////////////////////////////////////////////////////////////////
    // Z+Jets Fake Factor Selections
    } else if (m_zjets_3l || m_fake_zjets_3l || m_zjets2l_inc) {
        *sf << CutName("3 baseline leptons") << [](Superlink* /*sl*/) -> bool {
            return (m_leps.size() == 3);
        };
        if (m_zjets_3l) {
            *sf << CutName("3 signal and 0 inverted leptons") << [](Superlink* /*sl*/) -> bool {
                return (m_sigLeps.size() == 3 && m_invLeps.size() == 0);
            };
        } else if (m_fake_zjets_3l) {
            *sf << CutName("2 signal and 1 inverted lepton") << [](Superlink* /*sl*/) -> bool {
                return (m_sigLeps.size() == 2 && m_invLeps.size() == 1);
            };
        } else if (m_zjets2l_inc) {
            *sf << CutName(">=2 signal leptons") << [](Superlink* /*sl*/) -> bool {
                return (m_sigLeps.size() >= 2);
            };
        }
        *sf << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
            return (m_ZLeps.at(0)->q * m_ZLeps.at(1)->q < 0);
        };
        *sf << CutName("Z dilepton flavor (ee/mumu)") << [](Superlink* /*sl*/) -> bool {
            return m_ZLeps.at(0)->isEle() == m_ZLeps.at(1)->isEle();
        };
        *sf << CutName("|mZ_ll - Zmass| < 10 GeV") << [](Superlink* /*sl*/) -> bool {
            TLorentzVector ZLepsP4 = *m_ZLeps.at(0) + *m_ZLeps.at(1);
            return fabs(ZLepsP4.M() - ZMASS) < 10;
        };
    }
    *sf << CutName("pass trigger") << [](Superlink* /*sl*/) -> bool {
        return m_triggerPass.at("lepTrigs");
    };
}

void add_4bcutflow_cuts(Superflow* sf) {
    *sf << CutName("Error flags") << [](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(m_cutflags)
                && sl->tools->passTileErr(m_cutflags)
                && sl->tools->passSCTErr(m_cutflags)
                && sl->tools->passTTC(m_cutflags));
    };
    *sf << CutName("pass Good Vertex") << [](Superlink * sl) -> bool {
        return (sl->tools->passGoodVtx(m_cutflags));
    };
    *sf << CutName("pass Trigger") << [](Superlink * sl) -> bool {
        return (sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, "HLT_e17_lhloose_nod0_mu14"));
    };

    *sf << CutName("pass cleaing") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets)
                && sl->tools->passBadMuon(sl->preMuons));
    };
    *sf << CutName("exactly two base leptons") << [](Superlink* /*sl*/) -> bool {
        return m_leps.size() == 2;
    };

    *sf << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
        return (m_leps.at(0)->q * m_leps.at(1)->q < 0);
    };

    *sf << CutName("isolation") << [](Superlink* /*sl*/) -> bool {
        bool passIso1 = m_leps.at(0)->isEle() ? m_leps.at(0)->isoGradient : m_leps.at(0)->isoFCLoose;
        bool passIso2 = m_leps.at(1)->isEle() ? m_leps.at(1)->isoGradient : m_leps.at(1)->isoFCLoose;
        return passIso1 && passIso2;
    };

    *sf << CutName("exactly two signal leptons") << [](Superlink* /*sl*/) -> bool {
        return (m_sigLeps.size() == 2);
    };

    *sf << CutName("truth origin") << [](Superlink* /*sl*/) -> bool {
        bool promptLep1 = m_sigLeps.at(0)->mcOrigin == 10 || // Top
                          m_sigLeps.at(0)->mcOrigin == 12 || // W
                          m_sigLeps.at(0)->mcOrigin == 13 || // Z
                          m_sigLeps.at(0)->mcOrigin == 14 || // Higgs
                          m_sigLeps.at(0)->mcOrigin == 43;   // Diboson
        bool promptLep2 = m_sigLeps.at(1)->mcOrigin == 10 || // Top
                          m_sigLeps.at(1)->mcOrigin == 12 || // W
                          m_sigLeps.at(1)->mcOrigin == 13 || // Z
                          m_sigLeps.at(1)->mcOrigin == 14 || // Higgs
                          m_sigLeps.at(1)->mcOrigin == 43;   // Diboson
        return promptLep1 && promptLep2;
    };

    *sf << CutName("eta requirement") << [](Superlink* /*sl*/) -> bool {
        float eta1 = fabs(m_sigLeps.at(0)->eta);
        float eta2 = fabs(m_sigLeps.at(1)->eta);
        bool passEta1 = m_sigLeps.at(0)->isEle() ? eta1 < 2.47 : eta1 < 2.4;
        bool passEta2 = m_sigLeps.at(1)->isEle() ? eta2 < 2.47 : eta2 < 2.4;
        return passEta1 && passEta2;
    };

    *sf << CutName("pass HLT_e17_lhloose_nod0_mu14") << [](Superlink* /*sl*/) -> bool {
        return (m_triggerPass.at("HLT_e17_lhloose_nod0_mu14"));
    };

    *sf << CutName("m_ll > 20 GeV") << [](Superlink* /*sl*/) -> bool {
        TLorentzVector dilepP4 = *m_leps.at(0) + *m_leps.at(1);
        return dilepP4.M() > 20.0;
    };

    *sf << CutName("lep1Pt > 25GeV") << [](Superlink* /*sl*/) -> bool {
        return m_sigLeps.at(0)->Pt() > 25.0;
    };

    *sf << CutName("lep2Pt > 20GeV") << [](Superlink* /*sl*/) -> bool {
        return m_sigLeps.at(1)->Pt() > 20.0;
    };

    *sf << CutName("MET > 250GeV") << [](Superlink* /*sl*/) -> bool {
        return m_MET.Pt() > 250.0;
    };
}

void add_event_variables(Superflow* sf) {
    // Event weights
    *sf << NewVar("event weight (multi period)"); {
        *sf << HFTname("eventweight_multi");
        *sf << [](Superlink* sl, var_double*) -> double {
            return sl->weights->susynt_multi
                 * sl->weights->lepSf
                 * sl->weights->jvtSf
                 * sl->weights->btagSf
                 * sl->weights->trigSf
                 * sl->nt->evt()->wPileup // multi-period pileup weight
                 ;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("event weight (single period)"); {
        *sf << HFTname("eventweight_single");
        *sf << [](Superlink* sl, var_double*) -> double {
            return sl->weights->susynt
                 * sl->weights->lepSf
                 * sl->weights->jvtSf
                 * sl->weights->btagSf
                 * sl->weights->trigSf
                 * (sl->nt->evt()->wPileup / sl->nt->evt()->wPileup_period)
                 ;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("Period weight"); {
        *sf << HFTname("period_weight");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup_period;};
        *sf << SaveVar();
    }

    *sf << NewVar("Monte-Carlo generator event weight"); {
        *sf << HFTname("w");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->w;};
        *sf << SaveVar();
    }

    // Scale Factors
    *sf << NewVar("Pile-up weight (Multi-period)"); {
        *sf << HFTname("pupw_multi");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup;};
        *sf << SaveVar();
    }

    *sf << NewVar("Pile-up weight (Single-period)"); {
        *sf << HFTname("pupw_single");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup / sl->nt->evt()->wPileup_period;};
        *sf << SaveVar();
    }

    *sf << NewVar("Lepton scale factor"); {
        *sf << HFTname("lepSf");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->lepSf;};
        *sf << SaveVar();
    }

    *sf << NewVar("Trigger scale factor"); {
        *sf << HFTname("trigSf");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->trigSf;};
        *sf << SaveVar();
    }

    *sf << NewVar("B-tag scale factor"); {
        *sf << HFTname("btagSf");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->btagSf;};
        *sf << SaveVar();
    }

    *sf << NewVar("JVT scale factor"); {
        *sf << HFTname("jvtSf");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->jvtSf;};
        *sf << SaveVar();
    }

    // Event identifiers
    *sf << NewVar("Event run number"); {
        *sf << HFTname("runNumber");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->run; };
        *sf << SaveVar();
    }

    *sf << NewVar("Event number"); {
        *sf << HFTname("eventNumber");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->eventNumber; };
        *sf << SaveVar();
    }

    *sf << NewVar("lumi block"); {
        *sf << HFTname("lb");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->lb; };
        *sf << SaveVar();
    }

    *sf << NewVar("is Monte Carlo"); {
        *sf << HFTname("isMC");
        *sf << [](Superlink* sl, var_bool*) -> bool { return sl->isMC; };
        *sf << SaveVar();
    }

    *sf << NewVar("mcChannel (dsid)"); {
        *sf << HFTname("dsid");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->isMC ? sl->nt->evt()->mcChannel : 0.0;};
        *sf << SaveVar();
    }

    *sf << NewVar("treatAsYear"); {
        *sf << HFTname("treatAsYear");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->treatAsYear; };
        *sf << SaveVar();
    }

    // Event properties
    *sf << NewVar("Average Mu"); {
        *sf << HFTname("avgMu");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMu; };
        *sf << SaveVar();
    }

    *sf << NewVar("Average Mu (Data DF)"); {
        *sf << HFTname("avgMuDataSF");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMuDataSF; };
        *sf << SaveVar();
    }

    *sf << NewVar("Actual Mu"); {
        *sf << HFTname("actualMu");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMu; };
        *sf << SaveVar();
    }

    *sf << NewVar("Actual Mu (Data SF)"); {
        *sf << HFTname("actualMuDataSF");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMuDataSF; };
        *sf << SaveVar();
    }

    *sf << NewVar("Number of vertices"); {
        *sf << HFTname("nVtx");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->nVtx; };
        *sf << SaveVar();
    }

    *sf << NewVar("Number of tracks associated with primary vertex"); {
        *sf << HFTname("nTracksAtPV");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->nTracksAtPV; };
        *sf << SaveVar();
    }

    *sf << NewVar("pT ordering of signal and inverted lepton types"); {
        *sf << HFTname("recoLepOrderType");
        *sf << [](Superlink* /*sl*/, var_int*) -> int {
            bool l0isSig = false, l1isSig = false, l2isSig = false;
            bool l0isInv = false, l1isInv = false, l2isInv = false;
            if (m_leps.size() >= 2) {
                l0isSig = isSignal(m_leps.at(0));
                l1isSig = isSignal(m_leps.at(1));
                l0isInv = isInverted(m_leps.at(0));
                l1isInv = isInverted(m_leps.at(1));
                if (m_leps.size() >= 3) {
                    l2isSig = isSignal(m_leps.at(2));
                    l2isInv = isInverted(m_leps.at(2));
                }
            }
            if (m_leps.size() == 2) {
                if (l0isSig && l1isSig) return 1;
                if (l0isSig && l1isInv) return 2;
                if (l0isInv && l1isSig) return 3;
                if (l0isInv && l1isInv) return 4;
            } else if (m_leps.size() == 3) {
                if (l0isSig && l1isSig && l2isSig) return 5;
                if (l0isSig && l1isSig && l2isInv) return 6;
                if (l0isSig && l1isInv && l2isSig) return 7;
                if (l0isInv && l1isSig && l2isSig) return 8;
                if (l0isSig && l1isInv && l2isInv) return 9;
                if (l0isInv && l1isSig && l2isInv) return 10;
                if (l0isInv && l1isInv && l2isSig) return 11;
                if (l0isInv && l1isInv && l2isInv) return 12;
            }
            return 0;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("pT ordering of prompt and fnp lepton types"); {
        *sf << HFTname("truthLepOrderType");
        *sf << [](Superlink* sl, var_int*) -> int {
            if (!sl->isMC) return -1;
            bool l0isPmt = false, l1isPmt = false, l2isPmt = false;
            bool l0isFnp = false, l1isFnp = false, l2isFnp = false;
            if (m_leps.size() >= 2) {
                l0isPmt = isPrompt(m_leps.at(0));
                l1isPmt = isPrompt(m_leps.at(1));
                l0isFnp = isFNP(m_leps.at(0));
                l1isFnp = isFNP(m_leps.at(1));
                if (m_leps.size() >= 3) {
                    l2isPmt = isPrompt(m_leps.at(2));
                    l2isFnp = isFNP(m_leps.at(2));
                }
            }
            if (m_leps.size() == 2) {
                if (l0isPmt && l1isPmt) return 1;
                if (l0isPmt && l1isFnp) return 2;
                if (l0isFnp && l1isPmt) return 3;
                if (l0isFnp && l1isFnp) return 4;
            } else if (m_leps.size() == 3) {
                if (l0isPmt && l1isPmt && l2isPmt) return 5;
                if (l0isPmt && l1isPmt && l2isFnp) return 6;
                if (l0isPmt && l1isFnp && l2isPmt) return 7;
                if (l0isFnp && l1isPmt && l2isPmt) return 8;
                if (l0isPmt && l1isFnp && l2isFnp) return 9;
                if (l0isFnp && l1isPmt && l2isFnp) return 10;
                if (l0isFnp && l1isFnp && l2isPmt) return 11;
                if (l0isFnp && l1isFnp && l2isFnp) return 12;
            }
            return 0;
        };
        *sf << SaveVar();
    }
}

void add_trigger_variables(Superflow* sf) {
    ////////////////////////////////////////////////////////////////////////////
    // Trigger Variables
    // ADD_*_TRIGGER_VAR preprocessor defined

    ////////////////////////////////////////////////////////////////////////////
    // 2015
    ADD_LEP_TRIGGER_VAR(HLT_e24_lhmedium_L1EM20VH)
    ADD_LEP_TRIGGER_VAR(HLT_e60_lhmedium)
    ADD_LEP_TRIGGER_VAR(HLT_e120_lhloose)

    ADD_LEP_TRIGGER_VAR(HLT_2e12_lhloose_L12EM10VH)

    ADD_LEP_TRIGGER_VAR(HLT_mu20_iloose_L1MU15)
    ADD_LEP_TRIGGER_VAR(HLT_mu40)

    ADD_LEP_TRIGGER_VAR(HLT_2mu10)
    ADD_LEP_TRIGGER_VAR(HLT_mu18_mu8noL1)

    ADD_LEP_TRIGGER_VAR(HLT_e17_lhloose_mu14)
    ADD_LEP_TRIGGER_VAR(HLT_e7_lhmedium_mu24)

    ////////////////////////////////////////////////////////////////////////////
    // 2016
    ADD_LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0)
    ADD_LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1)

    ////////////////////////////////////////////////////////////////////////////
    // 2016-2018

    ADD_LEP_TRIGGER_VAR(HLT_e26_lhtight_nod0_ivarloose)
    ADD_LEP_TRIGGER_VAR(HLT_e60_lhmedium_nod0)
    ADD_LEP_TRIGGER_VAR(HLT_e140_lhloose_nod0)

    ADD_LEP_TRIGGER_VAR(HLT_mu26_ivarmedium)
    ADD_LEP_TRIGGER_VAR(HLT_mu50)

    ADD_LEP_TRIGGER_VAR(HLT_2mu14)
    ADD_LEP_TRIGGER_VAR(HLT_mu22_mu8noL1)

    ADD_LEP_TRIGGER_VAR(HLT_e17_lhloose_nod0_mu14)
    ADD_LEP_TRIGGER_VAR(HLT_e7_lhmedium_nod0_mu24)

    ////////////////////////////////////////////////////////////////////////////
    // 2017-2018
    ADD_LEP_TRIGGER_VAR(HLT_2e24_lhvloose_nod0)

    ADD_LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_mu8noL1)

    ////////////////////////////////////////////////////////////////////////////
    // 2018
    // L1_2EM15VHI was accidentally prescaled in periods B5-B8 of 2017
    // (runs 326834-328393) with an effective reduction of 0.6 fb-1
    ADD_LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0_L12EM15VHI)

    ////////////////////////////////////////////////////////////////////////////
    // Combined trigger

    *sf << NewVar("Pass single lepton triggers"); {
        *sf << HFTname("passSingleLepTrigs");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool {
            return m_triggerPass.at("singleLepTrigs");
        };
        *sf << SaveVar();
    }
    *sf << NewVar("Pass dilepton triggers"); {
        *sf << HFTname("passDilepTrigs");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool {
            return m_triggerPass.at("dilepTrigs");
        };
        *sf << SaveVar();
    }
    *sf << NewVar("Pass single or dilepton triggers"); {
        *sf << HFTname("passLepTrigs");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool {
            return m_triggerPass.at("lepTrigs");
        };
        *sf << SaveVar();
    }

    *sf << NewVar("Inverted lepton fired trigger"); {
        *sf << HFTname("trigMatchedToInvLep");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool {
            return (m_trigLep_idx0 >=0 && isInverted(m_leps.at(m_trigLep_idx0))) 
                || (m_trigLep_idx1 >=0 && isInverted(m_leps.at(m_trigLep_idx1)));
        };
        *sf << SaveVar();
    }
    *sf << NewVar("pT ordering of leptons firing trigger"); {
        *sf << HFTname("trigLepOrderType");
        *sf << [](Superlink* /*sl*/, var_int*) -> int {
            bool l0Fired = m_trigLep_idx0 == 0 || m_trigLep_idx1 == 0;
            bool l1Fired = m_trigLep_idx0 == 1 || m_trigLep_idx1 == 1;
            bool l2Fired = m_trigLep_idx0 == 2 || m_trigLep_idx1 == 2;
            if (m_leps.size() >= 2) {
                if ( l0Fired && !l1Fired) return 1;
                if (!l0Fired &&  l1Fired) return 2;
                if ( l0Fired &&  l1Fired) return 3;
            }
            if (m_leps.size() >= 3) {
                if ( l0Fired && !l1Fired && !l2Fired) return 4;
                if (!l0Fired &&  l1Fired && !l2Fired) return 5;
                if (!l0Fired && !l1Fired &&  l2Fired) return 6;
                if ( l0Fired &&  l1Fired && !l2Fired) return 7;
                if ( l0Fired && !l1Fired &&  l2Fired) return 8;
                if (!l0Fired &&  l1Fired &&  l2Fired) return 9;
            }
            return 0;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("Fired trigger"); {
        *sf << HFTname("firedTrig");
        *sf << [](Superlink* /*sl*/, var_int*) -> int {
            return m_trig_enum.at(m_firedTrig);
        };
        *sf << SaveVar();
    }
}
void add_lepton_variables(Superflow* sf) {

    ADD_LEPTON_VARS(lep);
    ADD_LEPTON_VARS(sigLep);
    ADD_LEPTON_VARS(invLep);
    ADD_LEPTON_VARS(ZLep);
    ADD_LEPTON_VARS(probeLep);
    add_lepton_property_flags(sf);
    add_lepton_property_indexes(sf);
}

void add_mc_lepton_variables(Superflow* sf) {
    ADD_LEPTON_VARS(promptLep);
    ADD_LEPTON_VARS(fnpLep);
    //ADD_LEPTON_VARS(promptSigLep);
    //ADD_LEPTON_VARS(promptInvLep);
    //ADD_LEPTON_VARS(fnpSigLep);
    //ADD_LEPTON_VARS(fnpInvLep);

    add_mc_lepton_property_flags(sf);
    add_mc_lepton_property_indexes(sf);
}
bool isSignal(const Susy::Lepton* lep, Superlink* sl) {
    if (lep == nullptr) return false;
    //auto it = find(sl->leptons->begin(), sl->leptons->end(), lepton);
    //return it != sl->leptons->end();
    if (lep->isEle()) {
        auto el = static_cast<const Susy::Electron*>(lep);
        return sl->tools->electronSelector().isSignal(el);
    } else /*isMu*/ {
        auto mu = static_cast<const Susy::Muon*>(lep);
        return sl->tools->muonSelector().isSignal(mu);
    }
}
bool isSignal(const Susy::Lepton* lep) {
    auto it = find(m_sigLeps.begin(), m_sigLeps.end(), lep);
    return it != m_sigLeps.end();
}

bool isInverted(const Susy::Lepton* lep, Superlink* sl) {
    if (lep == nullptr) return false;
    if (lep->isEle()) {
        auto el = static_cast<const Susy::Electron*>(lep);
        return sl->tools->electronSelector().isAntiID(el);
    } else /*isMu*/ {
        auto mu = static_cast<const Susy::Muon*>(lep);
        return sl->tools->muonSelector().isAntiID(mu);
    }
}
bool isInverted(const Susy::Lepton* lep) {
    auto it = find(m_invLeps.begin(), m_invLeps.end(), lep);
    return it != m_invLeps.end();
}
bool isPrompt(Susy::Lepton* lep) {
    IFF::Type t = get_IFF_class(lep);
    switch (t) {
        case IFF::Type::PromptElectron: return true;
        case IFF::Type::ChargeFlipPromptElectron: return true;
        //TODO case IFF::Type::NonPromptIsoElectron return true;
        case IFF::Type::PromptMuon: return true;
        default:
            return false;
    }
}
bool isFNP(Susy::Lepton* lep) {
    // Treats unknown truth types as fakes
    // Should aim to minimize unknowns
    // Reconsider this if unknowns are a problem
    return !(isPrompt(lep));
}
bool isUnknownTruth(Susy::Lepton* lep) {
    //IFF::Type t = get_IFF_class(lep);
    switch (get_IFF_class(lep)) {
        case IFF::Type::Unknown: return true;
        case IFF::Type::KnownUnknown: return true;
        default:
            return false;
    }
}

void add_lepton_property_flags(Superflow* sf) {
    *sf << NewVar("lepton is signal (i.e. ID)"); {
        *sf << HFTname("lepIsSig");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for (const Susy::Lepton* l : m_leps) { out.push_back( isSignal(l) ); }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("lepton is inverted (i.e. Anti-ID)"); {
        *sf << HFTname("lepIsInv");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for (const Susy::Lepton* l : m_leps) { out.push_back( isInverted(l) ); }
            return out;
        };
        *sf << SaveVar();
    }
}
void add_mc_lepton_property_flags(Superflow* sf) {
    *sf << NewVar("lepton is prompt"); {
        *sf << HFTname("lepIsPrompt");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (Susy::Lepton* l : m_leps) {
                bool result = sl->isMC ? isPrompt(l) : false;
                out.push_back(result);
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("lepton is fake or non-prompt"); {
        *sf << HFTname("lepIsFNP");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (Susy::Lepton* l : m_leps) {
                bool result = sl->isMC ? isFNP(l) : false;
                out.push_back(result);
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("lepton is fake or non-prompt"); {
        *sf << HFTname("lepTruthIsUnknown");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (Susy::Lepton* l : m_leps) {
                bool result = sl->isMC ? isUnknownTruth(l) : false;
                out.push_back(result);
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("lepton is Z-tagged"); {
        *sf << HFTname("lepIsZtagged");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for (Susy::Lepton* l : m_leps) {
                bool found = std::find(m_ZLeps.begin(), m_ZLeps.end(), l) != m_ZLeps.end();
                out.push_back(found);
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("lepton is trig matched"); {
        *sf << HFTname("lepIsTrigMatched");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out(m_leps.size(), false);
            if (m_trigLep_idx0 >= 0) out.at(m_trigLep_idx0) = true;
            if (m_trigLep_idx1 >= 0) out.at(m_trigLep_idx1) = true;
            return out;
        };
        *sf << SaveVar();
    }
}
void add_lepton_property_indexes(Superflow* sf) {
    *sf << NewVar("index of signal lepton (i.e. ID)"); {
        *sf << HFTname("sigLepIdx");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (isSignal(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of inverted leptons (i.e. anti-ID)"); {
        *sf << HFTname("invLepIdx");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (isInverted(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of probe leptons"); {
        *sf << HFTname("probeLepIdx");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            //for (unsigned int idx=0; idx < m_leps.size(); idx++) {
            //    if (isProbe(m_leps.at(idx))) { out.push_back(idx); }
            //}
            out.push_back(m_probeLep_idx);
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of Z-tagged leptons"); {
        *sf << HFTname("ZLepIdx");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            out.push_back(m_ztagged_idx1);
            out.push_back(m_ztagged_idx2);
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of trigger matched lepton"); {
        *sf << HFTname("trigMatchedLepIdx");
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            if (m_trigLep_idx0 >= 0) out.push_back(m_trigLep_idx0);
            if (m_trigLep_idx1 >= 0) out.push_back(m_trigLep_idx1);
            return out;
        };
        *sf << SaveVar();
    }
}
void add_mc_lepton_property_indexes(Superflow* sf) {
    *sf << NewVar("index of prompt leptons"); {
        *sf << HFTname("promptLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isPrompt(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of fake or non-prompt leptons"); {
        *sf << HFTname("fnpLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isFNP(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of prompt signal leptons"); {
        *sf << HFTname("promptSigLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isPrompt(m_leps.at(idx)) && isSignal(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of prompt inverted leptons"); {
        *sf << HFTname("promptInvLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isPrompt(m_leps.at(idx)) && isInverted(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of fake or non-prompt signal leptons"); {
        *sf << HFTname("fnpSigLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isFNP(m_leps.at(idx)) && isSignal(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("index of fake or non-prompt inverted leptons"); {
        *sf << HFTname("fnpInvLepIdx");
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for (unsigned int idx=0; idx < m_leps.size(); idx++) {
                if (sl->isMC && isFNP(m_leps.at(idx)) && isInverted(m_leps.at(idx))) { out.push_back(idx); }
            }
            return out;
        };
        *sf << SaveVar();
    }
}

void add_jet_variables(Superflow* sf) {
    *sf << NewVar("number of jets"); {
        *sf << HFTname("nJets");
        *sf << [](Superlink* sl, var_int*) -> int {return sl->jets->size(); };
        *sf << SaveVar();
    }

    *sf << NewVar("number of light jets"); {
        *sf << HFTname("nLightJets");
        *sf << [](Superlink* /*sl*/, var_int*) -> int {return m_light_jets.size(); };
        *sf << SaveVar();
    }

    *sf << NewVar("number of b-tagged jets"); {
        *sf << HFTname("nBJets");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfBJets(*sl->jets); };
        *sf << SaveVar();
    }

    *sf << NewVar("number of forward jets"); {
        *sf << HFTname("nForwardJets");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfFJets(*sl->jets); };
        *sf << SaveVar();
    }

    *sf << NewVar("number of non-b-tagged jets"); {
        *sf << HFTname("nNonBJets");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->jets->size() - sl->tools->numberOfBJets(*sl->jets); };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-1 Pt"); {
        *sf << HFTname("jet1Pt");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Pt() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-1 Eta"); {
        *sf << HFTname("jet1Eta");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-1 Phi"); {
        *sf << HFTname("jet1Phi");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-2 Pt"); {
        *sf << HFTname("jet2Pt");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Pt() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-2 Eta"); {
        *sf << HFTname("jet2Eta");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Eta() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("jet-2 Phi"); {
        *sf << HFTname("jet2Phi");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Phi() : -DBL_MAX;
        };
        *sf << SaveVar();
    }
}
void add_met_variables(Superflow* sf) {
    *sf << NewVar("transverse missing energy (Et)"); {
        *sf << HFTname("met");
        *sf << [](Superlink* sl, var_float*) -> double { return sl->met->Et; };
        *sf << SaveVar();
    }

    *sf << NewVar("transverse missing energy (Phi)"); {
        *sf << HFTname("metPhi");
        *sf << [](Superlink* sl, var_float*) -> double { return sl->met->phi; };
        *sf << SaveVar();
    }
}
void add_dilepton_variables(Superflow* sf) {
    *sf << NewVar("is e + e"); {
        *sf << HFTname("isElEl");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->isEle() && m_leps.at(1)->isEle(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is mu + mu"); {
        *sf << HFTname("isMuMu");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->isMu() && m_leps.at(1)->isMu(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is mu (lead) + e (sub)"); {
        *sf << HFTname("isMuEl");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->isMu() && m_leps.at(1)->isEle(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is e (lead) + mu (sub)"); {
        *sf << HFTname("isElMu");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->isEle() && m_leps.at(1)->isMu(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is opposite-sign"); {
        *sf << HFTname("isOS");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->q * m_leps.at(1)->q < 0; };
        *sf << SaveVar();
    }

    *sf << NewVar("is e + mu"); {
        *sf << HFTname("isDF");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_leps.at(0)->isEle() ^ m_leps.at(1)->isEle(); };
        *sf << SaveVar();
    }


    *sf << NewVar("mass of di-lepton system"); {
        *sf << HFTname("mll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_leps.at(0) + *m_leps.at(1)).M();};
        *sf << SaveVar();
    }

    *sf << NewVar("Pt of di-lepton system"); {
        *sf << HFTname("pTll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_leps.at(0) + *m_leps.at(1)).Pt();};
        *sf << SaveVar();
    }

    *sf << NewVar("Pt difference of di-lepton system"); {
        *sf << HFTname("dpTll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_leps.at(0)->Pt() - m_leps.at(1)->Pt(); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Eta of di-lepton system"); {
        *sf << HFTname("dEta_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return fabs(m_leps.at(0)->Eta() - m_leps.at(1)->Eta()); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Phi of di-lepton system"); {
        *sf << HFTname("dPhi_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return fabs(m_leps.at(0)->DeltaPhi(*m_leps.at(1))); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta R of di-lepton system"); {
        *sf << HFTname("dR_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_leps.at(0)->DeltaR(*m_leps.at(1)); };
        *sf << SaveVar();
    }
}
void add_Zlepton_variables(Superflow* sf) {
    *sf << NewVar("Z -> ee"); {
        *sf << HFTname("ZisElEl");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_ZLeps.at(0)->isEle() && m_ZLeps.at(1)->isEle(); };
        *sf << SaveVar();
    }

    *sf << NewVar("Z -> mumu"); {
        *sf << HFTname("ZisMuMu");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_ZLeps.at(0)->isMu() && m_ZLeps.at(1)->isMu(); };
        *sf << SaveVar();
    }

    *sf << NewVar("mass of Z-tagged leptons"); {
        *sf << HFTname("Zmass");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_ZLeps.at(0) + *m_ZLeps.at(1)).M();};
        *sf << SaveVar();
    }

    *sf << NewVar("Pt of Z-tagged leptons"); {
        *sf << HFTname("ZpT");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_ZLeps.at(0) + *m_ZLeps.at(1)).Pt();};
        *sf << SaveVar();
    }

    *sf << NewVar("Eta of Z-tagged leptons"); {
        *sf << HFTname("ZEta");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_ZLeps.at(0) + *m_ZLeps.at(1)).Eta();};
        *sf << SaveVar();
    }

    *sf << NewVar("Phi of Z-tagged leptons"); {
        *sf << HFTname("ZPhi");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return (*m_ZLeps.at(0) + *m_ZLeps.at(1)).Phi();};
        *sf << SaveVar();
    }

    *sf << NewVar("Pt difference of Z-tagged leptons"); {
        *sf << HFTname("dpT_ZLeps");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_ZLeps.at(0)->Pt() - m_ZLeps.at(1)->Pt(); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Eta of Z-tagged leptons"); {
        *sf << HFTname("dEta_ZLeps");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return fabs(m_ZLeps.at(0)->Eta() - m_ZLeps.at(1)->Eta()); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Phi of Z-tagged leptons"); {
        *sf << HFTname("dPhi_ZLeps");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return fabs(m_ZLeps.at(0)->DeltaPhi(*m_ZLeps.at(1))); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta R of Z-tagged leptons"); {
        *sf << HFTname("dR_ZLeps");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_ZLeps.at(0)->DeltaR(*m_ZLeps.at(1)); };
        *sf << SaveVar();
    }
}

void add_multi_object_variables(Superflow* sf) {
    // Jets and MET
    *sf << NewVar("delta Phi of leading jet and met"); {
        *sf << HFTname("dPhi_met_jet1");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Phi of subleading jet and met"); {
        *sf << HFTname("dPhi_met_jet2");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("stransverse mass"); {
        *sf << HFTname("MT2");
        *sf << [](Superlink* sl, var_float*) -> double {
            double mt2_ = kin::getMT2(m_sigLeps, *sl->met);
            return mt2_;
        };
        *sf << SaveVar();
    }

    // Leptons, Jets, and MET
    *sf << NewVar("Etmiss Rel"); {
        *sf << HFTname("metrel");
        *sf << [](Superlink* sl, var_float*) -> double { return kin::getMetRel(sl->met, m_sigLeps, *sl->jets); };
        *sf << SaveVar();
    }

    *sf << NewVar("Ht (m_Eff: lep + met + jet)"); {
        *sf << HFTname("ht");
        *sf << [](Superlink* sl, var_float*) -> double {
            double ht = sl->met->Et;
            for (const Susy::Lepton* l : m_leps) { ht += l->Pt(); }
            for (const Susy::Jet* j : *sl->jets) { ht += j->Pt(); }
            return ht;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("reco-level max(HT,pTV) "); {
        *sf << HFTname("max_HT_pTV_reco");
        *sf << [](Superlink* sl, var_float*) -> double {
            if (m_leps.size() < 2) return -DBL_MAX;
            double ht = 0.0;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                if (sl->jets->at(i)->Pt() < 20) {continue;}
                ht += sl->jets->at(i)->Pt();
            }
            TLorentzVector VllP4 = *m_leps.at(0) + *m_leps.at(1);
            return max(ht, VllP4.Pt());
        };
        *sf << SaveVar();
    }
}

void add_jigsaw_variables(Superflow* sf) {
    //ADD_JIGSAW_VAR(H_11_SS)
    //ADD_JIGSAW_VAR(H_21_SS)
    //ADD_JIGSAW_VAR(H_12_SS)
    //ADD_JIGSAW_VAR(H_22_SS)
    //ADD_JIGSAW_VAR(H_11_S1)
    //ADD_JIGSAW_VAR(H_11_SS_T)
    //ADD_JIGSAW_VAR(H_21_SS_T)
    //ADD_JIGSAW_VAR(H_22_SS_T)
    //ADD_JIGSAW_VAR(H_11_S1_T)
    ADD_JIGSAW_VAR(shat)
    ADD_JIGSAW_VAR(pTT_T)
    //ADD_JIGSAW_VAR(pTT_Z)
    ADD_JIGSAW_VAR(RPT)
    //ADD_JIGSAW_VAR(RPZ)
    //ADD_JIGSAW_VAR(RPT_H_11_SS)
    //ADD_JIGSAW_VAR(RPT_H_21_SS)
    //ADD_JIGSAW_VAR(RPT_H_22_SS)
    //ADD_JIGSAW_VAR(RPZ_H_11_SS)
    //ADD_JIGSAW_VAR(RPZ_H_21_SS)
    //ADD_JIGSAW_VAR(RPZ_H_22_SS)
    //ADD_JIGSAW_VAR(RPT_H_11_SS_T)
    //ADD_JIGSAW_VAR(RPT_H_21_SS_T)
    //ADD_JIGSAW_VAR(RPT_H_22_SS_T)
    //ADD_JIGSAW_VAR(RPZ_H_11_SS_T)
    //ADD_JIGSAW_VAR(RPZ_H_21_SS_T)
    //ADD_JIGSAW_VAR(RPZ_H_22_SS_T)
    ADD_JIGSAW_VAR(gamInvRp1)
    ADD_JIGSAW_VAR(MDR)
    ADD_JIGSAW_VAR(DPB_vSS)
    //ADD_JIGSAW_VAR(costheta_SS)
    //ADD_JIGSAW_VAR(dphi_v_SS)
    //ADD_JIGSAW_VAR(dphi_v1_i1_ss)
    //ADD_JIGSAW_VAR(dphi_s1_s2_ss)
    //ADD_JIGSAW_VAR(dphi_S_I_ss)
    //ADD_JIGSAW_VAR(dphi_S_I_s1)
}
void add_miscellaneous_variables(Superflow* sf) {

    *sf << NewVar("|cos(theta_b)|"); {
        *sf << HFTname("abs_costheta_b");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {
            TLorentzVector lp, lm, ll;
            lp = m_leps.at(0)->q > 0 ? *m_leps.at(0) : *m_leps.at(1);
            lm = m_leps.at(0)->q < 0 ? *m_leps.at(0) : *m_leps.at(1);
            ll = lp + lm;

            TVector3 boost = ll.BoostVector();
            lp.Boost(-boost);
            lm.Boost(-boost);

            double deta = lp.Eta() - lm.Eta();
            double costheta_b = tanh(0.5 * deta);

            return fabs(costheta_b);
        };
        *sf << SaveVar();
    }
}


void add_Zll_probeLep_variables(Superflow* sf) {
    *sf << NewVar("Mlll: Invariant mass of 3lep system"); {
      *sf << HFTname("mlll");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
          return (*m_ZLeps.at(0) + *m_ZLeps.at(1) + *m_probeLeps.at(0)).M();
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and ZLep1"); {
      *sf << HFTname("dR_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_ZLeps.at(0)).DeltaR(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and ZLep2"); {
      *sf << HFTname("dR_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_ZLeps.at(1)).DeltaR(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and Z"); {
      *sf << HFTname("dR_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        TLorentzVector ZLepsP4 = *m_ZLeps.at(0) + *m_ZLeps.at(1);
        return ZLepsP4.DeltaR(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and ZLep1"); {
      *sf << HFTname("dPhi_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_ZLeps.at(0)).DeltaPhi(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and ZLep2"); {
      *sf << HFTname("dPhi_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_ZLeps.at(1)).DeltaPhi(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and Z"); {
      *sf << HFTname("dPhi_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        TLorentzVector ZLepsP4 = *m_ZLeps.at(0) + *m_ZLeps.at(1);
        return ZLepsP4.DeltaPhi(*m_probeLeps.at(0));
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and ZLep1"); {
      *sf << HFTname("dEta_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return fabs(m_probeLeps.at(0)->Eta() - m_ZLeps.at(0)->Eta());
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and ZLep2"); {
      *sf << HFTname("dEta_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return fabs(m_probeLeps.at(0)->Eta() - m_ZLeps.at(1)->Eta());
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and Z"); {
      *sf << HFTname("dEta_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        TLorentzVector ZLepsP4 = *m_ZLeps.at(0) + *m_ZLeps.at(1);
        return fabs(m_probeLeps.at(0)->Eta() - ZLepsP4.Eta()) ;
      };
      *sf << SaveVar();
    }
}

void add_weight_systematics(Superflow* sf) {
    *sf << NewSystematic("FTAG EFF B"); {
        *sf << WeightSystematic(SupersysWeight::FT_EFF_B_UP, SupersysWeight::FT_EFF_B_DN);
        *sf << TreeName("FT_EFF_B");
        *sf << SaveSystematic();
    }
    *sf << NewSystematic("shift in electron ID efficiency"); {
        *sf << WeightSystematic(SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_DN);
        *sf << TreeName("EL_EFF_ID");
        *sf << SaveSystematic();
    }
    *sf << NewSystematic("shift in electron ISO efficiency"); {
        *sf << WeightSystematic(SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_DN);
        *sf << TreeName("EL_EFF_Iso");
        *sf << SaveSystematic();
    }
    *sf << NewSystematic("shift in electron RECO efficiency"); {
        *sf << WeightSystematic(SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_DN);
        *sf << TreeName("EL_EFF_Reco");
        *sf << SaveSystematic();
    }
}
void add_shape_systematics(Superflow* sf) {
    *sf << NewSystematic("shift in e-gamma resolution (UP)"); {
        *sf << EventSystematic(NtSys::EG_RESOLUTION_ALL_UP);
        *sf << TreeName("EG_RESOLUTION_ALL_UP");
        *sf << SaveSystematic();
    }
    *sf << NewSystematic("shift in e-gamma resolution (DOWN)"); {
        *sf << EventSystematic(NtSys::EG_RESOLUTION_ALL_DN);
        *sf << TreeName("EG_RESOLUTION_ALL_DN");
        *sf << SaveSystematic();
    }
    *sf << NewSystematic("shift in e-gamma scale (UP)"); {
        *sf << EventSystematic(NtSys::EG_SCALE_ALL_UP);
        *sf << TreeName("EG_SCALE_ALL_UP");
        *sf << SaveSystematic();
    }
}

bool is_1lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep, float pt_min) {
    if(!lep) return false;
    if (lep->Pt() < pt_min) return false;
    bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
    if (!trig_fired) return false;
    bool trig_matched = sl->tools->triggerTool().lepton_trigger_match(lep, trig_name);
    if (trig_matched) return true;
    return false;
}

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2, float pt_min1, float pt_min2) {
    if(!lep1 || ! lep2) return false;
    if (lep1->Pt() < pt_min1 || lep2->Pt() < pt_min2) return false;
    bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
    if (!trig_fired) return false;
    bool trig_matched = false;
    if (lep1->isMu() && lep2->isEle()) {
        // electron must be first argument for different flavor triggers
        trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep2, lep1, trig_name);
    } else {
        trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep1, lep2, trig_name);
    }
    return trig_matched;
}


IFF::Type get_IFF_class(Susy::Lepton* lep) {
    IFF::Type result = IFF::Type::Unknown;
    if (lep->isEle()) {
        Susy::Electron* ele = static_cast<Susy::Electron*>(lep);
        const xAOD::Electron* aod_ele = to_iff_aod_electron(*ele);
        result = m_truthClassifier.classify(*aod_ele);
        delete aod_ele;
    } else if (lep->isMu()) {
        Susy::Muon* mu = static_cast<Susy::Muon*>(lep);
        const xAOD::Muon* aod_mu = to_iff_aod_muon(*mu);
        result =  m_truthClassifier.classify(*aod_mu);
        delete aod_mu;
    }
    return result;
}

int to_int(IFF::Type t) {
    switch(t) { // Needs to be in sync with IFFTruthClassifier/IFFTruthClassifierDefs.h
        case IFF::Type::Unknown:                  return 0;
        case IFF::Type::KnownUnknown:             return 1;
        case IFF::Type::PromptElectron:           return 2;
        case IFF::Type::ChargeFlipPromptElectron: return 3;
        case IFF::Type::NonPromptPhotonConv:      return 4;
        case IFF::Type::PromptMuon:               return 5;
        case IFF::Type::PromptPhotonConversion:   return 6;
        case IFF::Type::ElectronFromMuon:         return 7;
        case IFF::Type::TauDecay:                 return 8;
        case IFF::Type::BHadronDecay:             return 9;
        case IFF::Type::CHadronDecay:             return 10;
        case IFF::Type::LightFlavorDecay:         return 11;
        default:
            cout << "WARNING :: Unknown IFF type " << t << '\n';
    }
    return 0;
}

const xAOD::Electron* to_iff_aod_electron(Susy::Electron& ele) {
    xAOD::Electron* e = new xAOD::Electron();
    e->makePrivateStore();
    e->auxdata<int>("truthType") = ele.mcType;
    e->auxdata<int>("truthOrigin") = ele.mcOrigin;
    e->auxdata<int>("firstEgMotherTruthType") = ele.mcFirstEgMotherTruthType;
    e->auxdata<int>("firstEgMotherTruthOrigin") = ele.mcFirstEgMotherTruthOrigin;
    e->auxdata<int>("firstEgMotherPdgId") = ele.mcFirstEgMotherPdgId;
    e->setCharge(ele.q);
    return e;
}
const xAOD::Muon* to_iff_aod_muon(Susy::Muon& muo) {
    xAOD::Muon* m = new xAOD::Muon();
    m->makePrivateStore();
    m->auxdata<int>("truthType") = muo.mcType;
    m->auxdata<int>("truthOrigin") = muo.mcOrigin;
    m->auxdata<int>("firstEgMotherTruthType") = muo.mcFirstEgMotherTruthType;
    m->auxdata<int>("firstEgMotherTruthOrigin") = muo.mcFirstEgMotherTruthOrigin;
    m->auxdata<int>("firstEgMotherPdgId") = muo.mcFirstEgMotherPdgId;
    m->setCharge(muo.q);
    return m;
}

