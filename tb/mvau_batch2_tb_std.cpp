/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
/******************************************************************************
 *
 *  Authors: Syed Asad Alam <syed.asad.alam@tcd.ie>
 *           Giulio Gambardella <giuliog@xilinx.com>
 *
 *  \file mvau_tb_std.cpp
 *
 *  Testbench for the Matrix Vector Activation Batch Unit HLS block
 *
 *****************************************************************************/
#define AP_INT_MAX_W 16384
#include <iostream>
#include <fstream>
#include <time.h>
#include <cmath>
#include <ctime>
#include <cstring>
#include <hls_stream.h>
#include <cstdlib>
#include "ap_int.h"
#include "weights.hpp"
#include "bnn-library.h"
#include "memdata.h"
#include "config.h"
#include "activations.hpp"
#include "weights.hpp"
#include "activations.hpp"
#include "interpret.hpp"
#include "mvau.hpp"
#include "conv.hpp"


using namespace hls;
using namespace std;

#define NUM_IMAGES 1
#define NUM_EXECUTIONS 2
#define MAX_IMAGES 2 // NUM_IMAGES*NUM_EXECUTIONS
void Testbench_mvau_batch2_std(stream<ap_inp<SIMD1*INPUT_PRECISION> > & in,
			       stream<ap_out<PE1*ACTIVATION_PRECISION> > & out);
//			unsigned int numReps);

