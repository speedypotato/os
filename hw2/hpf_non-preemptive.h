// Author: Erik Xu
// CS 149 Sec. 2, Spring 2018
// Header file for HPF Non-preemptive Scheduling Algorithm
#ifndef HPF_NONPREEMPTIVE_H
#define HPF_NONPREEMPTIVE_H

#include "syms.h"
#include <stdlib.h>
using std::fill;

// Struct definition for HPF priority queue data structure
struct HPFNonpreQueueData;
// Struct defintion for HPF priority queue comparator function
struct HPFNonpreComparator;
// Struct defintion to create HPF priority queue data structure
HPFNonpreQueueData ConvertToHPFData(const Job& jb, unsigned id);

#endif
