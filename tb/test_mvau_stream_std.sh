#!/bin/bash
# Copyright © 2020 Syed Asad Alam. All rights reserved.
# Running the test_mvau_stream_std.tcl using the following steps:
# 1) Remove old project directory
# 2) Generated weights
# 3) Run vivado_hls simulation
# 4) Copy the dumped files to RTL simulation folder
# 5) For weight files, only copy the firofm_channels)/(pe*simd)

WORKING_DIR=hls-syn-mvau-stream-std
if [ -d ${WORKING_DIR} ]; then
    echo "Removing project folder";
    rm -Rf ${WORKING_DIR};
else
    echo "Project folder does not exist";
fi
echo "Generating weights"
python gen_weigths.py
#--ifm_ch ${1} --ifm_dim ${2} --ofm_ch ${3} --kdim ${4} --inp_wl ${5} --wgt_wl ${6} --out_wl ${7} --simd ${8} --pe ${9}
echo "Running HLS synthesis simulation"
vivado_hls test_mvau_stream_std.tcl
echo "Copying dumped data"
cp hls-syn-mvau-stream-std/sol1/csim/build/{inp_act.memh,inp_wgt.memh,out_act.memh} ../../proj/sim/
echo "Running behavorial simulation of RTL"
cd ../../proj/sim
bash mvau_stream_test_v3.sh
if grep -q "Data MisMatch" xsim.log; then
    echo "RTL simulation failed"
    exit 0
elif grep -q "failed" xsim.log; then
    echo "RTL simulation failed"
    exit 0
else
    echo "RTL simulation successful"
fi
echo "Synthesizing MVAU Stream RTL"
cd ../syn
vivado -mode batch -source mvau_stream_synth.tcl
echo "Analysing results and Extracting performance data"
python extract_synthesis_data.py -o mvau_stream_std_results.csv
cd $FINN_HLS_ROOT/tb

exit 1
