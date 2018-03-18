    //For printing of event list:
    //TODO: print what sellers could not service all their customers due to ncps>10
    //TODO: make format compact 1 per line, list what columns meen at start, like previous hw
    //TODO: arrival time of cust is missing

/*
    cond_wait, cond_broadcast notes:

    In order for a thread to receive a signal, must already
    be waiting on the cv. To wait on the cv, the thread must own the mutex
    prior to the wait() call. When wait is called, the mutex is released,
    (can think immediately?) before wait() will return. A thread in wait() has 2 states:
    first is waiting on a cv signal. If a broadcast is issued, all threads advance past first state.
    Second state is acquiring the mutex, which only 1 thread can do, and wait() returns for it. In case of broadcast,
    All other threads are still in second state.

    Are the assignments specs busted? Thread 0 should not be the only one that
    has the ability to broadcast?

    Just want a barrier for initial start?

    And after that, why not just use mtx to protect shared data structure?
    Don't need a condition?

    "This is why we need conditional variable,
    when you're waiting on a condition variable,
    the associated mutex is not hold by your thread until it's signaled."
*/

#include "header.h"
#include "util.h"

//TransactHistory* histories;
int gNumServed[10];

TransactHistory ghist[100];//zeroed
#define isSlotEmpty(x) ((x).serviceDuration == 0) // all should have a time longer than, High seller has min 1 "min" (64 mis)

int bcastCounter = 10;
int gEventNo = 0;//for sequence

typedef struct
{
    int arrival;
    int burst;
} InitData;

static
int initDataCompare(const void* a, const void* b)
{
    //no overflow
    return ((const InitData *)a)->arrival - ((const InitData *)b)->arrival;
}

uint64_t seedval = DEFAULT_SEED;
int ncps=7;//same num customers per each seller

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned gTimeOfBroadcastMs;//set once in main, then const

//single lock controls all 5 of these
int gMidAlternate = 0;

int giA = 0;
int giB = 49;
int giC = 50;
int giD = 99;
/*
A->
    <-B
C->
    <-D
*/
typedef int (*fp_sell_t)(void);
//for all funcs, thread has lock before hand
//things that go away from mid, toward front or back
//can return -1: all seats full
int sell_mid_to_front(void)
{
    if (giB >= giA)
        return giB--;
    else
        return -1;//sold out
}

int sell_back_to_mid(void)
{
    if (giD >= giC)
        return giD--;
    else
        return sell_mid_to_front();
}

int sell_mid_to_back(void)
{
    if (giC <= giD)
        return giC++;
    else
        return -1;//sold out
}

int sell_front_to_mid(void)
{
    if (giA <= giB)
        return giA++;
    else
        return sell_mid_to_back();
}

int sell_mid_alternate(void)
{
    if ((gMidAlternate ^= 1) != 0)
    {
        int const res = sell_mid_to_back();
        return res > -1 ? res : sell_mid_to_front();
    }
    else
    {
        int const res = sell_mid_to_front();
        return res > -1 ? res : sell_mid_to_back();
    }
}

typedef struct
{
    fp_sell_t fpSell;
    short minBurstMs;
    short maxBurstMs;
} ThreadParams;

ThreadParams getParams(int tid)
{
    if (tid == 0)       return (ThreadParams){sell_front_to_mid,  1u*TIME_UNIT_MS, 2u*TIME_UNIT_MS};
    else if (tid < 4)   return (ThreadParams){sell_mid_alternate, 2u*TIME_UNIT_MS, 4u*TIME_UNIT_MS};
    else                return (ThreadParams){sell_back_to_mid,   4u*TIME_UNIT_MS, 7u*TIME_UNIT_MS};
}

