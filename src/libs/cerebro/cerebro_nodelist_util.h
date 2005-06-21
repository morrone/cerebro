/*****************************************************************************\
 *  $Id: cerebro_nodelist_util.h,v 1.1 2005-06-17 20:54:08 achu Exp $
\*****************************************************************************/

#ifndef _CEREBRO_NODELIST_UTIL_H
#define _CEREBRO_NODELIST_UTIL_H

#include "cerebro.h"

/*
 * _cerebro_nodelist_check
 *
 * Checks for a proper cerebro nodelist, setting the errnum
 * appropriately if an error is found.
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_nodelist_check(cerebro_nodelist_t nodelist);

/* 
 * _cerebro_nodelist_create
 *
 * Create and initialize a nodelist 
 *
 * Returns nodelist on success, NULL on error
 */
cerebro_nodelist_t _cerebro_nodelist_create(cerebro_t handle, 
                                            const char *metric_name);

/*
 * _cerebro_nodelist_append
 * 
 * Append additional nodelist data
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_nodelist_append(cerebro_nodelist_t nodelist,
			     const char *nodename,
                             u_int32_t metric_type,
                             u_int32_t metric_len,
                             void *metric_data);

/*
 * _cerebro_nodelist_sort
 * 
 * Sort the nodelist by nodename
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_nodelist_sort(cerebro_nodelist_t nodelist);

#endif /* _CEREBRO_NODELIST_UTIL_H */