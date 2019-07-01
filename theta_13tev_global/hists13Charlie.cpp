
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

vector<TFile*> * get_files(string prefix, vector<string> * names){
  vector<TFile*> * files = new vector<TFile*>();
  for(auto name : *names){
    TFile * file = TFile::Open( (prefix+name).c_str());
    if(!file or file->IsZombie()){
      cerr << __FUNCTION__ << ": can't open file name " << prefix+name << " skip ... " << endl;
      continue;
    }
    files->push_back( file );
  }
  return files;
}

vector <TTreeReader*> * get_readers(vector<TFile*> * flist, TString tree_name){
  vector <TTreeReader*> * readers = new vector <TTreeReader*>();
  for(auto file : *flist){
    TTreeReader * reader = new TTreeReader(tree_name, file);
    if(!reader->GetTree()){
      cerr << __FUNCTION__ << ": can't get ttree " << tree_name << " in file " << file->GetName() << " skip ... " << endl;
      continue;
    }
    readers->push_back( reader );
  }
  return readers;
}

void fill_hist(vector <TTreeReader*> * readers, TString var, TH1D * hist){
  for(auto reader : *readers){
    TTreeReaderValue<float> val(*reader, var);
    while(reader->Next()) hist->Fill( *val );
  }
}

void def_full_chain(string prefix, vector<string> * names, TString tree_name, TString var, TString hname, int NBINS, TFile * out_file,
double rmin = 0, double rmax = 1., string weight_and_cut="1", double def_weight = 1.){
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

    while(reader->Next())
      hist->Fill( *val, def_weight * weight.EvalInstance() );
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

  string PREFIX_NTUPLES;
  if(RELEASE=="march_15_17") PREFIX_NTUPLES = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-03-13_puppi/tuples_merged/";
  if(RELEASE=="april_24_17") PREFIX_NTUPLES = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-17_temp/tuples_merged/";
  if(RELEASE=="april_rePt")  PREFIX_NTUPLES = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-04-17_temp/tuples_merged2/";

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
  double MC_DEF_WEIGHT = 35867.060 * 0.989712022735707; // it is lumi
  TString tree_name = "Vars";
  double rmin = 0; 
  double rmax = 1;

  if(MODE == "QCD"){
    TFile * out_file = new TFile(OUTPUT_FILE_NAME, "RECREATE");
    TString var       = "BNN_qcd";//"BNN_qcd_mc";

    if(RELEASE=="march_15_17"){
      def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",      NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",       NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_OTHER,     tree_name, var,   "other",     NBINS, out_file, rmin, rmax, "weight", MC_DEF_WEIGHT);
    }
    if(RELEASE=="april_24_17"){
      def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",      NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",       NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_OTHER,     tree_name, var,   "other",     NBINS, out_file, rmin, rmax, "weight/weight_pttop", MC_DEF_WEIGHT);
    }
    if(RELEASE=="april_rePt"){
      def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",      NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",       NBINS, out_file, rmin, rmax, "1",      1.);
      def_full_chain(PREFIX_NTUPLES,  &FILES_OTHER,     tree_name, var,   "other",     NBINS, out_file, rmin, rmax, "weight_norm", 1.);
    }
    out_file->Close();
  }
  else if(MODE == "SM"){
    TFile * out_file = new TFile(OUTPUT_FILE_NAME, "RECREATE");

    TString var;
    string data_selection, mc_selection;
    if(RELEASE=="march_15_17"){
      data_selection = "(BNN_qcd > 0.7) * 1.";
      mc_selection   = "(BNN_qcd > 0.7) * weight";
      var    = "BNN_sm7";
    }
    if(RELEASE=="april_24_17"){
      data_selection = "(BNN_qcd > 0.7) * 1.";
      mc_selection   = "(BNN_qcd > 0.7) * weight / weight_pttop";
      var    = "BNN_SM_new";
    }
    if(RELEASE=="april_rePt"){
      data_selection = "(BNN_qcd > 0.7) * 1.";
      mc_selection   = "(BNN_qcd > 0.7) * weight_norm";
      var    = "BNN_SM_new";
      MC_DEF_WEIGHT = 1;
    }

    //string data_selection = "1.";
    //string mc_selection   = "weight";

    def_full_chain(PREFIX_NTUPLES,  &FILES_DATA,      tree_name, var,   "data",     NBINS, out_file, rmin, rmax, data_selection, 1.);
    def_full_chain(PREFIX_NTUPLES,  &FILES_QCD_DATA,  tree_name, var,   "QCD",      NBINS, out_file, rmin, rmax, data_selection, QCD_norm);
    def_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY",       NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar",    NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson",  NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    //def_full_chain(PREFIX_NTUPLES,    &FILES_WJETS,     tree_name, var,   "Wjets",    NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ",      NBINS, out_file, rmin, rmax, mc_selection+"* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc",       NBINS, out_file, rmin, rmax, mc_selection+"* (Wjets_type ==4)",   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb",       NBINS, out_file, rmin, rmax, mc_selection+"* (Wjets_type ==3)",   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother",   NBINS, out_file, rmin, rmax, mc_selection+"* ((Wjets_type ==5)+(Wjets_type ==6))",   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch",     NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch",     NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
    def_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch",    NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);

    //vector<string> up_down_systematics = { "weight_PileUpUp_norm", "weight_PileUpDown_norm" };
    vector<string> up_down_systematics = { "PileUp" };
    for(auto systematic : up_down_systematics){
      string mc_selection_up   = "weight_" + systematic + "Down_norm";
      string mc_selection_down = "weight_" + systematic + "Up_norm";

      sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* ((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+ "* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* (Wjets_type ==4)",                    mc_selection_down+"* (Wjets_type ==4)",                       MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* (Wjets_type ==3)",                    mc_selection_down+"* (Wjets_type ==3)",                       MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* ((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"* ((Wjets_type ==5)+(Wjets_type ==6))",    MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
    }

    // weight_btag_up_jes_norm 
    // jes, lf, hf, hfstats1, hfstats2, lfstats1, lfstats2, cferr1, cferr2
    vector<string> systematics_up_down = { "jes", "lf", "hf", "hfstats1", "hfstats2", "lfstats1", "lfstats2", "cferr1", "cferr2" };
    for(auto systematic : systematics_up_down){
      string mc_selection_up   = "weight_btag_up_" + systematic +"_norm";
      string mc_selection_down = "weight_btag_down_" + systematic +"_norm";

      sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* ((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+ "* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* (Wjets_type ==4)",                    mc_selection_down+"* (Wjets_type ==4)",                       MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* (Wjets_type ==3)",                    mc_selection_down+"* (Wjets_type ==3)",                       MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, 
        mc_selection_up+"* ((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"* ((Wjets_type ==5)+(Wjets_type ==6))",    MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
      sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
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
      string mc_selection   = "(BNN_qcd > 0.7) * weight";

      if(RELEASE=="march_15_17"){
        data_selection = "(BNN_qcd > 0.7) * 1.";
        mc_selection   = "(BNN_qcd > 0.7) * weight";
      }
      if(RELEASE=="april_rePt"){
        data_selection = "(BNN_qcd > 0.7) * 1.";
        mc_selection   = "(BNN_qcd > 0.7) * weight_norm";
        MC_DEF_WEIGHT = 1;
      }

      def_full_chain(PREFIX_NTUPLES, &FILES_DATA,      tree_name, var,   "data",     NBINS, out_file, rmin, rmax, data_selection, 1.);
      def_full_chain(PREFIX_NTUPLES, &FILES_QCD_DATA,  tree_name, var,   "QCD",      NBINS, out_file, rmin, rmax, data_selection, QCD_norm);
      def_full_chain(PREFIX_NTUPLES, &FILES_DY,        tree_name, var,   "DY",       NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_TT,        tree_name, var,   "ttbar",    NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_DIBOSON,   tree_name, var,   "Diboson",  NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_WJETS,     tree_name, var,   "WQQ",      NBINS, out_file, rmin, rmax, mc_selection+"* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_WJETS,     tree_name, var,   "Wc",       NBINS, out_file, rmin, rmax, mc_selection+"* (Wjets_type ==4)",   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_WJETS,     tree_name, var,   "Wb",       NBINS, out_file, rmin, rmax, mc_selection+"* (Wjets_type ==3)",   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_WJETS,     tree_name, var,   "Wother",   NBINS, out_file, rmin, rmax, mc_selection+"* ((Wjets_type ==5)+(Wjets_type ==6))",   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_SCHANAL,   tree_name, var,   "s_ch",     NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_TCHANAL,   tree_name, var,   "t_ch",     NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_TW,        tree_name, var,   "tW_ch",    NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);

      def_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TUG,  tree_name, var, "fcnc_tug",  NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);
      def_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TCG,  tree_name, var, "fcnc_tcg",  NBINS, out_file, rmin, rmax, mc_selection,   MC_DEF_WEIGHT);

      //vector<string> up_down_systematics = { "weight_PileUpUp_norm", "weight_PileUpDown_norm" };
          vector<string> up_down_systematics = { "PileUp" };
          for(auto systematic : up_down_systematics){
            string mc_selection_up   = "weight_" + systematic + "Down_norm";
            string mc_selection_down = "weight_" + systematic + "Up_norm";

            sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* ((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+ "* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* (Wjets_type ==4)",                    mc_selection_down+"* (Wjets_type ==4)",                       MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* (Wjets_type ==3)",                    mc_selection_down+"* (Wjets_type ==3)",                       MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* ((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"* ((Wjets_type ==5)+(Wjets_type ==6))",    MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);

            sys_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TUG,  tree_name, var, "fcnc_tug_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TCG,  tree_name, var, "fcnc_tcg_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
          }

          // weight_btag_up_jes_norm 
          // jes, lf, hf, hfstats1, hfstats2, lfstats1, lfstats2, cferr1, cferr2
          vector<string> systematics_up_down = { "jes", "lf", "hf", "hfstats1", "hfstats2", "lfstats1", "lfstats2", "cferr1", "cferr2" };
          for(auto systematic : systematics_up_down){
            string mc_selection_up   = "weight_btag_up_" + systematic +"_norm";
            string mc_selection_down = "weight_btag_down_" + systematic +"_norm";

            sys_full_chain(PREFIX_NTUPLES,  &FILES_DY,        tree_name, var,   "DY_"+systematic,       NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TT,        tree_name, var,   "ttbar_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_DIBOSON,   tree_name, var,   "Diboson_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "WQQ_"+systematic,      NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* ((Wjets_type ==2)+(Wjets_type ==1))", mc_selection_down+ "* ((Wjets_type ==2)+(Wjets_type ==1))",   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wc_"+systematic,       NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* (Wjets_type ==4)",                    mc_selection_down+"* (Wjets_type ==4)",                       MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wb_"+systematic,       NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* (Wjets_type ==3)",                    mc_selection_down+"* (Wjets_type ==3)",                       MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_WJETS,     tree_name, var,   "Wother_"+systematic,   NBINS, out_file, rmin, rmax, 
              mc_selection_up+"* ((Wjets_type ==5)+(Wjets_type ==6))", mc_selection_down+"* ((Wjets_type ==5)+(Wjets_type ==6))",    MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_SCHANAL,   tree_name, var,   "s_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TCHANAL,   tree_name, var,   "t_ch_"+systematic,     NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES,  &FILES_TW,        tree_name, var,   "tW_ch_"+systematic,    NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);

            sys_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TUG,  tree_name, var, "fcnc_tug_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
            sys_full_chain(PREFIX_NTUPLES, &FILES_FCNC_TCG,  tree_name, var, "fcnc_tcg_"+systematic,  NBINS, out_file, rmin, rmax, mc_selection_up, mc_selection_down,   MC_DEF_WEIGHT);
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







