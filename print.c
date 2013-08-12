#include "qtop.h" //examples of functions needed are incomplete
#include <unistd.h> //getlogin
#include <string.h> //strcmp
#include <stdio.h> //printf
#include <time.h>  //maketime
#include <stdlib.h> //malloc
#include <termcap.h> //columns
#include <math.h>    //ceil
#include "print.h"
#define max(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a>_b?_a:_b;})
#define min(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a<_b?_a:_b;})

//globals
const char* colours[7]  =   {"\x1B[41m", "\x1b[42m", "\x1b[43m", "\x1b[44m", "\x1b[45m", "\x1b[46m",""};
const char* basecols[]  =   {"\x1b[32m","\x1b[34m","\x1b[34m"};
const char* Highlight   =   "\x1b[31m";
const char* bold        =   "\x1b[1m";
const char* underline   =   "\x1b[4m";
const char* resetstr    =   "\x1b[0m";
const char* boxon       =   "\x1b(0";
const char* boxoff      =   "\x1b(B";
const char* gray        =   "\x1b[100m";
char boxchars[]         =   {0x71,  0x74,       0x75       };
typedef enum                {dash,  leftedge,   rightedge  } boxenum;
const char * headingfmt =   "\x1b[1;97m\x1b[100m\x1b[4m";
char * me = NULL;
int twidth = 1;
void heading_(const char* s) {printf("%s%s%s\n",headingfmt,s,resetstr); }
int  heading_nr(const char* s) {return  printf("%s%s",headingfmt,s) - strlen(headingfmt); }
int  heading_n(const char* s) {int r = heading_nr(s);printf(resetstr);return r;}
void heading_fill(const char* s) {for (int c = heading_nr(s);c<twidth;c++){putchar(' ');}printf(resetstr);}
static char termbuf [2048];
void SetupTerm()
{
   if(!isatty(fileno(stdout)))
   {
        Highlight="";
        resetstr="";
        underline="";
        for (int i=0;i<6;i++) {colours[i]="";}
        for (int i=0;i<3;i++) {basecols[i]="";}
        bold="";
        boxon="";
        boxoff="";
        gray="";
        boxchars[0]='-';boxchars[1]='|';boxchars[2]='|';
        headingfmt="";
   }
   else
   {
        tgetent(termbuf,getenv("TERM"));
        twidth=tgetnum("co");
   }
}
char StatStr (const job* j)
{
    switch (j->state)
    {
        case Q:return 'Q';
        case R:return 'R';
        case C:return 'C';
        case S:return 'S';
        default:
            printf("error in state\n");
            return 'E';
    }
}
int GetPropinfo(const node* n,propinfo** p)
{
    const int maxprops=10;
    propinfo* props = calloc(sizeof(propinfo),maxprops);
    int propcounter=0;
    const node* cn = n;
    while (cn != NULL)
    {
        int found=0;
        for (int i=0;i<propcounter;i++)
        {   
            if (!strcmp(cn->props,props[i].propname)) {found=1;break;printf("break");}
        }
        if (found==0) {props[propcounter].propname=cn->props;propcounter++;}
        cn=cn->next;
    }
    *p=props;
    return propcounter;

}
int UserNo (const user* u,const user* cu) //algorithm for allocating numbers is currently n^2 - could make it better with sorting, but typically number of users is small, so current method is simpler
{
    if (me == NULL) { me = getenv("LOGNAME");}
    if (!(strcmp(cu->name,me))) {return 0;}
    const user* start = u;
    int i=0;
    int mycount=cu->runcount + cu ->queuecount;
    while (start != NULL) 
    {
        int scount=start->runcount + start -> queuecount;
        if (        scount>mycount //more cores that the other
                ||  (scount==mycount && (strcmp(start->name,cu->name)>0)) //or alphabetical if cores tied
                ||  (!(strcmp(start->name,me)))) //I go first
        {i++;} //complex ordering based on number of queued jobs and running jobs then alphabetical
        start=start->next;
    }
    return i;
}
const char* UserColourStr(const int userno)
{
    if (userno>5) {return colours[6];} else {return colours[userno];}
}
int UserCount(const user* u)
{
    int ret = 0;
    while(u != NULL) {ret++;u=u->next;}
    return ret;
}

