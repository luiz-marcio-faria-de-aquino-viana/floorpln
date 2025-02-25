
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

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

typedef char str_t[256];

struct instance_t {
        double width;
	double height;
};

struct cell_t {
        char *id;
        int n;
	int current;
        int selected;
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

struct celllist_t cellslist = { 0, NULL, NULL };

struct graph_t    graph_g = { NULL };
struct graph_t    graph_h = { NULL };

double cwidth, cheight;
double carea = MAX_AREA;

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

int SelectNextSet(int mode);

void EvaluateDimension(struct node_t *p, double val, double *cval, int mode);
void EvaluateArea();

void DisplayResult();

#ifdef DEBUG
void DisplayCells();
void DisplayGraph(struct graph_t *p);
#endif

int main(int argc, char* argv[])
{
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

	EvaluateArea();
	DisplayResult();

	return 0;
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
        cell->n         = n;
	cell->current   = 0;
	cell->selected  = 0;
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

int SelectNextSet(int mode)
{
        struct cell_t *cell = cellslist.top;

	int flag = 1;
	
	while(cell != NULL) {
	        if(mode == ACT_SAVE)
		        cell->selected = cell->current;

      	        if( flag ) {
	                cell->current += 1;
		        if(cell->current >= cell->n)
			        cell->current = 0;
			else
			        flag = 0;
		}

		cell = cell->next;
        }
	return flag;
}

void EvaluateDimension(struct node_t *p, double val, double *cval, int mode)
{
	struct edge_t *edge = p->edges;

	struct cell_t *cell;
	struct node_t *dest;

	if(val > (*cval)) (*cval) = val;

#ifdef DEBUG
	printf("<DBG> Node = %s, Val = %f, CVal = %f\n", p->id, val, (*cval));
#endif
	while(edge != NULL) {
	        cell = edge->cell;
		dest = edge->dest;
#ifdef DEBUG
	        printf("<DBG> Cell = %s, Dest = %s\n", cell->id, dest->id);
#endif

		if(mode == SUM_WIDTH)
	                EvaluateDimension(dest, val + (cell->instances)[cell->current].width, cval, mode);
		else
		        EvaluateDimension(dest, val + (cell->instances)[cell->current].height, cval, mode);
		edge = edge->next;
        }
}

void EvaluateArea()
{
	double width, height;
	double area;

	int flag;

        do {
              width = 0.0;
              height = 0.0;

	      EvaluateDimension(graph_h.first, 0.0, &width, SUM_WIDTH);
#ifdef DEBUG
	      printf("<DBG> Evaluated width: %f\n", width);
#endif
	      EvaluateDimension(graph_g.first, 0.0, &height, SUM_HEIGHT);
#ifdef DEBUG
	      printf("<DBG> Evaluated height: %f\n", height);
#endif
	      area = width * height;
	      if(area < carea) {
		      cwidth = width;
	              cheight = height;
		      carea = area;

	              flag = SelectNextSet(ACT_SAVE);
              }
	      else
	              flag = SelectNextSet(ACT_NOSAVE);
        } while( !flag );
}

void DisplayResult()
{
        struct cell_t *cell = cellslist.top;

	printf("\nThe best dimention found is %f x %f = %f\n", cwidth, cheight, carea);

	printf("\nUsed cells' instances:\n\n");
	while(cell != NULL) {
	        printf("\t%s = %f x %f\n", cell->id, (cell->instances)[cell->selected].width, (cell->instances)[cell->selected].height); 
	        cell = cell->next;
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

#endif





