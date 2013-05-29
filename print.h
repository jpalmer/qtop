void printnode      (const node*,const user*);
void printuser      (const user* );
void printq         (const job*);
void printmyjobs    (const user* u);
void checkColour    ();
typedef struct
{
    const char * name;
    int cpucount;
    long long int ram;
    const job * nextjob;
} qstats;

