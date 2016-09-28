#!/bin/sh

#only one input param, output signed bootloader and efuse patten
#param1 is not signed u-boot.bin
aml_secureboot_sign_bootloader(){
	echo -----aml-secureboot-sign-bootloader ------
	printf "${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --bootsig "
    printf " --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY} "
    printf " --aeskey enable"
    printf " --input $1 "
    printf " --output $1.encrypt\n"
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --bootsig \
        --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY} \
        --aeskey enable \
        --input $1 \
        --output $1.encrypt
	echo ----- Made aml secure-boot singed bootloader: $1.encrypt --------
}

#input para is boot.img or recovery.img
aml_secureboot_sign_kernel(){
	echo -----aml-secureboot-sign-kernel ------
	printf "${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --imgsig"
    printf " --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}"
    printf " --input $1 --output ${1}.encrypt\n"
	${PRODUCT_AML_SECUREBOOT_SIGNTOOL} --imgsig \
        --amluserkey ${PRODUCT_AML_SECUREBOOT_USERKEY}  \
        --input $1 --output ${1}.encrypt
	echo ----- Made aml secure-boot singed bootloader: $1.encrypt --------
}

platform=$1
secureboot=$2

PRODUCT_AML_IMG_PACK_TOOL=${TOOL_DIR}/aml_image_v2_packer_new
PRODUCT_AML_IMG_SIMG_TOOL=${TOOL_DIR}/img2simg
PRODUCT_AML_IMG_PACK_DIR=${BINARIES_DIR}

####Step 1: rename dtb to dtb.img
#change gxb_p200.db to dtb.img
#Note: if you using more than one dtb, using 'vendor/amlogic/tools/dtbTool' to pack all your dtbs into the dtb.img
#echo "move your dtbs to add dtb.img"
#ln -sf `ls ${PRODUCT_AML_IMG_PACK_DIR}/*.dtb` ${PRODUCT_AML_IMG_PACK_DIR}/dtb.img

####Step 2: compress 1g 
ext4img=${PRODUCT_AML_IMG_PACK_DIR}/rootfs.ext2
sparseimg=${ext4img}.img2simg
update_sparse_img=0
if [ ! -f  ${sparseimg} ]; then update_sparse_img=1; fi
if [ ${update_sparse_img} -ne 1 ];then
    t1=`stat -c %Y ${ext4img}`
    t2=`stat -c %Y ${sparseimg}`
    if [ ${t1} -gt ${t2} ]; then
        echo "ext4 file time newer than sparse image"
        update_sparse_img=1
    fi
fi
if [ ${update_sparse_img} -eq 1 ]; then 
    echo "compress 1g ext4 image to compressed sparse format"
    ${PRODUCT_AML_IMG_SIMG_TOOL} ${ext4img} ${sparseimg}
    if [ ! -f ${sparseimg} ]; then 
        echo "fail to compress ext4 img to sparse format"
        exit 1
    fi
fi

####Step 3: pack none-secureboot burning image
aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.conf
burnPkg=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package.img
if [ "${secureboot}" != "y" ]; then
    echo "generate noraml burning package"
    echo "${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg} "
    ${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg} 
    if [ $? -ne 0 ]; then 
        echo fail to generate burning image;  
        rm ${burnPkg}
    fi
    exit $?
fi

####Step 4: pack secureboot burning image
aml_upgrade_package_conf=${PRODUCT_AML_IMG_PACK_DIR}/aml_upgrade_package_enc.conf

#########Support compiling out encrypted zip/aml_upgrade_package.img directly
#PRODCUT_AML_BOOTLOADER_PATH=./output/build/uboot-custom
PRODUCT_AML_SECUREBOOT_USERKEY=${BINARIES_DIR}/aml-user-key.sig
PRODUCT_AML_SECUREBOOT_SIGNTOOL=${TOOL_DIR}/aml_encrypt_${platform}

PRODUCT_OUTPUT_DIR=${BINARIES_DIR}
aml_bootloader=${PRODUCT_OUTPUT_DIR}/u-boot.bin
aml_secureboot_sign_bootloader ${aml_bootloader}
if [ ! -f ${aml_bootloader}.encrypt ]; then
    echo "fail to sign bootloader"
    exit 1
fi
#rename efuse patten name for windows USB_BURNING_TOOL
mv ${aml_bootloader}.encrypt.efuse SECURE_BOOT_SET

aml_kernel=${PRODUCT_OUTPUT_DIR}/boot.img
aml_secureboot_sign_kernel ${aml_kernel}
if [ ! -f ${aml_kernel}.encrypt ]; then
    echo "fail to sign kernel image"
    exit 1
fi

echo "${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg} "
${PRODUCT_AML_IMG_PACK_TOOL} -r ${aml_upgrade_package_conf} ${PRODUCT_AML_IMG_PACK_DIR} ${burnPkg} 
if [ $? -ne 0 ]; then 
    echo fail to generate burning image; 
    rm ${burnPkg}
fi
exit $?
