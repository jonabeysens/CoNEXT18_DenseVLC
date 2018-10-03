	.cdecls "main_pru0.c"

	.clink
	.global preamble_detector

preamble_detector: 

 ;; Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    &r0, C4, 4, 4     ;; load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        ;; clear bit 4 (STANDBY_INIT)
	SBCO    &r0, C4, 4, 4     ;; store the modified r0 back at the load addr
	
	LDI32 r1, 0x00010000 ;; Flag address
	LDI32 r2, 0 ;; Threshold flag
	LDI32 r6, 0xFFFFFFFF ;; Flag
	LDI32 r3, 0 ;; Sample value
	LDI32 r5, 0x00000FFF
	LDI32 r7, 0 ;; Counter
	LDI32 r8, 0x00000000 ;; Actual sample
	LDI32 r9, 0x00000000 ;; Previous sample
	LDI32 r11, 0 ;; JAL register
	LDI32 r12, 0 ;; Value after 0 Mask
	LDI32 r13, 0 ;; Value after 1 Mask
	SBBO &r13, r1, 16, 4
	LDI32 r14, 0 ;; Preamble counter
	LDI32 r15, 256 ;;
	
	LDI32 r18, 0 ;; Threshold ;; This is not real!!!!!!!!!!!!!!
	LDI32 r19, 9 ;; Oversampling rate
	LDI32 r21, 0 ;; Actual bits
	LDI32 r22, 5000 ;;Minimum
	LDI32 r23, 0 ;;Maximum
	LDI32 r25, 0 ;;Values received
	;LDI32 r26, 0x99999999 ;; Prepreamble to detect
	LDI32 r26, 0x66666666 ;; Prepreamble to detect
	LDI32 r28, 0x00000000 ;; Zero register
	SBBO &r28, r1, 0, 4
	LDI32 r28, 0xFFFFFFFF
;; WAIT FOR PREPREAMBLE ;;
GET_PREPREAMBLE:
	CLR r30, r30.t7
	sub r7, r7, 1
;; Set frequency	
	LDI32 r7, 100
;; Take sample
	
	JAL r11.w0, GET_SAMPLE
	
;; Get threshold just in case
	QBEQ MAINTAIN_TH, r2, 1
	JAL r11.w0, GET_THRESHOLD
MAINTAIN_TH:
;; Decode the sample
	QBLT LOW_th, r18, r3
	
	ADD r8, r8, 1
	LDI32 r9, 0
	JMP CONT
LOW_th:
	
	ADD r9, r9, 1
	LDI32 r8, 0
	JMP CONT
CONT:
	SUB r7, r7, 5
;; Decode 10 samples
	;; Check if we received more than 8 1's
	
	QBLT NOT_HIGH, r19, r8
	
	LSL r25, r25, 1
	SET r25, r25.t0
	LDI32 r8, 0
	
NOT_HIGH:
	
	
	;; Check if we received more than 8 0's
	QBLT NOT_LOW, r19, r9
	
	LSL r25, r25, 1
	CLR r25, r25.t0
	LDI32 r9, 0
NOT_LOW:	
	
WAIT:
	SUB r7, r7, 1
	QBNE WAIT, r7, 1
	;SBBO &r18, r1, 4, 4
	;; Check register with prepreamble
	QBNE NO_PREPREAMBLE, r26, r25
	LDI32 r2, 1
	
	LDI32 r28, 1
	SBBO &r28, r1, 0, 4
	SET r30, r30.t7	
	LDI32 r25, 0x00000000
	SUB r7, r7, 2
NO_PREPREAMBLE:
	JMP GET_PREPREAMBLE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; RX Code ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GET_SAMPLE:	
	
 	CLR r30, r30.t0 ;; set the CS line low (active low)
	LDI32 r4, 16 ;; going to write/read 24 bits (3 bytes)
	LDI32 r4, 16 ;; going to write/read 24 bits (3 bytes)
	SUB r7,r7, 3 ;; get sample and lines 51, 52
SPICLK_BIT:  
	SUB r4, r4, 1        ;; count down through the bits
	LSL r3, r3, 1
	SET r30, r30.t3
	SET r30, r30.t3
	QBBC DATAINLOW, r31, 2
	OR r3, r3, 0x00000001
	JMP CONT_RX
DATAINLOW: ;; 10
	OR r3, r3, 0x00000000
	JMP CONT_RX
CONT_RX:
	CLR r30, r30.t3
	SUB r7, r7, 5
	QBNE SPICLK_BIT, r4, 0
	SET r30, r30.t0 ;; pull the CS line high (end of sample)
	LSR r3, r3, 2        ;; SPICLK shifts left too many times left, shift right once
	AND r3, r3, r5 ;; AND the data with mask to give only the 10 LSBs
	;; Save the data in the shared memory
	
	JMP r11.w0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

;;;;;;;;;;;;;;;;;; Get Threshold ;;;;;;;;;;;;;;;;;;
GET_THRESHOLD:
	ADD r18, r18, r3
	ADD r14, r14, 1
	
	QBLT CONT_TH, r15, r14
	LDI32 r2, 1
	LSR r18, r18, 8
	SBBO &r18, r1, 16, 4
CONT_TH:
	SUB r7, r7, 1
	SUB r7, r7, 2
	JMP r11.w0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;