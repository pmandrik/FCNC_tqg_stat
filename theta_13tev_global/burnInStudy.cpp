
#include "getTable.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"

using namespace std;
using namespace mRoot;

/*
  0%, 5%, 10%, 15%, 20%, 25% ...
*/
vector <double> burn_fracs = {0.0, 0.05, 0.10, 0.15, 0.20, 0.25};

int burn_hist(TFile * file, string tname, string par_name, string output_name_prefix){
  TTree *tree = (TTree*)(file->Get( tname.c_str() ));
  cout << "process " << tname << " ... " << endl;
  if(not tree) return 0;

  Int_t weight;
  tree->SetBranchAddress("weight", &weight);
  vector <MyParameter*> parameters;
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

    if(not counter){
      hist->Draw("HIST");
      mRoot::tune_hist(hist);
    }
    else{
      hist->Draw("same HIST");
      hist->Rebin( hist->GetXaxis()->GetNbins()/parameters.at(0)->hist->GetXaxis()->GetNbins());
    }

    string x_title = hist->GetTitle();
    if( x_title == "sigma_t_ch" ) x_title = "#sigma_{t-ch} / #sigma_{t_ch}^{SM}";

    hist->Scale(1./norm, "width");
    hist->SetLineWidth(2);
    hist->SetFillStyle(3013);
    hist->SetLineColor(1700+counter);
    hist->SetFillColor(1700+counter);
    hist->SetStats(false);
    hist->SetTitle("");
    hist->GetXaxis()->SetTitle( x_title.c_str() );

    std::string entry_string = to_string(burn_fracs.at(i)).substr(0,4);
    entry_string += "     -#sigma,mean,+#sigma = " + to_string(l).substr(0,5) + " " + to_string(c).substr(0,5) + " " + to_string(u).substr(0,5);
  
    leg->AddEntry(hist, entry_string.c_str(), "f");
    counter++;
  }

  leg->Draw();
  canv_hists->Print( (output_name_prefix + "_" + tname + ".png" ).c_str() );
  canv_hists->Print( (output_name_prefix + "_" + tname + ".pdf" ).c_str() );
  return true;
}


void burnInStudy(string filename, string par_name, string output_name_prefix){
  cout << "burnInStudy(" + filename << "," + par_name + "," + output_name_prefix << ") ..." << endl;
  TFile file( filename.c_str() );
  file.ls();

  for(int counter = 0; counter < burn_fracs.size(); counter++)
    new TColor(1700+counter, 1. - counter / 10., 0. + counter / 10., 0.5);

  int index = 1;
  while(true){
    int answer = burn_hist(& file, "chain_" + to_string(index), par_name, output_name_prefix);
    if(not answer) break;
    index++;
  }

  cout << "burnInStudy(): done ..." << endl;
}




