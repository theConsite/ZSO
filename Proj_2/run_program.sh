runs=(4 9 27)

current_id=0

for (( i=1; i<=$1; i++ ))
do
    num_times=${runs[$((i-1))]}
    for (( j=0; j<num_times; j++ ))
    do
        ./Proj_2 "$current_id" "$1"
        current_id=$((current_id + 1))
    done
done