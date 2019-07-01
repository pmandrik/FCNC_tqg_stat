
// This script creates 1D histograms used as input for THETA

#include <TFile.h>
#include <TTree.h>
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include <TH1.h>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <iomanip>

using namespace std;

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

vector <TTreeReader*> * get_readers(vector<TFile*> * flist, const char * tree_name){
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

void fill_hist(vector <TTreeReader*> * readers, const char * var, TH1D * hist){
  for(auto reader : *readers){
    TTreeReaderValue<float> val(*reader, var);
    while(reader->Next()) hist->Fill( *val );
  }
}

void def_full_chain(string prefix, vector<string> * names, const char * tree_name, const char * var, const char * hname, int nbins, TFile * out_file){
  /*
  auto files  = get_files(prefix, names);
  auto riders = get_readers(files, tree_name);
  out_file->cd();
  TH1D * hist = new TH1D(hname, hname, nbins, 0, 1);
  fill_hist(riders, var, hist);
  out_file->Write();
  */

  out_file->cd();
  TH1D * hist = new TH1D(hname, hname, nbins, 0, 1);

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
      continue;
    }
    TTreeReaderValue<float> val(*reader, var);
    while(reader->Next()) hist->Fill( *val );
  }
  out_file->cd();
  hist->Write();
}

