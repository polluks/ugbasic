/*****************************************************************************
 * ugBASIC - an isomorphic BASIC language compiler for retrocomputers        *
 *****************************************************************************
 * Copyright 2021-2023 Marco Spedaletti (asimov@mclink.it)
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

#if defined(__c64__) || defined(__c128__)

#include "../ugbc.h"
#include <math.h>

static RGBi SYSTEM_PALETTE[] = {
        { 0x00, 0x00, 0x00, 0xff, 0, "BLACK" },
        { 0xff, 0xff, 0xff, 0xff, 1, "WHITE" },
        { 0x80, 0x00, 0x00, 0xff, 2, "RED" },
        { 0xaa, 0xff, 0xe6, 0xff, 3, "CYAN" },
        { 0xcc, 0x44, 0xcc, 0xff, 4, "VIOLET" },
        { 0x00, 0xcc, 0x55, 0xff, 5, "GREEN" },
        { 0x00, 0x00, 0xaa, 0xff, 6, "BLUE" },
        { 0xee, 0xee, 0x77, 0xff, 7, "YELLOW" },
        { 0xa1, 0x68, 0x3c, 0xff, 8, "ORANGE" },
        { 0xdd, 0x88, 0x65, 0xff, 9, "BROWN" },
        { 0xff, 0x77, 0x77, 0xff, 10, "LIGHT_RED" },
        { 0x33, 0x33, 0x33, 0xff, 11, "DARK_GREY" },
        { 0x77, 0x77, 0x77, 0xff, 12, "GREY" },
        { 0xaa, 0xff, 0x66, 0xff, 13, "LIGHT GREEN" },
        { 0x00, 0x88, 0xff, 0xff, 14, "LIGHT BLUE" },
        { 0xbb, 0xbb, 0xbb, 0xff, 15, "LIGHT GREY" }
};

static RGBi * commonPalette;
int lastUsedSlotInCommonPalette = 0;

/****************************************************************************
 * CODE SECTION
 ****************************************************************************/

RGBi * vic2_image_nearest_system_color( RGBi * _color ) {

    unsigned int minDistance = 0xffff;
    int colorIndex = 0;
    for (int j = 0; j < COLOR_COUNT; ++j) {
        int distance = rgbi_distance(&SYSTEM_PALETTE[j], _color);
        if (distance < minDistance) {
            minDistance = distance;
            colorIndex = j;
        }
    }

    return &SYSTEM_PALETTE[colorIndex];

}

/**
 * This method can be used to convert 
 *     8x8 RGB (3 bytes) pixel (_source) [8x8x3 = 192 bytes]
 * into 
 *     8x8 bitmap (1 bit) pixel + 1 (byte) [8x1 + 1 = 9 bytes]
 *       foreground and background color (_dest)
 * 
 * Since the 8x8 pixel area belong to a larger picture,
 * this function will need the picture _width in order
 * to move to the next line to analyze.
 */
static void vic2_image_converter_tile( Environment * _environment, char * _source, char * _dest, int _width, int _depth, int _source_width ) {

    int colorIndexesCount[COLOR_COUNT];
    memset(colorIndexesCount, 0, COLOR_COUNT * sizeof( int ) );
    int trans = 0;

    char * source = _source;

    // Clear the box and colors
    memset( _dest, 0, 9 );

    // Loop for all the box surface
    for (int y=0; y<8; ++y) {
        for (int x=0; x<8; ++x) {

            RGBi rgb;

            memset( &rgb, 0, sizeof( RGBi ) );

            // Take the color of the pixel
            rgb.red = *source;
            rgb.green = *(source + 1);
            rgb.blue = *(source + 2);
            if ( _depth > 3 ) {
                rgb.alpha = *(_source + 3);
            } else {
                rgb.alpha = 255;
            }
            if ( rgb.alpha == 0 ) {
                rgb.red = 0;
                rgb.green = 0;
                rgb.blue = 0;
            }

            if ( rgb.alpha < 255 ) {
                trans = 1;
            } else {
                RGBi *systemRgb = vic2_image_nearest_system_color( &rgb );
                ++colorIndexesCount[systemRgb->index];
            }

            source += _depth;

        }

        source += _depth * ( _source_width - 8 );

    }

    int colorBackground = 0;
    int colorBackgroundMax = 0;
    int colorForeground = 0;
    int colorForegroundMax = 0;
    for( int xx = 0; xx<COLOR_COUNT; ++xx ) {
        if ( colorIndexesCount[xx] > colorBackgroundMax ) {
            colorBackground = xx;
            colorBackgroundMax = colorIndexesCount[xx];
        };
    }

    colorIndexesCount[colorBackground] = 0;

    for( int xx = 0; xx<COLOR_COUNT; ++xx ) {
        if ( colorIndexesCount[xx] > colorForegroundMax ) {
            colorForeground = xx;
            colorForegroundMax = colorIndexesCount[xx];
        };
    }

    if ( trans ) {
        if ( colorForeground == 0 ) {
            colorForeground = colorBackground; 
            colorBackground = 0;
        } else {
            colorBackground = 0;
        }
    }

    if ( colorForeground == colorBackground ) {
        colorForeground = ( colorBackground == 0 ) ? 1 : 0;
    }

    source = _source;

    for (int y=0; y<8; ++y) {
        for (int x=0; x<8; ++x) {

            RGBi rgb;

            memset( &rgb, 0, sizeof( RGBi ) );

            rgb.red = *source;
            rgb.green = *(source + 1);
            rgb.blue = *(source + 2);
            if ( _depth > 3 ) {
                rgb.alpha = *(_source + 3);
            } else {
                rgb.alpha = 255;
            }
            if ( rgb.alpha == 0 ) {
                rgb.red = 0;
                rgb.green = 0;
                rgb.blue = 0;
            }

            RGBi *systemRgb = vic2_image_nearest_system_color( &rgb );

            char bitmask = 1 << ( 7 - ((x) & 0x7) );

            if ( rgb.alpha < 255 ) {
                *( _dest + y ) &= ~bitmask;
                adilinepixel(colorBackground);
            } else {
                if ( systemRgb->index != colorBackground ) {
                    adilinepixel(colorForeground);
                    *( _dest + y ) |= bitmask;
                    // printf("*");
                } else {
                   adilinepixel(colorBackground);
                     *( _dest + y ) &= ~bitmask;
                    // printf(" ");
                }
            }

            source += _depth;

        }

        source += _depth * ( _source_width - 8 );

    }

    *( _dest + 8 ) = ( colorForeground << 4 ) | colorBackground ;

}

/**
 * This method can be used to convert 
 *     WxH RGB (3/4 bytes) pixel (_source) [WxHx3/4 bytes]
 * into 
 *     WxH bitmap (1 bit) pixel + (W/8xH + W/8xH/8) (bytes)
 *       foreground and background color (_dest)
 * 
 * Since the WXH pixel area could belong to a larger picture,
 * this function will need the picture _source_width in order
 * to move to the next line to analyze.
 */
static void vic2_image_converter_tiles( Environment * _environment, char * _source, char * _dest, int _width, int _height, int _depth, int _source_width ) {

    int bitmapSize = ( _width>>3 ) * _height;
    int colormapSize = ( _width>>3 ) * (_height>>3);

    memset( _dest, 0, bitmapSize + colormapSize );

    adilinebeginbitmap("BMD2");

    for( int y=0; y<_height; y+=8 ) {
        for( int x=0; x<_width; x+=8 ) {

            char * source = _source + ( ( y * _source_width ) + x ) * _depth;
            char tile[9];

            vic2_image_converter_tile( _environment, source, tile, _width, _depth, _source_width );

            int offset = ((y>>3) * 8 *( _width >> 3 ) ) + ((x>>3) * 8) + ((y) & 0x07);
            // x = 8, y = 8
            // offset = ((8 >> 3) * 8 * (16>>3) ) + ((8>>3) * 8) + ((8) & 7)
            // offset = (1 * 8 * 2 ) + (1 * 8)
            // offset = 16 + 8 = 24

            char * destBitmap = _dest + offset;
            char * destColormap = _dest + bitmapSize + ( ( ( y >> 3 ) * ( _width >> 3 ) ) + ( x >> 3 ) );
            for( int i=0; i<8; ++i ) {
                *destBitmap = tile[i];
                ++destBitmap;
            }
            // printf("tile at %d,%d color = %2.2x\n", x, y, tile[8] );
            *destColormap = tile[8];            
        }
    }

    adilineendbitmap();

}

/**
 * This method can be used to convert 
 *     4x8 RGB (3/4 bytes) pixel (_source) [8x8x3/4 = 192/256 bytes]
 * into 
 *     4x8 colormap (2 bit) pixel + 1 (bytes) [8x1 + 2 = 10 bytes]
 *       color1 [hi], color2, color3 [hi], background (_dest)
 * 
 * Since the 4x8 pixel area belong to a larger picture,
 * this function will need the picture _width in order
 * to move to the next line to analyze. Moreover, background
 * color should be given since it is not settable and it will
 * be returned as low nibble of second color byte.
 */
static void vic2_image_converter_tile_multicolor( Environment * _environment, char * _source, char * _dest, int _width, int _depth, int _background, int _source_width ) {

    int colorIndexesCount[COLOR_COUNT];
    memset(colorIndexesCount, 0, COLOR_COUNT * sizeof( int ) );
    int trans = 0;

    char * source = _source;

    // Clear the box and colors
    memset( _dest, 0, 10 );

    // Loop for all the box surface
    for (int y=0; y<8; ++y) {
        for (int x=0; x<4; ++x) {

            RGBi rgb;

            memset( &rgb, 0, sizeof( RGBi ) );

            // Take the color of the pixel
            rgb.red = *source;
            rgb.green = *(source + 1);
            rgb.blue = *(source + 2);
            if ( _depth > 3 ) {
                rgb.alpha = *(source + 3);
            } else {
                rgb.alpha = 255;
            }
            if ( rgb.alpha == 0 ) {
                rgb.red = 0;
                rgb.green = 0;
                rgb.blue = 0;
            }

            if ( rgb.alpha < 255 ) {
                trans = 1;
            } else {

                RGBi *systemRgb = vic2_image_nearest_system_color( &rgb );

                ++colorIndexesCount[systemRgb->index];

            }

            source += _depth;

        }

        source += _depth * ( _source_width - 4 );

    }

    if ( trans ) {
        _background = 0;
    }
    
    colorIndexesCount[_background] = 0;

    int colorFirst = 0;
    int colorFirstMax = 0;
    int colorSecond = 0;
    int colorSecondMax = 0;
    int colorThird = 0;
    int colorThirdMax = 0;

    for( int xx = 0; xx<COLOR_COUNT; ++xx ) {
        if ( colorIndexesCount[xx] > colorFirstMax ) {
            colorFirst = xx;
            colorFirstMax = colorIndexesCount[xx];
        };
    }

    colorIndexesCount[colorFirst] = 0;

    for( int xx = 0; xx<COLOR_COUNT; ++xx ) {
        if ( colorIndexesCount[xx] > colorSecondMax ) {
            colorSecond = xx;
            colorSecondMax = colorIndexesCount[xx];
        };
    }

    colorIndexesCount[colorSecond] = 0;

    for( int xx = 0; xx<COLOR_COUNT; ++xx ) {
        if ( colorIndexesCount[xx] > colorThirdMax ) {
            colorThird = xx;
            colorThirdMax = colorIndexesCount[xx];
        };
    }

    colorIndexesCount[colorThird] = 0;

    source = _source;

    for (int y=0; y<8; ++y) {
        for (int x=0; x<4; ++x) {

            RGBi rgb;

            memset( &rgb, 0, sizeof( RGBi ) );

            rgb.red = *source;
            rgb.green = *(source + 1);
            rgb.blue = *(source + 2);
            if ( _depth > 3 ) {
                rgb.alpha = *(source + 3);
            } else {
                rgb.alpha = 255;
            }
            if ( rgb.alpha == 0 ) {
                rgb.red = 0;
                rgb.green = 0;
                rgb.blue = 0;
            }

            char colorIndex = 0;

            if ( rgb.alpha < 255 ) {
                adilinepixel(_background);
                colorIndex = 0;
            } else {

                RGBi *systemRgb = vic2_image_nearest_system_color( &rgb );

                if ( systemRgb->index == colorFirst ) {
                    adilinepixel(colorFirst);
                    colorIndex = 1;
                } else if ( systemRgb->index == colorSecond ) {
                    adilinepixel(colorSecond);
                    colorIndex = 2;
                } else if ( systemRgb->index == colorThird ) {
                    adilinepixel(colorThird);
                    colorIndex = 3;
                } else {
                    adilinepixel(_background);
                }

            }

            char bitmask = colorIndex << (6 - ((x & 0x3) * 2));

            *(_dest + y) |= bitmask;

            source += _depth;

        }

        source += _depth * ( _source_width - 4 );

    }

    *( _dest + 8 ) = ( colorFirst << 4 ) | colorSecond ;
    *( _dest + 9 ) = ( _background << 4 ) | colorThird;

}

/**
 * This method can be used to convert 
 *     WxH RGB (3/4 bytes) pixel (_source) [WxHx3/4 bytes]
 * into 
 *     WxH bitmap (2 bit) pixel + (W/4xH + 2*(W/4xH/8)) (bytes)
 *       color1 [hi], color2, color3 [hi], background (_dest)
 * 
 * Since the WXH pixel area could belong to a larger picture,
 * this function will need the picture _source_width in order
 * to move to the next line to analyze. Moreover, background
 * color is fixed also if it is returned as lower nibble
 * of one byte of 2 of colors.
 */
