// Author: Erik Xu
// CS 149 Sec. 2, Spring 2018
// Implementation of HPF Non-preemptive Scheduling Algorithm

#include "hpf_non-preemptive.h"

// Data structure for HPF Non-preemptive Priority Queue Element
struct HPFNonpreQueueData
{
    uint8_t  id;       // id of the job
    uint16_t priority; // SRT doesn't need this but don't want to make more types.
    uint8_t  arrival;  // jobs with the same priority but earlier arrival time are executed first
};

// Priority Queue Comparison Struct for HPF
// Element is ordered by priority, followed by arrival time
struct HPFNonpreComparator
{
    bool operator() (HPFNonpreQueueData a, HPFNonpreQueueData b) const
    {
      if (a.priority > b.priority) return true;
      else if (a.priority == b.priority && a.arrival > b.arrival) return true;
      else if (a.priority == b.priority && a.arrival < b.arrival) return false;
      else return false;
    }
};

// Convert Job to HPF Non-preemptive queue element
HPFNonpreQueueData ConvertToHPFData(const Job& jb, unsigned id)
{
    HPFNonpreQueueData dat;   // object returned
    dat.id = id;              // store id into object
    dat.arrival = jb.arrival; // store arrival time into object
    dat.priority = jb.priority; // Calculate priority based on whether it is aging algorithm
    return dat;
}

// HPF Non-Preemptive Scheduling Algorithm
// Priority Queue (min. heap) is used to keep track of jobs with highest priority (1 is greatest)
AlgoRet hpf_non_preemptive(const Job *job, int njobs, PerJobStats *stats, char *gantt)
{
    // Priority Queue that sorts job based on priority
    PriorityQueue<HPFNonpreQueueData, HPFNonpreComparator> pque(njobs);
    int j = 0;          // number of jobs completed
    int q = 0;          // elapsed quanta
    unsigned id;        // ID of current running job

    // Start queue with the first job
    pque.push(ConvertToHPFData(job[j], j));
    j++;
    HPFNonpreQueueData *top = pque.ptr_top();

    // Add new jobs into queue
    // quit while loop if: 1) after quanta 99 or 2) 12 jobs have been completed
    while (q < QUANTA && (j <= njobs && pque.size() != 0)) {
      id = top->id;
      // fast forward if current quanta is before arrival of next job
      if (q < job[id].arrival)
      {
          // print '.' to signify waiting from current quanta until job arrival
          fill(gantt+q, gantt+job[id].arrival, '.');
          // set current quanta to job arrival time
          q = job[id].arrival;
      }

      // job completion time: current quanta + burst
      int const comptime = q + job[id].burst;

      // store stats for j^th job
      // current quanta (beginning of processing time)  and completion time
      stats[id] = {q, comptime};

      // print letter to signify job is running
      // j ranges from 1 to 12, 1 + A = B, 2 + A = C, etc.
      fill(gantt+q, gantt+comptime, id+'A');
      q = comptime; // set current quanta to completion time of current job
      pque.pop();   // take the completed job

      // put into the queue of jobs that have arrived while the previous job was running
      for (int i = j; i < njobs; ++i) {
        // check if the arrival of the job
        if (job[i].arrival <= q) { //<-------
          pque.push(ConvertToHPFData(job[i],i));
          j++;
        }
        // if no job has arrived, put the next job in (we'll account for this in the fast forward, line 424)
        else {
            if (pque.empty()) {
              pque.push(ConvertToHPFData(job[i], i));
              j++;
            }
          break; // stop checking for jobs once the arrival is greater than the current quanta
        }
      }
      top = pque.ptr_top();
    } // end of while()

    // return jobs completed, elapsed quanta
    if (pque.size() == 0) {
      return {j, q};
    } else {
      return {j-int(pque.size()), q};
    }
} // end of hpf_non_preemptive()

/*
    (Jonathan):
    I am unsure about this, but this may be incorrect for some input:

    If before line 82 say q is 80 and j = njobs-3
    and that the three remaining jobs have arrival times and bursts of
    {71, 1},
    {72, 1},
    {73, 1}
    So they all get inserted b/c arrival times < q=80

    Then j==njobs and the algo terminates.
    But QUANTA is 100, and can still serve these jobs at times
    80,
    81,
    82
    Then last comptime is 83

    Stats aren't reflected.
*/