void* thread_func(void *ctx)
{
    int const tid = (int)(uintptr_t)ctx;
    ThreadParams const params = getParams(tid);
    const int ntotal = ncps;
    InitData custs[99];//NMAX
    //init customer arrivals and time to service
    {
        random32_t rng;
        srandom32(&rng, seedval*tid + tid);//i dunno... as aside pcg random has concept of multiple streams

        const int ntotal = ncps;
        InitData custs[99];//NMAX

        for (int i=0; i<ntotal; ++i)
            custs[i].arrival = random32_xrange(&rng, 0, 60*TIME_UNIT_MS),//arrive within the 60 "minutes"
            custs[i].burst = random32_xrange(&rng, params.minBurstMs, params.maxBurstMs+1);

        QSORT_TYPED(custs, custs+ntotal, initDataCompare);//sort asc arrival
    }

    pthread_mutex_lock(&mutex);
    if (--bcastCounter == 0)//memory dec guarded by above mutex
    {
        printf("9 of 10 threads waiting on cv, mutex owner thread %u will broadcast\n", tid);
        gTimeOfBroadcastMs = get_millisecs_stamp();
        pthread_cond_broadcast(&cond);
    }
    else
    {
        printf("Thread %u waiting on cv\n", tid);
        pthread_cond_wait(&cond, &mutex);
    }
    //mtx now acquired
    printf("Thread %u passed wait\n", tid);
    //for (int i=0; i<ntotal; ++i) printf("{%d, %d} ", custs[i].arrival, custs[i].burst); putchar('\n');
    pthread_mutex_unlock(&mutex);//do this now in case cust not arrived yet, dont sleep holding lock!

    int nsold=0;
    int q = get_millisecs_stamp() - gTimeOfBroadcastMs;

    for (;;)
    {
        int const arriv = custs[nsold].arrival;
        int const brst = custs[nsold].burst;

        if (q < arriv)
        {
            millisleep(arriv - q);
        }

        pthread_mutex_lock(&mutex);//enter critical section
        unsigned const i = params.fpSell();//seatpos
        int const eventOrder = gEventNo++;
        pthread_mutex_unlock(&mutex);//unlock right after

        if (i >= 100u)//concert full
            break;
        q = get_millisecs_stamp() - gTimeOfBroadcastMs;
        //index i is unique
        ghist[i].arrivalTime = arriv;
        ghist[i].serviceBeginTime = q;
        ghist[i].custIdent = nsold;
        ghist[i].sellerTid = tid;
        ghist[i].serviceDuration = brst;
        ghist[i].eventOrd = eventOrder;
        ghist[i].seatIdx = i;//duh, but will eventually sort the thing to print events in a list
        //do "work" to make this seam less silly. some kind of i/o...
        millisleep(brst);
        q += brst;
        if (++nsold == ntotal)
            break;
    }

    gNumServed[tid] = nsold;
    printf("Thread %u exiting\n", tid);
    return NULL;
}

static void fprint64x(uint64_t val);
static void printSeatChart(void);
static void printEventList(void);//this does an inplace sort, destroying matrix fmt

int main(int argc, char **argv)
{
    if (argc>1)
    {
        if (argc<=3)
        {
            ncps = atoi(argv[1]);
            if (ncps<=0 || ncps>=100)
            {
                puts("N must be in [1 to 99]");
                return 0;
            }

            if (argc==3)
            {
                seedval = strtoull(argv[2], NULL, 0);//0: determine base, eg 0x prefix
                if (!seedval)
                {
                    puts("invalid seed, using default");
                    seedval = DEFAULT_SEED;
                }
            }
        }
        else
        {
            printf("Usage: %s [N customers] [rng seed]\n", argv[0]);
            return 0;
        }
    }

    printf("n: customers per seller = %u\nseed = 0x", ncps);
    fprint64x(seedval);
    putchar('\n');


    pthread_t addthreads[9];//main is hi seller, so only 9 additional

    for (unsigned tid = 1; tid<10; ++tid)//maybe each thread should have dif thread func...
        xcreate_pthread_defattr(addthreads+tid-1, thread_func, (void *)(uintptr_t)tid);


    thread_func((void *)0);//run hi seller on main, make use of it

    for (int i=0; i<9; ++i)
        pthread_join(addthreads[i], NULL);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    puts("\noutput...\n");

    printSeatChart();//do before if no copy data
    printEventList();

    return 0;
}

static
int evCompare(const void* a, const void* b)
{
    return ((const TransactHistory *)a)->eventOrd - ((const TransactHistory *)b)->eventOrd;
}

static
TransactHistory* removeEmpty(TransactHistory* it, TransactHistory*const end)
{
    TransactHistory *w = it;//write pos
    for ( ; it!=end; ++it)
    {
        if (!isSlotEmpty(*it))
            *w++ = *it;
    }
    return w;
}

