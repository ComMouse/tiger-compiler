#!/bin/bash
#============some output color
SYS=$(uname -s)
if [[ $SYS == "Linux" ]]; then
	RED_COLOR='\E[1;31m'  
	GREEN_COLOR='\E[1;32m' 
	YELOW_COLOR='\E[1;33m' 
	BLUE_COLOR='\E[1;34m'  
	PINK='\E[1;35m'      
	RES='\E[0m'
fi


BIN=a.out
TESTCASEDIR=./testcases
ret_value=0
#echo $PROJDIR/$ite
#pwd
make clean >& /dev/null
make >& /dev/null
#echo $?
if [[ $? != 0 ]]; then
	echo -e "${RED_COLOR}[-_-]$ite: Compile Error${RES}"		
	exit 123
fi	
	for tcase in `ls $TESTCASEDIR/`
	do		
		if [ ${tcase##*.} = "tig" ]; then
			tfileName=${tcase##*/}
			./$BIN $TESTCASEDIR/$tfileName >& _tmp.txt
			grep "mov" $TESTCASEDIR/${tfileName}.s >& __tmp.txt

			if [ ! -s __tmp.txt ]; then
				echo -e "${BLUE_COLOR}[*_*]$ite: The output is not reasonable. [$tfileName]${RES}"
				rm -f _tmp.txt __tmp.txt $TESTCASEDIR/${tfileName}.s
				exit 234
			fi
			rm -f _tmp.txt __tmp.txt $TESTCASEDIR/${tfileName}.s
		fi
	done
echo -e "${GREEN_COLOR}[^_^]$ite: Pass Lab6A${RES}"
