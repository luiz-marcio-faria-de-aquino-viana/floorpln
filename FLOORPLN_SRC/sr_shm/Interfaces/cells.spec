# 0 /home/lmarcio/floorpln/par_sr/floorpln.sr 2+

global cells
 type TStr = string [ 256 ]

 type TInstance = rec (
 Width : real
 Height : real )

 type TCell = rec (
 CellId : TStr
 NumInstance : int
 InstanceList : ptr TInstance
 NextCell : ptr TCell )

 const FILE_CELLS := "cells.dat"

 var CellsList : ptr TCell

 body cells ; end ;