static void vic2_image_converter_tiles_multicolor( Environment * _environment, char * _source, char * _dest, int _width, int _height, int _depth, int _source_width, int _background ) {

    int bitmapSize = ( _width>>2 ) * _height;
    int colormap1Size = ( _width>>2 ) * (_height>>3);
    int colormap2Size = ( _width>>2 ) * (_height>>3);

    memset( _dest, 0, bitmapSize + colormap1Size + colormap2Size );

    adilinebeginbitmap("BMD4");

    for( int y=0; y<_height; y+=8 ) {
        for( int x=0; x<_width; x+=4 ) {

            char * source = _source + ( ( y * _source_width ) + x ) * _depth;
            char tile[10];

            vic2_image_converter_tile_multicolor( _environment, source, tile, _width, _depth, _background, _source_width );

            int offset = ((y>>3) * 8 *( _width >> 2 ) ) + ((x>>2) * 8) + ((y) & 0x07);

            char * destBitmap = _dest + offset;
            char * destColormap1 = _dest + bitmapSize + ( ( ( y >> 3 ) * ( _width >> 2 ) ) + ( x >> 2 ) );
            char * destColormap2 = _dest + bitmapSize + colormap1Size + ( ( ( y >> 3 ) * ( _width >> 2 ) ) + ( x >> 2 ) );
            for( int i=0; i<8; ++i ) {
                *destBitmap = tile[i];
                ++destBitmap;
            }
            *destColormap1 = tile[8];
            *destColormap2 = tile[9];
        }
    }

    adilineendbitmap();

}

/**
 * @brief <i>VIC-II</i>: emit code to check for collision
 * 
 * This function can be used to issue code aimed at verifying if a sprite has 
 * had a collision with another sprite. The result (0 = no collision, 0xff = 
 * collision occurred) is returned in the output variable.
 * 
 * @param _environment Current calling environment
 * @param _sprite_mask Sprite mask to use
 * @param _result Where to store the result
 */
Variable * vic2_collision( Environment * _environment, char * _sprite ) {

    _environment->bitmaskNeeded = 1;

    deploy( sprite, src_hw_vic2_sprites_asm );

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_SPRITE, 0 );
    Variable * result = variable_temporary( _environment, VT_SBYTE, "(collision result)");

    MAKE_LABEL

    outline1("LDA %s", sprite->realName);
    outline0("STA MATHPTR3");
    outline0("JSR SPRITECOL");
    outline1("STA %s", result->realName);

    return result;

}

/**
 * @brief <i>VIC-II</i>: emit code to check for collision
 * 
 * This function can be used to issue code aimed at verifying if a sprite has 
 * had a collision with a tile. The result (0 = no collision, 0xff = 
 * collision occurred) is returned in the output variable.
 * *
 * @param _environment Current calling environment
 * @param _sprite_mask Sprite mask to use
 * @param _result Where to store the result
 */
void vic2_hit( Environment * _environment, char * _sprite_mask, char * _result ) {

    // Generate unique label for ASM code.
    MAKE_LABEL

    // Check for collision using VIC-II registers
    outline0("LDA $D01F");
    outline1("AND %s", _sprite_mask);
    cpu6502_beq(_environment, label);

    outline0("LDA #$1");
    outline1("STA %s", _result);
    outline1("JMP %s_2", label);

    outhead1("%s:", label);
    outline0("LDA #0");
    outline1("STA %s", _result);
    outhead1("%s_2:", label);

}

/**
 * @brief <i>VIC-II</i>: emit code to change border color
 * 
 * This function can be used to issue code aimed at changing the
 * border color of the screen.
 * 
 * @param _environment Current calling environment
 * @param _border_color Border color to use
 */
void vic2_border_color( Environment * _environment, char * _border_color ) {

    outline1("LDA %s", _border_color );
    outline0("AND #$0f" );
    outline0("STA $D020");

}

/**
 * @brief <i>VIC-II</i>: emit code to change background color
 * 
 * This function can be used to issue code aimed at changing the
 * background color of the screen.
 * 
 * @param _environment Current calling environment
 * @param _index Index of the background color
 * @param _background_color Background color to use
 */
void vic2_background_color( Environment * _environment, int _index, int _background_color ) {
 
    outline1("LDA #$%2.2x", _background_color );
    outline0("AND #$0f" );
    outline1("STA $d021+%d", ( _index & 0x03 ) );
}

/**
 * @brief <i>VIC-II</i>: emit code to change background color
 * 
 * This function can be used to issue code aimed at changing the
 * background color of the screen.
 * 
 * @param _environment Current calling environment
 * @param _index Index of the background color
 * @param _background_color Background color to use
 */
void vic2_background_color_vars( Environment * _environment, char * _index, char * _background_color ) {
 
    outline1("LDA %s", _index);
    outline0("AND #$03");
    outline0("TAX");
    outline1("LDA %s", _background_color );
    outline0("AND #$0f" );
    outline0("STA $d021,X");
}

/**
 * @brief <i>VIC-II</i>: emit code to change background color
 * 
 * This function can be used to issue code aimed at changing the
 * background color of the screen.
 * 
 * @param _environment Current calling environment
 * @param _index Index of the background color
 * @param _background_color Background color to use
 */
void vic2_background_color_semivars( Environment * _environment, int _index, char * _background_color ) {
 
    outline1("LDA #$%2.2x", _index);
    outline0("AND #$03");
    outline0("TAX");
    outline1("LDA %s", _background_color );
    outline0("AND #$0f" );
    outline0("STA $d021,X");
}

/**
 * @brief <i>VIC-II</i>: emit code to retrieve background color
 * 
 * This function can be used to issue code aimed at retrieving the
 * background color of the screen.
 * 
 * @param _environment Current calling environment
 * @param _index Index of the background color
 * @param _background_color Background color to use
 */
void vic2_background_color_get_vars( Environment * _environment, char * _index, char * _background_color ) {
 
    outline1("LDA %s", _index);
    outline0("AND #$03");
    outline0("TAX");
    outline0("LDA $d021,X");
    outline0("AND #$0f" );
    outline1("STA %s", _background_color );
}

/**
 * @brief <i>VIC-II</i>: emit code to change common sprite's color 
 * 
 * This function can be used to issue code aimed at changing the
 * common color of the sprites.

 * @param _environment Current calling environment
 * @param _index Index of the background color
 * @param _common_color Index of the color to use
 */
void vic2_sprite_common_color( Environment * _environment, char * _index, char * _common_color ) {
 
    outline1("LDA %s", _index);
    outline0("AND #$03");
    outline0("TAX");
    outline1("LDA %s", _common_color );
    outline0("AND #$0f" );
    outline0("STA $D025,X" );

}

/**
 * @brief <i>VIC-II</i>: emit code to set raster irq
 * 
 * This function outputs assembly code needed to initialize a raster routine. 
 * In other words, asynchronously and in parallel with the execution of the 
 * main program, the routine starting from the label provided will be executed
 * when the vertical brush on the screen reaches the value of _position. 
 * This function is particularly useful when the position is communicated 
 * is given as an integer.
 * 
 * @param _environment Current calling environment
 * @param _label Label to jump to when vertical raster reach the value given
 * @param _positionlo The vertical position to wait for (bits 7..0)
 * @param _positionhi The vertical position to wait for (bit 8)
 */
void vic2_raster_at( Environment * _environment, char * _label, char * _positionlo, char * _positionhi ) {

    MAKE_LABEL

    outline0("LDA #%01111111"); // switch off CIA-1
    outline0("STA $DC0D");
    outline0("AND $D011");
    outline0("STA $D011");
    outline0("LDA $DC0D"); // acknowledge CIA-1
    outline0("LDA $DD0D"); // acknowledge CIA-2
    outline1("LDA #<%s", _label);
    outline0("STA $0314");
    outline1("LDA #>%s", _label);
    outline0("STA $0315");
    outline0("LDA #%00000001"); // enable raster interrupt signals from VIC
    outline0("STA $D01A");
    outline1("LDA %s", _positionlo );
    outline0("STA $D012");
    outline1("LDA %s", _positionhi );
    outline0("AND #%00000001" );
    cpu6502_beq(_environment, label);
    outline0("LDA $D011" );
    outline0("AND #%01111111" );
    outline0("ORA #%10000000" );
    outline0("STA $D011");
    outline1("JMP %s_2", label );
    outhead1("%s:", label );
    outline0("LDA $D011" );
    outline0("AND #%01111111" );
    outline0("STA $D011");
    outhead1("%s_2:", label );
    outline0("CLI");

}

/**
 * @brief <i>VIC-II</i>: emit code to wait for next raster irq
 * 
 * This function outputs assembly code needed to wait for the
 * next raster. Meanwhile, the execution of the main code will resume 
 * where it left off.
 * 
 * @param _environment Current calling environment
 *
 */
void vic2_next_raster( Environment * _environment ) {

    outline0("ASL $D019");
    outline0("JMP IRQSVC2");

}

/**
 * @brief <i>VIC-II</i>: emit code to wait for next raster irq at different position
 * 
 * This function outputs assembly code needed to wait for the
 * next raster on a different position with a different code to
 * execute. Meanwhile, the execution of the main code will resume 
 * where it left off.
 * 
 * @param _environment Current calling environment
 * @param _label Label to jump to when vertical raster reach the value given
 * @param _positionlo The vertical position to wait for (bits 7..0)
 * @param _positionhi The vertical position to wait for (bit 8)
 */
void vic2_next_raster_at( Environment * _environment, char * _label, char * _positionlo, char * _positionhi ) {

    MAKE_LABEL

    outline1("LDA %s", _positionlo);
    outline0("STA $D012");
    outline1("LDA %s", _positionhi );
    outline0("AND #%00000001" );
    cpu6502_beq(_environment, label);
    outline0("LDA $D011" );
    outline0("AND #%01111111" );
    outline0("ORA #%10000000" );
    outline0("STA $D011");
    outline1("JMP %s_2", label );
    outhead1("%s:", label );
    outline0("LDA $D011" );
    outline0("AND #%01111111" );
    outline0("STA $D011");
    outhead1("%s_2:", label );
    outline1("LDA #<%s", _label);
    outline0("STA $0314");
    outline1("LDA #>%s", _label);
    outline0("STA $0315");

    vic2_next_raster( _environment );

}

/**
 * @brief <i>VIC-II</i>: emit code to enable ECM
 * 
 * @param _environment Current calling environment
 */
void vic2_enable_ecm( Environment * _environment ) {

    outline0("LDA $D011" );
    outline0("ORA #%01000000");
    outline0("STA $D011" );
}

/**
 * @brief <i>VIC-II</i>: emit code to disable ECM
 * 
 * @param _environment Current calling environment
 */
void vic2_disable_ecm( Environment * _environment ) {

    outline0("LDA $D011" );
    outline0("AND #%10111111");
    outline0("STA $D011" );
}

/**
 * @brief <i>VIC-II</i>: emit code to enable MCM
 * 
 * @param _environment Current calling environment
 */
void vic2_enable_mcm( Environment * _environment ) {

    outline0("LDA $D016" );
    outline0("ORA #%00001000");
    outline0("STA $D016" );
}

/**
 * @brief <i>VIC-II</i>: emit code to disable ECM
 * 
 * @param _environment Current calling environment
 */
void vic2_disable_mcm( Environment * _environment ) {

    outline0("LDA $D016" );
    outline0("AND #%11110111");
    outline0("STA $D016" );
}

void vic2_bank_select( Environment * _environment, int _bank ) {
    
    outline0("LDA $DD00" );
    outline0("AND #%11111100");
    outline1("ORA #%2.2x", ( ~_bank ) & 0x03 );
    outline0("STA $DD00" );
}

static int rgbConverterFunction( int _red, int _green, int _blue ) {
    
    int colorIndex = 0;
    unsigned int minDistance = 0xffffffff;
    int j;

    RGBi rgb;
    rgb.red = _red;
    rgb.green = _green;
    rgb.blue = _blue;

    for (j = 0; j < sizeof(SYSTEM_PALETTE)/sizeof(RGBi); ++j) {
        int distance = rgbi_distance(&SYSTEM_PALETTE[j], &rgb);
        if (distance < minDistance) {
            minDistance = distance;
            colorIndex = j;
        }
    }

    return colorIndex;

}

