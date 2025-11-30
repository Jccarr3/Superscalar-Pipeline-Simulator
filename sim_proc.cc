#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
#include "pipeline.h"
#include <algorithm>

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/

int current_cycle = 0;
int global_seq = 0; 
int dic = 0;
int pl_status = 0;
int trace_status = 1;
int total_in_ROB = 0;              //used for keeping track of how many items are currently in the ROB
int total_in_IQ = 0;

int ROB_head = 0;                  //used to track current position in ROB(circular buffer style)
int ROB_tail = 0;                  //used to throw instructions into this index of ROB

int test_break = 0;
int test_count = 0;





int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    //int op_type, dest, src1, src2;  // Variables are read from trace file
    //uint64_t pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
    printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

    //pipeline instantiated
    //variables for instruction timing(updated in sim.cc)

    width = params.width;
    IQ_size = params.iq_size;
    ROB_size = params.rob_size;
    
    //variables for ROB management


    //define all stages of the pipeline based on the necessary size

    DE_stage.resize(width);
    RN_stage.resize(width);
    RR_stage.resize(width);
    DI_stage.resize(width);
    EX_stage.resize(width*5);
    WB_stage.resize(width*5);
    IQ.resize(IQ_size);
    ROB.resize(params.rob_size);

    ARF.resize(67, 1);
    RMT.resize(67);


    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
    //     printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    //fetch section
    while(advance_cycle()){
        if(current_cycle > 250) break;
        test_count++;
        
        //if(test_count % 50 == 0) break;
        retire();
        writeback();
        execute();
        issue();
        dispatch();
        regread();
        rename();
        decode();
        if(DE_stage[0].valid == 0){                     //if nothing in DE
            for(int i = 0; i < width; i++){
                DE_stage[i].valid = 0;
            }
            for(int i = 0; i < width; i++){
                if(fscanf(FP, "%lx %d %d %d %d", &DE_stage[i].pc, &DE_stage[i].op, &DE_stage[i].destr, &DE_stage[i].src1, &DE_stage[i].src2) != EOF){   //read file line
                    DE_stage[i].valid = 1;                  //set stage as valid
                    DE_stage[i].DE = current_cycle;         //set what cycle it enters the decode stage
                    DE_stage[i].seq = global_seq;           //store sequence number
                    global_seq++;                           //increment sequence counter
                    trace_status = 1;                                     //set trace empty flag if empty
                }  

                else{
                    trace_status = 0;
                    break;
                }                        
            }

        }  
    }
    //print_final();

    return 0;
}


//fetch code

//Decode function
void decode(){
    if(DE_stage[0].valid == 1 && RN_stage[0].valid == 0){       //if decode bundle contains something and rename bundle is empty
        for(int i = 0; i < width; i++){
            RN_stage[i] = DE_stage[i];              //move instruction from DE to RN
            RN_stage[i].RN = current_cycle;         //set cycle where instruction enters RN stage
            DE_stage[i].valid = 0;                  //clear decode stage
        }
    }
    return;
}
//Decode function^^^^^^

