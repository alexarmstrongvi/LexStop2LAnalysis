#pragma once

// std
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <sstream>
using std::stringstream;

// FakeBkdTools
#include "FakeBkgTools/ApplyFakeFactor.h"

// xAOD
#include "xAODBase/IParticle.h"

// ROOT
#include "TTree.h"

////////////////////////////////////////////////////////////////////////////////
// Configurable options
string m_fakeweight_branch_name = "fakeweight";
string m_fakeweight_stat_err_branch_name = "syst_FAKEFACTOR_Stat";
string m_fakeweight_syst_err_branch_name = "syst_FAKEFACTOR_Syst";

////////////////////////////////////////////////////////////////////////////////
// Enumerations
enum class TruthStatus { UNDEFINED, PROMPT, FAKE };
inline const char* toString(TruthStatus s) {
    switch (s) {
        case TruthStatus::UNDEFINED: return "Undefined";
        case TruthStatus::PROMPT:    return "Prompt";
        case TruthStatus::FAKE:      return "Fake";
        default:                     return "[Unknown TruthStatus]";
    }
}
enum class RecoStatus { UNDEFINED, ID, ANTIID, OTHER };
inline const char* toString(RecoStatus s) {
    switch (s) {
        case RecoStatus::UNDEFINED: return "Undefined";
        case RecoStatus::ID:        return "ID";
        case RecoStatus::ANTIID:    return "Anti-ID";
        case RecoStatus::OTHER:     return "Other";
        default:                    return "[Unknown RecoStatus]";
    }
}
struct leptonProperties {
    bool isEle; // False implies muon
    double Pt;  // GeV
    double Eta; // clusterBE2 eta for electrons
    double Phi;
    double M;
    int q;
    RecoStatus recoStatus;
    TruthStatus truthStatus;

    string info_str() const {
        stringstream ss;
        ss << "(isEle, recoStatus, truthStatus) = ("
           << isEle << ", " 
           << toString(recoStatus) << ", " 
           << toString(truthStatus) << "); "
           << "(pT [GeV], eta) = (" 
           << Pt << ", " << Eta << "); " 
           << "(phi, M, Q) = (" 
           << Phi << ", " << M << ", " << q << "); ";
        return ss.str();
    }

    void clear() {
        isEle = false; 
        recoStatus = RecoStatus::UNDEFINED;
        truthStatus = TruthStatus::UNDEFINED;
        Pt = Eta = Phi = M = -99.0;
        q = -99;
    }
};

////////////////////////////////////////////////////////////////////////////////
bool initialize_fakefactor_tool(ApplyFakeFactor& tool, string input_file);
const xAOD::IParticle* to_iparticle(const leptonProperties& lp); 
bool all_prompt(const vector<leptonProperties>& lps);
double get_fake_weight(const ApplyFakeFactor& /*tool*/, FakeBkgTools::Weight& wgt);
double get_stat_error(const ApplyFakeFactor& tool, FakeBkgTools::Weight& wgt, double nom);
double get_syst_error(const ApplyFakeFactor& tool, FakeBkgTools::Weight& wgt, double nom);

////////////////////////////////////////////////////////////////////////////////
class TreeHelperBase 
{
  public:
    TreeHelperBase() {}
    TreeHelperBase(TTree* tt) {_ttree = tt;}
    virtual ~TreeHelperBase() {}
    
    TTree& tr() { return *_ttree; }

    virtual void initialize() = 0;
    virtual int event_number() = 0;
    virtual bool isMC() = 0;
    
    vector<leptonProperties> leptons() {
        if (_leptons.size() == 0) build_leptons();
        return _leptons;
    }

    virtual void clear() {
        _leptons.clear();
    }

  protected:
    vector<leptonProperties> _leptons;
    virtual void build_leptons() = 0;

  private:
    TTree* _ttree;
};
