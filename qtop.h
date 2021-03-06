#define MAXCPUS 32
#define MINMAXCPUS 12
#define MAX_QUEUES 10
typedef struct node node;
typedef struct job job;
typedef struct user user;
typedef struct Que Que;
struct user
{
    const char* name;
    const char* realname;
    const job* jobs;
    job* jobsend;
    int runcount;
    int queuecount;
    user * next;
};
struct node
{ //some example values
    char up;
    const char* name; //node23
    int cores;//8
    double loadave;//8.12734
    long long int ramfree;//234178234 (measured in kb - use a long long in case we get 2TB of ram)
    long long int physram;
    job* users_using[MAXCPUS];
    int users_using_count;
    node* next;
    char* props;
};
typedef enum  {Q, R,C,S} jobstate;
struct job
{
    int number;
    jobstate state;
    int corecount;
    user* owner;
    job* next;
    job* usernext;
    int secondsremaining;
    char * queue;
    long long int ramrequested;
    int arrayid;
};
extern char* filternodes;
//not in .c file because it breaks syntax highlighting
#define findN(x,namein) \
       ({ typeof (x) t=x;\
          typeof (x) ret=NULL;\
        while(t!=NULL && t-> name != NULL){  if  (!strcmp(namein,t->name)) {ret=t;break;}\
            t=t->next;}ret;})