int main()
{
  
  static	ap_wgt<WIDTH> W1[OFM_Channels1][KERNEL_DIM][KERNEL_DIM][IFM_Channels1];
  
  static	ap_inp<INPUT_PRECISION> IMAGE[MAX_IMAGES][IFMDim1*IFMDim1][IFM_Channels1];
  static	ap_out<ACTIVATION_PRECISION> TEST[MAX_IMAGES][OFMDim1][OFMDim1][OFM_Channels1];
  stream<ap_inp<IFM_Channels1*INPUT_PRECISION> > input_stream("input_stream");
  stream<ap_out<OFM_Channels1*ACTIVATION_PRECISION> > output_stream("output_stream");
  unsigned int counter = 0;

    
  for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
    for (unsigned int oy = 0; oy < IFMDim1; oy++) {
      for (unsigned int ox = 0; ox < IFMDim1; ox++) {
	ap_inp<INPUT_PRECISION*IFM_Channels1> input_channel = 0;
	for(unsigned int channel = 0; channel < IFM_Channels1; channel++)
	  {
	    //counter = rand();
	    ap_inp<INPUT_PRECISION> input = (ap_inp<INPUT_PRECISION>)(counter);
	    IMAGE[n_image][oy*IFMDim1+ox][channel]= input;
	    input_channel = input_channel >> INPUT_PRECISION;
	    input_channel(IFM_Channels1*INPUT_PRECISION-1,(IFM_Channels1-1)*INPUT_PRECISION)=input;
	    counter++;
	  }
	input_stream.write(input_channel);
      }
    }
  }
  
  // initialize the weights
  constexpr int TX = (IFM_Channels1*KERNEL_DIM*KERNEL_DIM) / SIMD1;
  constexpr int TY = OFM_Channels1 / PE1;
  unsigned int kx=0;
  unsigned int ky=0;
  unsigned int chan_count=0;
  unsigned int out_chan_count=0;

  for (unsigned int oy = 0; oy < TY; oy++) {
    for(unsigned int pe=0;pe <PE1;pe++){
      for (unsigned int ox = 0; ox <TX; ox++) {
	for(unsigned int simd=0;simd<SIMD1;simd++){
	  W1[out_chan_count][kx][ky][chan_count] = PARAM::weights.weights(oy*TX + ox)[pe][simd];
	  //cout << "TILE " << oy*TX + ox << " PE " << pe << " SIMD " << simd << endl;
	  //cout << "IFM " << chan_count << " KX " << kx << " KY " << ky << " OFM " << out_chan_count << endl;
	  chan_count++;
	  if (chan_count==IFM_Channels1){
	    chan_count=0;
	    kx++;
	    if (kx==KERNEL_DIM){
	      kx=0;
	      ky++;
	      if (ky==KERNEL_DIM){
		ky=0;
		out_chan_count++;
		if (out_chan_count==OFM_Channels1){
		  out_chan_count=0;
		}
	      }
	    }
	  }
	}
      }
    }
  }

  // Preparing the inputs for the test bench
  unsigned const MatrixW = KERNEL_DIM * KERNEL_DIM * IFM_Channels1;
  unsigned const MatrixH = OFM_Channels1;
  unsigned const InpPerImage = IFMDim1*IFMDim1;

  hls::stream<ap_inp<SIMD1*INPUT_PRECISION> > wa_in("StreamingConvLayer_Batch.wa_in");
  hls::stream<ap_inp<SIMD1*INPUT_PRECISION> > convInp("StreamingConvLayer_Batch.convInp");
  hls::stream<ap_out<PE1*ACTIVATION_PRECISION> > mvOut("StreamingConvLayer_Batch.mvOut");

  StreamingDataWidthConverter_Batch<IFM_Channels1*INPUT_PRECISION, SIMD1*INPUT_PRECISION, InpPerImage>
    (input_stream, wa_in, MAX_IMAGES);

  // ConvolutionInputGenerator<KERNEL_DIM, IFM_Channels1, INPUT_PRECISION, IFMDim1, OFMDim1, SIMD1, 1>
  //   (wa_in, convInp, MAX_IMAGES, ap_resource_dflt());

  // Dumping the input activation stream
  //logStringStream<SIMD1*INPUT_PRECISION>("inp_act.mem",convInp);
  logStringStream<SIMD1*INPUT_PRECISION>("inp_act.mem",wa_in);
  
  // Performing Behavioral Convolution
  conv<MAX_IMAGES,IFMDim1,OFMDim1,IFM_Channels1,OFM_Channels1, KERNEL_DIM, 1, ap_inp<INPUT_PRECISION> >(IMAGE, W1, TEST);

  // Calling the HLS test bench twice to populate the II report
  for(int i = 0; i < NUM_EXECUTIONS; i++) {
    //Testbench_mvau_std(convInp, mvOut);//, MAX_IMAGES);
    Testbench_mvau_batch2_std(wa_in, mvOut);//, MAX_IMAGES);
  }
  
  // Converting the output stream
  StreamingDataWidthConverter_Batch<PE1*ACTIVATION_PRECISION, OFM_Channels1*ACTIVATION_PRECISION,
				    OFMDim1 * OFMDim1 * (OFM_Channels1 / PE1)>(mvOut, output_stream, MAX_IMAGES);

  // File initialization for dumping output activation
  std::ofstream OutAct_File;
  string out_act_fname = "out_act.mem";
  OutAct_File.open(out_act_fname.c_str());
  int err_counter = 0, err_perimage=0;
  ap_out<ACTIVATION_PRECISION> out_chan;
  for (unsigned int n_image = 0; n_image < MAX_IMAGES; n_image++) {
    for (unsigned int oy = 0; oy < OFMDim1; oy++) {
      for (unsigned int ox = 0; ox < OFMDim1; ox++) {
	for(int e=0;e<1;e++){
	  ap_out<OFM_Channels1*ACTIVATION_PRECISION> outElem = output_stream.read();
	  for(unsigned int channel = 0; channel < OFM_Channels1; channel++){
	    ap_out<ACTIVATION_PRECISION> EXP = TEST[n_image][ox][oy][channel + e * OFM_Channels1];
	    out_chan(ACTIVATION_PRECISION-1,0) = outElem((channel + 1)*ACTIVATION_PRECISION-1,channel*ACTIVATION_PRECISION);

	    if (EXP != out_chan){
	      std::cout << "ERROR: Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;
	      err_counter ++;
	      err_perimage++;
	    }else{
	      std::cout << "Expected["<<oy <<"]["<<ox<<"]["<<channel<<"]=" << EXP << " actual " <<  out_chan << std::endl;
	      // Logging HLS output to file
	      OutAct_File << hex << (unsigned long long)out_chan << "\n";
	    }
	  }
	}
      }
    }
    if(err_perimage == 0){
      std::cout << "Image # " << n_image << " passed the testing."<< std::endl;
    }
    else{
      err_perimage=0;
      std::cout << "Image # " << n_image << " failed the testing."<< std::endl;
    }
  }
  if(err_counter == 0){
    return 0;
  }
  else{
    return 1;
  }

}


