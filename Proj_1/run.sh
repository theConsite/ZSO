#!/bin/sh
I=1
while [ $I -le 30 ]; do
    ./Proj_1
    echo $I
    I=$((I + 1))
done