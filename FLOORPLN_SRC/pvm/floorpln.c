
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include"pvm3.h"

#define DEBUG

#define PVM_GROUP         "floorplan"

#define PVM_MSG_REQWKSET  1000
#define PVM_MSG_SNDWKSET  1010
#define PVM_MSG_REDUCE    1020
#define PVM_MSG_REQSELC   1030
#define PVM_MSG_SNDSELC   1040

#define TK_NULL           0
#define TK_COMMENT        1
#define TK_SYMBOL         2
#define TK_IDENTIFIER     3
#define TK_INTEGER        4
#define TK_FLOAT          5

#define SUM_WIDTH         0
#define SUM_HEIGHT        1

#define MAX_AREA          ((double) 1.0e99)

#define F_CELLS           "/home/lmarcio/PVM_ROOT/cells.dat"
#define F_GRAPH_G         "/home/lmarcio/PVM_ROOT/graph_g.dat"
#define F_GRAPH_H         "/home/lmarcio/PVM_ROOT/graph_h.dat"

#define ACT_SAVE    0
#define ACT_NOSAVE  1

#define K_TAB       8
#define K_LF       10
#define K_CR       13
#define K_SPC      32

#define L(c)       ( ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) )
#define N(c)       ((c >= '0') && (c <= '9'))
#define SPC(c)     ((c == K_TAB) || (c == K_CR) || (c == K_LF) || (c == K_SPC))
#define SYM(c)     (!SPC(c) && !L(c) && !N(c))

#define STRSZ(s) (strlen(s) + 1)

#define MIN_NPROCS        2
#define MAX_NPROCS       20

#define PARM_NPROC       "-np"

#define MIN(x, y)        ( (x < y) ? x : y )

typedef char str_t[256];

struct parm_t {
        int nproc;
};

struct dimension_t {
        int    ctid;
        double cwidth;
        double cheight;
        double carea;
};

struct process_t {
        int pid;
        int inum;
        int nproc;
        struct dimension_t cdim;
        int *current;
        int *selected;
        int *tids;
        int terminated;
};

struct instance_t {
        double width;
	double height;
};

struct cell_t {
        char *id;
        int ord;
        int n;
	int current;
	struct instance_t *instances;
	struct cell_t *next;
};

struct celllist_t {
        int n;
        struct cell_t *top;
	struct cell_t *botton;
};

struct node_t {
        char *id;
        int n;
	struct edge_t *edges;
        struct node_t *next;
};

struct edge_t {
        struct cell_t *cell;
	struct node_t *dest;
        struct edge_t *next;
};

struct graph_t {
        struct node_t *first;
        struct node_t *last;
};

struct parm_t     parm = { MIN_NPROCS };

struct celllist_t cellslist = { 0, NULL, NULL };

struct graph_t    graph_g = { NULL };
struct graph_t    graph_h = { NULL };

extern void MinimalArea();

void GetParm(int argc, char *argv[]);

int IsNull(char *s);
int IsComment(char *s);

int GetToken(char *tk, char **s);

void AddInstance(struct cell_t *cell, int pos, double width, double height);

struct cell_t *SearchCell(char *id);
struct cell_t *AddCell(char *id, int n);

void ProcessCellDef(char *s);
void RetrieveCells();

struct edge_t *AddLink(struct node_t *origin, struct cell_t *cell, struct node_t *dest);

struct node_t *SearchNode(struct graph_t *p, char *id);
struct node_t *AddNode(struct graph_t *p, char *id);

void ProcessNodeDef(struct graph_t *p, char *s);
void RetrieveGraph(struct graph_t *p, char *fname);

int SelectNextSet(struct process_t *proc);
int GetNextSet(struct process_t *proc);

void EvaluateDimension(struct node_t *org, int *cset, double val, double *cval, int mode);
void EvaluateArea(struct process_t *proc);

void Schedule(struct process_t *proc);

void DisplayResult(struct process_t *proc);

#ifdef DEBUG
void DisplayCells();
void DisplayGraph(struct graph_t *p);
void DisplayCurrentSet(int *cset);
#endif

