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
Fetch:
    - execute if DE is empty and more instructions to be handled
    - can fetch up to WIDTH instructions
DEcode: 
    - size of pipeline = WIDTH
    - only perform if DE contains a decode bundle and RN is empty
        - advance bundle to RN 
ReName: size of pipeline = WIDTH
    - perform if RR is empty and Rob has free entries to accept rename bundle
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

//struct that will be used to create instruction variables, allowing them to be tracked throughout the pipeline
typedef struct Instruction{
    //instruction information
    int pc, op, destr, src1, src2, valid, rdy1 = 1, rdy2 = 1;

    //Instruction pipeline timing information
    int seq, FE, DE, RN, RR, DI, IS, EX, WB, RT;

    //instruction rename info
    int src1_tag = -1, src2_tag = -1, destr_tag = -1;

} Instruction;

typedef struct Reorder_Buffer{
    //reorder buffer instruction variables
    int value, dst, rdy, exc, mis, pc;
    Instruction inst;

} Reorder_Buffer;

typedef struct Rename_Map_Table{
    int valid, tag = -1;
} Rename_Map_Table;

//struct that will be used to create instruction variables, allowing them to be tracked throughout the pipeline

vector<Instruction> DE_stage;
vector<Instruction> RN_stage;
vector<Instruction> RR_stage;
vector<Instruction> DI_stage;
vector<Instruction> EX_stage;
vector<Instruction> WB_stage;
vector<int> ARF;
vector<Rename_Map_Table> RMT;
vector<Reorder_Buffer> ROB;
vector<Instruction> IQ;
vector<Instruction> final_list;




//important variables
    int width = 0;
    //ROB
    int ROB_size = 0;
    int ROB_head = 0;                  //used to track current position in ROB(circular buffer style)
    int ROB_tail = 0;                  //used to throw instructions into this index of ROB
    int total_in_ROB = 0;              //used for keeping track of how many items are currently in the ROB


    //IQ
    int IQ_size = 0;
    int IQ_head = 0;                   //this will be used to keep track of the oldest element in the IQ
    int IQ_tail = 0;                   //used to insert items into this index of the IQ
    int total_in_IQ = 0;               //used to keep track of how many items are currently in the IQ




