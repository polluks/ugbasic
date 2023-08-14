﻿REM EXTERNAL INTEGRATION CALL MACHINE LANGUAGE IN BINARY FILE USING STACK
REM 
REM This small example explains how to invoke external
REM machine language, loaded from a binary file
REM on the system into a specific location. The external machine 
REM routine will receive parameters from stack and store the result
REM on the stack.
REM 
REM @italian
REM INTEGRAZIONE CODICE ESTERNO CHIAMARE CODICE MACCHINA DA FILE BINARIO CON USO DELLO STACK
REM
REM Questo piccolo esempio spiega come invocare codice
REM macchina esterno, caricato a sistema da un file binario.
REM La routine esterna riceverà parametri dallo stack
REM e restituirà il risultato sempre sullo stack.

	CLS
	
	executable := LOAD("executable_sum_stack.bin")
	
	address = VARPTR(executable)
	
	DIM op1 AS WORD, op2 AS WORD, sum AS WORD
	
	op1 = 42: op2 = 21
	
	SYS address WITH STACK(WORD)=op1, STACK(WORD)=op2 RETURN sum=STACK(WORD)
	
	PRINT sum
	