# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 9+

global defs
 const FALSE := 0
 const TRUE := 1

 const TK_NULL := 0
 const TK_COMMENT := 1
 const TK_SYMBOL := 2
 const TK_IDENTIFIER := 3
 const TK_INTEGER := 4
 const TK_FLOAT := 5

 const K_TAB := 8
 const K_LF := 10
 const K_CR := 13
 const K_SPC := 32

 const F_CELLS := "cells.dat"
 const F_GRAPH_G := "graph_g.dat"
 const F_GRAPH_H := "graph_h.dat"

 type TStr = string [ 256 ]
 body defs ; end ;
