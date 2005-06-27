/*****************************************************************************\
 *  $Id: clusterlist_module.c,v 1.4 2005-06-27 04:44:49 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */

#include "cerebro.h"
#include "cerebro/cerebro_clusterlist_module.h"
#include "cerebro/cerebro_constants.h"

#include "clusterlist_module.h"
#include "module_util.h"

#include "debug.h"
#include "ltdl.h"

/*
 * clusterlist_modules
 * clusterlist_modules_len
 *
 * dynamic clusterlist modules to search for by default
 */
char *clusterlist_modules[] = {
  "cerebro_clusterlist_gendersllnl.so",
  "cerebro_clusterlist_genders.so",
  "cerebro_clusterlist_hostsfile.so",
  NULL
};
int clusterlist_modules_len = 3;

#define CLUSTERLIST_FILENAME_SIGNATURE  "cerebro_clusterlist_"

#define CLUSTERLIST_MODULE_DIR          CLUSTERLIST_MODULE_BUILDDIR "/.libs"

#define CLUSTERLIST_MODULE_MAGIC_NUMBER 0x33882200

/* 
 * struct clusterlist_module
 *
 * clusterlist module handle
 */
struct clusterlist_module
{
  int32_t magic;
  lt_dlhandle dl_handle;
  struct cerebro_clusterlist_module_info *module_info;
};

extern struct cerebro_clusterlist_module_info default_clusterlist_module_info;
extern int module_setup_count;

/* 
 * _clusterlist_module_loader
 *
 * If compiled statically, attempt to load the module specified by the
 * module name.
 *
 * If compiled dynamically, attempt to load the module specified by
 * the module_path.
 *
 * Return 1 is module is loaded, 0 if not, -1 on fatal error
 */
static int
_clusterlist_module_loader(void *handle, char *module)
{
  lt_dlhandle dl_handle = NULL;
  struct cerebro_clusterlist_module_info *module_info = NULL;
  clusterlist_module_t clusterlist_handle = (clusterlist_module_t)handle;

  if (!module_setup_count)
    {
      CEREBRO_DBG(("cerebro_module_library uninitialized"));
      return -1;
    }

  if (!clusterlist_handle)
    {
      CEREBRO_DBG(("clusterlist_handle null"));
      return -1;
    }

  if (clusterlist_handle->magic != CLUSTERLIST_MODULE_MAGIC_NUMBER)
    {
      CEREBRO_DBG(("clusterlist_handle magic number invalid"));
      return -1;
    }

  if (!module)
    {
      CEREBRO_DBG(("module null"));
      return -1;
    }

  if (!(dl_handle = lt_dlopen(module)))
    {
      CEREBRO_DBG(("lt_dlopen: module=%s, %s", module, lt_dlerror()));
      goto cleanup;
    }
  
  /* clear lt_dlerror */
  lt_dlerror();

  if (!(module_info = lt_dlsym(dl_handle, "clusterlist_module_info")))
    {
      const char *err = lt_dlerror();
      if (err)
	CEREBRO_DBG(("lt_dlsym: module=%s, %s", module, err));
      goto cleanup;
    }

  if (!module_info->clusterlist_module_name)
    {
      CEREBRO_DBG(("name null"));
      goto cleanup;
    }

  if (!module_info->setup)
    {
      CEREBRO_DBG(("setup null"));
      goto cleanup;
    }
  
  if (!module_info->cleanup)
    {
      CEREBRO_DBG(("cleanup null"));
      goto cleanup;
    }
  
  if (!module_info->numnodes)
    {
      CEREBRO_DBG(("numnodes null"));
      goto cleanup;
    }
  
  if (!module_info->get_all_nodes)
    {
      CEREBRO_DBG(("get_all_nodes null"));
      goto cleanup;
    }

  if (!module_info->node_in_cluster)
    {
      CEREBRO_DBG(("node_in_cluster null"));
      goto cleanup;
    }

  if (!module_info->get_nodename)
    {
      CEREBRO_DBG(("get_nodename null"));
      goto cleanup;
    }

  clusterlist_handle->dl_handle = dl_handle;
  clusterlist_handle->module_info = module_info;
  return 1;

 cleanup:
  if (dl_handle)
    lt_dlclose(dl_handle);
  return 0;
}

