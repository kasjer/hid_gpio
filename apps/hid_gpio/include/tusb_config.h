/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#include "syscfg/syscfg.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * COMMON CONFIGURATION
 */

#include <tusb_hw.h>

/* defined by compiler flags for flexibility */
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#if MYNEWT_VAL_CHOICE(USBD_RHPORT, USB0)
#undef CFG_TUSB_RHPORT0_MODE
#if MYNEWT_VAL(USBD_HIGH_SPEED)
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#else
#define CFG_TUSB_RHPORT0_MODE  (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif
#define USBD_RHPORT_MODE CFG_TUSB_RHPORT0_MODE

#elif MYNEWT_VAL_CHOICE(USBD_RHPORT, USB1)
#undef CFG_TUSB_RHPORT1_MODE
#if MYNEWT_VAL(USBD_HIGH_SPEED)
#define CFG_TUSB_RHPORT1_MODE  (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)
#else
#define CFG_TUSB_RHPORT1_MODE  (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif
#define USBD_RHPORT_MODE CFG_TUSB_RHPORT1_MODE
#endif

#if MYNEWT_VAL(OS_SCHEDULING)
#define CFG_TUSB_OS                 OPT_OS_MYNEWT
#else
#define CFG_TUSB_OS                 OPT_OS_NONE
#endif
#define CFG_TUSB_DEBUG              MYNEWT_VAL(CFG_TUSB_DEBUG)

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif

/**
 * DEVICE CONFIGURATION
 */
#define CFG_TUD_ENDPOINT0_SIZE   MYNEWT_VAL(USBD_EP0_SIZE)

/* ------------- CLASS ------------- */
/*
 * If CDC_CONSOLE does not have specific values for endpoint configuration,
 * use values for unspecified CDC
 */
#if defined(MYNEWT_VAL_USBD_CDC_CONSOLE_NOTIFY_EP_SIZE)
#define USBD_CDC_CONSOLE_NOTIFY_EP_SIZE MYNEWT_VAL(USBD_CDC_CONSOLE_NOTIFY_EP_SIZE)
#else
#define USBD_CDC_CONSOLE_NOTIFY_EP_SIZE USBD_CDC_NOTIFY_EP_SIZE
#endif

#if defined(MYNEWT_VAL_USBD_CDC_CONSOLE_NOTIFY_EP)
#define USBD_CDC_CONSOLE_NOTIFY_EP      MYNEWT_VAL(USBD_CDC_CONSOLE_NOTIFY_EP)
#else
#define USBD_CDC_CONSOLE_NOTIFY_EP      USBD_CDC_NOTIFY_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_CONSOLE_DATA_OUT_EP)
#define USBD_CDC_CONSOLE_DATA_OUT_EP    MYNEWT_VAL(USBD_CDC_CONSOLE_DATA_OUT_EP)
#else
#define USBD_CDC_CONSOLE_DATA_OUT_EP    USBD_CDC_DATA_OUT_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_CONSOLE_DATA_IN_EP)
#define USBD_CDC_CONSOLE_DATA_IN_EP     MYNEWT_VAL(USBD_CDC_CONSOLE_DATA_IN_EP)
#else
#define USBD_CDC_CONSOLE_DATA_IN_EP     USBD_CDC_DATA_IN_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_CONSOLE_DATA_EP_SIZE)
#define USBD_CDC_CONSOLE_DATA_EP_SIZE   MYNEWT_VAL(USBD_CDC_CONSOLE_DATA_EP_SIZE)
#else
#define USBD_CDC_CONSOLE_DATA_EP_SIZE   USBD_CDC_DATA_EP_SIZE
#endif

/*
 * If CDC_HCI does not have specific values for endpoint configuration,
 * use values for unspecified CDC
 */
#if defined(MYNEWT_VAL_USBD_CDC_HCI_NOTIFY_EP)
#define USBD_CDC_HCI_NOTIFY_EP      MYNEWT_VAL(USBD_CDC_HCI_NOTIFY_EP)
#else
#define USBD_CDC_HCI_NOTIFY_EP      USBD_BTH_EVENT_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_HCI_NOTIFY_EP_SIZE)
#define USBD_CDC_HCI_NOTIFY_EP_SIZE MYNEWT_VAL(USBD_CDC_HCI_NOTIFY_EP_SIZE)
#else
#define USBD_CDC_HCI_NOTIFY_EP_SIZE USBD_CDC_NOTIFY_EP_SIZE
#endif

