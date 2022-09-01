#!/bin/bash
#EXE_DIR="/home/ewchan/Desktop/SP4/exe"
EXE_DIR="."

rm -rf $EXE_DIR/Database_Files/*

rm -rf $EXE_DIR/Email_Info/*.html

rm $EXE_DIR/Other/records.txt

touch $EXE_DIR/Other/records.txt

rm $EXE_DIR/Other/LastEmailTime.txt

touch $EXE_DIR/Other/LastEmailTime.txt

STR="15:55:31 06/08/20"

echo "$STR" >> "$EXE_DIR/Other/LastEmailTime.txt"

T1=$(date +%s%3N)
make -f "$EXE_DIR/makefile" && ./Automated_CSS_Alarm_Tool
T2=$(date +%s%3N)

echo "Execution Time : $(($T2-$T1))"
