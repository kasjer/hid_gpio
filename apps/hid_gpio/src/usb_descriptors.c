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

#include <syscfg/syscfg.h>
#include <bsp/bsp.h>
#include <string.h>
#include <tusb.h>
#include <device/usbd.h>
#include <os/util.h>
#include <console/console.h>
#include <hal/hal_gpio.h>
#include <hal/hal_bsp.h>

#define USBD_PRODUCT_RELEASE_NUMBER MYNEWT_VAL(USBD_PRODUCT_RELEASE_NUMBER)

#ifndef CONFIG_NUM
#define CONFIG_NUM 1
#endif

#define CDC_IF_STR_IX (MYNEWT_VAL(USBD_CDC_DESCRIPTOR_STRING) == NULL ? 0 : 4)
#define MSC_IF_STR_IX (MYNEWT_VAL(USBD_MSC_DESCRIPTOR_STRING) == NULL ? 0 : 5)
#define HID_IF_STR_IX (MYNEWT_VAL(USBD_HID_DESCRIPTOR_STRING) == NULL ? 0 : 6)
#define BTH_IF_STR_IX (MYNEWT_VAL(USBD_BTH_DESCRIPTOR_STRING) == NULL ? 0 : 7)
#define DFU_SLOT_NAME_IF_STR_IX 8
#define CDC_CONSOLE_IF_STR_IX 0

#if defined(MYNEWT_VAL_USBD_DFU_SLOT_NAME)
#define USBD_DFU_SLOT_NAME  MYNEWT_VAL(USBD_DFU_SLOT_NAME)
#else
#define USBD_DFU_SLOT_NAME  NULL
#endif

#ifndef CFG_TUD_DFU_DETACH_TIMEOUT
#define CFG_TUD_DFU_DETACH_TIMEOUT  1000
#endif

#if CFG_TUD_HID

#define HID_REPORT_DESC() \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                    ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD )                    ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,\
    HID_REPORT_ID(1)\
    /* Output 5-bit LED Indicator Kana | Compose | ScrollLock | CapsLock | NumLock */ \
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_LED                   )       ,\
      HID_USAGE_MIN    ( 1                                       ) ,\
      HID_USAGE_MAX    ( 5                                       ) ,\
      HID_REPORT_COUNT ( 5                                       ) ,\
      HID_REPORT_SIZE  ( 1                                       ) ,\
      HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ) ,\
      /* led padding */ \
      HID_REPORT_COUNT ( 1                                       ) ,\
      HID_REPORT_SIZE  ( 3                                       ) ,\
      HID_OUTPUT       ( HID_CONSTANT                            ) ,\
  HID_COLLECTION_END, \
  HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2)                     ,\
  HID_USAGE      ( 0 )                                             ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,\
    /* Report ID if any */\
    HID_REPORT_ID(5)\
      HID_USAGE_MIN    ( 0                                   )     ,\
      HID_USAGE_MAX    ( 0xFF                                )     ,\
      HID_LOGICAL_MIN  ( 0                                   )     ,\
      HID_LOGICAL_MAX  ( 0xFF                                )     ,\
      HID_REPORT_COUNT ( 8                                   )     ,\
      HID_REPORT_SIZE  ( 8                                   )     ,\
      HID_FEATURE      ( HID_DATA | HID_ARRAY | HID_ABSOLUTE )     ,\
  HID_COLLECTION_END \

const uint8_t desc_hid_report[] = {
    HID_REPORT_DESC(),
};

/*
 * Invoked when received GET HID REPORT DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
const uint8_t *
tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void)itf;
    return desc_hid_report;
}

#endif /* CFG_TUD_HID */

/* Microsoft OS 2.0 descriptors for NCM, they need BOS (USB 2.01) */
#define USBD_MS_OS20    (CFG_TUD_NCM && MYNEWT_VAL(USBD_NET_MS_OS20))

#if USBD_MS_OS20
#define USB_BCD     tu_htole16(0x201)
#else
#define USB_BCD     tu_htole16(0x200)
#endif

