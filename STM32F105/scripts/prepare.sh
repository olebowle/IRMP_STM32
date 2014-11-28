#!/bin/bash
mkdir -p ext_src
[[ -e ./ext_src/prepared ]] && exit 0
cd ./ext_src
for i in stsw-stm32121 stm32_f105-07_f2_f4_usb-host-device_lib stsw-stm32010; do
	if [[ ! -s $i.zip ]]; then
		wget "http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/firmware/$i.zip"
	fi
done
if [[ ! -s irmp.tar.gz ]]; then
	wget "http://www.mikrocontroller.net/svnbrowser/irmp/?view=tar" -O irmp.tar.gz
fi

cd ..
ar2='../../ext_src/stsw-stm32121.zip'
ar1='../ext_src/stsw-stm32121.zip'
ver='4.0.0'
path="STM32_USB-FS-Device_Lib_V$ver"
mkdir -p cmsis_boot
cd cmsis_boot
unzip -j $ar1 $path/Libraries/CMSIS/Device/ST/STM32F10x/Include/stm32f10x.h
unzip -j $ar1 $path/Libraries/CMSIS/Device/ST/STM32F10x/Include/system_stm32f10x.h
unzip -j $ar1 $path/Projects/Custom_HID/inc/stm32f10x_conf.h
mkdir -p startup
cd startup
unzip -j $ar2 $path/Libraries/CMSIS/Device/ST/STM32F10x/Source/Templates/gcc_ride7/startup_stm32f10x_cl.s
cd ../..
mkdir -p cmsis
cd cmsis
unzip -j $ar1 $path/Libraries/CMSIS/Include/core_cm3.h
unzip -j $ar1 $path/Libraries/CMSIS/Include/core_cmFunc.h
unzip -j $ar1 $path/Libraries/CMSIS/Include/core_cmInstr.h
cd ..
mkdir -p stm_lib
cd  stm_lib
mkdir -p inc
cd inc
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/misc.h
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/stm32f10x_flash.h
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/stm32f10x_gpio.h
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/stm32f10x_rcc.h
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/stm32f10x_tim.h
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/inc/stm32f10x_usart.h
cd ..
mkdir -p src
cd src
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/misc.c
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c
unzip -j $ar2 $path/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c
cd ../..

ar2='../../ext_src/stm32_f105-07_f2_f4_usb-host-device_lib.zip'
ar1='../ext_src/stm32_f105-07_f2_f4_usb-host-device_lib.zip'
ver='2.1.0'
path="STM32_USB-Host-Device_Lib_V$ver"
cd cmsis_boot
unzip -j $ar1 $path/Libraries/CMSIS/Device/ST/STM32F10x/Source/Templates/system_stm32f10x.c
cd ..
mkdir -p usb_hid
cd usb_hid
mkdir -p inc
cd inc
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Class/hid/inc/usbd_hid_core.h
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/inc/usbd_core.h
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/inc/usbd_def.h
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/inc/usbd_ioreq.h
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/inc/usbd_req.h
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/inc/usbd_usr.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_bsp.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_core.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_dcd.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_dcd_int.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_defines.h
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/inc/usb_regs.h
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/inc/usb_conf.h
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/inc/usbd_conf.h
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/inc/usbd_desc.h
cd ..
mkdir -p src 
cd src
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Class/hid/src/usbd_hid_core.c
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/src/usbd_core.c
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/src/usbd_ioreq.c
unzip -j $ar2 $path/Libraries/STM32_USB_Device_Library/Core/src/usbd_req.c
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/src/usb_core.c
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/src/usb_dcd.c
unzip -j $ar2 $path/Libraries/STM32_USB_OTG_Driver/src/usb_dcd_int.c
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/src/usb_bsp.c
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/src/usbd_desc.c
unzip -j $ar2 $path/Project/USB_Device_Examples/HID/src/usbd_usr.c

ar='../../ext_src/stsw-stm32010.zip'
ver='3.1.0'
path="STM32F10x_AN2594_FW_V$ver"
cd ../..
cd  stm_lib
cd inc
unzip -j $ar $path/Project/EEPROM_Emulation/inc/eeprom.h
cd ..
cd src
unzip -j $ar $path/Project/EEPROM_Emulation/src/eeprom.c

cd ../..
pwd
cd usb_hid
pwd
patch -p1 -i ../patches/usb_hid.patch
cd ..
cd stm_lib
patch -p1 -i ../patches/eeprom.patch
cd ..
cd cmsis_boot
patch -p1 -i ../patches/stm32f10x_conf.patch
cd ..

ar='../ext_src/irmp.tar.gz'
path="irmp"
mkdir -p irmp
cd irmp
tar -xvf $ar --strip-components=1 $path/irmp.c
tar -xvf $ar --strip-components=1 $path/irmp.h
tar -xvf $ar --strip-components=1 $path/irmpconfig.h
tar -xvf $ar --strip-components=1 $path/irmpprotocols.h
tar -xvf $ar --strip-components=1 $path/irmpsystem.h
tar -xvf $ar --strip-components=1 $path/irsnd.c
tar -xvf $ar --strip-components=1 $path/irsnd.h
tar -xvf $ar --strip-components=1 $path/irsndconfig.h
patch -p1 -i ../patches/irmp.patch

touch ../ext_src/prepared