# -*- coding: utf-8 -*-

"""
/////////////////////////////////////////////////////////////////////////////
  Author      :     P.S. Mandrik, IHEP
  Date        :     13/03/17
  Version     :     0.5.0
/////////////////////////////////////////////////////////////////////////////

  Python module AutoTHETADatacart.py provide function to create theta .cfg cards.
  ROOT-only, skip some of theta configurations, single input file
    
/////////////////////////////////////////////////////////////////////////////
  Changelog : 
    10/03/17 - 13/03/17  - days of creation, version 0.5.0
"""

import TemplateMaster

def_theta_template = """

// generated with AutoTHETADatacart.py and TemplateMaster.py at {% DATE %}

observables = {
  obs_name = {
    range = (0.0, 1.0);
    nbins = {% N_BINS %};
  };	
};

// input histograms definitions ----------------
{% for CHANAL in CHANALS %}
  // {% CHANAL.name %} ----------------
  // nominal
  {% CHANAL.name %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE %}";
    histoname = "{% CHANAL.name %}";
    use_errors = true; 
  };
{% for PARAMETER in CHANAL.interpol_params %}
  // {% CHANAL.name %} + {% PARAMETER %}
  {% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE %}";
    histoname = "{% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %}";
    use_errors = true; 
  };
  {% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %} = {
    type = "root_histogram";
    filename = "{% INPUT_FILE %}";
    histoname = "{% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %}";
    use_errors = true; 
  };

{% endfor %}

{% endfor %}

// parameters definition ----------------
  parameters = {% func theta_set_like PARAMETERS %}{% endfunc %};

// model definition ----------------
  {% MODEL_NAME %} = {
    obs_name = {
{% for CHANAL in CHANALS %}
        {% CHANAL.name %} = {
          coefficient-function = {type = "multiply"; factors = {% func theta_set_like CHANAL.mult_params %}{% endfunc %}; };
          histogram = { 
            type = "interpolating_histo";
            parameters = {% func theta_set_like CHANAL.interpol_params %}{% endfunc %};
            nominal-histogram = "@{% CHANAL.name %}";
{% for PARAMETER in CHANAL.interpol_params %}
            {% PARAMETER %}-plus-histogram  = "@{% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_PLUS %}";
            {% PARAMETER %}-minus-histogram = "@{% CHANAL.name %}_{% PARAMETER %}{% INTER_POSTFIX_MINUS %}";
{% endfor %}          };
        };
{% endfor %}
    };

    parameter-distribution = {
      type ="product_distribution";
      distributions ={% func theta_set_like_distr PARAMETERS %}{% endfunc %};
    };

    bb_uncertainties = {% BARLOW_BESTON %};
  };

// parameters distributions definitions ----------------
{% for DISTRIBUTION in PARAMETERS_DISTRIBUTIONS %}
  {% DISTRIBUTION.name %}-dist = {
    type = "{% DISTRIBUTION.type %}";
{% if DISTRIBUTION.is_flat %}
    {% DISTRIBUTION.name %} = {
{% for OTHER_PAIRS in DISTRIBUTION.other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
    };
{% else %}
    parameter = "{% DISTRIBUTION.name %}";
{% for OTHER_PAIRS in DISTRIBUTION.other %}    {% func theta_expand_pair OTHER_PAIRS %}{% endfunc %};
{% endfor %}  };
{% endif %}
{% endfor %}

// hypotest definition ----------------
{% if USE_MCMC %}
  hypotest = {
    type = "mcmc_chain";
    name = "mcmc_chain";
    iterations = {% MCMC_ITERATION_N %};
    burn-in    = 0; 
    re-init    = 0;
    output_database = {
      type = "rootfile_database";
      filename = "{% MODEL_NAME %}.root";
    };
  };
{% else %}
  hypotest = {
    type = "mle";
    name = "mle";
    parameters = {% func theta_set_like PARAMETERS %}{% endfunc %};
    minimizer = {
      type = "root_minuit";
    };
	};
{% endif %}

// main definition ----------------
  main = {
    data_source = {
      type = "histo_source";
      obs_name = {
        type= "root_histogram";
        filename ="{% INPUT_FILE_OBS %}";
        histoname = "data";
      };
      name = "source";
    };
    
    model = "@{% MODEL_NAME %}";
    producers = ("@hypotest");
    n-events = {% MCMC_CHAINS_N %};

    output_database = {
      type = "rootfile_database";
      filename = "{% MODEL_NAME %}.root";
      products_data = "*";
    };
  };

// final options  ----------------
  options = {
    plugin_files = ("$THETA_DIR/lib/root.so", "$THETA_DIR/lib/core-plugins.so" );
  };

"""

