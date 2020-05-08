/* ******************************************************************************** *
   Name   : DRP driver for RZ/A2M
   Date   : 2020.05.08
   Auther : Shimafuji Electric inc.

   Copyright (C) 2020 Shimafuji Electric inc. All rights reserved.
 * ******************************************************************************** */

#include "r_dk2_if.h"


#ifndef SF_DRP_H
#define SF_DRP_H

#ifdef  __cplusplus
extern  "C"
{
#endif  /* __cplusplus */

#define SF_TASK_NUM (12)
#define SF_DRP_INT_BLOCK ((int_cb_t)-1)

int32_t sf_DK2_Initialize(void);
int32_t sf_DK2_Load(const void *const pconfig,
                    const uint8_t top_tiles,
                    const uint32_t tile_pattern,
                    const load_cb_t pload,
                    const int_cb_t pint,
                    uint8_t *const paid);
int32_t sf_DK2_Start2(const uint8_t id, const void *const pparam, const uint32_t size, const int_cb_t pint);

#ifdef  __cplusplus
}
#endif

#endif /* SF_DRP_H */