int vic2_screen_mode_enable( Environment * _environment, ScreenMode * _screen_mode ) {

    Variable * colormapAddress = variable_retrieve( _environment, "COLORMAPADDRESS" );

    _environment->fontWidth = 8;
    _environment->fontHeight = 8;
    _environment->screenTiles = 255;
    switch( _screen_mode->id ) {
        case BITMAP_MODE_STANDARD:
            _environment->screenWidth = 320;
            _environment->screenHeight = 200;
            _environment->screenColors = 2;
            _environment->currentModeBW = 1;
            // This fix is necessary to set the starting address of the bitmap 
            // to $A000 (which is an address available on C=64).
            outline0("LDA $D018" );
            outline0("AND #%11110111");
            outline0("ORA #%00001000" );
            outline0("STA $D018" );

            // Let's enable monocolor graphics!
            outline0("LDA $D011" );
            outline0("AND #%11111000");
            outline0("ORA #%00100011");
            outline0("STA $D011" );
            outline0("LDA $D016" );
            outline0("AND #%11101111");
            outline0("STA $D016" );

            cpu_store_16bit( _environment, colormapAddress->realName, 0x8400 );

            cpu_store_8bit( _environment, "_PEN", 0X01 );
            cpu_store_8bit( _environment, "_PAPER", 0x00 );
            cpu_store_16bit( _environment, "CLIPX1", 0 );
            cpu_store_16bit( _environment, "CLIPX2", 319 );
            cpu_store_16bit( _environment, "CLIPY1", 0 );
            cpu_store_16bit( _environment, "CLIPY2", 199 );

            break;
        case BITMAP_MODE_MULTICOLOR:
            _environment->fontWidth = 4;
            _environment->screenWidth = 160;
            _environment->screenHeight = 200;
            _environment->screenColors = 4;
            _environment->currentModeBW = 2;
            // This fix is necessary to set the starting address of the bitmap 
            // to $A000 (which is an address available on C=64) instead of the 
            // address $8000.
            outline0("LDA $D018" );
            outline0("AND #%11110111");
            outline0("ORA #%00001000" );
            outline0("STA $D018" );

            // Let's enable multicolor graphics!
            outline0("LDA $D011" );
            outline0("AND #%11111000");
            outline0("ORA #%00100011");
            outline0("STA $D011" );
            outline0("LDA $D016" );
            outline0("ORA #%00010000");
            outline0("STA $D016" );

            cpu_store_16bit( _environment, colormapAddress->realName, 0x8400 );

            cpu_store_8bit( _environment, "_PEN", 0X01 );
            cpu_store_8bit( _environment, "_PAPER", 0x00 );

            cpu_store_16bit( _environment, "CLIPX1", 0 );
            cpu_store_16bit( _environment, "CLIPX2", 159 );
            cpu_store_16bit( _environment, "CLIPY1", 0 );
            cpu_store_16bit( _environment, "CLIPY2", 199 );
            
            break;
        case TILEMAP_MODE_STANDARD:
            _environment->screenWidth = 320;
            _environment->screenHeight = 200;
            _environment->screenColors = 16;
            _environment->currentModeBW = 0;
            // Let's disable graphics!
            outline0("LDA $D011" );
            outline0("AND #%11011000");
            outline0("ORA #%00000011");
            outline0("STA $D011" );
            outline0("LDA $D016" );
            outline0("AND #%11101111");
            outline0("STA $D016" );

            // // This fix is necessary to reset the lookup for rom character.
            // outline0("LDA $D018" );
            // outline0("AND #%11110000");
            // if ( _environment->fontConfig.schema || _environment->descriptors ) {
            //     outline0("ORA #$09");
            // } else {
            //     outline0("ORA #$07");
            // }
            // outline0("STA $D018" );

            cpu_store_16bit( _environment, colormapAddress->realName, 0xd800 );

            cpu_store_8bit( _environment, "_PEN", 0x01 );
            cpu_store_8bit( _environment, "_PAPER", 0x00 );

            cpu_store_16bit( _environment, "CLIPX1", 0 );
            cpu_store_16bit( _environment, "CLIPX2", 39 );
            cpu_store_16bit( _environment, "CLIPY1", 0 );
            cpu_store_16bit( _environment, "CLIPY2", 24 );

            break;
        case TILEMAP_MODE_MULTICOLOR:
            _environment->screenWidth = 160;
            _environment->screenHeight = 200;
            _environment->screenColors = 16;
            _environment->currentModeBW = 0;
            // Let's disable graphics!
            outline0("LDA $D011" );
            outline0("AND #%11011000");
            outline0("ORA #%00000011");
            outline0("STA $D011" );
            outline0("LDA $D016" );
            outline0("ORA #%00010000");
            outline0("STA $D016" );

            // // This fix is necessary to reset the lookup for rom character.
            // outline0("LDA $D018" );
            // outline0("AND #%11110000");
            // if ( _environment->fontConfig.schema || _environment->descriptors ) {
            //     outline0("ORA #$09");
            // } else {
            //     outline0("ORA #$07");
            // }
            // outline0("STA $D018" );

            cpu_store_16bit( _environment, colormapAddress->realName, 0xd800 );

            cpu_store_8bit( _environment, "_PEN", 0x01 );
            cpu_store_8bit( _environment, "_PAPER", 0x00 );
            cpu_store_16bit( _environment, "CLIPX1", 0 );
            cpu_store_16bit( _environment, "CLIPX2", 39 );
            cpu_store_16bit( _environment, "CLIPY1", 0 );
            cpu_store_16bit( _environment, "CLIPY2", 24 );

            break;
        case TILEMAP_MODE_EXTENDED:
            _environment->screenWidth = 160;
            _environment->screenHeight = 200;
            _environment->screenColors = 16;
            _environment->currentModeBW = 0;
            // Let's disable graphics!
            outline0("LDA $D011" );
            outline0("AND #%11011111");
            outline0("ORA #%01000011");
            outline0("STA $D011" );
            outline0("LDA $D016" );
            outline0("AND #%11101111");
            outline0("STA $D016" );

            // // This fix is necessary to reset the lookup for rom character.
            // outline0("LDA $D018" );
            // outline0("AND #%11110000");
            // if ( _environment->fontConfig.schema || _environment->descriptors ) {
            //     outline0("ORA #$09");
            // } else {
            //     outline0("ORA #$07");
            // }
            // outline0("STA $D018" );
            
            cpu_store_16bit( _environment, colormapAddress->realName, 0xd800 );

            cpu_store_8bit( _environment, "_PEN", 0x01 );
            cpu_store_8bit( _environment, "_PAPER", 0x00 );
            cpu_store_16bit( _environment, "CLIPX1", 0 );
            cpu_store_16bit( _environment, "CLIPX2", 39 );
            cpu_store_16bit( _environment, "CLIPY1", 0 );
            cpu_store_16bit( _environment, "CLIPY2", 24 );

            break;
        default:
            CRITICAL_SCREEN_UNSUPPORTED( _screen_mode->id );
    }

    cpu_store_16bit( _environment, "ORIGINX", 0 );
    cpu_store_16bit( _environment, "ORIGINY", 0 );

    cpu_store_16bit( _environment, "CURRENTWIDTH", _environment->screenWidth );
    cpu_store_16bit( _environment, "CURRENTHEIGHT", _environment->screenHeight );
    cpu_move_16bit( _environment, "CURRENTWIDTH", "RESOLUTIONX" );
    cpu_move_16bit( _environment, "CURRENTHEIGHT", "RESOLUTIONY" );
    cpu_store_8bit( _environment, "CURRENTTILES", _environment->screenTiles );
    _environment->screenTilesWidth = 40;
    cpu_store_8bit( _environment, "CURRENTTILESWIDTH", _environment->screenTilesWidth );
    _environment->screenTilesHeight = 25;
    cpu_store_8bit( _environment, "CURRENTTILESHEIGHT", _environment->screenTilesHeight );
    cpu_store_8bit( _environment, "FONTWIDTH", _environment->fontWidth );
    cpu_store_8bit( _environment, "FONTHEIGHT", _environment->fontHeight );

}

void vic2_bitmap_enable( Environment * _environment, int _width, int _height, int _colors ) {

    ScreenMode * mode = find_screen_mode_by_suggestion( _environment, 1, _width, _height, _colors, 8, 8 );

    if ( mode ) {
        vic2_screen_mode_enable( _environment, mode );

        cpu_store_8bit( _environment, "CURRENTMODE", mode->id );
        cpu_store_8bit( _environment, "CURRENTTILEMODE", 0 );

        _environment->currentMode = mode->id;
        _environment->currentTileMode = 0;
    } else {
        WARNING_SCREEN_MODE( -1 );
    }
}

void vic2_bitmap_disable( Environment * _environment ) {

    Variable * colormapAddress = variable_retrieve( _environment, "COLORMAPADDRESS" );

    // Let's disable graphics!
    outline0("LDA $D011" );
    outline0("AND #%11011111");
    outline0("STA $D011" );
    outline0("LDA $D016" );
    outline0("AND #%11101111");
    outline0("STA $D016" );

    cpu_store_16bit( _environment, colormapAddress->realName, 0xd800 );

}

void vic2_tilemap_enable( Environment * _environment, int _width, int _height, int _colors, int _tile_width, int _tile_height ) {

    ScreenMode * mode = find_screen_mode_by_suggestion( _environment, 0, _width, _height, _colors, _tile_width, _tile_height );

    if ( mode ) {

        vic2_screen_mode_enable( _environment, mode );

        _environment->currentMode = mode->id;
        _environment->currentTileMode = 1;

        cpu_store_8bit( _environment, "CURRENTMODE", mode->id );
        cpu_store_8bit( _environment, "CURRENTTILEMODE", 1 );

    } else {
        WARNING_SCREEN_MODE( -1 );
    }

}

void vic2_bitmap_at( Environment * _environment, char * _address ) {

    outline1("LDA %s", _address );
    outline0("LSR");
    outline0("LSR");
    outline0("LSR");
    outline0("LSR");
    outline0("LSR");
    outline0("LSR");
    outline0("AND #%00000001");
    outline0("ASL" );
    outline0("ASL" );
    outline0("ASL" );
    outline0("STA TMPPTR" );
    outline0("LDA $D018" );
    outline0("AND #%11110111");
    outline0("ORA TMPPTR" );
    outline0("STA $D018" );

}

void vic2_colormap_at( Environment * _environment, char * _address ) {

    // Create a unique variabled for ASM code.
    MAKE_LABEL

    // Let's check if we are in ECM mode.
    outline0("LDA $D011");
    outline0("BIT #%00100000");
    cpu6502_beq(_environment, label);

    // Change the colormap address that is the text address.
    vic2_textmap_at( _environment, _address );

    // If not, we are unable to change the colormap address.
    outline1("%s:", label);


}

void vic2_textmap_at( Environment * _environment, char * _address ) {

    // Create a unique variabled for ASM code.
    MAKE_LABEL

    outline1("LDA %s", _address );
    outline0("LSR");
    outline0("LSR");
    outline0("AND #$0f");
    outline0("ASL");
    outline0("ASL");
    outline0("ASL");
    outline0("ASL");
    outline0("STA TMPPTR");
    outline0("LDA $D018");
    outline0("AND #%00001111");
    outline0("ORA TMPPTR" );
    outline0("STA $D018");

}

void vic2_pset_int( Environment * _environment, int _x, int _y ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( plot, src_hw_vic2_plot_asm );
    
    outline1("LDA %2.2x", (_x & 0xff ) );
    outline0("STA PLOTX");
    outline1("LDA %2.2x", ( ( _x >> 8 ) & 0xff ) );
    outline0("STA PLOTX+1");
    outline1("LDA %2.2x", ( _y & 0xff ) );
    outline0("STA PLOTY");
    outline0("LDA #1");
    outline0("STA PLOTM");
    outline0("JSR PLOT");

}

void vic2_pset_vars( Environment * _environment, char *_x, char *_y ) {

    Variable * x = variable_retrieve( _environment, _x );
    Variable * y = variable_retrieve( _environment, _y );

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( plot, src_hw_vic2_plot_asm );
    
    outline1("LDA %s", x->realName );
    outline0("STA PLOTX");
    if ( VT_BITWIDTH( x->type ) > 8 ) {
        outline1("LDA %s", address_displacement(_environment, x->realName, "1") );
    } else {
        outline0("LDA #0");
    }
    outline0("STA PLOTX+1");
    outline1("LDA %s", y->realName );
    outline0("STA PLOTY");
    outline0("LDA #1");
    outline0("STA PLOTM");
    outline0("JSR PLOT");

}

void vic2_pget_color_vars( Environment * _environment, char *_x, char *_y, char * _result ) {

    Variable * x = variable_retrieve( _environment, _x );
    Variable * y = variable_retrieve( _environment, _y );
    Variable * result = variable_retrieve( _environment, _result );

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( plot, src_hw_vic2_plot_asm );
    
    outline1("LDA %s", x->realName );
    outline0("STA PLOTX");
    outline1("LDA %s", address_displacement(_environment, x->realName, "1") );
    outline0("STA PLOTX+1");
    outline1("LDA %s", y->realName );
    outline0("STA PLOTY");
    outline0("LDA #3");
    outline0("STA PLOTM");
    outline0("JSR PLOT");
    outline0("LDA PLOTM");
    outline1("STA %s", result->realName);

}

void vic2_screen_on( Environment * _environment ) {

    outline0("LDA $D011" );
    outline0("ORA #%00010000");
    outline0("STA $D011" );

}

void vic2_screen_off( Environment * _environment ) {
    
    outline0("LDA $D011" );
    outline0("AND #%11101111");
    outline0("STA $D011" );

}

void vic2_screen_rows( Environment * _environment, char * _rows ) {

    MAKE_LABEL

    outline1("LDA %s", _rows);
    outline0("CMP #24");
    outline1("BEQ %s", label);
    outline0("LDA $D011" );
    outline0("ORA #%00001000");
    outline0("STA $D011" );
    outline1("JMP %s_2", label);
    outhead1("%s:", label );
    outline0("LDA $D011" );
    outline0("AND #%11110111");
    outline0("STA $D011" );
    outline1("JMP %s_2", label);
    outhead1("%s_2:", label );

}

void vic2_screen_columns( Environment * _environment, char * _columns ) {

    MAKE_LABEL

    outline1("LDA %s", _columns);
    outline0("CMP #38");
    outline1("BEQ %s", label);
    outline0("LDA $D016" );
    outline0("ORA #%00001000");
    outline0("STA $D016" );
    outline1("JMP %s_2", label);
    outhead1("%s:", label );
    outline0("LDA $D016" );
    outline0("AND #%11110111");
    outline0("STA $D016" );
    outline1("JMP %s_2", label);
    outhead1("%s_2:", label );

}

void vic2_sprite_data_from( Environment * _environment, char * _sprite, char * _image ) {

    _environment->bitmaskNeeded = 1;

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );
    Variable * image = variable_retrieve_or_define( _environment, _image, VT_IMAGE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );

    outline1("LDA #<%s", image->realName );
    outline0("STA MATHPTR1"  );
    outline1("LDA #>%s", image->realName );
    outline0("STA MATHPTR2"  );
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEDATAFROM" );

}

void vic2_sprite_enable( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );

    _environment->bitmaskNeeded = 1;

    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEENABLE" );

}

void vic2_sprite_disable( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    _environment->bitmaskNeeded = 1;

    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEDISABLE" );

}

