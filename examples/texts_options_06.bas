    t$ = PEN$(YELLOW)+"YELLOW ON "+PAPER$(BLUE)+" BLUE"

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING REPLACE, NORMAL
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING REPLACE, PAPER
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING REPLACE, PEN
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING OR, NORMAL
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING OR, PAPER
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING OR, PEN
    PRINT t$
    WAIT 500 MS
    
    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING XOR, NORMAL
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING XOR, PAPER
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING XOR, PEN
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING AND, NORMAL
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING AND, PAPER
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING AND, PEN
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING IGNORE, NORMAL
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING IGNORE, PAPER
    PRINT t$
    WAIT 500 MS

    PEN DEFAULT PEN
    PAPER DEFAULT PAPER
    WRITING IGNORE, PEN
    PRINT t$
    WAIT 500 MS

    HALT