// ./main 
int main(int argc, char* argv[]){
  if (argc < 2) {
      cerr << "__FILE__ : __FUNCTION__ : not enough parameters, exit" << endl;
      return 1;
  }
  //---------- 1. DATA -------------------------------------------------------------------------------------------------------------------------
  string PREFIX_DATA = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples_Data/";
  vector <string> FILES_DATA = {"SingleMuB.part1.root",
                                "SingleMuB.part10.root",
                                "SingleMuB.part11.root",
                                "SingleMuB.part12.root",
                                "SingleMuB.part13.root",
                                "SingleMuB.part14.root",
                                "SingleMuB.part2.root",
                                "SingleMuB.part3.root",
                                "SingleMuB.part4.root",
                                "SingleMuB.part5.root",
                                "SingleMuB.part6.root",
                                "SingleMuB.part7.root",
                                "SingleMuB.part8.root",
                                "SingleMuB.part9.root",
                                "SingleMuC.part1.root",
                                "SingleMuC.part2.root",
                                "SingleMuC.part3.root",
                                "SingleMuC.part4.root",
                                "SingleMuC.part5.root",
                                "SingleMuC.part6.root",
                                "SingleMuC.part7.root",
                                "SingleMuD.part1.root",
                                "SingleMuD.part2.root",
                                "SingleMuD.part3.root",
                                "SingleMuD.part4.root",
                                "SingleMuD.part5.root",
                                "SingleMuD.part6.root",
                                "SingleMuD.part7.root",
                                "SingleMuD.part8.root",
                                "SingleMuD.part9.root",
                                "SingleMuE.part1.root",
                                "SingleMuE.part2.root",
                                "SingleMuE.part3.root",
                                "SingleMuE.part4.root",
                                "SingleMuE.part5.root",
                                "SingleMuE.part6.root",
                                "SingleMuE.part7.root",
                                "SingleMuE.part8.root",
                                "SingleMuE.part9.root",
                                "SingleMuF.part1.root",
                                "SingleMuF.part2.root",
                                "SingleMuF.part3.root",
                                "SingleMuF.part4.root",
                                "SingleMuF.part5.root",
                                "SingleMuF.part6.root",
                                "SingleMuF.part7.root",
                                "SingleMuG.part1.root",
                                "SingleMuG.part10.root",
                                "SingleMuG.part11.root",
                                "SingleMuG.part12.root",
                                "SingleMuG.part13.root",
                                "SingleMuG.part14.root",
                                "SingleMuG.part15.root",
                                "SingleMuG.part2.root",
                                "SingleMuG.part3.root",
                                "SingleMuG.part4.root",
                                "SingleMuG.part5.root",
                                "SingleMuG.part6.root",
                                "SingleMuG.part7.root",
                                "SingleMuG.part8.root",
                                "SingleMuG.part9.root"};

  //---------- 2. QCD DATA -------------------------------------------------------------------------------------------------------------------------
  string PREFIX_QCD_DATA = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples_QCD/";
  vector <string> FILES_QCD_DATA = {"SingleMuB.part1.root",
                                    "SingleMuB.part10.root",
                                    "SingleMuB.part11.root",
                                    "SingleMuB.part12.root",
                                    "SingleMuB.part13.root",
                                    "SingleMuB.part14.root",
                                    "SingleMuB.part2.root",
                                    "SingleMuB.part3.root",
                                    "SingleMuB.part4.root",
                                    "SingleMuB.part5.root",
                                    "SingleMuB.part6.root",
                                    "SingleMuB.part7.root",
                                    "SingleMuB.part8.root",
                                    "SingleMuB.part9.root",
                                    "SingleMuC.part1.root",
                                    "SingleMuC.part2.root",
                                    "SingleMuC.part3.root",
                                    "SingleMuC.part4.root",
                                    "SingleMuC.part5.root",
                                    "SingleMuC.part6.root",
                                    "SingleMuC.part7.root",
                                    "SingleMuD.part1.root",
                                    "SingleMuD.part2.root",
                                    "SingleMuD.part3.root",
                                    "SingleMuD.part4.root",
                                    "SingleMuD.part5.root",
                                    "SingleMuD.part6.root",
                                    "SingleMuD.part7.root",
                                    "SingleMuD.part8.root",
                                    "SingleMuD.part9.root",
                                    "SingleMuE.part1.root",
                                    "SingleMuE.part2.root",
                                    "SingleMuE.part3.root",
                                    "SingleMuE.part4.root",
                                    "SingleMuE.part5.root",
                                    "SingleMuE.part6.root",
                                    "SingleMuE.part7.root",
                                    "SingleMuE.part8.root",
                                    "SingleMuE.part9.root",
                                    "SingleMuF.part1.root",
                                    "SingleMuF.part2.root",
                                    "SingleMuF.part3.root",
                                    "SingleMuF.part4.root",
                                    "SingleMuF.part5.root",
                                    "SingleMuF.part6.root",
                                    "SingleMuF.part7.root",
                                    "SingleMuG.part1.root",
                                    "SingleMuG.part10.root",
                                    "SingleMuG.part11.root",
                                    "SingleMuG.part12.root",
                                    "SingleMuG.part13.root",
                                    "SingleMuG.part14.root",
                                    "SingleMuG.part15.root",
                                    "SingleMuG.part2.root",
                                    "SingleMuG.part3.root",
                                    "SingleMuG.part4.root",
                                    "SingleMuG.part5.root",
                                    "SingleMuG.part6.root",
                                    "SingleMuG.part7.root",
                                    "SingleMuG.part8.root",
                                    "SingleMuG.part9.root"};

  //---------- 3. OTHER -------------------------------------------------------------------------------------------------------------------------
  string PREFIX_OTHER = "/afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples/";
  vector <string> FILES_DY = {"DY10_50_0.root", "DY50_part2.root", "DY50_part3.root", "DY50_part4.root", "DY50_part5.root"};
  vector <string> FILES_TT = {"TTbar.part1.root", "TTbar.part2.root", "TTbar.part3.root", "TTbar.part4.root", "TTbar.part5.root", "TTbar.part6.root", "TTbar.part7.root"};
  vector <string> FILES_DIBOSON = {"WW1.root", "WW2.root", "WZ1.root", "WZ2.root", "ZZ1.root", "ZZ2.root"};
  vector <string> FILES_WJETS = {"W4Jets1.root", "W4Jets2.root", "W4Jets3.part1.root", "W4Jets3.part2.root", "WJets1.part1.root", "WJets2.part1.root", "Wjets_2p_NpR.part1.root", "Wjets_2p_NpR.part2.root", "Wjets_2p_asD.part1.root", "Wjets_2p_asD.part2.root", "Wjets_2p_asD.part3.root", "Wjets_3p_Qvy.part1.root", "Wjets_3p_Qvy.part2.root", "Wjets_3p_Qvy.part3.root", "Wjets_3p_Qvy.part4.root", "Wjets_3p_uZq.part1.root", "Wjets_3p_uZq.part2.root"};
  vector <string> FILES_SCHANAL = {"s-channel.root"};
  vector <string> FILES_TCHANAL = {"t-chan_t_KiO.part1.root", "t-chan_t_KiO.part2.root", "t-chan_t_KiO.part3.root", "t-chan_tbar_piQ.part1.root", "t-chan_tbar_piQ.part2.root"};
  vector <string> FILES_TW = {"tW_antitop1_Unm.root", "tW_antitop2_AzG.root", "tW_top1_NjE.root", "tW_top2_Fpi.root"};


  //---------- FILL HISTS -------------------------------------------------------------------------------------------------------------------------
  
  //void def_full_chain(string prefix, vector<string> * names, const char * tree_name, const char * var, const char * hname, int nbins, TFile * out_file)
  TFile * out_file = new TFile("hists13.root", "RECREATE");
  const char * tree_name = "Vars";
  const char * var       = "BNN_qcd_mc";
  int nbins              = atoi(argv[1]);
  def_full_chain(PREFIX_DATA,     &FILES_DATA,      tree_name, var,   "data",      nbins, out_file);
  def_full_chain(PREFIX_QCD_DATA, &FILES_QCD_DATA,  tree_name, var,   "QCD",       nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_DY,        tree_name, var,   "Drell_Yan", nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_TT,        tree_name, var,   "ttbar",     nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_DIBOSON,   tree_name, var,   "Diboson",   nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_WJETS,     tree_name, var,   "Wjets",     nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_SCHANAL,   tree_name, var,   "s_ch",      nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_TCHANAL,   tree_name, var,   "t_ch",      nbins, out_file);
  def_full_chain(PREFIX_OTHER,    &FILES_TW,        tree_name, var,   "tW_ch",     nbins, out_file);
  out_file->Close();
  return 0;
}


/*
  /afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples/WJets1.part1.root
  /afs/cern.ch/work/g/gvorotni/public/samples/13TeV/17-02-28_rereco_v4/tuples/WJets2.part1.root
*/







