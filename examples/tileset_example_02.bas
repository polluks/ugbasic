﻿REM @english
REM TILESET SUPPORT NAMED LOADING
REM
REM This example shows how to load and use a named tiles from tileset 
REM created with a standard tool like "Tiled". The tiles can be indexed
REM by their "class" (type) name. Tileset courtesy of Kenney.
REM
REM For more informations:
REM https://www.mapeditor.org/
REM https://kenney.nl/
REM
REM @italian
REM GESTIONE DEI TILESET CARICAMENTO
REM
REM Questo esempio mostra come si caricano e si utilizzano le tile
REM nominali di un tileset creato con un tool standard come "Tiled".
REM In generale, le tile possono essere indicate anche dal loro
REM nome (il campo "class" o "type"). Il tileset courtesy of Kenney.
REM
REM Per maggiori informazioni:
REM https://www.mapeditor.org/
REM https://kenney.nl/

BITMAP ENABLE(16)

tileset := LOAD TILESET("kenney-tiny-town.tsx")

x = (POSITION) 0
y = (POSITION) 0

FOR i=0 TO 32
	PUT IMAGE tileset FRAME i AT x,y
	x = x + 16
	IF ( x + 16 ) > ( SCREEN WIDTH ) THEN
		x = 0
		y = y + 16
	ENDIF
NEXT




