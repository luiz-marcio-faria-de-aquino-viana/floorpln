# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 317+

global Shared
 import Param , InitCells , InitGraphs

 const N : int := CellsList . NCells

 var Choice : rec (
 Area : real
 Width : real
 Height : real
 )

 var SSet [ N ] : int

 var WSet [ N ] : int

 var Terminated : int := 0

 op SelectNextSet ( ref CSet [ * ] : int ) returns R : int
 op UpdateChoice ( CArea : real ; CWidth : real ; CHeight : real ; CSet [ * ] : int )

 body Shared ; end ;
