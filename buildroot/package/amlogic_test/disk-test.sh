#!/bin/sh
#config your test here!

#loop number of every test
LOOP_NUM=1

#enable or disable a test plan here
#random, 4M
ENABLE_RANDOM1=y
#random, 2k
ENABLE_RANDOM2=n
#pattern(AA55AA55), 16M
ENABLE_PATTERN1=n
#pattern(FFFFFFFF), 16M
ENABLE_PATTERN2=n

#statistics
cycle=0
copy=0
copy_err=0
read_err=0
check_err=0

#global varible
infile="source.bin"
condition=1

create_testfile_random1()
{
    echo "Creating source file(random 4M) $infile..."
    dd if=/dev/urandom of="$infile" bs=1024 count=4000
    sync
}

create_testfile_random2()
{
    echo "Creating source file(random 2k) $infile..."
    dd if=/dev/urandom of="$infile" bs=1024 count=2
    sync
}

create_patternfile_1()
{
    echo "Creating source file(pattern1 AA55AA55) $infile..."
    rm $infile
    touch $infile
    i=2048
    test=""
    while [[ $i -gt 0 ]]
    do
    	test="\xA\xA\x5\x5\xA\xA\x5\x5$test"
    	let i--
    done
    
    i=1024
    while [[ $i -gt 0 ]]
    do
    	echo $test >> $infile
    	let i--
    done
    sync
}

create_patternfile_2()
{
    echo "Creating source file(pattern1 FFFFFFFF) $infile..."
    rm $infile
    touch $infile
    i=2048
    test=""
    while [[ $i -gt 0 ]]
    do
    	test="\xF\xF\xF\xF\xF\xF\xF\xF$test"
    	let i--
    done
    
    i=1024
    while [[ $i -gt 0 ]]
    do
    	echo $test >> $infile
    	let i--
    done
    sync
}

do_test()
{
	cycle=0
    i=$LOOP_NUM
    while [[ $i -gt 0 ]]
    do
        let cycle++
        echo "-------------cycle $cycle"

        let copy++
        # copy data files
        while [ 1 ]
        do
            condition=$(df -h | sed  "s/  */ /g" | grep "/tmp" | cut -d ' ' -f 4)
            echo $condition | grep -q "M"
            if [ "$?" -eq 0 ] 
            then
              echo "free size: $condition"
              if [ $(expr $(echo $condition | sed 's/.[0-9][A-Z]//g') + 0)  -le 30 ]
              then
                  echo "not enough $condition"
		  break
	      else
	          echo "enough $condition"
	      fi
	    else
	        echo "free size: $condition"
	    fi
            
            echo "--copies $copy"

            outfile="${infile}_${copy}"
            #copy file
            dd if="$infile" of="$outfile"
            if [ "$?" -ne 0 ]
              then
              let copy_err++
            fi
            
            sync

            #read file
            dd if="$outfile" of=/dev/null
            if [ "$?" -ne 0 ]
              then
              let read_err++
            fi

            let copy++
            
            sync
            
            #Check file
            diff $infile $outfile
            if [ "$?" -ne 0 ]
              then
              let check_err++
            fi

            trap "echo 'result:' $cycle 'cycles' $copy 'copies, Copy failed' $copy_err ', Read failed' $read_err ', Check failed' $check_err && rm -f ${infile}_* && exit;" 2
        done
        rm -f "$infile"_*
        sync
        let i--;
    done
}

echo -e "*################################################*"
echo -e "*                                                *"
echo -e "*                 Disk Test Tools                *"
echo -e "*                 ---------------                *"
echo -e "*################################################*"
echo -e "                copyrighted by"
echo -e "                    Amlogic Inc."
echo -e " "
echo -e "sure to start?(y/n)y:\c"
read -r answer </dev/tty
if [ "$answer" == 'n' ] || [ "$answer" == 'N' ]
then
    echo "reuslt: $cycle cycles, $copy copies, R/W failed: $err"
    rm -f ${infile}
    exit 0
fi

cd /tmp/
if [ "$ENABLE_RANDOM1" == 'y' ]|| [ "$ENABLE_RANDOM1" == 'Y' ]
then
    create_testfile_random1
    do_test
fi

if [ "$ENABLE_RANDOM2" == 'y' ]|| [ "$ENABLE_RANDOM2" == 'Y' ]
then
    create_patternfile_1
    do_test
fi

if [ "$ENABLE_PATTERN1" == 'y' ]|| [ "$ENABLE_PATTERN1" == 'Y' ]
then
    create_patternfile_2
    do_test
fi

if [ "$ENABLE_PATTERN1" == 'y' ]|| [ "$ENABLE_PATTERN2" == 'Y' ]
then
    create_testfile_random1
    do_testENABLE_PATTERN2
fi

echo ""
echo "Flash test Done!"
echo "    ENABLE_RANDOM1=$ENABLE_RANDOM1"
echo "    ENABLE_RANDOM2=$ENABLE_RANDOM2"
echo "    ENABLE_PATTERN1=$ENABLE_PATTERN1"
echo "    ENABLE_PATTERN2=$ENABLE_PATTERN2"
echo "  result:" 
echo "    $copy copies"
echo "    Copy failed $copy_err , Read failed $read_err , Check failed $check_err"

if [ "$copy_err" == '0' ] && [ "$copy_err" == '0' ] && [ "$copy_err" == '0' ]
then
	echo ""
	echo "The results seems fine!"
fi
