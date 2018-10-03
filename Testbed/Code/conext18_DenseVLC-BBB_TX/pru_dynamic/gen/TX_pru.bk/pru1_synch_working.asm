	.cdecls "main_pru1.c"

	.clink
	.global START


START:

 ; Enable the OCP master port -- allows transfer of data to Linux userspace
	LBCO    &r0, C4, 4, 4     ; load SYSCFG reg into r0 (use c4 const addr)
	CLR     r0, r0, 4        ; clear bit 4 (STANDBY_INIT)
	SBCO    &r0, C4, 4, 4     ; store the modified r0 back at the load addr
	
	
	LDI32 r30, 0x00000154 ;* Illumination pins high
	;;;;;;;;;;;;;;; TX1 ;;;;;;;;;;;;;;;;;;;;;;
	LDI32 r0, 0x00000000 ;* Address pointer 1
	LDI32 r1, 0x00000000 ;* bit counter 1
	LDI32 r2, 0x00000000 ;* Symbols left to be read 1
	LDI32 r3, 0x00000000 ;* Value of data to send 1
	
	;;;;;;;;;;;;;;; TX2 ;;;;;;;;;;;;;;;;;;;;;;
	LDI32 r4, 0x00001000 ;* Address pointer 2
	LDI32 r5, 0x00000000 ;* bit counter 2
	LDI32 r6, 0x00000000 ;* Symbols left to be read 2
	LDI32 r7, 0x00000000 ;* Value of data to send 2
	
	;;;;;;;;;;;;;;; TX3 ;;;;;;;;;;;;;;;;;;;;;;
	LDI32 r8, 0x00002000 ;* Address pointer 3
	LDI32 r9, 0x00000000 ;* bit counter 3
	LDI32 r10, 0x00000000 ;* Symbols left to be read 3
	LDI32 r11, 0x00000000 ;* Value of data to send 3
	
	;;;;;;;;;;;;;;; TX4 ;;;;;;;;;;;;;;;;;;;;;;
	LDI32 r12, 0x00004000 ;* Address pointer 4
	LDI32 r13, 0x00000000 ;* bit counter 4
	LDI32 r14, 0x00000000 ;* Symbols left to be read 4
	LDI32 r15, 0x00000000 ;* Value of data to send 4
	
	
	
	LDI32 r22, 0x00000000 ;* bit
	LDI32 r23, 0x00000000 ;* Frequency
	LDI32 r24, 0x00010000 ;* Address of prepreamble flag
	LDI32 r25, 0x00000000 ;* prepreamble
	LDI32 r26, 0x00000000 ;* tx memory start
	LDI32 r27, 0x00000000 ;* Zero
	LDI32 r28, 0x00000000
	LDI32 r29, 0x00000000 ;* Master register
	
	
	LDI32 r22, 0x00000000 ;* Reg to load pins register
	
TX:
	LBBO &r28, r24, 4, 4 ;;Read register with tx info
	LDI32 r29, 0x00000000
	QBEQ TX, r28, 0
	
	LBBO &r29, r24, 8, 4 ;; Master register
	SBBO &r27, r24, 4, 4
		
	OR r22, r22, r30
	
CONT_TX:	
	
	LDI32 r23, 1000 ;* Frequency
	SUB r23, r23, 1
	
	
;;;;;;;;;;;;;;;;;;;;;;;; TX1 ;;;;;;;;;;;;;;;;;;;;;;;
TX1:	
	LDI32 r26, 0x00000000 ;* Address of memory for tx1
	LBBO &r3, r0, 0, 4
	QBBC NOT_TX1, r28, 0
	QBNE CONT_TX1, r0, r26
	LBBO &r2, r0, 0, 4
	LDI32 r1, 0x00000000
	LDI32 r0, 0x00000004
	SUB r23, r23, 2
	SUB r23, r23, 1
	
CONT_TX1:
	SUB r23, r23, 1
	QBNE STILL_TX1, r2, 0
	CLR r22, r22.t10
	SET r22, r22.t8;; reset illumination pin
	CLR r28, r28.t0
	LDI32 r0, 0x00000000
	SUB r23, r23, 3
	SUB r23, r23, 1
	JMP NOT_TX1
STILL_TX1:
	SUB r23, r23, 1
	CLR r22, r22.t8;; Clear the illumination pin
	;LBBO &r3, r0, 0, 4
	LSL r3, r3, r1
	ADD r1, r1, 1
	QBBC ZERO1, r3, 31
	SET r22, r22.t10;; Set the pin
	JMP END_BIT1
ZERO1:
	CLR r22, r22.t10;; Clear the pin
	JMP END_BIT1
	ADD r23, r23, 1
	SUB r23, r23, 2
END_BIT1:
	SUB r2, r2, 1
	SUB r23, r23, 4;SUB r23, r23, 5
	SUB r23, r23, 1
	QBNE NOT_TX1, r1, 32
	ADD r0, r0, 4
	LDI32 r1, 0x00000000
	SUB r23, r23, 1
	SUB r23, r23, 1
NOT_TX1:
	SUB r23, r23, 1
	SUB r23, r23, 2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;; TX2 ;;;;;;;;;;;;;;;;;;;;;;;
TX2:	
	LDI32 r26, 0x00001000 ;* Address of memory for tx1
	LBBO &r7, r4, 0, 4
	QBBC NOT_TX2, r28, 1
	QBNE CONT_TX2, r4, r26
	LBBO &r6, r4, 0, 4
	LDI32 r5, 0x00000000
	LDI32 r4, 0x00001004
	SUB r23, r23, 2
	SUB r23, r23, 1
	
