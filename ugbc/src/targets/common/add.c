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
 * "COSÌ COM'È", SENZA GARANZIE O CONDIZIONI DI ALCUN TIPO, esplicite o
 * implicite. Consultare la Licenza per il testo specifico che regola le
 * autorizzazioni e le limitazioni previste dalla medesima.
 ****************************************************************************/

/****************************************************************************
 * INCLUDE SECTION 
 ****************************************************************************/

#include "../../ugbc.h"

/****************************************************************************
 * CODE SECTION 
 ****************************************************************************/

/**
 * @brief Emit code for <strong>ADD x,y,a TO b</strong>
 * 
 * @param _environment Current calling environment
 * @param _variable Variable to operate on
 * @param _expression Expression to add to the variable
 * @param _limit_lower Lower limit
 * @param _limit_upper Upper limit
 */
/* <usermanual>
@keyword ADD

@english

The ''ADD'' statement is used to increment the value of a numeric variable by 
a specified amount. In other words, it is like adding one number to another. 

The basic syntax take the ''var'' to which you want 
to add a value and the ''expr'' as the expression you want to add to the variable. 
The full syntax takes also two additional parameters: ''min'' and ''max'',
that are the minimum and maximum value that the variable can take after
the increment. In other words, the ''var''  is incremented, but its value is 
"squeezed" between ''min'' and ''max''. If the result of the addition 
had been greater than ''max'', the level would still have been set to ''max''.

The purpose of this second syntax is to prevent a variable from taking 
invalid value for your program. It can also help to simulating real-world 
systems: for example, in a game you can limit a character's life between 
0 and 100. This instruction can also help to create special effects: you can 
create bouncing or wrapping effects, by making a variable "bounce" between 
two values. In videogames, the typical use is to limit the maximum score 
in a game, or to preventing a difficulty level from exceeding a certain value.

@italian
L'istruzione ''ADD'' viene utilizzata per incrementare il valore di una 
variabile numerica di una quantità specificata. In altre parole, è come 
aggiungere un numero a un altro.

La sintassi di base prende la ''var'' a cui vuoi aggiungere un 
valore e il ''expr'' come espressione che si vuol aggiungere alla variabile.
La sintassi completa prende anche due parametri aggiuntivi: ''min'' e 
''max'', che sono il valore minimo e massimo che la variabile può 
assumere dopo l'incremento. In altre parole, la ''variable'' viene 
incrementata, ma il suo valore viene "compresso" tra ''min'' e ''max''. 
Se il risultato dell'addizione fosse stato maggiore di ''max'', 
il livello sarebbe stato comunque impostato su ''max''.

Lo scopo di questa seconda sintassi è impedire a una variabile di 
assumere valori non validi per il tuo programma. Può anche aiutare 
a simulare sistemi del mondo reale: ad esempio, in un gioco puoi 
limitare la vita di un personaggio tra 0 e 100. Questa istruzione 
può anche aiutare a creare effetti speciali: puoi creare effetti 
di rimbalzo o di avvolgimento, facendo "rimbalzare" una variabile
tra due valori. Nei videogiochi, l'uso tipico è quello di limitare 
il punteggio massimo in un gioco o di impedire che un livello di 
difficoltà superi un certo valore.

@syntax ADD var, expr [, min TO max]

@example ADD y,10
@example ADD x,42,1 TO 100

@usedInExample maths_fast_02.bas

@target all
</usermanual> */
void add_complex_vars( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper ) { 

    MAKE_LABEL

    char lessThanLabel[MAX_TEMPORARY_STORAGE]; sprintf( lessThanLabel, "%sl", label );
    char greaterThanLabel[MAX_TEMPORARY_STORAGE]; sprintf( greaterThanLabel, "%sg", label );
    char endLabel[MAX_TEMPORARY_STORAGE]; sprintf( endLabel, "%se", label );
    
    variable_add_inplace_vars( _environment, _variable, _expression );

    if ( _limit_lower ) {

        Variable * less = variable_less_than( _environment, _variable, _limit_lower, 0 );

        cpu_bveq( _environment, less->realName, greaterThanLabel );

        variable_move( _environment, _limit_upper, _variable );

        cpu_jump( _environment, endLabel );

        cpu_label( _environment, greaterThanLabel );

        if ( _limit_upper ) {

            Variable * greater = variable_greater_than( _environment, _variable, _limit_upper, 0 );

            cpu_bveq( _environment, greater->realName, endLabel );

            variable_move( _environment, _limit_lower, _variable );

        }
        
        cpu_label( _environment, endLabel );

    }

}

/**
 * @brief Emit code for <strong>ADD x,y,a TO b</strong>
 * 
 * @param _environment Current calling environment
 * @param _variable Variable to operate on
 * @param _expression Expression to add to the variable
 * @param _limit_lower Lower limit
 * @param _limit_upper Upper limit
 */
void add_complex_mt( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper ) { 

    parser_array_init( _environment );
    parser_array_index_symbolic( _environment, "PROTOTHREADCT" );
    Variable * array = variable_retrieve( _environment, _variable );
    if ( array->type != VT_TARRAY ) {
        CRITICAL_NOT_ARRAY( _variable );
    }
    Variable * value = variable_move_from_array( _environment, array->name );
    parser_array_cleanup( _environment );

    add_complex_vars( _environment, value->name, _expression, _limit_lower, _limit_upper );

    parser_array_init( _environment );
    parser_array_index_symbolic( _environment, "PROTOTHREADCT" );
    array = variable_retrieve( _environment, _variable );
    if ( array->type != VT_TARRAY ) {
        CRITICAL_NOT_ARRAY( _variable );
    }
    variable_move_array( _environment, array->name, value->name );
    parser_array_cleanup( _environment );

}

/**
 * @brief Emit code for <strong>ADD x,y,a TO b</strong>
 * 
 * @param _environment Current calling environment
 * @param _variable Variable to operate on
 * @param _expression Expression to add to the variable
 * @param _limit_lower Lower limit
 * @param _limit_upper Upper limit
 */
void add_complex_array( Environment * _environment, char * _variable, char * _expression, char * _limit_lower, char * _limit_upper ) { 

    Variable * array = variable_retrieve( _environment, _variable );
    if ( array->type != VT_TARRAY ) {
        CRITICAL_NOT_ARRAY( _variable );
    }
    Variable * value = variable_move_from_array( _environment, array->name );

    add_complex_vars( _environment, value->name, _expression, _limit_lower, _limit_upper );

    variable_move_array( _environment, array->name, value->name );

}

void add_complex( Environment * _environment, char * _variable, int _expression, int _limit_lower, int _limit_upper ) { 

    MAKE_LABEL

    char lessThanLabel[MAX_TEMPORARY_STORAGE]; sprintf( lessThanLabel, "%sl", label );
    char greaterThanLabel[MAX_TEMPORARY_STORAGE]; sprintf( greaterThanLabel, "%sg", label );
    char endLabel[MAX_TEMPORARY_STORAGE]; sprintf( endLabel, "%se", label );
    
    variable_add_inplace( _environment, _variable, _expression );

    Variable * less = variable_less_than_const( _environment, _variable, _limit_lower, 0 );

    cpu_bveq( _environment, less->realName, greaterThanLabel );

    variable_store( _environment, _variable, _limit_upper );

    cpu_jump( _environment, endLabel );

    cpu_label( _environment, greaterThanLabel );

    Variable * lesser = variable_less_than_const( _environment, _variable, _limit_upper, 1 );

    cpu_bvneq( _environment, lesser->realName, endLabel );

    variable_store( _environment, _variable, _limit_lower );

    cpu_label( _environment, endLabel );

}
