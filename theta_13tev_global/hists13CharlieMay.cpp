
// This script creates 1D histograms used as input for THETA

#include <TFile.h>
#include <TTree.h>
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TTreeFormula.h"
#include <TH1.h>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iomanip>

using namespace std;

template <typename T, typename... Args>
vector <string> vsum(vector<T> v1, vector<T> v2){
  v1.insert(v1.begin(), v2.begin(), v2.end());
  return v1;
}

template <typename T, typename... Args>
vector <string> vsum(vector<T> v1, vector<T> v2, Args... args){
  v1.insert(v1.begin(), v2.begin(), v2.end());
  return vsum(v1, args...);
}

void def_full_chain(string prefix, vector<string> * names, TString tree_name, TString var, TString hname, int NBINS, TFile * out_file,
double rmin = 0, double rmax = 1., string weight_and_cut="1", double def_weight = 1.){
  //cout << "process " << hname << " with weight " <<  weight_and_cut << "x" << def_weight << endl;
  out_file->cd();
  TH1D * hist = new TH1D(hname, hname, NBINS, rmin, rmax);
  hist->Sumw2();

  for(auto name : *names){
    TFile * file = TFile::Open( (prefix+name).c_str());
    if(!file or file->IsZombie()){
      cerr << __FUNCTION__ << ": can't open file name " << prefix+name << " skip ... " << endl;
      continue;
    }

    TTreeReader * reader = new TTreeReader(tree_name, file);
    if(!reader->GetTree()){
      cerr << __FUNCTION__ << ": can't get ttree " << tree_name << " in file " << file->GetName() << " skip ... " << endl;
      continue;
    }

    TObject* br = reader->GetTree()->GetListOfBranches()->FindObject(var);
    if(!br){
      cerr << __FUNCTION__ << ": can't get branch " << var << " in file " << file->GetName() << " skip ... " << endl;
      reader->GetTree()->Print();
      continue;
    }

    TTreeReaderValue<float> val(*reader, var);
    TTreeFormula weight(weight_and_cut.c_str(), weight_and_cut.c_str(), reader->GetTree());

    //cout << weight_and_cut << endl;
    while(reader->Next()){
      hist->Fill( *val, def_weight * weight.EvalInstance() );
      // if( weight_and_cut == "weight_PileUpDown_norm" ) cout << *val << " " << def_weight * weight.EvalInstance() << endl;
      // if( weight_and_cut == "weight_PileUpUp_norm" ) cout << *val << " " << def_weight * weight.EvalInstance() << endl;
    }
  }
  out_file->cd();
  hist->Write();
  // hist->Print(); // check if not nan FIXME
}

void sys_full_chain(string prefix, vector<string> * names, TString tree_name, TString var, TString hname, int NBINS, TFile * out_file,
double rmin = 0, double rmax = 1., string weight_and_cut_up="1", string weight_and_cut_down="1", double def_weight = 1.){
  def_full_chain(prefix, names, tree_name, var, hname + TString("Up")  , NBINS, out_file, rmin, rmax, weight_and_cut_up  , def_weight);
  def_full_chain(prefix, names, tree_name, var, hname + TString("Down"), NBINS, out_file, rmin, rmax, weight_and_cut_down, def_weight);
}

