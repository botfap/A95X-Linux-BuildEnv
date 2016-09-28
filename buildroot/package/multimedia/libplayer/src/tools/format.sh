#!/bin/bash
#Format the source code(s)
astyle --style=linux --indent=spaces=4 --brackets=linux --indent-col1-comments --pad-oper --pad-header \
       --unpad-paren --add-brackets --convert-tabs --mode=c --lineend=linux $@ >/tmp/astyle.log

#Deal with the log of astyle 
sed -i -e 's;formatted  ;;g' /tmp/astyle.log;
sed -i -e 's;unchanged\* ;;g' /tmp/astyle.log;

#Define global variable
DIR=$PWD
#Get the directory of the source code(s) to generate the patch
line=`sed -n 1p /tmp/astyle.log`
DIR=`dirname $line`

#We needn't /tmp/astyle.log any more, delete it.
rm -rf /tmp/astyle.log

#Check if the patch file existed
if [ -e ${DIR}/linux_style.patch ]; then 
	rm -rf ${DIR}/linux_style.patch
fi


#Generate the patch
DATE_TIME=$(date '+%H.%M.%S.%N')
suffix=.orig
list=`find ${DIR} -name "*"${suffix}`
echo $list
for i in $list;
do
	filename=`echo $i | sed 's/'${suffix}'$//'`;
	diff -Nura $filename${suffix} $filename >> ${PWD}/linux_style-${DATE_TIME}.patch;
	rm -rf $filename${suffix}
done