CONT_TX2:
	SUB r23, r23, 1
	QBNE STILL_TX2, r6, 0
	CLR r22, r22.t7
	SET r22, r22.t6;; reset illumination pin
	CLR r28, r28.t1
	LDI32 r4, 0x00001000
	SUB r23, r23, 3
	SUB r23, r23, 1
	JMP NOT_TX2
STILL_TX2:	
	SUB r23, r23, 1
	CLR r22, r22.t6;; Clear the illumination pin
	;LBBO &r7, r4, 0, 4
	LSL r7, r7, r5
	ADD r5, r5, 1
	QBBC ZERO2, r7, 31
	SET r22, r22.t7;; Set the pin
	JMP END_BIT2
ZERO2:
	CLR r22, r22.t7;; Clear the pin
	JMP END_BIT2
	ADD r23, r23, 1
	SUB r23, r23, 2
END_BIT2:
	SUB r6, r6, 1
	SUB r23, r23, 4;SUB r23, r23, 5
	SUB r23, r23, 1
	QBNE NOT_TX2, r5, 32
	ADD r4, r4, 4
	LDI32 r5, 0x00000000
	SUB r23, r23, 1
	SUB r23, r23, 1
NOT_TX2:
	SUB r23, r23, 1
	SUB r23, r23, 2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;; TX3 ;;;;;;;;;;;;;;;;;;;;;;;
TX3:	
	LDI32 r26, 0x00002000 ;* Address of memory for tx1
	LBBO &r11, r8, 0, 4
	QBBC NOT_TX3, r28, 2
	QBNE CONT_TX3, r8, r26
	LBBO &r10, r8, 0, 4
	LDI32 r9, 0x00000000
	LDI32 r8, 0x00002004
	SUB r23, r23, 2
	SUB r23, r23, 1
	
CONT_TX3:
	SUB r23, r23, 1
	QBNE STILL_TX3, r10, 0
	CLR r22, r22.t5
	SET r22, r22.t4;; reset illumination pin
	CLR r28, r28.t2
	LDI32 r8, 0x00002000
	SUB r23, r23, 3
	SUB r23, r23, 1
	JMP NOT_TX3
STILL_TX3:	
	SUB r23, r23, 1
	CLR r22, r22.t4;; Clear the illumination pin
	;LBBO &r11, r8, 0, 4
	LSL r11, r11, r9
	ADD r9, r9, 1
	QBBC ZERO3, r11, 31
	SET r22, r22.t5;; Set the pin
	JMP END_BIT3
ZERO3:
	CLR r22, r22.t5;; Clear the pin
	JMP END_BIT3
	ADD r23, r23, 1
	SUB r23, r23, 2
END_BIT3:
	SUB r10, r10, 1
	SUB r23, r23, 4;SUB r23, r23, 5
	SUB r23, r23, 1
	QBNE NOT_TX3, r9, 32
	ADD r8, r8, 4
	LDI32 r9, 0x00000000
	SUB r23, r23, 1
	SUB r23, r23, 1
NOT_TX3:
	SUB r23, r23, 1
	SUB r23, r23, 2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;; TX4 ;;;;;;;;;;;;;;;;;;;;;;;
TX4:	
	LDI32 r26, 0x00002000 ;* Address of memory for tx1
	LBBO &r15, r12, 0, 4
	QBBC NOT_TX4, r28, 3
	QBNE CONT_TX4, r12, r26
	LBBO &r14, r12, 0, 4
	LDI32 r13, 0x00000000
	LDI32 r12, 0x00002004
	SUB r23, r23, 2
	SUB r23, r23, 1
	
CONT_TX4:
	SUB r23, r23, 1
	QBNE STILL_TX4, r14, 0
	CLR r22, r22.t3
	SET r22, r22.t2;; reset illumination pin
	CLR r28, r28.t3
	LDI32 r12, 0x00002000
	SUB r23, r23, 3
	SUB r23, r23, 1
	JMP NOT_TX4
STILL_TX4:	
	SUB r23, r23, 1
	CLR r22, r22.t2;; Clear the illumination pin
	;LBBO &r15, r12, 0, 4
	LSL r15, r15, r13
	ADD r13, r13, 1
	QBBC ZERO4, r15, 31
	SET r22, r22.t3;; Set the pin
	JMP END_BIT4
ZERO4:
	CLR r22, r22.t3;; Clear the pin
	JMP END_BIT4
	ADD r23, r23, 1
	SUB r23, r23, 2
END_BIT4:
	SUB r14, r14, 1
	SUB r23, r23, 4;SUB r23, r23, 5
	SUB r23, r23, 1
	QBNE NOT_TX4, r13, 32
	ADD r12, r12, 4
	LDI32 r13, 0x00000000
	SUB r23, r23, 1
	SUB r23, r23, 1
NOT_TX4:
	SUB r23, r23, 1
	SUB r23, r23, 2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	QBNE MASTER, r29, 0
WAIT_PREPRE:
	LBBO &r25, r24, 0, 4 ;* Wait here for the prepreamble
	QBNE WAIT_PREPRE, r25, 1
	SET r29, r29.t0
	SBBO &r27, r24, 0, 4
	SUB r23, r23, 2

	
MASTER:	
	MOV r30, r22
	SUB r23, r23, 1
	SUB r23, r23, 1
WAIT:	
	SUB r23, r23, 1
	QBNE WAIT, r23, 0
		
	
	QBEQ TX, r28, 0
	JMP CONT_TX