void vic2_sprite_at( Environment * _environment, char * _sprite, char * _x, char * _y ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );
    Variable * x = variable_retrieve_or_define( _environment, _x, VT_POSITION, 0 );
    Variable * y = variable_retrieve_or_define( _environment, _y, VT_POSITION, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    _environment->bitmaskNeeded = 1;

    outline1("LDA %s", x->realName );
    outline0("STA MATHPTR0"  );
    outline1("LDA %s", address_displacement(_environment, x->realName, "1") );
    outline0("STA MATHPTR1"  );
    outline1("LDX %s", y->realName );
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEAT" );

}

void vic2_sprite_expand_vertical( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEEXPANDY" );

}

void vic2_sprite_expand_horizontal( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEEXPANDX" );

}

void vic2_sprite_compress_vertical( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITECOMPRESSY" );

}

void vic2_sprite_compress_horizontal( Environment * _environment, char * _sprite ) {

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITECOMPRESSX" );

}

void vic2_sprite_multicolor( Environment * _environment, char * _sprite ) {

    _environment->bitmaskNeeded = 1;

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEMULTICOLOR" );

}

void vic2_sprite_monocolor( Environment * _environment, char * _sprite ) {

    _environment->bitmaskNeeded = 1;

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITEMONOCOLOR" );

}

void vic2_sprite_color( Environment * _environment, char * _sprite, char * _color ) {

    _environment->bitmaskNeeded = 1;

    Variable * sprite = variable_retrieve_or_define( _environment, _sprite, VT_BYTE, 0 );
    Variable * color = variable_retrieve_or_define( _environment, _color, VT_COLOR, COLOR_WHITE );

    deploy( sprite, src_hw_vic2_sprites_asm );
    
    outline1("LDA %s", color->realName );
    outline1("LDY %s", sprite->realName );
    outline0("JSR SPRITECOLOR" );

}

void vic2_tiles_at( Environment * _environment, char * _address ) {

    outline1("LDA %s", _address);
    outline0("LSR");
    outline0("LSR");
    outline0("LSR");
    outline0("AND #$07");
    outline0("ASL");
    outline0("STA TMPPTR");
    outline0("LDA $D018");
    outline0("AND #%00001111");
    outline0("ORA TMPPTR");
    outline0("STA $D018");

}

void vic2_vertical_scroll( Environment * _environment, char * _displacement ) {

    outline0("LDA $D011" );
    outline0("AND #%11111000");
    outline1("ORA %s", _displacement );
    outline0("STA $D011" );

}

void vic2_horizontal_scroll( Environment * _environment, char * _displacement ) {

    outline0("LDA $D016" );
    outline0("AND #%11111000");
    outline1("ORA %s", _displacement );
    outline0("STA $D016" );

}

void vic2_busy_wait( Environment * _environment, char * _timing ) {

    MAKE_LABEL

    outline1("LDA %s", _timing );
    outline0("STA TMPPTR");
    outline1("LDA %s", address_displacement(_environment, _timing, "1") );
    outline0("STA TMPPTR+1");
    outhead1("%sfirst:", label );
    outline0("LDA $01");
    outhead1("%ssecond:", label );
    outline0("CMP $D012");
    outline1("BNE %ssecond", label);
    outhead1("%sthird:", label );
    outline0("CMP $D012");
    outline1("BEQ %sthird", label);
    outline0("DEC TMPPTR");
    outline0("LDA TMPPTR");
    outline0("CMP #$FF");
    outline1("BNE %sfirst", label);
    outline0("DEC TMPPTR+1");
    outline0("LDA TMPPTR+1");
    outline0("CMP #$FF");
    outline1("BNE %sfirst", label);
    
}

void vic2_get_width( Environment * _environment, char *_result ) {

    outline0("LDA CURRENTWIDTH" );
    outline1("STA %s", _result );
    outline0("LDA CURRENTWIDTH+1" );
    outline1("STA %s", address_displacement(_environment, _result, "1") );

}

void vic2_tiles_get( Environment * _environment, char *_result ) {

    outline0("LDA CURRENTTILES" );
    outline1("STA %s", _result );

}

void vic2_tiles_get_width( Environment * _environment, char *_result ) {

    outline0("LDA CURRENTTILESWIDTH" );
    outline1("STA %s", _result );

}

void vic2_get_height( Environment * _environment, char *_result ) {

    outline0("LDA CURRENTHEIGHT" );
    outline1("STA %s", _result );
    outline0("LDA CURRENTHEIGHT+1" );
    outline1("STA %s", address_displacement(_environment, _result, "1") );

}

void vic2_tiles_get_height( Environment * _environment, char *_result ) {

    outline0("LDA CURRENTTILESHEIGHT" );
    outline1("STA %s", _result );

}

void vic2_cls( Environment * _environment ) {
    
    if ( _environment->currentMode == 2 || _environment->currentMode == 3 ) {
        deploy( clsGraphic, src_hw_vic2_cls_graphic_asm );
        outline0("JSR CLSG");
    } else {
        deploy( clsText, src_hw_vic2_cls_text_asm );
        outline0("JSR CLST");
    }

}

void vic2_scroll_text( Environment * _environment, int _direction ) {

    if ( _direction > 0 ) {
        deploy( vScrollTextDown, src_hw_vic2_vscroll_text_down_asm );
        outline0("JSR VSCROLLTDOWN");
    } else {
        deploy( vScrollTextUp, src_hw_vic2_vscroll_text_up_asm );
        outline0("JSR VSCROLLTUP");
    }

}

void vic2_text( Environment * _environment, char * _text, char * _text_size ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vScrollTextUp, src_hw_vic2_vscroll_text_up_asm );
    deploy( textEncodedAt, src_hw_vic2_text_at_asm );

    outline1("LDA %s", _text);
    outline0("STA TEXTPTR" );
    outline1("LDA %s", address_displacement(_environment, _text, "1"));
    outline0("STA TEXTPTR+1" );
    outline1("LDA %s", _text_size);
    outline0("STA TEXTSIZE" );

    if ( _environment->currentMode == 2 || _environment->currentMode == 3 ) {
        deploy( clsGraphic, src_hw_vic2_cls_graphic_asm );
        deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
        deploy( textEncodedAtGraphic, src_hw_vic2_text_at_graphic_asm );
        outline0("JSR TEXTATBITMAPMODE");
    } else {
        deploy( clsText, src_hw_vic2_cls_text_asm );
        deploy( textEncodedAtText, src_hw_vic2_text_at_text_asm );
        outline0("JSR TEXTATTILEMODE");
    }

}

/* <usermanual>
@keyword RGB

@target c64
@target c128
</usermanual> */

void vic2_initialization( Environment * _environment ) {

    deploy( vic2vars, src_hw_vic2_vars_asm );
    deploy_deferred( vic2startup, src_hw_vic2_startup_asm );

    variable_import( _environment, "CURRENTMODE", VT_BYTE, 0 );
    variable_global( _environment, "CURRENTMODE" );
    variable_import( _environment, "CURRENTTILEMODE", VT_BYTE, 1 );
    variable_global( _environment, "CURRENTTILEMODE" );

    variable_import( _environment, "CURRENTWIDTH", VT_POSITION, 320 );
    variable_global( _environment, "CURRENTWIDTH" );
    variable_import( _environment, "CURRENTHEIGHT", VT_POSITION, 200  );
    variable_global( _environment, "CURRENTHEIGHT" );
    variable_import( _environment, "CURRENTTILES", VT_BYTE, 255 );
    variable_global( _environment, "CURRENTTILES" );
    variable_import( _environment, "CURRENTTILESWIDTH", VT_BYTE, 40 );
    variable_global( _environment, "CURRENTTILESWIDTH" );
    variable_import( _environment, "CURRENTTILESHEIGHT", VT_BYTE, 25 );
    variable_global( _environment, "CURRENTTILESHEIGHT" );
    variable_import( _environment, "FONTWIDTH", VT_BYTE, 8 );
    variable_global( _environment, "FONTWIDTH" );
    variable_import( _environment, "FONTHEIGHT", VT_BYTE, 8 );
    variable_global( _environment, "FONTHEIGHT" );

    SCREEN_MODE_DEFINE( BITMAP_MODE_STANDARD, 1, 320, 200, 2, 8, 8, "Standard Bitmap Mode" );
    SCREEN_MODE_DEFINE( BITMAP_MODE_MULTICOLOR, 1, 160, 200, 4, 8, 8, "Multicolor Bitmap Mode"  );
    SCREEN_MODE_DEFINE( TILEMAP_MODE_STANDARD, 0, 40, 25, 2, 8, 8, "Standard Character Mode" );
    SCREEN_MODE_DEFINE( TILEMAP_MODE_MULTICOLOR, 0, 40, 25, 16, 8, 8, "Multicolor Character Mode" );
    SCREEN_MODE_DEFINE( TILEMAP_MODE_EXTENDED, 0, 40, 25, 20, 8, 8, "Extended Multicolor Character Mode" );

    outline0("JSR VIC2STARTUP");

    variable_import( _environment, "XGR", VT_POSITION, 0 );
    variable_global( _environment, "XGR" );
    variable_import( _environment, "YGR", VT_POSITION, 0 );
    variable_global( _environment, "YGR" );
    variable_import( _environment, "LINE", VT_WORD, (unsigned short) (0xffff) );
    variable_global( _environment, "LINE" );
    variable_import( _environment, "TABCOUNT", VT_BYTE, 4 );
    variable_global( _environment, "TABCOUNT" );

    variable_import( _environment, "CLIPX1", VT_POSITION, 0 );
    variable_global( _environment, "CLIPX1" );
    variable_import( _environment, "CLIPX2", VT_POSITION, 319 );
    variable_global( _environment, "CLIPX2" );
    variable_import( _environment, "CLIPY1", VT_POSITION, 0 );
    variable_global( _environment, "CLIPY1" );
    variable_import( _environment, "CLIPY2", VT_POSITION, 199 );
    variable_global( _environment, "CLIPY2" );

    variable_import( _environment, "ORIGINX", VT_POSITION, 0 );
    variable_global( _environment, "ORIGINX" );
    variable_import( _environment, "ORIGINY", VT_POSITION, 0 );
    variable_global( _environment, "ORIGINY" );

    variable_import( _environment, "RESOLUTIONX", VT_POSITION, 0 );
    variable_global( _environment, "RESOLUTIONX" );
    variable_import( _environment, "RESOLUTIONY", VT_POSITION, 0 );
    variable_global( _environment, "RESOLUTIONY" );

    variable_import( _environment, "XSCROLLPOS", VT_BYTE, 4 );
    variable_global( _environment, "XSCROLLPOS" );
    variable_import( _environment, "YSCROLLPOS", VT_BYTE, 4 );
    variable_global( _environment, "YSCROLLPOS" );
    variable_import( _environment, "XSCROLL", VT_BYTE, 0 );
    variable_global( _environment, "XSCROLL" );
    variable_import( _environment, "YSCROLL", VT_BYTE, 0 );
    variable_global( _environment, "YSCROLL" );

    variable_import( _environment, "SPRITECOUNT", VT_SPRITE, 0 );
    variable_global( _environment, "SPRITECOUNT" );

    _environment->fontWidth = 8;
    _environment->fontHeight = 8;
    _environment->screenTilesWidth = 40;
    _environment->screenTilesHeight = 25;
    _environment->screenWidth = _environment->screenTilesWidth * 8;
    _environment->screenHeight = _environment->screenTilesHeight * 8;

    font_descriptors_init( _environment, 1 );

    _environment->currentRgbConverterFunction = rgbConverterFunction;
    _environment->screenShades = 16;
    _environment->screenColors = 16;

    outline0("JSR VIC2FINALIZATION");

}

static RGBi * multicolorSpritePalette[2];

void vic2_finalization( Environment * _environment ) {

    outhead0("VIC2FINALIZATION:");

    if ( multicolorSpritePalette[0] ) {
        outline1("LDA #$%2.2x", multicolorSpritePalette[0]->index );
        outline0("STA $D025" );
    }

    if ( multicolorSpritePalette[1] ) {
        outline1("LDA #$%2.2x", multicolorSpritePalette[1]->index );
        outline0("STA $D026" );
    }

    // This fix is necessary to reset the lookup for rom character.
    outline0("LDA $D018" );
    outline0("AND #%11110000");
    if ( _environment->fontConfig.schema || _environment->descriptors ) {
        outline0("ORA #$09");
    } else {
        outline0("ORA #$07");
    }
    outline0("STA $D018" );

    outline0("RTS");

}

void vic2_hscroll_line( Environment * _environment, int _direction ) {

    deploy( textHScroll, src_hw_vic2_hscroll_text_asm );

    Variable * y = variable_retrieve( _environment, "YCURSYS" );
    outline1("LDA #$%2.2x", ( _direction & 0xff ) );
    outline0("STA DIRECTION" );
    outline1("LDA %s", y->realName );
    outline0("STA CLINEY");

    outline0("JSR HSCROLLLT");

}

void vic2_hscroll_screen( Environment * _environment, int _direction ) {

    deploy( textHScroll, src_hw_vic2_hscroll_text_asm );

    outline1("LDA #$%2.2x", ( _direction & 0xff ) );
    outline0("STA DIRECTION" );

    outline0("JSR HSCROLLST");
}

void vic2_back( Environment * _environment ) {

    deploy( back, src_hw_vic2_back_asm );

    outline0("JSR BACK");

}

void vic2_cline( Environment * _environment, char * _characters ) {

    deploy( textCline, src_hw_vic2_cline_asm );
    Variable * x = variable_retrieve( _environment, "XCURSYS" );
    Variable * y = variable_retrieve( _environment, "YCURSYS" );

    if ( _characters ) {
        outline1("LDA %s", _characters);
    } else {
        outline0("LDA #0");
    }
    outline0("STA CHARACTERS");
    outline1("LDA %s", x->realName );
    outline0("STA CLINEX" );
    outline1("LDA %s", y->realName );
    outline0("STA CLINEY");
    outline0("JSR CLINE");

}

