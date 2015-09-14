#include <sys/ioctl.h>
#include <unistd.h> //getlogin
#include <string.h> //strcmp
#include <stdio.h> //printf
#include <time.h>  //maketime
#include <stdlib.h> //malloc
#include <math.h>    //ceil
#include <sys/types.h>
#include <pwd.h>
#include "qtop.h" //examples of functions needed are incomplete
#include "print.h"


//globals
const char* colours[7]  =   {"\x1B[41;30m", "\x1b[42;30m", "\x1b[43;30m", "\x1b[44;30m", "\x1b[45;30m", "\x1b[46;30m",""};
const char* fgcolours[7]=   {"\x1B[31m", "\x1b[32m", "\x1b[33m", "\x1b[34m", "\x1b[35m", "\x1b[36m",""};
const char* basecols[]  =   {"\x1b[32m","\x1b[34m","\x1b[31m"};
const char* Highlight   =   "\x1b[31m";
const char* bold        =   "\x1b[1m";
const char* underline   =   "\x1b[4m";
const char* resetstr    =   "\x1b[0m";
const char* boxon       =   "\x1b(0";
const char* boxoff      =   "\x1b(B";
const char* gray        =   "\x1b[47m";
const char* black        =  "\x1b[40m";
char boxchars[]         =   {0x71,  0x74,       0x75       };
typedef enum                {dash,  leftedge,   rightedge  } boxenum;
const char * headingfmt =   "\x1b[48;5;7;30;4m";
char * me = NULL;
int twidth = 1;
void heading_(const char* s) {printf("%s%s%s\n",headingfmt,s,resetstr); }
int  heading_nr(const char* s) {return  printf("%s%s",headingfmt,s) - strlen(headingfmt); }
int  heading_n(const char* s) {int r = heading_nr(s);printf(resetstr);return r;}
void heading_fill(const char* s) {for (int c = heading_nr(s);c<twidth;c++){putchar(' ');}printf(resetstr);}
void SetupTerm()
{
 /*   if(!isatty(fileno(stdout)))
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
        black="";
        boxchars[0]='-';boxchars[1]='|';boxchars[2]='|';
        headingfmt="";
    }
    else
    {*/
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        twidth=w.ws_col;
   // }
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
            if (!strcmp(cn->props,props[i].propname)) {found=1;break;}
        }
        if (found==0) {props[propcounter].propname=cn->props;propcounter++;}
        cn=cn->next;
    }
    *p=props;
    return propcounter;

}
void setme()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    me = pw->pw_name;
}
int UserNo (const user* u,const user* cu) //algorithm for allocating numbers is currently n^2 - could make it better with sorting, but typically number of users is small, so current method is simpler
{
    if (me == NULL)
    {
        setme();
    }
    if (!(strcmp(cu->name,me))) {return 0;}
    const user* start = u;
    int i=1; //me always gets user 0
    int mycount=cu->runcount + cu ->queuecount;
    while (start != NULL)
    {
        int scount=start->runcount + start -> queuecount;
        if (        scount>mycount //more cores that the other
                ||  (scount==mycount && (strcmp(start->name,cu->name)>0)))                //or alphabetical if cores tied
        {if (strcmp(start->name,me)) {i++;}} //complex ordering based on number of queued jobs and running jobs then alphabetical
        start=start->next;
    }
    return i;
}
const char* UserColourStr(const int userno,const int fg)
{
    if (fg==0)
    {
        if (userno>5) {return fgcolours[6];} else {return fgcolours[userno];}
    }
    else
    {
        if (userno>5) {return colours[6];} else {return colours[userno];}
    }
}
int UserCount(const user* u)
{
    if (me == NULL) { setme();}
    int ret = 1;
    while(u != NULL) {if (strcmp(u->name,me)){ret++;}u=u->next;}
    return ret;
}
int HighLoadNode = 0;
void actuallyprintnode (const node* const cn,const int propcount, const propinfo* const props,const user* const u)
{
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
        printf("%s%s%s %s%5.2f%s ",nodecol,cn->name,resetstr,cn->loadave > (float)cn->users_using_count+1.0?(HighLoadNode++, Highlight):"",cn->loadave,resetstr);
        printf(boxon);
        putchar(boxchars[leftedge]);
        int i=0;
        for (;i<cn->cores;i++)
        {
            const char* colorstr = resetstr;
            char entry = boxchars[dash];
            if (i< cn-> users_using_count)
            {
                int myuserno=UserNo(u,cn->users_using[i]->owner);
                colorstr = UserColourStr(myuserno,0);
                entry='X';
            }
            const char* bg = "";
            if (cn->ramfree < 0) {bg=black;}
            else if (((float)(cn->physram -cn->ramfree))/ ((float)cn->physram) > ((float)(i+1))/((float)cn->cores) ) {bg=gray;}
            printf ("%s%s%c%s",colorstr,bg,entry,resetstr);
        }
        printf("%s",resetstr);
        putchar(boxchars[rightedge]);
        printf(boxoff);
        for (;i<MINMAXCPUS;i++) {putchar(' ');} //blanks
    }
    else
    {
        printf("%s%s  ERROR ERROR ERROR  %s",Highlight,cn->name,resetstr);
    }
}
void printnode(const node* n,const user* u)
{
    const char* First = "Name    Load Usage          ";
    const char* others = "Name    Load Usage          ";
    int width=(twidth< (int)strlen(First))?1:(twidth-strlen(First))/strlen(others) + 1; //magic numbers related to lengths of strings below.
    heading_fill(First);
    printf("\n");
    propinfo* props;
    int propcount=GetPropinfo(n,&props);
    const node** nodes  = calloc(sizeof(node*),width+1); //to gurantee that nodes[1] exists
    int nodecount = 0;
    const node* dummy = n;
    while (dummy != NULL) {
        if (dummy->cores <= MINMAXCPUS) {nodecount++;}
        dummy=dummy->next;
        }
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
        do
        {dummy= dummy-> next;} while  ( dummy != NULL && dummy->cores > MINMAXCPUS);
        c++;
    }
    int colindex=0;
    int printed = 0;
    node* bign=nodes[0];
    do
    {
        if (bign->cores > MINMAXCPUS) {actuallyprintnode(bign,propcount,props,u);printf("\n");}
        bign=bign->next;
    } while (bign != NULL);
    int sum = heading_n(First);
    for (int k=1;k<width;k++){sum += heading_n(others);}
    for (;sum<twidth;sum++) {heading_n(" ");}
    printf("\n");
    while (printed != nodecount)
    {
        const node* cn = nodes[colindex];

        if (cn->cores <= MINMAXCPUS) {actuallyprintnode(cn,propcount,props,u);}
        nodes[colindex]=nodes[colindex]-> next;
        if (colindex+1==width) {printf("\n");colindex=0;}else {printf(" ");colindex++;} //either print a serperator or a newline
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
                    if (pout)
                    {
                        char* datebuffer = malloc(100);//100 characters should be enough
                        char * s = fgets(datebuffer,100,pout); //TODO: the strchr on the next line might return null - need tto check
                        if (s)
                        {
                            strchr(datebuffer,'\n')[0]=0; //cut adds an extra newline - annoying
                            printf("  %11s\n",datebuffer);
                        }
                        else
                        {
                            printf("error with showstart\n");
                        }
                        free(datebuffer);
                    }
                    else {printf("unable to run showstart");}
                    free(command);
                }
                else {printf("\n");}
            j=j->usernext;
            }
        }
        u=u->next;
    }
}
void printSomeJobs(const job* j, const jobstate s,const int count)
{
    int i = 0;
    int accum=0;
    while(j != NULL) //print the first 2 jobs in some state
    {
       if (j->state==s)
        {
            if (i==0)
            {
                if (s==R)
                {
                    accum += printf("    Running (%4i)| ",count);
                }
                else if (s==Q)
                {
                    accum += printf("    Queued  (%4i)| ",count);
                }
                else if (s==S)
                {
                    accum += printf("    Suspended     | ");
                }
            }
            accum += printf("%ix%i ",j->number,j->corecount);
            i++;
        }
        j=j->usernext;
    }

    if (i!= 0)
    {
        for (;accum<(twidth);accum++) {printf(" ");} //fill in some white space
        printf("\n");
    }
 //   while (j != NULL) {if (j->state==s) {accum += printf ("..");break;}j=j->next;}
}
void printuser(const user* u)
{
    if (u != NULL)
    {
        heading_fill("Name              | Jobs");
        const user* start=u;
        const int count = UserCount(u);
        for (int i=0;i<count;i++)
        {
            u=start;
            while (u != NULL)
            {
                if (UserNo(start,u)==i)
                {
                    int initial = printf ("%s%-18s| ",UserColourStr(i,1),u->realname) - strlen(UserColourStr(i,1));
                    for (;initial<(twidth);initial++) {printf(" ");} //fill in some white space
                    printf("\n");
                    printSomeJobs(u->jobs,R,u->runcount);
                    printSomeJobs(u->jobs,Q,u->queuecount);
                    printSomeJobs(u->jobs,S,0);
                    printf("%s",resetstr);
                }
                u=u->next;
            }
        }
    }
}
void printMyJobCount(const user* u)
{
    if (u != NULL)
    {
        const user* start = u;
        while(u != NULL && UserNo(start,u) != 0)
        {
            u =u->next;
        }
        if (u != NULL)
        {
            int jcount = 0;
            const job* j  = u -> jobs;
            while(j != NULL) {if(j->state != C){jcount+=j->corecount;}j=j->usernext;}
            printf("%i",jcount);
        }
        else
        {
            printf("0");
        }
    }
}
void printq(const job* j)
{
    int foundqcount=0;
    qstats * queues = malloc (sizeof(qstats)* MAX_QUEUES);
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
    for (int i=2;i<16;i+=2) {c+=printf("%3i ",i); }
    for (int i=16;i<=MAXCPUS;i+=4) {c+=printf("%3i ",i); }
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
                for (int j=2;j<16;j+=2) {props[i].free[j/2]+= freecpus/j; }
                for (int j=16;j<=MAXCPUS;j+=4) {props[i].free[8+(j-16)/4]+= freecpus/j; }
            }
            cn=cn->next;
        }
        //hacky method to shorten the group names to make the text neater
        if (strchr(props[i].propname,',') != NULL)
        {
            strchr(props[i].propname,',')[0] = 0;
        }
        printf("%s%-10s |",basecols[i],props[i].propname);
        for (int j=0;j<1+16/2  + (MAXCPUS-16)/4 ;j++) {printf("%3i ",props[i].free[j]);}
        printf("%s\n",resetstr);
    }
}
void FreeCpu(const node* n)
{
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
        printf("%i",props[i].free[0]);
    }
}
void printfooter()
{
    if (HighLoadNode>0)
    {
        printf("============================\n");
        printf("%s%s\n",Highlight,"ONE OR MORE NODES HAVE A HIGH LOAD AVERAGE");
        printf("%s\n","THIS RESULTS IN DEGRADED PERFORMANCE");
        printf("%s\n","MAYBE YOU NEED TO USE `-singleCompThread` WITH MATLAB?");
        printf("%s",resetstr);
        printf("============================\n");
    }
}
