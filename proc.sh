#! /bin/bash

echo "testing superscalar-pipeline"


#compile code

g++ -o experimentation experimentation.cc

#Large ROB, effect of IQ size
# for x in "val_trace_gcc1" "val_trace_perl1"
# do
#     echo $x
#     for i in 1 2 4 8
#     do 
#         echo "width = $i"
#         for j in 8 16 32 64 128 256
#         do  
#             ./experimentation 512 $j $i $x
#         done
#     done
# done

#Effect of ROB size using ideal IQ size
for x in "val_trace_gcc1" "val_trace_perl1"
do  
    echo $x
    for i in 1 2 4 8
    do  
        echo "width = $i"
        if [ "$i" -eq 1 ]; then
            for j in 32 64 128 256 512
            do
                ./experimentation $j 8 $i $x
            done

        elif [ "$i" -eq 2 ]; then 
        for j in 32 64 128 256 512
            do
                ./experimentation $j 16 $i $x
            done

        elif [ "$i" -eq 4 ]; then
        for j in 32 64 128 256 512
            do
                ./experimentation $j 64 $i $x
            done

        elif [ "$i" -eq 8 ]; then
        for j in 32 64 128 256 512
            do
                if [ "$x" = "val_trace_gcc1" ]; then
                    ./experimentation $j 64 $i $x
                elif [ "$x" = "val_trace_perl1" ]; then
                    ./experimentation $j 128 $i $x
                fi
            done
        fi
    done
done





