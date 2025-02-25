# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 144+

global InitGraphs
 import Param , InitCells

 const F_GRAPH_G := "graph_g.dat"
 const F_GRAPH_H := "graph_h.dat"

 type TNode = rec (
 NodeId : TStr ,
 NEdges : int ,
 Edges : ptr TEdge ,
 Next : ptr TNode )

 type TEdge = rec (
 CellPt : ptr TCell ,
 DstNodePt : ptr TNode ,
 Next : ptr TEdge )

 type TGraph = rec (
 NNodes : int ,
 First : ptr TNode ,
 Last : ptr TNode )

 var GraphG : TGraph
 var GraphH : TGraph

 op DisplayGraph ( Graph : TGraph )
 body InitGraphs ; end ;