//rename function
void rename(){
        //check if ROB has free entries
    if((ROB_size - total_in_ROB) < width) {
        if(current_cycle < 200){
            //printf("ROB ERROR: %d", total_in_ROB);
        }
        return;                      //if dispatch stage is full, do nothing
    }
        //check if RR is empty 
    if(RR_stage[0].valid != 0 || RN_stage[0].valid == 0)  return;                         //do nothing if the regread stage is occupied or rename stage is empty

        
    for(int i = 0; i < width; i++){
        if (RN_stage[i].valid == 0){            //only runs if at end of trace and not moving full bundle
            continue;
        }
        total_in_ROB++;                         //increase the number of items in ROB
        //adds instruction to ROB 
        if(RN_stage[i].destr != -1){                       //if the instruction has a destination reg
            ROB[ROB_tail].dst = RN_stage[i].destr;         //set destination register in ROB
        }
        else{
            ROB[ROB_tail].dst = -1;                        //set ROB destination register as null
        }
        ROB[ROB_tail].rdy = 0;                             //set ROB ready bit as 0
        ROB[ROB_tail].pc = RN_stage[i].pc;                 //match the program counter(used later for setting items in the ROB as ready)
        ROB[ROB_tail].inst = RN_stage[i];                  //store entire instruction information 
        //adds instruction to ROB^^^^^

        //rename source registers
        if(RN_stage[i].src1 != -1){
            if(RMT[RN_stage[i].src1].valid == 1){        //source register 1 has been renamed(check RMT)
                RN_stage[i].src1_tag = RMT[RN_stage[i].src1].tag;       //rename instruction src reg to name from RMT
            }
        }
        
        if(RN_stage[i].src2 != -1){             //checks if there is a second source register
            if((RMT[RN_stage[i].src2].valid == 1)){        //source register 2 has been renamed
                RN_stage[i].src2_tag = RMT[RN_stage[i].src2].tag;       //rename instruction src reg to name from RMT
            }
        }   
        //rename source registers^^^^^^^

        //rename destination register and set RMT
        if(RN_stage[i].destr != -1){            //check if there is a destination register
            RMT[RN_stage[i].destr].valid = 1;           //set RMT index valid
            RMT[RN_stage[i].destr].tag = ROB_tail;      //rename register in RMT to the index of instruction in ROB
            RN_stage[i].destr_tag = ROB_tail;           //rename destination register in Instruction package
        }       

        ROB_tail = (ROB_tail + 1) % ROB.size();             //move tail to next open space ensuring wraparound
        RR_stage[i] = RN_stage[i];                          //move instruction from rename to register read
        RR_stage[i].RR = current_cycle;                     //set register read timing information
    }

    for(int i = 0; i < width; i++){
        RN_stage[i].valid = 0;              //clear the RN stage signal decode to move forward
    }

    return;
}
//rename function^^^^^^

//register read function
void regread(){
    //check if DI is empty 
    if(RR_stage[0].valid == 0) {
        // if(current_cycle < 200){
        //     printf("failed at:%d", current_cycle);
        // }
        return;                      //if dispatch stage is full, do nothing
    }
    if(DI_stage[0].valid == 1) {
       
        return;                      //if dispatch stage is full, do nothing
    }

    //check if source registers are ready and set bit accordingly(if it is then mark it as ready)
    for(int i = 0; i < width; i++){
        if(RR_stage[i].valid == 0){             //this only occurs if at the end of file and not moving full bundle
            continue;
        }
        if(RR_stage[i].src1_tag == -1){     //register 1 has not been renamed and can be assumed ready
            RR_stage[i].rdy1 = 1;
        }
        else if(RR_stage[i].rdy1 != 1){                 //if reg 1 has been renamed and it hasn't been set ready by preceding stages
            RR_stage[i].rdy1 = ROB[RR_stage[i].src1_tag].rdy;       //set ready status of register 1 based on the ROB ready value
        }
        if(RR_stage[i].src2_tag == -1){     //register 2 has not been renamed and can be assumed ready
            RR_stage[i].rdy2 = 1;
        }
        else if(RR_stage[i].rdy2 != 1){                 //if reg 2 has been renamed and it hasn't been set ready by preceding stages
            RR_stage[i].rdy2 = ROB[RR_stage[i].src2_tag].rdy;       //set ready status of register 2 based on the ROB ready value
        }

        DI_stage[i] = RR_stage[i];              //move to dispatch stage
        DI_stage[i].DI = current_cycle;         //set time it entered DI stage
        RR_stage[i].valid = 0;                  //set reg read stage as empty after moving
        //printf("%d ",current_cycle);
    }

    
    return;
}
//register read function^^^^^

