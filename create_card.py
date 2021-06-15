# -*- coding: utf-8 -*-

import sys
import copy

sys.path.append( "/afs/cern.ch/user/p/pmandrik/public/analysis/MSU_statistics_13TEV/20-04-03_tqg/FCNC_tqg_stat/theta_13tev_global/" )

import AutoDatacard as atd


import math
def qcd(args):
  datacard = atd.DatacardMaster("qcd", args.nbins)

  chanal_qcd   = atd.Chanal( "QCD" )
  chanal_other = atd.Chanal( "other" )

  datacard.chanals  += [ chanal_qcd, chanal_other ]

  f_qcd = atd.Parameter( "f_QCD", "flat_distribution", "mult" )
  f_qcd.options["mean"]  = 1.0
  f_qcd.options["range"] = '(0.0,"inf")'
  chanal_qcd.parameters = [ f_qcd ]

  f_other = atd.Parameter( "f_Other", "flat_distribution", "mult")
  f_other.options["mean"]  = 1.0
  f_other.options["range"] = '(0.0,"inf")'
  chanal_other.parameters = [ f_other ]

  datacard.use_mcmc = True
  datacard.input_file_mc   = args.input
  datacard.mcmc_iters  = args.niters
  return datacard

def sm(args):
  datacard = atd.DatacardMaster("sm", args.nbins)

  chanals_names = [
    "t_ch",    31./207., '(-0.5,0.5)', # 0.10
    "s_ch",    0.10, '(-2.85,3.0)',
    "tW_ch",   0.15, '(-6.0,1.5)',
    "ttbar",   0.15, '(-4.0,5.0)',
    "Diboson", 0.20, '(-3.0,3.0)',
    "DY",      0.20, '(-3.0,3.0)',
    "WQQ",     0.30, '(-1.0,4.5)',
    "Wc",      0.30, '(-2.5,2.0)',
    "Wb",      0.30, '(-2.5,3.5)',
    "Wother",  0.30, '(-3.5,1.5)',
    "Wlight",  0.30, '(-3.5,1.5)',
  # "Wjets",   0.30, '(-3.5,1.5)',
    "QCD",     1.00, '(-3.0,1.5)',
  ]

  mult_pars = [ "lumi" ]
  mult_errs = [ 0.025  ]

  interp_pars  = ["jes", "lf", "hf", "hfstats1", "hfstats2", "lfstats1", "lfstats2", "cferr1", "cferr2" ]
  interp_pars += ["PileUp", "pdf"]
  interp_pars += ["UnclMET", "MER"] # PUJetIdTag
  interp_pars += ["JER_eta0_193", "JER_eta193_25", "JER_eta25_3_p0_50", "JER_eta25_3_p50_Inf", "JER_eta3_5_p0_50", "JER_eta3_5_p50_Inf"]
  interp_pars += ["JEC_eta0_25", "JEC_eta25_5"]
  interp_pars += ["LepId", "LepTrig", "LepIso"]
  muRmuF_pars  = ["Fac", "Ren", "RenFac"]
  interp_pars += muRmuF_pars
  xsr_pars     = ["Isr", "Fsr"]
  #interp_pars += xsr_pars
  #interp_pars = []

  has_muRmuF, has_xsr = [], []
  has_muRmuF = ["s_ch", "ttbar", "WQQ", "Wb", "Wc", "Wother", "DY"]
  #has_xsr    = ["ttbar", "tW_ch"]

  pss = ["_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"]
  pss = []
  pss_names = []
  for item in ["isr", "fsr"] :
    for ps in pss:
      interp_pars += [ item + ps ]
      pss_names += [ item + ps ]

  datacard.parameters_order_list  = [ "sigma_" + name for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ) ] 
  datacard.parameters_order_list += mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = atd.Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  0.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(-2.5,2.5)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = atd.Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  hdamp_par  = atd.Parameter( "hdamp", "gauss", "shape")
  UETune_par = atd.Parameter( "UETune", "gauss", "shape")
  with_hdamp, with_UETune  = [], []
  # with_hdamp  = ["ttbar", "tW_ch"]
  # with_UETune = ["ttbar", "tW_ch"]

  # define chanals
  for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ):
    chanal        = atd.Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)

    interp_pars = []
    if name in with_hdamp  : interp_pars += [ hdamp_par  ]
    if name in with_UETune : interp_pars += [ UETune_par ]

    for param in common_interp_pars:
      if param.name in muRmuF_pars  and name not in has_muRmuF : continue
      if param.name in xsr_pars     and name not in has_xsr    : continue
      interp_pars += [ param ]

    if name != "t_ch":
      norm_parameter = atd.Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]  =  0.0
      norm_parameter.options["width"] =  err
      norm_parameter.options["range"] = rang
      chanal.parameters += [ norm_parameter ]
    else :
      norm_parameter = atd.Parameter( "sigma_" + name, "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 1.0
      norm_parameter.options["range"] = rang
      chanal.parameters += [ norm_parameter ]

    if name != "QCD" : chanal.parameters += interp_pars

    datacard.chanals += [ chanal ]

  return datacard

def fcnc_1d(args, coupling_hist_name):
  datacard = atd.DatacardMaster("fcnc_1d_CHANGE_THIS_NAME", args.nbins)

  chanals_names = [
    "t_ch",    31./207., '(-0.5,0.5)', # 0.10
    "s_ch",    0.10, '(-2.85,3.0)',
    "tW_ch",   0.15, '(-6.0,1.5)',
    "ttbar",   0.15, '(-4.0,5.0)',
    "Diboson", 0.20, '(-3.0,3.0)',
    "DY",      0.20, '(-3.0,3.0)',
    "WQQ",     0.30, '(-1.0,4.5)',
    "Wc",      0.30, '(-2.5,2.0)',
    "Wb",      0.30, '(-2.5,3.5)',
    "Wother",  0.30, '(-3.5,1.5)',
    #"Wjets",   0.30, '(-3.5,1.5)',
    "QCD",     1.00, '(-3.0,1.5)',
    coupling_hist_name, 0.0, '(-0.5,0.5)', # 0.10
  ]

  mult_pars = ["lumi"]
  mult_errs = [0.025]

  interp_pars  = ["TagRate", "MistagRate" ] # "PUJetIdTag" "PUJetIdMistag"
  interp_pars  = ["jes", "lf", "hf", "hfstats1", "hfstats2", "lfstats1", "lfstats2", "cferr1", "cferr2"]
  interp_pars += ["UnclMET", "PileUp", "pdf", "PUJetIdTag", "MER"]
  interp_pars += ["JER_eta0_193", "JER_eta193_25", "JER_eta25_3_p0_50", "JER_eta25_3_p50_Inf", "JER_eta3_5_p0_50", "JER_eta3_5_p50_Inf"]
  interp_pars += ["JEC_eta0_25", "JEC_eta25_5"]
  interp_pars += ["LepId", "LepTrig", "LepIso"]
  ren_pars     = ["Fac", "Ren", "RenFac"]
  interp_pars += ren_pars
  xsr_pars     = ["Isr", "Fsr"]
  interp_pars += xsr_pars

  pss = ["_G2GG_muR_", "_G2QQ_muR_", "_Q2QG_muR_", "_X2XG_muR_", "_G2GG_cNS_", "_G2QQ_cNS_", "_Q2QG_cNS_", "_X2XG_cNS_"]
  pss = []
  pss_names = []

  for item in ["isr", "fsr"] :
    for ps in pss:
      interp_pars += [ item + ps ]
      pss_names += [ item + ps ]

  has_red = ["s_ch", "t_ch", "ttbar", "WQQ", "Wb", "Wc", "Wother", "DY"]
  has_xsr = ["s_ch", "t_ch", "ttbar", "tW_ch"]
  
  has_red = ["s_ch", "ttbar", "WQQ", "Wb", "Wc", "Wother", "DY"]
  has_xsr = []

  datacard.parameters_order_list  = [ "sigma_" + name for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ) ] 
  datacard.parameters_order_list += mult_pars + interp_pars

  # define common mult parameters
  common_mult_pars = []
  for name, err in zip(mult_pars, mult_errs) :
    parameter = atd.Parameter( name, "log_normal", "mult")
    parameter.options["mean"]    =  0.0
    parameter.options["width"]   =  err
    parameter.options["range"]   =  '(-2.5,2.5)'
    common_mult_pars += [ parameter ]

  # define common interp parameters
  common_interp_pars = []
  for name in interp_pars :
    parameter = atd.Parameter( name, "gauss", "shape")
    common_interp_pars += [ parameter ]

  hdamp_par  = atd.Parameter( "hdamp", "gauss", "shape")
  UETune_par = atd.Parameter( "UETune", "gauss", "shape")
  with_hdamp, with_UETune  = [], []
  # with_hdamp  = ["ttbar", "tW_ch"]
  # with_UETune = ["ttbar", "tW_ch"]

  # define chanals
  for name, err, rang in zip( chanals_names[::3], chanals_names[1::3], chanals_names[2::3] ):
    chanal        = atd.Chanal( name )
    chanal.parameters = copy.copy(common_mult_pars)

    interp_pars = []
    if name in with_hdamp  : interp_pars += [ hdamp_par  ]
    if name in with_UETune : interp_pars += [ UETune_par ]

    for param in common_interp_pars:
      if param.name in ren_pars  and name not in has_red : continue
      if param.name in xsr_pars  and name not in has_xsr : continue
      if param.name in pss_names : continue
      interp_pars += [ param ]

    if name == "fcnc_tcg" :
      norm_parameter = atd.Parameter("KC", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
      chanal.used_in_toydata = False
    elif name == "fcnc_tug" :
      norm_parameter = atd.Parameter("KU", "flat_distribution", "mult")
      norm_parameter.options["mean"]  = 0.0
      norm_parameter.options["range"] = '(0.0,4.0)'
      chanal.parameters += [ norm_parameter, norm_parameter ]
      chanal.used_in_toydata = False
    else :
      norm_parameter = atd.Parameter( "sigma_" + name, "log_normal", "mult")
      norm_parameter.options["mean"]    =  0.0
      norm_parameter.options["width"]   =  err
      norm_parameter.options["range"] = '(-5.0,5.0)'
      chanal.parameters += [ norm_parameter ]

    if name != "QCD" : chanal.parameters += common_interp_pars
    datacard.chanals += [ chanal ]

  return datacard

def FcncTugModel(args):
  datacard = fcnc_1d(args, "fcnc_tug")
  datacard.name = "FcncTugModel"
  return datacard

def FcncTcgModel(args):
  datacard = fcnc_1d(args, "fcnc_tcg")
  datacard.name = "FcncTcgModel"
  return datacard

def expected_sm(args):
  datacard = sm(args)
  datacard.model_name_toy    = "expected_data"

  datacard.input_file_mc   = args.input
  datacard.input_file_data = args.input_data
  datacard.mcmc_iters      = args.niters
  datacard.mcmc_chains     = args.nchains
  datacard.dice_poisson    = False
  datacard.dice_systematic = False
  datacard.azimov          = True
  datacard.enable_barlow_beston = True
  
  params = []
  for chanal in datacard.chanals: params += chanal.parameters
  params = list(set(params))
  
  return datacard
    
def sys_impact(args):
  ### PARCE RESULTS
  dic = {}
  file = open("../../sm/getTable_SM.tex")
  data = file.read()
  print data
  for line in data.split("\n"):
    # sigma\_t\_ch & 0.897 & 0.944 & 0.993 \\
    if line.count("&") != 3 : continue
    if line.count("central") != 0 : continue
    parts = line.split("&")
    name = parts[0].replace("\\", "").strip()
    print parts
    dic[ name ] = [float(parts[1]), float(parts[3].split()[0])]
  ### #
  print dic
  def get_posterior_median_width(name):
    return dic[ name ][0], dic[ name ][1]
  
  datacard = expected_sm(args)

  dcard_exp  = copy.deepcopy(datacard)
  dcard_exp.name = "expected_"+dcard_exp.name + ""
  dcard_exp.save( args.mode.split(" ") )

  datacard.input_file_mc   = args.input
  datacard.input_file_data = args.input_data
  datacard.mcmc_iters      = args.niters
  datacard.enable_barlow_beston = False
  datacard.seed            = 0
  datacard.dice_poisson    = False
  datacard.dice_systematic = False
  datacard.azimov          = True
  datacard.mcmc_chains     = args.nchains

  unmarges = [ ["ttbar", "colourFlipUp"],["ttbar", "erdOnUp"],["ttbar", "QCDbasedUp"] ]
  for chname, sys in unmarges:
    dcard = copy.deepcopy(datacard)
    dcard.name = "expected_" + dcard.name + "_" + sys

    for chanal in dcard.chanals:
      if chanal.name != chname : continue
      chanal.alt_hist_name = chname + "_" + sys

    dcard.save( args.mode.split(" ") )
  
  params = []
  for chanal in datacard.chanals: params += chanal.parameters
  params = list(set(params))

  for param in params:
    print param.name, "<----------------------------"
    dcard = copy.deepcopy(datacard)
    dcard.name = dcard.name + "_" + param.name
    assepted_pars = ["cta_norm", "uta_norm"]
    if param.name in assepted_pars: continue

    pars = []
    for chanal in dcard.chanals: pars += chanal.parameters
    pars = list(set(pars))

    for par in pars:
      if par.name != param.name : continue

      par_mean  = par.options.get("mean", None)
      par_width = par.options.get("width", None)

      par_mean  = 1.0
      if par.type == "shape" : 
        par_mean  = 0.0
        par_width = 1.0
        par = atd.Parameter( par.name, "delta_distribution", "shape" )
      else :       par = atd.Parameter( par.name, "delta_distribution", "mult" )
  
      par_left, par_right  = get_posterior_median_width( par.name )
      print par_left, par_right


      for chanal in dcard.chanals: 
        for i in xrange(len(chanal.parameters)):
          if chanal.parameters[i].name == par.name : 
            chanal.parameters[i] = par
      
      par.options["mean"]  = par_left
      dcard_minus = copy.deepcopy(dcard)
      dcard_minus.name = "expected_"+dcard.name + "_minus"
      dcard_minus.save( args.mode.split(" ") )

      par.options["mean"]  = par_right
      dcard_plus  = copy.deepcopy(dcard)
      dcard_plus.name = "expected_"+dcard.name + "_plus"
      dcard_plus.save( args.mode.split(" ") )

      if par.type == "shape" : pass
      else : pass 

    for chanal in dcard.chanals:
      print chanal.name
      print [p.name for p in chanal.parameters]
      print [p.options for p in chanal.parameters]
  

  return None;


if __name__ == "__main__": 
  import argparse
  parser = argparse.ArgumentParser(description='Produce some theta .cfg datacards ... ')
  parser.add_argument('--fcnc_fix',   dest='fcnc_fix',   type=str,   default='', help='')
  parser.add_argument('--fname',      dest='fname',      type=str,   default='test', help='predifined function name to call')
  parser.add_argument('--nbins',      dest='nbins',      type=int,   default=10,     help='number of bins in histograms')
  parser.add_argument('--niters',     dest='niters',     type=int,   default=50000,     help='number of iters')
  parser.add_argument('--input',      dest='input',      type=str,   default='input.root', help='root input file name')
  parser.add_argument('--input_data', dest='input_data', type=str,   default='', help='root input toys file name')
  parser.add_argument('--input_unmarg', dest='input_unmarg', type=str,   default='', help='root input unmarginal systematic file name')
  parser.add_argument('--mode',       dest='mode',       type=str,   default="latex cl theta", help='')
  parser.add_argument('--signal_norm',dest='signal_norm',type=float, default=1.0, help='')
  parser.add_argument('--nchains',dest='nchains',type=int, default=1, help='')
  args = parser.parse_args()

  print "create_card.py call ..."
  flist = [f for f in globals().values() if (type(f) == type(atd.test) and f.__name__ == args.fname) ]

  if flist : 
    print "create_card.py will use ", args.fname, " -> ", flist, "with parameters ", args.input, args
    datacard = flist[0](args)

    # set some options
    if datacard: 
      datacard.input_file_mc   = args.input
      datacard.input_file_data = args.input_data
      datacard.mcmc_iters      = args.niters
      datacard.mcmc_chains     = args.nchains
      datacard.dice_poisson    = False
      datacard.dice_systematic = False
      datacard.azimov          = True
      datacard.enable_barlow_beston = True

      datacard.save( args.mode.split(" ") )
  else     : atd.test()























