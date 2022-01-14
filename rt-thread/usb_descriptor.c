/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb_descriptor.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
        .bLength            = sizeof(tusb_desc_device_t),
        .bDescriptorType    = TUSB_DESC_DEVICE,
        .bcdUSB             = 0x0200,
        .bDeviceClass       = 0x00,
        .bDeviceSubClass    = 0x00,
        .bDeviceProtocol    = 0x00,
        .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor           = PKG_TINYUSB_DEVICE_VID,
        .idProduct          = PKG_TINYUSB_DEVICE_PID,
        .bcdDevice          = 0x0100,

        .iManufacturer      = 0x01,
        .iProduct           = 0x02,
        .iSerialNumber      = 0x03,

        .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
{
#ifdef PKG_TINYUSB_DEVICE_HID_KEYBOARD
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
#endif
#ifdef PKG_TINYUSB_DEVICE_HID_MOUSE
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
#endif
#ifdef PKG_TINYUSB_DEVICE_HID_CONSUMER
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
#endif
#ifdef PKG_TINYUSB_DEVICE_HID_GAMEPAD
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD))
#endif
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
#if CFG_TUD_CDC
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
#endif
#if CFG_TUD_MSC
    ITF_NUM_MSC,
#endif
#if CFG_TUD_HID
    ITF_NUM_HID,
#endif
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + \
                             TUD_MSC_DESC_LEN * CFG_TUD_MSC + \
                             TUD_CDC_DESC_LEN * CFG_TUD_CDC + \
                             TUD_HID_DESC_LEN * CFG_TUD_HID)

#define EPNUM_CDC_NOTIF   0x81
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x82

#define EPNUM_MSC_OUT     0x03
#define EPNUM_MSC_IN      0x83

#define EPNUM_HID     0x84

uint8_t const desc_fs_configuration[] =
{
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
#if CFG_TUD_CDC
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
#endif
#if CFG_TUD_MSC
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
#endif
#if CFG_TUD_HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 6, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
#endif
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations

    return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
        (const char[]) {0x09, 0x04},   // 0: is supported language is English (0x0409)
        "TinyUSB",                     // 1: Manufacturer
        "TinyUSB Device",              // 2: Product
        "123456",                      // 3: Serials, should use chip ID
        "TinyUSB CDC",
        "TinyUSB MSC",
        "TinyUSB HID",
};

static uint16_t desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;

        const char *str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        // Convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++)
        {
            desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return desc_str;
}
