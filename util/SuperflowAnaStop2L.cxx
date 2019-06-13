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

// ROOT
#include "TChain.h"
#include "TVectorD.h"
#include "TF1.h"

// xAOD
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "xAODBase/IParticle.h"
#include "xAODEgamma/Electron.h"
#include "xAODMuon/Muon.h"

// ASG
#include "AsgTools/StatusCode.h"
#include "PathResolver/PathResolver.h"

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

//FakeBkdTools
#include "FakeBkgTools/ApplyFakeFactor.h"
#include "FakeBkgTools/FakeBkgHelpers.h"

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
Susy::AnalysisType m_ana_type = Susy::AnalysisType::Ana_Stop2L4B;
const float ZMASS = 91.2;
const float GeVtoMeV = 1000.0;
const string m_ff_file = "LexStop2LAnalysis/fakeFactorHists.root";
//const string m_ff_file = "LexStop2LAnalysis/fakeFactorDummy.root";

////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////
TChain* create_new_chain(string input, string ttree_name, bool verbose);
Superflow* create_new_superflow(SFOptions sf_options, TChain* chain);
void set_global_variables(Superflow* sf);
void add_cleaning_cuts(Superflow* sf);
void add_analysis_cuts(Superflow* sf);
void add_4bcutflow_cuts(Superflow* sf);
void add_event_variables(Superflow* sf);
void add_trigger_variables(Superflow* sf);
void add_lepton_variables(Superflow* sf);
void add_jet_variables(Superflow* sf);
void add_met_variables(Superflow* sf);
void add_dilepton_variables(Superflow* sf);
void add_multi_object_variables(Superflow* sf);
void add_jigsaw_variables(Superflow* sf);
void add_miscellaneous_variables(Superflow* sf);
void add_fakefactor_variables(Superflow* sf);
void add_zjets_fakefactor_variables(Superflow* sf);
void add_zjets2l_inc_variables(Superflow* sf);

void add_weight_systematics(Superflow* sf);
void add_shape_systematics(Superflow* sf);


// Selections (set with user input)
bool m_baseline_DF = false;
bool m_baseline_SF = false;
bool m_zjets_3l = false;
bool m_fake_baseline_DF = false;
bool m_fake_baseline_SF= false;
bool m_fake_zjets_3l = false;
bool m_zjets2l_inc = false;

// globals for use in superflow cuts and variables
// TODO: Do I need static variables
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
static map<string, bool> m_triggerPass;

static TF1 pu_profile("pu_profile","gausn", -250, 250);

static jigsaw::JigsawCalculator m_calculator;
static std::map< std::string, float> m_jigsaw_vars;

static ApplyFakeFactor m_fake_tool;
static FakeBkgTools::Weight m_wgt;
static bool m_add_fakes = false;

// Helpful functions
int get_lepton_truth_class(Susy::Lepton* lepton);
bool is_antiID_lepton(Susy::Lepton* lepton);
vector<const xAOD::IParticle*> to_iparticle_vec(Superlink* sl, vector<Susy::Lepton*> leps);
xAOD::IParticle* to_iparticle(Susy::Lepton* lep, bool isTight);
bool is_ID_lepton(Superlink* sl, Susy::Lepton* lepton);

bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons, float pt_min = 0);
#define ADD_1LEP_TRIGGER_VAR(trig_name, leptons) { \
    *sf << NewVar(#trig_name" trigger bit"); { \
        *sf << HFTname(#trig_name); \
        *sf << [=](Superlink* /*sl*/, var_bool*) -> bool { \
            return m_triggerPass.at(#trig_name); \
        }; \
        *sf << SaveVar(); \
    } \
}
//return is_1lep_trig_matched(sl, #trig_name, leptons);

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2, float pt_min1 = 0, float pt_min2 = 0);
#define ADD_2LEP_TRIGGER_VAR(trig_name, lep1, lep2) { \
  *sf << NewVar(#trig_name" trigger bit"); { \
      *sf << HFTname(#trig_name); \
      *sf << [=](Superlink* /*sl*/, var_bool*) -> bool { \
          return m_triggerPass.at(#trig_name); \
      }; \
      *sf << SaveVar(); \
  } \
}
// return is_2lep_trig_matched(sl, #trig_name, lep1, lep2);

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
    *sf << NewVar(#lep_name" Pt"); { \
        *sf << HFTname(#lep_name"Pt"); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Pt() : -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" Eta"); { \
        *sf << HFTname(#lep_name"Eta"); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Eta(): -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" Phi"); { \
        *sf << HFTname(#lep_name"Phi"); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->Phi(): -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" Energy"); { \
        *sf << HFTname(#lep_name"E"); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->E(): -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" charge"); { \
        *sf << HFTname(#lep_name"q"); \
        *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name ? m_##lep_name->q: -INT_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" flavor"); { \
        *sf << HFTname(#lep_name"Flav"); \
        *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name ? m_##lep_name->isEle() ? 0 : 1: -INT_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" d0sigBSCorr"); { \
      *sf << HFTname(#lep_name"_d0sigBSCorr"); \
      *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->d0sigBSCorr: -DBL_MAX; }; \
      *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" z0SinTheta"); { \
      *sf << HFTname(#lep_name"_z0SinTheta"); \
      *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_##lep_name ? m_##lep_name->z0SinTheta(): -DBL_MAX; }; \
      *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" Truth Class"); { \
        *sf << HFTname(#lep_name"TruthClass"); \
        *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_##lep_name ? get_lepton_truth_class(m_##lep_name): -INT_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar(#lep_name" Iso"); { \
      *sf << HFTname(#lep_name"Iso"); \
      *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { \
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
      *sf << SaveVar(); \
    } \
    *sf << NewVar(#lep_name" transverse mass"); { \
        *sf << HFTname(#lep_name"mT"); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { \
            if (!m_##lep_name) return -DBL_MAX; \
            double dphi = m_##lep_name->DeltaPhi(m_MET); \
            double pT2 = m_##lep_name->Pt()*m_MET.Pt(); \
            double lep_mT = sqrt(2 * pT2 * ( 1 - cos(dphi) )); \
            return lep_mT; \
        }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar("delta Phi of "#lep_name" and met"); { \
        *sf << HFTname("deltaPhi_met_"#lep_name); \
        *sf << [](Superlink* /*sl*/, var_float*) -> double { \
          return m_##lep_name ? abs(m_##lep_name->DeltaPhi(m_MET)) : -DBL_MAX; }; \
        *sf << SaveVar(); \
    } \
    \
    *sf << NewVar("dR between "#lep_name" and closest jet"); { \
      *sf << HFTname("dR_"#lep_name"_jet"); \
      *sf << [](Superlink* sl, var_double*) -> double { \
          if (!m_##lep_name) return -DBL_MAX; \
          double dPhi = sl->jets->size() ? DBL_MAX : -DBL_MAX; \
          for (Susy::Jet* jet : *sl->jets) { \
            float tmp_dphi = fabs(jet->DeltaPhi(*m_##lep_name)); \
            if (tmp_dphi < dPhi) dPhi = tmp_dphi; \
          } \
          return dPhi; \
      }; \
      *sf << SaveVar(); \
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
    add_jet_variables(superflow);
    add_met_variables(superflow);
    add_dilepton_variables(superflow);
    add_multi_object_variables(superflow);
    add_jigsaw_variables(superflow);
    add_miscellaneous_variables(superflow);
    add_fakefactor_variables(superflow);
    if (m_zjets_3l || m_fake_zjets_3l) {
        add_zjets_fakefactor_variables(superflow);
    };
    if (m_zjets2l_inc) {
        add_zjets2l_inc_variables(superflow);
    };

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
void set_global_variables(Superflow* sf) {
    // Jigsaw
    m_calculator.initialize("TTMET2LW");

    // Fake estimates
    // Add if fake weight input file exists and proper event selection chosen
    bool fake_region = m_fake_baseline_DF || m_fake_baseline_SF || m_fake_zjets_3l;
    if (!fake_region) {
        m_add_fakes = false;
    } else {
        string fullpath = PathResolverFindDataFile(m_ff_file); 
        if (fullpath != "") {
            m_fake_tool.setProperty("InputFiles", fullpath);
            m_fake_tool.setProperty("EnergyUnit", "GeV");
            //m_fake_tool.setProperty("OutputLevel", );
            //m_fake_tool.setProperty("SkipUncertainties", true);
            //m_fake_tool.setProperty("ConvertWhenMissing", true);
            if (!m_fake_tool.initialize().isSuccess()) {
                cout << "ERROR :: Unable to initialize fake factor tool\n";
                exit(1);
            } else {
                cout << "INFO :: Fake background tool initialized\n";
            }
            m_add_fakes = true;
        } else {
            cout << "WARNING :: Fake input file (" << m_ff_file << ") not found.\n";
            cout << "INFO :: Not adding fake weight branch\n";
            m_add_fakes = false;
        }
    }

    *sf << CutName("read in") << [](Superlink* sl) -> bool {
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
        LeptonVector fakeToolLeps;
        for (Susy::Lepton* lepton : *sl->baseLeptons) {
            if (is_antiID_lepton(lepton)) { 
                m_invLeps.push_back(lepton); 
                fakeToolLeps.push_back(lepton); 
            } else if (is_ID_lepton(sl, lepton)) {
                fakeToolLeps.push_back(lepton);
            }
        }
        int ztagged_idx1 = -1;
        int ztagged_idx2 = -1;
        if (sl->leptons->size() >= 2) {
            float Z_diff = FLT_MAX;
            for (uint ii = 0; ii < sl->leptons->size(); ++ii) {
                Susy::Lepton *lep_ii = sl->leptons->at(ii);
                for (uint jj = ii+1; jj < sl->leptons->size(); ++jj) {
                    Susy::Lepton *lep_jj = sl->leptons->at(jj);
                    bool SF = lep_ii->isEle() == lep_jj->isEle();
                    bool OS = lep_ii->q * lep_jj->q < 0;
                    if (!SF || !OS) continue;
                    float Z_diff_cf = fabs((*lep_ii+*lep_jj).M() - ZMASS);
                    if (Z_diff_cf < Z_diff) {
                        Z_diff = Z_diff_cf;
                        ztagged_idx1 = ii;
                        ztagged_idx2 = jj;
                    }
                }
            }
        }
        bool ztagged = ztagged_idx1 >= 0 && ztagged_idx2 >=0;
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
        } else if ((m_fake_baseline_DF || m_fake_baseline_SF) && sl->leptons->size() == 1 && m_invLeps.size() == 1) {
            m_lep1 = sl->leptons->at(0);
            m_lep2 = m_invLeps.at(0);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_probeLep1 = m_lep2;
            if (m_invLeps.size() >= 2) { m_probeLep2 = m_invLeps.at(1); }
            m_triggerLeptons.push_back(m_lep1);
            // Note: Cannot use dilepton triggers without introducing trigger bias
        } else if (m_zjets_3l && sl->leptons->size() == 3 && ztagged) {
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
        } else if (m_fake_zjets_3l && sl->leptons->size() == 2 && m_invLeps.size() == 1 && ztagged) {
            m_lep1 = sl->leptons->at(ztagged_idx1);
            m_lep2 = sl->leptons->at(ztagged_idx2);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_probeLep1 = m_invLeps.at(0);
            if (m_invLeps.size() >= 2) { m_probeLep2 = m_invLeps.at(1); }
            m_triggerLeptons.push_back(m_lep1);
            m_triggerLeptons.push_back(m_lep2);
        } else if (m_zjets2l_inc && sl->leptons->size() >= 2 && ztagged) {
            m_lep1 = sl->leptons->at(ztagged_idx1);
            m_lep2 = sl->leptons->at(ztagged_idx2);
            m_dileptonP4 = *m_lep1 + *m_lep2;
            m_triggerLeptons.push_back(m_lep1);
            m_triggerLeptons.push_back(m_lep2);
        } else {
            // Backup assignments; should fail cutflow
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

        // Triggers
        ////////////////////////////////////////////////////////////////////////////
        // 2015
        m_triggerPass.emplace("HLT_e24_lhmedium_L1EM20VH", is_1lep_trig_matched(sl, "HLT_e24_lhmedium_L1EM20VH", m_triggerLeptons, 25));
        m_triggerPass.emplace("HLT_e60_lhmedium", is_1lep_trig_matched(sl, "HLT_e60_lhmedium", m_triggerLeptons, 61));
        m_triggerPass.emplace("HLT_e120_lhloose", is_1lep_trig_matched(sl, "HLT_e120_lhloose", m_triggerLeptons, 121));

        m_triggerPass.emplace("HLT_2e12_lhloose_L12EM10VH", is_2lep_trig_matched(sl, "HLT_2e12_lhloose_L12EM10VH", m_el0, m_el1, 13, 13));

        m_triggerPass.emplace("HLT_mu20_iloose_L1MU15", is_1lep_trig_matched(sl, "HLT_mu20_iloose_L1MU15", m_triggerLeptons, 21));
        m_triggerPass.emplace("HLT_mu40", is_1lep_trig_matched(sl, "HLT_mu40", m_triggerLeptons, 41));

        // TODO: Add to SusyNt, ADD_2LEP_TRIGGER_VAR(HLT_2mu10, m_mu0, m_mu1)
        m_triggerPass.emplace("HLT_mu18_mu8noL1", is_2lep_trig_matched(sl, "HLT_mu18_mu8noL1", m_mu0, m_mu1, 19, 9));

        m_triggerPass.emplace("HLT_e17_lhloose_mu14", is_2lep_trig_matched(sl, "HLT_e17_lhloose_mu14", m_el0, m_mu0, 18, 15));
        m_triggerPass.emplace("HLT_e7_lhmedium_mu24", is_2lep_trig_matched(sl, "HLT_e7_lhmedium_mu24", m_el0, m_mu0, 8, 25));

        ////////////////////////////////////////////////////////////////////////////
        // 2016
        m_triggerPass.emplace("HLT_2e17_lhvloose_nod0", is_2lep_trig_matched(sl, "HLT_2e17_lhvloose_nod0", m_el0, m_el1, 18, 18));
        m_triggerPass.emplace("HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1", is_2lep_trig_matched(sl, "HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1", m_el0, m_mu0, 27, 9));

        ////////////////////////////////////////////////////////////////////////////
        // 2016-2018

        m_triggerPass.emplace("HLT_e26_lhtight_nod0_ivarloose", is_1lep_trig_matched(sl, "HLT_e26_lhtight_nod0_ivarloose", m_triggerLeptons, 27));
        m_triggerPass.emplace("HLT_e60_lhmedium_nod0", is_1lep_trig_matched(sl, "HLT_e60_lhmedium_nod0", m_triggerLeptons, 61));
        m_triggerPass.emplace("HLT_e140_lhloose_nod0", is_1lep_trig_matched(sl, "HLT_e140_lhloose_nod0", m_triggerLeptons, 141));

        m_triggerPass.emplace("HLT_mu26_ivarmedium", is_1lep_trig_matched(sl, "HLT_mu26_ivarmedium", m_triggerLeptons, 27));
        m_triggerPass.emplace("HLT_mu50", is_1lep_trig_matched(sl, "HLT_mu50", m_triggerLeptons, 51));

        // TODO: Add to SusyNt, ADD_2LEP_TRIGGER_VAR(HLT_2mu14, m_mu0, m_mu1)
        m_triggerPass.emplace("HLT_mu22_mu8noL1", is_2lep_trig_matched(sl, "HLT_mu22_mu8noL1", m_mu0, m_mu1, 23, 9));

        m_triggerPass.emplace("HLT_e17_lhloose_nod0_mu14", is_2lep_trig_matched(sl, "HLT_e17_lhloose_nod0_mu14", m_el0, m_mu0, 18, 15));
        m_triggerPass.emplace("HLT_e7_lhmedium_nod0_mu24", is_2lep_trig_matched(sl, "HLT_e7_lhmedium_nod0_mu24", m_el0, m_mu0, 8, 25));

        ////////////////////////////////////////////////////////////////////////////
        // 2017-2018
        m_triggerPass.emplace("HLT_2e24_lhvloose_nod0", is_2lep_trig_matched(sl, "HLT_2e24_lhvloose_nod0", m_el0, m_el1, 25, 25));

        m_triggerPass.emplace("HLT_e26_lhmedium_nod0_mu8noL1", is_2lep_trig_matched(sl, "HLT_e26_lhmedium_nod0_mu8noL1", m_el0, m_mu0, 27, 9));

        ////////////////////////////////////////////////////////////////////////////
        // 2018
        // L1_2EM15VHI was accidentally prescaled in periods B5-B8 of 2017
        // (runs 326834-328393) with an effective reduction of 0.6 fb-1
        m_triggerPass.emplace("HLT_2e17_lhvloose_nod0_L12EM15VHI", is_2lep_trig_matched(sl, "HLT_2e17_lhvloose_nod0_L12EM15VHI", m_el0, m_el1, 18, 18));
        
        ////////////////////////////////////////////////////////////////////////////
        // Combined triggers
        int year = sl->nt->evt()->treatAsYear;
        
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
                          || m_triggerPass.at("HLT_mu18_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_mu24");
        } else if (year == 2016) {
            passDilepTrig |= m_triggerPass.at("HLT_2e17_lhvloose_nod0")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          //|| m_triggerPass.at("HLT_2mu14")
                          //|| m_triggerPass.at("HLT_e26_lhmedium_nod0_L1EM22VHI_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        } else if (year == 2017) {
            passDilepTrig |= m_triggerPass.at("HLT_2e24_lhvloose_nod0")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          //|| m_triggerPass.at("HLT_2mu14")
                          || m_triggerPass.at("HLT_e26_lhmedium_nod0_mu8noL1")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        } else if (year == 2018) {
            passDilepTrig |= m_triggerPass.at("HLT_2e24_lhvloose_nod0")
                          || m_triggerPass.at("HLT_2e17_lhvloose_nod0_L12EM15VHI")
                          || m_triggerPass.at("HLT_mu22_mu8noL1")
                          //|| m_triggerPass.at("HLT_2mu14")
                          || m_triggerPass.at("HLT_e17_lhloose_nod0_mu14")
                          || m_triggerPass.at("HLT_e26_lhmedium_nod0_mu8noL1")
                          || m_triggerPass.at("HLT_e7_lhmedium_nod0_mu24");
        }
        m_triggerPass.emplace("dilepTrigs", passDilepTrig);

        bool passTrig = passSingleLepTrig || passDilepTrig; 
        m_triggerPass.emplace("lepTrigs", passTrig);

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

        bool found_probeLeps = m_probeLep1 != nullptr;
        if (m_add_fakes && found_probeLeps) {
            // New IParticles added to heap
            vector<const xAOD::IParticle*> leptons = to_iparticle_vec(sl, fakeToolLeps);
            m_fake_tool.addEvent(leptons);
            // Selection = final reco state for which one wants fake estimates
            string selection = m_fake_zjets_3l ? "3T,0!T,OS" : "2T,0!T,OS";  
            // Process = fake processes one aims to model with data-driven estimates 
            string process = ">=1F";
            if (m_fake_tool.getEventWeight(m_wgt, selection, process) != StatusCode::SUCCESS) { 
                cout << "ERROR: ApplyFakeFactor::getEventWeight() failed\n";
                exit(1); 
            }
            // Free up memory
            for (const xAOD::IParticle* p : leptons) { delete p; }
            leptons.clear();
        }
        ////////////////////////////////////////////////////////////////////////
        return true; // All events pass this cut
    };
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
    *sf << CutName("pass cosmic muon veto") << [](Superlink* sl) -> bool {
        return (sl->tools->passCosmicMuon(sl->baseMuons));
    };
    *sf << CutName("pass jet cleaning") << [](Superlink* sl) -> bool {
        return (sl->tools->passJetCleaning(sl->baseJets));
    };
}
void add_analysis_cuts(Superflow* sf) {
    ////////////////////////////////////////////////////////////////////////////
    // Baseline Selections
    if (m_baseline_DF || m_baseline_SF || m_fake_baseline_DF || m_fake_baseline_SF) {
        *sf << CutName("exactly two base leptons") << [](Superlink* sl) -> bool {
            return sl->baseLeptons->size() == 2;
        };
            
        if (m_baseline_DF || m_baseline_SF) {
            *sf << CutName("exactly two signal leptons") << [](Superlink* sl) -> bool {
                return (sl->leptons->size() == 2);
            };
        } else if (m_fake_baseline_DF || m_fake_baseline_SF) {
            *sf << CutName("1 signal and 1 inverted lepton") << [](Superlink* sl) -> bool {
                return (sl->leptons->size() == 1 && m_invLeps.size() == 1);
            };
        }

        *sf << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
            return (m_lep1->q * m_lep2->q < 0);
        };
        if (m_baseline_DF || m_fake_baseline_DF) {
            *sf << CutName("dilepton flavor (emu/mue)") << [](Superlink* /*sl*/) -> bool {
                return (m_lep1->isEle() != m_lep2->isEle());
            };
        } else if (m_baseline_SF || m_fake_baseline_SF) {
            *sf << CutName("dilepton flavor (ee/mumu)") << [](Superlink* /*sl*/) -> bool {
                return m_lep1->isEle() == m_lep2->isEle();
            };

            *sf << CutName("|m_ll - Zmass| > 10 GeV") << [](Superlink* /*sl*/) -> bool {
                return fabs(m_dileptonP4.M() - ZMASS) > 10;
            };
        }
        *sf << CutName("m_ll > 20 GeV") << [](Superlink* /*sl*/) -> bool {
            return m_dileptonP4.M() > 20.0;
        };
    ////////////////////////////////////////////////////////////////////////////
    // Z+Jets Fake Factor Selections
    } else if (m_zjets_3l || m_fake_zjets_3l || m_zjets2l_inc) {
        if (m_zjets_3l) {
            *sf << CutName("3 signal leptons") << [](Superlink* sl) -> bool {
                return (sl->leptons->size() == 3 && m_invLeps.size() == 0);
            };
        } else if (m_fake_zjets_3l) {
            *sf << CutName("2 signal and 1 inverted lepton") << [](Superlink* sl) -> bool {
                return (sl->leptons->size() == 2 && m_invLeps.size() == 1);
            };
        } else if (m_zjets2l_inc) {
            *sf << CutName("2+ signal leptons") << [](Superlink* sl) -> bool {
                return (sl->leptons->size() >= 2);
            };
        }
        *sf << CutName("opposite sign") << [](Superlink* /*sl*/) -> bool {
            return (m_lep1->q * m_lep2->q < 0);
        };
        *sf << CutName("Z dilepton flavor (ee/mumu)") << [](Superlink* /*sl*/) -> bool {
            return m_lep1->isEle() == m_lep2->isEle();
        };
        *sf << CutName("|mZ_ll - Zmass| < 10 GeV") << [](Superlink* /*sl*/) -> bool {
            return fabs(m_dileptonP4.M() - ZMASS) < 10;
        };
    }
}

void add_4bcutflow_cuts(Superflow* sf) {
    *sf << CutName("exactly two base leptons") << [](Superlink* sl) -> bool {
        return sl->baseLeptons->size() == 2;
    };
    
    *sf << CutName("opposite sign") << [](Superlink* sl) -> bool {
        return (sl->baseLeptons->at(0)->q * sl->baseLeptons->at(1)->q < 0);
    };
    
    *sf << CutName("isolation") << [](Superlink* sl) -> bool {
        bool passIso1 = sl->baseLeptons->at(0)->isEle() ? sl->baseLeptons->at(0)->isoGradient : sl->baseLeptons->at(0)->isoFCLoose;
        bool passIso2 = sl->baseLeptons->at(1)->isEle() ? sl->baseLeptons->at(1)->isoGradient : sl->baseLeptons->at(1)->isoFCLoose;
        return passIso1 && passIso2;
    };
    
    *sf << CutName("exactly two signal leptons") << [](Superlink* sl) -> bool {
        return (sl->leptons->size() == 2);
    };
    
    *sf << CutName("truth origin") << [](Superlink* /*sl*/) -> bool {
        bool promptLep1 = m_lep1->mcOrigin == 10 || // Top
                          m_lep1->mcOrigin == 12 || // W
                          m_lep1->mcOrigin == 13 || // Z
                          m_lep1->mcOrigin == 14 || // Higgs
                          m_lep1->mcOrigin == 43;   // Diboson
        bool promptLep2 = m_lep2->mcOrigin == 10 || // Top
                          m_lep2->mcOrigin == 12 || // W
                          m_lep2->mcOrigin == 13 || // Z
                          m_lep2->mcOrigin == 14 || // Higgs
                          m_lep2->mcOrigin == 43;   // Diboson
        return promptLep1 && promptLep2;
    };
    
    *sf << CutName("eta requirement") << [](Superlink* /*sl*/) -> bool {
        float eta1 = fabs(m_lep1->eta);
        float eta2 = fabs(m_lep2->eta);
        bool passEta1 = m_lep1->isEle() ? eta1 < 2.47 : eta1 < 2.4;
        bool passEta2 = m_lep2->isEle() ? eta2 < 2.47 : eta2 < 2.4;
        return passEta1 && passEta2;
    };


    *sf << CutName("m_ll > 20 GeV") << [](Superlink* /*sl*/) -> bool {
        return m_dileptonP4.M() > 20.0;
    };

    *sf << CutName("lep1Pt > 25GeV") << [](Superlink* /*sl*/) -> bool {
        return m_lep1->Pt() > 25.0;
    };

    *sf << CutName("lep2Pt > 20GeV") << [](Superlink* /*sl*/) -> bool {
        return m_lep2->Pt() > 20.0;
    };

    *sf << CutName("MET > 250GeV") << [](Superlink* /*sl*/) -> bool {
        return m_MET.Pt() > 250.0;
    };
}

void add_event_variables(Superflow* sf) {
    // Event weights
    *sf << NewVar("event weight"); {
        *sf << HFTname("eventweight");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->weights->product() * sl->nt->evt()->wPileup; };
        *sf << SaveVar();
    }

    *sf << NewVar("event weight (multi period)"); {
        *sf << HFTname("eventweight_multi");
        *sf << [](Superlink* sl, var_double*) -> double { return sl->weights->product_multi() * sl->nt->evt()->wPileup; };
        *sf << SaveVar();
    }

    *sf << NewVar("event weight (single period)"); {
        *sf << HFTname("eventweight_single");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->product() * sl->nt->evt()->wPileup / sl->nt->evt()->wPileup_period; };
        *sf << SaveVar();
    }

    *sf << NewVar("event weight (no scale factors)"); {
        *sf << HFTname("eventweight_noSF");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt;};
        *sf << SaveVar();
    }

    *sf << NewVar("multiperiod event weight (no scale factors)"); {
        *sf << HFTname("eventweight_noSF_multi");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->weights->susynt_multi;};
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
    if (m_add_fakes) {
        *sf << NewVar("fake factor weight"); {
            *sf << HFTname("fakeweight");
            *sf << [](Superlink* /*sl*/, var_double*) -> double { return m_wgt.value; };
            *sf << SaveVar();
        }
    }

    // Scale Factors
    *sf << NewVar("Pile-up weight"); {
        *sf << HFTname("pupw");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->nt->evt()->wPileup;};
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
        *sf << [](Superlink* sl, var_bool*) -> bool { return sl->nt->evt()->isMC ? true : false; };
        *sf << SaveVar();
    }

    *sf << NewVar("mcChannel (dsid)"); {
        *sf << HFTname("dsid");
        *sf << [](Superlink* sl, var_double*) -> double {return sl->isMC ? sl->nt->evt()->mcChannel : 0.0;};
        *sf << SaveVar();
    }

    *sf << NewVar("treatAsYear"); {
        // 15/16 Year ID
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

    *sf << NewVar("Pileup density"); {
        *sf << HFTname("pileup_density");
        *sf << [](Superlink* sl, var_double*) -> double {
            float actual_mu = sl->nt->evt()->actualMu;
            float sigmaZ = sl->nt->evt()->beamPosSigmaZ;
            float beamPosZ = sl->nt->evt()->beamPosZ;
            float pvZ = sl->nt->evt()->pvZ;

            pu_profile.SetParameter(0, actual_mu); // normalization
            pu_profile.SetParameter(1, beamPosZ); // mean
            pu_profile.SetParameter(2, sigmaZ); // sigma

            return pu_profile.Eval(pvZ);
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
    
}
void add_lepton_variables(Superflow* sf) {

    ADD_LEPTON_VARS(lep1)
    ADD_LEPTON_VARS(lep2)

    //*sf << NewVar("lep1 truth type"); {
    //    *sf << HFTname("lep1TruthType");
    //    *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcType; };
    //    *sf << SaveVar();
    //}
    //*sf << NewVar("lep1 truth origin"); {
    //    *sf << HFTname("lep1TruthOrigin");
    //    *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcOrigin; };
    //    *sf << SaveVar();
    //}
    //*sf << NewVar("lep1 truth mother PDG ID"); {
    //    *sf << HFTname("lep1TruthMotherPdgID");
    //    *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcBkgMotherPdgId; };
    //    *sf << SaveVar();
    //}
    //*sf << NewVar("lep1 truth mother origin"); {
    //    *sf << HFTname("lep1TruthMotherOrigin");
    //    *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_lep1->mcBkgTruthOrigin; };
    //    *sf << SaveVar();
    //}

    *sf << NewVar("number of signal leptons"); {
        *sf << HFTname("nSigLeps");
        *sf << [](Superlink* sl, var_int*) -> int { return sl->leptons->size();};
        *sf << SaveVar();
    }
    
    *sf << NewVar("number of inverted leptons"); {
        *sf << HFTname("nInvLeps");
        *sf << [](Superlink* /*sl*/, var_int*) -> int { return m_invLeps.size();};
        *sf << SaveVar();
    }

    //////////////////////////////////////////////////////////////////////////////
    // Electrons
    *sf << NewVar("Electron ID (non-inclusive)"); {
      *sf << HFTname("El_ID");
      *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
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
      *sf << SaveVar();
    }
    //////////////////////////////////////////////////////////////////////////////
    // Muons
    *sf << NewVar("Muon ID (non-inclusive)"); {
      *sf << HFTname("Mu_ID");
      *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
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
      *sf << SaveVar();
    }
}
void add_jet_variables(Superflow* sf) {
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
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() && m_lep2->isEle(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is mu + mu"); {
        *sf << HFTname("isMuMu");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isMu() && m_lep2->isMu(); };
        *sf << SaveVar();
    }
    
    *sf << NewVar("is mu (lead) + e (sub)"); {
        *sf << HFTname("isMuEl");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isMu() && m_lep2->isEle(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is e (lead) + mu (sub)"); {
        *sf << HFTname("isElMu");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() && m_lep2->isMu(); };
        *sf << SaveVar();
    }

    *sf << NewVar("is opposite-sign"); {
        *sf << HFTname("isOS");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->q * m_lep2->q < 0; };
        *sf << SaveVar();
    }

    *sf << NewVar("is e + mu"); {
        *sf << HFTname("isDF");
        *sf << [](Superlink* /*sl*/, var_bool*) -> bool { return m_lep1->isEle() ^ m_lep2->isEle(); };
        *sf << SaveVar();
    }


    *sf << NewVar("mass of di-lepton system"); {
        *sf << HFTname("mll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {return m_dileptonP4.M();};
        *sf << SaveVar();
    }

    *sf << NewVar("Pt of di-lepton system"); {
        *sf << HFTname("pTll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_dileptonP4.Pt(); };
        *sf << SaveVar();
    }

    *sf << NewVar("Pt difference of di-lepton system"); {
        *sf << HFTname("dpTll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_lep1->Pt() - m_lep2->Pt(); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Eta of di-lepton system"); {
        *sf << HFTname("deta_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lep1->Eta() - m_lep2->Eta()); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Phi of di-lepton system"); {
        *sf << HFTname("dphi_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return abs(m_lep1->DeltaPhi(*m_lep2)); };
        *sf << SaveVar();
    }

    *sf << NewVar("delta R of di-lepton system"); {
        *sf << HFTname("dR_ll");
        *sf << [](Superlink* /*sl*/, var_float*) -> double { return m_lep1->DeltaR(*m_lep2); };
        *sf << SaveVar();
    }
}
void add_multi_object_variables(Superflow* sf) {
    // Jets and MET
    *sf << NewVar("delta Phi of leading jet and met"); {
        *sf << HFTname("deltaPhi_met_jet1");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("delta Phi of subleading jet and met"); {
        *sf << HFTname("deltaPhi_met_jet2");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 2 ? sl->jets->at(1)->DeltaPhi(m_MET) : -DBL_MAX;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("jet-1 Pt"); {
        *sf << HFTname("jet1Pt");
        *sf << [](Superlink* sl, var_float*) -> double {
            return sl->jets->size() >= 1 ? sl->jets->at(0)->Pt() : -DBL_MAX;
        };
        *sf << SaveVar();
    }

    *sf << NewVar("stransverse mass"); {
        *sf << HFTname("MT2");
        *sf << [](Superlink* sl, var_float*) -> double {
            double mt2_ = kin::getMT2(*sl->leptons, *sl->met);
            return mt2_;
        };
        *sf << SaveVar();
    }

    // Leptons, Jets, and MET
    *sf << NewVar("Etmiss Rel"); {
        *sf << HFTname("metrel");
        *sf << [](Superlink* sl, var_float*) -> double { return kin::getMetRel(sl->met, *sl->leptons, *sl->jets); };
        *sf << SaveVar();
    }

    *sf << NewVar("Ht (m_Eff: lep + met + jet)"); {
        *sf << HFTname("ht");
        *sf << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;

            ht += m_lep1->Pt() + m_lep2->Pt();
            ht += sl->met->Et;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                ht += sl->jets->at(i)->Pt();
            }
            return ht;
        };
        *sf << SaveVar();
    }
    *sf << NewVar("reco-level max(HT,pTV) "); {
        *sf << HFTname("max_HT_pTV_reco");
        *sf << [](Superlink* sl, var_float*) -> double {
            double ht = 0.0;
            for (int i = 0; i < (int)sl->jets->size(); i++) {
                if (sl->jets->at(i)->Pt() < 20) {continue;}
                ht += sl->jets->at(i)->Pt();
            }
            return max(ht, m_dileptonP4.Pt());
        };
        *sf << SaveVar();
    }
}

void add_jigsaw_variables(Superflow* sf) {
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
void add_miscellaneous_variables(Superflow* sf) {

    *sf << NewVar("|cos(theta_b)|"); {
        *sf << HFTname("abs_costheta_b");
        *sf << [](Superlink* /*sl*/, var_float*) -> double {
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
        *sf << SaveVar();
    }
}

void add_fakefactor_variables(Superflow* sf) {
    ADD_LEPTON_VARS(probeLep1)
    //ADD_LEPTON_VARS(probeLep2)
}

void add_zjets_fakefactor_variables(Superflow* sf) {
    *sf << NewVar("Mll of 2nd-closest Z pair"); {
      *sf << HFTname("Z2_mll");
      *sf << [](Superlink* sl, var_double*) -> double {
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
      *sf << SaveVar();
    }
    *sf << NewVar("Mlll: Invariant mass of 3lep system"); {
      *sf << HFTname("mlll");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
          return (*m_lep1 + *m_lep2 + *m_probeLep1).M();
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and Zlep1"); {
      *sf << HFTname("dR_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep1).DeltaR(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and Zlep2"); {
      *sf << HFTname("dR_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep2).DeltaR(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaR of probeLep1 and Z"); {
      *sf << HFTname("dR_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return m_dileptonP4.DeltaR(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and Zlep1"); {
      *sf << HFTname("dPhi_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep1).DeltaPhi(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and Zlep2"); {
      *sf << HFTname("dPhi_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return (*m_lep2).DeltaPhi(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaPhi of probeLep1 and Z"); {
      *sf << HFTname("dPhi_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return m_dileptonP4.DeltaPhi(*m_probeLep1);
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and Zlep1"); {
      *sf << HFTname("dEta_ZLep1_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return m_probeLep1->Eta() - m_lep1->Eta() ;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and Zlep2"); {
      *sf << HFTname("dEta_ZLep2_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return m_probeLep1->Eta() - m_lep2->Eta() ;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("DeltaEta of probeLep1 and Z"); {
      *sf << HFTname("dEta_Z_probeLep1");
      *sf << [](Superlink* /*sl*/, var_double*) -> double {
        return m_probeLep1->Eta() - m_dileptonP4.Eta() ;
      };
      *sf << SaveVar();
    }
}

void add_zjets2l_inc_variables(Superflow* sf) {
    *sf << NewVar("lepton pt"); {
      *sf << HFTname("lepPt");
      *sf << [=](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : *sl->leptons) { out.push_back(lepton->Pt()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("lepton eta"); {
      *sf << HFTname("lepEta");
      *sf << [=](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : *sl->leptons) { out.push_back(lepton->Eta()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("lepton phi"); {
      *sf << HFTname("lepPhi");
      *sf << [=](Superlink* sl, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : *sl->leptons) { out.push_back(lepton->Phi()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("lepton charge"); { 
        *sf << HFTname("lepq"); 
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> { 
            vector<int> out;
            for(auto& lepton : *sl->leptons) { out.push_back(lepton->q); }
            return out;
        };
        *sf << SaveVar(); 
    } 
    
    *sf << NewVar("lepton flavor"); { 
        *sf << HFTname("lepFlav"); 
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for(auto& lepton : *sl->leptons) { out.push_back(lepton->isEle()); }
            return out;
        };
        *sf << SaveVar(); 
    } 
    
    *sf << NewVar("lepton d0sigBSCorr"); { 
      *sf << HFTname("lep_d0sigBSCorr"); 
      *sf << [](Superlink* sl, var_float_array*) -> vector<double> {
            vector<double> out;
            for(auto& lepton : *sl->leptons) { out.push_back(lepton->d0sigBSCorr); }
            return out;
        };
      *sf << SaveVar(); 
    } 
    
    *sf << NewVar("lepton z0SinTheta"); { 
      *sf << HFTname("lep_z0SinTheta"); 
      *sf << [](Superlink* sl, var_float_array*) -> vector<double> {
            vector<double> out;
            for(auto& lepton : *sl->leptons) { out.push_back(lepton->z0SinTheta()); }
            return out;
        };
      *sf << SaveVar(); 
    } 
    
    *sf << NewVar("lepton Truth Class"); { 
        *sf << HFTname("lepTruthClass"); 
        *sf << [](Superlink* sl, var_int_array*) -> vector<int> {
            vector<int> out;
            for(auto& lepton : *sl->leptons) { out.push_back(get_lepton_truth_class(lepton)); }
            return out;
        };
        *sf << SaveVar(); 
    } 
    ////////////////////////////////////////
    // Anti-ID variables
    *sf << NewVar("inverted lepton pt"); {
      *sf << HFTname("lepPt");
      *sf << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_invLeps) { out.push_back(lepton->Pt()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("inverted lepton eta"); {
      *sf << HFTname("lepEta");
      *sf << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_invLeps) { out.push_back(lepton->Eta()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("inverted lepton phi"); {
      *sf << HFTname("invLepPhi");
      *sf << [=](Superlink* /*sl*/, var_float_array*) -> vector<double> {
        vector<double> out;
        for(auto& lepton : m_invLeps) { out.push_back(lepton->Phi()); }
        return out;
      };
      *sf << SaveVar();
    }
    *sf << NewVar("inverted lepton charge"); { 
        *sf << HFTname("invLepq"); 
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> { 
            vector<int> out;
            for(auto& lepton : m_invLeps) { out.push_back(lepton->q); }
            return out;
        };
        *sf << SaveVar(); 
    } 
    
    *sf << NewVar("inverted lepton flavor"); { 
        *sf << HFTname("invLepFlav"); 
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for(auto& lepton : m_invLeps) { out.push_back(lepton->isEle()); }
            return out;
        };
        *sf << SaveVar(); 
    } 
    
    *sf << NewVar("inverted lepton d0sigBSCorr"); { 
      *sf << HFTname("invLep_d0sigBSCorr"); 
      *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> {
            vector<double> out;
            for(auto& lepton : m_invLeps) { out.push_back(lepton->d0sigBSCorr); }
            return out;
        };
      *sf << SaveVar(); 
    } 
    
    *sf << NewVar("inverted lepton z0SinTheta"); { 
      *sf << HFTname("invLep_z0SinTheta"); 
      *sf << [](Superlink* /*sl*/, var_float_array*) -> vector<double> {
            vector<double> out;
            for(auto& lepton : m_invLeps) { out.push_back(lepton->z0SinTheta()); }
            return out;
        };
      *sf << SaveVar(); 
    } 
    
    *sf << NewVar("inverted lepton Truth Class"); { 
        *sf << HFTname("invLepTruthClass_aID"); 
        *sf << [](Superlink* /*sl*/, var_int_array*) -> vector<int> {
            vector<int> out;
            for(auto& lepton : m_invLeps) { out.push_back(get_lepton_truth_class(lepton)); }
            return out;
        };
        *sf << SaveVar(); 
    } 
}

void add_weight_systematics(Superflow* sf) {
    if (m_add_fakes) {
        *sf << NewVar("shift in fake factor from statistical uncertainty"); {
            *sf << HFTname("syst_FAKEFACTOR_Stat");
            *sf << [](Superlink* /*sl*/, var_double*) -> double { 
                double statUnc = 1;
                for(auto& kv : m_wgt.uncertainties) {
                    if (m_fake_tool.isStatisticalUncertainty(kv.first)) {
                        statUnc *= 0.5 * (fabs(kv.second.up) + fabs(kv.second.down));
                    }
                }
                return statUnc; 
            };
            *sf << SaveVar();
        }
        *sf << NewVar("shift in fake factor from statistical uncertainty"); {
            *sf << HFTname("syst_FAKEFACTOR_Syst");
            *sf << [](Superlink* /*sl*/, var_double*) -> double { 
                float systUnc = 1;
                for(auto& kv : m_wgt.uncertainties) {
                    if (m_fake_tool.isSystematicUncertainty(kv.first)) {
                        systUnc *= 0.5 * (fabs(kv.second.up) + fabs(kv.second.down));
                    }
                }
                return systUnc; 
            };
            *sf << SaveVar();
        }
    }
    *sf << NewSystematic("FTAG EFF B"); {
        *sf << WeightSystematic(SupersysWeight::FT_EFF_B_UP, SupersysWeight::FT_EFF_B_DN);
        *sf << TreeName("FT_EFF_B");
        *sf << SaveSystematic();
    }
    //*sf << NewSystematic("shift in electron ID efficiency"); {
    //    *sf << WeightSystematic(SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_ID_TOTAL_Uncorr_DN);
    //    *sf << TreeName("EL_EFF_ID");
    //    *sf << SaveSystematic();
    //}
    //*sf << NewSystematic("shift in electron ISO efficiency"); {
    //    *sf << WeightSystematic(SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Iso_TOTAL_Uncorr_DN);
    //    *sf << TreeName("EL_EFF_Iso");
    //    *sf << SaveSystematic();
    //}
    //*sf << NewSystematic("shift in electron RECO efficiency"); {
    //    *sf << WeightSystematic(SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_UP, SupersysWeight::EL_EFF_Reco_TOTAL_Uncorr_DN);
    //    *sf << TreeName("EL_EFF_Reco");
    //    *sf << SaveSystematic();
    //}
}
void add_shape_systematics(Superflow* sf) {
    (void)sf;
    //*sf << NewSystematic("shift in e-gamma resolution (UP)"); {
    //    *sf << EventSystematic(NtSys::EG_RESOLUTION_ALL_UP);
    //    *sf << TreeName("EG_RESOLUTION_ALL_UP");
    //    *sf << SaveSystematic();
    //}
    //*sf << NewSystematic("shift in e-gamma resolution (DOWN)"); {
    //    *sf << EventSystematic(NtSys::EG_RESOLUTION_ALL_DN);
    //    *sf << TreeName("EG_RESOLUTION_ALL_DN");
    //    *sf << SaveSystematic();
    //}
    //*sf << NewSystematic("shift in e-gamma scale (UP)"); {
    //    *sf << EventSystematic(NtSys::EG_SCALE_ALL_UP);
    //    *sf << TreeName("EG_SCALE_ALL_UP");
    //    *sf << SaveSystematic();
    //}
}

bool is_1lep_trig_matched(Superlink* sl, string trig_name, LeptonVector leptons, float pt_min) {
    for (Susy::Lepton* lep : leptons) {
        if(!lep) continue;
        if (lep->Pt() < pt_min) continue;
        bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
        if (!trig_fired) continue;
        bool trig_matched = sl->tools->triggerTool().lepton_trigger_match(lep, trig_name);
        if (trig_matched) return true;
    }
    return false;
}

bool is_2lep_trig_matched(Superlink* sl, string trig_name, Susy::Lepton* lep1, Susy::Lepton* lep2, float pt_min1, float pt_min2) {
    if(!lep1 || ! lep2) return false;
    if (lep1->Pt() < pt_min1 || lep2->Pt() < pt_min2) return false;
    bool trig_fired = sl->tools->triggerTool().passTrigger(sl->nt->evt()->trigBits, trig_name);
    if (!trig_fired) return false;
    bool trig_matched = sl->tools->triggerTool().dilepton_trigger_match(sl->nt->evt(), lep1, lep2, trig_name);
    return trig_matched;
}

bool is_antiID_lepton(Susy::Lepton* lepton) {
    bool pt_pass = 0, eta_pass = 0, iso_pass = 0, id_pass = 0, ip_pass = 0;
    bool passID_cuts = 0, passAntiID_cuts = 0;
    if (lepton->isEle()) {
        const Susy::Electron* ele = dynamic_cast<const Susy::Electron*>(lepton);
        //float absEtaBE = fabs(ele->clusEtaBE);
        pt_pass  = ele->pt > 0; //15;
        //eta_pass = (absEtaBE < 1.37 || 1.52 < absEtaBE) && fabs(ele->eta) < 2.47;
        eta_pass = fabs(ele->eta) < 2.47;
        iso_pass = ele->isoGradient;
        id_pass  = ele->mediumLLH;
        ip_pass = ele->d0sigBSCorr < 5;
        passID_cuts = iso_pass && id_pass;
        passAntiID_cuts = ele->looseLLHBLayer;
    } else if (lepton->isMu()) {
        const Susy::Muon* mu = dynamic_cast<const Susy::Muon*>(lepton);
        pt_pass  = mu->pt > 0; //10;
        eta_pass = fabs(mu->eta) < 2.7;
        iso_pass = mu->isoFCLoose;
        id_pass  = mu->medium;
        ip_pass = mu->d0sigBSCorr < 3;
        passID_cuts = iso_pass && id_pass;
        passAntiID_cuts = mu->medium;
    }
    return pt_pass && eta_pass && ip_pass && passAntiID_cuts && !passID_cuts;
}
bool is_ID_lepton(Superlink* sl, Susy::Lepton* lepton) {
    auto it = find(sl->leptons->begin(), sl->leptons->end(), lepton);
    return it != sl->leptons->end();
    //for (Susy::Lepton* sig_lepton : *sl->leptons) {
    //    if (lepton == sig_lepton) {
    //        return true;
    //    }
    //}
    //return false;
}

int get_lepton_truth_class(Susy::Lepton* lepton) {
    if (lepton==nullptr) return -INT_MAX;

    // Get Truth information
    int T = lepton->mcType;
    int O = lepton->mcOrigin;
    int MO = lepton->mcBkgTruthOrigin; // TODO: Update. BkgTruthOrigin != motherOrigin
    int MT = 0; // Not stored in SusyNt::Lepton
    int M_ID = lepton->mcBkgMotherPdgId;
    //int MO = lepton->mcFirstEgMotherTruthOrigin;
    //int MT = lepton->mcFirstEgMotherTruthType;
    //int M_ID = lepton->mcFirstEgMotherPdgId;

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

vector<const xAOD::IParticle*> to_iparticle_vec(Superlink* sl, LeptonVector leps) {
    vector<const xAOD::IParticle*> particles;
    for (Susy::Lepton* lep : leps) {
        // IParticle added to heap
        bool isTight = is_ID_lepton(sl, lep);
        const xAOD::IParticle* ipar = to_iparticle(lep, isTight);
        particles.push_back(ipar);
    }
    return particles;
}

xAOD::IParticle* to_iparticle(Susy::Lepton* lep, bool isTight) {
    if (lep->isEle()) {
        xAOD::Electron* e = new xAOD::Electron();
        e->makePrivateStore();
        e->setP4(lep->Pt() * GeVtoMeV, lep->Eta(), lep->Phi(), lep->M());
        e->setCharge(lep->q);
        e->auxdata<char>("Tight") = isTight;
        return static_cast<xAOD::IParticle*>(e);
    } else {
        xAOD::Muon* m = new xAOD::Muon();
        m->makePrivateStore();
        m->setP4(lep->Pt() * GeVtoMeV, lep->Eta(), lep->Phi());
        m->setCharge(lep->q);
        m->auxdata<char>("Tight") = isTight;
        return static_cast<xAOD::IParticle*>(m);
    }
}
