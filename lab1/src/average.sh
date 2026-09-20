#!/bin/bash
count=$#
sum=0
for arg in "$@"; do
    sum=$((sum + arg))
done
avr=$((sum / count))
echo "count: $count"
echo "avr: $avr"
