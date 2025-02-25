# 0 /home/lmarcio/floorpln/sr_shm/floorpln.sr 94+

global init_graphs
 import param , init_cells

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
 body init_graphs ; end ;
