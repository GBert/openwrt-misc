;--------------------------------------------------------
; File Created by SDCC : free open source ANSI-C Compiler
; Version 3.4.1 #9092 (Oct 30 2014) (Linux)
; This file was generated Sun Mar 15 21:16:37 2015
;--------------------------------------------------------
; PIC port for the 14-bit core
;--------------------------------------------------------
;	.file	"main.c"
	list	p=12f1501
	radix dec
	include "p12f1501.inc"
;--------------------------------------------------------
; config word(s)
;--------------------------------------------------------
	__config _CONFIG1, 0x39e4
	__config _CONFIG2, 0x3fff
;--------------------------------------------------------
; external declarations
;--------------------------------------------------------
	extern	_STATUSbits
	extern	_BSRbits
	extern	_INTCONbits
	extern	_PORTAbits
	extern	_PIR1bits
	extern	_PIR2bits
	extern	_PIR3bits
	extern	_T1CONbits
	extern	_T1GCONbits
	extern	_T2CONbits
	extern	_TRISAbits
	extern	_PIE1bits
	extern	_PIE2bits
	extern	_PIE3bits
	extern	_OPTION_REGbits
	extern	_PCONbits
	extern	_WDTCONbits
	extern	_OSCCONbits
	extern	_OSCSTATbits
	extern	_ADCON0bits
	extern	_ADCON1bits
	extern	_ADCON2bits
	extern	_LATAbits
	extern	_CM1CON0bits
	extern	_CM1CON1bits
	extern	_CMOUTbits
	extern	_BORCONbits
	extern	_FVRCONbits
	extern	_DACCON0bits
	extern	_DACCON1bits
	extern	_APFCONbits
	extern	_ANSELAbits
	extern	_PMCON1bits
	extern	_VREGCONbits
	extern	_WPUAbits
	extern	_IOCAPbits
	extern	_IOCANbits
	extern	_IOCAFbits
	extern	_NCO1ACCLbits
	extern	_NCO1ACCHbits
	extern	_NCO1ACCUbits
	extern	_NCO1INCLbits
	extern	_NCO1INCHbits
	extern	_NCO1CONbits
	extern	_NCO1CLKbits
	extern	_PWM1DCLbits
	extern	_PWM1DCHbits
	extern	_PWM1CONbits
	extern	_PWM1CON0bits
	extern	_PWM2DCLbits
	extern	_PWM2DCHbits
	extern	_PWM2CONbits
	extern	_PWM3DCLbits
	extern	_PWM3DCHbits
	extern	_PWM3CONbits
	extern	_PWM4DCLbits
	extern	_PWM4DCHbits
	extern	_PWM4CONbits
	extern	_CWG1DBRbits
	extern	_CWG1DBFbits
	extern	_CWG1CON0bits
	extern	_CWG1CON1bits
	extern	_CWG1CON2bits
	extern	_CLCDATAbits
	extern	_CLC1CONbits
	extern	_CLC1POLbits
	extern	_CLC1SEL0bits
	extern	_CLC1SEL1bits
	extern	_CLC1GLS0bits
	extern	_CLC1GLS1bits
	extern	_CLC1GLS2bits
	extern	_CLC1GLS3bits
	extern	_CLC2CONbits
	extern	_CLC2POLbits
	extern	_CLC2SEL0bits
	extern	_CLC2SEL1bits
	extern	_CLC2GLS0bits
	extern	_CLC2GLS1bits
	extern	_CLC2GLS2bits
	extern	_CLC2GLS3bits
	extern	_STATUS_SHADbits
	extern	_INDF0
	extern	_INDF1
	extern	_PCL
	extern	_STATUS
	extern	_FSR0
	extern	_FSR0L
	extern	_FSR0H
	extern	_FSR1
	extern	_FSR1L
	extern	_FSR1H
	extern	_BSR
	extern	_WREG
	extern	_PCLATH
	extern	_INTCON
	extern	_PORTA
	extern	_PIR1
	extern	_PIR2
	extern	_PIR3
	extern	_TMR0
	extern	_TMR1
	extern	_TMR1L
	extern	_TMR1H
	extern	_T1CON
	extern	_T1GCON
	extern	_TMR2
	extern	_PR2
	extern	_T2CON
	extern	_TRISA
	extern	_PIE1
	extern	_PIE2
	extern	_PIE3
	extern	_OPTION_REG
	extern	_PCON
	extern	_WDTCON
	extern	_OSCCON
	extern	_OSCSTAT
	extern	_ADRES
	extern	_ADRESL
	extern	_ADRESH
	extern	_ADCON0
	extern	_ADCON1
	extern	_ADCON2
	extern	_LATA
	extern	_CM1CON0
	extern	_CM1CON1
	extern	_CMOUT
	extern	_BORCON
	extern	_FVRCON
	extern	_DACCON0
	extern	_DACCON1
	extern	_APFCON
	extern	_ANSELA
	extern	_PMADR
	extern	_PMADRL
	extern	_PMADRH
	extern	_PMDAT
	extern	_PMDATL
	extern	_PMDATH
	extern	_PMCON1
	extern	_PMCON2
	extern	_VREGCON
	extern	_WPUA
	extern	_IOCAP
	extern	_IOCAN
	extern	_IOCAF
	extern	_NCO1ACC
	extern	_NCO1ACCL
	extern	_NCO1ACCH
	extern	_NCO1ACCU
	extern	_NCO1INC
	extern	_NCO1INCL
	extern	_NCO1INCH
	extern	_NCO1INCU
	extern	_NCO1CON
	extern	_NCO1CLK
	extern	_PWM1DCL
	extern	_PWM1DCH
	extern	_PWM1CON
	extern	_PWM1CON0
	extern	_PWM2DCL
	extern	_PWM2DCH
	extern	_PWM2CON
	extern	_PWM3DCL
	extern	_PWM3DCH
	extern	_PWM3CON
	extern	_PWM4DCL
	extern	_PWM4DCH
	extern	_PWM4CON
	extern	_CWG1DBR
	extern	_CWG1DBF
	extern	_CWG1CON0
	extern	_CWG1CON1
	extern	_CWG1CON2
	extern	_CLCDATA
	extern	_CLC1CON
	extern	_CLC1POL
	extern	_CLC1SEL0
	extern	_CLC1SEL1
	extern	_CLC1GLS0
	extern	_CLC1GLS1
	extern	_CLC1GLS2
	extern	_CLC1GLS3
	extern	_CLC2CON
	extern	_CLC2POL
	extern	_CLC2SEL0
	extern	_CLC2SEL1
	extern	_CLC2GLS0
	extern	_CLC2GLS1
	extern	_CLC2GLS2
	extern	_CLC2GLS3
	extern	_BSR_ICDSHAD
	extern	_STATUS_SHAD
	extern	_WREG_SHAD
	extern	_BSR_SHAD
	extern	_PCLATH_SHAD
	extern	_FSR0L_SHAD
	extern	_FSR0H_SHAD
	extern	_FSR1L_SHAD
	extern	_FSR1H_SHAD
	extern	_STKPTR
	extern	_TOSL
	extern	_TOSH
	extern	__sdcc_gsinit_startup
