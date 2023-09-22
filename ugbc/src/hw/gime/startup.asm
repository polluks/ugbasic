; /*****************************************************************************
;  * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
;  *****************************************************************************
;  * Copyright 2021-2023 Marco Spedaletti (asimov@mclink.it)
;  *
;  * Licensed under the Apache License, Version 2.0 (the "License");
;  * you may not use this file except in compliance with the License.
;  * You may obtain a copy of the License at
;  *
;  * http://www.apache.org/licenses/LICENSE-2.0
;  *
;  * Unless required by applicable law or agreed to in writing, software
;  * distributed under the License is distributed on an "AS IS" BASIS,
;  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;  * See the License for the specific language governing permissions and
;  * limitations under the License.
;  *----------------------------------------------------------------------------
;  * Concesso in licenza secondo i termini della Licenza Apache, versione 2.0
;  * (la "Licenza"); è proibito usare questo file se non in conformità alla
;  * Licenza. Una copia della Licenza è disponibile all'indirizzo:
;  *
;  * http://www.apache.org/licenses/LICENSE-2.0
;  *
;  * Se non richiesto dalla legislazione vigente o concordato per iscritto,
;  * il software distribuito nei termini della Licenza è distribuito
;  * "COSì COM'è", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
;  * implicite. Consultare la Licenza per il testo specifico che regola le
;  * autorizzazioni e le limitazioni previste dalla medesima.
;  ****************************************************************************/
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
;*                                                                             *
;*                            STARTUP ROUTINE FOR GIME                         *
;*                                                                             *
;*                             by Marco Spedaletti                             *
;*                                                                             *
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

GIMEINIT0   equ   $FF90
GIMEINIT1   equ   $FF91
GIMEIRQENR  equ   $FF92
GIMEFIRQENR equ   $FF93
GIMETIMSB   equ   $FF94
GIMETILSB   equ   $FF95
GIMERSVD1   equ   $FF96
GIMERSVD2   equ   $FF97
GIMEVIDM    equ   $FF98
GIMEVIDR    equ   $FF99
GIMEBRDR    equ   $FF9A
GIMERSVD3   equ   $FF9B
GIMEVSCRL   equ   $FF9C
GIMEVOFF1   equ   $FF9D
GIMEVOFF0   equ   $FF9E
GIMEHOFF    equ   $FF9F

GIMEMMU0   equ   $FFA0
GIMEMMU1   equ   $FFA1
GIMEMMU2   equ   $FFA2
GIMEMMU3   equ   $FFA3
GIMEMMU4   equ   $FFA4
GIMEMMU5   equ   $FFA5
GIMEMMU6   equ   $FFA6
GIMEMMU7   equ   $FFA7

GIMEMMU8   equ   $FFA8
GIMEMMU9   equ   $FFA9
GIMEMMUA   equ   $FFAA
GIMEMMUB   equ   $FFAB
GIMEMMUC   equ   $FFAC
GIMEMMUD   equ   $FFAD
GIMEMMUE   equ   $FFAE
GIMEMMUF   equ   $FFAF

GIMESTARTUP

	LDA   #%01001100
	STA   GIMEINIT0
	LDA   #%00000011
	STA   GIMEVIDM
	LDA   #%00100101
	STA   GIMEVIDR
	LDA   #0
	STA   GIMEBRDR
	CLR   GIMEVSCRL
	LDA   #$C0
	STA   GIMEVOFF1
	CLR   GIMEVOFF0
	CLR   GIMEHOFF

    LDA   #$30
    STA   GIMEMMU6

    RTS