//Dispatch function
void dispatch(){
    if(DI_stage[0].valid == 0) {
       // printf("%d ",current_cycle);
        return;                  // if the Dispatch stage is empty then do nothing
    }
    if(IQ_size - total_in_IQ < width) return;         // if the there is not enough space in the IQ then do nothing

    //insert instruction into open location of the IQ(step through IQ to find open space)
    for(int i = 0; i < width; i++){
        //printf("%d ",total_in_IQ);

        for(int j = 0; j < IQ_size; j++){           //step through IQ to find empty spot
            if(IQ[j].valid == 0){
                IQ[j] = DI_stage[i];                //insert new instruction at empty spot
                IQ[j].IS = current_cycle;           //set IQ timing information for instruction
                DI_stage[i].valid = 0;              //clear DI_stage
                total_in_IQ++;
                break;
            }
        }
    }

    return;
}
//dispatch function^^^^^^^

//issue function
void issue(){
    int ready_index = 0;

    int max = find_max();                   //used to set the newest item and work back from

    for(int i = 0; i < width; i++){         //for loop to limit number of instructions moved to the Ex stage
        ready_index = find_min_ready(max);      //index of instruction ready to go to executre
        //printf("test print %d\n", ready_index);

        if(ready_index < IQ_size){                  //if ready index is found

            for(int j = 0; j < width*5; j++){       //increment through execute stage to find open index
                if(EX_stage[j].valid == 0){
                    EX_stage[j] = IQ[ready_index];          //insert ready index into execute
                    EX_stage[j].EX = current_cycle;
                    IQ[ready_index].valid = 0;              //set old IQ element as empty
                    total_in_IQ--;
                    break;
                }
            }
        }
    }
    return;
}
//issue function^^^^^^

//execute function
void execute(){
    int wb_index = 0;


    for(int i = 0; i < width*5; i++){
        //type 0 instruction handling
        if((EX_stage[i].op == 0) && (EX_stage[i].valid)){    //if the type is 0 and it has been in execute for 1 cycle
            if(current_cycle - EX_stage[i].EX > 0){
                WB_stage[wb_index] = EX_stage[i];
                WB_stage[wb_index].WB = current_cycle;
                EX_stage[i].valid = 0;

                wb_index++;            
            }
            //wakeup handling
            IQ_wakeup(i);
            DI_wakeup(i);
            RR_wakeup(i);


        }
        //type 1 instruction handling
        if((EX_stage[i].op == 1) && (EX_stage[i].valid)){    //if the type is 1 and it has been in execute for 2 cycles
            if((current_cycle - EX_stage[i].EX) > 1){
                IQ_wakeup(i);
                DI_wakeup(i);
                RR_wakeup(i);
            }
            if((current_cycle - EX_stage[i].EX) > 1){
                WB_stage[wb_index] = EX_stage[i];
                WB_stage[wb_index].WB = current_cycle;
                EX_stage[i].valid = 0;
                wb_index++;     
            }      
 
        }
        //type 2 instruction handling
        if((EX_stage[i].op == 2) && (EX_stage[i].valid)){    //if the type is 2 and it has been in execute for 5 cycles
            if((current_cycle - EX_stage[i].EX) > 4){
                //wakeup handling
                IQ_wakeup(i);
                DI_wakeup(i);
                RR_wakeup(i);
            }
            if((current_cycle - EX_stage[i].EX) > 4){
                WB_stage[wb_index] = EX_stage[i];
                WB_stage[wb_index].WB = current_cycle;
                EX_stage[i].valid = 0;
                wb_index++;            
            }

            
        }


    }
    return;
}
//execute function^^^^^

//Writback Function
void writeback(){
    for(int i = 0; i < width*5; i++){           //move through every item in the WB unit
        if(WB_stage[i].valid == 0)  break;
        for(int j = 0; j < ROB_size; j++){      //search ROB for matching item
            if((ROB[j].inst.seq == WB_stage[i].seq)){

                ROB[j].rdy = 1;                 //set corresponding instruction to ready in the ROB
                ROB[j].inst = WB_stage[i];      //update instruction timing information in ROB
                ROB[j].inst.RT = current_cycle;
                WB_stage[i].valid = 0;          //remove instruction from the writeback stage
            }
        }  

    }
    return;
}
//Writeback Function^^^^^^