int main(int argc, char *argv[])
{
        struct process_t proc;
	int numt;

#ifdef DEBUG
	if(pvm_parent() == PvmNoParent) {
	        printf("<DBG(%d)> Starting output catch service.\n", time(NULL));
	        pvm_catchout(stdout);
		pvm_setopt(PvmShowTids, PvmRouteDirect);
	}
#endif

        GetParm(argc, argv);

        RetrieveCells();

	RetrieveGraph(&graph_g, F_GRAPH_G);
	RetrieveGraph(&graph_h, F_GRAPH_H);

	proc.pid = pvm_mytid();

	if((proc.inum = pvm_joingroup(PVM_GROUP)) < 0) {
                pvm_perror("ERR: Could not join to the group.\n");
		pvm_exit();
		exit(1);
	}

        proc.nproc = parm.nproc;

 	if((proc.tids = (int *) malloc(proc.nproc * sizeof(int))) == NULL) {
                printf("ERR: Can't allocate memory.\n");
	        exit(1);
        }

	if((proc.current = (int *) malloc(cellslist.n * sizeof(int))) == NULL) {
                printf("ERR: Can't allocate memory.\n");
		exit(1);
        }

	if((proc.selected = (int *) malloc(cellslist.n * sizeof(int))) == NULL) {
                printf("ERR: Can't allocate memory.\n");
		exit(1);
        }

        proc.cdim.ctid = proc.pid;
	proc.cdim.cwidth = 0;
	proc.cdim.cheight = 0;
	proc.cdim.carea = MAX_AREA;

	proc.terminated = 0;

	if(pvm_parent() == PvmNoParent) {
#ifdef DEBUG
	        printf("<DBG(%d)> Starting master task t%x.\n", time(NULL), proc.pid);
#endif
	        numt = pvm_spawn("PVM_ROOT/floorpln", argv, 0, "", proc.nproc - 1, proc.tids);
	        proc.nproc = MIN(proc.nproc, numt + 1);
#ifdef DEBUG
	        printf("<DBG(%d)> Started %d processes of %d requested.\n", time(NULL), proc.nproc, parm.nproc);
#endif
	} else {
#ifdef DEBUG
	        printf("<DBG(%d)> Starting slave task t%x.\n", time(NULL), proc.pid);
#endif
	}

	pvm_freezegroup(PVM_GROUP, proc.nproc);
#ifdef DEBUG
	printf("<DBG(%d)> Task t%x is on the first barrier.\n", time(NULL), proc.pid);
#endif
        pvm_barrier(PVM_GROUP, proc.nproc);
#ifdef DEBUG
	printf("<DBG(%d)> Task t%x pass throwght the first barrier.\n", time(NULL), proc.pid);
#endif

	if(pvm_parent() == PvmNoParent) {
#ifdef DEBUG
        printf("<DBG(%d)> Master task t%x is begining its process...\n", time(NULL), proc.pid);
#endif
	        Schedule(&proc);
	} else {
#ifdef DEBUG
        printf("<DBG(%d)> Slave task t%x is begining its process...\n", time(NULL), proc.pid);
#endif
	        EvaluateArea(&proc);
	}

#ifdef DEBUG
	        printf("<DBG(%d)> Task t%x is processing the reduction...\n", time(NULL), proc.pid);
#endif
	pvm_reduce(MinimalArea, &proc, sizeof(struct process_t), PVM_BYTE, PVM_MSG_REDUCE, PVM_GROUP, 0);

	if(pvm_parent() == PvmNoParent) {
#ifdef DEBUG
	        printf("<DBG(%d)> Master task t%x got the minimal area value.\n", time(NULL), proc.pid);
#endif
		pvm_initsend(PvmDataDefault);
		pvm_pkint(&(proc.cdim).ctid, 1, 1);
                pvm_mcast(proc.tids, proc.nproc - 1, PVM_MSG_REQSELC);

		pvm_recv((proc.cdim).ctid, PVM_MSG_SNDSELC);
		pvm_upkint(proc.selected, cellslist.n, 1);

                DisplayResult(&proc);
        }
	else {
	        pvm_recv(pvm_parent(), PVM_MSG_REQSELC);
                pvm_upkint(&(proc.cdim).ctid, 1, 1);

		if(proc.pid == (proc.cdim).ctid) {
                        pvm_initsend(PvmDataDefault);
			pvm_pkint(proc.selected, cellslist.n, 1);
			pvm_send(pvm_parent(), PVM_MSG_SNDSELC);
                }
        }

	pvm_lvgroup(PVM_GROUP);
	pvm_exit();

	return 0;
}

