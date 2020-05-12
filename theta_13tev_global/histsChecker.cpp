
#include <regex>
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRootNiceColors.cpp"

#include "/afs/cern.ch/user/p/pmandrik/public/PMANDRIK_LIBRARY/pmlib_root_hist_drawer.hh"

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

#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/mRootStackDrawer.cpp"
void histsChecker(TString inputFileName, TString postfix, string diff_mode, int dummy){
  // TString inp_file_name = "hists13Charlie_SM.root";
  // TString inp_file_name = "hist_copy.root";
  // TString inp_file_name = "hists13Charlie_SM_fix.root";
  gErrorIgnoreLevel = kWarning;
  TFile * file = TFile::Open(inputFileName);
    
  mRoot::StackDrawer drawer;
  drawer.AddHistsFromFile( file, "W.+", "(Wjets)|(.+Down$)|(.+Up$)|(.+_alt)|(.+_weight_.+)|(.+_scale_.+)" );
  drawer.legend_width = 0.25;
  drawer.data_hist = (TH1D*)file->Get("Wjets");

  TCanvas * canv = drawer.GetCanvas( "Comparison_wjets" );
  canv->Print( "Comparison_wjets.png" );
  
  // draw all hists
  // for every draw central and shifted hists
  auto hists_central = get_all_from_folder(file, ".+", "(.+Down$)|(.+Up$)|(.+_alt)|(.+_weight_.+)|(.+_scale_.+)");
  vector <int> indexes = getNiceColors(50);

  for(auto hist : *hists_central){
    TCanvas * canv = new TCanvas(hist->GetName(), hist->GetName(), 640 * 3, 640*2);

    canv->Divide(3, 2);
  
    TH1* h = (TH1*)hist;

    cout << " " << h->GetName() << " ... " << endl;
    check_template(h);

    auto shifts = get_all_from_folder(file, string(hist->GetName()) + "_.+", "("+string(hist->GetName()) + "_alt.*)|(.+_weight_.+)");
    int color_i = 0;
    h->SetLineColor( 1 );
    h->SetLineWidth( 3 );

    TLegend * legend = new TLegend(0.05,0.05,0.95,0.95);
    legend->AddEntry(h,  h->GetName(), "l");
   

    list<TH1D*> herrs;
    string lname = "";
    int index = 0;
    int prev_index = -1;

    vector<TH1*> hists_other;
    for(auto shift : *shifts){
      TH1* hs = (TH1*)shift;
      check_template(hs);
      hs->SetLineColor( indexes.at(color_i) );
      hs->SetLineWidth( 2 );
      hists_other.push_back( hs );

      lname.size() < string(hs->GetTitle()).size() ? lname = string(hs->GetTitle()) : lname;

      TH1D* hist_err = (TH1D*)hs->Clone();
      hist_err->Add( h, -1. );
      hist_err->Divide( h );
      hist_err->SetMarkerStyle( 30 + color_i );
      hist_err->SetFillStyle( 3000+color_i );
      hist_err->SetFillColor( indexes.at(color_i) );
      color_i += 1;

      index++;

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

    // TH1D* central, const std::vector<TH1D*> & others
    for(int i = 1; i <= 4; i ++){
      canv->cd( i );
      vector<TH1*> hists_other_i;
      for(int j = (i-1) * hists_other.size() / 4; j < i * hists_other.size() / 4 ; j++ )
        hists_other_i.push_back( hists_other.at(j) );
      pm::draw_hists_difference( h, hists_other_i, diff_mode );
    }

    THStack * stack = new THStack(TString(h->GetName()) + "_stack", TString(h->GetName()) + "_stack");
    for(auto it = herrs.begin(); it!=herrs.end(); ++it){
      stack->Add( (*it) );
      
      string title = string((*it)->GetTitle());
      for(int i = (lname.size() - title.size())*1.3; i>=0; i--) title += " ";
      title += " - " + to_string( (*it)->Integral()/(*it)->GetNbinsX() );
      legend->AddEntry(*it,  title.c_str(), "lf");
    }

    canv->cd(5);
    legend->Draw();

    canv->cd(6);
    stack->Draw("hist f");

    canv->Print(postfix + TString(hist->GetTitle()) + TString(".png"));
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