#if defined(MYNEWT_VAL_USBD_CDC_HCI_DATA_OUT_EP)
#define USBD_CDC_HCI_DATA_OUT_EP    MYNEWT_VAL(USBD_CDC_HCI_DATA_OUT_EP)
#else
#define USBD_CDC_HCI_DATA_OUT_EP    USBD_BTH_DATA_OUT_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_HCI_DATA_IN_EP)
#define USBD_CDC_HCI_DATA_IN_EP     MYNEWT_VAL(USBD_CDC_HCI_DATA_IN_EP)
#else
#define USBD_CDC_HCI_DATA_IN_EP     USBD_BTH_DATA_IN_EP
#endif

#if defined(MYNEWT_VAL_USBD_CDC_HCI_DATA_EP_SIZE)
#define USBD_CDC_HCI_DATA_EP_SIZE   MYNEWT_VAL(USBD_CDC_HCI_DATA_EP_SIZE)
#else
#define USBD_CDC_HCI_DATA_EP_SIZE   USBD_CDC_DATA_EP_SIZE
#endif


#if MYNEWT_VAL(USBD_CDC)
#define CFG_CDC                  MYNEWT_VAL(USBD_CDC)
#else
#define CFG_CDC                  0
#endif

#if MYNEWT_VAL(CONSOLE_USB)
#define CFG_CDC_CONSOLE          MYNEWT_VAL(CONSOLE_USB)
#else
#define CFG_CDC_CONSOLE          0
#endif

#if MYNEWT_VAL(USBD_CDC_HCI)
#define CFG_CDC_HCI              MYNEWT_VAL(USBD_CDC_HCI)
#else
#define CFG_CDC_HCI              0
#endif

#define CFG_TUD_CDC              ((CFG_CDC) + (CFG_CDC_CONSOLE) + (CFG_CDC_HCI))

#if MYNEWT_VAL(USBD_HID)
#define CFG_TUD_HID              MYNEWT_VAL(USBD_HID)
#else
#define CFG_TUD_HID              0
#endif
#if MYNEWT_VAL(USBD_MSC)
#define CFG_TUD_MSC              MYNEWT_VAL(USBD_MSC)
#else
#define CFG_TUD_MSC               0
#endif
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0
#define CFG_TUD_USBTMC           0
#define CFG_TUD_DFU_RT           0
#if MYNEWT_VAL(USBD_DFU)
#define CFG_TUD_DFU              MYNEWT_VAL(USBD_DFU)
#else
#define CFG_TUD_DFU              0
#endif
#if MYNEWT_VAL(USBD_DFU_RUNTIME)
#define CFG_TUD_DFU_RUNTIME      MYNEWT_VAL(USBD_DFU_RUNTIME)
#else
#define CFG_TUD_DFU_RUNTIME      0
#endif
#if MYNEWT_VAL(USBD_BTH)
#define CFG_TUD_BTH              MYNEWT_VAL(USBD_BTH)
#else
#define CFG_TUD_BTH              0
#endif

/* Minimal number for alternative interfaces that is recognized by Windows as Bluetooth radio controller */
#define CFG_TUD_BTH_ISO_ALT_COUNT 2

#if MYNEWT_VAL(USBD_CDC_RX_BUFSIZE)
#define CFG_TUD_CDC_RX_BUFSIZE   MYNEWT_VAL(USBD_CDC_RX_BUFSIZE)
#else
#define CFG_TUD_CDC_RX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif

#if MYNEWT_VAL(USBD_CDC_TX_BUFSIZE)
#define CFG_TUD_CDC_TX_BUFSIZE   MYNEWT_VAL(USBD_CDC_TX_BUFSIZE)
#else
#define CFG_TUD_CDC_TX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif

/* HID buffer size Should be sufficient to hold ID (if any) + Data */
#define CFG_TUD_HID_BUFSIZE      16

/* MSC Buffer size of Device Mass storage */
#if MYNEWT_VAL(USBD_MSC_EP_BUFSIZE)
#define CFG_TUD_MSC_EP_BUFSIZE   MYNEWT_VAL(USBD_MSC_EP_BUFSIZE)
#else
#define CFG_TUD_MSC_EP_BUFSIZE   512
#endif

#ifndef CFG_TUD_DFU_XFER_BUFSIZE
#define CFG_TUD_DFU_XFER_BUFSIZE    MYNEWT_VAL(USBD_DFU_BLOCK_SIZE)
#endif

#if MYNEWT_VAL(USBD_DFU_DETACH_TIMEOUT)
#define CFG_TUD_DFU_DETACH_TIMEOUT  MYNEWT_VAL(USBD_DFU_DETACH_TIMEOUT)
#else
#define CFG_TUD_DFU_DETACH_TIMEOUT  1000
#endif

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
