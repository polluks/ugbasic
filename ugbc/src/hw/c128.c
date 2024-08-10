/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021-2024 Marco Spedaletti (asimov@mclink.it)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *----------------------------------------------------------------------------
 * Concesso in licenza secondo i termini della Licenza Apache, versione 2.0
 * (la "Licenza"); è proibito usare questo file se non in conformità alla
 * Licenza. Una copia della Licenza è disponibile all'indirizzo:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Se non richiesto dalla legislazione vigente o concordato per iscritto,
 * il software distribuito nei termini della Licenza è distribuito
 * "COSì COM'è", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
 * implicite. Consultare la Licenza per il testo specifico che regola le
 * autorizzazioni e le limitazioni previste dalla medesima.
 ****************************************************************************/

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#ifdef __c128__

#include "../ugbc.h"

/****************************************************************************
 * CODE SECTION
 ****************************************************************************/

void c128_xpen( Environment * _environment, char * _destination ) {

    MAKE_LABEL

    outline0("LDA $D013");    
    outline0("ASL" );
    outline1("STA %s", _destination);
    outline1("BCC %s", label );
    outline0("LDA #1");    
    outline1("STA _%s", address_displacement(_environment, _destination, "1"));
    outhead1("%s:", label );

}

void c128_ypen( Environment * _environment, char * _destination ) {

    MAKE_LABEL

    outline0("LDA $d014");
    outline1("STA %s", _destination);    
    outline0("LDA #0");
    outline1("STA %s", address_displacement(_environment, _destination, "1"));    
   
}

void c128_inkey( Environment * _environment, char * _key ) {

    deploy( keyboard, src_hw_c128_keyboard_asm);

    outline0("JSR INKEY");
    outline1("STA %s", _key);

}

void c128_wait_key( Environment * _environment, int _release ) {

    deploy( keyboard, src_hw_c128_keyboard_asm );

    if ( _release ) {
        outline0("JSR WAITKEYRELEASE");
    } else {
        outline0("JSR WAITKEY");
    }
   
}

void c128_key_state( Environment * _environment, char *_scancode, char * _result ) {

    _environment->bitmaskNeeded = 1;

    MAKE_LABEL

    deploy( keyboard, src_hw_c128_keyboard_asm );

    outline1("LDX %s", _scancode);
    outline0("JSR KEYSTATE");
    cpu_ctoa( _environment );
    outline1("STA %s", _result);

}

void c128_scancode( Environment * _environment, char * _result ) {

    _environment->bitmaskNeeded = 1;

    deploy( keyboard, src_hw_c128_keyboard_asm);

    outline0("JSR SCANCODE");
    outline1("STA %s", _result );
   
}

void c128_asciicode( Environment * _environment, char * _result ) {

    _environment->bitmaskNeeded = 1;

    deploy( keyboard, src_hw_c128_keyboard_asm);

    outline0("JSR ASCIICODE");
    outline1("STA %s", _result );
   
}

void c128_key_pressed( Environment * _environment, char *_scancode, char * _result ) {

    _environment->bitmaskNeeded = 1;

    MAKE_LABEL

    deploy( keyboard, src_hw_c128_keyboard_asm );

    outline1("LDX %s", _scancode);
    outline0("JSR KEYPRESSED");
    cpu_ctoa( _environment );
    outline1("STA %s", _result);

}

