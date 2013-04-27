/************************************
USB Device Hardware Abstraction

Alan Ott
Signal 11 Software
2013-04-08
*************************************/


#ifndef USB_HAL_H__
#define UAB_HAL_H__

#ifdef _PIC18
#define NEEDS_PULL /* Whether to pull up D+/D- with SFR_PULL_EN. */
#define HAS_LOW_SPEED

#define BDNADR_TYPE              uint16_t

#define SFR_FULL_SPEED_EN        UCFGbits.FSEN
#define SFR_PULL_EN              UCFGbits.UPUEN
#define SFR_ON_CHIP_XCVR_DIS     UCFGbits.UTRDIS
#define SET_PING_PONG_MODE(n)    do { UCFGbits.PPB0 = n & 1; UCFGbits.PPB1 = n & 2; } while (0)

#define SFR_USB_INTERRUPT_FLAGS  UIR
#define SFR_USB_RESET_IF         UIRbits.URSTIF
#define SFR_USB_STALL_IF         UIRbits.STALLIF
#define SFR_USB_TOKEN_IF         UIRbits.TRNIF
#define SFR_USB_SOF_IF           UIRbits.SOFIF
#define SFR_USB_IF               PIR2bits.USBIF

#define SFR_USB_INTERRUPT_EN     UIE
#define SFR_TRANSFER_IE          UIEbits.TRNIE
#define SFR_STALL_IE             UIEbits.STALLIE
#define SFR_RESET_IE             UIEbits.URSTIE
#define SFR_SOF_IE               UIEbits.SOFIE
#define SFR_USB_IE               PIE2bits.USBIE

#define SFR_USB_EXTENDED_INTERRUPT_EN UEIE

#define SFR_EP_MGMT_TYPE         UEP1bits_t /* TODO test */
#define SFR_EP_MGMT(n)           UEP##n##bits
#define SFR_EP_MGMT_HANDSHAKE    EPHSHK
#define SFR_EP_MGMT_STALL        EPSTALL
#define SFR_EP_MGMT_OUT_EN       EPOUTEN
#define SFR_EP_MGMT_IN_EN        EPINEN
#define SFR_EP_MGMT_CON_DIS      EPCONDIS /* disable control transfers */

#define SFR_USB_ADDR             UADDR
#define SFR_USB_EN               UCONbits.USBEN
#define SFR_USB_PKT_DIS          UCONbits.PKTDIS

#define SFR_USB_STATUS           USTAT
#define SFR_USB_STATUS_EP        USTATbits.ENDP
#define SFR_USB_STATUS_DIR       USTATbits.DIR
#define SFR_USB_STATUS_PPBI      USTATbits.PPBI

#define CLEAR_ALL_USB_IF()       SFR_USB_INTERRUPT_FLAGS = 0 //TODO TEST!
#define CLEAR_USB_RESET_IF()     SFR_USB_RESET_IF = 0
#define CLEAR_USB_STALL_IF()     SFR_USB_STALL_IF = 0
#define CLEAR_USB_TOKEN_IF()     SFR_USB_TOKEN_IF = 0
#define CLEAR_USB_SOF_IF()       SFR_USB_SOF_IF = 0

/* Buffer Descriptor BDnSTAT flags. On Some MCUs, apparently, when handing
 * a buffer descriptor to the SIE, there's a race condition that can happen
 * if you don't set the BDnSTAT byte as a single operation. This was observed
 * on the PIC18F46J50 when sending 8-byte IN-transactions while doing control
 * transfers. */
#define BDNSTAT_UOWN   0x80
#define BDNSTAT_DTS    0x40
#define BDNSTAT_DTSEN  0x08
#define BDNSTAT_BSTALL 0x04
#define BDNCNT_MASK    0x03ff /* 10 bits of BDnCNT in BDnSTAT_CNT */