// ./main 
int main(int argc, char* argv[]){
  cout << "hists13Charlie.cpp, welcome to the converter from mensura ntuples to hists ..." << endl;
  if (argc < 5) {
      cerr << "hists13Charlie.cpp, not enough parameters, exit" << endl;
      cerr << "   argv[1] - MODE - this is what to to bild - qcd, fcnc, sm etc " << endl;
      cerr << "   argv[2] - RELEASE - this is to apply changes beetwen mensura releases " << endl;
      cerr << "   argv[3] - OUTPUT_FILE_NAME - output file name for hists" << endl;
      cerr << "   argv[4] - NBINS - number of bins in hists" << endl;
      cerr << "   argv[5] - QCD_norm optional qcd normalization factor" << endl;
      return 1;
  }

  TString MODE             =  TString(argv[1]) ; // this is what to to bild - qcd, fcnc, sm etc 
  TString RELEASE          =  TString(argv[2]) ; // this is changes beetwen mensura releases
  TString OUTPUT_FILE_NAME =  TString(argv[3]);
  int NBINS                =  atoi(argv[4]);

  double QCD_norm = -666;
  if(argc > 5) QCD_norm = atof(argv[5]);
  if(QCD_norm < 0 and MODE != "QCD"){
    cerr << "Bad  QCD normalization, please provide correct value, exit ..." << endl;
    return 1;
  }

  cout << "hists13Charlie.cpp, Will use MODE \"" << MODE << "\", RELEASE \"" << RELEASE << "\", with QCD_norm = " << QCD_norm << endl;

  string CENTRAL_FOLDER = "Central";
  vector<string> VARIATION_SYS = {"JEC", "JER", "UnclMET"};

  string PREFIX_NTUPLES;
  string PATH_PREFIX;
  string PATH_EXCLUDE;
  if(RELEASE=="april_Sys"){ 
    PATH_PREFIX  = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/tuples_merged/";
    PATH_EXCLUDE = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-27_new_syst/bnn_SM_new/";
  }
  else{
    cerr << "Unknow RELEASE, please provide correct value, exit ..." << endl;
    return 1;
  }

  //---------- 1. DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DATA = {"Data.root"};

  //---------- 2. QCD DATA -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_QCD_DATA = {"QCD_data.root"};

  //---------- 3. OTHER -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_DY = {"DY.root"};
  vector <string> FILES_TT = {"ttbar.root"};
  vector <string> FILES_DIBOSON = {"WW.root", "WZ.root", "ZZ.root"};
  vector <string> FILES_WJETS = {"Wjets.root"};
  vector <string> FILES_SCHANAL = {"s-channel.root"};
  vector <string> FILES_TCHANAL = {"t-channel.root"};
  vector <string> FILES_TW = {"tW-channel.root"};

  vector <string> FILES_OTHER = vsum(FILES_DY, FILES_TT, FILES_DIBOSON, FILES_WJETS, FILES_SCHANAL, FILES_TCHANAL, FILES_TW);

  //---------- 4. ANOMAL -------------------------------------------------------------------------------------------------------------------------
  vector <string> FILES_FCNC_TUG = {"FCNC_tug.root"};
  vector <string> FILES_FCNC_TCG = {"FCNC_tcg.root"};

  //---------- FILL HISTS -------------------------------------------------------------------------------------------------------------------------
  TString tree_name = "Vars";
  double rmin = 0; 
  double rmax = 1;

  if(MODE == "QCD"){
    TFile * out_file = new TFile(OUTPUT_FILE_NAME, "RECREATE");
    TString var       = "BNN_qcd";

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER +"/";
    def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",      NBINS, out_file, rmin, rmax, "1",      1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",       NBINS, out_file, rmin, rmax, "1",      1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_OTHER,     tree_name, var,   "other",     NBINS, out_file, rmin, rmax, "weight_norm", 1.);

    out_file->Close();
  }
  else if(MODE == "SM"){
    TFile * out_file = new TFile(OUTPUT_FILE_NAME, "RECREATE");

    TString var;
    string data_selection, mc_selection;

    data_selection = "(BNN_qcd > 0.7) * 1.";
    mc_selection   = "(BNN_qcd > 0.7) * weight_norm";
    var            = "BNN_SM_new";

    string mc_selection_WQQ    = mc_selection+"* ((Wjets_type ==2)+(Wjets_type ==1))";
    string mc_selection_Wc     = mc_selection+"* (Wjets_type ==4)";
    string mc_selection_Wb     = mc_selection+"* (Wjets_type ==3)";
    string mc_selection_Wother = mc_selection+"* ((Wjets_type ==5)+(Wjets_type ==6))";

    PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER+"/";
    def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",     NBINS, out_file, rmin, rmax, data_selection,      1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",      NBINS, out_file, rmin, rmax, data_selection,      QCD_norm);
    def_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY",       NBINS, out_file, rmin, rmax, mc_selection,        1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar",    NBINS, out_file, rmin, rmax, mc_selection,        1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson",  NBINS, out_file, rmin, rmax, mc_selection,        1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ",      NBINS, out_file, rmin, rmax, mc_selection_WQQ,    1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc",       NBINS, out_file, rmin, rmax, mc_selection_Wc,     1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb",       NBINS, out_file, rmin, rmax, mc_selection_Wb,     1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother",   NBINS, out_file, rmin, rmax, mc_selection_Wother, 1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch",     NBINS, out_file, rmin, rmax, mc_selection,        1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch",     NBINS, out_file, rmin, rmax, mc_selection,        1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch",    NBINS, out_file, rmin, rmax, mc_selection,        1.);

    // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
    vector<string> up_down_systematics = { "PileUp", "btag_jes", "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" };
    for(auto systematic : up_down_systematics){
      string mc_selection_up   = "(BNN_qcd > 0.7) * weight_" + systematic + "Up_norm";
      string mc_selection_down = "(BNN_qcd > 0.7) * weight_" + systematic + "Down_norm";

      sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, mc_selection_up+"*((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+"*((Wjets_type ==2)+(Wjets_type ==1))",   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up+"*(Wjets_type ==4)",                    mc_selection_down+"*(Wjets_type ==4)",                      1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up+"*(Wjets_type ==3)",                    mc_selection_down+"*(Wjets_type ==3)",                      1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, mc_selection_up+"*((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"*((Wjets_type ==5)+(Wjets_type ==6))",   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
    }

    // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT FILDERS
    for(auto fprefix : VARIATION_SYS){
      vector<string> pstfixs = {"Down", "Up"};
      for(auto pstfix : pstfixs){
        string sname = fprefix + pstfix;
        PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
        def_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+sname,       NBINS, out_file, rmin, rmax, mc_selection,        1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+sname,    NBINS, out_file, rmin, rmax, mc_selection,        1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+sname,  NBINS, out_file, rmin, rmax, mc_selection,        1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+sname,      NBINS, out_file, rmin, rmax, mc_selection_WQQ,    1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+sname,       NBINS, out_file, rmin, rmax, mc_selection_Wc,     1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+sname,       NBINS, out_file, rmin, rmax, mc_selection_Wb,     1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+sname,   NBINS, out_file, rmin, rmax, mc_selection_Wother, 1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+sname,     NBINS, out_file, rmin, rmax, mc_selection,        1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+sname,     NBINS, out_file, rmin, rmax, mc_selection,        1.);
        def_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+sname,    NBINS, out_file, rmin, rmax, mc_selection,        1.);
      }
    }

    out_file->Close();
  }
  else if(MODE == "FCNC"){
    vector <string> caplings = {"tcg", "tug"};
    for(auto capling : caplings){
      string out_name = argv[3];
      out_name = out_name.substr(0, out_name.find(".root") );
      TFile * out_file = new TFile( (out_name + "_" + capling + ".root").c_str(), "RECREATE");

      TString var       = "BNN_" + capling; // bnn_tug4//"BNN_qcd_mc";

      string data_selection = "(BNN_qcd > 0.7) * 1.";
      string mc_selection   = "(BNN_qcd > 0.7) * weight_norm";

      string mc_selection_WQQ    = mc_selection+"* ((Wjets_type ==2)+(Wjets_type ==1))";
      string mc_selection_Wc     = mc_selection+"* (Wjets_type ==4)";
      string mc_selection_Wb     = mc_selection+"* (Wjets_type ==3)";
      string mc_selection_Wother = mc_selection+"* ((Wjets_type ==5)+(Wjets_type ==6))";

      PREFIX_NTUPLES = PATH_PREFIX + CENTRAL_FOLDER+"/";
      def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",     NBINS, out_file, rmin, rmax, data_selection,      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",      NBINS, out_file, rmin, rmax, data_selection,      QCD_norm);
      def_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY",       NBINS, out_file, rmin, rmax, mc_selection,        1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar",    NBINS, out_file, rmin, rmax, mc_selection,        1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson",  NBINS, out_file, rmin, rmax, mc_selection,        1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ",      NBINS, out_file, rmin, rmax, mc_selection_WQQ,    1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc",       NBINS, out_file, rmin, rmax, mc_selection_Wc,     1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb",       NBINS, out_file, rmin, rmax, mc_selection_Wb,     1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother",   NBINS, out_file, rmin, rmax, mc_selection_Wother, 1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch",     NBINS, out_file, rmin, rmax, mc_selection,        1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch",     NBINS, out_file, rmin, rmax, mc_selection,        1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch",    NBINS, out_file, rmin, rmax, mc_selection,        1.);

      def_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TUG,  tree_name, var, "fcnc_tug",  NBINS, out_file, rmin, rmax, mc_selection,   1.);
      def_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TCG,  tree_name, var, "fcnc_tcg",  NBINS, out_file, rmin, rmax, mc_selection,   1.);

      // FILL SYSTEMATIC WITCH PRESENT IN THE CENTRAL SAME FILES
      vector<string> up_down_systematics = { "PileUp", "btag_jes", "btag_lf", "btag_hf", "btag_hfstats1", "btag_hfstats2", "btag_lfstats1", "btag_lfstats2", "btag_cferr1", "btag_cferr2" };
      for(auto systematic : up_down_systematics){
        string mc_selection_up   = "(BNN_qcd > 0.7) * weight_" + systematic + "Up_norm";
        string mc_selection_down = "(BNN_qcd > 0.7) * weight_" + systematic + "Down_norm";

        sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, mc_selection_up+"*((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+"*((Wjets_type ==2)+(Wjets_type ==1))",   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up+"*(Wjets_type ==4)",                    mc_selection_down+"*(Wjets_type ==4)",                      1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up+"*(Wjets_type ==3)",                    mc_selection_down+"*(Wjets_type ==3)",                      1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, mc_selection_up+"*((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"*((Wjets_type ==5)+(Wjets_type ==6))",   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);

        sys_full_chain(PREFIX_NTUPLES,  &FILES_FCNC_TUG,  tree_name, var,   "fcnc_tug_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
        sys_full_chain(PREFIX_NTUPLES,  &FILES_FCNC_TCG,  tree_name, var,   "fcnc_tcg_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   1.);
      }

      // FILL SYSTEMATIC WITCH PRESENT IN THE DIFFERENT FILDERS
      for(auto fprefix : VARIATION_SYS){
        vector<string> pstfixs = {"Down", "Up"};
        for(auto pstfix : pstfixs){
          string sname = fprefix + pstfix;
          PREFIX_NTUPLES = PATH_PREFIX + fprefix + pstfix+"/";
          def_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+sname,       NBINS, out_file, rmin, rmax, mc_selection,        1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+sname,    NBINS, out_file, rmin, rmax, mc_selection,        1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+sname,  NBINS, out_file, rmin, rmax, mc_selection,        1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+sname,      NBINS, out_file, rmin, rmax, mc_selection_WQQ,    1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+sname,       NBINS, out_file, rmin, rmax, mc_selection_Wc,     1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+sname,       NBINS, out_file, rmin, rmax, mc_selection_Wb,     1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+sname,   NBINS, out_file, rmin, rmax, mc_selection_Wother, 1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+sname,     NBINS, out_file, rmin, rmax, mc_selection,        1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+sname,     NBINS, out_file, rmin, rmax, mc_selection,        1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+sname,    NBINS, out_file, rmin, rmax, mc_selection,        1.);

          def_full_chain(PREFIX_NTUPLES,  &FILES_FCNC_TUG,  tree_name, var,   "fcnc_tug_"+sname,  NBINS, out_file, rmin, rmax, mc_selection,   1.);
          def_full_chain(PREFIX_NTUPLES,  &FILES_FCNC_TCG,  tree_name, var,   "fcnc_tcg_"+sname,  NBINS, out_file, rmin, rmax, mc_selection,   1.);
        }
      }

      out_file->Close();
    }
  }
  else cout << __FILE__ " : unknow mode " << MODE << endl;

  cout << "hists13Charlie.cpp, done ..." << endl;

  return 0;
}


/*
  /afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples/WJets1.part1.root
  /afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples/WJets2.part1.root
*/