const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    .bDeviceClass       = MYNEWT_VAL(USBD_DEVICE_CLASS),
    .bDeviceSubClass    = MYNEWT_VAL(USBD_DEVICE_SUBCLASS),
    .bDeviceProtocol    = MYNEWT_VAL(USBD_DEVICE_PROTOCOL),

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = MYNEWT_VAL(USBD_VID),
    .idProduct          = MYNEWT_VAL(USBD_PID),
    .bcdDevice          = USBD_PRODUCT_RELEASE_NUMBER,

    .iManufacturer      = 0x02,
    .iProduct           = 0x03,
    .iSerialNumber      = 0x01,

    .bNumConfigurations = 0x01
};

#if TUD_OPT_HIGH_SPEED

/*
 * device qualifier is mostly similar to device descriptor since we don't change configuration based on speed.
 */
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength            = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB             = USB_BCD,

#if CFG_TUD_BTH
    .bDeviceClass       = TUD_BT_APP_CLASS,
    .bDeviceSubClass    = TUD_BT_APP_SUBCLASS,
    .bDeviceProtocol    = TUD_BT_PROTOCOL_PRIMARY_CONTROLLER,
#elif CFG_TUD_CDC || CFG_TUD_AUDIO || USBD_NET_ENABLED
    /*
     * Use Interface Association Descriptor (IAD) for CDC, Audio and Network
     * As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
     */
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
#else
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
#endif

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00,
};

/*
 * Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
 * device_qualifier descriptor describes information about a high-speed capable device that would
 * change if the device were operating at the other speed. If not highspeed capable stall this request.
 */
const uint8_t *
tud_descriptor_device_qualifier_cb(void)
{
    return (const uint8_t *)&desc_device_qualifier;
}

#endif

/*
 * Invoked when received GET DEVICE DESCRIPTOR
 * Application return pointer to descriptor
 */
const uint8_t *
tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/*
 * Configuration Descriptor
 */

enum {
#if CFG_TUD_BTH
    ITF_NUM_BTH,
#if CFG_TUD_BTH_ISO_ALT_COUNT > 0
    ITF_NUM_BTH_VOICE,
#endif
#endif

#if USBD_NET_ENABLED
    /* Windows expects RNDIS to be the first function */
    ITF_NUM_NET,
    ITF_NUM_NET_DATA,
#endif

#if CFG_CDC_CONSOLE
    ITF_NUM_CDC_CONSOLE,
    ITF_NUM_CDC_CONSOLE_DATA,
#endif

#if CFG_CDC_HCI
    ITF_NUM_CDC_HCI,
    ITF_NUM_CDC_HCI_DATA,
#endif

#if CFG_CDC
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
#endif

#if CFG_TUD_MSC
    ITF_NUM_MSC,
#endif

#if CFG_TUD_HID
    ITF_NUM_HID,
#endif

#if CFG_TUD_AUDIO
    ITF_NUM_AUDIO_CONTROL,
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
    ITF_NUM_AUDIO_STREAMING_OUT,
#endif
#if CFG_TUD_AUDIO_ENABLE_EP_IN
    ITF_NUM_AUDIO_STREAMING_IN,
#endif
#endif

#if CFG_TUD_DFU
    ITF_NUM_DFU,
#endif

#if CFG_TUD_DFU_RUNTIME
    ITF_NUM_DFU_RUNTIME,
#endif

    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + \
                             CFG_CDC * TUD_CDC_DESC_LEN + \
                             CFG_CDC_CONSOLE * TUD_CDC_DESC_LEN + \
                             CFG_CDC_HCI * TUD_CDC_DESC_LEN + \
                             CFG_TUD_MSC * TUD_MSC_DESC_LEN + \
                             CFG_TUD_HID * TUD_HID_DESC_LEN + \
                             CFG_TUD_BTH * TUD_BTH_DESC_LEN + \
                             CFG_TUD_DFU * TUD_DFU_DESC_LEN(1) + \
                             CFG_TUD_DFU_RUNTIME * TUD_DFU_RT_DESC_LEN + \
                             0)

const uint8_t desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(CONFIG_NUM, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                          MYNEWT_VAL(USBD_CONFIGURATION_MAX_POWER)),

