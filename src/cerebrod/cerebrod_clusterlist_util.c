/*****************************************************************************\
 *  $Id: cerebrod_clusterlist_util.c,v 1.5 2005-03-30 05:41:45 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <assert.h>
#include <errno.h>

#include "cerebrod.h"
#include "cerebrod_clusterlist.h"
#include "cerebrod_clusterlist_util.h"
#include "cerebrod_error.h"
#include "wrappers.h"

int 
cerebrod_clusterlist_parse_filename(char **options, char **filename)
{
  int i = 0;
  char *clusterlist_module_name = cerebrod_clusterlist_module_name();

  assert(options);
  assert(filename);

  *filename = NULL;

  while (options[i] != NULL)
    {
      if (strstr(options[i], "filename"))
	{
	  char *p = strchr(options[i], '=');

	  if (!p)
	    cerebrod_err_exit("%s clusterlist module: filename unspecified", 
			      clusterlist_module_name);

	  p++;
	  if (p == '\0')
	    cerebrod_err_exit("%s clusterlist module: filename unspecified", 
			      clusterlist_module_name);

	  *filename = Strdup(p);
	}
      else
	cerebrod_err_exit("%s clusterlist module: option '%s' unrecognized", 
			  clusterlist_module_name, options[i]);

      i++;
    }

  if (*filename != NULL)
    {
      struct stat buf;

      if (stat(*filename, &buf) < 0)
        cerebrod_err_exit("%s clusterlist module: filename '%s' not found",
			  clusterlist_module_name, *filename);
    }

  return 0;
}

int 
cerebrod_clusterlist_copy_nodename(char *node, char *buf, unsigned int buflen)
{
  int len;
  char *clusterlist_module_name = cerebrod_clusterlist_module_name();

  assert(node);
  assert(buf);
  assert(buflen > 0);

  len = strlen(node);

  if ((len + 1) > buflen)
    cerebrod_err_exit("%s(%s:%d): %s clusterlist module: "
		      "cerebrod_clusterlist_copy_nodename: "
		      "buflen too small: %d %d", 
		      __FILE__, __FUNCTION__, __LINE__,
		      clusterlist_module_name, len, buflen);

  strcpy(buf, node);

  return 0;
}