void GetParm(int argc, char *argv[])
{
        str_t s;
        int i;

	for(i = 1; i < argc; i++) {
	        if( !strncasecmp(argv[i], PARM_NPROC, strlen(PARM_NPROC)) ) {
                        strcpy(s, &argv[i][strlen(PARM_NPROC)]);
			parm.nproc = atoi(s);
			if(parm.nproc < MIN_NPROCS) {
			        printf("ERR: Number of processors must be higher or equal %d.\n", MIN_NPROCS);
                                exit(1);
                        } else if(parm.nproc > MAX_NPROCS) {
                                printf("ERR: Number of processors must be belower or equal %d.\n", MAX_NPROCS);
                                exit(1);
                        }
		}
	}
}

int IsNull(char *s)
{
        str_t token;
	if(GetToken(token, &s) == TK_NULL) return 1;
	return 0;
}

int IsComment(char *s)
{
        str_t token;
	if(GetToken(token, &s) == TK_COMMENT) return 1;
	return 0;
}

int GetToken(char *tk, char **s)
{
        int cstage = 1;
        char *p = (*s);

	int tk_type = TK_NULL; 

	while((*p) != '\0') {
	        switch(cstage) {
		        case 1 :
			        if( L((*p)) ) {
				        cstage = 2;
					(*tk++) = (*p++);
					tk_type = TK_IDENTIFIER;
                                } else if( N((*p)) ) {
                                        cstage = 3;
                                        (*tk++) = (*p++);
					tk_type = TK_INTEGER;
                                } else if((*p) == '.') {
                                        cstage = 4;
					(*tk++) = (*p++);
					tk_type = TK_FLOAT;
				} else if((*p) == '#') {
                                        (*tk++) = (*p++);
					while((*p) != '\0') p++;
					(*s) = p;
					tk_type = TK_COMMENT;
                                } else if(SYM((*p)) && ((*p) != '.') && ((*p) != '#')) {
                                        (*tk++) = (*p++);
					(*tk) = '\0';
					(*s) = p;
					tk_type = TK_SYMBOL;
                                } else {
                                        p++;
			        }
                                break;
		        case 2 :
			        if( L((*p)) || N((*p)) ) {
				        (*tk++) = (*p++);
			        } else {
                                        (*tk) = '\0';
					(*s) = p;
                                        return tk_type;
                                }
                                break;
                        case 3 :
			        if( N((*p)) ) {
                                        (*tk++) = (*p++);
			        } else if((*p) == '.') {
                                        cstage = 4;
                                        (*tk++) = (*p++);
                                        tk_type = TK_FLOAT;
                                } else {
                                        (*tk) = '\0';
                                        (*s) = p;
                                        return tk_type;
                                }
                                break;
		        case 4 :
			        if( N((*p)) ) {
                                        (*tk++) = (*p++);
                                } else {
                                        (*tk) = '\0';
                                        (*s) = p;
                                        return tk_type;
                                }
	        }
        }
	return tk_type;
}

void AddInstance(struct cell_t *cell, int pos, double width, double height)
{
        (cell->instances)[pos].width = width;
	(cell->instances)[pos].height = height;
}

struct cell_t *SearchCell(char *id)
{
        struct cell_t *cell = cellslist.top;
	while(cell != NULL) {
	        if( !strcmp(cell->id, id) ) return cell;
		cell = cell->next;
	}
	return NULL;
}

struct cell_t *AddCell(char *id, int n)
{
        struct cell_t *cell;