def_theta_table="""
  % generated with AutoTHETADatacart.py and TemplateMaster.py at {% DATE %}
\documentclass{article} 
\usepackage{graphicx} 
\usepackage{xcolor}
\usepackage{multirow}

\\begin{document} 
  \\begin{center} 

  % table of parameter vx chanals
  Parameters vs Chanals table:\\\\
  \\begin{small}
    \\setlength{\\tabcolsep}{2pt} % General space between cols (6pt standard)
    \\renewcommand{\\arraystretch}{1}
    \\begin{tabular}{ | c | {% for CHANAL in CHANALS %} c |{% endfor %}  }
    \hline
    \multirow{2}{*}{Parameters} & \multicolumn{ {% CHANALS.__len__ %}}{ |c| }{ Chanals } \\\\ 
    \cline{2-{% CHANALS.__len__ %}}
   {% for CHANAL in CHANALS %} & {% CHANAL.name %} {% endfor %} \\\\ 
    \hline
    \hline
  {% for PARAMETER in PARAMETERS %}
      {% PARAMETER %} {% for CHANAL in CHANALS %} & {% func tabulate_check_intersection CHANAL PARAMETER %}{% endfunc %} {% endfor %} \\\\
      \hline
  {% endfor %}
    \hline
    \\end{tabular}
  *N - template normalization parameter,\\\\
  *V - shape variation parameter\\
  \\end{small}
  
  \\newpage
  % table of parameter vx chanals
  Parameters descriptions table:\\\\
  \\begin{scriptsize}
    \\setlength{\\tabcolsep}{3pt} % General space between cols (6pt standard)
    \\renewcommand{\\arraystretch}{2}
    \\begin{tabular}{ | c | c | c |  }
    \hline
      Parameters & Prior Type & Prior Options \\\\
    \hline
    \hline
  {% for PARAMETER in PARAMETERS_DISTRIBUTIONS %}
      {% PARAMETER.name %} & {% PARAMETER.type %} & \shortstack{{% func tabulate_get_stack PARAMETER.other %}{% endfunc %}} \\\\
      \hline
  {% endfor %}
    \hline
    \\end{tabular}
  \\end{scriptsize}

  \end{center} 
\end{document} 
"""

import os.path
import copy

def tabulate_check_intersection(text, chanal, parameter):
  if parameter in chanal.mult_params : return "N"
  if parameter in chanal.interpol_params : return "V"
  else : return "-"

def tabulate_get_stack(text, list):
  answer = ""
  for i, item in enumerate(list):
    if type(item[1]) == type(1.1):
      answer += str(item[0]) + " = " + "{0:.2e}".format( item[1] )
    else : answer += str(item[0]) + " = " + str(item[1])
    if i != len(list)-1 : answer += " \\\\ "
  return answer

def theta_set_like(text, list=[]):
  answer = "("
  for i, item in enumerate(list):
    answer += "\"" + text + item + "\""
    if i != len(list)-1 : answer += ", "
  return answer + ")"

def theta_set_like_distr(text, list=[]):
  answer = "("
  for i, item in enumerate(list):
    answer += "\"@"+ item + "-dist\""
    if i != len(list)-1 : answer += ", "
  return answer + ")"

def theta_expand_pair(text, list=[]):
  return str(list[0]) + " = " + str(list[1]);

class ChanalMC():
  def __init__(self, name):
    self.name = name
    self.mult_params = []
    self.interpol_params = []

  def has_parameter(self, name):
    if name in self.mult_params : return True
    if name in self.interpol_params : return True
    return False

class ParameterDistribution():
  def __init__(self, name, type):
    self.name = name
    self.type = type
    self.other = []

  def is_flat(self):
    if self.type ==  "flat_distribution" : return True
    return False

