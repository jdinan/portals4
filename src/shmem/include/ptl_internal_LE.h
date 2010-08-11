#ifndef PTL_INTERNAL_LE_H
#define PTL_INTERNAL_LE_H

void INTERNAL PtlInternalLENISetup(
    ptl_size_t limit);

void INTERNAL PtlInternalLENITeardown(
    void);

int INTERNAL PtlInternalLEDeliver(
    ptl_table_entry_t * restrict t,
    ptl_internal_header_t * restrict h);

#endif
