#   Copyright (c) 2019, Xilinx, Inc.
#   All rights reserved.
# 
#   Redistribution and use in source and binary forms, with or without 
#   modification, are permitted provided that the following conditions are met:
#
#   1.  Redistributions of source code must retain the above copyright notice, 
#       this list of conditions and the following disclaimer.
#
#   2.  Redistributions in binary form must reproduce the above copyright 
#       notice, this list of conditions and the following disclaimer in the 
#       documentation and/or other materials provided with the distribution.
#
#   3.  Neither the name of the copyright holder nor the names of its 
#       contributors may be used to endorse or promote products derived from 
#       this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
#   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  
import numpy as np
import os
import sys
import random 
import subprocess

outFileWeights = open("memdata.h" , "wt")
outFileConfig = open("config.h" , "wt")
outFileWeightsFull = open("../../proj/sim/weights_mem_full.memh","wt")

kernel_dim = 2
stride = 1
input_precision = 4
ifm_channels = 4
ofm_channels = 3
ifm_dimension = 4
ofm_dimension = (ifm_dimension-kernel_dim)/stride+1

activation_precision = 4
expand = 1
simd = 4
pe = 3
w_precision = 1
mmv=1

tile = ifm_channels *kernel_dim*kernel_dim * ofm_channels // (simd*pe)

outFileConfig.write("#define KERNEL_DIM %d \n" % kernel_dim)
outFileConfig.write("#define SIMD1 %d \n" % simd)
outFileConfig.write("#define PE1 %d \n" % pe)
outFileConfig.write("#define MMV1 %d \n" % mmv)
outFileConfig.write("#define WIDTH %d \n" % w_precision)

outFileConfig.write("#define IFM_Channels1 %d \n" % ifm_channels)
outFileConfig.write("#define OFM_Channels1 %d \n" % ofm_channels)
outFileConfig.write("#define IFMDim1 %d \n" % ifm_dimension)
outFileConfig.write("#define OFMDim1 %d \n" % ofm_dimension)
outFileConfig.write("#define STRIDE %d \n" % stride)
outFileConfig.write("#define INPUT_PRECISION %d \n" % input_precision)
outFileConfig.write("#define TILE1 %d \n" % tile)

outFileConfig.write("#define ACTIVATION_PRECISION %d \n" % activation_precision)

outFileConfig.close()
outFileWeights.write("#ifndef PARAMS_HPP\n")
outFileWeights.write("#define PARAMS_HPP\n")

outFileWeights.write("namespace PARAM{ \n")
if (w_precision == 1):
		outFileWeights.write("static BinaryWeights<%d,%d,%d> weights= {\n{\n" %(simd,pe,tile))
else:
		outFileWeights.write("static FixedPointWeights<%d,ap_int<%d>,%d,%d> weights= {\n{\n" %(simd,w_precision,pe,tile))

weights_dict = dict()
for p in range(pe):
		weights_list = []
		dict_key = p
		fname_src = "../../proj/src/mvau_top/weight_mem"+str(p)+".memh"
		fname_sim = "../../proj/sim/weight_mem"+str(p)+".memh"
		outFileWeightsHexSrc = open(fname_src,"wt")
		outFileWeightsHexSim = open(fname_sim,"wt")		
		outFileWeights.write("{ \n")
		for t in range(tile):
				width = simd*w_precision;
				val = random.randint(0, 1<<width-1)
				outFileWeights.write("%s" % hex(val))
				formatted_string = format(val,"x")
				outFileWeightsHexSrc.write("%s" % formatted_string)
				outFileWeightsHexSim.write("%s" % formatted_string)
				weights_list.append(formatted_string)		
				if t!=tile-1:
						outFileWeights.write(",\n")
						outFileWeightsHexSrc.write("\n")
						outFileWeightsHexSim.write("\n")
		outFileWeights.write("} \n")
		if p!=pe-1:
				outFileWeights.write(",")
		weights_dict[dict_key] = weights_list
		outFileWeightsHexSrc.close()
		outFileWeightsHexSim.close()	

outFileWeights.write("}\n};\n } \n")
outFileWeights.write("#endif \n")
outFileWeights.close()

nf = ofm_channels/pe
matrix_w = kernel_dim*kernel_dim*ifm_channels // (simd)

for n in np.arange(nf):
		for p in weights_dict:
				r = int(len(weights_dict[p])/nf)
				for wt in np.arange(r):
						k = int(wt+matrix_w*n)
						outFileWeightsFull.write("%s\n" % weights_dict[p][k])
				
outFileWeightsFull.close()


