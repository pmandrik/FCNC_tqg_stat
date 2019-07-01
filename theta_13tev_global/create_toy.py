
import sys
import ROOT as root

def get_interpolation(hist_nom, hist_up, hist_down, coff):
  hist = hist_nom.Clone()
  for nbin in xrange(1, hist_nom.GetNBinsX()):
    pass
  return

def make_toy(nom_hists, args):
  input = root.TFile.Open(args.input)
  
  toy_hist = None
  for hist_name, coff in nom_hists.iteritems():
    hist = input.Get( hist_name );
    print hist
    hist.Scale( coff )
    if not toy_hist:
      toy_hist = hist.Clone()
      continue
    toy_hist += hist

  toy_hist.SetName("data")
  toy_hist.SetTitle("data")
  print toy_hist

  input = root.TFile(args.output, "RECREATE")
  toy_hist.Write()

def simple(args):
  # toy data = \sum mc_templates

  hists_x_coff = {
    "t_ch"   : 1.,
    "s_ch"   : 1.,
    "tW_ch"  : 1.,
    "ttbar"  : 1.,
    "Diboson": 1.,
    "DY"     : 1.,
    "WQQ"    : 1.,
    "Wc"     : 1.,
    "Wb"     : 1.,
    "Wother" : 1.,
    "QCD"    : 1.,
  }

  make_toy(hists_x_coff, args)

def main():
  import argparse
  parser = argparse.ArgumentParser(description='Produce some theta .cfg datacards ... ')
  parser.add_argument('--input',  dest='input',    type=str, default='input.root', help='root input file name')
  parser.add_argument('--output', dest='output',   type=str, default='output.root', help='root output file name')
  parser.add_argument('--fname',  dest='fname',    type=str, default='test', help='predifined function name to call')
  args = parser.parse_args()

  print "create_toy.py call ..."
  flist = [f for f in globals().values() if (type(f) == type(main) and f.__name__ == args.fname) ]

  if flist : 
    print "create_toy.py will use ", args.fname, " -> ", flist, "with parameters ", args.input, args
    flist[0](args)
  else : 
    print "create_toy.py cant find related function, exit ... "
    return 1
  return 0

# --nbins=$NBINS --niters=$NITERS --input="hists_SM.root" --output="toy_hists_SM.root" --QCD_mid=$f_QCD_mid --fname="simple"
if __name__ == "__main__": main()



