#!/bin/bash
N=100

for ((i = 1; i < $N + 1; i++))
do
./a.out > output & ./a.out input
diff input output

./a.out input & ./a.out > output
diff input output

echo " $i / $N "

done
