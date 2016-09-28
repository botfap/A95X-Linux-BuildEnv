for file in `ls`  
do  
  newfile =`echo $i | sed -n 's/elf32/elf32-em4/'`  
  mv $file $newfile  
done 
