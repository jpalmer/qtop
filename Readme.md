Qtop
====

Provides some simple stats for a PBS based cluster.

Some parts are highly dependent on my local configuration.

Statistics provided
-------------------

Who is using which cores on various nodes

Memory in use and load averages for all nodes

Queue lengths

fancy coloured output!!

time until a job starts (using showstart)

Sample output
-------------
    Name     Load  CPU            MemFree   MemRequestable
    node01   0.02  ------------   30.44GB   31.43GB
    node02   0.04  ------------   28.10GB   31.43GB
    node03   0.01  ------------   31.09GB   31.43GB
    node04   0.05  ------------   31.10GB   31.43GB
    node05   0.02  ------------   31.06GB   31.43GB
    node06   0.00  ------------   31.07GB   31.43GB
    node07  10.00  1111111111--   30.65GB   31.43GB
    node08  10.06  1111111111--   30.60GB   31.43GB
    node09  10.00  1111111111--   30.52GB   31.43GB
    node10  10.00  1111111111--   30.60GB   31.43GB
    node11  10.00  1111111111--   30.61GB   31.43GB
    node12  10.05  1111111111--   30.66GB   31.43GB
    node13  10.00  1111111111--   30.66GB   31.43GB
    node14   0.01  2222222222--   31.07GB   31.43GB
    node15   9.01  1111111111--   30.69GB   31.43GB
    node16  10.00  1111111111--   30.58GB   31.43GB
    node31   0.00  2222222222--   31.08GB   31.42GB
    node32   0.00  2222222222--   31.08GB   31.42GB
    node33  10.00  1111111111--   30.60GB   31.42GB
    node34   0.00  2222222222--   31.05GB   31.42GB
    node35   0.00  2222222222--   31.09GB   31.42GB
    node21   4.66  33333333        9.86GB   15.67GB
    node22   2.01  33333333       12.07GB   15.67GB
    node23   8.00  44444444       14.85GB   15.67GB
    node24   8.00  44444444       14.94GB   15.67GB
    node25   5.64  55550000       11.89GB   14.11GB
    node26   7.99  44444444       14.98GB   15.67GB
    node27   8.00  44444444       14.92GB   15.67GB
    USERS
    N  Name         Run  Queue  Running jobs             Queued jobs
    0  ********       4    184  216347x4                 216347x184 
    1  ******       100      0  216346x100               
    2  *****         50      0  214966x50                
    3  ********      16     32  216314x8 216316x8        216317x8 216318x8 ...
    4  ****          32      0  216270x8 216271x8 ...    
    5  ******         4      0  216324x4                 
    QUEUES
    Name           Q Length     Q Ram  NextJob: ID        USER CPU         RAM 
    *****               216    0.39GB       216317    ********   8      0.00GB
    MY JOBS
    No    state CPU   RAM
    216347    C 8  0.39GB
    216347    R 4  0.39GB
    216347    Q 184  0.39GB

