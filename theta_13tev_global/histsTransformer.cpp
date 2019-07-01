
#include <regex>

vector <TObject*> * get_all_from_folder(TDirectory * dir, string include_rexp=".+", string exclude_rexp=""){
  vector <TObject*> * objs = new vector <TObject*>;

  regex reg_incl( include_rexp );
  regex reg_excl( exclude_rexp );

  auto keys = dir->GetListOfKeys();

  for(int i = 0; i < keys->GetSize(); i++){
    auto obj =  ((TKey*) keys->At(i))->ReadObj();
    string name = obj->GetName();

    if( !regex_match(name, reg_incl) ) continue;
    if( regex_match(name, reg_excl) ) continue;

    objs->push_back( obj );
  }

  return objs;
}

void histsTransformer(TString inputFileName, TString outFileName){
  TFile * file = TFile::Open(inputFileName);
  auto hists_central = get_all_from_folder(file);

  TFile * out_file = new TFile(outFileName, "RECREATE");
  for(auto obj : *hists_central){
    TH1* hist = (TH1*)obj;

    int nbins =     hist->GetXaxis()->GetNbins();
    for(int nbins = 1; nbins <= hist->GetXaxis()->GetNbins(); nbins++) {
      // cout << hist->GetBinError(nbins) << " " << endl;
      hist->SetBinContent(nbins, max(1000. * hist->GetBinContent(nbins), 0.));
      hist->SetBinError(nbins, 0.1 * hist->GetBinError(nbins));
    }
    hist->Write();
  }
}




