
#include <regex>
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
using namespace mRoot;

#include "histsChecker.cpp"

void histsCompUnmarg(TString inputMain, TString inputUnmarg, TString postfix){
  gErrorIgnoreLevel = kWarning;
  TFile * file_main   = TFile::Open(inputMain);
  TFile * file_unmarg = TFile::Open(inputUnmarg);

  auto hists_central = get_all_from_folder(file_main, ".+", "(.+Down)|(.+Up)");
  vector <int> indexes = getNiceColors(35);

  for(auto hist : *hists_central){
    TH1* h = (TH1*)hist;

    cout << " " << h->GetName() << " ... " << endl;
    string name = h->GetName();

    auto shifts = get_all_from_folder(file_unmarg, name + "_.+");
    if(shifts->size() == 0) { msg("skip ... "); continue; }

    TCanvas * canv = new TCanvas(hist->GetName(), hist->GetName(), 640, 640);

    int color_i = 0;
    h->SetLineColor( 1 );
    h->SetLineWidth( 3 );
    double max = h->GetMaximum();

    h->Draw("hist");
    for(auto shift : *shifts){
      TH1* hs = (TH1*)shift;
      check_template(hs);
      hs->Draw("hist same");
      hs->SetLineColor( indexes.at(color_i) );
      hs->SetLineWidth( 2 );
      max = TMath::Max(max, hs->GetMaximum());
      color_i++;
    }
    h->GetYaxis()->SetRangeUser(0, max+max*0.1);
    h->Draw("hist same");

    canv->Print(postfix + TString(hist->GetTitle()) + TString(".png"));
  }
}