#if CFG_TUD_BTH
    TUD_BTH_DESCRIPTOR(ITF_NUM_BTH, BTH_IF_STR_IX, USBD_BTH_EVENT_EP, USBD_BTH_EVENT_EP_SIZE,
                       USBD_BTH_EVENT_EP_INTERVAL, USBD_BTH_DATA_IN_EP, USBD_BTH_DATA_OUT_EP,
                       (USBD_RHPORT_MODE & OPT_MODE_HIGH_SPEED) ? 512 : USBD_BTH_DATA_EP_SIZE,
                       0, 9, 17, 25, 33, 49),
#endif

#if CFG_CDC_CONSOLE
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_CONSOLE, CDC_CONSOLE_IF_STR_IX, USBD_CDC_CONSOLE_NOTIFY_EP,
                       USBD_CDC_CONSOLE_NOTIFY_EP_SIZE, USBD_CDC_CONSOLE_DATA_OUT_EP, USBD_CDC_CONSOLE_DATA_IN_EP,
                       (USBD_RHPORT_MODE & OPT_MODE_HIGH_SPEED) ? 512 : USBD_CDC_CONSOLE_DATA_EP_SIZE),
#endif

#if CFG_CDC_HCI
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_HCI, CDC_HCI_IF_STR_IX, USBD_CDC_HCI_NOTIFY_EP, USBD_CDC_HCI_NOTIFY_EP_SIZE,
                       USBD_CDC_HCI_DATA_OUT_EP, USBD_CDC_HCI_DATA_IN_EP,
                       (USBD_RHPORT_MODE & OPT_MODE_HIGH_SPEED) ? 512 : USBD_CDC_HCI_DATA_EP_SIZE),
#endif

#if CFG_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, CDC_IF_STR_IX, USBD_CDC_NOTIFY_EP, USBD_CDC_NOTIFY_EP_SIZE,
                       USBD_CDC_DATA_OUT_EP, USBD_CDC_DATA_IN_EP,
                       (USBD_RHPORT_MODE & OPT_MODE_HIGH_SPEED) ? 512 : USBD_CDC_DATA_EP_SIZE),
#endif

#if CFG_TUD_MSC
    /* TODO: MSC not handled yet */
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, MSC_IF_STR_IX, USBD_MSC_DATA_OUT_EP, USBD_MSC_DATA_IN_EP,
                       (USBD_RHPORT_MODE & OPT_MODE_HIGH_SPEED) ? 512 : 64),
#endif

#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, HID_IF_STR_IX, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                       USBD_HID_REPORT_EP, USBD_HID_REPORT_EP_SIZE, USBD_HID_REPORT_EP_INTERVAL),
#endif

#if CFG_TUD_DFU
    TUD_DFU_DESCRIPTOR(ITF_NUM_DFU, 1, DFU_SLOT_NAME_IF_STR_IX, DFU_ATTR_CAN_DOWNLOAD,
                       CFG_TUD_DFU_DETACH_TIMEOUT, CFG_TUD_DFU_XFER_BUFSIZE),
#endif

#if CFG_TUD_DFU_RUNTIME
    TUD_DFU_RT_DESCRIPTOR(ITF_NUM_DFU_RUNTIME, DFU_SLOT_NAME_IF_STR_IX, DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_WILL_DETACH,
                          CFG_TUD_DFU_DETACH_TIMEOUT, CFG_TUD_DFU_XFER_BUFSIZE),
#endif
};

/* Descriptor is composed from many parts, check declared length. */
TU_VERIFY_STATIC(sizeof(desc_configuration) == CONFIG_TOTAL_LEN,
                 "Configuration descriptor length mismatch");