class DatacardMaster():
  def __init__(self, name, n_bins, path=""):
    self.interpolating_postfixes = ["Up", "Down"]
    self.mc_chanals = []
    self.parameters_distributions = []
    self.input_file = ".root"
    self.input_file_obs = ".root"
    self.enable_barlow_beston = "true"
    self.name = name
    self.path = ""
    self.mcmc_iters = 10000
    self.mcmc_chains = 10
    self.n_nibs = n_bins
    self.use_mcmc = True

  def get_all_parameters(self):
    parameters = []
    for par in self.parameters_distributions:
      if par not in parameters : 
        parameters.append( par.name );
    return parameters

  def get_def_dic(self):
    dic = {}
    from time import gmtime, strftime
    dic["DATE"] = strftime("%Y-%m-%d %H:%M:%S", gmtime())
    dic["CHANALS"] = self.mc_chanals;
    dic["PARAMETERS_DISTRIBUTIONS"] = self.parameters_distributions;
    dic["INPUT_FILE_OBS"] = self.input_file
    dic["INPUT_FILE"] = self.input_file
    dic["INTER_POSTFIX_PLUS"]  = self.interpolating_postfixes[0]
    dic["INTER_POSTFIX_MINUS"] = self.interpolating_postfixes[1]
    dic["BARLOW_BESTON"] = self.enable_barlow_beston
    dic["N_BINS"] = self.n_nibs
    dic["MODEL_NAME"] = self.name
    dic["PARAMETERS"] = self.get_all_parameters()
    dic["MCMC_ITERATION_N"] = self.mcmc_iters
    dic["MCMC_CHAINS_N"] = self.mcmc_chains
    dic["USE_MCMC"] = self.use_mcmc

    dic["theta_set_like"] = theta_set_like
    dic["theta_set_like_distr"] = theta_set_like_distr
    dic["theta_expand_pair"] = theta_expand_pair
    dic["tabulate_check_intersection"] = tabulate_check_intersection
    dic["tabulate_get_stack"] = tabulate_get_stack

    TemplateMaster.add_TemplateMaster_dictionary(dic)
    return dic

  def parce_datacard(self):
    dic = self.get_def_dic()
    return TemplateMaster.parce_template(def_theta_template, dic)

  def parce_table(self):
    dic = self.get_def_dic()
    return TemplateMaster.parce_template(def_theta_table, dic)

  def save(self):
    datacard = self.parce_datacard()
    full_name = os.path.join(self.path, self.name+".cfg")
    f = open(full_name, "w")
    f.write(datacard)
    f.close()

    table = self.parce_table()
    table = table.replace("_", "\\_")
    full_name = os.path.join(self.path, self.name+".tex")
    f = open(full_name, "w")
    f.write(table)
    f.close()

def test():
  print "AutoTHETADatacard test ... "
  datacard = DatacardMaster("theta_test")
  datacard.n_nibs = 50;

  chanals = [
    "Drell-Yan", 
    "Diboson",
    "s-ch",
    "t-ch",
    "tw-ch",
    "ttbar",
    "Wjets_WQQ",
    "Wjets_Wc",
    "Wjets_W_light",
    "Wjets_W_other",
    "QCD"
  ]

  common_mult_pars = ["id", "iso", "trig", "lumi"]
  common_interp_pars = ["TagRate", "MistagRate","JEC","JER","METUnclustered", "PileUp"]
  
  for name in chanals:
    chanal_mc = ChanalMC( name )
    chanal_mc.mult_params = common_mult_pars[:-1]
    chanal_mc.interpol_params = common_interp_pars[:-1]
    datacard.mc_chanals += [ chanal_mc ]
    break

  for par in common_mult_pars :
    distr = ParameterDistribution(par, "gauss")
    distr.other += [ ["mean", "1.0"] ]
    distr.other += [ ["width", "0.283"] ]
    distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
    datacard.parameters_distributions += [ distr ]

  print datacard.parce_datacard()
  datacard.save()

