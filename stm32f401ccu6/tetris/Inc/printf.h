#ifndef TETRIS_PRINTF_H
#define TETRIS_PRINTF_H

#ifdef DEBUG
extern char usb_tx_buf[];  // main.c
#define PRINTF(fmt, args...) ({                                                     \
    sprintf(usb_tx_buf, fmt, ##args);                                               \
    while (CDC_Transmit_FS((uint8_t *)usb_tx_buf, strlen(usb_tx_buf)) == USBD_BUSY) \
        ;                                                                           \
})
#else
#define PRINTF(fmt, args...) ((void)0)
#endif

#endif