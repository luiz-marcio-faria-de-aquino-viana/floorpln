# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 15+

global InitCells
 import Param

 const F_CELLS := "cells.dat"

 type TInstance = rec (
 Width : real ,
 Height : real ,
 Next : ptr TInstance )

 type TCell = rec (
 CellId : TStr ,
 CellOrd : int ,
 NInstances : int ,
 Instances : ptr TInstance ,
 Next : ptr TCell )

 type TCellsList = rec (
 NCells : int ,
 FirstCell : ptr TCell ,
 LastCell : ptr TCell )

 var CellsList : TCellsList

 op CellDim ( PtrCell : ptr TCell ; Pos : int ; Mode : int ) returns R : real
 op DisplayCells ( )

 body InitCells ; end ;
