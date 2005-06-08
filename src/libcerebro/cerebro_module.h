/*****************************************************************************\
 *  $Id: cerebro_module.h,v 1.23 2005-06-08 22:54:38 achu Exp $
\*****************************************************************************/

#ifndef _CEREBRO_MODULE_H
#define _CEREBRO_MODULE_H

#include "cerebro/cerebro_config.h"

typedef struct cerebro_clusterlist_module *cerebro_clusterlist_module_t;
typedef struct cerebro_config_module *cerebro_config_module_t; 

/*
 * _cerebro_module_load_clusterlist_module
 *
 * Find and load the clusterlist module.  If none is found, cerebro
 * library will assume a default clusterlist module.
 * 
 * Returns clusterlist module handle on success, NULL on error
 */
cerebro_clusterlist_module_t _cerebro_module_load_clusterlist_module(void);

/*
 * _cerebro_module_unload_clusterlist_module
 *
 * Unload the clusterlist module specified by the handle.
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_module_unload_clusterlist_module(cerebro_clusterlist_module_t handle);

/*
 * _cerebro_module_load_config_module
 *
 * Find and load the config module.  If none is found, cerebro
 * library will assume a default config module.
 * 
 * Returns 1 if module is found and loaded, 0 if one isn't found and
 * the default is loaded, -1 on fatal error
 */
cerebro_config_module_t _cerebro_module_load_config_module(void);

/*
 * _cerebro_module_unload_config_module
 *
 * Unload the config module specified by the handle
 *
 * Returns 0 on success, -1 on error
 */
int _cerebro_module_unload_config_module(cerebro_config_module_t handle);

/*
 * _cerebro_clusterlist_module_name
 *
 * Return clusterlist module name
 */
char *_cerebro_clusterlist_module_name(cerebro_clusterlist_module_t handle);

/*
 * _cerebro_clusterlist_module_setup
 *
 * call clusterlist module setup function
 */
int _cerebro_clusterlist_module_setup(cerebro_clusterlist_module_t handle);

/*
 * _cerebro_clusterlist_module_cleanup
 *
 * call clusterlist module parse cleanup function
 */
int _cerebro_clusterlist_module_cleanup(cerebro_clusterlist_module_t handle);

/*
 * _cerebro_clusterlist_module_numnodes
 *
 * call clusterlist module numnodes function
 */
int _cerebro_clusterlist_module_numnodes(cerebro_clusterlist_module_t handle);

/*
 * _cerebro_clusterlist_module_get_all_nodes
 *
 * call clusterlist module get all nodes function
 */
int _cerebro_clusterlist_module_get_all_nodes(cerebro_clusterlist_module_t handle,
                                              char ***nodes);

/*
 * _cerebro_clusterlist_module_node_in_cluster
 *
 * call clusterlist module node in cluster function
 */
int _cerebro_clusterlist_module_node_in_cluster(cerebro_clusterlist_module_t handle,
                                                const char *node);

/*
 * _cerebro_clusterlist_module_get_nodename
 *
 * call clusterlist module get nodename function
 */
int _cerebro_clusterlist_module_get_nodename(cerebro_clusterlist_module_t handle,
                                             const char *node, 
                                             char *buf, 
                                             unsigned int buflen);

/*
 * _cerebro_config_module_name
 *
 * Return config module name
 */
char *_cerebro_config_module_name(cerebro_config_module_t handle);

/*
 * _cerebro_config_module_setup
 *
 * call config module setup function
 */
int _cerebro_config_module_setup(cerebro_config_module_t handle);

/*
 * _cerebro_config_module_cleanup
 *
 * call config module parse cleanup function
 */
int _cerebro_config_module_cleanup(cerebro_config_module_t handle);

/*
 * _cerebro_config_module_load_default
 *
 * call config module get all nodes function
 */
int _cerebro_config_module_load_default(cerebro_config_module_t handle,
                                        struct cerebro_config *conf);

#endif /* _CEREBRO_MODULE_H */
