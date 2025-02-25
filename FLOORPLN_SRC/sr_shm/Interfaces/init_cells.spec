# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 10+

global init_cells
 import param

 const F_CELLS := "cells.dat"

 type TInstance = rec (
 Width : real ,
 Height : real ,
 Next : ptr TInstance )

 type TCell = rec (
 CellId : TStr ,
 NInstances : int ,
 Instances : ptr TInstance ,
 Next : ptr TCell )

 type TCellsList = rec (
 NCells : int ,
 FirstCell : ptr TCell ,
 LastCell : ptr TCell )

 var CellsList : TCellsList
 body init_cells ; end ;