#--------------------------------------------------------------------------
import math
def qcd_march(nbins, input, args):
  datacard = DatacardMaster("theta_qcd_march", nbins)
  chanals = [
    "other",
    "QCD"
  ]

  mult_pars = ["f_QCD", "f_Other"]

  for name in chanals:
    chanal_mc = ChanalMC( name )
    if name == "QCD": chanal_mc.mult_params = [ "f_QCD" ]
    #else :            chanal_mc.mult_params = [ "f_Other" ]
    datacard.mc_chanals  += [ chanal_mc ]

  for par in mult_pars :
    if par != "f_QCD":  continue
    distr = ParameterDistribution(par, "flat_distribution")
    distr.other += [ ["fix-sample-value", 1.0] ]
    distr.other += [ ["range", '(0.0, "inf")']]
    datacard.parameters_distributions += [ distr ]

  datacard.use_mcmc = True
  datacard.input_file = input
  datacard.mcmc_iters  = 100000
  datacard.mcmc_chains = 1
  datacard.save()

def gauss_to_theta_lognormal(val):
  tval = math.sqrt( math.log(0.5 + 0.5 * math.sqrt(1 + 4 * val*val)) )
  #print val, tval
  return tval

def sm_8TeV(nbins, input, args):
  datacard = DatacardMaster("theta_sm_8TeV", nbins)
  datacard.input_file = input

  chanals = [
    "t_ch",
    "s_ch",
    "tW_ch",
    "ttbar",
    "Diboson",
    "DY", 
    "WQQ",
    "Wc",
    "Wlight",
    "Wother",
    "QCD"
  ]

  int_pars = ["PileUp", "TagRate", "MistagRate", "JEC", "JER", "METUnclustered"]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.026]

  # this is log-normals ...
  xsec_errors = {
    "s_ch"   : 0.148,
    "tW_ch"  : 0.128,   
    "ttbar"  : 0.148, 
    "Diboson": 0.283, 
    "DY"     : 0.283, 
    "WQQ"    : 0.694, 
    "Wc"     : 0.694,
    "Wlight" : 0.434,
    "Wother" : 0.434, 
    "QCD"    : 0.694
  }
  #print xsec_errors

  for name in chanals:
    chanal_mc = ChanalMC( name )
    chanal_mc.mult_params = copy.copy(mult_pars)
    datacard.mc_chanals  += [ chanal_mc ]

    if name != "QCD":
      chanal_mc.interpol_params = copy.copy(int_pars)

    if name != "t_ch" and True:
      chanal_mc.mult_params += [ "sigma_" + name ]
      xerror = xsec_errors.get( name )
      if True : 
        distr = ParameterDistribution("sigma_" + name, "log_normal")
        distr.other += [ ["mu", 0.0] ]
        distr.other += [ ["sigma", xerror ] ]
        datacard.parameters_distributions += [ distr ]
      else :
        distr = ParameterDistribution("sigma_" + name, "gauss")
        distr.other += [ ["mean", "1.0"] ]
        distr.other += [ ["width", xerror] ]
        distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
        datacard.parameters_distributions += [ distr ]
    if name == "t_ch" :
      chanal_mc.mult_params += [ "sigma_" + name ]
      distr = ParameterDistribution("sigma_" + name, "flat_distribution")
      distr.other += [ ["fix-sample-value", 1.0] ]
      distr.other += [ ["range", '(0.0, "inf")']]
      datacard.parameters_distributions += [ distr ]

  for par, err in zip(mult_pars, mult_errs) :
    distr = ParameterDistribution(par, "gauss")
    distr.other += [ ["mean", "1.0"] ]
    distr.other += [ ["width", err ] ]
    distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
    datacard.parameters_distributions += [ distr ]

  for par in int_pars:
    distr = ParameterDistribution(par, "gauss")
    distr.other += [ ["mean", "0.0"] ]
    distr.other += [ ["width", "1.0"] ]
    distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
    datacard.parameters_distributions += [ distr ]

  datacard.use_mcmc = True
  datacard.input_file = input
  datacard.mcmc_iters  = 500000
  datacard.mcmc_chains = 1
  datacard.save()
  return datacard

