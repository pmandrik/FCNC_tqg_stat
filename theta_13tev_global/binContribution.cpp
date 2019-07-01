
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
using namespace mRoot;


void binContribution(string inputFileName){
  TFile * file = TFile::Open(inputFileName.c_str());
  auto hists = get_all_from_folder(file, ".+");

  string basename = get_name_without_extension_and_path( inputFileName );

  // remover one bin from hists
  auto hist = (TH1*)hists->at(0);
  int nbins = hist->GetXaxis()->GetNbins();

  for(int i = 1; 2*i <= nbins; i+=1){
    TFile file_out( (basename + "_" + to_string(i) + ".root").c_str(), "RECREATE");
    for(auto iter : *hists){
      TH1D* h = (TH1D*)iter;
      TH1D* hb = copy_tree_without_bins( h, { 2*i-1, 2*i } );
    }
    file_out.Write();
  }
}
