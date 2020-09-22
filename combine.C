
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRoot.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_tree_to_hist.hh"
using namespace mRoot;

TH1D* combine_hists(TH1D* h1, TH1D* h2){
  TList *list = new TList;
  vector<double> values;
  for(int i = 1; i <= h1->GetXaxis()->GetNbins(); i++)
    values.push_back( h1->GetBinContent( i ) );
  for(int i = 1; i <= h2->GetXaxis()->GetNbins(); i++)
    values.push_back( h2->GetBinContent( i ) );

  TH1D * h = new TH1D( h1->GetName(), h1->GetTitle(), values.size(), h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax() + h2->GetXaxis()->GetXmax() );
  for(int i = 0; i < values.size(); i++){
    h->SetBinContent( 1+i, values.at(i) );
  }

  return h;
}

void combine(std::string file_1, std::string file_2, std::string f_out){

  TFile *f1 = TFile::Open( file_1.c_str() );
  auto hists_f1 = get_all_from_folder(f1, ".+");

  TFile *f2 = TFile::Open( file_2.c_str() );
  auto hists_f2 = get_all_from_folder(f2, ".+");

  TFile *fo = TFile::Open( f_out.c_str(), "RECREATE" );
  for(int i = 0; i < hists_f1->size(); i++){
    TH1D* h1 = (TH1D*)hists_f1->at(i);
    // h1->Print();
    bool has_clone = false;
    for(int j = 0; j < hists_f2->size(); j++){
      TH1D* h2 = (TH1D*)hists_f2->at(j);
      if( std::string( h1->GetName() ) != std::string( h2->GetName() ) ) continue;
      auto h3 = combine_hists( h1, h2 );
      // h3->Print();
      has_clone = true;
    }
    if(not has_clone) h1->Print();
  } 

  fo->Write();
}