def sm_march(nbins, input, args):
  datacard = DatacardMaster("theta_sm_march", nbins)
  datacard.input_file = input

  chanals = [
    "t_ch",
    "s_ch",
    "tW_ch",
    "ttbar",
    "Diboson",
    "DY", 
    "WQQ",
    "Wc",
    "Wb",
    "Wother",
    "QCD"
  ]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.062]

  xsec_errors = {
    "t_ch"   : 4. / 136.02,
    "s_ch"   : 0.09  / 3.34,       # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/SingleTopRefXsec#Single_top_t_channel_cross_secti 
    "tW_ch"  : 1.80  / 71.7,       # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/SingleTopRefXsec#Single_top_t_channel_cross_secti 
    "ttbar"  : 29.20 / 831.76,     # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/TtbarNNLO
    "Diboson": pow(pow(118.7*0.025,2) + pow(16.91*0.032, 2) + pow(45.01*0.054,2), 0.5) / (118.7 + 16.91 + 45.01), # https://arxiv.org/pdf/1408.5243.pdf https://arxiv.org/pdf/1405.2219.pdf  https://twiki.cern.ch/twiki/bin/view/CMS/GenXsecTaskForce
    "DY"     : 17.26 / 1921.8,     # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#DY_Z
    "WQQ"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wc"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wb"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wother"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "QCD"    : max(abs(args.QCD_low - args.QCD_mid), abs(args.QCD_upp - args.QCD_mid))
  }
  #print xsec_errors

  for name in chanals:
    chanal_mc = ChanalMC( name )
    chanal_mc.mult_params = copy.copy(mult_pars)
    datacard.mc_chanals  += [ chanal_mc ]

    if name != "t_ch" and True:
      chanal_mc.mult_params += [ "sigma_" + name ]
      xerror = xsec_errors.get( name )
      if True : 
        distr = ParameterDistribution("sigma_" + name, "log_normal")
        distr.other += [ ["mu", 0.0] ]
        distr.other += [ ["sigma", gauss_to_theta_lognormal(xerror) ] ]
        datacard.parameters_distributions += [ distr ]
      else :
        distr = ParameterDistribution("sigma_" + name, "gauss")
        distr.other += [ ["mean", "1.0"] ]
        distr.other += [ ["width", xerror] ]
        distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
        datacard.parameters_distributions += [ distr ]
    if name == "t_ch" :
      chanal_mc.mult_params += [ "sigma_" + name ]
      distr = ParameterDistribution("sigma_" + name, "flat_distribution")
      distr.other += [ ["fix-sample-value", 1.0] ]
      distr.other += [ ["range", '(0.0, "inf")']]
      datacard.parameters_distributions += [ distr ]

  for par, err in zip(mult_pars, mult_errs) :
    distr = ParameterDistribution(par, "log_normal")
    distr.other += [ ["mu", 0.0] ]
    distr.other += [ ["sigma", gauss_to_theta_lognormal(err) ] ]
    datacard.parameters_distributions += [ distr ]

  datacard.use_mcmc = True
  datacard.input_file = input
  datacard.mcmc_iters  = 500000
  datacard.mcmc_chains = 1
  datacard.save()
  return datacard

