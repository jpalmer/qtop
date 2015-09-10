#include <pbs_error.h> //pbs
#include <pbs_ifl.h>   //more pbs
#include <string.h> //strchr
#include <stdlib.h> //malloc
#include <stdio.h> //printf
#include <getopt.h> //long opts
#include <sys/types.h> //for types for getpwnam
#include <pwd.h>        //getpwnam
#include "qtop.h"
#include "print.h"
void insertJobToUserJobList (user * u,job* j)
{
    if (u->jobs==NULL) //special case for first job for a user
    { u->jobs=j;}
    else{ u->jobsend->usernext=j;}
    u->jobsend=j;
}
void AggregateUserInfo(user * u)
{
    while (u != NULL)
    {
        u->queuecount=0;
        u->runcount=0;
        const job* j = u->jobs;
        while (j != NULL)
        {
            if (j->state==Q) {u->queuecount += j->corecount;}
            else if (j -> state==R) {u -> runcount += j->corecount;}
            j=j->usernext;
        }
        u=u->next;
    }
}
//Get info from the equivalent of pbsnodes
node* GetNodeInfo(const int connection)
{
    struct batch_status* pbsnodes = pbs_statnode(connection,"",(struct attrl *)NULL,"");
    node* n=malloc(sizeof(node));
    node* cn=n;
    node* pn=n;
    while (pbsnodes != NULL)
    {
        cn->name = pbsnodes->name;
        cn->users_using_count=0;
        struct attrl* at = pbsnodes-> attribs;
        while (at != NULL)
        {
            if (!strcmp(at->name,"np")) //np contains available number of cores (different to OS reported number of cores nor medphys
            {
                cn->cores=atoi(at->value);
            }
            if (!strcmp(at->name,"state"))
            {
                if (strstr(at->value,"down")!= NULL  || strstr(at->value,"offline")!= NULL) {cn->up=1;} else {cn->up=0;}
            }
            if (!strcmp(at->name,"properties"))
            {
                cn->props=at->value;
            }
            if (!strcmp(at->name,"status"))
            {
                char* totmem = strstr(at->value,"totmem=")+7;
                char* availmem = strstr(at->value,"availmem=")+9;
                char* physmemc = strstr(at->value,"physmem=")+8;
                long long int physmem=strtoll(physmemc,NULL,10);
                cn->ramfree=physmem-strtoll(totmem,NULL,10)+strtoll(availmem,NULL,10);
                cn->physram=physmem;
                char* loadave=strstr(at->value,"loadave=")+8;
                cn->loadave=atof(loadave);
            }
            at=at->next;
        }
        pbsnodes=pbsnodes->next;
        if (pbsnodes != NULL)
        {        
            if (filternodes==NULL || strcmp(cn->props,filternodes)==0)
            {
                cn->next=malloc(sizeof(node));
                pn=cn;
                cn=cn->next;
            }
            else {pn->next=NULL;}
        } else {cn->next=NULL;}
    }
    return n;
}
user* adduser (user* u,char* name)
{
    if (u->name != NULL)
    {
        while (u->next != NULL)
        {u=u->next;}
        u->next=malloc(sizeof(user));
        u=u->next;
    }
    struct passwd* pdata = getpwnam(name);
    u->name=name;
    char* buf = malloc(sizeof(char)*20);
    strcpy(buf,pdata->pw_gecos);
    u->realname=buf;
    u->jobs=NULL;
    u->jobsend=NULL;
    u->runcount=0;
    u->queuecount=0;
    u->next=NULL;
    return u;
}
job* GetJobInfo(const int connection,node* n,user** u) //u is a second return value
{
    struct batch_status* qstat=pbs_statjob(connection,NULL,NULL,NULL);
    user* users=calloc(sizeof(user),1);
    *u=users;
    job* ret = calloc(sizeof(job),1);
    job* curjob=ret;
    while (qstat != NULL)
    {
        memset(curjob,0,sizeof(job)); //all properties have a default 0 value and this avoids valgrind complaints
        curjob->number = atoi(qstat->name);
        curjob->arrayid=-1;
        struct attrl* attribs=qstat->attribs;
        while (attribs != NULL)
        {
            if (!strcmp(attribs->name,"job_array_id"))
            {
                if (attribs->value!= NULL)
                {
                    curjob->arrayid=atoi(attribs->value);
                }
            }
            if (!strcmp(attribs->name,"job_state"))
            {
                char c = *(attribs->value);
                if      (c=='Q') {curjob->state = Q;} 
                else if (c=='R') {curjob->state = R;}
                else if (c=='C') {curjob->state = C;} 
                else if (c=='S') {curjob->state = S;}
                else {printf ("unknown status %s\n",attribs->value);}
            }
            if (!strcmp(attribs->name,"Resource_List")) 
            {
                if (!strcmp(attribs->resource,"nodes"))
                {
                    int nodect=atoi(attribs->value);
                    char* equals=index(attribs->value,'=');
                    int corespernode=equals==NULL ?1:atoi(equals+1); //sometimes jobs started with qsub -i have node corespernode - just assume it is one
                    curjob->corecount=nodect*corespernode;
                }
                if (!strcmp(attribs->resource,"mem"))
                {
                    curjob->ramrequested=strtoll(attribs->value,NULL,10)*1024L;//everyone should request in MB
                }
            } 
            if (!strcmp(attribs->name,"Walltime")) 
            {
                if (!strcmp(attribs->resource,"remaining"))
                {   //TODO: this does not quite accurately get seconds remaining
                    curjob->secondsremaining=atoi(attribs->value)*10; //where does the 10 come from?
                }
            }
            if (!strcmp(attribs->name,"Job_Owner"))
            {
                char * indat = index(attribs->value,'@');
                *indat=0;//replace @ with null terminator
                user* owner=findN(users,attribs->value);
                if (owner==NULL) {owner=adduser(users,attribs->value);}
                curjob->owner=owner;
                insertJobToUserJobList(owner,curjob);
            }
            if (!strcmp(attribs->name,"queue"))
            {
                curjob->queue=attribs->value;
            }
            if (!strcmp(attribs->name,"exec_host") && curjob -> state != C && curjob -> state != S)
            {
                //count the number of '/'
                char * str = attribs->value;
                int count=0;
                while (1) {str=strchr(str,'/');if (str==NULL){break;};str++;count++;}
                int i = 0;
                char * val=attribs->value;
                while (i<count)
                {
                    val=strchr(val,'n');
                    val[6]=0;
                    node* execnode=findN(n,val);
                    if (execnode != NULL) {execnode->users_using[execnode->users_using_count++]=curjob;}
                    //printf ("%s %i\n",execnode->name,execnode->users_using_count);
                    i++;
                    val[6]=' ';
                    val++;
                }
            } 
            attribs=attribs->next;
        }
        qstat=qstat->next;
        if (qstat != NULL)
        {
            curjob->next=malloc(sizeof(job));
            curjob=curjob->next;
        }
        else {curjob -> next=NULL;}
    }
    if (users->name==NULL) {*u=NULL;ret=NULL;} //no users found
    AggregateUserInfo(users);
    return ret;
}
void coalescejobs (job* j) //explot the fact that jobs are returned in order - if any array jobs they should be immediately after each other
{
    while (j != NULL)
    {
        job* next = j-> next;
        job* prev=j;
        while (next != NULL && next->number==j->number)  
        {
            if (next->state==j->state)
            {
                j->corecount+=next->corecount;
                prev->next=next->next; //effectively delete this sub-job and store the info in a parent job
                prev->usernext=next->usernext; //use this pointer once next is freed
                free(next);
                next=prev->usernext;
            }
            else
            {
                prev=next;
                next=next->next;
            }
        }
        j=j->next;
    }
}

