
#include <regex>
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRootNiceColors.cpp"
using namespace mRoot;

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

void check_template(TH1 * hist){
  int nbins = hist->GetXaxis()->GetNbins();
  bool hist_is_ok = true;
  for(int nbins = 1; nbins <= hist->GetXaxis()->GetNbins(); nbins++) {
    // cout << hist->GetBinError(nbins) << " " << endl;
    float value = hist->GetBinContent(nbins);

    if( string(hist->GetName()) == string("Wjets"))
      msg("Wjets", nbins, value);
    if( string(hist->GetName()) == string("Wjets_JESUp"))
      msg("Wjets_JESUp", nbins, value);
    if( string(hist->GetName()) == string("Wjets_JESDown"))
      msg("Wjets_JESUp", nbins, value);

    if(value <= 0){
      msg("WARNING !!! ", hist->GetName(), "bin", nbins, value);
      hist_is_ok = false;
    }
  }
  if(hist_is_ok) msg("hist is ok ", hist->GetName());
}

void histsChecker(TString inputFileName, TString postfix){
  // TString inp_file_name = "hists13Charlie_SM.root";
  // TString inp_file_name = "hist_copy.root";
  // TString inp_file_name = "hists13Charlie_SM_fix.root";
  gErrorIgnoreLevel = kWarning;
  TFile * file = TFile::Open(inputFileName);

  // draw all hists
  // for every draw central and shifted hists
  auto hists_central = get_all_from_folder(file, ".+", "(.+Down$)|(.+Up$)|(.+_alt)|(.+_weight_.+)|(.+_scale_.+)");
  vector <int> indexes = getNiceColors(50);

  for(auto hist : *hists_central){
    TCanvas * canv = new TCanvas(hist->GetName(), hist->GetName(), 640 * 3, 640);
    canv->Divide(3, 1);
  
    TH1* h = (TH1*)hist;

    cout << " " << h->GetName() << " ... " << endl;
    check_template(h);

    double max = h->GetMaximum();
    auto shifts = get_all_from_folder(file, string(hist->GetName()) + "_.+", "("+string(hist->GetName()) + "_alt.*)|(.+_weight_.+)");
    int color_i = 0;
    h->SetLineColor( 1 );
    h->SetLineWidth( 3 );

    TLegend * legend = new TLegend(0.05,0.05,0.95,0.95);
    legend->AddEntry(h,  h->GetName(), "l");
    
    canv->cd(1);
    h->Draw("hist");

    list<TH1D*> herrs;
    string lname = "";
    for(auto shift : *shifts){
      TH1* hs = (TH1*)shift;
      check_template(hs);
      hs->Draw("hist same");
      if(max < hs->GetMaximum()) max = hs->GetMaximum();
      hs->SetLineColor( indexes.at(color_i) );
      hs->SetLineWidth( 2 );

      lname.size() < string(hs->GetTitle()).size() ? lname = string(hs->GetTitle()) : lname;

      TH1D* hist_err = (TH1D*)hs->Clone();
      hist_err->Add( h, -1. );
      hist_err->Divide( h );
      hist_err->SetMarkerStyle( 30 + color_i );
      hist_err->SetFillStyle( 3000+color_i );
      hist_err->SetFillColor( indexes.at(color_i) );
      color_i += 1;

      for(int i = 1; i < hist_err->GetNbinsX()+1; i++){
        hist_err->SetBinContent(i, abs(hist_err->GetBinContent(i)));
      }

      for(auto it = herrs.begin(); it!=herrs.end(); ++it){
        if( (*it)->Integral() < hist_err->Integral() ){
          herrs.insert(it, hist_err);
          goto skipp_add;
        }
      }
      herrs.push_back(hist_err);
      skipp_add: ;
    }


    h->GetYaxis()->SetRangeUser(0, max+max*0.1);
    h->Draw("hist same");

    THStack * stack = new THStack(TString(h->GetName()) + "_stack", TString(h->GetName()) + "_stack");
    for(auto it = herrs.begin(); it!=herrs.end(); ++it){
      stack->Add( (*it) );
      
      string title = string((*it)->GetTitle());
      for(int i = (lname.size() - title.size())*1.3; i>=0; i--) title += " ";
      title += " - " + to_string( (*it)->Integral()/(*it)->GetNbinsX() );
      legend->AddEntry(*it,  title.c_str(), "lf");
    }

    canv->cd(2);
    legend->Draw();

    canv->cd(3);
    stack->Draw("hist f");

    canv->Print(postfix + TString(hist->GetTitle()) + TString(".png"));

    // Draw relative fluctuatios
/*
    for(auto shift : *shifts){
      TH1* hs = (TH1*)shift;

      string name_canv = string(h->GetName()) + "_" + hs->GetName();
      TCanvas * canv_rel = new TCanvas(name_canv.c_str(), name_canv.c_str(), 640, 640);
      TH1D* hist_err = (TH1D*)hs->Clone();
      hist_err->Add( h, -1. );
      hist_err->Divide( h );
      hist_err->SetLineWidth( 2 );
      hist_err->SetStats(false);
      hist_err->Draw("hist");
      hist_err->SetLineColor(kRed);

      string error_name = string( hs->GetName() ).substr( string(hist->GetTitle()).size()+1, string( hs->GetName() ).size() );
      if( error_name.at( error_name.size()-1 ) == 'n' ) continue;
      error_name = error_name.substr( 0, error_name.size()-2 );
      cout << error_name << endl;

      TH1D* shift_down = nullptr;
      for(auto shift_d : *shifts){
        TH1D* hs_hist = (TH1D*)shift_d;
        string hist_name_full = string( hs_hist->GetName() );
        if( hist_name_full.size() < string(hist->GetTitle()).size() ) continue;
        string channel_name = string( hs_hist->GetName() ).substr(0, string(hist->GetTitle()).size() );
        // cout << channel_name << " " << string(hist->GetTitle()) << endl;
        if(channel_name != string(hist->GetTitle()) ) continue;
        string error_name_down = string( hs_hist->GetName() ).substr( string(hist->GetTitle()).size()+1, string( hs_hist->GetName() ).size() );
        if( error_name_down.at( error_name_down.size()-1 ) != 'n' ) continue;
        error_name_down = error_name_down.substr( 0, error_name_down.size()-4 );
        //cout << error_name_down << " <= " << endl;
        if(error_name_down != error_name) continue;
        cout << "!!! " << hs->GetName() << " " << hs_hist->GetName() << endl;
        shift_down = (TH1D*)hs_hist->Clone();
        shift_down->Add( h, -1. );
        shift_down->Divide( h );
      }
      shift_down->Draw("SAME hist");
      shift_down->Print();
      shift_down->SetLineColor(kBlue);
      
      double max = TMath::Max(hist_err->GetBinContent(hist_err->GetMaximumBin()), shift_down->GetBinContent(shift_down->GetMaximumBin()));
      double min = TMath::Min(hist_err->GetBinContent(hist_err->GetMinimumBin()), shift_down->GetBinContent(shift_down->GetMinimumBin()));
      if(min > 0) hist_err->SetMinimum( min*0.9 );
      else        hist_err->SetMinimum( min*1.1 );
      hist_err->SetMaximum( max*1.1 );
      canv_rel->Print(postfix + TString(hist->GetTitle()) + TString("_err_") + hs->GetName() + TString(".png"));
    }
*/
  }

}

