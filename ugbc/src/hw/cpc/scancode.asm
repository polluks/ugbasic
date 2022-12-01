; /*****************************************************************************
;  * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
;  *****************************************************************************
;  * Copyright 2021-2022 Marco Spedaletti (asimov@mclink.it)
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
;*                      SCAN CODE ROUTINE FOR CPC                              *
;*                                                                             *
;*                             by Marco Spedaletti                             *
;*                                                                             *
;* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

KEYMAP:      DS 10

SCANCODEENTIRE:
    DI
    LD HL, KEYMAP
    LD BC, $f782
    OUT (C), C
    LD BC, $f40e
    LD E, B
    OUT (C), C
    LD BC, $f6c0
    LD D, B
    OUT (C), C
    XOR A
    OUT (C), A
    LD BC, $f792
    OUT (C), C
    LD A, $40
    LD C, $4A
SCANCODEENTIREL1:
    LD B, D
    OUT (C), A
    LD B, E
    INI
    INC A
    CP C
    JR C, SCANCODEENTIREL1
    LD BC, $f782
    OUT (C), C
    EI
    RET

SCANCODEPRECISE:
    CALL SCANCODEENTIRE
	PUSH DE
	LD HL, BITMASK
	LD A, 0
	LD D, A
    LD A, E
    AND $07
    LD E, A
	ADC HL, DE
	LD A, (HL)
	LD B, A
    POP DE
    LD HL, KEYMAP
    SRL D
    RR E
    SRL D
    RR E
    SRL D
    RR E
    ADC HL, DE
    AND B
	CP B
    RET

SCANCODE:
    CALL SCANCODEENTIRE
    LD HL, KEYMAP
    LD A, $1
    LD E, A
    LD A, $0
    LD D, A
    LD B, A
SCANCODEL10:
    LD A, $0
    ; LD E, A
SCANCODEL1:
    PUSH BC
    PUSH AF
	PUSH DE
    PUSH HL
	LD HL, BITMASK
	LD A, 0
	LD D, A
    LD A, E
    AND $07
    LD E, A
	ADD HL, DE
	LD A, (HL)
	LD B, A
    POP HL
    POP DE

    LD A, (HL)
    XOR $ff
    AND B
    CP B
    JR Z, SCANCODEVALUE
    POP AF
    POP BC
    INC E
    INC A
    CP 8
    JR NZ, SCANCODEL1
    INC HL
    INC B
    LD A, B
    CP 10
    JR NZ, SCANCODEL10 
    LD A, $0
    LD D, A
    LD E, A
    PUSH BC
    PUSH AF
    JMP SCANCODEVALUE2
SCANCODEVALUE:
SCANCODEVALUE2:
    LD IXH, A
    POP AF
    POP BC
    LD A, IXH
    RET