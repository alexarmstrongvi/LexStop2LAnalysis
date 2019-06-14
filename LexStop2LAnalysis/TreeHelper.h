#pragma once

// std
#include <string>
using std::string;
#include <sstream>
using std::stringstream;
#include <iostream>
using std::cout;
using std::cerr;

// addFakeFactor (TreeHelperBase)
#include "LexStop2LAnalysis/addFakeFactor.h"

// ROOT
#include "TTree.h"
#include "TLorentzVector.h"

////////////////////////////////////////////////////////////////////////////////
// User implementation of TreeHelper
// Eveything below here is specific to the input ttree the user intends to use
////////////////////////////////////////////////////////////////////////////////

#define BUILD_LEPTON(lp, lep, isMCevent) { \
        lp.isEle = lep##Flav == 0; \
        lp.Pt = lep##Pt; \
        lp.Eta = lep##Eta; \
        lp.Phi = lep##Phi; \
        TLorentzVector tlv; \
        tlv.SetPtEtaPhiE(lp.Pt, lp.Eta, lp.Phi, lep##E); \
        lp.M = tlv.M(); \
        lp.Eta = lep##Eta; \
        lp.q = lep##q; \
        lp.truthStatus = isMCevent ? get_truth_status(lep##TruthClass) : TruthStatus::UNDEFINED; \
}


class TreeHelper : public TreeHelperBase
{
  public:
    TreeHelper(TTree* tt) : TreeHelperBase(tt) {}
    ~TreeHelper() {}

    //////////////////////////////////////////////////////////////////////////////
    // Define branch variables
    int eventNumber;
    bool isMCevent;
    int nSigLeps;
    int nInvLeps;

    int lep1Flav;
    float lep1Pt;
    float lep1Eta;
    float lep1Phi;
    float lep1E;
    int lep1q;
    int lep1TruthClass;

    int lep2Flav;
    float lep2Pt;
    float lep2Eta;
    float lep2Phi;
    float lep2E;
    int lep2q;
    int lep2TruthClass;

    int probeLep1Flav;
    float probeLep1Pt;
    float probeLep1Eta;
    float probeLep1Phi;
    float probeLep1E;
    int probeLep1q;
    int probeLep1TruthClass;

    int probeLep2Flav;
    float probeLep2Pt;
    float probeLep2Eta;
    float probeLep2Phi;
    float probeLep2E;
    int probeLep2q;
    int probeLep2TruthClass;

    // Base class functions
    void initialize() override {
        tr().SetBranchAddress("eventNumber", &eventNumber);
        tr().SetBranchAddress("isMC", &isMCevent);
        tr().SetBranchAddress("nSigLeps", &nSigLeps);
        tr().SetBranchAddress("nInvLeps", &nInvLeps);
        tr().SetBranchAddress("lep1Flav", &lep1Flav);
        tr().SetBranchAddress("lep1Pt", &lep1Pt);
        tr().SetBranchAddress("lep1Eta", &lep1Eta);
        tr().SetBranchAddress("lep1Phi", &lep1Phi);
        tr().SetBranchAddress("lep1E", &lep1E);
        tr().SetBranchAddress("lep1q", &lep1q);
        tr().SetBranchAddress("lep1TruthClass", &lep1TruthClass);
        tr().SetBranchAddress("lep2Flav", &lep2Flav);
        tr().SetBranchAddress("lep2Pt", &lep2Pt);
        tr().SetBranchAddress("lep2Eta", &lep2Eta);
        tr().SetBranchAddress("lep2Phi", &lep2Phi);
        tr().SetBranchAddress("lep2E", &lep2E);
        tr().SetBranchAddress("lep2q", &lep2q);
        tr().SetBranchAddress("lep2TruthClass", &lep2TruthClass);
        tr().SetBranchAddress("probeLep1Flav", &probeLep1Flav);
        tr().SetBranchAddress("probeLep1Pt", &probeLep1Pt);
        tr().SetBranchAddress("probeLep1Eta", &probeLep1Eta);
        tr().SetBranchAddress("probeLep1Phi", &probeLep1Phi);
        tr().SetBranchAddress("probeLep1E", &probeLep1E);
        tr().SetBranchAddress("probeLep1q", &probeLep1q);
        tr().SetBranchAddress("probeLep1TruthClass", &probeLep1TruthClass);
        tr().SetBranchAddress("probeLep2Flav", &probeLep2Flav);
        tr().SetBranchAddress("probeLep2Pt", &probeLep2Pt);
        tr().SetBranchAddress("probeLep2Eta", &probeLep2Eta);
        tr().SetBranchAddress("probeLep2Phi", &probeLep2Phi);
        tr().SetBranchAddress("probeLep2E", &probeLep2E);
        tr().SetBranchAddress("probeLep2q", &probeLep2q);
        tr().SetBranchAddress("probeLep2TruthClass", &probeLep2TruthClass);
    }

    int event_number() override { return eventNumber; }
    bool isMC() override { return isMCevent; }

    void build_leptons() override {
        if (nSigLeps > 0) {
            leptonProperties id_lep1;
            BUILD_LEPTON(id_lep1, lep1, isMCevent);
            id_lep1.recoStatus = RecoStatus::ID;
            _leptons.push_back(id_lep1);
        }
        if (nSigLeps > 1) {
            leptonProperties id_lep2;
            BUILD_LEPTON(id_lep2, lep2, isMCevent);
            id_lep2.recoStatus = RecoStatus::ID;
            _leptons.push_back(id_lep2);
        }
        if (nSigLeps > 2) { 
            leptonProperties id_lep3;
            BUILD_LEPTON(id_lep3, probeLep1, isMCevent);
            _leptons.push_back(id_lep3); 
            if (nSigLeps > 3) {
                cout << "WARNING :: Not setup to handle events with >3 ID leptons. Duplicating 3rd ID lepton.\n";
                _leptons.push_back(id_lep3);
            }
        }
       
        if (nInvLeps > 0) { 
            leptonProperties anti_id_lep1;
            BUILD_LEPTON(anti_id_lep1, probeLep1, isMCevent);
            anti_id_lep1.recoStatus = RecoStatus::ANTIID;
            _leptons.push_back(anti_id_lep1);
        } 
        if (nInvLeps > 1) { 
            leptonProperties anti_id_lep2;
            BUILD_LEPTON(anti_id_lep2, probeLep2, isMCevent);
            anti_id_lep2.recoStatus = RecoStatus::ANTIID;
            _leptons.push_back(anti_id_lep2);
            //cout << "WARNING :: Not setup to handle events with >1 anti-ID leptons. Duplicating 1st anti-ID lepton.\n";
            //_leptons.push_back(anti_id_lep1);
            if (nInvLeps > 2) { 
                cout << "WARNING :: Not setup to handle events with >2 anti-ID leptons. Duplicating 2nd anti-ID lepton.\n";
                _leptons.push_back(anti_id_lep2); 
            }
            if (nInvLeps > 3) { 
                cout << "WARNING :: Not setup to handle events with >3 anti-ID leptons. Duplicating 2nd anti-ID lepton.\n";
                _leptons.push_back(anti_id_lep2); 
            }
        }
    }

    // User-specific helper functions
    inline TruthStatus get_truth_status(int truth_class) {
        if (truth_class == 1 || truth_class == 2) {
            return TruthStatus::PROMPT;
        } else {
            return TruthStatus::FAKE;
        }
    }
  
};
