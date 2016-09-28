#!/usr/bin/python

import sys
import re
import os
import time

base_url = 'http://openlinux.amlogic.com:8000/download/ARM/'

pkg = {
   'customer':'BR2_PACKAGE_AML_CUSTOMER_GIT_URL', 
   'kernel':'BR2_LINUX_KERNEL_CUSTOM_REPO_URL', 
   'gpu':'BR2_PACKAGE_GPU_GIT_URL',
   'wifi':'BR2_PACKAGE_WIFI_FW_CUSTOM_GIT_REPO_URL',
   '8188eu':'BR2_PACKAGE_RTK8188EU_GIT_REPO_URL',
   '8192cu':'BR2_PACKAGE_RTK8192CU_GIT_REPO_URL',
   '8192du':'BR2_PACKAGE_RTK8192DU_GIT_REPO_URL',
   '8192eu':'BR2_PACKAGE_RTK8192EU_GIT_REPO_URL',
   '8192es':'BR2_PACKAGE_RTK8192ES_GIT_REPO_URL',
   '8189es':'BR2_PACKAGE_RTK8189ES_GIT_REPO_URL',
   '8189ftv':'BR2_PACKAGE_RTK8189FTV_GIT_REPO_URL',
   '8723bs':'BR2_PACKAGE_RTK8723BS_GIT_REPO_URL',
   '8723au':'BR2_PACKAGE_RTK8723AU_GIT_REPO_URL',
   '8723bu':'BR2_PACKAGE_RTK8723BU_GIT_REPO_URL',
   '8811au':'BR2_PACKAGE_RTK8811AU_GIT_REPO_URL',
   '8812au':'BR2_PACKAGE_RTK8812AU_GIT_REPO_URL',
   '8822bu':'BR2_PACKAGE_RTK8822BU_GIT_REPO_URL',
   'ap6xxx':'BR2_PACKAGE_BRCMAP6XXX_GIT_REPO_URL',
   'mtk7601':'BR2_PACKAGE_MTK7601_GIT_REPO_URL',
   'mtk7603':'BR2_PACKAGE_MTK7603_GIT_REPO_URL',
   'usi':'BR2_PACKAGE_BRCMUSI_GIT_REPO_URL',
   'tvin':'BR2_PACKAGE_AML_TVIN_GIT_REPO_URL',
   'pmu':'BR2_PACKAGE_AML_PMU_GIT_REPO_URL',
   'thermal':'BR2_PACKAGE_AML_THERMAL_URL',
   'touch':'BR2_PACKAGE_AML_TOUCH_GIT_REPO_URL',
   'nand':'BR2_PACKAGE_AML_NAND_GIT_URL',
   'uboot':'BR2_TARGET_UBOOT_CUSTOM_REPO_URL', 
   'uboot_customer':'BR2_PACKAGE_AML_UBOOT_CUSTOMER_GIT_REPO_URL'
}

repos = { 
   'customer':'kernel/customer', 
   'kernel':'kernel/common', 
   'gpu':'platform/hardware/arm/gpu', 
   'wifi':'platform/hardware/amlogic/wifi', 
   '8188eu':'platform/hardware/wifi/realtek/drivers/8188eu', 
   '8192cu':'platform/hardware/wifi/realtek/drivers/8192cu', 
   '8192du':'platform/hardware/wifi/realtek/drivers/8192du', 
   '8192eu':'platform/hardware/wifi/realtek/drivers/8192eu', 
   '8192es':'platform/hardware/wifi/realtek/drivers/8192es', 
   '8189es':'platform/hardware/wifi/realtek/drivers/8189es', 
   '8189ftv':'platform/hardware/wifi/realtek/drivers/8189ftv', 
   '8723bs':'platform/hardware/wifi/realtek/drivers/8723bs', 
   '8723au':'platform/hardware/wifi/realtek/drivers/8723au', 
   '8723bu':'platform/hardware/wifi/realtek/drivers/8723bu', 
   '8811au':'platform/hardware/wifi/realtek/drivers/8811au', 
   '8812au':'platform/hardware/wifi/realtek/drivers/8812au', 
   '8822bu':'platform/hardware/wifi/realtek/drivers/8822bu',
   'ap6xxx':'platform/hardware/wifi/broadcom/drivers/ap6xxx',
   'mtk7601':'platform/hardware/wifi/mtk/drivers/mtk7601',
   'mtk7603':'platform/hardware/wifi/mtk/drivers/mtk7603',
   'usi':'platform/hardware/wifi/broadcom/drivers/usi', 
   'tvin':'linux/amlogic/tvin', 
   'pmu':'platform/hardware/amlogic/pmu', 
   'thermal':'platform/hardware/amlogic/thermal', 
   'touch':'platform/hardware/amlogic/touch', 
   'nand':'platform/hardware/amlogic/nand', 
   'uboot':'uboot', 
   'uboot_customer':'uboot/customer'
}

