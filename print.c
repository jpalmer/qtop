#include "qtop.h" //examples of functions needed are incomplete
#include <unistd.h> //getlogin
#include <string.h> //strcmp
#include <stdio.h> //printf
#include <time.h>  //maketime
#include <stdlib.h> //malloc
#include "print.h"
const char* colours[7]  =   {"\x1B[31m","\x1b[32m","\x1b[33m","\x1b[34m","\x1b[35m","\x1b[36m",""};
const char* Highlight   =   "\x1b[41m\x1b[32m";
const char* bold        =   "\x1b[1m";
const char* resetstr    =   "\x1b[0m";
char * me = NULL;
void heading(const char* s) {printf("%s%s%s\n",bold,s,resetstr); }
void checkColour()
{
   if(!isatty(fileno(stdout)))
   {
        Highlight="";
        resetstr="";
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
        default:
            printf("error in state\n");
            return 'E';
    }
}
int UserNo (const user* u,const user* cu)
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
    heading("NODES");
    printf("Name     Load  CPU            MemFree   MemRequestable\n");
    while (n != NULL)
    {
        int i=0;
        long long int requestedram=0L;
        if (n->up==0)
        {
            printf("%s  %s%5.2f%s  ",n->name,n->loadave > (float)n->cores+1.5?Highlight:"",n->loadave,resetstr);
            for (;i<n->users_using_count;i++)
            {
                requestedram += n->users_using[i]->ramrequested;
                int myuserno=UserNo(u,n->users_using[i]->owner);
                printf ("%s%i%s",UserColourStr(myuserno),myuserno,resetstr);
            }
            for (;i<n->cores;i++) {putchar('-');} //fill in blank cpu
            for (;i<MAXCPUS;i++) {putchar(' ');}
            if (n->ramfree<0) {printf("%s",Highlight);}
            printf("  %6.2fGB%s   %5.2fGB",((double)n->ramfree)/1024.0/1024.0,resetstr,(double)(n->physram-requestedram )/1024.0/1024.0);
            printf("\n");
        }
        else
        {
            printf("%s%s  ERROR ERROR ERROR ERROR ERROR ERROR %s\n",Highlight,n->name,resetstr);
        }
        n=n->next;
    }
}
void printmyjobs(const user* u)
{
    char * me = getlogin();
    heading("MY JOBS");
    printf("No    state CPU  RAM     STARTING IN\n");
    while (u != NULL)
    {
        if (!strcmp(me,u->name))
        {
            const job * j=u->jobs;
            if (j == NULL)
            {
                printf("No jobs for me\n");
            }
            while (j != NULL)
            {
                printf("%i    %c %3i %5.2fGB",j->number,StatStr( j),j->corecount,(double)j->ramrequested/1024.0/1024.0);
                if (j->state==Q) //if job queued then call showstart to print the start time for the job (based on reported runtimes, job likely to start even earlier than this
                {
                    char* command = malloc(1024);
                    sprintf(command,"showstart %i | grep start | tr -s ' ' | cut -d ' ' -f4",j->number); //get time to start
                    FILE* pout=popen(command,"r");
                    char* datebuffer = malloc(100);//100 characters should be enough
                    fgets(datebuffer,100,pout);
                    strchr(datebuffer,'\n')[0]=0; //cut adds an extra newline - annoying
                    printf("  %11s\n",datebuffer);
                }
                else {printf("\n");}
            j=j->usernext;
            }
        }
        u=u->next;
    }
}
void printuser(const user* u)
{
    heading("USERS");
    printf("N  Name         Run  Queue  Running jobs             Queued jobs\n");
    const user* start=u;
    const int count = UserCount(u);
    for (int i=0;i<count;i++)
    {
        u=start;
        while (u != NULL)
        {
            if (UserNo(start,u)==i)
            {
                printf ("%s%i  %-10s  %4i   %4i  ",UserColourStr(i),i,u->name,u->runcount,u->queuecount);
                int i=0;
                int accum=0;
                const job * j = u->jobs;
                while(i<2 && j != NULL)
                {
                    if (j->state==R)
                    {
                    i ++;
                    accum+= printf("%ix%i ",j->number,j->corecount);
                    }
                    j=j->usernext;
                }
                if (j != NULL && j->state==R) {accum += printf ("...");}
                for (;accum<25;accum++) {printf(" ");} //fill in some white space
                i=0;
                j = u->jobs;
                while(i<2 && j != NULL)
                {
                    if (j->state==Q)
                    {
                    i ++;
                    accum+= printf("%ix%i ",j->number,j->corecount);
                                       }
                    j=j->usernext;
                }
                if (j != NULL && j->state==Q) {accum += printf ("...");}
                printf("%s\n",resetstr);
            }
            u=u->next;
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
    heading("QUEUES");
    printf("Name           Q Length     Q Ram  NextJob: ID        USER CPU         RAM \n");
    for (int i=0;i<foundqcount;i++)
    {
        const qstats q = queues[i];
        const job j = *q.nextjob;
             //qname      qCPU  qram           jno juser jcpu jram
        printf("%-14s     %4i  %6.2fGB       %6i  %10s  %2i  %8.2fGB\n",q.name,q.cpucount,(float)q.ram/1024.0/1024.0,
                j.number,j.owner->name,j.corecount, (float)j.ramrequested/1024.0/1024.0);
    }
    free(queues);
}
