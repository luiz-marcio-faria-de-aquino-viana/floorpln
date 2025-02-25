# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 34+

global init
 import defs

 type TInstance = rec (
 W : real ,
 H : real )

 type TCell = rec (
 CellId : ptr char ,
 NInstances : int ,
 Instances : ptr TInstance ,
 Next : ptr TCell )

 type TCellsList = rec (
 NCells : int ,
 FirstCell : ptr TCell ,
 LastCell : ptr TCell )

 body init ; end ;
