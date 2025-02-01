( Initialize D32 )
\ 32 OUTPUT pinMode

\ 32 HIGH digitalWrite
\ 1000 sleep
\ 32 LOW digitalWrite
\ 1000 sleep

( : is a compile-time construct for defining words )
\ : SQUARE
\     DUP *
\ ;

\ : LTE-FIVE
\     DUP 5 < IF
\         ." LT 5" CR
\     ELSE
\         ." GTE 5" CR
\     THEN
\     DROP
\ ;


( DO..LOOP is a compile-time construct for loops )
\ : COUNT-UP
\     0 10 DO
\         I .
\     LOOP
\ ;


( ." is a compile-time word for printing things )
\ : GREETING
\     ." Hello, World!" CR
\ ;

