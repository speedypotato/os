#ifndef H_HEADER
#define H_HEADER

#define DEFAULT_SEED UINT64_C(0xdeadbeefcafe2357)
#define TIME_UNIT_MS 64

//there are 9 ADDITIONAL threads,
//main takes the role of the high seller

typedef struct
{
    int arrivalTime;//since broadcast, event 1
    int serviceBeginTime;//since broadcast, even 2
    int serviceDuration;//+arrival -> event 3
    short seatIdx;
    short custIdent;
} TransactHistory;

static
int sortByArrival(const void* a, const void* b)
{
    //all should be < 64, no overflow
    return ((const TransactHistory *)a)->arrivalTime - ((const TransactHistory *)b)->arrivalTime;
}

#endif // H_HEADER
