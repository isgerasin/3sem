#!/bin/bash

for ((i = 1; i < 500; i++))
do
./a.out > output & ./a.out input
diff input output 

./a.out input & ./a.out > output
diff input output

done