static unsigned tidToSellerChars(unsigned tid)
{
    if (tid == 0)       return 'H' | (unsigned)'A'<<8;
    else if (tid < 4)   return 'M' | (unsigned)('A'-1u+tid) << 8;
    else                return 'L' | (unsigned)('A'-4u+tid) << 8;
}

static void printEventList(void)//this does an inplace sort, destroying matrix fmt
{
    TransactHistory *const end = removeEmpty(ghist+0, ghist+100);
    QSORT_TYPED(ghist+0, end, evCompare);//destroyed, copy before if need do other things
    int const nSoldAll = end - ghist;

    for (int i=0; i!=nSoldAll; ++i)
    {
        TransactHistory const x = ghist[i];
        unsigned const cc = tidToSellerChars(x.sellerTid);
        printf("Reservation %2d made at time %3d in slot %2d\n", i, x.serviceBeginTime, x.seatIdx);
        printf("\"Sealing the deal\" took at least %3d ms\n", x.serviceDuration);
        printf("Seller id: %2d, %c%c. Cust id to specific seller %2d\n\n", x.sellerTid, (char)cc, (char)(cc>>8), x.custIdent);
    }

    if (nSoldAll == 100) puts("All 100 seats were filled");
    else                 printf("Not enough buyers to sell out, %d seats unfilled\n", 100-nSoldAll);

    //TODO: print what sellers could not service all their customers due to ncps>10
    //TODO: make format compact 1 per line, list what columns meen at start, like previous hw
    //TODO: arrival time of cust is missing
}

static
void fprint64x(uint64_t val)
{
    char buf[23], *p=buf;//23, 16 hex digits, 8 bytes -> 7 seps
    int hpos=15;
    unsigned hval;
    goto enter;
    do{
        *p++ = ':';
    enter:
        hval = (val >> (hpos-- * 4u)) & 15;
        *p++ = hval + (hval<10u ? '0' : 'a'-10);

        hval = (val >> (hpos-- * 4u)) & 15;
        *p++ = hval + (hval<10u ? '0' : 'a'-10);
    } while (hpos > -1);

    fwrite(buf, 1, 23, stdout);
}

//"21MB01:555" means: 21'st sold to Mid seller B's customer id 1 at 555
static void printSeatChart(void)
{
    enum{Rows = 10, Width = 10};
    const TransactHistory *prow = ghist, *const end = ghist + Rows*Width;
    for (; prow!=end; prow+=Width)
    {
        int j=0;
        goto enter;

        do {
            putchar(' ');
        enter: (void)0;
            TransactHistory const x = prow[j];
            if (!isSlotEmpty(x))
            {
                unsigned const cc = tidToSellerChars(x.sellerTid);
                printf("%02d%c%c%02d:%03d", x.eventOrd, (char)cc, (char)(cc>>8), x.custIdent, x.serviceBeginTime);
            }
            else
                fputs("...none...", stdout);
        }while (++j != Width);

        putchar('\n');
    }
    putchar('\n');
}
//jesus christ my eyes,
//neat addition: add ansi esc sequences to add some color alternation?
/*

n: customers per seller = 7
seed = 0xde:ad:be:ef:ca:fe:23:57
Thread 3 waiting on cv
Thread 6 waiting on cv
Thread 7 waiting on cv
Thread 0 waiting on cv
Thread 9 waiting on cv
Thread 5 waiting on cv
Thread 4 waiting on cv
Thread 1 waiting on cv
Thread 2 waiting on cv
9 of 10 threads waiting on cv, mutex owner thread 8 will broadcast
Thread 8 passed wait
Thread 3 passed wait
Thread 6 passed wait
Thread 7 passed wait
Thread 0 passed wait
Thread 9 passed wait
Thread 5 passed wait
Thread 4 passed wait
Thread 1 passed wait
Thread 2 passed wait
Thread 8 exiting
Thread 1 exiting
Thread 5 exiting
Thread 3 exiting
Thread 0 exiting
Thread 2 exiting
Thread 7 exiting
Thread 4 exiting
Thread 9 exiting
Thread 6 exiting

output...

12HA00:110 16HA01:161 26HA02:236 31HA03:302 54HA04:511 58HA05:531 67HA06:596 ...none... ...none... ...none...
...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none...
...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none... ...none...
...none... ...none... ...none... ...none... ...none... ...none... ...none... 69LA06:599 68LF06:596 66MB06:595 <--wrap handled
63MB05:572 61MB04:549 59MA06:544 50MA05:426 46MA04:384 38MA03:345 35MA02:315 24MB01:224 19MA00:179 03MB00:033
01MC00:028 07MC01:063 23MA01:218 25MC02:224 37MB02:325 41MC03:356 49MC04:423 56MB03:526 60MC05:545 62MC06:568 <- second seat reserved 01MC00:028
65LD06:584 64LC06:579 57LB06:529 55LE06:512 53LB05:468 52LE05:443 51LF05:442 48LA05:418 47LB04:416 45LD05:383
44LE04:378 43LC05:365 42LB03:365 40LA04:353 39LF04:351 36LE03:317 34LC04:314 33LF03:306 32LB02:306 30LA03:277
29LE02:260 28LC03:256 27LD04:241 22LE01:215 21LD03:195 20LC02:187 18LA02:173 17LF02:168 15LD02:153 14LB01:140
13LF01:116 11LA01:109 10LC01:107 09LD01:095 08LB00:087 06LE00:051 05LF00:041 04LD00:035 02LC00:032 00LA00:027 <- first seat reserved

*/

