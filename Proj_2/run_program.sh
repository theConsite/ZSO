#!/bin/bash

levels=2
debug=false

disp_help(){
    echo "Usage: $0 [--levels <1-3> (Default 2)]"
    exit
}

disp_arg_error(){
    echo "Invalid argument, check --help for usage instructions"
    exit
}

disp_lvl_val_err(){
    echo "Invalid value, levels should have value between 1 and 3"
    exit
}

while [ $# -gt 0 ]; do
    if [[ $1 == "--help" ]]; then
        disp_help
    elif [[ $1 == "--levels" ]]; then
        if [[ -n $2 && $2 =~ ^[1-3]$ ]]; then
            levels=$2
            shift
        else
            disp_lvl_val_err
        fi
    else
        disp_arg_error
    fi
    shift
done

nodes_on_level=(4 9 27)

current_id=0

for (( i=1; i<=$levels; i++ ))
do
    num_times=${nodes_on_level[$((i-1))]}
    for (( j=0; j<num_times; j++ ))
    do
        ./Proj_2 "$current_id" "$1" &
        current_id=$((current_id + 1))
    done
done