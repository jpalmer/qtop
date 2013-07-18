#include "qtop.h" //examples of functions needed are incomplete
#include <unistd.h> //getlogin
#include <string.h> //strcmp
#include <stdio.h> //printf
#include <time.h>  //maketime
#include <stdlib.h> //malloc
#include "print.h"
//globals
const char* colours[7]  =   {"\x1B[31m", "\x1b[32m", "\x1b[33m", "\x1b[34m", "\x1b[35m", "\x1b[36m",""};
const char* Highlight   =   "\x1b[41m\x1b[32m";
const char* bold        =   "\x1b[1m";
const char* underline   =   "\x1b[4m";
const char* resetstr    =   "\x1b[0m";
const char* boxon       =   "\x1b(0";
const char* boxoff      =   "\x1b(B";
const char* gray        =   "\x1b[100m";
const char boxchars[]   =   {0x71,  0x74,       0x75,       0x6C,       0x6b,        0x6e,   0x77,   0x76};
const char* headingfmt  =   "\x1b[1;97m\x1b[100m\x1b[4m";
typedef enum               {dash,  leftedge,   rightedge,  topleft,    topright,    cross,  downT,  upT} boxenum;
char * me = NULL;
void heading_(const char* s) {printf("%s%s%s\n",headingfmt,s,resetstr); }
void checkColour()
{
   if(!isatty(fileno(stdout)))
   {
        Highlight="";
        resetstr="";
        underline="";
        for (int i=0;i<6;i++) {colours[i]="";}
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
int UserNo (const user* u,const user* cu) //algorithm for allocating numbers is currently n^2 - could make it better with sorting, but typically number of users is small, so current method is simpler
{
    if (me == NULL) { me = getlogin();}
    if (!(strcmp(cu->name,me))) {return 0;}
    const user* start = u;
    while (u != cu) {u=u->next;}
    int i=0;
    int mycount=u->runcount + u ->queuecount;
    while (start != NULL) 
    {
        int scount=start->runcount + start -> queuecount;
        if (        scount>mycount //more cores that the other
                ||  (scount==mycount && (strcmp(start->name,u->name)>0)) //or alphabetical if cores tied
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
    heading_("Name    Load Usage   Mem: Free avail | Name    Load Usage   Mem: Free avail");
    int* prevline = calloc(sizeof(int),MAXCPUS);
    int* curline = calloc(sizeof(int),MAXCPUS);
    int count = 0;
    while (n != NULL)
    {
        int i=0;
        long long int requestedram=0L;
        if (n->up==0)
        {
            printf("%s %s%5.2f%s ",n->name,n->loadave > (float)n->cores+1.5?Highlight:"",n->loadave,resetstr);
            for (;i<n->users_using_count;i++)
            {
                requestedram += n->users_using[i]->ramrequested;
                int myuserno=UserNo(u,n->users_using[i]->owner);
                printf ("%s%i%s",UserColourStr(myuserno),myuserno,resetstr);
                curline[i]=0;
            }
            printf(boxon);
            if(i==n->cores) {}
            else if (i==n->cores-1)
                {printf(boxon);putchar(0x78);printf(boxoff);curline[i]=1;i++;}
            else
            {
                curline[i]=1;
                putchar(boxchars[leftedge]);i++;
                for (;i<n->cores-1;i++) 
                {
                    if (n->next != NULL && n->next->next!=NULL&& ((n->next->next-> users_using_count == i && n->next->next->cores!=i ) || (i==n->next->next->cores-1 && n->next->next->users_using_count<i))) 
                        {putchar(boxchars[downT]);curline[i]=1;}
                    else{putchar(prevline[i]==1?(boxchars[upT]):(boxchars[dash]));curline[i]=0;}
                } 
                curline[i]=1;
                putchar(boxchars[rightedge]);i++;
            }
            printf(boxoff);
            for (;i<MAXCPUS;i++) {putchar(' ');} //blanks
            if (n->ramfree<0) {printf("%s",Highlight);}
            printf(" %2.0fGB%s  %2.0fGB",((double)n->ramfree)/1024.0/1024.0,resetstr,(double)(n->physram-requestedram )/1024.0/1024.0);
        }
        else
        {
            printf("%s%s  ERROR ERROR ERROR ERROR ERROR ERROR %s",Highlight,n->name,resetstr);
        }
        count++;
        if (count%2==0) {printf("\n");}else {printf(" | ");}
        n=n->next;
        int* temp=prevline;
        prevline=curline;
        curline=temp;
    }
}
void printmyjobs(const user* u)
{
    char * me = getlogin();
    while (u != NULL)
    {
        if (!strcmp(me,u->name))
        {
            const job * j=u->jobs;
            if (j != NULL)
            {
                heading_("Job No     |state CPU  RAM     STARTING IN");
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
    while(i<2 && j != NULL) //print the first 2 running jobs
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
        heading_("N Name     | Run    Q Running jobs          Queued jobs           Suspended jobs");
        const user* start=u;
        const int count = UserCount(u);
        for (int i=0;i<count;i++)
        {
            u=start;
            while (u != NULL)
            {
                if (UserNo(start,u)==i)
                {
                    printf ("%s%i %-8s%s |%s%4i %4i ",UserColourStr(i),i,u->name,resetstr,UserColourStr(i),u->runcount,u->queuecount);
                    int accum = printSomeJobs(u->jobs,R);
                    for (;accum<22;accum++) {printf(" ");} //fill in some white space
                    accum += printSomeJobs(u->jobs,Q);
                    for (;accum<44;accum++) {printf(" ");} //fill in some white space
                    printSomeJobs(u->jobs,S);
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
        heading_("Queue Name | Q Length     Q Ram  NextJob: ID        USER CPU         RAM");
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
typedef struct propinfo propinfo;
struct propinfo 
{
    char* propname;
    int* free;
};
void PropStats(const node* n)
{
    const int maxprops=10;
    propinfo* props = calloc(sizeof(propinfo),maxprops);
    const char vbar=0x78;
    int propcounter=0;
    printf("%sCPU Avail  |  %i ",headingfmt,1);
    for (int i=2;i<=MAXCPUS;i+=2) {printf("%3i ",i);}
    printf("%s\n",resetstr);
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
        printf("%-10s |",props[i].propname);
        for (int j=0;j<1+MAXCPUS/2;j++) {printf("%3i ",props[i].free[j]);}
        printf("\n");
    }
}