void printnode(const node* n,const user* u)
{
    const char* First = "Name    Load Usage   Mem: Free avail ";
    const char* others= "| Name    Load Usage   Mem: Free avail ";
    int width=(twidth< (int)strlen(First))?1:(twidth-strlen(First))/strlen(others) + 1; //magic numbers related to lengths of strings below.  
    int sum = heading_n(First);
    for (int k=1;k<width;k++){sum += heading_n(others);}
    for (;sum<twidth;sum++) {heading_n(" ");}
    printf("\n");
    propinfo* props;
    int propcount=GetPropinfo(n,&props); 
    const node** nodes  = calloc(sizeof(node*),width+1); //to gurantee that nodes[1] exists
    int nodecount = 0;
    const node* dummy = n;
    while (dummy != NULL) {dummy=dummy->next;nodecount++;}
    dummy=n;
    int c = 0;
    int nodecount_r=((int)ceil((float)nodecount/(float)width)) ; //so divisibility tests work
    int excesscount = nodecount % width;
    if (nodecount_r/width<nodecount/width) {nodecount_r=nodecount/width ;}
    nodes[0]=n;
    while (dummy != NULL) //set up first element in each column
    {
        int offset=min(excesscount,c/nodecount_r);
        if (((c-offset)/nodecount_r)*nodecount_r==(c-offset)) 
            {nodes[(c-offset)/nodecount_r]=dummy;}
        dummy= dummy-> next;
        c++;
    }
    int colindex=0;
    int printed = 0;
    while (printed != nodecount)
    {
        const node* cn = nodes[colindex];
        long long int requestedram=0L;
        if (cn->up==0)
        {
            const char* nodecol = resetstr;
            for (int i=0;i<propcount;i++)
            {
                if (!strcmp(cn->props,props[i].propname))
                    {
                    nodecol=basecols[i];
                    }
            }

            printf("%s%s%s %s%5.2f%s ",nodecol,cn->name,resetstr,cn->loadave > (float)cn->cores+1.5?Highlight:"",cn->loadave,resetstr);
            printf(boxon);
            int i=0;
            for (;i<cn->cores;i++)
            {
                const char* colorstr = resetstr;
                if (i< cn-> users_using_count)
                {
                    requestedram += cn->users_using[i]->ramrequested;
                    int myuserno=UserNo(u,cn->users_using[i]->owner);
                    colorstr = UserColourStr(myuserno);
                }
                if (i==0)       {printf ("%s%c",colorstr,boxchars[leftedge]);}
                else if (i==cn->cores-1){printf ("%s%c",colorstr,boxchars[rightedge]);}
                else            {printf ("%s%c",colorstr,boxchars[dash]);}

            }
            printf("%s",resetstr);
            printf(boxoff);
            for (;i<MAXCPUS;i++) {putchar(' ');} //blanks
            if (cn->ramfree<0) {printf("%s",Highlight);}
            printf(" %2.0fGB%s  %2.0fGB",((double)cn->ramfree)/1024.0/1024.0,resetstr,(double)(cn->physram-requestedram )/1024.0/1024.0);
        }
        else
        {
            printf("%s%s  ERROR ERROR ERROR ERROR ERROR ERROR %s",Highlight,cn->name,resetstr);
        }
        nodes[colindex]=nodes[colindex]-> next;
        if (colindex+1==width) {printf("\n");colindex=0;}else {printf(" | ");colindex++;} //either print a serperator or a newline
        printed++;
        
    }
    if (colindex != 0 ) {printf("\n");} //if we didn't end with a newline print a newline
}
void printmyjobs(const user* u)
{
    while (u != NULL)
    {
        if (!strcmp(me,u->name))
        {
            const job * j=u->jobs;
            if (j != NULL)
            {
                heading_fill("Job No     |state CPU  RAM     STARTING IN");
            }
            while (j != NULL)
            {
                printf("%i     |   %c %3i %5.2fGB",j->number,StatStr( j),j->corecount,(double)j->ramrequested/1024.0/1024.0);
                if (j->state==Q || j-> state==S) //if job queued then call showstart to print the start time for the job (based on reported runtimes, job likely to start even earlier than this
                {
                    char* command = malloc(1024);
                    if (j -> arrayid == -1)
                    {
                        sprintf(command,"showstart %i | grep start | tr -s ' ' | cut -d ' ' -f4",j->number); //get time to start
                    }
                    else
                    {
                        sprintf(command,"showstart %i-%i | grep start | tr -s ' ' | cut -d ' ' -f4",j->number,j->arrayid); //get time to start
                    }
                    FILE* pout=popen(command,"r");
                    char* datebuffer = malloc(100);//100 characters should be enough
                    fgets(datebuffer,100,pout);
                    strchr(datebuffer,'\n')[0]=0; //cut adds an extra newline - annoying
                    printf("  %11s\n",datebuffer);
                    free(command);free(datebuffer);
                }
                else {printf("\n");}
            j=j->usernext;
            }
        }
        u=u->next;
    }
}
int printSomeJobs(const job* j, const jobstate s)
{
    int i = 0;
    int accum=0;
    while(i<2 && j != NULL) //print the first 2 jobs in some state
    {
       if (j->state==s)
        {
            i++;
            accum+= printf("%ix%i ",j->number,j->corecount);
        }
        j=j->usernext;
    }
    while (j != NULL) {if (j->state==s) {accum += printf ("...");break;}j=j->next;}
    return accum;
}
void printuser(const user* u)
{
    if (u != NULL)
    {
        heading_fill("N Name     | Run    Q Running jobs          Queued jobs           Suspended jobs");
        const user* start=u;
        const int count = UserCount(u);
        for (int i=0;i<count;i++)
        {
            u=start;
            while (u != NULL)
            {
                if (UserNo(start,u)==i)
                {
                    int initial = printf ("%s%i %-8s |%4i %4i ",UserColourStr(i),i,u->name,u->runcount,u->queuecount) - strlen(UserColourStr(i));
                    int accum = printSomeJobs(u->jobs,R);
                    for (;accum<22;accum++) {printf(" ");} //fill in some white space
                    accum += printSomeJobs(u->jobs,Q);
                    for (;accum<44;accum++) {printf(" ");} //fill in some white space
                    printSomeJobs(u->jobs,S);
                    for (;accum<(twidth-initial);accum++) {printf(" ");} //fill in some white space
                    printf("%s\n",resetstr);
                }
                u=u->next;
            }
        }
    }
}
void printq(const job* j)
{
    int foundqcount=0;
    qstats * queues = malloc (sizeof(qstats*)* MAX_QUEUES);
    while (j != NULL)
    {
        if (j->state==Q)
        {
            char found=0;
            for (int i=0;i<foundqcount;i++)
            {
                if (!strcmp(j->queue,queues[i].name))
                {
                   queues[i].cpucount+=j->corecount;
                   queues[i].ram += j->ramrequested;
                   found=1;
                }
            }
            if (found==0)
            {
                queues[foundqcount].name=j->queue;
                queues[foundqcount].ram=j->ramrequested;
                queues[foundqcount].cpucount=j->corecount;
                queues[foundqcount].nextjob=j;
                foundqcount++;
            }
        }
        j =j->next;
    }
    if (foundqcount != 0)
    {
        heading_fill("Queue Name | Q Length     Q Ram  NextJob: ID        USER CPU         RAM");
        for (int i=0;i<foundqcount;i++)
        {
            const qstats q = queues[i];
            const job j = *q.nextjob;
                 //qname      qCPU  qram           jno juser jcpu jram
            printf("%-11s|     %4i  %6.2fGB       %6i  %10s  %2i  %8.2fGB\n",q.name,q.cpucount,(float)q.ram/1024.0/1024.0,
                    j.number,j.owner->name,j.corecount, (float)j.ramrequested/1024.0/1024.0);
        }
    }
    free(queues);
}


void PropStats(const node* n)
{
    int c = heading_nr("CPU Avail  |  1 ");
    for (int i=2;i<=MAXCPUS;i+=2) {c+=printf("%3i ",i); }
    for (;c<twidth;c++) {putchar(' ');}
    printf("%s\n",resetstr);
    propinfo* props;
    int propcounter=GetPropinfo(n,&props);
    const node* cn=n;
    for (int i=0;i<propcounter;i++)
    {
        props[i].free=calloc(sizeof(int),MAXCPUS/2 +1);
        cn=n;
        while (cn != NULL)
        {
            if (!strcmp(cn->props,props[i].propname))
            {
                int freecpus=cn->cores - cn->users_using_count;
                props[i].free[0] += freecpus;
                for (int j=2;j<=MAXCPUS;j+=2) {props[i].free[j/2]+= freecpus/j; }
            }
            cn=cn->next;
        }
        printf("%s%-10s |",basecols[i],props[i].propname);
        for (int j=0;j<1+MAXCPUS/2;j++) {printf("%3i ",props[i].free[j]);}
        printf("%s\n",resetstr);
    }
}