void histsChecker(TString inputFileName_A, TString inputFileName_B, TString postfix){
  TFile * file_a = TFile::Open(inputFileName_A);
  auto hists_central_a = get_all_from_folder(file_a, ".+", "(.+Down)|(.+Up)|(.+_weight_.+)");

  TFile * file_b = TFile::Open(inputFileName_B);
  auto hists_central_b = get_all_from_folder(file_b, ".+", "(.+Down)|(.+Up)|(.+_weight_.+)");

  for(auto hist : *hists_central_a){
    TH1* hist_a = (TH1*)hist;
    TCanvas * canv = new TCanvas(hist_a->GetName(), hist_a->GetName(), 640, 640);
  
    string name = hist_a->GetName();
    auto hist_f = find_if(hists_central_b->begin(), hists_central_b->end(), [name] (const TObject* h) { return string( h->GetName() ) == name; } );
    if(hist_f == hists_central_b->end()) continue;
    cout << "THIS!!!" << endl;
    TH1* hist_b = (TH1*)(*hist_f);
    hist_b->Print();
    cout << "THIaS!!!" << hist_b << endl;

    hist_a->Draw("HIST");
    hist_a->SetLineColor(2);
    hist_a->SetLineWidth(3);

    hist_b->Draw("HIST same");
    hist_b->SetLineWidth(2);

    hist_a->SetMaximum( std::max( hist_b->GetMaximum(), hist_a->GetMaximum()) * 1.01 );

    canv->Print(postfix + TString(hist_a->GetTitle()) + TString(".png"));
  }

}

void histsChecker(TString inputFileName, TString postfix, int mode){
  TFile * file = TFile::Open(inputFileName);

  // draw all hists
  // for every draw central and shifted hists
  auto hists_central = get_all_from_folder(file, ".+", "(.+Down)|(.+Up)|(.+_weight_.+)");
  vector <int> indexes = getNiceColors(50);

  for(auto hist : *hists_central){
    TCanvas * canv = new TCanvas(hist->GetName(), hist->GetName(), 640, 640);
  
    TH1* h = (TH1*)hist;

    cout << h->GetName() << endl;
    double max = h->GetMaximum();
    auto shifts = get_all_from_folder(file, string(hist->GetName()) + ".+JER.+");
    int color_i = 0;
    h->SetLineColor( 1 );
    h->SetLineWidth( 3 );

    h->Draw("hist");

    for(auto shift : *shifts){
      TH1* hs = (TH1*)shift;
      hs->Draw("hist same");
      if(max < hs->GetMaximum()) max = hs->GetMaximum();
      hs->SetLineColor( indexes.at(color_i++) );
      hs->SetLineWidth( 2 );
    }

    h->GetYaxis()->SetRangeUser(0, max+max*0.1);
    h->Draw("hist same");

    if(shifts->size() == 2) {;
      int nbins =     h->GetXaxis()->GetNbins();
      cout << h->GetName() << " "<< shifts->at(0)->GetName() << " " << shifts->at(1)->GetName() << endl;
      for(int n = 1; n <= nbins; n++) {
        cout << h->GetBinContent(n) << " "<< ((TH1*)shifts->at(0))->GetBinContent(n) << " " << ((TH1*)shifts->at(1))->GetBinContent(n) << endl;
      }
    }

    canv->Print(postfix + TString(hist->GetTitle()) + TString(".png"));
  }

}