void TestPBSFunc(const int connection)
{
    struct batch_status* bs = pbs_statjob(connection,NULL,NULL,NULL);
    while (bs != NULL)
    {
        printf("%s %s\n",bs->name, bs-> text);
        struct attrl* attr = bs-> attribs;
        while (attr != NULL)
        {
            printf("    %s-%s-%s\n",attr->name,attr->value,attr->resource);
            attr=attr->next;
        }
        bs=bs->next;
    }
}
char* filternodes=NULL;
int showfree=0;
int brief = 0;
struct option long_options[]= {{"help",no_argument,0,'h'},{"filter",required_argument,0,'f'},{"brief",no_argument,0,'b'},{"free",required_argument,0,'F'},{0,0,0,0}};
int main(int argc,char** argv)
{
    int c;
    while (1)
    {
        int option_index=0;
        c=getopt_long(argc,argv,"hf:",long_options,&option_index);
        if (c== -1) {break;}
        switch (c)
        {
            case 'h':
                printf(    "      qtop prints information about the usage of a PBS based cluster. "
                        "\n\n      First nodes are printed with theior load averages.  Xs are drawn in a colour corresponding to the user who requested a core on a node.  A grey bar shows the ram usage.  This bar is in black when swap is in use."
                        "\n\n      Next, information about each user is printed (running / queued jobs etc."
                        "\n\n      Then, information about your jobs is printed."
                        "\n\n      Finally, the number of runnable jobs for each subcluster is reported according to how many CPUs you need"
                        "\n\n      Currently testing a new layout in preparation for the new larger nodes"
                        "\n\n      options:"
                        "\n          --help print this string"
                        "\n          --filter ARG filter nodes to just a specific type (i.e. medphys/complex/cmt)"
                        "\n          --free ARG print free core count for a specific type (i.e. medphys/complex/cmt)"
                        "\n          -b show my total running + queued jobs"
                        "\n\n      Any questions, contact John Palmer (j.palmer@physics.usyd.edu.au)\n");
                exit(EXIT_SUCCESS);
            case 'f':
                filternodes=optarg;
                break;
            case 'F':
                filternodes=optarg;
                showfree=1;
                break;
            case 'b':
                brief=1;
                break;
        }
    }
    const int connection = pbs_connect("localhost");
   // TestPBSFunc(connection);
    if (connection<0) {printf("pbs error: %s\n",pbs_strerror(pbs_errno)); return EXIT_FAILURE;}
    user* users;
    node* n = GetNodeInfo(connection);
    job* j = GetJobInfo(connection,n,&users);
    SetupTerm();
    if (brief==1) {coalescejobs(j); printMyJobCount(users);return EXIT_SUCCESS;}
    if (showfree==1) {FreeCpu(n);return EXIT_SUCCESS;}
    printnode(n,users);
    coalescejobs(j); //so that when jobs are printed it looks nice
    printuser(users);
    printq(j);
    printmyjobs(users);
    PropStats(n);
    printfooter();
    return EXIT_SUCCESS;
}
