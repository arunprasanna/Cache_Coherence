Disclaimer: I am not responsible for honor code violations as a result of this public repository.
# Cache_Coherence
A simulator that maintains coherent states implementing different bus based coherence protocols for a 4, 8 or 16 processor system.
The protocols folder contains all the modified protocols that have been implemented.
To run type: make and hit enter.
To run a particular protocol on a particular system type:
./sim_trace  -t trace_directory  -p protocol

Example:
./sim_trace -t traces/16proc_validation -p MOESIF