//Retire function
void retire(){
    for(int i = 0; i < width;i++){                  //retire up to WIDTH items from the ROB
        if(ROB[ROB_head].rdy == 1){

            //write code to reset corresponding RMT space(only reset if the tags match)
            if(ROB[ROB_head].dst != -1){            //this is done to avoid out of bounds access of vector
                if(RMT[ROB[ROB_head].dst].tag == ROB[ROB_head].inst.destr_tag){         //if destination register tag is the same as in RMT
                    RMT[ROB[ROB_head].dst].tag = -1;                    //set tag to something that won't match anything
                    RMT[ROB[ROB_head].dst].valid = 0;                   //set as invalid(i.e. empty)
                }
            }
            printf("%d  fu{%d}  src{%d,%d}  dst{%d}  FE{%d,%d}  DE{%d,%d}  RN{%d,%d}  RR{%d,%d}  DI{%d,%d}  IS{%d,%d}  EX{%d,%d}  WB{%d,%d}  RT{%d,%d}\n", 
            ROB[ROB_head].inst.seq, ROB[ROB_head].inst.op, ROB[ROB_head].inst.src1, ROB[ROB_head].inst.src2, ROB[ROB_head].inst.destr, 
            ROB[ROB_head].inst.DE - 1, 1, 
            ROB[ROB_head].inst.DE, ROB[ROB_head].inst.RN - ROB[ROB_head].inst.DE,
            ROB[ROB_head].inst.RN, ROB[ROB_head].inst.RR - ROB[ROB_head].inst.RN,
            ROB[ROB_head].inst.RR, ROB[ROB_head].inst.DI - ROB[ROB_head].inst.RR,
            ROB[ROB_head].inst.DI, ROB[ROB_head].inst.IS - ROB[ROB_head].inst.DI,
            ROB[ROB_head].inst.IS, ROB[ROB_head].inst.EX - ROB[ROB_head].inst.IS,
            ROB[ROB_head].inst.EX, ROB[ROB_head].inst.WB - ROB[ROB_head].inst.EX,
            ROB[ROB_head].inst.WB, 1,
            ROB[ROB_head].inst.RT, current_cycle - ROB[ROB_head].inst.RT);

            //final_list.push_back(ROB[ROB_head].inst);
            ROB_head = (ROB_head + 1) % ROB_size;           //increment head point(basically voids previous input)
            total_in_ROB--;                                 //decrease number perceived number of items in ROB
            dic++;
        }
    }
    return;
}
//Retire Function^^^^^^

//Advance cycle function
int advance_cycle(){
    int stages_empty = 1;
    current_cycle++;
    if((DE_stage[0].valid == 1) || (RN_stage[0].valid == 1) ||
       (RR_stage[0].valid == 1) || (DI_stage[0].valid == 1) ||
       (EX_stage[0].valid == 1) || (WB_stage[0].valid == 1)){
        stages_empty = 0;
       }

    if(stages_empty && IQ_status() && ROB_status() && current_cycle > 10){
        return 0;
    }

    return 1;
    
}
//Advance cycle function^^^^^





//IQ helper functions
int find_min_ready(int x){
    int min_val = x;
    int min_index = IQ_size;
    int test_variable = 0;

    for(int i = 0; i < IQ_size; i++){

        if(IQ[i].valid == 0){           //ensures only occupied indexes are used
            test_variable++;
            //printf("%d",test_variable);
            continue;
        }
        if((IQ[i].seq <= min_val) && (IQ[i].rdy1 == 1) && (IQ[i].rdy2 == 1)){        //checks if all regs are ready and if it is the minimum item
            min_val = IQ[i].seq;            //change new minimum value
            min_index = i;                  //set minimum index
        }
    }

    return min_index;
}