	if((cell = (struct cell_t *) malloc(sizeof(struct cell_t))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
	        exit(1);
	}
	    
	if((cell->id = (char *) malloc(STRSZ(id))) == NULL) {
		printf("ERR: Can't allocate memory.\n");
		exit(1);
	}

	if((cell->instances = (struct instance_t *) malloc(n * sizeof(struct instance_t))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
	        exit(1);
	}

	strcpy(cell->id, id);
	cell->ord       = cellslist.n;
        cell->n         = n;
	cell->current   = 0;
	cell->next      = NULL;

        if(cellslist.top == NULL)    cellslist.top = cell;
        if(cellslist.botton != NULL) (cellslist.botton)->next = cell;
	cellslist.botton = cell;
	cellslist.n += 1;

	return cell;
}

void ProcessCellDef(char *s)
{
        str_t cellid, token;
	int tk_type;

	double width, height;
	int n;

	int i;

        struct cell_t *cell;

        if(GetToken(cellid, &s) != TK_IDENTIFIER) {
                printf("ERR: Expected an identifier: %s\n", s);
                exit(1);
	}

	if((cell = SearchCell(token)) != NULL) {
	        printf("ERR: Duplicated identifier: %s\n", s);
		exit(1);
	}

	if(GetToken(token, &s) != TK_INTEGER) {
	        printf("ERR: Expected an integer value: %s\n", s); 
	        exit(1);
        }
	n = atoi(token);

	cell = AddCell(cellid, n);


	for(i = 0; i < n; i++) {
	        if(((tk_type = GetToken(token, &s)) != TK_FLOAT) && (tk_type != TK_INTEGER)) {
		        printf("ERR: Expected a numerical value: %s\n", s);
			exit(1);
		}
		width = atof(token);

		if(((tk_type = GetToken(token, &s)) != TK_FLOAT) && (tk_type != TK_INTEGER)) {
		        printf("ERR: Expected a numerical value: %s\n", s);
			exit(1);
		}
		height = atof(token);

	        AddInstance(cell, i, width, height);
        }
}

void RetrieveCells()
{
        FILE *fp;
	str_t s;

	if((fp = fopen(F_CELLS, "r")) == NULL) {
	        printf("ERR: Can't open cells' definition file.\n");
	        exit(1); 
	}

	while(fgets(s, sizeof(str_t), fp) != NULL) {
	        s[sizeof(str_t) - 1] = '\0';
	        if( !IsNull(s) && !IsComment(s) )
                        ProcessCellDef(s);
	}

	fclose(fp);
}

struct edge_t *AddLink(struct node_t *origin, struct cell_t *cell, struct node_t *dest)
{
        struct edge_t *p;

	if((p = (struct edge_t *) malloc(sizeof(struct edge_t))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
		exit(1);
	}

	p->cell = cell;
	p->dest = dest;
	
	p->next = origin->edges;
        origin->edges = p;

	return p;
}

struct node_t *SearchNode(struct graph_t *p, char *id)
{
        struct node_t *node = p->first;
	while(node != NULL) {
	        if( !strcmp(node->id, id) ) return node;
		node = node->next;
	}
	return NULL;
}

struct node_t *AddNode(struct graph_t *p, char *id)
{
        struct node_t *node;

	if((node = (struct node_t *) malloc(sizeof(struct node_t))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
	        exit(1);
	}

	if((node->id = (char *) malloc(STRSZ(id))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
	        exit(1);
	}

	strcpy(node->id, id);
	node->n = 0;
	node->edges = NULL;
	node->next = NULL;

	if(p->first == NULL) p->first = node;
	if(p->last != NULL) (p->last)->next = node;
        p->last = node;

	return node;
}

void ProcessNodeDef(struct graph_t *p, char *s)
{
        str_t token;

	struct node_t *origin;

	struct cell_t *cell;
	struct node_t *dest;

	int i;

        if(GetToken(token, &s) != TK_IDENTIFIER) {
                printf("ERR: Expected an identifier: %s\n", s);
                exit(1);
	}

	if((origin = SearchNode(p, token)) == NULL)
	        origin = AddNode(p, token);

	if(GetToken(token, &s) != TK_INTEGER) {
	        printf("ERR: Expected an integer value: %s\n", s); 
	        exit(1);
        }

	origin->n = atoi(token);

	for(i = 0; i < origin->n; i++) {
	        if(GetToken(token, &s) != TK_IDENTIFIER) {
                        printf("ERR: Expected an identifier: %s\n", s);
                        exit(1);
		}
		
		if((cell = SearchCell(token)) == NULL) {
		        printf("ERR: Cell identifier not defined: %s\n", token);
			exit(1);
		}

	        if(GetToken(token, &s) != TK_IDENTIFIER) {
                        printf("ERR: Expected an identifier: %s\n", s);
                        exit(1);
		}
		
		if((dest = SearchNode(p, token)) == NULL)
		        dest = AddNode(p, token);

		AddLink(origin, cell, dest);
	}
}

void RetrieveGraph(struct graph_t *p, char *fname)
{
        FILE *fp;
	str_t s;

	if((fp = fopen(fname, "r")) == NULL) {
	        printf("ERR: Can't open graph's topology file.\n");
	}

	while(fgets(s, sizeof(str_t), fp) != NULL) {
	        s[sizeof(str_t) - 1] = '\0';
	        if( !IsNull(s) && !IsComment(s) )
                        ProcessNodeDef(p, s);
	}
}

int SelectNextSet(struct process_t *proc)
{
        struct cell_t *cell;
	int flag = 1;

	int *cset = proc->current;

        cell = cellslist.top;
        while(cell != NULL) {
	        (*cset) = cell->current;
		if( flag ) {
	                cell->current += 1;
		        if(cell->current < cell->n)
                                flag = 0;
                        else
		                cell->current = 0;
                }
		cell = cell->next;
		cset++;
	}

	return !flag;
}

void Schedule(struct process_t *proc)
{
        int tid;
	int i;

        while( SelectNextSet(proc) ) {
                pvm_recv(-1, PVM_MSG_REQWKSET);
	        pvm_upkint(&tid, 1, 1);
#ifdef DEBUG
	        printf("<DBG(%d)> Working set requested from task t%x.\n", time(NULL), tid);
                printf("<DBG(%d)> Master task t%x is sending the working set.\n", time(NULL), proc->pid);
	        DisplayCurrentSet(proc->current);
#endif
	        pvm_initsend(PvmDataDefault);
		pvm_pkint(&proc->terminated, 1, 1);
                pvm_pkint(proc->current, cellslist.n, 1);
                pvm_send(tid, PVM_MSG_SNDWKSET);
#ifdef DEBUG
	        printf("<DBG(%d)> Working set sent to task t%x.\n", time(NULL), tid);
#endif
        }

#ifdef DEBUG
	printf("<DBG(%d)> Master task t%x finish his work.\n", time(NULL), proc->pid);
#endif
	proc->terminated = 1;

	for(i = 0; i < proc->nproc - 1; i++) {
                pvm_recv(-1, PVM_MSG_REQWKSET);
                pvm_upkint(&tid, 1, 1);

	        pvm_initsend(PvmDataDefault);
	        pvm_pkint(&proc->terminated, 1, 1);
	        pvm_mcast(proc->tids, proc->nproc - 1, PVM_MSG_SNDWKSET);
        }
}

int GetNextSet(struct process_t *proc)
{
        pvm_initsend(PvmDataDefault);
	pvm_pkint(&proc->pid, 1, 1);
	pvm_send(pvm_parent(), PVM_MSG_REQWKSET);
#ifdef DEBUG
        printf("<DBG(%d)> Working set resquested by task t%x.\n", time(NULL), proc->pid);
#endif
	pvm_recv(-1, PVM_MSG_SNDWKSET);
	pvm_upkint(&proc->terminated, 1, 1);

	if( proc->terminated ) return 0;

        pvm_upkint(proc->current, cellslist.n, 1);
#ifdef DEBUG
        printf("<DBG(%d)> Working set received by task t%x.\n", time(NULL), proc->pid);
#endif
	return 1;
}

void EvaluateDimension(struct node_t *org, int *cset, double val, double *cval, int mode)
{
	struct edge_t *edge = org->edges;

	struct cell_t *cell;
	struct node_t *dest;

	if(val > (*cval)) (*cval) = val;

	while(edge != NULL) {
	        cell = edge->cell;
		dest = edge->dest;

		if(mode == SUM_WIDTH)
	                EvaluateDimension(dest, cset, val + (cell->instances)[(cset[cell->ord])].width, cval, mode);
		else
		        EvaluateDimension(dest, cset, val + (cell->instances)[(cset[cell->ord])].height, cval, mode);

		edge = edge->next;
        }
}

void EvaluateArea(struct process_t *proc)
{
        double width, height;
	double area;

	while( GetNextSet(proc) ) {
	        width = 0.0;
		height = 0.0;

		EvaluateDimension(graph_h.first, proc->current, 0.0, &width, SUM_WIDTH);
		EvaluateDimension(graph_g.first, proc->current, 0.0, &height, SUM_HEIGHT);
		area = width * height;

		if(area < (proc->cdim).carea) {
		        (proc->cdim).cwidth = width;
			(proc->cdim).cheight = height;
			(proc->cdim).carea = area;
			memcpy(proc->selected, proc->current, cellslist.n * sizeof(int));
		}
        };

#ifdef DEBUG
	printf("<DBG(%d) Slave task t%x finish his job.\n", time(NULL), proc->pid);
	DisplayResult(proc);
#endif

}

void DisplayResult(struct process_t *proc)
{
        struct cell_t *cell = cellslist.top;

	int *p = proc->selected;

	printf("\nThe best dimention found is %f x %f = %f\n", (proc->cdim).cwidth, (proc->cdim).cheight, (proc->cdim).carea);

	printf("\nUsed cells' instances:\n\n");
	while(cell != NULL) {
	        printf("\t%s[%d] = ", cell->id, (*p));
                printf("%f x ", (cell->instances)[(*p)].width);
                printf("%f\n", (cell->instances)[(*p)].height); 
	        cell = cell->next;
		p++;
	}
	printf("\n\n");
}

#ifdef DEBUG

void DisplayCells()
{
        struct cell_t *cell = cellslist.top;
	struct instance_t *p;

	int i;

	printf("\n<DBG(%d)> Number of cells = %d.\n", time(NULL), cellslist.n);

	while(cell != NULL) {
	        printf("<DBG(%d)> Cell id = %s.\n", time(NULL), cell->id);
		printf("<DBG(%d)> Number of instances = %d.\n", time(NULL), cell->n);

	        p = cell->instances;
		for(i = 0; i < cell->n; i++) {
		        printf("<DBG(%d)> Instance = %d.\n", time(NULL), i);
			printf("<DBG(%d)> Dimension = %f x %f.\n", time(NULL), p->width, p->height);
			p++;
		}
		cell = cell->next;
	} 
}

void DisplayGraph(struct graph_t *p)
{
        struct node_t *node = p->first;
	struct edge_t *edge;

	int i;

	while(node != NULL) {
	        printf("<DBG(%d)> Node id = %s.\n", time(NULL), node->id);
		printf("<DBG(%d)> Number of edges = %d.\n", time(NULL), node->n);

		i = 1;

		edge = node->edges;
		while(edge != NULL) {
		        printf("<DGB(%d)> Edge number = %d.\n", time(NULL), i);
			printf("<DBG(%d)> Cell id = %s.\n", time(NULL), (edge->cell)->id);
			printf("<DBG(%d)> Destination node = %s.\n", time(NULL), (edge->dest)->id);
			edge = edge->next;
			i += 1;
	        }
		node = node->next;
        }
}

void DisplayCurrentSet(int *cset)
{
        struct cell_t *cell = cellslist.top;

	printf("\n<DBG(%d)> Working set:\n", time(NULL));
	printf("\tcell\tcurrent\t\t\t\tworking set\n");
	printf("\t----\t-------\t\t\t\t-----------\n");
	while(cell != NULL) {
	        printf("\t(%s)", cell->id);
                printf("\t%d %f x %f\t", cell->current, (cell->instances)[cell->current].width, (cell->instances)[cell->current].height);
                printf("\t%d %f x %f\n", *cset, (cell->instances)[(*cset)].width, (cell->instances)[(*cset)].height);
		cell = cell->next;
		cset++;
	}
}

#endif

void MinimalArea(int *datatype, void *x, void *y, int *count, int *info)
{
        struct process_t *xp = x;
	struct process_t *yp = y;

	if((yp->cdim).carea < (xp->cdim).carea) {
                (xp->cdim).ctid = (yp->cdim).ctid;
                (xp->cdim).cwidth = (yp->cdim).cwidth;
		(xp->cdim).cheight = (yp->cdim).cheight;
		(xp->cdim).carea = (yp->cdim).carea;
        }
}




