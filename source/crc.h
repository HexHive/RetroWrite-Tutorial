#ifndef CRC_H
#define CRC_H

unsigned long update_crc(unsigned long crc, unsigned char *buf,
                        int len);

unsigned long crc(unsigned char *buf, int len);

#endif