/* Buffer Descriptor
 *
 * This represents the Buffer Descriptor as laid out in the PIC18F4550
 * Datasheet.  A buffer descriptor contains data about either an in or out
 * endpoint buffer.  Bufffer descriptors are almost the same on all 8-bit
 * parts, best I've so far been able to tell.  The fields that aren't in the
 * newer datasheets like KEN and INCDIS aren't used, so it doesn't hurt to
 * have them here on those parts.
 *
 * While the layout is very similar on 16-bit parts, a different struct is
 * required on 16-bit for several reasons, including endianness (the 8-bit
 * BC/BDnSTAT bits are effectively big-endian), and the ability to optimize
 * for each platform (eg: writing BDnSTAT/BDnCNT as a 16-bit word on 16-bit
 * platforms).
 */
struct buffer_descriptor {
	union {
		struct {
			/* When receiving from the SIE. (USB Mode) */
			uint8_t BC8 : 1;
			uint8_t BC9 : 1;
			uint8_t PID : 4; /* See enum PID */
			uint8_t reserved: 1;
			uint8_t UOWN : 1;
		};
		struct {
			/* When giving to the SIE (CPU Mode) */
			uint8_t /*BC8*/ : 1;
			uint8_t /*BC9*/ : 1;
			uint8_t BSTALL : 1;
			uint8_t DTSEN : 1;
			uint8_t INCDIS : 1;
			uint8_t KEN : 1;
			uint8_t DTS : 1;
			uint8_t /*UOWN*/ : 1;
		};
		uint8_t BDnSTAT;
	} STAT;
	uint8_t BDnCNT;
	BDNADR_TYPE BDnADR; /* BDnADRL and BDnADRH; */
};

#ifdef LARGE_EP
#define SET_BDN(REG, FLAGS, CNT) do { REG.BDnCNT = (CNT); \
           REG.STAT.BDnSTAT = (FLAGS) | ((CNT) & 0x300) >> 8; } while(0)
#define BDN_LENGTH(REG) ( (REG.STAT.BDnSTAT & 0x03) << 8 | REG.BDnCNT )
#else
#define SET_BDN(REG, FLAGS, CNT) do { REG.BDnCNT = (CNT); \
                                      REG.STAT.BDnSTAT = (FLAGS); } while(0)
#define BDN_LENGTH(REG) (REG.BDnCNT)
#endif

#ifdef _18F46J50
#define BD_ADDR 0x400
//#define BUFFER_ADDR
#else
#error "CPU not supported yet"
#endif

/* Compiler stuff. Probably should be somewhere else. */
#ifdef __C18
	#define FAR far
	#define memcpy_from_rom(x,y,z) memcpypgm2ram(x,(rom void*)y,z);
	#define BD_ATTR_TAG
	#define XC8_BUFFER_ADDR_TAG
#elif defined __XC8
	#define memcpy_from_rom(x,y,z) memcpy(x,y,z);
	#define FAR
	#define BD_ATTR_TAG @##BD_ADDR
	#ifdef BUFFER_ADDR
		#define XC8_BUFFER_ADDR_TAG @##BUFFER_ADDR
	#else
		#define XC8_BUFFER_ADDR_TAG
	#endif
#endif

#elif __XC16__

#define USB_NEEDS_POWER_ON
#define USB_NEEDS_SET_BD_ADDR_REG

#define BDNADR_TYPE              void *

#define SFR_PULL_EN              /* Not used on PIC24 */
#define SFR_ON_CHIP_XCVR_DIS     U1CNFG2bits.UTRDIS
#define SET_PING_PONG_MODE(n)    U1CNFG1bits.PPB = n

#define SFR_USB_INTERRUPT_FLAGS  U1IR
#define SFR_USB_RESET_IF         U1IRbits.URSTIF
#define SFR_USB_STALL_IF         U1IRbits.STALLIF
#define SFR_USB_TOKEN_IF         U1IRbits.TRNIF
#define SFR_USB_SOF_IF           U1IRbits.SOFIF
#define SFR_USB_IF               IFS5bits.USB1IF

#define SFR_USB_INTERRUPT_EN     U1IE
#define SFR_TRANSFER_IE          U1IEbits.TRNIE
#define SFR_STALL_IE             U1IEbits.STALLIE
#define SFR_RESET_IE             U1IEbits.URSTIE
#define SFR_SOF_IE               U1IEbits.SOFIE
#define SFR_USB_IE               IEC5bits.USB1IE