static int calculate_image_size( Environment * _environment, int _width, int _height, int _mode ) {

    switch( _mode ) {

        case BITMAP_MODE_STANDARD:

            return 3 + ( ( _width >> 3 ) * _height ) + ( ( _width >> 3 ) * ( _height >> 3 ) );

        case BITMAP_MODE_MULTICOLOR:

            return 3 + ( ( _width >> 2 ) * _height ) + 2 * ( ( _width >> 2 ) * ( _height >> 3 ) ) + 1;

        case BITMAP_MODE_AH:
        case BITMAP_MODE_AIFLI:
        case BITMAP_MODE_ASSLACE:
        case BITMAP_MODE_ECI:
        case BITMAP_MODE_IAFLI:
        case BITMAP_MODE_IH:
        case BITMAP_MODE_MRFLI:
        case BITMAP_MODE_MUCSUFLI:
        case BITMAP_MODE_MUCSUH:
        case BITMAP_MODE_MUFLI:
        case BITMAP_MODE_MUIFLI:
        case BITMAP_MODE_NUFLI:
        case BITMAP_MODE_NUIFLI:
        case BITMAP_MODE_SH:
        case BITMAP_MODE_SHFLI:
        case BITMAP_MODE_SHI:
        case BITMAP_MODE_SHIFLI:
        case BITMAP_MODE_SHIFXL:
        case BITMAP_MODE_UFLI:
        case BITMAP_MODE_UIFLI:
        case BITMAP_MODE_TRIFLI:
        case BITMAP_MODE_XFLI:
        case BITMAP_MODE_XIFLI:
        case BITMAP_MODE_FLI:
        case BITMAP_MODE_HCB:
        case BITMAP_MODE_IFLI:
        case BITMAP_MODE_MUCSU:
        case BITMAP_MODE_MCI:
        case BITMAP_MODE_MEGATEXT:
        case BITMAP_MODE_PRS:
        case TILEMAP_MODE_STANDARD:
        case TILEMAP_MODE_MULTICOLOR:
        case TILEMAP_MODE_EXTENDED:
            break;
    }

    return 0;

}

static int calculate_images_size( Environment * _environment, int _frames, int _width, int _height, int _mode ) {

    switch( _mode ) {

        case BITMAP_MODE_STANDARD:

            return 3 + ( 3 + ( ( _width >> 3 ) * _height ) + ( ( _width >> 3 ) * ( _height >> 3 ) ) ) * _frames;

        case BITMAP_MODE_MULTICOLOR:

            return 3 + ( 3 + ( ( _width >> 2 ) * _height ) + 2 * ( ( _width >> 2 ) * ( _height >> 3 ) ) + 1 ) * _frames;

        case BITMAP_MODE_AH:
        case BITMAP_MODE_AIFLI:
        case BITMAP_MODE_ASSLACE:
        case BITMAP_MODE_ECI:
        case BITMAP_MODE_IAFLI:
        case BITMAP_MODE_IH:
        case BITMAP_MODE_MRFLI:
        case BITMAP_MODE_MUCSUFLI:
        case BITMAP_MODE_MUCSUH:
        case BITMAP_MODE_MUFLI:
        case BITMAP_MODE_MUIFLI:
        case BITMAP_MODE_NUFLI:
        case BITMAP_MODE_NUIFLI:
        case BITMAP_MODE_SH:
        case BITMAP_MODE_SHFLI:
        case BITMAP_MODE_SHI:
        case BITMAP_MODE_SHIFLI:
        case BITMAP_MODE_SHIFXL:
        case BITMAP_MODE_UFLI:
        case BITMAP_MODE_UIFLI:
        case BITMAP_MODE_TRIFLI:
        case BITMAP_MODE_XFLI:
        case BITMAP_MODE_XIFLI:
        case BITMAP_MODE_FLI:
        case BITMAP_MODE_HCB:
        case BITMAP_MODE_IFLI:
        case BITMAP_MODE_MUCSU:
        case BITMAP_MODE_MCI:
        case BITMAP_MODE_MEGATEXT:
        case BITMAP_MODE_PRS:
        case TILEMAP_MODE_STANDARD:
        case TILEMAP_MODE_MULTICOLOR:
        case TILEMAP_MODE_EXTENDED:
            break;
    }

    return 0;

}

static Variable * vic2_image_converter_bitmap_mode_standard( Environment * _environment, char * _source, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _transparent_color, int _flags ) {

    image_converter_asserts( _environment, _width, _height, _offset_x, _offset_y, &_frame_width, &_frame_height );

    RGBi * palette = malloc_palette( MAX_PALETTE );
    
    int paletteColorCount = rgbi_extract_palette(_environment, _source, _width, _height, _depth, palette, MAX_PALETTE, ( ( _flags & FLAG_EXACT ) ? 0 : 1 ) /* sorted */);

    if (paletteColorCount > 16) {
        CRITICAL_IMAGE_CONVERTER_TOO_COLORS( paletteColorCount );
    }

    int i, j, k;

    commonPalette = palette_match( palette, paletteColorCount, SYSTEM_PALETTE, sizeof(SYSTEM_PALETTE) / sizeof(RGBi) );
    commonPalette = palette_remove_duplicates( commonPalette, paletteColorCount, &paletteColorCount );
    lastUsedSlotInCommonPalette = paletteColorCount;
    adilinepalette( "CPM1:%d", paletteColorCount, commonPalette );

    adilinepalette( "CPMS:%ld", sizeof(SYSTEM_PALETTE) / sizeof(RGBi), SYSTEM_PALETTE );

    Variable * result = variable_temporary( _environment, VT_IMAGE, 0 );
    result->originalColors = lastUsedSlotInCommonPalette;
    memcpy( result->originalPalette, commonPalette, lastUsedSlotInCommonPalette * sizeof( RGBi ) );

    int bufferSize = calculate_image_size( _environment, _frame_width, _frame_height, BITMAP_MODE_STANDARD );

    adiline3("BMP:%4.4x:%4.4x:%2.2x", _frame_width, _frame_height, BITMAP_MODE_STANDARD );

    // printf("bufferSize = %d\n", bufferSize );

    char * buffer = malloc ( bufferSize );
    memset( buffer, 0, bufferSize );

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGBi rgb;

    *(buffer) = ( _frame_width & 0xff );
    *(buffer+1) = ( (_frame_width>>8) & 0xff );
    *(buffer+2) = _frame_height;

    _source += ( ( _offset_y * _width ) + _offset_x ) * 3;

    vic2_image_converter_tiles( _environment, _source, buffer+3, _frame_width, _frame_height, _depth, _width );

    // printf("----\n");

    variable_store_buffer( _environment, result->name, buffer, bufferSize, 0 );

    // printf("----\n");

    return result;

}

static Variable * vic2_image_converter_multicolor_mode_standard( Environment * _environment, char * _source, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _transparent_color, int _flags ) {

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );

    image_converter_asserts( _environment, _width, _height, _offset_x, _offset_y, &_frame_width, &_frame_height );

    RGBi * palette = malloc_palette( MAX_PALETTE );
    
    int paletteColorCount = rgbi_extract_palette(_environment, _source, _width, _height, _depth, palette, MAX_PALETTE, ( ( _flags & FLAG_EXACT ) ? 0 : 1 ) /* sorted */);

    if (paletteColorCount > 16) {
        CRITICAL_IMAGE_CONVERTER_TOO_COLORS( paletteColorCount );
    }

    int i, j, k;

    commonPalette = palette_match( palette, paletteColorCount, SYSTEM_PALETTE, sizeof(SYSTEM_PALETTE) / sizeof(RGBi) );
    commonPalette = palette_remove_duplicates( commonPalette, paletteColorCount, &paletteColorCount );
    lastUsedSlotInCommonPalette = paletteColorCount;
    adilinepalette( "CPM1:%d", paletteColorCount, commonPalette );

    adilinepalette( "CPMS:%ld", sizeof(SYSTEM_PALETTE) / sizeof(RGBi), SYSTEM_PALETTE );

    Variable * result = variable_temporary( _environment, VT_IMAGE, 0 );
    result->originalColors = lastUsedSlotInCommonPalette;
    memcpy( result->originalPalette, commonPalette, lastUsedSlotInCommonPalette * sizeof( RGBi ) );

    int bufferSize = calculate_image_size( _environment, _frame_width, _frame_height, BITMAP_MODE_MULTICOLOR );
    
    adiline3("BMP:%4.4x:%4.4x:%2.2x", _frame_width, _frame_height, BITMAP_MODE_MULTICOLOR );

    char * buffer = malloc ( bufferSize );
    memset( buffer, 0, bufferSize );

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, offsetc, bitmask;

    // Color of the pixel to convert
    RGBi rgb;

    *(buffer) = ( _frame_width & 0xff );
    *(buffer+1) = ( _frame_width >> 8 ) & 0xff;
    *(buffer+2) = _frame_height;

    _source += ( ( _offset_y * _width ) + _offset_x ) * _depth;

    vic2_image_converter_tiles_multicolor( _environment, _source, buffer+3, _frame_width, _frame_height, _depth, _width, palette[0].index );
    
    variable_store_buffer( _environment, result->name, buffer, bufferSize, 0 );

    return result;

}

static Variable * vic2_image_converter_tilemap_mode_standard( Environment * _environment, char * _source, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _transparent_color, int _flags ) {

    image_converter_asserts( _environment, _width, _height, _offset_x, _offset_y, &_frame_width, &_frame_height );

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );

    RGBi palette[MAX_PALETTE];

    int colorUsed = rgbi_extract_palette(_environment, _source, _width, _height, _depth, palette, MAX_PALETTE, (_flags & FLAG_EXACT) /* sorted */ );

    commonPalette = palette_match( palette, colorUsed, SYSTEM_PALETTE, sizeof(SYSTEM_PALETTE) / sizeof(RGBi) );
    commonPalette = palette_remove_duplicates( commonPalette, colorUsed, &colorUsed );
    adilinepalette( "CPM1:%d", colorUsed, commonPalette );

    adilinepalette( "CPMS:%ld", sizeof(SYSTEM_PALETTE) / sizeof(RGBi), SYSTEM_PALETTE );

    Variable * result = variable_temporary( _environment, VT_IMAGE, 0 );
    result->originalColors = colorUsed;
    memcpy( result->originalPalette, commonPalette, colorUsed * sizeof( RGBi ) );

    int bufferSize;
    
    if ( colorUsed == 2 ) {
        bufferSize = 3 + ( ( _frame_width >> 3 ) * ( _frame_height >> 3 ) ) + 1;
    } else {
        bufferSize = 3 + 2* ( ( _frame_width >> 3 ) * ( _frame_height >> 3 ) );
    } 

    // printf("bufferSize = %d\n", bufferSize );

    char * buffer = malloc ( bufferSize );
    memset( buffer, 0, bufferSize );

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGBi rgb;

    *(buffer) = _frame_width;
    *(buffer+1) = _frame_height;

    if ( colorUsed > 2 ) {
        *(buffer+2) = 1;
    } else {
        *(buffer+2) = 0;
    }

    int cx=0,cy=0;

    _source += ( ( _offset_y * _width ) + _offset_x ) * _depth;

    // commonTileDescriptors = precalculate_tile_descriptors_for_font( data_fontvic1_bin );

    for( cy=0; cy<(_frame_height >> 3);++cy) {
        for( cx=0; cx<(_frame_width >> 3);++cx) {

            char *source = _source + ( ( cy * 8 * _width ) + cx * 8 ) * _depth;

            TileData tileData;
            memset(&tileData,0,sizeof(TileData));

            int mostFrequentColor[16];
            memset(&mostFrequentColor[0],0,sizeof(int)*16);

            int colorIndex = 0;

            // Loop for all the source surface.
            for (image_y = 0; image_y < 8; ++image_y) {
                for (image_x = 0; image_x < 8; ++image_x) {

                    // Take the color of the pixel
                    rgb.red = *source;
                    rgb.green = *(source + 1);
                    rgb.blue = *(source + 2);
                    if ( _depth > 3 ) {
                        rgb.alpha = *(_source + 3);
                    } else {
                        rgb.alpha = 255;
                    }

                    if ( rgb.alpha < 255 ) {
                        colorIndex = commonPalette[0].index;
                    } else {

                        int minDistance = 9999;
                        for( int i=0; i<colorUsed; ++i ) {
                            int distance = rgbi_distance(&commonPalette[i], &rgb );
                            if ( distance < minDistance ) {
                                minDistance = distance;
                                colorIndex = commonPalette[i].index;
                            }
                        }

                    }

                    mostFrequentColor[colorIndex]++;

                    // printf("%d", i );

                    // Calculate the relative tile
                    
                    // Calculate the offset starting from the tile surface area
                    // and the bit to set.
                    offset = (image_y & 0x07);
                    bitmask = 1 << ( 7 - (image_x & 0x7) );

                    if ( !colorIndex ) {
                        tileData.data[offset] &= ~bitmask;
                        // printf("%1.1x", colorIndex );
                    } else {
                        tileData.data[offset] |= bitmask;
                        // printf("%1.1x", colorIndex );
                    }

                    source += _depth;

                }

                source += _depth * ( _width - 8 );

                // printf("\n" );

            }

            // printf("\n" );
            
            TileDescriptor * t = calculate_tile_descriptor( &tileData );

            if ( ! _environment->descriptors ) {
                _environment->descriptors = precalculate_tile_descriptors_for_font( data_font_alpha_bin, data_font_alpha_bin_len / 8 );
                _environment->descriptors->first = 0;
                _environment->descriptors->firstFree = ( (data_font_alpha_bin_len / 8) );
                _environment->descriptors->lastFree = 255;
                _environment->descriptors->count = _environment->descriptors->firstFree;
            }

            int tile = calculate_exact_tile( t, _environment->descriptors );

            if ( tile == -1 ) {
                if ( _environment->descriptors->count < 256 ) {
                    tile = (_environment->descriptors->count++);
                    _environment->descriptors->descriptor[tile] = t; 
                    memcpy( &_environment->descriptors->data[tile], &tileData, sizeof( TileData ) ); 
                    // printf("*** tile = %d\n", tile );
                } else {
                    tile = calculate_nearest_tile( t, _environment->descriptors );
                    // printf("--- tile = %d\n", tile );
                }
            } else {
                // printf("    tile = %d\n", tile );
            }

            // printf("    colorUsed = %d\n", colorUsed );
            int mostFrequentColorIndex = 1;
            int mostFrequentColorCount = 0;
            for(int i=0;i<colorUsed;++i) {
                if ( mostFrequentColorCount < mostFrequentColor[commonPalette[i].index] ) {
                    mostFrequentColorCount = mostFrequentColor[commonPalette[i].index];
                    mostFrequentColorIndex = commonPalette[i].index;
                }
            }

            *(buffer + 3 + (cy * ( _frame_width >> 3 ) ) + cx ) = tile;
            if ( colorUsed > 2 ) {
                *(buffer + 3 + ( ( _frame_width >> 3 ) * ( _frame_height >> 3 ) ) + (cy * ( _frame_width >> 3 ) ) + cx ) = ( ( mostFrequentColorIndex & 0x07 ) );
            }

            // printf("\ntile: %2.2x\n", tile );

        }
        // printf("\n");
    }

    if ( colorUsed <= 2 ) {
        *(buffer + 3 + ( ( _frame_width >> 3 ) * ( _frame_height >> 3 ) ) ) = commonPalette[0].index == 0 ? commonPalette[1].index : commonPalette[0].index;
    }
    // printf("----\n");

    variable_store_buffer( _environment, result->name, buffer, bufferSize, 0 );

    // printf("----\n");

    return result;

}

