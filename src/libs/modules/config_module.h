/*****************************************************************************\
 *  $Id: config_module.h,v 1.3 2005-07-02 13:20:45 achu Exp $
\*****************************************************************************/

#ifndef _CONFIG_MODULE_H
#define _CONFIG_MODULE_H

#include "cerebro/cerebro_config.h"

typedef struct config_module *config_module_t;

/*
 * config_module_load
 *
 * Find and load the config module.  If none is found, will assume a
 * default config module.
 * 
 * Returns config module handle on success, NULL on error
 */
config_module_t config_module_load(void);

/*
 * config_module_unload
 *
 * Unload/cleanup the config module specified by the handle
 *
 * Returns 0 on success, -1 on error
 */
int config_module_unload(config_module_t handle);

/*
 * config_module_name
 *
 * Return config module name
 */
char *config_module_name(config_module_t handle);

/*
 * config_module_setup
 *
 * call config module setup function
 */
int config_module_setup(config_module_t handle);

/*
 * config_module_cleanup
 *
 * call config module cleanup function
 */
int config_module_cleanup(config_module_t handle);

/*
 * config_module_load_config
 *
 * call config module get all nodes function
 */
int config_module_load_config(config_module_t handle, 
			      struct cerebro_config *conf);


#endif /* _CONFIG_MODULE_H */
