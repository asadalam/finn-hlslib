##############################################################################
 #  Copyright (c) 2019, Xilinx, Inc.
 #  All rights reserved.
 #
 #  Redistribution and use in source and binary forms, with or without
 #  modification, are permitted provided that the following conditions are met:
 #
 #  1.  Redistributions of source code must retain the above copyright notice,
 #     this list of conditions and the following disclaimer.
 #
 #  2.  Redistributions in binary form must reproduce the above copyright
 #      notice, this list of conditions and the following disclaimer in the
 #      documentation and/or other materials provided with the distribution.
 #
 #  3.  Neither the name of the copyright holder nor the names of its
 #      contributors may be used to endorse or promote products derived from
 #      this software without specific prior written permission.
 #
 #  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 #  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 #  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 #  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 #  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 #  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 #  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 #  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 #  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 #  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 #  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #
###############################################################################
###############################################################################
 #
 #  Authors: Syed Asad Alam <syed.asad.alam@tcd.ie>
 #
 # \file test_mvau_stream_binwgt.tcl
 #
 # Tcl script for HLS csim, synthesis and cosim of the matrix vector activation
 # streaming unit using 1-bit weights and input activation
 #
###############################################################################
open_project hls-syn-mvau-stream-binwgt
add_files mvau_stream_top_binwgt.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
add_files -tb mvau_stream_tb_binwgt.cpp -cflags "-std=c++0x -I$::env(FINN_HLS_ROOT) -I$::env(FINN_HLS_ROOT)/tb" 
set_top Testbench_mvau_stream_binwgt
open_solution sol1
set_part {xczu3eg-sbva484-1-i}
create_clock -period 5 -name default
## C-simulation
csim_design
## Synthesizing HLS and finding synthesis execution time
# set t0 [clock clicks -milliseconds]
# csynth_design
# set t1 [expr {([clock clicks -milliseconds] - $t0)/1000.}]
# ## Co-simulation (C+HLS generated RTL)
# cosim_design
# ## Exporting design for analysis
# set t2 [clock clicks -milliseconds]
# export_design -flow syn -rtl verilog -format ip_catalog
# set t3 [expr {([clock clicks -milliseconds] - $t2)/1000.}]
# set t4 [expr {$t1 + $t3}]
# set outfile [open "hls_exec.rpt" w]
# puts $outfile $t4
# close $outfile
exit