;--------------------------------------------------------
; global declarations
;--------------------------------------------------------
	global	_init
	global	_main

	global PSAVE
	global SSAVE
	global WSAVE
	global STK12
	global STK11
	global STK10
	global STK09
	global STK08
	global STK07
	global STK06
	global STK05
	global STK04
	global STK03
	global STK02
	global STK01
	global STK00

sharebank udata_ovr 0x0070
PSAVE	res 1
SSAVE	res 1
WSAVE	res 1
STK12	res 1
STK11	res 1
STK10	res 1
STK09	res 1
STK08	res 1
STK07	res 1
STK06	res 1
STK05	res 1
STK04	res 1
STK03	res 1
STK02	res 1
STK01	res 1
STK00	res 1

;--------------------------------------------------------
; global definitions
;--------------------------------------------------------
;--------------------------------------------------------
; absolute symbol definitions
;--------------------------------------------------------
;--------------------------------------------------------
; compiler-defined variables
;--------------------------------------------------------
;--------------------------------------------------------
; initialized data
;--------------------------------------------------------
;--------------------------------------------------------
; overlayable items in internal ram 
;--------------------------------------------------------
;	udata_ovr
;--------------------------------------------------------
; reset vector 
;--------------------------------------------------------
STARTUP	code 0x0000
	nop
	pagesel __sdcc_gsinit_startup
	goto	__sdcc_gsinit_startup
