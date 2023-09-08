REM @english
REM MATHEMATIC ROUTINES WITH FAST OPERATIONS (USING MUL)
REM
REM This example will show the use of fast integer operators, with the use of ''MUL'' instruction. 
REM Those instructions will operate without the use of intermediate results, in order
REM to give the maximum throughtput in multiplicate values.
REM
REM @italian
REM ROUTINE DI MATEMATICA CON PRIORITA' ASSOCIATIVA (3)
REM
REM Questo esempio mostrerà l'uso di operatori interi veloci, con l'uso dell'istruzione ''MUL''. 
REM Tali istruzioni funzioneranno senza l'uso di risultati intermedi, al fine di fornire il massimo 
REM rendimento nel prodotto di valori. 
REM
REM @include atari,atarixl,c128,c64,coco,coleco,cpc

   CLS
   
   a% = 42
   b% = 10
   MUL a%, b%
   PRINT "42 * 10 = ";a%
   