def fcnc_march(nbins, input, args):
  datacard = DatacardMaster("theta_fcnc_march", nbins)
  datacard.input_file = input

  chanals = [
    "t_ch",
    "s_ch",
    "tW_ch",
    "ttbar",
    "Diboson",
    "DY", 
    "WQQ",
    "Wc",
    "Wb",
    "Wother",
    "QCD",
    "fcnc_tug",
    "fcnc_tcg"
  ]

  mult_pars = ["id", "iso", "trig", "lumi"]
  mult_errs = [0.005, 0.002, 0.002, 0.062]

  xsec_errors = {
    "t_ch"   : 4. / 136.02,
    "s_ch"   : 0.09  / 3.34,       # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/SingleTopRefXsec#Single_top_t_channel_cross_secti 
    "tW_ch"  : 1.80  / 71.7,       # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/SingleTopRefXsec#Single_top_t_channel_cross_secti 
    "ttbar"  : 29.20 / 831.76,     # https://twiki.cern.ch/twiki/bin/view/LHCPhysics/TtbarNNLO
    "Diboson": pow(pow(118.7*0.025,2) + pow(16.91*0.032, 2) + pow(45.01*0.054,2), 0.5) / (118.7 + 16.91 + 45.01), # https://arxiv.org/pdf/1408.5243.pdf https://arxiv.org/pdf/1405.2219.pdf  https://twiki.cern.ch/twiki/bin/view/CMS/GenXsecTaskForce
    "DY"     : 17.26 / 1921.8,     # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#DY_Z
    "WQQ"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wc"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wb"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "Wother"  : 2312.7 / 61526.7,   # https://twiki.cern.ch/twiki/bin/viewauth/CMS/SummaryTable1G25ns#W_jets
    "QCD"    : max(abs(args.QCD_low - args.QCD_mid), abs(args.QCD_upp - args.QCD_mid))
  }
  #print xsec_errors

  for name in chanals:
    chanal_mc = ChanalMC( name )
    chanal_mc.mult_params = copy.copy(mult_pars)
    datacard.mc_chanals  += [ chanal_mc ]

    if name not in ["fcnc_tug", "fcnc_tcg"] and True:
      chanal_mc.mult_params += [ "sigma_" + name ]
      xerror = xsec_errors.get( name )
      if True : 
        distr = ParameterDistribution("sigma_" + name, "log_normal")
        distr.other += [ ["mu", 0.0] ]
        distr.other += [ ["sigma", gauss_to_theta_lognormal(xerror) ] ]
        datacard.parameters_distributions += [ distr ]
      else :
        distr = ParameterDistribution("sigma_" + name, "gauss")
        distr.other += [ ["mean", "1.0"] ]
        distr.other += [ ["width", xerror] ]
        distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
        datacard.parameters_distributions += [ distr ]
    if name == "fcnc_tcg" :
      chanal_mc.mult_params += ["KC", "KC"]
    if name == "fcnc_tug" :
      chanal_mc.mult_params += ["KU", "KU"]
      
  distr_KU = ParameterDistribution("KC", "flat_distribution")
  distr_KU.other += [ ["fix-sample-value", 1.0] ]
  distr_KU.other += [ ["range", '(0.0, "inf")']]
  datacard.parameters_distributions += [ distr_KU ]

  distr_KC = ParameterDistribution("KU", "flat_distribution")
  distr_KC.other += [ ["fix-sample-value", 1.0] ]
  distr_KC.other += [ ["range", '(0.0, "inf")']]
  datacard.parameters_distributions += [ distr_KC ]

  #distr = ParameterDistribution("sigma_other", "flat_distribution")
  #distr.other += [ ["fix-sample-value", 1.0] ]
  #distr.other += [ ["range", '(0.0, "inf")']]
  #datacard.parameters_distributions += [ distr ]

  for par, err in zip(mult_pars, mult_errs) :
    distr = ParameterDistribution(par, "log_normal")
    #distr.other += [ ["mean", "1.0"] ]
    #distr.other += [ ["width", err ] ]
    #distr.other += [ ["range", "(\"-inf\", \"inf\")"]]
    distr.other += [ ["mu", 0.0] ]
    distr.other += [ ["sigma", gauss_to_theta_lognormal(err) ] ]
    datacard.parameters_distributions += [ distr ]

  datacard.use_mcmc = True
  datacard.input_file = input
  datacard.mcmc_iters  = 500000
  datacard.mcmc_chains = 1
  datacard.save()
  return datacard

#--------------------------------------------------------------------------

if __name__ == "__main__": 
  import argparse
  parser = argparse.ArgumentParser(description='Produce some theta .cfg datacards ... ')
  parser.add_argument('--fname', dest='fname', type=str, default='test', help='predifined function name to call')
  parser.add_argument('--nbins', dest='nbins', type=int, default=10,     help='number of bins in histograms')
  parser.add_argument('--input', dest='input', type=str, default='input.root', help='root input file name')
  parser.add_argument('--QCD_mid', dest='QCD_mid', type=float, default=0, help='')
  parser.add_argument('--QCD_low', dest='QCD_low', type=float, default=0, help='')
  parser.add_argument('--QCD_upp', dest='QCD_upp',  type=float, default=0, help='')
  args = parser.parse_args()

  flist = [f for f in globals().values() if (type(f) == type(test) and f.__name__ == args.fname) ]
  if flist : flist[0](args.nbins, args.input, args)
  else     : test()








