#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sim_proc.h"
#include "pipeline.h"

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
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    uint64_t pc; // Variable holds the pc read from input file
    
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
    int current_cycle = 0;
    int global_seq = 0; 
    width = params.width;
    
    //variables for ROB management


    //define all stages of the pipeline based on the necessary size

    DE_stage.resize(width);
    RN_stage.resize(width);
    RR_stage.resize(width);
    DI_stage.resize(width);
    EX_stage.resize(width*5);
    WB_stage.resize(width*5);
    IQ.resize(params.iq_size);
    ROB.resize(params.rob_size);

    ARF.resize(67);
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
    while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
        printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly

    //fetch section
    if(DE_stage[0].valid == 0){                     //if nothing in DE
        for(int i = 0; i < params.width; i++){
            fscanf(FP, "%lx %d %d %d %d", &DE_stage[i].pc, &DE_stage[i].op, &DE_stage[i].destr, &DE_stage[i].src1, &DE_stage[i].src2);  //read file line

            DE_stage[i].valid = 1;                  //set stage as valid
            DE_stage[i].seq = global_seq;           //store sequence number
            global_seq++;                           //increment sequence counter
        }
    }  
    //fetch section complete

    //Decode
    if(DE_stage[0].valid == 1 && RN_stage[0].valid == 0){       //if decode bundle contains something and rename bundle is empty
        for(int i = 0; i < params.width; i++){
            RN_stage[i] = DE_stage[i];              //move instruction from DE to RN
            DE_stage[i].valid = 0;                  //clear decode stage
        }
    }
    //Decode 

    //Rename
    rename();
    //Rename^^^^^^



    return 0;
}


//fetch code


//rename code
void rename(){
        //check if ROB has free entries
    if((ROB.size() - total_in) < width)  return;         //do nothing if the ROB is full
        
        //check if RR is empty 
    if(RR_stage[0].valid != 0)  return;                         //do nothing if the rename stage is occupied

        
    for(int i = 0; i < width; i++){
        //adds instruction to ROB 
        if(RN_stage[i].destr != -1){                    
            ROB[ROB_tail].dst = RN_stage[i].destr;
        }
        else{
            ROB[ROB_tail].dst = -1;
        }
        ROB[ROB_tail].rdy = 0;
        ROB[ROB_tail].exc = 0;
        ROB[ROB_tail].mis = 0;
        ROB[ROB_tail].pc = RN_stage[i].pc;
        //adds instruction to ROB^^^^^

        //rename source registers
        if(RMT[RN_stage[i].src1].valid == 1){        //source register 1 has been renamed
            RN_stage[i].src1_tag = RMT[RN_stage[i].src1].tag;       //rename instruction src reg to name from RMT
        }
        if(RN_stage[i].src2 != -1){
            if((RMT[RN_stage[i].src2].valid == 1)){        //source register 2 has been renamed
                RN_stage[i].src2_tag = RMT[RN_stage[i].src2].tag;       //rename instruction src reg to name from RMT
            }
        }   
        //rename source registers^^^^^^^

        //rename destination register and set RMT
        if(RN_stage[i].destr != -1){            //check if there is a destination register
            RMT[RN_stage[i].destr].valid = 1;           //set RMT index valid
            RMT[RN_stage[i].destr].tag = ROB_tail;      //rename register in RMT
            RN_stage[i].destr_tag = ROB_tail;           //rename destination register in Instruction package
        }       

        ROB_tail = (ROB_tail + 1) % ROB.size();             //move tail to next open space ensuring wraparound
        RR_stage[i] = RN_stage[i];
    }

    for(int i = 0; i < width; i++){
        RN_stage[i].valid = 0;              //clear the RN stage signal decode to move forward
    }
    //Rename^^^^^^
}