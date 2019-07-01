
#include "getTable.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"

using namespace std;
using namespace mRoot;

/*
  0%, 5%, 10%, 15%, 20%, 25% ...
*/

double burnInStudy(string filename, string par_name, string postfix){
  TFile file( filename.c_str() );

  file.ls();

  TTree *tree = dynamic_cast<TTree*>(file.Get("chain_1"));
  if(not tree){
    msg("Cant get \"chain_1\" from file ");
    return 1;
  }

  Int_t weight;
  tree->SetBranchAddress("weight", &weight);
  vector <MyParameter*> parameters;
  vector <double> burn_fracs = {0.0, 0.05, 0.10, 0.15, 0.20, 0.25};
  TObjArray * mycopy = tree->GetListOfBranches();
  TIter iObj(mycopy);
  while (TObject* obj = iObj()) {
    if(obj->GetName() == par_name){
      for(auto frac : burn_fracs){
        MyParameter * parameter = new MyParameter(tree, obj->GetName(), frac);
        for(int l = 0; l < (int)tree->GetEntriesFast(); ++l){
          tree->GetEntry(l);
          parameter->ReadEntrie( weight );
        }
        parameters.push_back( parameter );
      }
      break;
    }
  }

  cout <<  par_name << endl;
  TCanvas * canv_hists = new TCanvas("Burn", "Burn", 800, 600);
  int counter = 0;
  TLegend * leg  = new TLegend(0.52,0.70,0.99,0.92);
  leg->SetHeader("Burn-in fraction:");

  double norm = parameters.at(0)->hist->Integral();
  for(int i = 0; i < parameters.size(); i++){
    auto param = parameters.at(i);
    TH1D * hist = param->hist;

    double alpha = 0.3173;
    double l = get_qv(hist, alpha*0.5 );
    double c = get_qv(hist, 0.5) ;
    double u = get_qv(hist,  1. - alpha*0.5 );

    cout << hist->GetName() << " ---- " << hist->GetMean() << " - " << l << " " << c << " " << u << endl;

    new TColor(1700+counter, 1. - counter / 10., 0. + counter / 10., 0.5);
    if(not counter){
      hist->Draw("HIST");
      mRoot::tune_hist(hist);
    }
    else{
      hist->Draw("same HIST");
      hist->Rebin( hist->GetXaxis()->GetNbins()/parameters.at(0)->hist->GetXaxis()->GetNbins());
    }

    hist->Scale(1./norm, "width");
    hist->SetLineWidth(2);
    hist->SetFillStyle(3013);
    hist->SetLineColor(1700+counter);
    hist->SetFillColor(1700+counter);
    hist->SetStats(false);

    leg->AddEntry(hist, (to_string(burn_fracs.at(i))).c_str(), "f");
    counter++;
  }

  leg->Draw();
  canv_hists->Print( ("burnInStudy_" + postfix + ".png").c_str() );
  cout << "done!" << endl;
  return 0;
}




