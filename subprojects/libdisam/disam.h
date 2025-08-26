/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *             Worldwide Copyright (c) Byte Designs Ltd (2024)             *
 *          Version 7.2 (build 2024/11/30) (expire cobol shared)           *
 *                                                                         *
 *                            Byte Designs Ltd.                            *
 *                            20568 - 32 Avenue                            *
 *                               LANGLEY, BC                               *
 *                             V2Z 2C8 CANADA                              *
 *                                                                         *
 *                       Sales: sales@bytedesigns.com                      *
 *                     Support: support@bytedesigns.com                    *
 *              Phone: (604) 534 0722     Fax: (604) 534 2601              *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <iswrap.h>

#define ISAMseekMode(M)	((M)&0xFF)
#define ISAMlockMode(M)	((M)&0xFF00)