clusterlist_module_t 
clusterlist_module_load(void)
{
  struct clusterlist_module *clusterlist_handle = NULL;
  int rv;
  
  if (module_setup() < 0)
    return NULL;

  if (!(clusterlist_handle = (struct clusterlist_module *)malloc(sizeof(struct clusterlist_module))))
    {
      CEREBRO_DBG(("out of memory"));
      return NULL;
    }
  memset(clusterlist_handle, '\0', sizeof(struct clusterlist_module));
  clusterlist_handle->magic = CLUSTERLIST_MODULE_MAGIC_NUMBER;
      
#if CEREBRO_DEBUG
  if ((rv = find_known_module(CLUSTERLIST_MODULE_DIR,
			      clusterlist_modules,
			      clusterlist_modules_len,
			      _clusterlist_module_loader,
			      clusterlist_handle)) < 0)
    goto cleanup;

  if (rv)
    goto out;
#endif /* CEREBRO_DEBUG */

  if ((rv = find_known_module(CEREBRO_MODULE_DIR,
			      clusterlist_modules,
			      clusterlist_modules_len,
			      _clusterlist_module_loader,
			      clusterlist_handle)) < 0)
    goto cleanup;

  if (rv)
    goto out;
  
  if ((rv = find_modules(CEREBRO_MODULE_DIR,
			 CLUSTERLIST_FILENAME_SIGNATURE,
			 _clusterlist_module_loader,
			 clusterlist_handle,
			 1)) < 0)
    goto cleanup;
  
  if (rv)
    goto out;

  clusterlist_handle->dl_handle = NULL;
  clusterlist_handle->module_info = &default_clusterlist_module_info;
 out:
  return clusterlist_handle;

 cleanup:
  if (clusterlist_handle)
    {
      if (clusterlist_handle->dl_handle)
        lt_dlclose(clusterlist_handle->dl_handle);
      free(clusterlist_handle);
    }
  module_cleanup();
  return NULL;
}

/* 
 * clusterlist_module_handle_check
 *
 * Check for proper clusterlist module handle
 *
 * Returns 0 on success, -1 on error
 */
static int
clusterlist_module_handle_check(clusterlist_module_t clusterlist_handle)
{
  if (!clusterlist_handle 
      || clusterlist_handle->magic != CLUSTERLIST_MODULE_MAGIC_NUMBER
      || !clusterlist_handle->module_info)
    {
      CEREBRO_DBG(("cerebro handle invalid"));
      return -1;
    }

  return 0;
}

int
clusterlist_module_unload(clusterlist_module_t clusterlist_handle)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;

  clusterlist_module_cleanup(clusterlist_handle);

  clusterlist_handle->magic = ~CLUSTERLIST_MODULE_MAGIC_NUMBER;
  if (clusterlist_handle->dl_handle)
    lt_dlclose(clusterlist_handle->dl_handle);
  clusterlist_handle->module_info = NULL;
  free(clusterlist_handle);

  module_cleanup();
  return 0;
}

char *
clusterlist_module_name(clusterlist_module_t clusterlist_handle)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return NULL;

  return (clusterlist_handle->module_info)->clusterlist_module_name;
}

int
clusterlist_module_setup(clusterlist_module_t clusterlist_handle)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->setup)());
}

int
clusterlist_module_cleanup(clusterlist_module_t clusterlist_handle)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->cleanup)());
}

int
clusterlist_module_numnodes(clusterlist_module_t clusterlist_handle)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->numnodes)());
}

int
clusterlist_module_get_all_nodes(clusterlist_module_t clusterlist_handle, 
				 char ***nodes)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->get_all_nodes)(nodes));
}

int
clusterlist_module_node_in_cluster(clusterlist_module_t clusterlist_handle, 
				   const char *node)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->node_in_cluster)(node));
}

int
clusterlist_module_get_nodename(clusterlist_module_t clusterlist_handle,
				const char *node, 
				char *buf, 
				unsigned int buflen)
{
  if (clusterlist_module_handle_check(clusterlist_handle) < 0)
    return -1;
  
  return ((*(clusterlist_handle->module_info)->get_nodename)(node, buf, buflen));
}
