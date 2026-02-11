/**
 * FatFsApi.h
 *
 * \brief Facade to the device driver functions used by ff15a FatFS implementation.
 *
 *  Created on: Dec 25, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_DISKIOFACADE_H_
#define CM7_INC_DISKIOFACADE_H_

#include <stdint.h>
#include "ff.h"
#include "diskio.h"

uint8_t MMC_disk_status(void);

uint8_t RAM_disk_status(void);

uint8_t USB_disk_status(void);

uint8_t MMC_disk_initialize(void);

uint8_t RAM_disk_initialize(void);

uint8_t USB_disk_initialize(void);

uint8_t MMC_disk_read(BYTE *buff, LBA_t sector, UINT count);

uint8_t RAM_disk_read(BYTE *buff, LBA_t sector, UINT count);

uint8_t USB_disk_read(BYTE *buff, LBA_t sector, UINT count);

uint8_t MMC_disk_write(
    const BYTE *buff, /* Data to be written */
    LBA_t sector, /* Start sector in LBA */
    UINT count /* Number of sectors to write */
);

uint8_t RAM_disk_write(const BYTE *buff, LBA_t sector, UINT count);

uint8_t USB_disk_write(const BYTE *buff, LBA_t sector, UINT count);

/**
 * Multi media card (MMC) disk control finction.
 * \brief The disk_ioctl function is called to control device specific features and miscellaneous functions other than generic read/write.
 */
DRESULT MMC_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
);

DRESULT RAM_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
);

DRESULT USB_disk_ioctl(
    BYTE cmd, /* Control code */
    void *buff /* Buffer to send/receive control data */
);

#endif /* CM7_INC_DISKIOFACADE_H_ */
