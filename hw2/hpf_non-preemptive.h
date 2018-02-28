#ifndef HPF_NONPREEMPTIVE_H
#define HPF_NONPREEMPTIVE_H

#include "syms.h"
#include <stdlib.h>
using std::fill;

struct HPFNonpreQueueData;
struct HPFNonpreComparator;
HPFNonpreQueueData ConvertToHPFData(const Job& jb, unsigned id);

#endif
