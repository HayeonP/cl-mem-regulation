#ifndef PRFCNT_H
#define PRFCNT_H

/**
 *  Performance counters set.
 *
 * The hardware has limited number of registers to accumulate hardware counters
 * values. On the other hand, the number of performance counters exceeds the number
 * of registers. Therefore we cannot collect all the performance counters all together.
 * There are a few different disjoint sets of counters that can be collected
 * together in a profiling session.
 */
enum prfcnt_set{
    prfcnt_set_primary,   /**< Primary. */
    prfcnt_set_secondary, /**< Secondary. */
    prfcnt_set_tertiary,  /**< Tertiary. */
};

#endif