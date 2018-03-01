#include "syms.h"

typedef struct JobID
{
    Job job;
    char id;
} JobID;

AlgoRet sjf(const Job* job, int njobs, PerJobStats* stats, char* gantt) {

    JobID jobs_id[njobs];
    for(int j = 0; j < njobs; j++)
    {
        jobs_id[j].job = job[j];
        jobs_id[j].id = 'A' + j;
    }

    auto cmp = [](JobID left, JobID right) { return (left.job.arrival) > (right.job.arrival);};
    std::priority_queue<JobID, std::vector<JobID>, decltype(cmp) > sjfQueue(cmp); // Priority queue for shortest job first

    int j = 0; // Keeps track of job count
    int q = 0; // Elapsed quanta
    JobID jb;
    char eq;

    while(j < njobs || !sjfQueue.empty() && q < 100)
    {
        while(q >= jobs_id[j].job.arrival && j < njobs)
        {
            sjfQueue.push(jobs_id[j]);
            j++;
        }
        if(!sjfQueue.empty())
        {
            jb = sjfQueue.top();
            sjfQueue.pop();

            int response;
            int arrival = jb.job.arrival;
            int burst = jb.job.burst;
            int wait = q - arrival;
            int turnaround = wait + jb.job.burst;
            gantt[q++] = jb.id;
            for(int i = 0; i <= burst; i++)
            {
                gantt[q++] = '.';
            }
            //printf("Job %c:\tArrival: %d\tBurst: %d\tWait:%d\tTurnaround:%d\tQuantum:%d\n", jb.id, arrival, burst, wait, turnaround, q);
        }
        q++;
    }
    return {j, q};
}