#define SFR_USB_EXTENDED_INTERRUPT_EN U1EIE

#define SFR_EP_MGMT_TYPE         U1EP1BITS
#define SFR_EP_MGMT(n)           U1EP##n##bits
#define SFR_EP_MGMT_HANDSHAKE    EPHSHK
#define SFR_EP_MGMT_STALL        EPSTALL
#define SFR_EP_MGMT_IN_EN        EPTXEN   /* In/out from HOST perspective */
#define SFR_EP_MGMT_OUT_EN       EPRXEN
#define SFR_EP_MGMT_CON_DIS      EPCONDIS /* disable control transfers */
                                 /* Ignoring RETRYDIS and LSPD for now */
#define SFR_USB_ADDR             U1ADDR
#define SFR_USB_EN               U1CONbits.USBEN
#define SFR_USB_PKT_DIS          U1CONbits.PKTDIS


#define SFR_USB_STATUS           U1STAT
#define SFR_USB_STATUS_EP        U1STATbits.ENDPT
#define SFR_USB_STATUS_DIR       U1STATbits.DIR
#define SFR_USB_STATUS_PPBI      U1STATbits.PPBI

#define SFR_USB_POWER            U1PWRCbits.USBPWR
#define SFR_BD_ADDR_REG          U1BDTP1

#define BDnCNT                   STAT.BDnCNT_byte /* buffer descriptor */

#define SFR_OTGEN                U1OTGCONbits.OTGEN
#define SFR_DPPULUP              U1OTGCONbits.DPPULUP

#define CLEAR_ALL_USB_IF()       do { SFR_USB_INTERRUPT_FLAGS = 0xff; U1EIR = 0xff; } while(0)
#define CLEAR_USB_RESET_IF()     SFR_USB_INTERRUPT_FLAGS = 0x1
#define CLEAR_USB_STALL_IF()     SFR_USB_INTERRUPT_FLAGS = 0x80
#define CLEAR_USB_TOKEN_IF()     SFR_USB_INTERRUPT_FLAGS = 0x08
#define CLEAR_USB_SOF_IF()       SFR_USB_INTERRUPT_FLAGS = 0x4

#define BDNSTAT_UOWN   0x8000
#define BDNSTAT_DTS    0x4000
#define BDNSTAT_DTSEN  0x0800
#define BDNSTAT_BSTALL 0x0400

/* Buffer Descriptor
 *
 * This struct represents BDnSTAT in the datasheet. See the comment in the
 * 8-bit section above for more information on buffer descriptors.
 */
struct buffer_descriptor {
	union {
		struct {
			/* When receiving from the SIE. (USB Mode) */
			uint16_t BC : 10;
			uint16_t PID : 4; /* See enum PID */
			uint16_t DTS: 1;
			uint16_t UOWN : 1;
		};
		struct {
			/* When giving to the SIE (CPU Mode) */
			uint16_t /*BC*/ : 10;
			uint16_t BSTALL : 1;
			uint16_t DTSEN : 1;
			uint16_t reserved : 2;
			uint16_t DTS : 1;
			uint16_t /*UOWN*/ : 1;
		};
		struct {
			uint8_t BDnSTAT_lsb;
			uint8_t BDnSTAT; /* High byte, where the flags are */
		};
		uint16_t BDnSTAT_CNT; /* BDnSTAT and BDnCNT as a 16-bit */
	}STAT;
	BDNADR_TYPE BDnADR;
};

#define SET_BDN(REG, FLAGS, CNT) \
                     do { REG.STAT.BDnSTAT_CNT = (FLAGS) | (CNT); } while(0)

#ifdef LARGE_EP
	#define BDN_LENGTH(REG) (REG.STAT.BC)
#else
	#define BDN_LENGTH(REG) (REG.STAT.BDnSTAT_lsb)
#endif


#define BD_ADDR
#define BUFFER_ADDR
#define BD_ATTR_TAG __attribute__((aligned(512)))
#define XC8_BUFFER_ADDR_TAG

/* Compiler stuff. Probably should be somewhere else. */
#define FAR
#define memcpy_from_rom(x,y,z) memcpy(x,y,z);

#endif



#endif /* USB_HAL_H__ */