#!/bin/bash

for ((i = 1; i < 4; i++))
do
./a.out > output & ./a.out input
diff input output

./a.out > output & ./a.out > output & ./a.out input
diff input output

./a.out	> output & ./a.out input & ./a.out input
diff input output
done