/**
 * Invoked when received GET CONFIGURATION DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
const uint8_t *
tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;

    return desc_configuration;
}

const char *string_desc_arr[] = {
    MYNEWT_VAL(USBD_VENDOR_STRING),
    MYNEWT_VAL(USBD_PRODUCT_STRING),
    NULL,
    NULL,
    MYNEWT_VAL(USBD_HID_DESCRIPTOR_STRING),
    NULL,
#ifdef MYNEWT_VAL_USBD_DFU_SLOT_NAME
    MYNEWT_VAL_USBD_DFU_SLOT_NAME,
#else
    NULL
#endif
};

static uint16_t desc_string[MYNEWT_VAL(USBD_STRING_DESCRIPTOR_MAX_LENGTH) + 1];

uint16_t *desc_string_from_ascii(uint16_t *uchars, size_t uchar_count, const char *astr)
{
    if (astr == NULL) {
        uchars = NULL;
    } else {
        size_t len = strlen(astr);
        if (len > uchar_count - 1) {
            len = uchar_count - 1;
        }
        for (size_t i = 0; i < len; ++i) {
            uchars[i + 1] = astr[i];
        }
        if (len > 0) {
            uchars[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);
        } else {
            uchars = NULL;
        }
    }

    return uchars;
}

static uint8_t
hex_digit(int v)
{
    v &= 0xF;

    return (v < 10) ? (v + '0') : (v + 'A' - 10);
}

#if USBD_MS_OS20

#if MYNEWT_VAL(USBD_WINDOWS_COMP_ID)
#error "USBD_NET_MS_OS20 can not be used with USBD_WINDOWS_COMP_ID"
#endif

#define MS_OS_20_VENDOR_CODE    0x01
/* wIndex of GET_MS_OS_20_DESCRIPTOR vendor request */
#define MS_OS_20_DESCRIPTOR_INDEX   0x07
#define MS_OS_20_DESC_LEN       (0x0A + 0x08 + 0x08 + 0x14)
#define BOS_TOTAL_LEN           (TUD_BOS_DESC_LEN + \
                                 TUD_BOS_MICROSOFT_OS_DESC_LEN)

static const uint8_t desc_bos[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, MS_OS_20_VENDOR_CODE),
};

/*
 * Microsoft OS 2.0 descriptor set: NCM function gets WINNCM compatible ID
 * so Windows binds its inbox NCM driver (UsbNcm) automatically.
 */
static const uint8_t desc_ms_os_20[] = {
    /* Set header: length, type, Windows version (8.1+), total length */
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    /* Configuration subset: length, type, config index, reserved, length */
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
    /* Function subset: length, type, first interface, reserved, length */
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    ITF_NUM_NET, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),
    /* Compatible ID: length, type, compatible ID, sub-compatible ID */
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'N', 'C', 'M', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN,
                 "MS OS 2.0 descriptor length mismatch");

/*
 * Invoked when received GET BOS DESCRIPTOR request
 */
const uint8_t *
tud_descriptor_bos_cb(void)
{
    return desc_bos;
}

bool
tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                           const tusb_control_request_t *request)
{
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR ||
        request->bRequest != MS_OS_20_VENDOR_CODE ||
        request->wIndex != MS_OS_20_DESCRIPTOR_INDEX) {
        return false;
    }
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

    return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20,
                            sizeof(desc_ms_os_20));
}
#endif /* USBD_MS_OS20 */

#if MYNEWT_VAL(USBD_WINDOWS_COMP_ID)

#define MICROSOFT_OS_STRING_DESCRIPTOR  0xEE
#define COMPATIBILITY_FEATURE_REQUEST   0xFE

static const uint8_t microsoft_os_string_descriptor[] = {
    0x12,                           /* BYTE     Descriptor length (18 bytes) */
    0x03,                           /* BYTE     Descriptor type (3 = String) */
    0x4D, 0x00, 0x53, 0x00,         /* 7 WORDS Unicode String (LE)  Signature: "MSFT100" */
    0x46, 0x00, 0x54, 0x00,
    0x31, 0x00, 0x30, 0x00,
    0x30, 0x00,
    COMPATIBILITY_FEATURE_REQUEST,  /* BYTE Vendor Code */
    0x00                            /* BYTE Padding */
};

struct {
    uint32_t len;
    uint16_t version;
    uint16_t four;
    uint8_t number_of_sections;
    uint8_t reserved1[7];
    uint8_t itf;
    uint8_t reserved2;
    uint8_t compatible_id[8];
    uint8_t sub_compatible_id[8];
    uint8_t reserved3[6];
} static windows_compat_id = {
    .len = tu_htole32(40),
    .version = tu_htole16(0x100),
    .four = tu_htole16(4),
    .number_of_sections = 1,
    .reserved2 = 1,
    .itf = MYNEWT_VAL(USBD_WINDOWS_COMP_INTERFACE),
    .compatible_id = MYNEWT_VAL(USBD_WINDOWS_COMP_ID_STRING),
};

