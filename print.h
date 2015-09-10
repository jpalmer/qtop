//functions
void printnode      (const node*,const user*);
void printuser      (const user* );
void printMyJobCount(const user* );
void printq         (const job*);
void printmyjobs    (const user* u);
void PropStats      (const node* n);
void  FreeCpu        (const node* n);
void SetupTerm      ();
void printfooter();
//structs
typedef struct
{
    const char * name;
    int cpucount;
    long long int ram;
    const job * nextjob;
    const char** nodenames;
} qstats;
typedef struct propinfo propinfo;
struct propinfo 
{
    char* propname;
    int* free;
};
#define max(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a>_b?_a:_b;})
#define min(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a<_b?_a:_b;})
