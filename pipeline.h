#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <vector>
#include <string.h>

using namespace std;

/*
ROB_SIZE(number of entries in the ROB), IQ_SIZE(Number of entries in IQ), WIDTH(number of pipelined function units) are defined by the simulator
**Consider creating a struct titled instruction(this will be used to store )

Operation Types:
0: latency of 1 cycle
1: latency of 2 cycles
2: latency of 5 cycles

pipeline registers:
DEcode: 
    - size of pipeline = WIDTH
    - only perform if DE contains a decode bundle and RN is empty
        - advance bundle to RN 
ReName: size of pipeline = WIDTH
    - perform if RR is empty or Rob has free entries to accept rename bundle
        - allocate entry in ROB for instruction
        - rename source registers
        - rename destination reg(if it has one)
Register Read: size of pipeline = WIDTH
    - if DI is empty, process RR bundle and advance it to DI
    - see if renamed source operands are ready
    - **Ensure that wakeup call is sent to IQ, RR and another stage
DIspatch: size = WIDTH
    - if dispatch bundle present:
        - if number of free IQ entries >= size of dispatch bundle
            - dispatch all instructions from DI to IQ
Issue: IQ_SIZE
    - issue the up to (WIDTH) oldest instructions from IQ
    - could potentially use linked list or vector to implement
    - issue work:
        - remove instruction from IQ
        - add to execute_list(set time based on instruction type)
Execute: size = WIDTH*5
    - check for instructions that are finishing this cycle
    - remove from the list
    - add to WB
    - wakeup dependent instructions(i.e. set source reg ready flags) in IQ, DI, and RR
Writeback: size = WIDTH*5
    - process writeback bundle
    - for each instruction in writeback, mark ready in the ROB
Retire: size = ROB_SIZE
    - retire up to the WIDTH ready instructions from head of ROB
*/