void c128_scanshift( Environment * _environment, char * _shifts ) {

    MAKE_LABEL

    outline0("LDA #0");
    outline1("STA %s", _shifts);
    outline0("LDA #$10");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$80");
    outline1("BNE %snoleft", label);
    outline0("LDA #1");
    outline1("STA %s", _shifts);
    outhead1("%snoleft:", label );

    outline0("LDA #$20");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$10");
    outline1("BNE %snoright", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #2");
    outline1("STA %s", _shifts);
    outhead1("%snoright:", label );

}

void c128_keyshift( Environment * _environment, char * _shifts ) {

    deploy( scancode, src_hw_c128_scancode_asm);

    MAKE_LABEL

    outline0("JSR SCANCODE");

    outline0("LDA #0");
    outline1("STA %s", _shifts);
    outline0("LDA #$10");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$80");
    outline1("BNE %snoleft", label);
    outline0("LDA #1");
    outline1("STA %s", _shifts);
    outhead1("%snoleft:", label );

    outline0("LDA #$20");
    outline0("STA $DC00");
    outline0("LDA $DC01");
    outline0("AND #$10");
    outline1("BNE %snoright", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #2");
    outline1("STA %s", _shifts);
    outhead1("%snoright:", label );

    outline0("LDA $028D");
    outline0("AND #$01");
    outline1("BEQ %snocaps", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #4");
    outline1("STA %s", _shifts);
    outhead1("%snocaps:", label );

    outline0("LDA $028D");
    outline0("AND #$04");
    outline1("BEQ %snocontrol", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #8");
    outline1("STA %s", _shifts);
    outhead1("%snocontrol:", label );

    outline0("LDA $028D");
    outline0("AND #$02");
    outline1("BEQ %snoalt", label);
    outline1("LDA %s", _shifts);
    outline0("ORA #$30");
    outline1("STA %s", _shifts);
    outhead1("%snoalt:", label );

}

void c128_clear_key( Environment * _environment ) {

    outline0("JSR CLEARKEY");
   
}

void c128_sys_call( Environment * _environment, int _destination ) {

    outline0("PHA");
    outline1("LDA #$%2.2x", (_destination & 0xff ) );
    outline0("STA SYSCALL0+1");
    outline1("LDA #$%2.2x", ((_destination>>8) & 0xff ) );
    outline0("STA SYSCALL0+2");
    outline0("PLA");
    outline0("JSR SYSCALL");

}

void c128_timer_set_status_on( Environment * _environment, char * _timer ) {
    
    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline0("LDY #$1" );
    outline0("JSR TIMERSETSTATUS" );

}

void c128_timer_set_status_off( Environment * _environment, char * _timer ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline0("LDY #$0" );
    outline0("JSR TIMERSETSTATUS" );

}

void c128_timer_set_counter( Environment * _environment, char * _timer, char * _counter ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    if ( _counter ) {
        outline1("LDA %s", _counter );
    } else {
        outline0("LDA #0" );
    }
    outline0("STA MATHPTR2");
    if ( _counter ) {
        outline1("LDA %s", address_displacement( _environment, _counter, "1" ) );
    }
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETCOUNTER" );

}

void c128_timer_set_init( Environment * _environment, char * _timer, char * _init ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline1("LDA %s", _init );
    outline0("STA MATHPTR2");
    outline1("LDA %s", address_displacement( _environment, _init, "1" ) );
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETINIT" );

}

void c128_timer_set_address( Environment * _environment, char * _timer, char * _address ) {

    deploy( timer, src_hw_6502_timer_asm);

    if ( _timer ) {
        outline1("LDX %s", _timer );
    } else {
        outline0("LDX #0" );
    }
    outline1("LDA #<%s", _address );
    outline0("STA MATHPTR2");
    outline1("LDA #>%s", _address );
    outline0("STA MATHPTR3");
    outline0("JSR TIMERSETADDRESS" );

}

void c128_dload( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size ) {

    deploy( dload, src_hw_c128_dload_asm);

    MAKE_LABEL
    
    Variable * filename = variable_retrieve( _environment, _filename );
    Variable * tnaddress = variable_temporary( _environment, VT_ADDRESS, "(address of target_name)");
    Variable * tnsize = variable_temporary( _environment, VT_BYTE, "(size of target_name)");

    Variable * address = NULL;
    if ( _address ) {
        address = variable_retrieve( _environment, _address );
    }
    Variable * size = NULL;
    if ( _size ) {
        size = variable_retrieve( _environment, _size );
    }

    switch( filename->type ) {
        case VT_STRING:
            cpu_move_8bit( _environment, filename->realName, tnsize->realName );
            cpu_addressof_16bit( _environment, filename->realName, tnaddress->realName );
            cpu_inc_16bit( _environment, tnaddress->realName );
            break;
        case VT_DSTRING:
            cpu_dsdescriptor( _environment, filename->realName, tnaddress->realName, tnsize->realName );
            break;
    }

    outline1("LDA %s", tnaddress->realName);
    outline0("STA TMPPTR");
    outline1("LDA %s", address_displacement(_environment, tnaddress->realName, "1"));
    outline0("STA TMPPTR+1");
    outline1("LDA %s", tnsize->realName);
    outline0("STA MATHPTR0");

    if ( address ) {

        outline1("LDA %s", address->realName);
        outline0("STA TMPPTR2");
        outline1("LDA %s", address_displacement(_environment, address->realName, "1"));
        outline0("STA TMPPTR2+1");
        outline0("LDA #0");
        outline0("STA MATHPTR1");

    }

    outline0("JSR C128DLOAD");

}

void c128_dsave( Environment * _environment, char * _filename, char * _offset, char * _address, char * _size ) {

    deploy( dsave, src_hw_c128_dsave_asm);

    MAKE_LABEL
    
    Variable * filename = variable_retrieve( _environment, _filename );
    Variable * tnaddress = variable_temporary( _environment, VT_ADDRESS, "(address of target_name)");
    Variable * tnsize = variable_temporary( _environment, VT_BYTE, "(size of target_name)");

    Variable * address = NULL;
    if ( _address ) {
        address = variable_retrieve( _environment, _address );
    }
    Variable * size = NULL;
    if ( _size ) {
        size = variable_retrieve( _environment, _size );
    }

    switch( filename->type ) {
        case VT_STRING:
            cpu_move_8bit( _environment, filename->realName, tnsize->realName );
            cpu_addressof_16bit( _environment, filename->realName, tnaddress->realName );
            cpu_inc_16bit( _environment, tnaddress->realName );
            break;
        case VT_DSTRING:
            cpu_dsdescriptor( _environment, filename->realName, tnaddress->realName, tnsize->realName );
            break;
    }

    outline1("LDA %s", tnaddress->realName);
    outline0("STA TMPPTR");
    outline1("LDA %s", address_displacement(_environment, tnaddress->realName, "1"));
    outline0("STA TMPPTR+1");
    outline1("LDA %s", tnsize->realName);
    outline0("STA MATHPTR0");

    if ( address ) {

        outline1("LDA %s", address->realName);
        outline0("STA TMPPTR2");
        outline1("LDA %s", address_displacement(_environment, address->realName, "1"));
        outline0("STA TMPPTR2+1");
        outline0("LDA #0");
        outline0("STA MATHPTR1");

    }

    if ( size ) {

        outline1("LDA %s", size->realName);
        outline0("STA MATHPTR4");
        outline1("LDA %s", address_displacement(_environment, size->realName, "1"));
        outline0("STA MATHPTR5");

    } else {

        outline0("LDA #$ff");
        outline0("STA MATHPTR4");
        outline0("STA MATHPTR5");

    }

    outline0("JSR C128DSAVE");

}

void c128_put_key(  Environment * _environment, char *_string, char * _size ) {

    outline1("LDA %s", _string );
    outline0("STA TMPPTR" );
    outline1("LDA %s", address_displacement( _environment, _string, "1" ) );
    outline0("STA TMPPTR+1" );
    outline1("LDX %s", _size );
    outline0("JSR PUTKEY" );

}

#endif