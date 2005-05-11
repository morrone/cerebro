/*****************************************************************************\
 *  $Id: cerebro_clusterlist_util.h,v 1.2 2005-05-11 21:59:57 achu Exp $
\*****************************************************************************/

#ifndef _CEREBRO_CLUSTERLIST_UTIL_H
#define _CEREBRO_CLUSTERLIST_UTIL_H

#include "cerebro.h"

/* 
 * _cerebro_load_clusterlist_module
 *
 * Find and load clusterlist module
 *
 * Returns 1 if clusterlist module was loaded, 0 if not, -1 on fatal error
 */
int _cerebro_load_clusterlist_module(cerebro_t handle);

/* 
 * _cerebro_unload_clusterlist_module
 *
 * Unload clusterlist module
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_unload_clusterlist_module(cerebro_t handle);

#endif /* _CEREBRO_CLUSTERLIST_UTIL_H */