branches = { 
    'customer':'BR2_PACKAGE_AML_CUSTOMER_VERSION',
    'kernel':'BR2_LINUX_KERNEL_CUSTOM_REPO_VERSION',
    'wifi':'BR2_PACKAGE_WIFI_CUSTOM_GIT_VERSION',
    '8188eu':'BR2_PACKAGE_RTK8188EU_GIT_VERSION',
    '8192cu':'BR2_PACKAGE_RTK8192CU_GIT_VERSION',
    '8192du':'BR2_PACKAGE_RTK8192DU_GIT_VERSION',
    '8192eu':'BR2_PACKAGE_RTK8192EU_GIT_VERSION',
    '8192es':'BR2_PACKAGE_RTK8192ES_GIT_VERSION',
    '8189es':'BR2_PACKAGE_RTK8189ES_GIT_VERSION',
    '8189ftv':'BR2_PACKAGE_RTK8189FTV_GIT_VERSION',
    '8723bs':'BR2_PACKAGE_RTK8723BS_GIT_VERSION',
    '8723au':'BR2_PACKAGE_RTK8723AU_GIT_VERSION',
    '8723bu':'BR2_PACKAGE_RTK8723BU_GIT_VERSION',
    '8811au':'BR2_PACKAGE_RTK8811AU_GIT_VERSION',
    '8812au':'BR2_PACKAGE_RTK8812AU_GIT_VERSION',
    '8822bu':'BR2_PACKAGE_RTK8822BU_GIT_VERSION',
    'ap6xxx':'BR2_PACKAGE_BRCMAP6XXX_GIT_VERSION',
    'mtk7601':'BR2_PACKAGE_MTK7601_GIT_VERSION',
    'mtk7603':'BR2_PACKAGE_MTK7603_GIT_VERSION',
    'usi':'BR2_PACKAGE_BRCMUSI_GIT_VERSION',
    'tvin':'BR2_PACKAGE_AML_TVIN_GIT_VERSION',
    'pmu':'BR2_PACKAGE_AML_PMU_GIT_VERSION',
    'thermal':'BR2_PACKAGE_AML_THERMAL_VERSION',
    'touch':'BR2_PACKAGE_AML_TOUCH_GIT_VERSION',
    'nand':'BR2_PACKAGE_AML_NAND_VERSION',
    'uboot':'BR2_TARGET_UBOOT_CUSTOM_REPO_VERSION',
    'uboot_customer':'BR2_PACKAGE_AML_UBOOT_CUSTOMER_GIT_VERSION'
}

tar = { 
   'customer':'aml_customer',
   'kernel':'arm-src-kernel',
   'gpu':'gpu',
   'wifi':'wifi-fw',
   '8188eu':'rtk8188eu',
   '8192cu':'rtk8192cu',
   '8192du':'rtk8192du',
   '8192eu':'rtk8192eu',
   '8192es':'rtk8192es',
   '8189es':'rtk8189es',
   '8189ftv':'rtk8189ftv',
   '8723bs':'rtk8723bs',
   '8723au':'rtk8723au',
   '8723bu':'rtk8723bu',
   '8811au':'rtk8811au',
   '8812au':'rtk8812au',
   '8822bu':'rtk8822bu',
   'ap6xxx':'brcmap6xxx',
   'mtk7601':'mtk7601',
   'mtk7603':'mtk7603',
   'usi':'brcmusi',
   'tvin':'aml_tvin',
   'pmu':'aml_pmu',
   'thermal':'aml_thermal',
   'touch':'aml_touch',
   'nand':'aml_nand',
   'uboot':'uboot',
   'uboot_customer':'aml_uboot_customer'
}

location = { 
   'customer':'customer',
   'kernel':'kernel',
   'gpu':'gpu',
   'wifi':'wifi',
   '8188eu':'wifi',
   '8192cu':'wifi',
   '8192du':'wifi',
   '8192eu':'wifi',
   '8192es':'wifi',
   '8189es':'wifi',
   '8189ftv':'wifi',
   '8723bs':'wifi',
   '8723au':'wifi',
   '8723bu':'wifi',
   '8811au':'wifi',
   '8812au':'wifi',
   '8822bu':'wifi',
   'ap6xxx':'wifi',
   'mtk7601':'wifi',
   'mtk7603':'wifi',
   'usi':'wifi',
   'tvin':'modules',
   'pmu':'modules',
   'thermal':'modules',
   'touch':'modules',
   'nand':'modules',
   'uboot':'u-boot',
   'uboot_customer':'u-boot'
}

tarball_cfg = {
   'kernel':'BR2_LINUX_KERNEL_CUSTOM_TARBALL_LOCATION BR2_LINUX_KERNEL_CUSTOM_TARBALL', 
   'gpu':'BR2_PACKAGE_GPU_CUSTOM_TARBALL_LOCATION', 
   'uboot':'BR2_TARGET_UBOOT_CUSTOM_TARBALL_LOCATION BR2_TARGET_UBOOT_CUSTOM_TARBALL'
}

