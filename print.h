//functions
void printnode      (const node*,const user*);
void printuser      (const user* );
void printq         (const job*);
void printmyjobs    (const user* u);
void PropStats      (const node* n);
void checkColour    ();
//structs
typedef struct
{
    const char * name;
    int cpucount;
    long long int ram;
    const job * nextjob;
    const char** nodenames;
} qstats;
