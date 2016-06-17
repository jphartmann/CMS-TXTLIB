/* Object deck contents                                              */
/*                                John Hartmann 18 Jun 2012 12:24:36 */

/*********************************************************************/
/* Common to assembler and loader.                                   */
/*                                                                   */
/* Change activity:                                                  */
/*18 Jun 2012  New header file.                                      */
/*********************************************************************/


#if !defined(_JPH_OBJDECK_H)
   #define _JPH_OBJDECK_H

#define E_BLANK 0x40
#define E_1 0xf1
#define E_2 0xf2
#define E_ESD "\xC5\xE2\xC4"
#define E_XSD "\xE7\xE2\xC4"
#define E_TXT "\xE3\xE7\xE3"
#define E_RLD "\xD9\xD3\xC4"
#define E_END "\xC5\xD5\xC4"

enum esdtype
{
   esd_sd=       0x00,                /* 00 SD                       */
   esd_ld=       0x01,                /* 01 LD                       */
   esd_er=       0x02,                /* 02 ER                       */
   esd_pc=       0x04,                /* 04 PC                       */
   esd_cm=       0x05,                /* 05 CM COM                   */
   esd_xd=       0x06,                /* 06 XD(PR)                   */
   esd_wx=       0x0a,                /* 0A WX                       */
   /* New in release 6                                               */
   esd_sq=        0x0D,               /* Quad-aligned SD             */
   esd_pq=        0x0E,               /* Quad-aligned PC             */
   esd_cq=        0x0F,               /* Quad-aligned CM             */
};

enum esdflag
{
   esd_amode31=  0x02,
   esd_amode24=  0x01,                /* [explicit AMODE 24]         */
   esd_amode64=  0x10,
   esd_rmode31=  0x04,
   esd_rmode64=  0x20,
   esd_rent=     0x08,
   esd_amodeany= esd_amode24 | esd_amode31,
   esd_amodeany31=esd_amodeany,
   esd_amodeany64=esd_amodeany31|esd_amode64,
};

struct esditem                        /* Object format.              */
{
   unsigned char symbol[8];
   unsigned char type;                /* Enum esdtype                */
   unsigned char address[3];          /* Bigendian                   */
   unsigned char flags;               /* enum esdflag                */
   unsigned char length[3];           /* or LDID.  Bigendian         */
};

enum rldflag
{
   /* This bit is now specified as reserved.                         */
   rldf_undefd=0x80,                  /* Undefined (when in module)  */
   rldf_add4=0x40,                    /* Length is four more (5..8)  */
   rldf_CXD=0x30,                     /* PR length                   */
   rldf_Q=0x20,                       /* DXD offset                  */
   rldf_V=0x10,                       /* V-type address reference    */
   rldf_A=0,                          /* Normal relocation           */
   rldf_type=0x30,                    /* four previous ones          */
   rldf_length=0x0c,                  /* Length -1                   */
   rldf_negative=2,                   /* Negative direction if on    */
   rldf_same=1,                    /* pos and rel ids elided in next */
   rldf_relative = rldf_CXD | rldf_add4, /* Relative 2 or 4 in halfwords */
};

/* RLD  item.   Note  that certain combinations in the Flag byte are */
/* used  to  indicate Relative-Immediate relocation items: x111-00xx */
/* 2-byte; x111-10xx 4-byte                                          */

struct rlditem
{
   unsigned char reloc[2];      /* Relocation ID--what to reloc with */
   unsigned char posid[2];            /* Where to relocate           */
   unsigned char flags;               /* enum rldflag                */
   unsigned char address[3];          /* Where to relocate           */
};

struct endcard
{
   unsigned char fill3[16];
   unsigned char idrcnt[1];          /* Count of IDR items following */
   struct idr
   {
      unsigned char id[10];
      unsigned char version[4];
      unsigned char date[5];
   } idrs[2];
};

struct xsd
{
   /* These eight bytes conveniently replace the name.               */
   unsigned char namelength[4];       /* Total length of name        */
   unsigned char nameoffset[4];       /* First byte of name (base 1) */
   unsigned char esdtype;             /* Enum esdtype                */
   unsigned char address[3];          /* Assembled address           */
   union
   {
      unsigned char alignment;        /* PR alignment -1 (7 = dwd)   */
      unsigned char esdflag;          /* Enum esdflag                */
   };
   unsigned char length[3];           /* or LDID.  Bigendian         */
   unsigned char symbol[40];          /* Or part thereof             */
};

struct card
{
   unsigned char ctype[4];            /* e.g., x'02' ESD             */
   unsigned char fill1[1];
   unsigned char address[3];          /* 6 Start address BIGENDIAN   */
   unsigned char fill2[2];            /* 9                           */
   unsigned char vfl[2]; /* 11 Bytes in the variable field BIGENDIAN */
   unsigned char fill3[2];            /* 13                          */
   unsigned char esdid[2];            /* 15 Esdid or blank BIGENDIAN */
   union                              /* 17                          */
   {
      struct esditem esd[3];
      struct xsd xsd;
      unsigned char txt[56];          /* Or RLD                      */
      struct endcard end;
   } d;
   unsigned char seq[8];
};

#endif