drop = {
        'kernel':'BR2_LINUX_KERNEL_CUSTOM_GIT',
        'uboot':'BR2_TARGET_UBOOT_CUSTOM_GIT'
}

filename = dict()

def getserver(string):
    server = re.compile('<remote fetch=\"(.*)\" name=.*/>') 
    string = string.strip()
    loc = re.match(server, string)
    if loc != None:
        return loc.group(1)

def getpkg(string):
    rev = re.compile('<project name=\"([a-z0-9/]*)\".* revision=\"([a-z0-9]*)\" upstream=\"(.*)\"/>')
    string = string.strip()
    loc = re.match(rev, string)
    if loc != None:
        return loc.group(1), loc.group(2), loc.group(3)
    else:
        rev = re.compile('<project name=\"([a-z0-9/]*)\".* revision=\"([a-z0-9_]*)\"/>')
        loc = re.match(rev, string)
        if loc != None:
            return loc.group(1), loc.group(2), loc.group(2)
        else:
            rev = re.compile('<project dest-branch=\"([a-z0-9\-\.]*)\".* name=\"([a-z0-9/]*)\".* path=\".*\" revision=\"([a-z0-9]*)\" upstream=\"(.*)\"/>')
            loc = re.match(rev, string)
            if loc != None:
                return loc.group(2), loc.group(3), loc.group(1)
            else:
                rev = re.compile('<project dest-branch=\"([a-z0-9\-\.]*)\".* name=\"([a-z0-9/]*)\".* revision=\"([a-z0-9]*)\" upstream=\"(.*)\"/>')
                loc = re.match(rev, string)
                if loc != None:
                    return loc.group(2), loc.group(3), loc.group(1)

def download_pkg(xml, config, download = 0):
    server_addr = None
    for pkgline in open(config, 'r'):
        for i in pkg.keys():
            if pkg[i] in pkgline:
              for line in open(xml, 'r'):
                  if server_addr == None:
                      server_addr = getserver(line)
                  else:
                      if repos[i] in line:
                          name, pkgloc, b = getpkg(line)
                          branch = re.compile('refs/heads/([a-zA-Z0-9\-]*)')
                          loc = re.match(branch, b)
                          if loc != None:
                              b = loc.group(1)
                          if name != repos[i]:
                              break
                          server = server_addr + repos[i]
                          date = time.strftime("%Y-%m-%d")
                          folder = tar[i] + '-' + date + '-' + pkgloc[0:10]
                          if download:
                              cmd = 'rm -rf %s' % folder
                              print cmd
                              os.system(cmd)
                              cmd = 'git clone --depth=0 %s -b %s %s' % (server, b, folder)
                              print cmd
                              os.system(cmd)
                              cmd = 'cd %s; git checkout %s' % (folder, pkgloc)
                              print cmd
                              os.system(cmd)
                              cmd = 'cd %s; git archive --format=tar.gz %s --prefix=%s/ -o %s.tar.gz' % (folder, pkgloc, folder, folder)
                              print cmd
                              os.system(cmd)
                              cmd = 'mv %s/%s.tar.gz .' % (folder, folder)
                              print cmd
                              os.system(cmd)
                          filename[i] = folder + ".tar.gz"

def create_cfg(config):
    newfile = config.replace('defconfig', 'release_defconfig', 1)
    f = open(newfile, 'w')
    for pkgline in open(config, 'r'):
        for i in pkg.keys():
            if pkg[i] in pkgline:
                if pkgline[0] == '#': # ignore comments
                    break
                if tarball_cfg.has_key(i) != False:
                    cfg = tarball_cfg[i].split()
                    new_cfg = '%s=\"%s\"\n' % (cfg[0], base_url + location[i] + "/" + filename[i])
                    check = None
                    if len(cfg) > 1:
                        check = '%s=y\n' % cfg[1]
                        f.write(check)
                    f.write(new_cfg)
                    # hack to add uboot patch dir
                    if i == 'uboot':
                        hash_name = filename[i][17:-7]
                        patch_cfg = 'BR2_TARGET_UBOOT_PATCH=\"boot/uboot/%s\"\n' % hash_name 
                        f.write(patch_cfg)
                else:
                    cfg = pkgline.split('=')
                    new_cfg = '%s=\"%s\"\n' % (cfg[0], base_url + location[i])
                    version = filename[i].replace(tar[i] + '-', '', 1)
                    version = version.replace('.tar.gz', '', 1)
                    version = '%s=\"%s\"\n' % (branches[i], version)
                    f.write(version)
                    f.write(new_cfg)
                break
            elif i in branches and branches[i] in pkgline:
                break
            elif i in drop and drop[i] in pkgline:
                break
        else:
            f.write(pkgline)
    f.close()

if __name__ == '__main__':
    xml = sys.argv[1]
    cfg = sys.argv[2]
    download = len(sys.argv) == 4 and sys.argv[3] == "download"
    download_pkg(xml, cfg, download)
    create_cfg(cfg)