static Variable * vic2_image_converter_tilemap_mode_multicolor( Environment * _environment, char * _source, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _transparent_color, int _flags ) {

    image_converter_asserts( _environment, _width, _height, _offset_x, _offset_y, &_frame_width, &_frame_height );

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );

    RGBi palette[MAX_PALETTE];

    int colorUsed = rgbi_extract_palette(_environment, _source, _width, _height, _depth, palette, MAX_PALETTE, (_flags & FLAG_EXACT) /* sorted */ );

    commonPalette = palette_match( palette, colorUsed, SYSTEM_PALETTE, ( sizeof(SYSTEM_PALETTE) / sizeof(RGBi) ) / 2 );
    commonPalette = palette_remove_duplicates( commonPalette, colorUsed, &colorUsed );
    adilinepalette( "CPM1:%d", colorUsed, commonPalette );

    adilinepalette( "CPMS:%ld", ( sizeof(SYSTEM_PALETTE) / sizeof(RGBi) ) / 2, SYSTEM_PALETTE );

    Variable * result = variable_temporary( _environment, VT_IMAGE, 0 );
    result->originalColors = colorUsed;
    memcpy( result->originalPalette, commonPalette, colorUsed * sizeof( RGBi ) );

    int bufferSize;
    
    bufferSize = 3 + 2* ( ( _frame_width >> 2 ) * ( _frame_height >> 3 ) ) + 2;

    // printf("bufferSize = %d\n", bufferSize );

    char * buffer = malloc ( bufferSize );
    memset( buffer, 0, bufferSize );

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of tiles
    int tile_x, tile_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGBi rgb;

    *(buffer) = _frame_width;
    *(buffer+1) = _frame_height;
    *(buffer+2) = 0;

    int cx=0,cy=0;

    _source += ( ( _offset_y * _width ) + _offset_x ) * _depth;

    // commonTileDescriptors = precalculate_tile_descriptors_for_font( data_fontvic1_bin );

    for( cy=0; cy<(_frame_height >> 3);++cy) {
        for( cx=0; cx<(_frame_width >> 2);++cx) {

            char *source = _source + ( ( cy * 8 * _width ) + cx * 4 ) * _depth;

            TileData tileData;
            memset(&tileData,0,sizeof(TileData));

            int mostFrequentColor[16];
            memset(&mostFrequentColor[0],0,sizeof(int)*16);

            int colorIndex = 0;

            // Loop for all the source surface.
            for (image_y = 0; image_y < 8; ++image_y) {
                for (image_x = 0; image_x < 4; ++image_x) {

                    // Take the color of the pixel
                    rgb.red = *source;
                    rgb.green = *(source + 1);
                    rgb.blue = *(source + 2);
                    if ( _depth > 3 ) {
                        rgb.alpha = *(_source + 3);
                    } else {
                        rgb.alpha = 255;
                    }

                    colorIndex = 0;
                    if ( rgb.alpha < 255 ) {
                        colorIndex = commonPalette[0].index;
                    } else {

                        int minDistance = 9999;
                        for( int i=0; i<colorUsed; ++i ) {
                            int distance = rgbi_distance(&commonPalette[i], &rgb );
                            if ( distance < minDistance ) {
                                minDistance = distance;
                                colorIndex = commonPalette[i].index;
                            }
                        }

                    }

                    mostFrequentColor[colorIndex]++;

                    // printf("%d", i );

                    // Calculate the relative tile
                    
                    // Calculate the offset starting from the tile surface area
                    // and the bit to set.
                    offset = (image_y & 0x07);
                    bitmask = 1 << ( 6 - (2*(image_x & 0x3)) );

                    if ( colorIndex == 0 ) {
                        tileData.data[offset] &= ~(3*bitmask);
                    } else if ( colorIndex == commonPalette[1].index ) {
                        tileData.data[offset] |= bitmask;
                    } else if ( colorIndex == commonPalette[2].index ) {
                        tileData.data[offset] |= 2*bitmask;
                    } else {
                        if ( !colorIndex ) {
                            tileData.data[offset] &= ~(3*bitmask);
                        } else {
                            tileData.data[offset] |= (3*bitmask);
                        }
                    }
                    // printf("%1.1x", colorIndex );

                    source += _depth;

                }

                source += _depth * ( _width - 4 );

                // printf("\n" );

            }

            // printf("\n" );
            
            TileDescriptor * t = calculate_tile_descriptor( &tileData );

            if ( ! _environment->descriptors ) {
                _environment->descriptors = precalculate_tile_descriptors_for_font( data_font_alpha_bin, data_font_alpha_bin_len / 8 );
                _environment->descriptors->first = 0;
                _environment->descriptors->firstFree = ( (data_font_alpha_bin_len / 8) );
                _environment->descriptors->lastFree = 255;
                _environment->descriptors->count = _environment->descriptors->firstFree;
            }

            int tile = calculate_exact_tile( t, _environment->descriptors );

            if ( tile == -1 ) {
                if ( _environment->descriptors->count < 256 ) {
                    tile = (_environment->descriptors->count++);
                    _environment->descriptors->descriptor[tile] = t; 
                    memcpy( &_environment->descriptors->data[tile], &tileData, sizeof( TileData ) ); 
                    // printf("*** tile = %d\n", tile );
                } else {
                    tile = calculate_nearest_tile( t, _environment->descriptors );
                    // printf("--- tile = %d\n", tile );
                }
            } else {
                // printf("    tile = %d\n", tile );
            }

            // printf("    colorUsed = %d\n", colorUsed );
            int mostFrequentColorIndex = 1;
            int mostFrequentColorCount = 0;
            for(int i=3;i<colorUsed;++i) {
                if ( mostFrequentColorCount < mostFrequentColor[commonPalette[i].index] ) {
                    mostFrequentColorCount = mostFrequentColor[commonPalette[i].index];
                    mostFrequentColorIndex = commonPalette[i].index;
                }
            }

            *(buffer + 3 + (cy * ( _frame_width >> 3 ) ) + cx ) = tile;
            *(buffer + 3 + ( ( _frame_width >> 2 ) * ( _frame_height >> 3 ) ) + (cy * ( _frame_width >> 2 ) ) + cx ) = 0x8 | ( ( mostFrequentColorIndex & 0x03 ) );

            // printf("\ntile: %2.2x\n", tile );

        }
        // printf("\n");
    }

    *(buffer + 3 + 2* ( ( _frame_width >> 2 ) * ( _frame_height >> 3 ) ) ) = commonPalette[0].index;
    *(buffer + 3 + 2* ( ( _frame_width >> 2 ) * ( _frame_height >> 3 ) ) + 1 ) = commonPalette[1].index;
    *(buffer + 3 + 2* ( ( _frame_width >> 2 ) * ( _frame_height >> 3 ) ) + 2 ) = commonPalette[2].index;

    // printf("----\n");

    variable_store_buffer( _environment, result->name, buffer, bufferSize, 0 );

    // printf("----\n");

    return result;

}

Variable * vic2_image_converter( Environment * _environment, char * _data, int _width, int _height, int _depth, int _offset_x, int _offset_y, int _frame_width, int _frame_height, int _mode, int _transparent_color, int _flags ) {

    switch( _mode ) {

        case BITMAP_MODE_STANDARD:

            return vic2_image_converter_bitmap_mode_standard( _environment, _data, _width, _height, _depth, _offset_x, _offset_y, _frame_width, _frame_height, _transparent_color, _flags );

        case BITMAP_MODE_MULTICOLOR:

            return vic2_image_converter_multicolor_mode_standard( _environment, _data, _width, _height, _depth, _offset_x, _offset_y, _frame_width, _frame_height, _transparent_color, _flags );

        case BITMAP_MODE_AH:
        case BITMAP_MODE_AIFLI:
        case BITMAP_MODE_ASSLACE:
        case BITMAP_MODE_ECI:
        case BITMAP_MODE_IAFLI:
        case BITMAP_MODE_IH:
        case BITMAP_MODE_MRFLI:
        case BITMAP_MODE_MUCSUFLI:
        case BITMAP_MODE_MUCSUH:
        case BITMAP_MODE_MUFLI:
        case BITMAP_MODE_MUIFLI:
        case BITMAP_MODE_NUFLI:
        case BITMAP_MODE_NUIFLI:
        case BITMAP_MODE_SH:
        case BITMAP_MODE_SHFLI:
        case BITMAP_MODE_SHI:
        case BITMAP_MODE_SHIFLI:
        case BITMAP_MODE_SHIFXL:
        case BITMAP_MODE_UFLI:
        case BITMAP_MODE_UIFLI:
        case BITMAP_MODE_TRIFLI:
        case BITMAP_MODE_XFLI:
        case BITMAP_MODE_XIFLI:
        case BITMAP_MODE_FLI:
        case BITMAP_MODE_HCB:
        case BITMAP_MODE_IFLI:
        case BITMAP_MODE_MUCSU:
        case BITMAP_MODE_MCI:
        case BITMAP_MODE_MEGATEXT:
        case BITMAP_MODE_PRS:
        case TILEMAP_MODE_STANDARD:
            return vic2_image_converter_tilemap_mode_standard( _environment, _data, _width, _height, _depth, _offset_x, _offset_y, _frame_width, _frame_height, _transparent_color, _flags );
        case TILEMAP_MODE_MULTICOLOR:
            return vic2_image_converter_tilemap_mode_multicolor( _environment, _data, _width, _height, _depth, _offset_x, _offset_y, _frame_width, _frame_height, _transparent_color, _flags );
        case TILEMAP_MODE_EXTENDED:
            break;
    }

    WARNING_IMAGE_CONVERTER_UNSUPPORTED_MODE( _mode );

    return vic2_new_image( _environment, 8, 8, BITMAP_MODE_STANDARD );

}

