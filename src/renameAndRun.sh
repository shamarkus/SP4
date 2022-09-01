#!/usr/bin/env bash
ACAT_PATH=/home/admin/Desktop/ACAT/exe
cd $ACAT_PATH
FAKE_DATE="20010101"
FAKE_TIME="23:59:59"


if [ "$#" -ne 3 ]; then
    echo "Incorrect number of parameters"
    echo "-------Usage-------"
    echo "bash renameAndRun.sh [YYYYMM start_day end_day]"
    echo "ex: bash renameAndRun.sh 202003 1 30"
    echo "The above will run the ACAT tool on all files for March 2020 from the 1st to the 30th (20200301 to 20200330)"
	echo "-------------------"
    exit 1
fi

YEAR_MONTH_ARG=$1
START_DAY=$2
END_DAY=$3
if ! [[ $YEAR_MONTH_ARG =~ [0-9]{6} ]] || [[ $START_DAY -gt 31 ||$START_DAY -lt 1 ]] || [[ $END_DAY -gt 31 || $END_DAY -lt 1 ]] ; then
    echo "Invalid parameters - Parameter 1 must be YYYYMM, parameters 2 and 3 must be 1-31"
    exit 1
fi

for ((day=START_DAY;day<=END_DAY;day++)); do
	printf -v DAY "%02d" $day
	if [[ -z `find . -name "${YEAR_MONTH_ARG}${DAY}*.log"` ]]; then
		echo "No file dated under ${YEAR_MONTH_ARG}${DAY} was found. Skipping."
		continue
	fi
	echo "Processing files dated under ${YEAR_MONTH_ARG}${DAY}.."
	rm Other/records.txt
	#Rename files to the fake date
	find . -name "${YEAR_MONTH_ARG}${DAY}*.log" -exec rename ${YEAR_MONTH_ARG}${DAY} $FAKE_DATE {} +
	echo ${YEAR_MONTH_ARG}${DAY} > last_renamed.log
	faketime "$FAKE_DATE $FAKE_TIME" ./Automated_CSS_Alarm_Tool
	#echo "faketime "$FAKE_DATE $FAKE_TIME" ./Automated_CSS_Alarm_Tool > /dev/null 2>&1"
	#echo "find . -name ${FAKE_DATE}*.log -exec rename $FAKE_DATE ${YEAR_MONTH_ARG}${DAY} {} +"
	find . -name "$FAKE_DATE*.log" -exec rename $FAKE_DATE ${YEAR_MONTH_ARG}${DAY} {} +
done
cd /home/admin/Desktop/ACAT/exe/Incidents
sort -u CSS_Alarms.csv > unique.csv && mv unique.csv CSS_Alarms.csv 
