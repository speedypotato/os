#include "header.h"
#include "util.h"

TransactHistory* histories;
int nsold[10];
int ncps=7;//same num customers per each seller

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned gTimeOfBroadcastMs;//set once in main, then const

//static unsigned gseats[100];//zero init

//for all indexing, use signed comparisons, less error prone.
//array only of size 100
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

fp_sell_t getSellFunc(int tid)
{
    if (tid == 0)       return sell_front_to_mid;//hi seller
    else if (tid < 4)   return sell_mid_alternate;
    else                return sell_back_to_mid;
}

void* thread_func(void *ctx)
{
    int const tid = (int)(uintptr_t)ctx;
    if (tid!=0)
    {
        pthread_mutex_lock(&mutex);//wtf? if they all need to hold this beforehand, how will the cv_wait execute below?
        printf("Thread %u waiting on cv\n", tid);
        pthread_cond_wait(&cond, &mutex);//kjlasdkjasdgkjadgalfgaklfgalfd;gajf sdfjsd;fa
        //mtx now acquired
    }
    //thread 0 has lock
    printf("Thread %u doing work...\n", tid);
    for (;;)
    {
        break;
    }

    pthread_mutex_unlock(&mutex);

    printf("Thread %u exiting\n", tid);
    return NULL;
}

static void fprint64x(uint64_t val);

int main(int argc, char **argv)
{
    uint64_t seedval = DEFAULT_SEED;

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

    printf("n: customers per seller = %u\nseed = 0x ", ncps);
    fprint64x(seedval);
    putchar('\n');

    random32_t rng;
    srandom32(&rng, seedval);

    typedef TransactHistory *iter;
    pthread_t addthreads[9];

    iter begin = histories = XMALLOC_UNITS(TransactHistory, ncps*10);
    iter end = begin + ncps;
    for (iter it=begin; it != end; ++it)//do single seller, but don't spawn thread yet
    {
        it->arrivalTime = random32(&rng) & 63;
        it->serviceDuration = random32_xrange(&rng, 1*64, 2*64 - 1);
    }
    QSORT_TYPED(begin, end, sortByArrival);

    for (int tid=1; tid<4; ++tid)
    {
        begin = end;
        end += ncps;
        for (iter it=begin; it != end; ++it)
        {
            it->arrivalTime = random32(&rng) & 63;
            it->serviceDuration = random32_xrange(&rng, 2*64, 4*64 - 1);
        }
        QSORT_TYPED(begin, end, sortByArrival);
        xcreate_pthread_defattr(addthreads+tid-1, thread_func, (void *)(uintptr_t)tid);
    }

    for (int tid=4; tid<10; ++tid)
    {
        begin = end;
        end += ncps;
        for (iter it=begin; it != end; ++it)
        {
            it->arrivalTime = random32(&rng) & 63;
            it->serviceDuration = random32_xrange(&rng, 4*64, 7*64 - 1);
        }
        QSORT_TYPED(begin, end, sortByArrival);
        xcreate_pthread_defattr(addthreads+tid-1, thread_func, (void *)(uintptr_t)tid);
    }
    puts("Thread 0 broadcasting cv");
    millisleep(300);//"HACK" ensure all threads are waiting on the lock...
    gTimeOfBroadcastMs = get_millisecs_stamp();
    pthread_mutex_lock(&mutex);//APPARENTLY ALL THE OTHER THREADS NEED TO HAVE THE MTX FIRST?
    pthread_cond_broadcast(&cond);
    thread_func((void *)0);//run hi seller on main, make use of it

    for (int i=0; i<9; ++i)
        pthread_join(addthreads[i], NULL);

    puts("output...");

    free(histories);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    return 0;
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
