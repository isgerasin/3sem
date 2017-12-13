#!/bin/bash

a=0 ;
i=0 ;
while /bin/true  
do
	echo "1234567890" >> $1; 
	((a=$a+11)) ;
	for ((i=2; i < 15; i++ ))
	do 
		./a.out $i $1 > out ; 
#		sleep 0.05 ;
		if diff $1 out
		then
			echo "ok $a $i" ;
		else
			exit ;
		fi
	done
done