int find_max(){
    int max_value = 0;
    int test = 0;

    for(int i = 0; i < IQ_size; i++){
        if(IQ[i].valid == 0){
            test++;
            //printf("%d",test);
        }

        if((IQ[i].seq >= max_value) && (IQ[i].valid == 1)){        //checks if all regs are ready and if it is the minimum item
            max_value = IQ[i].seq;            //change new minimum value
        }
    }
    return max_value;
}
//IQ helper FUnctions^^^^^^

//Execute helper functions
void IQ_wakeup(int ex_index){
    for(int i = 0; i < IQ_size; i++){       //walk through IQ to find any depended source registers
        if(IQ[i].src1_tag == EX_stage[ex_index].destr_tag)  {
            IQ[i].rdy1 = 1;                 //if source register 1 name matches freed destination register then set ready
        }
        if(IQ[i].src2_tag == EX_stage[ex_index].destr_tag)  {
            IQ[i].rdy2 = 1;                 //if source register 2 name matches freed destination register then set ready
        }
    }
}

void DI_wakeup(int ex_index){
    for(int i = 0; i < width; i++){       //walk through IQ to find any depended source registers
        if(DI_stage[i].src1_tag == EX_stage[ex_index].destr_tag)  {
            DI_stage[i].rdy1 = 1;                 //if source register 1 name matches freed destination register then set ready
        }
        if(DI_stage[i].src2_tag == EX_stage[ex_index].destr_tag)  {
            DI_stage[i].rdy2 = 1;                 //if source register 2 name matches freed destination register then set ready
        }
    }
}

void RR_wakeup(int ex_index){
    for(int i = 0; i < width; i++){       //walk through IQ to find any depended source registers
        if(RR_stage[i].src1_tag == EX_stage[ex_index].destr_tag)  {
            RR_stage[i].rdy1 = 1;                 //if source register 1 name matches freed destination register then set ready
        }
        if(RR_stage[i].src2_tag == EX_stage[ex_index].destr_tag)  {
            RR_stage[i].rdy2 = 1;                 //if source register 2 name matches freed destination register then set ready
        }
    }
}
//Execute helper functions^^^^^

//Advance cycle functions
int IQ_status(){
    int all_clear = 1;
    for(int i = 0; i < IQ_size; i++){
        if(IQ[i].valid == 1){
            all_clear = 0;
        }
    }

    return all_clear;
}

int ROB_status(){
    if(total_in_ROB == 0){
        return 1;
    }
    return 0;
}
//Advance cycle functions

//printing functions
void print_final(){
    sort(final_list.begin(), final_list.end(), [](const Instruction& a, const Instruction& b){
        return a.seq < b.seq;
    });

    for(uint32_t i = 0; i < final_list.size(); i++){
        printf("%d  fu{%d}  src{%d,%d}  dst{%d}  FE{%d,%d}  DE{%d,%d}  RN{%d,%d}  RR{%d,%d}  DI{%d,%d}  IS{%d,%d}  EX{%d,%d}  WB{%d,%d}  RT{%d,%d}\n", 
            final_list[i].seq, final_list[i].op, final_list[i].src1, final_list[i].src2, final_list[i].destr, 
            final_list[i].DE - 1, 1, 
            final_list[i].DE, final_list[i].RN - final_list[i].DE,
            final_list[i].RN, final_list[i].RR - final_list[i].RN,
            final_list[i].RR, final_list[i].DI - final_list[i].RR,
            final_list[i].DI, final_list[i].IS - final_list[i].DI,
            final_list[i].IS, final_list[i].EX - final_list[i].IS,
            final_list[i].EX, final_list[i].WB - final_list[i].EX,
            final_list[i].WB, 1,
            final_list[i].RT, 1);
    }
    return;
}