#!/bin/ash

if [ -z $repeat ]; then
	repeat=36000
fi
echo "Test count: $repeat" 

governor=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
freq_max=/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
freq_tbl=/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
freq_cur=/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq
delay=1
 
if [ -e ${governor} ]; then   
    echo "  "	 
    echo "  "	 
    echo "==========Auto Freq Test Start. ===================="
    echo " Change governor to Performance"
    echo performance  > ${governor}    
    sleep $delay
    echo "Governor is now:" `cat ${governor}`
    echo "Freq Table:" `cat ${freq_tbl}`
else    
    echo "$governor Not Exist, Please Check A"
    exit 0
fi


val=`cat ${freq_tbl}  | awk '{print $4}'`
freq_nr=`cat ${freq_tbl}  | awk '{print NF}'`

cnt=0
while true 
do
let "cnt=$cnt+1"
if [ $cnt -ge $repeat ];then       
echo Auto CPU Scaling Test Finished          
exit 0                      
fi 
rand=`date +%s`
#let "freq_idx=$rand *7 % ${freq_nr}"
let "freq_idx=$rand % ${freq_nr}"
let "freq_idx=$freq_idx + 1"

echo "  "
echo "  "
echo "=============Auto Freq. cnt=$cnt, freq_idx=$freq_idx ================"

case  "$freq_idx"  in    
1 ) target_freq=`cat ${freq_tbl}  | awk '{print $1}'`  ;; 
2 ) target_freq=`cat ${freq_tbl}  | awk '{print $2}'`  ;;
3 ) target_freq=`cat ${freq_tbl}  | awk '{print $3}'`  ;; 
4 ) target_freq=`cat ${freq_tbl}  | awk '{print $4}'`  ;;
5 ) target_freq=`cat ${freq_tbl}  | awk '{print $5}'`  ;;
6 ) target_freq=`cat ${freq_tbl}  | awk '{print $6}'`  ;;
7 ) target_freq=`cat ${freq_tbl}  | awk '{print $7}'`  ;;
8 ) target_freq=`cat ${freq_tbl}  | awk '{print $8}'`  ;;
9 ) target_freq=`cat ${freq_tbl}  | awk '{print $9}'`  ;;
10) target_freq=`cat ${freq_tbl}  | awk '{print $10}'`  ;;
11) target_freq=`cat ${freq_tbl}  | awk '{print $11}'`  ;;
12) target_freq=`cat ${freq_tbl}  | awk '{print $12}'`  ;;
13) target_freq=`cat ${freq_tbl}  | awk '{print $13}'`  ;;
14) target_freq=`cat ${freq_tbl}  | awk '{print $14}'`  ;;
15) target_freq=`cat ${freq_tbl}  | awk '{print $15}'`  ;;
16) target_freq=`cat ${freq_tbl}  | awk '{print $16}'`  ;;
17) target_freq=`cat ${freq_tbl}  | awk '{print $17}'`  ;;
18) target_freq=`cat ${freq_tbl}  | awk '{print $18}'`  ;;
* ) echo "Freq_Index OverFlow" ;;
esac


cur_freq=`cat ${freq_cur}`
if [ ${cur_freq} != ${target_freq} ]; then
    echo "echo ${target_freq} >  ${freq_max}"
    echo ${target_freq} > ${freq_max}
    sleep $delay 


    cur_freq=`cat ${freq_cur}`
    echo "    cat $freq_cur"
    echo "    $cur_freq"
	
    #if [ ${cur_freq} != ${target_freq} ]; then
	#echo "========== Auto Freq Error! CurFreq != TargetFreq =========="
    #exit 0
#	fi
else
    echo "TargetFreq==CurFreq, Do Nothing. "
    sleep $delay 
fi
done
