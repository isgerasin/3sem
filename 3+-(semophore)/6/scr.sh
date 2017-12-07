#!/bin/bash

for ((i = 1; i < 20; i++))
do
rm output
./a.out > output & ./a.out input
diff input output 

rm output
./a.out > output & ./a.out > output & ./a.out input
 diff input output

rm output
./a.out	> output & ./a.out input & ./a.out input
diff input output
done
