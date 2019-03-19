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
#include <string>
#include <getopt.h>
#include <map>

// ROOT
#include "TChain.h"
#include "TVectorD.h"
#include "TF1.h"

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

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string ttree_name, bool verbose);
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain);
void set_global_variables(Superflow* cutflow);
void add_cleaning_cuts(Superflow* cutflow);
void add_analysis_cuts(Superflow* cutflow);
void add_event_variables(Superflow* cutflow);
void add_trigger_variables(Superflow* cutflow);
void add_lepton_variables(Superflow* cutflow);
void add_jet_variables(Superflow* cutflow);
void add_met_variables(Superflow* cutflow);
void add_dilepton_variables(Superflow* cutflow);
void add_multi_object_variables(Superflow* cutflow);
void add_jigsaw_variables(Superflow* cutflow);
void add_miscellaneous_variables(Superflow* cutflow);
void add_fakefactor_variables(Superflow* cutflow);
void add_zjets_fakefactor_variables(Superflow* cutflow);

void add_weight_systematics(Superflow* cutflow);
void add_shape_systematics(Superflow* cutflow);

// Selections (set with user input)
bool m_baseline_DF = false;
bool m_baseline_SF = false;
bool m_zjets_3l = false;
bool m_fake_baseline_DF = false;
bool m_fake_baseline_SF= false;
bool m_fake_zjets_3l = false;

// globals for use in superflow cuts and variables
static int m_cutflags = 0;
static JetVector m_light_jets;
static TLorentzVector m_dileptonP4;
static TLorentzVector m_MET;
static Susy::Lepton* m_lep1;
static Susy::Lepton* m_lep2;
static Susy::Electron *m_el0, *m_el1;
static Susy::Muon *m_mu0, *m_mu1;
static LeptonVector m_triggerLeptons;
static LeptonVector m_invLeps;
static Susy::Lepton* m_probeLep1;
static Susy::Lepton* m_probeLep2;

static TF1 pu_profile("pu_profile","gausn", -250, 250);

static jigsaw::JigsawCalculator m_calculator;
static std::map< std::string, float> m_jigsaw_vars;

// Helpful functions
int get_lepton_truth_class(Susy::Lepton* lepton);
bool is_antiID_lepton(Susy::Lepton* lepton);
bool is_ID_lepton(Superlink* sl, Susy::Lepton* lepton);

bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons);
#define ADD_1LEP_TRIGGER_VAR(trig_name, leptons) { \
    *cutflow << NewVar(#trig_name" trigger bit"); { \
        *cutflow << HFTname(#trig_name); \
        *cutflow << [=](Superlink* sl, var_bool*) -> bool { \
            return is_1lep_trig_matched(sl, #trig_name, leptons); \
        }; \
        *cutflow << SaveVar(); \
    } \
}

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2);
#define ADD_2LEP_TRIGGER_VAR(trig_name, lep1, lep2) { \
  *cutflow << NewVar(#trig_name" trigger bit"); { \
      *cutflow << HFTname(#trig_name); \
      *cutflow << [=](Superlink* sl, var_bool*) -> bool { \
          return is_2lep_trig_matched(sl, #trig_name, lep1, lep2); \
      }; \
      *cutflow << SaveVar(); \
  } \
}

// Addings a jigsaw variable
#define ADD_JIGSAW_VAR(var_name) { \
    *cutflow << NewVar(#var_name); { \
        *cutflow << HFTname(#var_name); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_jigsaw_vars.at(#var_name); }; \
        *cutflow << SaveVar(); \
    } \
}

// Adding main lepton variables
#define ADD_LEPTON_VARS(lep_name) { \
    *cutflow << NewVar(#lep_name" Pt"); { \
        *cutflow << HFTname(#lep_name"Pt"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Pt() : -DBL_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" Eta"); { \
        *cutflow << HFTname(#lep_name"Eta"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Eta(): -DBL_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" Phi"); { \
        *cutflow << HFTname(#lep_name"Phi"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Phi(): -DBL_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" Energy"); { \
        *cutflow << HFTname(#lep_name"E"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->E(): -DBL_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" charge"); { \
        *cutflow << HFTname(#lep_name"q"); \
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name ? m_##lep_name->q: -INT_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" flavor"); { \
        *cutflow << HFTname(#lep_name"Flav"); \
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name ? m_##lep_name->isEle() ? 0 : 1: -INT_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" d0sigBSCorr"); { \
      *cutflow << HFTname(#lep_name"_d0sigBSCorr"); \
      *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->d0sigBSCorr: -DBL_MAX; }; \
      *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" z0SinTheta"); { \
      *cutflow << HFTname(#lep_name"_z0SinTheta"); \
      *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->z0SinTheta(): -DBL_MAX; }; \
      *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" Truth Class"); { \
        *cutflow << HFTname(#lep_name"TruthClass"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? get_lepton_truth_class(m_##lep_name): -INT_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar(#lep_name" Iso"); { \
      *cutflow << HFTname(#lep_name"Iso"); \
      *cutflow << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
        vector<int> out; \
        if (!m_##lep_name) return out; \
        out.push_back(-1); \
        bool flag = false; \
        if (m_##lep_name->isoGradient)               { flag=true; out.push_back(0);} \
        if (m_##lep_name->isoGradientLoose)          { flag=true; out.push_back(1);} \
        if (m_##lep_name->isoLoose)                  { flag=true; out.push_back(2);} \
        if (m_##lep_name->isoLooseTrackOnly)         { flag=true; out.push_back(3);} \
        if (m_##lep_name->isoFixedCutTightTrackOnly) { flag=true; out.push_back(4);} \
        if (m_##lep_name->isoFCLoose)                { flag=true; out.push_back(5);} \
        if (m_##lep_name->isoFCTight)                { flag=true; out.push_back(6);} \
        if (m_##lep_name->isoFCTightTrackOnly)       { flag=true; out.push_back(7);} \
        if (!flag) out.push_back(8); \
        return out; \
      }; \
      *cutflow << SaveVar(); \
    } \
    *cutflow << NewVar(#lep_name" transverse mass"); { \
        *cutflow << HFTname(#lep_name"mT"); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { \
            if (!m_##lep_name) return -DBL_MAX; \
            double dphi = m_##lep_name->DeltaPhi(m_MET); \
            double pT2 = m_##lep_name->Pt()*m_MET.Pt(); \
            double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) )); \
            return lep_mT; \
        }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar("delta Phi of "#lep_name" and met"); { \
        *cutflow << HFTname("deltaPhi_met_"#lep_name); \
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { \
          return m_##lep_name ? abs(m_##lep_name->DeltaPhi(m_MET)) : -DBL_MAX; }; \
        *cutflow << SaveVar(); \
    } \
    \
    *cutflow << NewVar("dR between "#lep_name" and closest jet"); { \
      *cutflow << HFTname("dR_"#lep_name"_jet"); \
      *cutflow << [](Superlink* sl, var_double*) -> double { \
          if (!m_##lep_name) return -DBL_MAX; \
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
          for (Susy::Jet* jet : *sl->jets) { \
            float tmp_dphi = fabs(jet->DeltaPhi(*m_##lep_name)); \
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; \
          } \
          return dPhi; \
      }; \
      *cutflow << SaveVar(); \
    } \
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
    } else if (options.ana_selection == "baseline_SF") {
        m_baseline_SF = true;
        cout << "INFO :: Running baseline SF selections\n";
    } else if (options.ana_selection == "zjets3l") {
        m_zjets_3l = true;
        cout << "INFO :: Running Z+jets selections\n";
    } else if (options.ana_selection == "fake_baseline_DF") {
        m_fake_baseline_DF = true;
        cout << "INFO :: Running baseline DF denominator selections\n";
    } else if (options.ana_selection == "fake_baseline_SF") {
        m_fake_baseline_SF = true;
        cout << "INFO :: Running baseline SF denominator selections\n";
    } else if (options.ana_selection == "fake_zjets3l") {
        m_fake_zjets_3l = true;
        cout << "INFO :: Running Z+jets denominator selections\n";
    } else {
        cout << "ERROR :: Unknown analysis selection:" << options.ana_selection << '\n';
        exit(1);
    }
    // New TChain* added to heap, remember to delete later
    TChain* chain = create_new_chain(options.input, m_input_ttree_name, m_verbose);

    Long64_t tot_num_events = chain->GetEntries();
    options.n_events_to_process = (options.n_events_to_process < 0 ? tot_num_events : options.n_events_to_process);

    ////////////////////////////////////////////////////////////
    // Initialize & configure the analysis
    //  > Superflow inherits from SusyNtAna : TSelector
    ////////////////////////////////////////////////////////////
    Superflow* cutflow = create_new_superflow(options, chain);

    cout << options.ana_name << "    Total Entries: " << chain->GetEntries() << endl;
    //if (options.run_mode == SuperflowRunMode::single_event_syst) cutflow->setSingleEventSyst(nt_sys_);

    // Set variables for use in other cuts/vars. MUST ADD FIRST!
    set_global_variables(cutflow);

    // Event selections
    add_cleaning_cuts(cutflow);
    add_analysis_cuts(cutflow);

    // Output variables
    add_event_variables(cutflow);
    add_trigger_variables(cutflow);
    add_lepton_variables(cutflow);
    add_jet_variables(cutflow);
    add_met_variables(cutflow);
    add_dilepton_variables(cutflow);
    add_multi_object_variables(cutflow);
    add_jigsaw_variables(cutflow);
    add_miscellaneous_variables(cutflow);
    add_fakefactor_variables(cutflow);
    if (m_zjets_3l || m_fake_zjets_3l) {
        add_zjets_fakefactor_variables(cutflow);
    };

    // Systematics
    //add_weight_systematics(cutflow);
    //add_shape_systematics(cutflow);

    // Run Superflow
    chain->Process(cutflow, options.input.c_str(), options.n_events_to_process);

    // Clean up
    delete cutflow;
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
    Superflow* cutflow = new Superflow(); // initialize the cutflow
    cutflow->setAnaName(sf_options.ana_name);
    cutflow->setAnaType(m_ana_type);
    cutflow->setLumi(m_lumi);
    cutflow->setSampleName(sf_options.input);
    cutflow->setRunMode(sf_options.run_mode);
    cutflow->setCountWeights(m_print_weighted_cutflow);
    cutflow->setChain(chain);
    cutflow->setDebug(sf_options.dbg);
    cutflow->nttools().initTriggerTool(ChainHelper::firstFile(sf_options.input, 0.0));
    if(sf_options.suffix_name != "") {
        cutflow->setFileSuffix(sf_options.suffix_name);
    }
    if(sf_options.sumw_file_name != "") {
        cout << sf_options.ana_name
             << "    Reading sumw for sample from file: "
             << sf_options.sumw_file_name << endl;
        cutflow->setUseSumwFile(sf_options.sumw_file_name);
    }
    return cutflow;
}
void set_global_variables(Superflow* cutflow) {
    m_calculator.initialize("TTMET2LW");

    *cutflow << CutName("read in") << [](Superlink* sl) -> bool {
        ////////////////////////////////////////////////////////////////////////
        // Reset all globals used in cuts/variables
        m_cutflags = 0;
        m_light_jets.clear();
        m_dileptonP4 = {};
        m_MET = {};
        m_lep1 = m_lep2 = 0;
        m_el0 = m_el1 = 0;
        m_mu0 = m_mu1 = 0;
        m_triggerLeptons.clear();
        m_invLeps.clear();
        m_probeLep1 = m_probeLep2 = 0;

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

        for (Susy::Lepton* lepton : *sl->baseLeptons) {
            if (is_antiID_lepton(lepton)) { m_invLeps.push_back(lepton); }
        }
        int ztagged_idx1 = -1;
        int ztagged_idx2 = -1;
        if (sl->leptons->size() >= 3) {
            float Z_diff = FLT_MAX;
            for (uint ii = 0; ii < sl->leptons->size(); ++ii) {
                Susy::Lepton *lep_ii = sl->leptons->at(ii);
                for (uint jj = ii+1; jj < sl->leptons->size(); ++jj) {
                    Susy::Lepton *lep_jj = sl->leptons->at(jj);
                    float Z_diff_cf = fabs((*lep_ii+*lep_jj).M() - ZMASS);
                    if (Z_diff_cf < Z_diff) {
                        Z_diff = Z_diff_cf;
                        ztagged_idx1 = ii;
                        ztagged_idx2 = jj;
                    }
                }
            }
        }
        // Define the following for each region's ntuples
        //     m_lep1
        //     m_lep2
        //     m_probeLep1
        //     m_probeLep2
        //     m_triggerLeptons
        if ((m_baseline_DF || m_baseline_SF) && sl->leptons->size() == 2) {
            m_lep1 = sl->leptons->at(0);
            m_lep2 = sl->leptons->at(1);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_probeLep1 = m_lep2 ; // TODO: Find better way of tagging fake lepton
            m_probeLep2 = m_lep1;
            m_triggerLeptons.push_back(m_lep1);
            m_triggerLeptons.push_back(m_lep2);
        } else if ((m_fake_baseline_DF || m_fake_baseline_SF) && sl->leptons->size() == 1 && m_invLeps.size() >= 1) {
            m_lep1 = sl->leptons->at(0);
            m_lep2 = m_invLeps.at(0);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_probeLep1 = m_lep2;
            if (m_invLeps.size() >= 2) { m_probeLep2 = m_invLeps.at(1); }
            m_triggerLeptons.push_back(m_lep1);
            // Note: Cannot use dilepton triggers without introducing trigger bias
        } else if (m_zjets_3l && sl->leptons->size() == 3) {
            m_lep1 = sl->leptons->at(ztagged_idx1);
            m_lep2 = sl->leptons->at(ztagged_idx2);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            int probeLep_idx1 = -1; // Assume fake is highest pT non-Z-tagged lepton
            if      (ztagged_idx1 != 0 && ztagged_idx2 != 0) { probeLep_idx1 = 0; } 
            else if (ztagged_idx1 != 1 && ztagged_idx2 != 1) { probeLep_idx1 = 1; } 
            else if (ztagged_idx1 != 2 && ztagged_idx2 != 2) { probeLep_idx1 = 2; }
            m_probeLep1 = sl->leptons->at(probeLep_idx1); //TODO: Test how often this assumption is valid
            m_probeLep2 = nullptr;
            m_triggerLeptons.push_back(m_lep1);
            m_triggerLeptons.push_back(m_lep2);
        } else if (m_fake_zjets_3l && sl->leptons->size() == 2 && m_invLeps.size() >= 1) {
            m_lep1 = sl->leptons->at(0);
            m_lep2 = sl->leptons->at(1);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_probeLep1 = m_invLeps.at(0);
            if (m_invLeps.size() >= 2) { m_probeLep2 = m_invLeps.at(1); }
            m_triggerLeptons.push_back(m_lep1);
            m_triggerLeptons.push_back(m_lep2);
        } else {
            if (sl->leptons->size() >= 1) {
                m_lep1 = sl->leptons->at(0);
                if (sl->leptons->size() >= 2) {
                    m_lep2 = sl->leptons->at(1);
                    m_dileptonP4 = *m_lep1 + *m_lep2;
                }
            }
        }
        // Set globals for dilepton trigger matching
        for (Susy::Lepton* lep : m_triggerLeptons) {
            if (lep->isEle()) {
                if (!m_el0) m_el0 = dynamic_cast<Susy::Electron*>(lep);
                else if (!m_el1) m_el1 = dynamic_cast<Susy::Electron*>(lep);
            }
            else if (lep->isMu()) {
                if (!m_mu0) m_mu0 = dynamic_cast<Susy::Muon*>(lep);
                else if (!m_mu1) m_mu1 = dynamic_cast<Susy::Muon*>(lep);
            }
        }

        // Jigsaw variables
        if (sl->leptons->size() >= 2) {
            // build the object map for the calculator
            // the TTMET2LW calculator expects "leptons" and "met"
            std::map<std::string, std::vector<TLorentzVector>> object_map;
            object_map["leptons"] = { *m_lep1, *m_lep2 };
            object_map["met"] = { m_MET };
            m_calculator.load_event(object_map);
            m_jigsaw_vars = m_calculator.variables();
        }

        ////////////////////////////////////////////////////////////////////////
        return true; // All events pass this cut
    };
}
void add_cleaning_cuts(Superflow* cutflow) {
    *cutflow << CutName("Pass GRL") << [](Superlink* sl) -> bool {
        return (sl->tools->passGRL(m_cutflags));
    };
    *cutflow << CutName("Error flags") << [](Superlink* sl) -> bool {
        return (sl->tools->passLarErr(m_cutflags)
                && sl->tools->passTileErr(m_cutflags)
                && sl->tools->passSCTErr(m_cutflags)
                && sl->tools->passTTC(m_cutflags));
    };
    *cutflow << CutName("pass Good Vertex") << [](Superlink * sl) -> bool {
        return (sl->tools->passGoodVtx(m_cutflags));
    };
    *cutflow << CutName("pass bad muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passBadMuon(sl->preMuons));
    };
    *cutflow << CutName("pass cosmic muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passCosmicMuon(sl->baseMuons));
    };
    *cutflow << CutName("pass jet cleaning") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };
}
void add_analysis_cuts(Superflow* cutflow) {
    if (m_baseline_DF || m_baseline_SF) {
        *cutflow << CutName("exactly two base leptons") << [](Superlink* sl) -> bool {
            return sl->baseLeptons->size() == 2;
        };

        *cutflow << CutName("exactly two signal leptons") << [](Superlink* sl) -> bool {
            return (sl->leptons->size() == 2);
        };

        *cutflow << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
            return (m_lep1->q * m_lep2->q < 0);
        };
    }
    if (m_baseline_DF) {
        *cutflow << CutName("dilepton flavor (emu/mue)") << [](Superlink* /*sl*/) -> bool {
            return (m_lep1->isEle() != m_lep2->isEle());
        };
    }
    if (m_baseline_SF) {
        *cutflow << CutName("dilepton flavor (ee/mumu)") << [](Superlink* /*sl*/) -> bool {
            return m_lep1->isEle() == m_lep2->isEle();
        };

        *cutflow << CutName("|m_ll - Zmass| > 10 GeV") << [](Superlink* /*sl*/) -> bool {
            return fabs(m_dileptonP4.M() - ZMASS) > 10;
        };
    }
    if (m_zjets_3l) {
        *cutflow << CutName("3 signal leptons") << [](Superlink* sl) -> bool {
            return (sl->leptons->size() == 3);
        };
    } else if (m_fake_zjets_3l) {
        *cutflow << CutName("2 signal and 1+ inverted lepton") << [](Superlink* sl) -> bool {
            return (sl->leptons->size() == 2 && m_invLeps.size() >= 1);
        };
    }
    if (m_zjets_3l || m_fake_zjets_3l) {
        *cutflow << CutName("|mZ_ll - Zmass| < 10 GeV") << [](Superlink* /*sl*/) -> bool {
            return fabs(m_dileptonP4.M() - ZMASS) < 10;
        };
        *cutflow << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
            return (m_lep1->q * m_lep2->q < 0);
        };
        *cutflow << CutName("Z dilepton flavor (ee/mumu)") << [](Superlink* /*sl*/) -> bool {
            return m_lep1->isEle() == m_lep2->isEle();
        };
    }

    if (m_baseline_DF || m_baseline_SF) {
        *cutflow << CutName("m_ll > 20 GeV") << [](Superlink* /*sl*/) -> bool {
            return m_dileptonP4.M() > 20.0;
        };
    }

    *cutflow << CutName("tau veto") << [](Superlink* sl) -> bool {
        return sl->taus->size() == 0;
    };

}
void add_event_variables(Superflow* cutflow) {
    // Event weights
    *cutflow << NewVar("event weight"); {
        *cutflow << HFTname("eventweight");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->weights->product() * sl->nt->evt()->wPileup; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("event weight (multi period)"); {
        *cutflow << HFTname("eventweight_multi");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->weights->product_multi() * sl->nt->evt()->wPileup; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("event weight (single period)"); {
        *cutflow << HFTname("eventweight_single");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->product() * sl->nt->evt()->wPileup / sl->nt->evt()->wPileup_period; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("event weight (no scale factors)"); {
        *cutflow << HFTname("eventweight_noSF");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("multiperiod event weight (no scale factors)"); {
        *cutflow << HFTname("eventweight_noSF_multi");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt_multi;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Period weight"); {
        *cutflow << HFTname("period_weight");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup_period;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Monte-Carlo generator event weight"); {
        *cutflow << HFTname("w");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->w;};
        *cutflow << SaveVar();
    }

    // Scale Factors
    *cutflow << NewVar("Pile-up weight"); {
        *cutflow << HFTname("pupw");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Lepton scale factor"); {
        *cutflow << HFTname("lepSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->lepSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Trigger scale factor"); {
        *cutflow << HFTname("trigSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->trigSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("B-tag scale factor"); {
        *cutflow << HFTname("btagSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->btagSf;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("JVT scale factor"); {
        *cutflow << HFTname("jvtSf");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->weights->jvtSf;};
        *cutflow << SaveVar();
    }

    // Event identifiers
    *cutflow << NewVar("Event run number"); {
        *cutflow << HFTname("runNumber");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->run; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Event number"); {
        *cutflow << HFTname("eventNumber");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->eventNumber; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("lumi block"); {
        *cutflow << HFTname("lb");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->lb; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is Monte Carlo"); {
        *cutflow << HFTname("isMC");
        *cutflow << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("mcChannel (dsid)"); {
        *cutflow << HFTname("dsid");
        *cutflow << [](Superlink* sl, var_double*) -> double {return sl->isMC ? sl->nt->evt()->mcChannel : 0.0;};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("treatAsYear"); {
        // 15/16 Year ID
        *cutflow << HFTname("treatAsYear");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->nt->evt()->treatAsYear; };
        *cutflow << SaveVar();
    }

    // Event properties
    *cutflow << NewVar("Average Mu"); {
        *cutflow << HFTname("avgMu");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMu; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Average Mu (Data DF)"); {
        *cutflow << HFTname("avgMuDataSF");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->avgMuDataSF; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Actual Mu"); {
        *cutflow << HFTname("actualMu");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMu; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Actual Mu (Data SF)"); {
        *cutflow << HFTname("actualMuDataSF");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->actualMuDataSF; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Number of vertices"); {
        *cutflow << HFTname("nVtx");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->nVtx; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Number of tracks associated with primary vertex"); {
        *cutflow << HFTname("nTracksAtPV");
        *cutflow << [](Superlink* sl, var_double*) -> double { return sl->nt->evt()->nTracksAtPV; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pileup density"); {
        *cutflow << HFTname("pileup_density");
        *cutflow << [](Superlink* sl, var_double*) -> double {
            float actual_mu = sl->nt->evt()->actualMu;
            float sigmaZ = sl->nt->evt()->beamPosSigmaZ;
            float beamPosZ = sl->nt->evt()->beamPosZ;
            float pvZ = sl->nt->evt()->pvZ;

            pu_profile.SetParameter(0, actual_mu); // normalization
            pu_profile.SetParameter(1, beamPosZ); // mean
            pu_profile.SetParameter(2, sigmaZ); // sigma

            return pu_profile.Eval(pvZ);
        };
        *cutflow << SaveVar();
    }
}

void add_trigger_variables(Superflow* cutflow) {
    ////////////////////////////////////////////////////////////////////////////
    // Trigger Variables
    // ADD_*_TRIGGER_VAR preprocessor defined

    ////////////////////////////////////////////////////////////////////////////
    // 2015
    ADD_1LEP_TRIGGER_VAR(HLT_e24_lhmedium_L1EM20VH, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e120_lhloose, m_triggerLeptons)

    ADD_2LEP_TRIGGER_VAR(HLT_2e12_lhloose_L12EM10VH, m_el0, m_el1)

    ADD_1LEP_TRIGGER_VAR(HLT_mu20_iloose_L1MU15, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu40, m_triggerLeptons)

    // TODO: HLT_2mu10
    ADD_2LEP_TRIGGER_VAR(HLT_mu18_mu8noL1, m_mu0, m_mu1)

    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_mu14, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_mu24, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2016
    ADD_2LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0, m_el0, m_el1)
    ADD_2LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2016-2018

    ADD_1LEP_TRIGGER_VAR(HLT_e26_lhtight_nod0_ivarloose, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e60_lhmedium_nod0, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_e140_lhloose_nod0, m_triggerLeptons)

    ADD_1LEP_TRIGGER_VAR(HLT_mu26_ivarmedium, m_triggerLeptons)
    ADD_1LEP_TRIGGER_VAR(HLT_mu50, m_triggerLeptons)

    // TODO: HLT_2mu14
    ADD_2LEP_TRIGGER_VAR(HLT_mu22_mu8noL1, m_mu0, m_mu1)

    ADD_2LEP_TRIGGER_VAR(HLT_e17_lhloose_nod0_mu14, m_el0, m_mu0)
    ADD_2LEP_TRIGGER_VAR(HLT_e7_lhmedium_nod0_mu24, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2017-2018
    ADD_2LEP_TRIGGER_VAR(HLT_2e24_lhvloose_nod0, m_el0, m_el1)

    ADD_2LEP_TRIGGER_VAR(HLT_e26_lhmedium_nod0_mu8noL1, m_el0, m_mu0)

    ////////////////////////////////////////////////////////////////////////////
    // 2018
    // L1_2EM15VHI was accidentally prescaled in periods B5-B8 of 2017
    // (runs 326834-328393) with an effective reduction of 0.6 fb-1
    ADD_2LEP_TRIGGER_VAR(HLT_2e17_lhvloose_nod0_L12EM15VHI, m_el0, m_el1)
}
void add_lepton_variables(Superflow* cutflow) {

    ADD_LEPTON_VARS(lep1)
    ADD_LEPTON_VARS(lep2)

    //*cutflow << NewVar("lep1 truth type"); {
    //    *cutflow << HFTname("lep1TruthType");
    //    *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcType; };
    //    *cutflow << SaveVar();
    //}
    //*cutflow << NewVar("lep1 truth origin"); {
    //    *cutflow << HFTname("lep1TruthOrigin");
    //    *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcOrigin; };
    //    *cutflow << SaveVar();
    //}
    //*cutflow << NewVar("lep1 truth mother PDG ID"); {
    //    *cutflow << HFTname("lep1TruthMotherPdgID");
    //    *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcBkgMotherPdgId; };
    //    *cutflow << SaveVar();
    //}
    //*cutflow << NewVar("lep1 truth mother origin"); {
    //    *cutflow << HFTname("lep1TruthMotherOrigin");
    //    *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcBkgTruthOrigin; };
    //    *cutflow << SaveVar();
    //}

    *cutflow << NewVar("number of signal leptons"); {
        *cutflow << HFTname("nSigLeps");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->leptons->size();};
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("number of inverted leptons"); {
        *cutflow << HFTname("nInvLeps");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int { return m_invLeps.size();};
        *cutflow << SaveVar();
    }

    //////////////////////////////////////////////////////////////////////////////
    // Electrons
    *cutflow << NewVar("Electron ID (non-inclusive)"); {
      *cutflow << HFTname("El_ID");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& el : *sl->electrons) {
          if (el->tightLLH) out.push_back(0);
          else if (el->mediumLLH) out.push_back(1);
          else if (el->looseLLHBLayer) out.push_back(2);
          else if (el->looseLLH) out.push_back(3);
          else if (el->veryLooseLLH) out.push_back(4);
          else out.push_back(5);
        }
        return out;
      };
      *cutflow << SaveVar();
    }
    //////////////////////////////////////////////////////////////////////////////
    // Muons
    *cutflow << NewVar("Muon ID (non-inclusive)"); {
      *cutflow << HFTname("Mu_ID");
      *cutflow << [](Superlink* sl, var_int_array*) -> vector<int> {
        vector<int> out;
        for (auto& mu : *sl->muons) {
          if (mu->tight) out.push_back(0);
          else if (mu->medium) out.push_back(1);
          else if (mu->loose) out.push_back(2);
          else if (mu->veryLoose) out.push_back(3);
          else if (!mu->veryLoose) out.push_back(4);
        }
        return out;
      };
      *cutflow << SaveVar();
    }
}
void add_jet_variables(Superflow* cutflow) {
    *cutflow << NewVar("number of light jets"); {
        *cutflow << HFTname("nLightJets");
        *cutflow << [](Superlink* /*sl*/, var_int*) -> int {return m_light_jets.size(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of b-tagged jets"); {
        *cutflow << HFTname("nBJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfBJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of forward jets"); {
        *cutflow << HFTname("nForwardJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->tools->numberOfFJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("number of non-b-tagged jets"); {
        *cutflow << HFTname("nNonBJets");
        *cutflow << [](Superlink* sl, var_int*) -> int { return sl->jets->size() - sl->tools->numberOfBJets(*sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Pt"); {
        *cutflow << HFTname("jet1Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Pt() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Eta"); {
        *cutflow << HFTname("jet1Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-1 Phi"); {
        *cutflow << HFTname("jet1Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Pt"); {
        *cutflow << HFTname("jet2Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Pt() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Eta"); {
        *cutflow << HFTname("jet2Eta");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Eta() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("jet-2 Phi"); {
        *cutflow << HFTname("jet2Phi");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->Phi() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }
}
void add_met_variables(Superflow* cutflow) {
    *cutflow << NewVar("transverse missing energy (Et)"); {
        *cutflow << HFTname("met");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->met->Et; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("transverse missing energy (Phi)"); {
        *cutflow << HFTname("metPhi");
        *cutflow << [](Superlink* sl, var_float*) -> double { return sl->met->phi; };
        *cutflow << SaveVar();
    }
}
void add_dilepton_variables(Superflow* cutflow) {
    *cutflow << NewVar("is e + e"); {
        *cutflow << HFTname("isElEl");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() && m_lep2->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is mu + mu"); {
        *cutflow << HFTname("isMuMu");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isMu() && m_lep2->isMu(); };
        *cutflow << SaveVar();
    }
    
    *cutflow << NewVar("is mu (lead) + e (sub)"); {
        *cutflow << HFTname("isMuEl");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isMu() && m_lep2->isEle(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is e (lead) + mu (sub)"); {
        *cutflow << HFTname("isElMu");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() && m_lep2->isMu(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is opposite-sign"); {
        *cutflow << HFTname("isOS");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->q * m_lep2->q < 0; };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("is e + mu"); {
        *cutflow << HFTname("isDF");
        *cutflow << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() ^ m_lep2->isEle(); };
        *cutflow << SaveVar();
    }


    *cutflow << NewVar("mass of di-lepton system"); {
        *cutflow << HFTname("mll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double {return m_dileptonP4.M();};
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pt of di-lepton system"); {
        *cutflow << HFTname("pTll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_dileptonP4.Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Pt difference of di-lepton system"); {
        *cutflow << HFTname("dpTll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lep1->Pt() - m_lep2->Pt(); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Eta of di-lepton system"); {
        *cutflow << HFTname("deta_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lep1->Eta() - m_lep2->Eta()); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of di-lepton system"); {
        *cutflow << HFTname("dphi_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lep1->DeltaPhi(*m_lep2)); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta R of di-lepton system"); {
        *cutflow << HFTname("dR_ll");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double { return m_lep1->DeltaR(*m_lep2); };
        *cutflow << SaveVar();
    }
}
void add_multi_object_variables(Superflow* cutflow) {
    // Jets and MET
    *cutflow << NewVar("delta Phi of leading jet and met"); {
        *cutflow << HFTname("deltaPhi_met_jet1");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("delta Phi of subleading jet and met"); {
        *cutflow << HFTname("deltaPhi_met_jet2");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("jet-1 Pt"); {
        *cutflow << HFTname("jet1Pt");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Pt() : -DBL_MAX;
        };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("stransverse mass"); {
        *cutflow << HFTname("MT2");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double mt2_ = kin::getMT2(*sl->leptons, *sl->met);
            return mt2_;
        };
        *cutflow << SaveVar();
    }

    // Leptons, Jets, and MET
    *cutflow << NewVar("Etmiss Rel"); {
        *cutflow << HFTname("metrel");
        *cutflow << [](Superlink* sl, var_float*) -> double { return kin::getMetRel(sl->met, *sl->leptons, *sl->jets); };
        *cutflow << SaveVar();
    }

    *cutflow << NewVar("Ht (m_Eff: lep + met + jet)"); {
        *cutflow << HFTname("ht");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;

            ht += m_lep1->Pt() + m_lep2->Pt();
            ht += sl->met->Et;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                ht += sl->jets->at(i)->Pt();
            }
            return ht;
        };
        *cutflow << SaveVar();
    }
    *cutflow << NewVar("reco-level max(HT,pTV) "); {
        *cutflow << HFTname("max_HT_pTV_reco");
        *cutflow << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                if (sl->jets->at(i)->Pt() < 20) {continue;}
                ht += sl->jets->at(i)->Pt();
            }
            return max(ht, m_dileptonP4.Pt());
        };
        *cutflow << SaveVar();
    }
}

void add_jigsaw_variables(Superflow* cutflow) {
    ADD_JIGSAW_VAR(H_11_SS)
    ADD_JIGSAW_VAR(H_21_SS)
    ADD_JIGSAW_VAR(H_12_SS)
    ADD_JIGSAW_VAR(H_22_SS)
    ADD_JIGSAW_VAR(H_11_S1)
    ADD_JIGSAW_VAR(H_11_SS_T)
    ADD_JIGSAW_VAR(H_21_SS_T)
    ADD_JIGSAW_VAR(H_22_SS_T)
    ADD_JIGSAW_VAR(H_11_S1_T)
    ADD_JIGSAW_VAR(shat)
    ADD_JIGSAW_VAR(pTT_T)
    ADD_JIGSAW_VAR(pTT_Z)
    ADD_JIGSAW_VAR(RPT)
    ADD_JIGSAW_VAR(RPZ)
    ADD_JIGSAW_VAR(RPT_H_11_SS)
    ADD_JIGSAW_VAR(RPT_H_21_SS)
    ADD_JIGSAW_VAR(RPT_H_22_SS)
    ADD_JIGSAW_VAR(RPZ_H_11_SS)
    ADD_JIGSAW_VAR(RPZ_H_21_SS)
    ADD_JIGSAW_VAR(RPZ_H_22_SS)
    ADD_JIGSAW_VAR(RPT_H_11_SS_T)
    ADD_JIGSAW_VAR(RPT_H_21_SS_T)
    ADD_JIGSAW_VAR(RPT_H_22_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_11_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_21_SS_T)
    ADD_JIGSAW_VAR(RPZ_H_22_SS_T)
    ADD_JIGSAW_VAR(gamInvRp1)
    ADD_JIGSAW_VAR(MDR)
    ADD_JIGSAW_VAR(DPB_vSS)
    ADD_JIGSAW_VAR(costheta_SS)
    ADD_JIGSAW_VAR(dphi_v_SS)
    ADD_JIGSAW_VAR(dphi_v1_i1_ss)
    ADD_JIGSAW_VAR(dphi_s1_s2_ss)
    ADD_JIGSAW_VAR(dphi_S_I_ss)
    ADD_JIGSAW_VAR(dphi_S_I_s1)
}
void add_miscellaneous_variables(Superflow* cutflow) {

    *cutflow << NewVar("|cos(theta_b)|"); {
        *cutflow << HFTname("abs_costheta_b");
        *cutflow << [](Superlink* /*sl*/, var_float*) -> double {
            TLorentzVector lp, lm, ll;
            lp = m_lep1->q > 0 ? *m_lep1 : *m_lep2;
            lm = m_lep1->q < 0 ? *m_lep1 : *m_lep2;
            ll = lp + lm;

            TVector3 boost = ll.BoostVector();
            lp.Boost(-boost);
            lm.Boost(-boost);

            double deta = lp.Eta() - lm.Eta();
            double costheta_b = tanh(0.5 * deta);

            return fabs(costheta_b);
        };
        *cutflow << SaveVar();
    }
}

void add_fakefactor_variables(Superflow* cutflow) {
    ADD_LEPTON_VARS(probeLep1)
    ADD_LEPTON_VARS(probeLep2)
}

void add_zjets_fakefactor_variables(Superflow* cutflow) {
    *cutflow << NewVar("Mll of 2nd-closest Z pair"); {
      *cutflow << HFTname("Z2_mll");
      *cutflow << [](Superlink* sl, var_double*) -> double {
          float Z_diff = FLT_MAX;
          float Z2_mass = FLT_MAX;
          for (uint ii = 0; ii < sl->baseLeptons->size(); ++ii) {
              Susy::Lepton *lep_ii = sl->baseLeptons->at(ii);
              if (lep_ii == m_lep1 || lep_ii == m_lep2) continue;
              for (uint jj = ii+1; jj < sl->baseLeptons->size(); ++jj) {
                  Susy::Lepton *lep_jj = sl->baseLeptons->at(jj);
                  if (lep_jj == m_lep1 || lep_jj == m_lep2) continue;
                  float Z_diff_cf = fabs((*lep_ii+*lep_jj).M() - ZMASS);
                  bool SFOS_leps = (lep_ii->isEle() == lep_jj->isEle()) && (lep_ii->q * lep_jj->q < 0);
                  if (Z_diff_cf < Z_diff && SFOS_leps) {
                      Z_diff = Z_diff_cf;
                      Z2_mass = (*lep_ii+*lep_jj).M();
                  }
              }
          }
          return Z2_mass;
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("Mlll: Invariant mass of 3lep system"); {
      *cutflow << HFTname("mlll");
      *cutflow << [](Superlink* /*sl*/, var_double*) -> double {
          return (*m_lep1 + *m_lep2 + *m_probeLep1).M();
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("DeltaR of probeLep1 and Zlep1"); {
      *cutflow << HFTname("dR_ZLep1_probeLep1");
      *cutflow << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep1).DeltaR(*m_probeLep1);
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("DeltaR of probeLep1 and Zlep2"); {
      *cutflow << HFTname("dR_ZLep2_probeLep1");
      *cutflow << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep2).DeltaR(*m_probeLep1);
      };
      *cutflow << SaveVar();
    }
    *cutflow << NewVar("DeltaR of probeLep1 and Z"); {
      *cutflow << HFTname("dR_Z_probeLep1");
      *cutflow << [](Superlink* /*sl*/, var_double*) -> double {
        return m_dileptonP4.DeltaR(*m_probeLep1);
      };
      *cutflow << SaveVar();
    }
}

void add_weight_systematics(Superflow* cutflow) {
    *cutflow << NewSystematic("shift in electron ID efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_ID");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron ISO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_Iso");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in electron RECO efficiency"); {
        *cutflow << WeightSystematic(SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_DN);
        *cutflow << TreeName("EL_EFF_Reco");
        *cutflow << SaveSystematic();
    }
}
void add_shape_systematics(Superflow* cutflow) {
    *cutflow << NewSystematic("shift in e-gamma resolution (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_UP);
        *cutflow << TreeName("EG_RESOLUTION_ALL_UP");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma resolution (DOWN)"); {
        *cutflow << EventSystematic(NtSys::EG_RESOLUTION_ALL_DN);
        *cutflow << TreeName("EG_RESOLUTION_ALL_DN");
        *cutflow << SaveSystematic();
    }
    *cutflow << NewSystematic("shift in e-gamma scale (UP)"); {
        *cutflow << EventSystematic(NtSys::EG_SCALE_ALL_UP);
        *cutflow << TreeName("EG_SCALE_ALL_UP");
        *cutflow << SaveSystematic();
    }
}

bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons) {
    for (Susy::Lepton* lep : leptons) {
        if(!lep) continue;
        bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
        if (!trig_fired) continue;
        bool trig_matched = sl->tools->triggerTool().lepton_trigger_match(lep, trig_name);
        if (trig_matched) return true;
    }
    return false;
}

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2) {
    if(!lep1 || ! lep2) return false;
    bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
    if (!trig_fired) return false;
    bool trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep1, lep2, trig_name);
    return trig_matched;
}

bool is_antiID_lepton(Susy::Lepton* lepton) {
    bool pt_pass = 0, eta_pass = 0, iso_pass = 0, id_pass = 0;
    bool passID_cuts = 0, passAntiID_cuts = 0;
    if (lepton->isEle()) {
        const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(lepton);
        //float absEtaBE = fabs(ele->clusEtaBE);
        pt_pass  = ele->pt > 0; //15;
        //eta_pass = (absEtaBE < 1.37 || 1.52 < absEtaBE) && fabs(ele->eta) < 2.47;
        eta_pass = fabs(ele->eta) < 2.47;
        iso_pass = ele->isoGradient;
        id_pass  = ele->mediumLLH;
        passID_cuts = iso_pass && id_pass;
        passAntiID_cuts = ele->looseLLHBLayer;
    } else if (lepton->isMu()) {
        const Susy::Muon* mu = dynamic_cast<const Susy::Muon*>(lepton);
        pt_pass  = mu->pt > 0; //10;
        eta_pass = fabs(mu->eta) < 2.47;
        iso_pass = mu->isoFCLoose;
        id_pass  = mu->medium;
        passID_cuts = iso_pass && id_pass;
        passAntiID_cuts = mu->medium;
    }
    return pt_pass && eta_pass && passAntiID_cuts && !passID_cuts;
}
bool is_ID_lepton(Superlink* sl, Susy::Lepton* lepton) {
    for (Susy::Lepton* sig_lepton : *sl->leptons) {
        if (lepton == sig_lepton) {
            return true;
        }
    }
    return false;
}

int get_lepton_truth_class(Susy::Lepton* lepton) {
    if (lepton==nullptr) return -INT_MAX;

    // Get Truth information
    int T = lepton->mcType;
    int O = lepton->mcOrigin;
    int MO = lepton->mcBkgTruthOrigin; // TODO: Update. BkgTruthOrigin != motherOrigin
    int MT = 0; // Not stored in SusyNt::Lepton
    int M_ID = lepton->mcBkgMotherPdgId;

    using namespace MCTruthPartClassifier;

    bool mother_is_el = fabs(M_ID) == 11;
    //bool mother_is_piZero = fabs(M_ID) == 111;
    bool bkgEl_from_phoConv = T==BkgElectron && O==PhotonConv;
    bool bkgEl_from_EMproc = T==BkgElectron && O==ElMagProc;
    bool fromSMBoson = O==WBoson || O==ZBoson || O==Higgs || O==DiBoson;
    bool MfromSMBoson = MO==WBoson || MO==ZBoson || MO==Higgs || MO==DiBoson;
    //bool noChargeFlip = M_ID*lepton->q < 0;
    //bool chargeFlip = M_ID*lepton->q > 0;

    // Defs from https://indico.cern.ch/event/725960/contributions/2987219/attachments/1641430/2621432/TruthDef_April242018.pdf
    bool promptEl1 = T==IsoElectron; //&& noChargeFlip;
    bool promptEl2 = (bkgEl_from_phoConv && mother_is_el); //&& noChargeFlip;
    bool promptEl3 = bkgEl_from_EMproc && MT==IsoElectron && (MO==top || MfromSMBoson);
    bool promptEl = promptEl1 || promptEl2 || promptEl3;

    bool promptEl_from_FSR1 = bkgEl_from_phoConv && MO==FSRPhot;
    bool promptEl_from_FSR2 = T==NonIsoPhoton && O==FSRPhot;
    bool promptEl_from_FSR = promptEl_from_FSR1 || promptEl_from_FSR2;

    //bool promptChargeFlipEl1 = T==IsoElectron && chargeFlip;
    //bool promptChargeFlipEl2 = (bkgEl_from_phoConv && mother_is_el) && chargeFlip;
    //bool promptChargeFlipEl = promptChargeFlipEl1 || promptChargeFlipEl2;

    bool promptMuon = T==IsoMuon && (O==top || fromSMBoson || O==HiggsMSSM || O==MCTruthPartClassifier::SUSY);

    bool promptPho1 = T==IsoPhoton && O==PromptPhot;
    bool promptPho2 = bkgEl_from_phoConv && MT==IsoPhoton && MO==PromptPhot;
    bool promptPho3 = bkgEl_from_EMproc  && MT==IsoPhoton && MO==PromptPhot;
    bool promptPho4 = bkgEl_from_phoConv && MT==BkgPhoton && MO==UndrPhot;
    bool promptPho5 = T==BkgPhoton && O==UndrPhot;
    bool promptPho = promptPho1 || promptPho2 || promptPho3 || promptPho4 || promptPho5;

    bool hadDecay1 = T==BkgElectron && (O==DalitzDec || O==ElMagProc || O==LightMeson || O==StrangeMeson);
    bool hadDecay2 = bkgEl_from_phoConv && MT==BkgPhoton && (MO==PiZero || MO==LightMeson || MO==StrangeMeson);
    bool hadDecay3 = bkgEl_from_EMproc && ((MT==BkgElectron && MO==StrangeMeson) || (MT==BkgPhoton && MO==PiZero));
    bool hadDecay4 = T==BkgPhoton && (O==LightMeson || O==PiZero);
    bool hadDecay5 = T==BkgMuon && (O==LightMeson || O==StrangeMeson || O==PionDecay || O==KaonDecay);
    bool hadDecay6 = T==Hadron;
    bool hadDecay = hadDecay1 || hadDecay2 || hadDecay3 || hadDecay4 || hadDecay5 || hadDecay6;

    bool Mu_as_e1 = (T==NonIsoElectron || T==NonIsoPhoton) && O==Mu;
    bool Mu_as_e2 = bkgEl_from_EMproc && MT==NonIsoElectron && MO==Mu;
    bool Mu_as_e3 = bkgEl_from_phoConv && MT==NonIsoPhoton && MO==Mu;
    bool Mu_as_e = Mu_as_e1 || Mu_as_e2 || Mu_as_e3;

    bool HF_tau1 =  (T==NonIsoElectron || T==NonIsoPhoton) && O==TauLep;
    bool HF_tau2 =  bkgEl_from_phoConv && MT==NonIsoPhoton && MO==TauLep;
    bool HF_tau3 =  T==NonIsoMuon && O==TauLep;
    bool HF_tau =  HF_tau1 || HF_tau2 || HF_tau3;

    bool HF_B1 = T==NonIsoElectron && (O==BottomMeson || O==BBbarMeson || O==BottomBaryon);
    bool HF_B2 = T==BkgPhoton && O==BottomMeson;
    bool HF_B3 = bkgEl_from_phoConv && MT==BkgPhoton && MO==BottomMeson;
    bool HF_B4 = (T==IsoMuon || T==NonIsoMuon) && (O==BottomMeson || O==BBbarMeson || O==BottomBaryon);
    bool HF_B = HF_B1 || HF_B2 || HF_B3 || HF_B4;

    bool HF_C1 = T==NonIsoElectron && (O==CharmedMeson || O==CharmedBaryon || O==CCbarMeson);
    bool HF_C2 = T==BkgElectron && O==CCbarMeson;
    bool HF_C3 = T==BkgPhoton && (O==CharmedMeson || O==CCbarMeson);
    bool HF_C4 = bkgEl_from_phoConv && MT==BkgPhoton && (MO==CharmedMeson || MO==CCbarMeson);
    bool HF_C5 = T==NonIsoMuon && (O==CharmedMeson || O==CharmedBaryon || O==CCbarMeson);
    bool HF_C6 = (T==IsoMuon || T==BkgMuon) && (O==CCbarMeson || MO==CCbarMeson);
    bool HF_C =  HF_C1 || HF_C2 || HF_C3 || HF_C4 || HF_C5 || HF_C6;

    if      (promptEl)           return 1;
    else if (promptMuon)         return 2;
    else if (promptPho)          return 3;
    else if (promptEl_from_FSR)  return 4;
    else if (hadDecay)           return 5;
    else if (Mu_as_e)            return 6;
    else if (HF_tau)             return 7;
    else if (HF_B)               return 8;
    else if (HF_C)               return 9;
    else if (bkgEl_from_phoConv) return 10;
    else if (!(T || O || MT || MO || M_ID)) return -1; // No Truth Info
    else if (T && O && !(MT || MO || M_ID)) return -2; // No Mother Info
    else if (T && !O) return -3; // No Origin Info
    // else if (promptChargeFlipEl) return 2;
    else if (T && O && M_ID) {
        cout << "Unexpected Truth Class: "
             << "T = " << T << ", "
             << "O = " << O << ", "
             << "MT = " << MT << ", "
             << "MO = " << MO << ", "
             << "M_ID = " << M_ID << endl;
    }
    return 0;
}