struct __attribute__((packed)) {
    uint32_t len;
    uint16_t version;
    uint16_t index;
    uint16_t count;
    uint32_t size;
    uint32_t property_data_type;
    uint16_t property_name_len;
    uint16_t property_name[21];
    uint32_t property_data_len;
    uint16_t property[40];
} static interface_guid = {
    .len = tu_htole32(146),
    .version = tu_htole16(0x100),
    .index = tu_htole16(5),
    .count = tu_htole16(1),
    .size = tu_htole32(136),
    .property_data_type = tu_htole32(7),
    .property_name_len = tu_htole16(42),
    .property_name = {},
    .property_data_len = tu_htole32(80),
    .property = {},
};

static const char guid_name[] = "DeviceInterfaceGUIDs";
static const char if_guid[] = "{02f5f07d-abac-4c80-96ac-2f67f695e3e5}";

static void
ascii_to_utf16le(void *dst, const char *src, size_t buf_size)
{
    int str_len = strlen(src);
    uint8_t *ptr = dst;

    for (size_t i = 0; i < buf_size; ++i) {
        *ptr++ = i < str_len ? src[i] : 0;
        *ptr++ = 0;
    }
}

bool
tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t *request)
{
    if (request->wIndex == 0x04 && request->bRequest == COMPATIBILITY_FEATURE_REQUEST) {
        if (stage == CONTROL_STAGE_SETUP) {
            return tud_control_xfer(rhport, request, (void *)&windows_compat_id, 40);
        } else {
            return true;
        }
    } else if (request->wIndex == 0x05 &&
               request->bRequest == COMPATIBILITY_FEATURE_REQUEST) {
        if (stage == CONTROL_STAGE_SETUP) {
            ascii_to_utf16le(interface_guid.property_name, guid_name,
                             ARRAY_SIZE(interface_guid.property_name));
            ascii_to_utf16le(interface_guid.property, if_guid,
                             ARRAY_SIZE(interface_guid.property));
            /* Change one character so each guid is different */
            interface_guid.property[1] = tu_htole16(request->wValue + '0');
            return tud_control_xfer(rhport, request, &interface_guid,
                                    sizeof(interface_guid));
        } else {
            return true;
        }
    }
    return false;
}
#endif
/*
 * Invoked when received GET STRING DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 */
const uint16_t *
tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    uint16_t *desc_str = desc_string;

#if MYNEWT_VAL(USBD_WINDOWS_COMP_ID)
    if (index == MICROSOFT_OS_STRING_DESCRIPTOR) {
        return (const uint16_t *)microsoft_os_string_descriptor;
    }
#endif
    if (index == 0) {
        desc_string[1] = MYNEWT_VAL(USBD_LANGID);
        desc_string[0] = (TUSB_DESC_STRING << 8) | (2 * 1 + 2);
    } else if (index == 1) {
        int len = hal_bsp_hw_id_len();
        uint8_t serial[2 * len + 1];
        hal_bsp_hw_id(serial + len, len);
        uint8_t *src = serial + len;
        uint8_t *dst = serial;
        for (int i = 0; i < len; ++i) {
            *dst++ = hex_digit(*src >> 4);
            *dst++ = hex_digit(*src++);
        }
        *dst = 0;
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), (const char *)serial);
    } else if (index - 2 < ARRAY_SIZE(string_desc_arr)) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), string_desc_arr[index - 2]);
    } else if (index == 32) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), "HASH:hid_gpio:"MYNEWT_VAL(REPO_HASH_HID_GPIO));
    } else if (index == 33) {
        desc_str = desc_string_from_ascii(desc_string, ARRAY_SIZE(desc_string), "HASH:apache-mynewt_core:"MYNEWT_VAL(REPO_HASH_APACHE_MYNEWT_CORE));
    }

    return desc_str;
}