;--------------------------------------------------------
; code
;--------------------------------------------------------
code_main	code
;***
;  pBlock Stats: dbName = M
;***
;entry:  _main	;Function start
; 2 exit points
;has an exit
;functions called:
;   _init
;   _init
;; Starting pCode block
_main	;Function start
; 2 exit points
;	.line	27; "main.c"	init();
	CALL	_init
;	.line	30; "main.c"	TRISA5 = 1;
	BANKSEL	_TRISAbits
	BSF	_TRISAbits,5
;	.line	31; "main.c"	TRISA2 = 0;
	BCF	_TRISAbits,2
_00109_DS_
;	.line	35; "main.c"	while(LATA5) {}
	BANKSEL	_LATAbits
	BTFSC	_LATAbits,5
	GOTO	_00109_DS_
;	.line	36; "main.c"	TRISA2 = 0; // set ouuput 
	BANKSEL	_TRISAbits
	BCF	_TRISAbits,2
;	.line	37; "main.c"	LATA2 = 0;
	BANKSEL	_LATAbits
	BCF	_LATAbits,2
;	.line	38; "main.c"	LATA2 = 0;
	BCF	_LATAbits,2
;	.line	39; "main.c"	LATA2 = 1;
	BSF	_LATAbits,2
;	.line	40; "main.c"	TRISA2 = 1; // high z
	BANKSEL	_TRISAbits
	BSF	_TRISAbits,2
_00112_DS_
;	.line	41; "main.c"	while(!LATA5) {}
	BANKSEL	_LATAbits
	BTFSC	_LATAbits,5
	GOTO	_00109_DS_
	GOTO	_00112_DS_
	RETURN	
; exit point of _main

;***
;  pBlock Stats: dbName = C
;***
;entry:  _init	;Function start
; 2 exit points
;has an exit
;; Starting pCode block
_init	;Function start
; 2 exit points
;	.line	10; "main.c"	OSCCON  = 0b01111000; // int osc: 16Mhz
	MOVLW	0x78
	BANKSEL	_OSCCON
	MOVWF	_OSCCON
;	.line	12; "main.c"	INTCON  = 0;          // Clear interrupt flag bits
	BANKSEL	_INTCON
	CLRF	_INTCON
;	.line	13; "main.c"	GIE     = 0;          // Global irq disable
	BCF	_INTCONbits,7
;	.line	14; "main.c"	T0IE    = 0;          // Timer0 irq disable
	BCF	_INTCONbits,5
;	.line	15; "main.c"	TMR0    = 0;          // Clear timer0 register
	CLRF	_TMR0
;	.line	18; "main.c"	ANSELA  = 0;
	BANKSEL	_ANSELA
	CLRF	_ANSELA
;	.line	19; "main.c"	ADCON0  = 0;
	BANKSEL	_ADCON0
	CLRF	_ADCON0
;	.line	20; "main.c"	ADCON1  = 0;
	CLRF	_ADCON1
;	.line	21; "main.c"	ADCON2  = 0;
	CLRF	_ADCON2
;	.line	22; "main.c"	CM1CON0 = 0;
	BANKSEL	_CM1CON0
	CLRF	_CM1CON0
;	.line	23; "main.c"	CM1CON1 = 0;
	CLRF	_CM1CON1
	RETURN	
; exit point of _init


;	code size estimation:
;	   27+   11 =    38 instructions (   98 byte)

	end
