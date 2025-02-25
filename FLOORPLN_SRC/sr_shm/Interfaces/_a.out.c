/* srl info for a.out */

#include "/usr/lib/srmulti.h"
#include "/usr/lib/sr.h"

static char SR_version[] = 
	"SR version 2.3.1, December 1995";

int sr_max_co_stmts = 1000000;
int sr_max_classes = 1000000;
int sr_max_loops = 10000;
int sr_max_operations = 1000000;
int sr_max_processes = 1000000;
int sr_max_rmt_reqs = 1000000;
int sr_max_resources = 1000000;
int sr_max_semaphores = 1000000;
int sr_stack_size = 40000;
int sr_async_flag = 0;
char sr_exec_path[] = "/usr/lib/srx";

int N_main = 0;  extern void R_main(), F_main();
int N_Shared = 1;  extern void R_Shared(), F_Shared();
int N_InitGraphs = 2;  extern void R_InitGraphs(), F_InitGraphs();
int N_InitCells = 3;  extern void R_InitCells(), F_InitCells();
int N_Param = 4;  extern void R_Param(), F_Param();

Rpat sr_rpatt[] = {
    { "main", R_main, F_main },
    { "Shared", R_Shared, F_Shared },
    { "InitGraphs", R_InitGraphs, F_InitGraphs },
    { "InitCells", R_InitCells, F_InitCells },
    { "Param", R_Param, F_Param },
};
int sr_num_rpats = 5;
