#ifndef txt_database_hh
#define txt_database_hh 1

/////////////////////////////////////////////////////////////////////////////
//  Author      :     P.Mandrik, IHEP
//  Date        :     02/09/17
//  Version     :     0.5.0
/////////////////////////////////////////////////////////////////////////////
//
//  Python module TxtDatabase.py provide class for write and read
//  access into simple human readeble txt database:
//
//    order : key : comment : list of data as single string with commas
//    order : key : comment : list of data as single string with commas
//    order : key : comment : list of data as single string with commas
//    
/////////////////////////////////////////////////////////////////////////////
//  Changelog : 
//    02/09/17 v0.5.0
//    days of creation, version 0.5.0
/////////////////////////////////////////////////////////////////////////////

static void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

static void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());
}

std::string strip(std::string s) {
  ltrim(s);
  rtrim(s);
  return s;
}

vector<string> * split_and_strip_string(string str, const string & sep = " "){
  int sep_size = sep.size();
  vector<string> * answer = new vector<string>();

  for(int i = 0; i + sep_size < str.size(); i++){
    if( sep == str.substr(i, sep_size) ){
      answer->push_back( strip( string(str.substr(0, i)) ) ); 
      str = str.substr(i + sep_size, str.size() - i - sep_size);
      i = 0;
    }
  }

  answer->push_back(str);
  return answer;
}

struct TxtData{
  TxtData(){}
  TxtData(std::string v, std::string c, int o) : value(v), comment(c), order(o) {}
  std::string value;
  std::string comment;
  std::string key;
  int order;
};

class TxtDatabase {
  public:
  TxtDatabase(){ next_order = 0; }
  TxtDatabase(std::string file_name){
    fname = file_name;
    next_order = 0;
    Read();
  }

  void Print(){
    std::vector<TxtData> data_list;
    for(auto item : data){  
      item.second.key = item.first;
      data_list.push_back( item.second );
    }
    sort(data_list.begin(), data_list.end(), [] ( const TxtData &a, const TxtData &b ) { return a.order < b.order; } );

    for(auto item : data_list) 
      cout << to_string(item.order) << " : " << item.key << " : " << item.comment << " : " << item.value << endl;
  }

  void Read(){
    std::string line;
    ifstream ifile ( fname.c_str() );
    if(ifile.is_open()){
      while( getline(ifile,line) ){
        // cout << line << '\n';
        auto items = split_and_strip_string(line, ":");
        if(items->size() != 4) { delete items; continue; }

        TxtData new_data;
        new_data.order   =  atoi( items->at(0).c_str() );
        new_data.key     =  strip(items->at(1)) ;
        new_data.comment =  strip(items->at(2)) ;
        new_data.value   =  strip(items->at(3)) ;

        data[ new_data.key ] = new_data;

        delete items;
        next_order = max(next_order, new_data.order);
      }
      ifile.close();
      return;
    }
  }

  vector<TxtData> GetMatch(string include_rexp, string exclude_rexp = ""){
    vector<TxtData> answer;

    regex reg_incl( include_rexp );
    regex reg_excl( exclude_rexp );

    for(auto item : data){
      if( !regex_match(item.first, reg_incl) ) continue;
      if(  regex_match(item.first, reg_excl) ) continue;

      answer.push_back( item.second );
    }
    return answer;
  }

  void Write(){
    ofstream ofile;
    ofile.open( fname.c_str() );
    std::vector<TxtData> data_list;
    for(auto item : data){  
      item.second.key = item.first;
      data_list.push_back( item.second );
    }
    sort(data_list.begin(), data_list.end(), [] ( const TxtData &a, const TxtData &b ) { return a.order < b.order; } );

    for(auto item : data_list) 
      ofile << to_string(item.order) << " : " << strip(item.key) << " : " << strip(item.comment) << " : " << strip(item.value) << endl;

    ofile.close();
  }

  std::string GetItem(const std::string & key){
    auto item = data.find( key );
    if(item != data.end()) return item->second.value;
    cout << "TxtDatabase.GetItem() " << fname << " wrong key = " << key << endl;
    return "";
  }

  std::string GetComment(const std::string & key){
    auto item = data.find( key );
    if(item != data.end()) return item->second.value;
    cout << "TxtDatabase.GetComment() " << fname << " wrong key = " << key << endl;
    return "";
  }

  void SetItem(const std::string & key, const std::string & value, const std::string & comment=""){
    auto item = data.find( key );
    if(item != data.end() ){
      int order = item->second.order;
      data[key] = TxtData(value, comment, order);
      return;
    }
    data[key] = TxtData(value, comment, next_order++);
  }
  
  map<std::string, TxtData> data;

  private:
  std::string fname;
  int next_order;
};

#endif