Variable * vic2_sprite_converter( Environment * _environment, char * _source, int _width, int _height, int _depth, RGBi * _color, int _flags ) {

    RGBi palette[MAX_PALETTE];

    int colorUsed = vic2_palette_extract( _environment, _source, _width, _height, _depth, _flags, &palette[0] );

//    int colorUsed = rgbi_extract_palette(_environment, _source, _width, _height, _depth, palette, MAX_PALETTE, 1 /* sorted */ );

    if ( ! _color ) {

        if ( _flags & SPRITE_FLAG_MULTICOLOR ) {
            if (colorUsed > 4) {
                CRITICAL_IMAGE_CONVERTER_TOO_COLORS( colorUsed );
            }
        } else {
            if (colorUsed > 2) {
                CRITICAL_IMAGE_CONVERTER_TOO_COLORS( colorUsed );
            }
        }

    }
    
    Variable * result = variable_temporary( _environment, VT_IMAGE, 0 );    
    result->originalColors = colorUsed;

    int i, j, k;

    // for( i=0; i<colorUsed; ++i ) {
    //     int minDistance = 0xffff;
    //     int colorIndex = 0;
    //     for (j = 0; j < sizeof(SYSTEM_PALETTE)/sizeof(RGBi); ++j) {
    //         int distance = rgbi_distance(&SYSTEM_PALETTE[j], &palette[i]);
    //         // printf("%d <-> %d [%d] = %d [min = %d]\n", i, j, SYSTEM_PALETTE[j].index, distance, minDistance );
    //         if (distance < minDistance) {
    //             // printf(" candidated...\n" );
    //             for( k=0; k<i; ++k ) {
    //                 if ( palette[k].index == SYSTEM_PALETTE[j].index ) {
    //                     // printf(" ...used!\n" );
    //                     break;
    //                 }
    //             }
    //             if ( k>=i ) {
    //                 // printf(" ...ok! (%d)\n", SYSTEM_PALETTE[j].index );
    //                 minDistance = distance;
    //                 colorIndex = j;
    //             }
    //         }
    //     }
    //     palette[i].index = SYSTEM_PALETTE[colorIndex].index;
    //     strcpy( palette[i].description, SYSTEM_PALETTE[colorIndex].description );
    //     // printf("%d) %d %2.2x%2.2x%2.2x\n", i, palette[i].index, palette[i].red, palette[i].green, palette[i].blue);
    // }

    if ( _flags & SPRITE_FLAG_MULTICOLOR ) {

        // printf("PALETTE 0: %d\n", palette[0].index );
        // printf("PALETTE 1: %d %2.2x%2.2x%2.2x\n", palette[1].index, palette[1].red, palette[1].green, palette[1].blue );
        // printf("PALETTE 2: %d\n", palette[2].index );
        // printf("PALETTE 3: %d\n", palette[3].index );

        if ( palette[0].index == SYSTEM_PALETTE[0].index ) {

        } else {
            rgbi_move( &palette[2], &palette[3] );
            rgbi_move( &palette[1], &palette[2] );
            rgbi_move( &palette[0], &palette[1] );
            rgbi_move( &SYSTEM_PALETTE[0], &palette[0] );
        }

        // printf("PALETTE 0: %d\n", palette[0].index );
        // printf("PALETTE 1: %d %2.2x%2.2x%2.2x\n", palette[1].index, palette[1].red, palette[1].green, palette[1].blue );
        // printf("PALETTE 2: %d\n", palette[2].index );
        // printf("PALETTE 3: %d\n", palette[3].index );

        // printf("Color used = %d\n", colorUsed);

        // printf("Decoding multicolor sprite color #0\n");

        if ( !multicolorSpritePalette[0] ) {
            // printf("Initializing with color 1 (%d)\n", palette[1].index );
            multicolorSpritePalette[0] = malloc( sizeof( RGBi ) );
            memset( multicolorSpritePalette[0], 0, sizeof( RGBi ) );
            rgbi_move( &palette[1], multicolorSpritePalette[0] );
        }

        // printf("Decoding multicolor sprite color #1\n");

        if ( !multicolorSpritePalette[1] ) {
            // printf("Initializing with color 2 (%d)\n", palette[2].index );
            multicolorSpritePalette[1] = malloc( sizeof( RGBi ) );
            memset( multicolorSpritePalette[1], 0, sizeof( RGBi ) );
            rgbi_move( &palette[2], multicolorSpritePalette[1] );
        }

        RGBi temporaryPalette[MAX_PALETTE];
        memset( temporaryPalette, 0, sizeof( RGBi ) * MAX_PALETTE );

        rgbi_move( &SYSTEM_PALETTE[0], &temporaryPalette[0] );

        for( int i=1; i<colorUsed; ++i ) {
            if ( rgbi_equals_rgba( &palette[i], multicolorSpritePalette[0] ) ) {
                // printf("%d) Color #%d == sprite palette 0\n", i, palette[i].index );
                rgbi_move( &palette[i], &temporaryPalette[1] );
            } else if ( rgbi_equals_rgba( &palette[i], multicolorSpritePalette[1] ) ) {
                // printf("%d) Color #%d == sprite palette 1\n", i, palette[i].index );
                rgbi_move( &palette[i], &temporaryPalette[3] );
            } else {
                // printf("%d) Color #%d == sprite custom\n", i, palette[i].index );
                rgbi_move( &palette[i], &temporaryPalette[2] );
            }
        }

        rgbi_move( &temporaryPalette[0], &palette[0] );
        // printf("PALETTE 0: %d\n", palette[0].index );
        rgbi_move( &temporaryPalette[1], &palette[1] );
        // printf("PALETTE 1: %d\n", palette[1].index );
        rgbi_move( &temporaryPalette[2], &palette[2] );
        // printf("PALETTE 2: %d\n", palette[2].index );
        rgbi_move( &temporaryPalette[3], &palette[3] );
        // printf("PALETTE 3: %d\n", palette[3].index );

    }

    memcpy( result->originalPalette, palette, MAX_PALETTE * sizeof( RGBi ) );
    
    int bufferSize = 64;

    // printf("bufferSize = %d\n", bufferSize );

    char * buffer = malloc ( bufferSize );
    memset( buffer, 0, bufferSize );

    // Position of the pixel in the original image
    int image_x, image_y;
    
    // Position of the pixel, in terms of offset and bitmask
    int offset, bitmask;

    // Color of the pixel to convert
    RGBi rgb;

    // Loop for all the source surface.
    for (image_y = 0; image_y < _height; ++image_y) {
        if ( image_y == 21 ) {
            break;
        }
        for (image_x = 0; image_x < _width; ++image_x) {

            if ( _flags & SPRITE_FLAG_MULTICOLOR ) {
                if ( image_x == 12 ) {
                    break;
                }
            } else {
                if ( image_x == 24 ) {
                    break;
                }
            }

            // Take the color of the pixel
            rgb.red = *_source;
            rgb.green = *(_source + 1);
            rgb.blue = *(_source + 2);
            if ( _depth > 3 ) {
                rgb.alpha = *(_source + 3);
            } else {
                rgb.alpha = 255;
            }

            if ( rgb.alpha < 255 ) {
                i = 0;
            } else {

                if ( ! _color ) {
                    int minDistance = 0xffff;
                    RGBi * color = NULL;
                    i = 0;
                    for( int k=0; k<colorUsed; ++k ) {
                        int distance = rgbi_distance( &palette[k], &rgb );
                        if ( distance < minDistance ) {
                            minDistance = distance;
                            color = &palette[k];
                            i = k;
                        }
                    }
                    // for( i=0; i<colorUsed; ++i ) {
                    //     // printf( "%d) %2.2x%2.2x%2.2x == %2.2x%2.2x%2.2x\n", i, palette[i].red, palette[i].green, palette[i].blue, rgb.red, rgb.green, rgb.blue );
                    //     if ( rgbi_equals_rgba( &palette[i], color ) ) {
                    //         break;
                    //     }
                    // }
                } else {
                    int minDistance = 0xffff;
                    RGBi * color = NULL;
                    for( int k=0; k<colorUsed; ++k ) {
                        int distance = rgbi_distance( &palette[k], &rgb );
                        if ( distance < minDistance ) {
                            minDistance = distance;
                            color = &palette[k];
                        }
                    }
                    if ( rgbi_equals_rgba( _color, color ) ) {
                        i = 1;
                    } else {
                        i = 0;
                    }
                }

            }

            if ( _flags & SPRITE_FLAG_MULTICOLOR ) {

                // Calculate the offset starting from the tile surface area
                // and the bit to set.
                offset = ( image_y * 3 ) + (image_x >> 2);
                bitmask = i << (6 - ((image_x & 0x3) * 2));

                if ( i > 0 ) {
                    *( buffer + offset) |= bitmask;
                    // printf("%1.1x", i );
                } else {
                    *( buffer + offset) &= ~bitmask;
                    // printf("%1.1x", i );
                }

            } else {

                // Calculate the offset starting from the tile surface area
                // and the bit to set.
                offset = ( image_y * 3 ) + (image_x >> 3);
                bitmask = 1 << ( 7 - (image_x & 0x7) );

                if ( i == 1 ) {
                    *( buffer + offset) |= bitmask;
                    //printf("*");
                } else {
                    *( buffer + offset) &= ~bitmask;
                    //printf(" ");
                }

            }

            _source += _depth;

        }

        // printf("\n");
        _source += _depth * ( _width - image_x );

        // printf("\n" );

    }

    if ( _color ) {
        *(buffer+63) = _color->index;
    } else {
        if ( _flags & SPRITE_FLAG_MULTICOLOR ) {
            *(buffer+63) = palette[3].index;
        } else {
            *(buffer+63) = palette[1].index;
        }
    }

    // printf("\n----\n");

    variable_store_buffer( _environment, result->name, buffer, bufferSize, 0 );

    // printf("----\n");

    return result;

}

static void vic2_load_image_address_to_other_register( Environment * _environment, char * _register, char * _source, char * _sequence, char * _frame, int _frame_size, int _frame_count ) {

    outline1("LDA #<%s", _source );
    outline1("STA %s", _register );
    outline1("LDA #>%s", _source );
    outline1("STA %s", address_displacement(_environment, _register, "1") );

    if ( _sequence ) {

        outline0("CLC" );
        outline1("LDA %s", _register );
        outline0("ADC #3" );
        outline1("STA %s", _register );
        outline1("LDA %s", address_displacement(_environment, _register, "1") );
        outline0("ADC #0" );
        outline1("STA %s", address_displacement(_environment, _register, "1") );
        if ( strlen(_sequence) == 0 ) {

        } else {
            outline1("LDA #<OFFSETS%4.4x", _frame_size * _frame_count );
            outline0("STA MATHPTR0" );
            outline1("LDA #>OFFSETS%4.4x", _frame_size * _frame_count );
            outline0("STA MATHPTR1" );
            outline0("CLC" );
            outline1("LDA %s", _sequence );
            outline0("ASL" );
            outline0("TAY" );
            outline1("LDA %s", _register );
            outline0("ADC (MATHPTR0), Y" );
            outline1("STA %s", _register );
            outline0("INY" );
            outline1("LDA %s", address_displacement(_environment, _register, "1") );
            outline0("ADC (MATHPTR0), Y" );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
        }

        if ( _frame ) {
            if ( strlen(_frame) == 0 ) {

            } else {
                outline1("LDA #<OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0" );
                outline1("LDA #>OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR1" );
                outline0("CLC" );
                outline1("LDA %s", _frame );
                outline0("ASL" );
                outline0("TAY" );
                outline1("LDA %s", _register );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", _register );
                outline0("INY" );
                outline1("LDA %s", address_displacement(_environment, _register, "1") );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", address_displacement(_environment, _register, "1") );
            }
        }

    } else {

        if ( _frame ) {
            outline0("CLC" );
            outline1("LDA %s", _register );
            outline0("ADC #3" );
            outline1("STA %s", _register );
            outline1("LDA %s", address_displacement(_environment, _register, "1") );
            outline0("ADC #0" );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
            if ( strlen(_frame) == 0 ) {

            } else {
                outline1("LDA #<OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0" );
                outline1("LDA #>OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0+1" );
                outline0("CLC" );
                outline1("LDA %s", _frame );
                outline0("ASL" );
                outline0("TAY" );
                outline1("LDA %s", _register );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", _register );
                outline0("INY" );
                outline1("LDA %s", address_displacement(_environment, _register, "1") );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", address_displacement(_environment, _register, "1") );
            }
        }

    }

}

static void vic2_load_image_address_to_register( Environment * _environment, char * _register, Resource * _source, char * _sequence, char * _frame, int _frame_size, int _frame_count ) {

    if ( !_sequence && !_frame ) {
        if ( _source->isAddress ) {
            outline1("LDA %s", _source->realName );
            outline1("STA %s", _register );
            outline1("LDA %s", address_displacement(_environment, _source->realName, "1") );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
        } else {
            outline1("LDA #<%s", _source->realName );
            outline1("STA %s", _register );
            outline1("LDA #>%s", _source->realName );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
        }
    } else {
        if ( _source->isAddress ) {
            outline1("LDA %s", _source->realName );
            outline0("STA TMPPTR" );
            outline1("LDA %s", address_displacement(_environment, _source->realName, "1") );
            outline0("STA TMPPTR+1" );
        } else {
            outline1("LDA #<%s", _source->realName );
            outline0("STA TMPPTR" );
            outline1("LDA #>%s", _source->realName );
            outline0("STA TMPPTR+1" );
        }

        if ( _sequence ) {
            outline0("CLC" );
            outline0("LDA TMPPTR" );
            outline0("ADC #3" );
            outline0("STA TMPPTR" );
            outline0("LDA TMPPTR+1" );
            outline0("ADC #0" );
            outline0("STA  TMPPTR+1" );

            if ( strlen(_sequence) == 0 ) {

            } else {
                outline1("LDA %s", _sequence );
                outline0("STA MATHPTR0" );
                outline1("JSR %soffsetsequence", _source->realName );
            }
            if ( _frame ) {
                if ( strlen(_frame) == 0 ) {

                } else {
                    outline1("LDA %s", _frame );
                    outline0("STA MATHPTR0" );
                    outline1("JSR %soffsetframe", _source->realName );
                }
            }

        } else {

            if ( _frame ) {
                outline0("CLC" );
                outline0("LDA TMPPTR" );
                outline0("ADC #3" );
                outline0("STA TMPPTR" );
                outline0("LDA TMPPTR+1" );
                outline0("ADC #0" );
                outline0("STA  TMPPTR+1" );
                if ( strlen(_frame) == 0 ) {

                } else {
                    outline1("LDA %s", _frame );
                    outline0("STA MATHPTR0" );
                    outline1("JSR %soffsetframe", _source->realName );
                }
            }

        }

        if ( _source->isAddress ) {
            outline0("LDA TMPPTR" );
            outline1("STA %s", _register );
            outline0("LDA TMPPTR+1" );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
        } else {
            outline0("LDA TMPPTR" );
            outline1("STA %s", _register );
            outline0("LDA TMPPTR+1" );
            outline1("STA %s", address_displacement(_environment, _register, "1") );
        }

    }

}


void vic2_put_image( Environment * _environment, Resource * _image, char * _x, char * _y, char * _frame, char * _sequence, int _frame_size, int _frame_count, char * _flags ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( putimage, src_hw_vic2_put_image_asm );

    MAKE_LABEL

    vic2_load_image_address_to_register( _environment, "TMPPTR", _image, _sequence, _frame, _frame_size, _frame_count );

    outline1("LDA %s", _x );
    outline0("STA IMAGEX" );
    outline1("LDA %s", address_displacement(_environment, _x, "1") );
    outline0("STA IMAGEX+1" );
    outline1("LDA %s", _y );
    outline0("STA IMAGEY" );
    outline1("LDA %s", address_displacement(_environment, _y, "1") );
    outline0("STA IMAGEY+1" );
    outline1("LDA %s", _flags );
    outline0("STA IMAGEF" );
    outline1("LDA %s", address_displacement(_environment, _flags, "1") );
    outline0("STA IMAGET" );

    outline0("JSR PUTIMAGE");

}

