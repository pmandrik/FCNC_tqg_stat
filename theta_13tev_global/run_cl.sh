
srcdir=$(dirname $0)
workdir=$(pwd)

cd $srcdir/CMSSW_8_1_0/src
eval `scram runtime -sh`
cd $workdir

echo "run_cl with command \""$*"\""
time $*