/*
Reservation  0 made at time  27 in slot 99
"Sealing the deal" took at least  51 ms
Seller id:  4, LA. Cust id to specific seller  0

Reservation  1 made at time  28 in slot 50
"Sealing the deal" took at least  33 ms
Seller id:  3, MC. Cust id to specific seller  0

Reservation  2 made at time  32 in slot 98
"Sealing the deal" took at least  68 ms
Seller id:  6, LC. Cust id to specific seller  0

Reservation  3 made at time  33 in slot 49
"Sealing the deal" took at least  23 ms
Seller id:  2, MB. Cust id to specific seller  0

Reservation  4 made at time  35 in slot 97
"Sealing the deal" took at least  52 ms
Seller id:  7, LD. Cust id to specific seller  0

Reservation  5 made at time  41 in slot 96
"Sealing the deal" took at least  69 ms
Seller id:  9, LF. Cust id to specific seller  0

Reservation  6 made at time  51 in slot 95
"Sealing the deal" took at least  43 ms
Seller id:  8, LE. Cust id to specific seller  0

Reservation  7 made at time  63 in slot 51
"Sealing the deal" took at least  30 ms
Seller id:  3, MC. Cust id to specific seller  1

Reservation  8 made at time  87 in slot 94
"Sealing the deal" took at least  53 ms
Seller id:  5, LB. Cust id to specific seller  0

Reservation  9 made at time  95 in slot 93
"Sealing the deal" took at least  58 ms
Seller id:  7, LD. Cust id to specific seller  1

Reservation 10 made at time 107 in slot 92
"Sealing the deal" took at least  69 ms
Seller id:  6, LC. Cust id to specific seller  1

Reservation 11 made at time 109 in slot 91
"Sealing the deal" took at least  64 ms
Seller id:  4, LA. Cust id to specific seller  1

Reservation 12 made at time 110 in slot  0
"Sealing the deal" took at least  18 ms
Seller id:  0, HA. Cust id to specific seller  0

Reservation 13 made at time 116 in slot 90
"Sealing the deal" took at least  52 ms
Seller id:  9, LF. Cust id to specific seller  1

Reservation 14 made at time 140 in slot 89
"Sealing the deal" took at least  64 ms
Seller id:  5, LB. Cust id to specific seller  1

Reservation 15 made at time 153 in slot 88
"Sealing the deal" took at least  42 ms
Seller id:  7, LD. Cust id to specific seller  2

Reservation 16 made at time 161 in slot  1
"Sealing the deal" took at least  10 ms
Seller id:  0, HA. Cust id to specific seller  1

Reservation 17 made at time 168 in slot 87
"Sealing the deal" took at least  61 ms
Seller id:  9, LF. Cust id to specific seller  2

Reservation 18 made at time 173 in slot 86
"Sealing the deal" took at least  70 ms
Seller id:  4, LA. Cust id to specific seller  2

Reservation 19 made at time 179 in slot 48
"Sealing the deal" took at least  24 ms
Seller id:  1, MA. Cust id to specific seller  0

Reservation 20 made at time 187 in slot 85
"Sealing the deal" took at least  59 ms
Seller id:  6, LC. Cust id to specific seller  2

Reservation 21 made at time 195 in slot 84
"Sealing the deal" took at least  45 ms
Seller id:  7, LD. Cust id to specific seller  3

Reservation 22 made at time 215 in slot 83
"Sealing the deal" took at least  45 ms
Seller id:  8, LE. Cust id to specific seller  1

Reservation 23 made at time 218 in slot 52
"Sealing the deal" took at least  33 ms
Seller id:  1, MA. Cust id to specific seller  1

Reservation 24 made at time 224 in slot 47
"Sealing the deal" took at least  31 ms
Seller id:  2, MB. Cust id to specific seller  1

Reservation 25 made at time 224 in slot 53
"Sealing the deal" took at least  31 ms
Seller id:  3, MC. Cust id to specific seller  2

Reservation 26 made at time 237 in slot  2
"Sealing the deal" took at least  10 ms
Seller id:  0, HA. Cust id to specific seller  2

Reservation 27 made at time 241 in slot 82
"Sealing the deal" took at least  64 ms
Seller id:  7, LD. Cust id to specific seller  4

Reservation 28 made at time 256 in slot 81
"Sealing the deal" took at least  58 ms
Seller id:  6, LC. Cust id to specific seller  3

Reservation 29 made at time 261 in slot 80
"Sealing the deal" took at least  57 ms
Seller id:  8, LE. Cust id to specific seller  2

Reservation 30 made at time 278 in slot 79
"Sealing the deal" took at least  58 ms
Seller id:  4, LA. Cust id to specific seller  3

Reservation 31 made at time 302 in slot  3
"Sealing the deal" took at least  19 ms
Seller id:  0, HA. Cust id to specific seller  3

Reservation 32 made at time 306 in slot 78
"Sealing the deal" took at least  59 ms
Seller id:  5, LB. Cust id to specific seller  2

Reservation 33 made at time 307 in slot 77
"Sealing the deal" took at least  45 ms
Seller id:  9, LF. Cust id to specific seller  3

Reservation 34 made at time 314 in slot 76
"Sealing the deal" took at least  44 ms
Seller id:  6, LC. Cust id to specific seller  4

Reservation 35 made at time 316 in slot 46
"Sealing the deal" took at least  29 ms
Seller id:  1, MA. Cust id to specific seller  2

Reservation 36 made at time 318 in slot 75
"Sealing the deal" took at least  60 ms
Seller id:  8, LE. Cust id to specific seller  3

Reservation 37 made at time 326 in slot 54
"Sealing the deal" took at least  22 ms
Seller id:  2, MB. Cust id to specific seller  2

Reservation 38 made at time 346 in slot 45
"Sealing the deal" took at least  39 ms
Seller id:  1, MA. Cust id to specific seller  3

Reservation 39 made at time 352 in slot 74
"Sealing the deal" took at least  48 ms
Seller id:  9, LF. Cust id to specific seller  4

Reservation 40 made at time 353 in slot 73
"Sealing the deal" took at least  65 ms
Seller id:  4, LA. Cust id to specific seller  4

Reservation 41 made at time 356 in slot 55
"Sealing the deal" took at least  39 ms
Seller id:  3, MC. Cust id to specific seller  3

Reservation 42 made at time 365 in slot 72
"Sealing the deal" took at least  46 ms
Seller id:  6, LC. Cust id to specific seller  5

Reservation 43 made at time 365 in slot 71
"Sealing the deal" took at least  51 ms
Seller id:  5, LB. Cust id to specific seller  3

Reservation 44 made at time 379 in slot 70
"Sealing the deal" took at least  65 ms
Seller id:  8, LE. Cust id to specific seller  4

Reservation 45 made at time 384 in slot 69
"Sealing the deal" took at least  52 ms
Seller id:  7, LD. Cust id to specific seller  5

Reservation 46 made at time 385 in slot 44
"Sealing the deal" took at least  31 ms
Seller id:  1, MA. Cust id to specific seller  4

Reservation 47 made at time 417 in slot 68
"Sealing the deal" took at least  52 ms
Seller id:  5, LB. Cust id to specific seller  4

Reservation 48 made at time 418 in slot 67
"Sealing the deal" took at least  67 ms
Seller id:  4, LA. Cust id to specific seller  5

Reservation 49 made at time 423 in slot 56
"Sealing the deal" took at least  36 ms
Seller id:  3, MC. Cust id to specific seller  4

Reservation 50 made at time 426 in slot 43
"Sealing the deal" took at least  32 ms
Seller id:  1, MA. Cust id to specific seller  5

Reservation 51 made at time 442 in slot 66
"Sealing the deal" took at least  45 ms
Seller id:  9, LF. Cust id to specific seller  5

Reservation 52 made at time 444 in slot 65
"Sealing the deal" took at least  60 ms
Seller id:  8, LE. Cust id to specific seller  5

Reservation 53 made at time 469 in slot 64
"Sealing the deal" took at least  40 ms
Seller id:  5, LB. Cust id to specific seller  5

Reservation 54 made at time 510 in slot  4
"Sealing the deal" took at least  11 ms
Seller id:  0, HA. Cust id to specific seller  4

Reservation 55 made at time 512 in slot 63
"Sealing the deal" took at least  48 ms
Seller id:  8, LE. Cust id to specific seller  6

Reservation 56 made at time 526 in slot 57
"Sealing the deal" took at least  23 ms
Seller id:  2, MB. Cust id to specific seller  3

Reservation 57 made at time 528 in slot 62
"Sealing the deal" took at least  56 ms
Seller id:  5, LB. Cust id to specific seller  6

Reservation 58 made at time 531 in slot  5
"Sealing the deal" took at least  11 ms
Seller id:  0, HA. Cust id to specific seller  5

Reservation 59 made at time 543 in slot 42
"Sealing the deal" took at least  25 ms
Seller id:  1, MA. Cust id to specific seller  6

Reservation 60 made at time 545 in slot 58
"Sealing the deal" took at least  22 ms
Seller id:  3, MC. Cust id to specific seller  5

Reservation 61 made at time 549 in slot 41
"Sealing the deal" took at least  20 ms
Seller id:  2, MB. Cust id to specific seller  4

Reservation 62 made at time 568 in slot 59
"Sealing the deal" took at least  24 ms
Seller id:  3, MC. Cust id to specific seller  6

Reservation 63 made at time 571 in slot 40
"Sealing the deal" took at least  23 ms
Seller id:  2, MB. Cust id to specific seller  5

Reservation 64 made at time 579 in slot 61
"Sealing the deal" took at least  70 ms
Seller id:  6, LC. Cust id to specific seller  6

Reservation 65 made at time 584 in slot 60
"Sealing the deal" took at least  58 ms
Seller id:  7, LD. Cust id to specific seller  6

Reservation 66 made at time 594 in slot 39
"Sealing the deal" took at least  34 ms
Seller id:  2, MB. Cust id to specific seller  6

Reservation 67 made at time 595 in slot  6
"Sealing the deal" took at least  17 ms
Seller id:  0, HA. Cust id to specific seller  6

Reservation 68 made at time 596 in slot 38
"Sealing the deal" took at least  52 ms
Seller id:  9, LF. Cust id to specific seller  6

Reservation 69 made at time 599 in slot 37
"Sealing the deal" took at least  43 ms
Seller id:  4, LA. Cust id to specific seller  6
Not enough buyers to sell out, 30 seats unfilled
*/

extern inline//use default attrs, and exit if fails
void xcreate_pthread_defattr(pthread_t *pres, void* func(void*), void* arg);
extern inline
void* xmalloc(size_t nbytes);
extern inline//seed it, state can't be zero
void srandom32(random32_t *prng, uint64_t init);
extern inline
uint32_t random32(random32_t *p);
extern inline
unsigned long get_millisecs_stamp(void);
extern inline
void millisleep(unsigned long msecs);
extern inline
uint32_t random32_xrange(random32_t *p, unsigned ibegin, unsigned iend);
extern inline
void byte_copy(const void* from, const void* fend, void* to);

//unsigned start = get_millisecs_stamp();
//millisleep(3*1000);
//unsigned stop = get_millisecs_stamp();
//printf("%u\n", stop-start);

//__sync_fetch_and_add(&gu32, 1u);//lock xadd

////the void* contains on object memory address, and the contents at that address are a function pointer.
////returns -1 if all slots full
////thinking more about this... its kind of weired and not that useful
//typedef int (*fp_sell_t)(void *);
