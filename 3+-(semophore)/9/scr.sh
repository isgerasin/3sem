#!/bin/bash

for ((i = 1; i < 5; i++))
do
./a.out > output & ./a.out input
diff input output 

./a.out input & ./a.out > output
diff input output

./a.out > output & ./a.out > output & ./a.out input
 diff input output

./a.out	> output & ./a.out input & ./a.out input
diff input output
done
