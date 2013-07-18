#include "qtop.h" //examples of functions needed are incomplete
#include <unistd.h> //getlogin
#include <string.h> //strcmp
#include <stdio.h> //printf
#include <time.h>  //maketime
#include <stdlib.h> //malloc
#include "print.h"
//globals
const char* colours[7]  =   {"\x1B[41m", "\x1b[42m", "\x1b[43m", "\x1b[44m", "\x1b[45m", "\x1b[46m",""};
const char* basecols[]  =   {"\x1b[42m","\x1b[44m","\x1b[44m"};
const char* Highlight   =   "\x1b[31m";
const char* bold        =   "\x1b[1m";
const char* underline   =   "\x1b[4m";
const char* resetstr    =   "\x1b[0m";
const char* boxon       =   "\x1b(0";
const char* boxoff      =   "\x1b(B";
const char* gray        =   "\x1b[100m";
const char boxchars[]   =   {0x71,  0x74,       0x75       };
typedef enum               {dash,  leftedge,   rightedge  } boxenum;
const char* headingfmt  =   "\x1b[1;97m\x1b[100m\x1b[4m";
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
    if (me == NULL) { me = getlogin();}
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
    heading_("Name    Load Usage   Mem: Free avail | Name    Load Usage   Mem: Free avail     ");
    int count = 0;
    propinfo* props;
    int propcount=GetPropinfo(n,&props);
    while (n != NULL)
    {
        long long int requestedram=0L;
        if (n->up==0)
        {
            
            printf("%s %s%5.2f%s ",n->name,n->loadave > (float)n->cores+1.5?Highlight:"",n->loadave,resetstr);
            printf(boxon);
            int i=0;
            for (;i<n->cores;i++)
            {
                const char* colorstr = resetstr;
                if (i< n-> users_using_count)
                {
                    requestedram += n->users_using[i]->ramrequested;
                    int myuserno=UserNo(u,n->users_using[i]->owner);
                    colorstr = UserColourStr(myuserno);
                }
                if (i==0)       {printf ("%s%c",colorstr,boxchars[leftedge]);}
                if (i==n->cores-1){printf ("%s%c",colorstr,boxchars[rightedge]);}
                else            {printf ("%s%c",colorstr,boxchars[dash]);}

            }
            printf("%s",resetstr);
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
                heading_("Job No     |state CPU  RAM     STARTING IN                                      ");
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
                    printf ("%s%i %-8s |%4i %4i ",UserColourStr(i),i,u->name,u->runcount,u->queuecount);
                    int accum = printSomeJobs(u->jobs,R);
                    for (;accum<22;accum++) {printf(" ");} //fill in some white space
                    accum += printSomeJobs(u->jobs,Q);
                    for (;accum<44;accum++) {printf(" ");} //fill in some white space
                    printSomeJobs(u->jobs,S);
                    for (;accum<58;accum++) {printf(" ");} //fill in some white space
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


void PropStats(const node* n)
{
    printf("%sCPU Avail  |  %i ",headingfmt,1);
    for (int i=2;i<=MAXCPUS;i+=2) {printf("%3i ",i);}
    printf("                                        %s\n",resetstr);
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
        printf("%-10s |",props[i].propname);
        for (int j=0;j<1+MAXCPUS/2;j++) {printf("%3i ",props[i].free[j]);}
        printf("\n");
    }
}
