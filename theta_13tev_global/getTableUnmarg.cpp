
#include "getQuantiles.cpp"
#include "TxtDatabase.cpp"
#include "getTable.cpp"
#include "/afs/cern.ch/user/p/pmandrik/public/global_cfg/msg.hh"
using namespace mRoot;

using namespace std;

struct Systematic {
  Systematic(){ up = -999; down = -999; }
  void set_value(string name, string value){
    if( name.find("_up_") != std::string::npos or name.find("Up_") != std::string::npos)     up   = atof(value.c_str());
    if( name.find("_down_") != std::string::npos or name.find("Down_") != std::string::npos) down = atof(value.c_str());
  }

  string get_name(){
    return name;
  }

  float up, down;
  string name;
};

double getTableTDR(string varname, string mode_name, string postfix){
  TxtDatabase dbase("../analyse.db");

  double centra_cta  = atof(  dbase.GetMatch("cta_norm_expected_theta").at(0).value.c_str() );
  double central_uta = atof(  dbase.GetMatch("uta_norm_expected_theta").at(0).value.c_str() );
  
  // get central value and difference // sigma_t_ch_cl // 
  vector<Systematic> systematics;
  for(int i = 3; i < 12; i++){
    string cta_name = "cta_norm_expected_weight_" + to_string(i) + "_theta";
    string uta_name = "uta_norm_expected_weight_" + to_string(i) + "_theta";

    auto cta_matches = dbase.GetMatch( cta_name );
    auto uta_matches = dbase.GetMatch( uta_name );

    Systematic new_sys;
    new_sys.up   = atof(cta_matches.at(0).value.c_str());
    new_sys.down = atof(uta_matches.at(0).value.c_str());

    new_sys.name = "weight_" + to_string(i);
    systematics.push_back( new_sys );
  }

  // SYSTEMATIC TABLE ===================================================================
  string out_string = "\\documentclass{article} \n \\usepackage{graphicx}";
         //out_string += "\\usepackage[paperheight=16.75in,paperwidth=27.25in]{geometry}";
         out_string += "\n \\begin{document} \n";
         out_string += "\\begin{center} \n \\begin{tabular}{ | c | c | c | } \n";
         out_string += "\\hline Systematic & $c \\rightarrow t\\gamma$ & $u \\rightarrow t\\gamma$ \\\\ \n \\hline \n";

  for(auto sys : systematics){
    if(sys.up < 0 or sys.down < 0) continue;
    msg( sys.get_name(), centra_cta, sys.up, central_uta, sys.down);
    out_string += sys.get_name() + " & " + get_perc((centra_cta-sys.up)/centra_cta, 3) + " & " + get_perc((central_uta-sys.down)/central_uta, 3) + " \\\\ \n ";
  }
  out_string += " \\hline \\end{tabular} \n \\end{center} \n";

  out_string += " \n \\end{document} \n";
  ReplaceStringInPlace( out_string, string("_"), string("\\_"));

  ofstream out( ("getTableUnmarg_" + postfix + ".tex").c_str() );
  out << out_string << endl;
  out.close();

  return 0;
}

double getTableUnmarg(string varname, string mode_name, string postfix){
  if(mode_name == "expected_theta") return getTableTDR(varname, mode_name, postfix);
  // read current database
  cout << "getTableUnmarg(" + varname + "," + postfix + ") ... " << endl;
  TxtDatabase dbase("../analyse.db");
  
  // get central value and difference // sigma_t_ch_cl // 
  vector<Systematic> systematics;

  vector<string> sys_names = {"JEC", "tW_ch_scale", "t_ch_scale", "ttbar_scale" };
  for(auto sys_name : sys_names){
    auto matches_down = dbase.GetMatch(varname + "_cl.+" + sys_name + "_down.+");
    auto matches_up   = dbase.GetMatch(varname + "_cl.+" + sys_name + "_up.+");
    if( not matches_down.size() or not matches_up.size() ) {
      matches_down = dbase.GetMatch(varname + "_cl.+" + sys_name + "Down.+");
      matches_up   = dbase.GetMatch(varname + "_cl.+" + sys_name + "Up.+");
    }
    if( not matches_down.size() or not matches_up.size() ) continue;
    
    Systematic new_sys;
    new_sys.set_value( matches_up.at(0).key,   matches_up.at(0).value   );
    new_sys.set_value( matches_down.at(0).key, matches_down.at(0).value );
    new_sys.name = sys_name;
    systematics.push_back( new_sys );
  }

  auto matches_central = dbase.GetMatch(varname + "_cl.+" + mode_name, "(.+down.+)|(.+up.+)|(.+Down.+)|(.+Up.+)");
  cout << matches_central.size() << endl;
  float central = atof( matches_central.at(0).value.c_str() );
  cout << "central = " << central << endl;

  // SYSTEMATIC TABLE ===================================================================
  string out_string = "\\documentclass{article} \n \\usepackage{graphicx}";
         //out_string += "\\usepackage[paperheight=16.75in,paperwidth=27.25in]{geometry}";
         out_string += "\n \\begin{document} \n";
         out_string += "variable = " + varname + ", central value = " + get_string(central, 3) + "\n";
         out_string += "\\begin{center} \n \\begin{tabular}{ | c | c | c | c | c| } \n";
         out_string += "\\hline Systematic & Down & Up & Down Relative & Up Relative\\\\ \n \\hline \n";

  for(auto sys : systematics){
    if(sys.up < 0 or sys.down < 0) continue;
    msg(sys.name, sys.down, sys.up, get_perc((central-sys.down)/central, 3), get_perc((central-sys.up)/central, 3) );
    out_string += sys.get_name() + " & " + get_string(sys.down, 4) + " & " + get_string(sys.up, 4) + " & " + get_perc((central-sys.down)/central, 3) + " & " + get_perc((central-sys.up)/central, 3) + " \\\\ \n ";
  }
  out_string += " \\hline \\end{tabular} \n \\end{center} \n";

  float total_unmarg_unc = 0;
  for(auto sys : systematics){
    total_unmarg_unc += pow(max(abs(central-sys.down), abs(central-sys.up)), 2);
  }
  total_unmarg_unc = sqrt( total_unmarg_unc );
  out_string += "Total unmarginal systematic uncertantie = " +  get_string(total_unmarg_unc, 4) + " " + get_perc(total_unmarg_unc/central, 3) + "\\\\";
  msg("Total unmarginal systematic uncertantie = " +  get_string(total_unmarg_unc, 4) + " " + get_perc(total_unmarg_unc/central, 3));

  out_string += " \n \\end{document} \n";
  ReplaceStringInPlace( out_string, string("_"), string("\\_"));

  ofstream out( ("getTableUnmarg_" + postfix + ".tex").c_str() );
  out << out_string << endl;
  out.close();

  return 0;
}


  /*
  \begin{center}
  \begin{tabular}{ c c c }
   cell1 & cell2 & cell3 \\ 
   cell4 & cell5 & cell6 \\  
   cell7 & cell8 & cell9    
  \end{tabular}
  \end{center}
  */





