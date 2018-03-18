#ifndef H_HEADER
#define H_HEADER

#define DEFAULT_SEED UINT64_C(0xdeadbeefcafe2357)
#define TIME_UNIT_MS 10

//there are 9 ADDITIONAL threads,
//main takes the role of the high seller

typedef struct
{
    int arrivalTime;//since broadcast, event 1

    short serviceBeginTime;//since broadcast, even 2
    short eventOrd;

    short serviceDuration;//+arrival -> event 3
    short seatIdx;

    short sellerTid;
    short custIdent;
} TransactHistory;

#endif // H_HEADER