void vic2_calculate_sequence_frame_offset( Environment * _environment, char * _offset, char * _sequence, char * _frame, int _frame_size, int _frame_count ) {

    outline0("LDA #0");
    outline1("STA %s", _offset );
    outline1("STA %s", address_displacement(_environment, _offset, "1") );

    if ( _sequence ) {

        outline0("CLC" );
        outline1("LDA %s", _offset );
        outline0("ADC #3" );
        outline1("STA %s", _offset );
        outline1("LDA %s", address_displacement(_environment, _offset, "1") );
        outline0("ADC #0" );
        outline1("STA %s", address_displacement(_environment, _offset, "1") );
        if ( strlen(_sequence) == 0 ) {

        } else {
            outline1("LDA #<OFFSETS%4.4x", _frame_size * _frame_count );
            outline0("STA MATHPTR0" );
            outline1("LDA #>OFFSETS%4.4x", _frame_size * _frame_count );
            outline0("STA MATHPTR1" );
            outline0("CLC" );
            outline1("LDA %s", _sequence );
            outline0("ASL" );
            outline0("TAY" );
            outline1("LDA %s", _offset );
            outline0("ADC (MATHPTR0), Y" );
            outline1("STA %s", _offset );
            outline0("INY" );
            outline1("LDA %s", address_displacement(_environment, _offset, "1") );
            outline0("ADC (MATHPTR0), Y" );
            outline1("STA %s", address_displacement(_environment, _offset, "1") );
        }

        if ( _frame ) {
            if ( strlen(_frame) == 0 ) {

            } else {
                outline1("LDA #<OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0" );
                outline1("LDA #>OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR1" );
                outline0("CLC" );
                outline1("LDA %s", _frame );
                outline0("ASL" );
                outline0("TAY" );
                outline1("LDA %s", _offset );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", _offset );
                outline0("INY" );
                outline1("LDA %s", address_displacement(_environment, _offset, "1") );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", address_displacement(_environment, _offset, "1") );
            }
        }

    } else {

        if ( _frame ) {
            outline0("CLC" );
            outline1("LDA %s", _offset );
            outline0("ADC #3" );
            outline1("STA %s", _offset );
            outline1("LDA %s", address_displacement(_environment, _offset, "1") );
            outline0("ADC #0" );
            outline1("STA %s", address_displacement(_environment, _offset, "1") );
            if ( strlen(_frame) == 0 ) {

            } else {
                outline1("LDA #<OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0" );
                outline1("LDA #>OFFSETS%4.4x", _frame_size );
                outline0("STA MATHPTR0+1" );
                outline0("CLC" );
                outline1("LDA %s", _frame );
                outline0("ASL" );
                outline0("TAY" );
                outline1("LDA %s", _offset );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", _offset );
                outline0("INY" );
                outline1("LDA %s", address_displacement(_environment, _offset, "1") );
                outline0("ADC (MATHPTR0), Y" );
                outline1("STA %s", address_displacement(_environment, _offset, "1") );
            }
        }

    }

}

void vic2_blit_image( Environment * _environment, char * _sources[], int _source_count, char * _blit, char * _x, char * _y, char * _frame, char * _sequence, int _frame_size, int _frame_count, int _flags ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( blitimage, src_hw_vic2_blit_image_asm );

    if ( _source_count > 2 ) {
        CRITICAL_BLIT_TOO_MUCH_SOURCES( );
    }

    MAKE_LABEL

    outhead1("blitimage%s:", label);

    outline1("LDA #<%s", _blit );
    outline0("STA BLITIMAGEBLITADDR+1" );
    outline1("LDA #>%s", _blit );
    outline0("STA BLITIMAGEBLITADDR+2" );

    if ( _source_count > 0 ) {
        Resource resource;
        resource.realName = strdup( _sources[0] );
        resource.type = VT_IMAGE;
        resource.isAddress = 0;
        vic2_load_image_address_to_register( _environment, "BLITTMPPTR", &resource, _sequence, _frame, _frame_size, _frame_count );
    } else {
        outline0( "LDA #$0" );
        outline0( "STA BLITTMPPTR" );
        outline0( "STA BLITTMPPTR+1" );
    }

    if ( _source_count > 1 ) {
        Resource resource;
        resource.realName = strdup( _sources[1] );
        resource.type = VT_IMAGE;
        resource.isAddress = 0;
        vic2_load_image_address_to_register( _environment, "BLITTMPPTR2", &resource, _sequence, _frame, _frame_size, _frame_count );
    } else {
        outline0( "LDA #$0" );
        outline0( "STA BLITTMPPTR2" );
        outline0( "STA BLITTMPPTR2+1" );
    }

    outhead1("putimage%s:", label);

    outline1("LDA %s", _x );
    outline0("STA IMAGEX" );
    outline1("LDA %s", address_displacement(_environment, _x, "1") );
    outline0("STA IMAGEX+1" );
    outline1("LDA %s", _y );
    outline0("STA IMAGEY" );
    outline1("LDA %s", address_displacement(_environment, _y, "1") );
    outline0("STA IMAGEY+1" );
    outline1("LDA #$%2.2x", ( _flags & 0xff ) );
    outline0("STA IMAGEF" );
    outline1("LDA #$%2.2x", ( (_flags>>8) & 0xff ) );
    outline0("STA IMAGET" );

    outline0("JSR BLITIMAGE");

}

void vic2_wait_vbl( Environment * _environment ) {

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( vbl, src_hw_vic2_vbl_asm);

    outline0("JSR VBL");

}

Variable * vic2_new_image( Environment * _environment, int _width, int _height, int _mode ) {

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );

    int size = calculate_image_size( _environment, _width, _height, _mode );

    if ( ! size ) {
        CRITICAL_NEW_IMAGE_UNSUPPORTED_MODE( _mode );
    }

    Variable * result = variable_temporary( _environment, VT_IMAGE, "(new image)" );

    char * buffer = malloc ( size );
    memset( buffer, 0, size );

    *(buffer) = (_width & 0xff);
    *(buffer+1) = (_width>>8) & 0xff;
    *(buffer+2) = _height;

    result->valueBuffer = buffer;
    result->size = size;
    
    return result;

}

Variable * vic2_new_images( Environment * _environment, int _frames, int _width, int _height, int _mode ) {

    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );

    int size = calculate_images_size( _environment, _frames, _width, _height, _mode );
    int frameSize = calculate_image_size( _environment, _width, _height, _mode );

    if ( ! size ) {
        CRITICAL_NEW_IMAGES_UNSUPPORTED_MODE( _mode );
    }

    Variable * result = variable_temporary( _environment, VT_IMAGES, "(new images)" );

    char * buffer = malloc ( size );
    memset( buffer, 0, size );

    *(buffer) = _frames;
    *(buffer+1) = ( _width & 0xff );
    *(buffer+2) = ( _width >> 8 ) & 0xff;
    for( int i=0; i<_frames; ++i ) {
        *(buffer+3+(i*frameSize)) = ( _width & 0xff );
        *(buffer+3+(i*frameSize)+1) = ( ( _width >> 8 ) & 0xff );
        *(buffer+3+(i*frameSize)+2) = ( _height & 0xff );
    }

    result->valueBuffer = buffer;
    result->frameSize = frameSize;
    result->size = size;
    
    return result;

}

void vic2_get_image( Environment * _environment, char * _image, char * _x, char * _y, char * _frame, char * _sequence, int _frame_size, int _frame_count, int _palette ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( vic2varsGraphic, src_hw_vic2_vars_graphic_asm );
    deploy( getimage, src_hw_vic2_get_image_asm );

    MAKE_LABEL

    vic2_load_image_address_to_other_register( _environment, "TMPPTR", _image, _sequence, _frame, _frame_size, _frame_count );

    outline1("LDA %s", _x );
    outline0("STA IMAGEX" );
    outline1("LDA %s", address_displacement(_environment, _x, "1") );
    outline0("STA IMAGEX+1" );
    outline1("LDA %s", _y );
    outline0("STA IMAGEY" );
    outline1("LDA %s", address_displacement(_environment, _y, "1") );
    outline0("STA IMAGEY+1" );
    outline1("LDA #$%2.2x", _palette );
    outline0("STA IMAGET" );

    outline0("JSR GETIMAGE");

}

void vic2_scroll( Environment * _environment, int _dx, int _dy ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( scroll, src_hw_vic2_scroll_asm);
    deploy( textHScroll, src_hw_vic2_hscroll_text_asm );
    deploy( vScrollTextDown, src_hw_vic2_vscroll_text_down_asm );
    deploy( vScrollTextUp, src_hw_vic2_vscroll_text_up_asm );

    outline1("LDA #$%2.2x", (unsigned char)(_dx&0xff) );
    outline0("STA MATHPTR0" );
    outline1("LDA #$%2.2x", (unsigned char)(_dy&0xff) );
    outline0("STA MATHPTR1" );
    outline0("JSR SCROLL");

}

void vic2_put_tile( Environment * _environment, char * _tile, char * _x, char * _y ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( tiles, src_hw_vic2_tiles_asm );

    outline1("LDA %s", _tile );
    outline0("STA TILET" );
    outline1("LDA %s", _x );
    outline0("STA TILEX" );
    outline1("LDA %s", _y );
    outline0("STA TILEY" );
    outline0("LDA #1" );
    outline0("STA TILEW" );
    outline0("STA TILEH" );
    outline0("STA TILEW2" );
    outline0("STA TILEH2" );

    outline0("JSR PUTTILE");

}

void vic2_move_tiles( Environment * _environment, char * _tile, char * _x, char * _y ) {

    Variable * tile = variable_retrieve( _environment, _tile );
    Variable * x = variable_retrieve( _environment, _x );
    Variable * y = variable_retrieve( _environment, _y );

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( tiles, src_hw_vic2_tiles_asm );

    outline1("LDA %s", tile->realName );
    outline0("STA TILET" );
    outline1("LDA %s", x->realName );
    outline0("STA TILEX" );
    outline1("LDA %s", y->realName );
    outline0("STA TILEY" );
    outline1("LDA %s", address_displacement(_environment, tile->realName, "1") );
    outline0("STA TILEW" );
    outline0("STA TILEW2" );
    outline1("LDA %s", address_displacement(_environment, tile->realName, "2") );
    outline0("STA TILEH" );
    outline0("STA TILEH2" );
    outline1("LDA %s", address_displacement(_environment, tile->realName, "3") );
    outline0("STA TILEA" );

    int size = ( tile->originalWidth >> 3 ) * ( tile->originalHeight >> 3 );

    if ( size ) {
        outline1("LDA #<OFFSETS%4.4x", size );
        outline0("STA TMPPTR2" );
        outline1("LDA #>OFFSETS%4.4x", size );
        outline0("STA TMPPTR2+1" );
    } else {
        outline0("LDA #0" );
        outline0("STA TMPPTR2" );
        outline0("STA TMPPTR2+1" );
    }

    outline0("JSR MOVETILE");

}

void vic2_put_tiles( Environment * _environment, char * _tile, char * _x, char * _y, char *_w, char *_h ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( tiles, src_hw_vic2_tiles_asm );

    outline1("LDA %s", _tile );
    outline0("STA TILET" );
    outline1("LDA %s", _x );
    outline0("STA TILEX" );
    outline1("LDA %s", _y );
    outline0("STA TILEY" );
    outline1("LDA %s", address_displacement(_environment, _tile, "1") );
    outline0("STA TILEW" );
    if ( _w ) {
        outline1("LDA %s", _w );
    }
    outline0("STA TILEW2" );
    outline1("LDA %s", address_displacement(_environment, _tile, "2") );
    outline0("STA TILEH" );
    if ( _h ) {
        outline1("LDA %s", _h );
    }
    outline0("STA TILEH2" );

    outline0("JSR PUTTILE");

}

void vic2_tile_at( Environment * _environment, char * _x, char * _y, char * _result ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( tiles, src_hw_vic2_tiles_asm );

    outline1("LDA %s", _x );
    outline0("STA TILEX" );
    outline1("LDA %s", _y );
    outline0("STA TILEY" );

    outline0("JSR TILEAT");

    outline0("LDA TILET" );
    outline1("STA %s", _result );

}

void vic2_use_tileset( Environment * _environment, char * _tileset ) {

    deploy( vic2vars, src_hw_vic2_vars_asm);
    deploy( tiles, src_hw_vic2_tiles_asm );

    outline1("LDA %s", _tileset );

    outline0("JSR USETILESET");

}

Variable * vic2_get_raster_line( Environment * _environment ) {

    Variable * result = variable_temporary( _environment, VT_WORD, "(raster line)" );

    outline0( "LDA $D012" );
    outline1( "STA %s", result->realName );
    outline0( "LDA $D011" );
    outline0( "ROL" );
    outline0( "ROL" );
    outline0( "AND #$01" );
    outline1( "STA %s", address_displacement(_environment, result->realName, "1") );

    return result;
    
}

void vic2_slice_image( Environment * _environment, char * _image, char * _frame, char * _sequence, int _frame_size, int _frame_count, char * _destination ) {

}

int vic2_palette_extract( Environment * _environment, char * _data, int _width, int _height, int _depth, int _flags, RGBi * _palette ) {

    int paletteColorCount = rgbi_extract_palette(_environment, _data, _width, _height, _depth, _palette, MAX_PALETTE, ( ( _flags & FLAG_EXACT ) ? 0 : 1 ) /* sorted */);

    memcpy( _palette, palette_match( _palette, paletteColorCount, SYSTEM_PALETTE, sizeof(SYSTEM_PALETTE) / sizeof(RGBi) ), paletteColorCount * sizeof( RGBi ) );

    int uniquePaletteCount = 0;

    memcpy( _palette, palette_remove_duplicates( _palette, paletteColorCount, &uniquePaletteCount ), paletteColorCount * sizeof( RGBi ) );

    return uniquePaletteCount;

}

#endif