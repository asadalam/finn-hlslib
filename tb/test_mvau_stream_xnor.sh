#!/bin/bash
# Copyright © 2020 Syed Asad Alam. All rights reserved.
# Running the test_mvau_stream_xnor.tcl using the following steps:
# 1) Remove old project directory
# 2) Generated weights
# 3) Run vivado_hls simulation
# 4) Copy the dumped files to RTL simulation folder

WORKING_DIR=hls-syn-mvau-stream-xnor
if [ -d ${WORKING_DIR} ]; then
    echo "Removing project folder";
    rm -Rf ${WORKING_DIR};
else
    echo "Project folder does not exist";
fi
echo "Generating weights"
### Default arguments for gen_weights_fn.py
ifm_ch=${1:-4}
ifm_dim=${2:-4}
ofm_ch=${3:-4}
kdim=${4:-2}
inp_wl=${5:-8}
wgt_wl=${6:-1}
out_wl=${7:-16}
simd=${8:-2}
pe=${9:-2}
python gen_weights_fn.py --ifm_ch ${ifm_ch} --ifm_dim ${ifm_dim} --ofm_ch ${ofm_ch} --kdim ${kdim} --inp_wl ${inp_wl} --wgt_wl ${wgt_wl} --out_wl ${out_wl} --simd ${simd} --pe ${pe}
echo "Running HLS simulation"
vivado_hls test_mvau_stream_xnor.tcl
echo "Copying dumped data"
cp hls-syn-mvau-stream-xnor/sol1/csim/build/{inp_act.memh,inp_wgt.memh,out_act.memh} ${MVAU_RTL_ROOT}/proj/sim/
exit 1
