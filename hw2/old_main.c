/*
	useful? how start something before it arrives?
	https://www.tutorialspoint.com/operating_system/os_process_scheduling_algorithms.htm
	this better:
	https://www.cs.uic.edu/~jbell/CourseNotes/OperatingSystems/5_CPU_Scheduling.html
	defs:
	https://www.geeksforgeeks.org/gate-notes-operating-system-process-scheduling/
	
	one of guys write report, others review
	5(6, HPF has two versions, with each ver using 2 pq's each) algos?
	how many jobs in total? (cpu can't be idle for more than 2 quanta?)
	how many jobs can cpu work on at once? (can bars in time chart be on top of eachother?)
	
	thinking of display:
	#=2 quanta
	.=1 quanta
	or make term super wide and paste to diff doc
	
	use specific seed and test that cpu never idle for >2 quanta (dif between adj in sorted by arival time <=2, ?)
	^or prefill arrival time ourselves, then rand gen duration and priority
	^or arrival time = prev arrival time + randInclusive(1,2)//NO
	>>>keep track of sum of total durations needed (gen dur first), and do startTime = prevStart + randInclusive(max(sumDur+2,99))
	
	CPU can only run 1 proc at a time, so horizonal stretches in graph can never overlap
	
	A|*  *
id	B| *  *
	C|  *  *
	  0123456789
		time
	

	
preempt:
interrupt and resume later

Turnarund time:
Time required for a particular process to complete, from 
submission time to completion. It is equal to the sum total of
Waiting time and Execution time.

Response time:
The time taken in a program from the issuance of a 
command to the commence beginning of a response to that command
(i.e., the time-interval between submission of a request, and the 
start of execution)


time chart for only 1 run?, but stats as average of five?
Also:
-waittime, response, and turnaround for each of ~10 preocesses
-throughput for algo as whole (wouldnt this be the same? (10 procs)/(100 time units))

FCFS (non-preemptive easiest)
service time is when actually can start, sum of prevs.
*/

struct Process
{
	int arrival;
	int burst;
	int priority;
	//int id;//name for time chart?
};

int main(void)
{
	
	
	
	return 0;
}

#if 0
#define ROFL(X) do{ \
double const dAvg##X = double(X##Sum)/NJOBS; \
aa_##X += dAvg##X; \
printf("%3d/%d = %6f : Average " #X "\n", X##Sum, NJOBS, dAvg##X); }while(0);

ROFL(wait);
ROFL(response);
ROFL(turnaround);

#undef ROFL
#endif
/*
	one of guys write report, others review
	5(6, HPF has two versions, with each ver using 2 pq's each) algos?
	how many jobs in total? (cpu can't be idle for more than 2 quanta?)
	how many jobs can cpu work on at once? (can bars in time chart be on top of eachother?)

	thinking of display:
	#=2 quanta
	.=1 quanta
	or make term super wide and paste to diff doc

	use specific seed and test that cpu never idle for >2 quanta (dif between adj in sorted by arival time <=2, ?)
	^or prefill arrival time ourselves, then rand gen duration and priority
	^or arrival time = prev arrival time + randInclusive(1,2)//NO
	>>>keep track of sum of total durations needed (gen dur first), and do startTime = prevStart + randInclusive(max(sumDur+2,99))

	CPU can only run 1 proc at a time, so horizonal stretches in graph can never overlap
      ABCABC
	A|*  *
id	B| *  *
	C|  *  *
	  0123456789
		time



preempt:
interrupt and resume later

Turnarund time:
Time required for a particular process to complete, from
submission time to completion. It is equal to the sum total of
Waiting time and Execution time.

Response time:
The time taken in a program from the issuance of a
command to the commence beginning of a responseonse to that command
(i.e., the time-interval between submission of a request, and the
start of execution)


time chart for only 1 run?, but stats as average of five?
Also:
-waittime, responseonse, and turnaround for each of ~10 preocesses
-throughput for algo as whole (wouldnt this be the same? (10 procs)/(100 time units))

FCFS (non-preemptive easiest)
service time is when actually can start, sum of prevs.



1-4, small range priority queue?

shuffle bag notes:
    */
	
	//these priority queue classes my be useful, but you don't have to use them
//they have the same methods as std::priority_queue.
//top(), pop(), push(), size() and empty() or of interest
//typedef PQBase<PriorityCompT> MostImportantQueue;
//typedef PQBase<RemTimeCompT> ShortestTimeQueue;
struct PriorityCompT{ bool operator()(Job a, Job b)const{ return a.priority>b.priority; } };
struct RemTimeCompT { bool operator()(Job a, Job b)const{ return a.burst   >b.burst; } };

//    void top_updated()
//    {
//        //This could be done more efficiently, but I don't
//        //want to paste my own heap code.
//        const Job x = this->top();
//        this->pop();
//        this->push(x);
//    }