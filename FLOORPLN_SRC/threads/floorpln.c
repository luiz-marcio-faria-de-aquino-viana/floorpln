
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

#define DEBUG

#define TK_NULL           0
#define TK_COMMENT        1
#define TK_SYMBOL         2
#define TK_IDENTIFIER     3
#define TK_INTEGER        4
#define TK_FLOAT          5

#define SUM_WIDTH         0
#define SUM_HEIGHT        1

#define MAX_AREA          ((double) 1.0e99)

#define F_CELLS           "cells.dat"
#define F_GRAPH_G         "graph_g.dat"
#define F_GRAPH_H         "graph_h.dat"

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

#define MIN_NPROCS        1
#define MAX_NPROCS       20

#define PARM_NPROC       "-np"

typedef char str_t[256];

struct parm_t {
        int nproc;
};

struct process_t {
        int pid;
        pthread_t thread;
        double cwidth;
        double cheight;
        double carea;
        int *selected;
};

struct proclist_t {
        int nproc;
        int terminated;
        struct process_t *procs;
};

struct instance_t {
        double width;
	double height;
};

struct cell_t {
        char *id;
        int ord;
        int n;
        pthread_mutex_t lock_current;
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

struct proclist_t proclist = { 0, 0, NULL };

struct celllist_t cellslist = { 0, NULL, NULL };

struct graph_t    graph_g = { NULL };
struct graph_t    graph_h = { NULL };

void GetParm(int argc, char *argv[]);

int IsNull(char *s);
int IsComment(char *s);

int GetToken(char *tk, char **s);

struct process_t *CreateProc(struct process_t *proc);

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

int SelectNextSet(int *cset);

void EvaluateDimension(struct node_t *org, int *cset, double val, double *cval, int mode);
void *EvaluateArea(void *p);

void DisplayResult(struct process_t *proc);

#ifdef DEBUG
void DisplayCells();
void DisplayGraph(struct graph_t *p);
void DisplayCurrentSet(int *cset);
#endif

int main(int argc, char *argv[])
{
        struct process_t *cproc = NULL;
	double carea = MAX_AREA;

	int i;

        GetParm(argc, argv);

        RetrieveCells();
#ifdef DEBUG
	DisplayCells();
#endif

	RetrieveGraph(&graph_g, F_GRAPH_G);
#ifdef DEBUG
        DisplayGraph(&graph_g);
#endif

	RetrieveGraph(&graph_h, F_GRAPH_H);
#ifdef DEBUG
        DisplayGraph(&graph_h);
#endif

	if((proclist.procs = (struct process_t *) malloc(parm.nproc * sizeof(struct process_t))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
                exit(1);
	}

	for(i = 0; i < parm.nproc; i++)
	        CreateProc(&proclist.procs[i]);

	for(i = 0; i < parm.nproc; i++) {
	        if( pthread_join(proclist.procs[i].thread, NULL) ) {
                        printf("ERR: When waiting for the thread %d.\n", i);
			exit(1);
                }
                if(proclist.procs[i].carea < carea) {
                        cproc = &proclist.procs[i];
                        carea = cproc->carea;
		}
        }

	DisplayResult(cproc);

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

struct process_t *CreateProc(struct process_t *proc)
{
        int i;

        if((proc->selected = (int *) malloc(cellslist.n * sizeof(int))) == NULL) {
                printf("ERR: Can't allocate memory.\n");
                exit(1);
        }

	proc->pid = proclist.nproc;
	proclist.nproc += 1;

        proc->cwidth = 0.0;
        proc->cheight = 0.0;
        proc->carea = MAX_AREA;

        for(i = 0; i < cellslist.n; i++)
                proc->selected[i] = 0;

        if(pthread_create(&proc->thread, NULL, EvaluateArea, proc) != 0) {
                printf("ERR: Can't create thread.\n");
                exit(1);
        }

	return proc;
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
	pthread_mutex_init(&cell->lock_current, NULL);
	cell->current   = 0;
	cell->next      = NULL;

        if(cellslist.top == NULL)    cellslist.top = cell;
        if(cellslist.botton != NULL) (cellslist.botton)->next = cell;
	cellslist.botton = cell;
	cellslist.n += 1;

#ifdef DEBUG
        printf("<DBG> Created cell: %s\n", cell->id);
#endif

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

#ifdef DEBUG
        printf("<DBG> Cell's identifier: %s\n", token);
#endif

	if((cell = SearchCell(token)) != NULL) {
	        printf("ERR: Duplicated identifier: %s\n", s);
		exit(1);
	}

	if(GetToken(token, &s) != TK_INTEGER) {
	        printf("ERR: Expected an integer value: %s\n", s); 
	        exit(1);
        }
	n = atoi(token);

#ifdef DEBUG
                printf("<DBG> Number os cell's instances: %s\n", token);
#endif
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
#ifdef DEBUG
                printf("<DBG> Line read: %s\n", s);  
#endif
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
#ifdef DEBUG
                printf("<DBG> Line read: %s\n", s);  
#endif
	        if( !IsNull(s) && !IsComment(s) )
                        ProcessNodeDef(p, s);
	}
}

int SelectNextSet(int *cset)
{
        struct cell_t *cell;
	int flag = 1;

        cell = cellslist.top;
	while(cell != NULL) {
                pthread_mutex_lock(&cell->lock_current);
                (*cset) = cell->current;
		if( flag ) {
	                cell->current += 1;
		        if(cell->current < cell->n)
                                flag = 0;
                        else
		                cell->current = 0;
                }
		pthread_mutex_unlock(&cell->lock_current);
		cell = cell->next;
		cset++;
        }

	if( flag ) proclist.terminated = 1;
	return !flag;
}

void EvaluateDimension(struct node_t *org, int *cset, double val, double *cval, int mode)
{
	struct edge_t *edge = org->edges;

	struct cell_t *cell;
	struct node_t *dest;

	if(val > (*cval)) (*cval) = val;

#ifdef DEBUG
	printf("<DBG> Node = %s, Val = %f, CVal = %f\n", org->id, val, (*cval));
#endif
	while(edge != NULL) {
	        cell = edge->cell;
		dest = edge->dest;
#ifdef DEBUG
	        printf("<DBG> Cell = %s, Dest = %s\n", cell->id, dest->id);
#endif

		if(mode == SUM_WIDTH)
	                EvaluateDimension(dest, cset, val + (cell->instances)[(cset[cell->ord])].width, cval, mode);
		else
		        EvaluateDimension(dest, cset, val + (cell->instances)[(cset[cell->ord])].height, cval, mode);

		edge = edge->next;
        }
}

void *EvaluateArea(void *p)
{
        struct process_t *proc = (struct process_t *) p;

        double width, height;
	double area;

        int *currset;

	if((currset = (int *) malloc(cellslist.n * sizeof(int))) == NULL) {
	        printf("ERR: Can't allocate memory.\n");
		exit(1);
	}

	while( !proclist.terminated ){
                SelectNextSet(currset);
#ifdef DEBUG
	        DisplayCurrentSet(currset);
#endif
	        width = 0.0;
		height = 0.0;

		EvaluateDimension(graph_h.first, currset, 0.0, &width, SUM_WIDTH);
		EvaluateDimension(graph_g.first, currset, 0.0, &height, SUM_HEIGHT);

		area = width * height;

		if(area < proc->carea) {
		        proc->cwidth = width;
			proc->cheight = height;
			proc->carea = area;
			memcpy(proc->selected, currset, cellslist.n * sizeof(int));
		}
#ifdef DEBUG
		printf("<DBG> PROC(%d): Calculated dimension: %f x %f\n", proc->pid, width, height);
		printf("<DBG> PROC(%d): Calculated area: %f\n", proc->pid, area);
		printf("<DBG> PROC(%d): Best dimension: %f x %f\n", proc->pid, proc->cwidth, proc->cheight);
		printf("<DBG> PROC(%d): Best area: %f\n", proc->pid, proc->carea);
#endif

        };

	return NULL;
}

void DisplayResult(struct process_t *proc)
{
        struct cell_t *cell = cellslist.top;

	int *p = proc->selected;

	printf("\nThe best dimention found is %f x %f = %f\n", proc->cwidth, proc->cheight, proc->carea);

	printf("\nUsed cells' instances:\n\n");
	while(cell != NULL) {
	        printf("\t%s = %f x %f\n", cell->id, (cell->instances)[(*p)].width, (cell->instances)[(*p)].height); 
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

	printf("\n<DBG> Number of cells = %d.\n", cellslist.n);

	while(cell != NULL) {
	        printf("<DBG> Cell id = %s.\n", cell->id);
		printf("<DBG> Number of instances = %d.\n", cell->n);

	        p = cell->instances;
		for(i = 0; i < cell->n; i++) {
		        printf("<DBG> Instance = %d.\n", i);
			printf("<DBG> Dimension = %f x %f.\n", p->width, p->height);
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
	        printf("<DBG> Node id = %s.\n", node->id);
		printf("<DBG> Number of edges = %d.\n", node->n);

		i = 1;

		edge = node->edges;
		while(edge != NULL) {
		        printf("<DGB> Edge number = %d.\n", i);
			printf("<DBG> Cell id = %s.\n", (edge->cell)->id);
			printf("<DBG> Destination node = %s.\n", (edge->dest)->id);
			edge = edge->next;
			i += 1;
	        }
		node = node->next;
        }
}

void DisplayCurrentSet(int *cset)
{
        struct cell_t *cell = cellslist.top;

	printf("\n<DBG> Working set:\n");
	while(cell != NULL) {
	        printf("\t(%s)", cell->id);
                printf("\tcurrent = %d %f x %f\t", cell->current, (cell->instances)[cell->current].width, (cell->instances)[cell->current].height);
                printf("\tworking set = %d %f x %f\n", *cset, (cell->instances)[(*cset)].width, (cell->instances)[(*cset)].height);
		cell = cell->next;
		cset++;
	}
}

#endif





