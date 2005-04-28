/*****************************************************************************\
 *  $Id: cerebro_clusterlist_hostsfile.c,v 1.6 2005-04-28 18:47:25 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#include <ctype.h>
#endif /* STDC_HEADERS */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include "cerebro_defs.h"
#include "cerebro_error.h"
#include "cerebro_clusterlist_module.h"
#include "cerebro_clusterlist_util.h"

#include "fd.h"
#include "list.h"

#define HOSTSFILE_CLUSTERLIST_MODULE_NAME "hostsfile"
#define HOSTSFILE_PARSE_BUFLEN            4096

/* 
 * hosts
 *
 * list of all hosts
 */
static List hosts = NULL;

/*  
 * hostsfile_file
 *
 * hostsfile database
 */
static char *hostsfile_file = NULL;

/* 
 * hostsfile_clusterlist_parse_options
 *
 * parse options for the hostsfile clusterlist module
 */
static int
hostsfile_clusterlist_parse_options(char **options)
{
  if (hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts non-null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if (options)
    return cerebro_clusterlist_parse_filename(options, 
                                              &hostsfile_file, 
                                              HOSTSFILE_CLUSTERLIST_MODULE_NAME);

  return 0;
}

/* 
 * _readline
 * 
 * read a line from the hostsfile.  Buffer guaranteed to be null terminated.
 *
 * - fd - file descriptor to read from
 * - buf - buffer pointer
 * - buflen - buffer length
 *
 * Return amount of data read into the buffer, -1 on error
 */
static int
_readline(int fd, char *buf, int buflen)
{
  int ret;

  if (!buf)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: null buffer", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if ((ret = fd_read_line(fd, buf, buflen)) < 0)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: fd_read_line: %s", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
      return -1;
    }
  
  /* buflen - 1 b/c fd_read_line guarantees null termination */
  if (ret >= (buflen-1))
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: "
                        "fd_read_line: line truncation",
                        __FILE__, __FUNCTION__, __LINE__, 
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  return ret;
}

/* 
 * _remove_comments
 *
 * remove comments from the buffer
 *
 * - buf - buffer pointer
 * - buflen - buffer length
 *
 * Return length of buffer left after comments were removed, -1 on error
 */
static int
_remove_comments(char *buf, int buflen)
{
  int i, comment_flag, retlen;

  if (!buf)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: null buffer", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if (strchr(buf, '#') == NULL)
    return buflen;

  i = 0;
  comment_flag = 0;
  retlen = buflen;
  while (i < buflen)
    {
      if (comment_flag)
        {
          buf[i] = '\0';
          retlen--;
        }

      if (buf[i] == '#')
        {
          buf[i] = '\0';
          comment_flag++;
          retlen--;
        }
      i++;
    }

  return retlen;
}

/* 
 * _remove_trailing_whitespace
 *
 * remove trailing whitespace from the buffer
 *
 * - buf - buffer pointer
 * - buflen - buffer length
 *
 * Return length of buffer left after trailing whitespace was removed, -1 on error
 */
static int
_remove_trailing_whitespace(char *buf, int buflen)
{
  char *temp;
  
  if (!buf)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: null buffer", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  temp = buf + buflen;
  for (--temp; temp >= buf; temp--) 
    {
      if (isspace(*temp))
        *temp = '\0';
      else
        break;
      buflen--;
    }

  return buflen;
}

/* 
 * _move_past_whitespace
 *
 * move past whitespace at the beginning of the buffer
 *
 * - buf - buffer pointer
 *
 * Return pointer to beginning of first non-whitespace char, NULL on error
 */
static char *
_move_past_whitespace(char *buf)
{
  if (!buf)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: null buffer", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return NULL;
    }

  while (*buf != '\0' && isspace(*buf))
    buf++;

  return buf;
}

/* 
 * hostsfile_clusterlist_setup
 *
 * hostsfile clusterlist module setup function.  Open hostsfile, read
 * each line of the hostsfile, and save hosts into hosts list.
 */
static int 
hostsfile_clusterlist_setup(void)
{
  int len, fd = -1;
  char buf[HOSTSFILE_PARSE_BUFLEN];
  char *file, *p;

  if (hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts non-null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return 0;
    }

  if (!(hosts = list_create((ListDelF)free)))
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: list_create: %s", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
      goto cleanup;
    }

  if (hostsfile_file)
    file = hostsfile_file;
  else
    file = CEREBRO_CLUSTERLIST_HOSTSFILE_DEFAULT;

  if ((fd = open(file, O_RDONLY)) < 0)
    {
      cerebro_err_debug("hostsfile clusterlist file '%s' cannot be opened", file);
      goto cleanup;
    }
 
  while ((len = _readline(fd, buf, HOSTSFILE_PARSE_BUFLEN)) > 0)
    {
      char *hostPtr;
      char *str;

      if ((len = _remove_comments(buf, len)) == 0)
        continue;

      if (len < 0)
        goto cleanup;

      if ((len = _remove_trailing_whitespace(buf, len)) == 0)
        continue;

      if (len < 0)
        goto cleanup;

      if (!(hostPtr = _move_past_whitespace(buf)))
        goto cleanup;

      if (hostPtr[0] == '\0')
        continue;

      if (strchr(hostPtr, ' ') || strchr(hostPtr, '\t'))
        {
          cerebro_err_debug("hostsfile clusterlist parse error: "
                            "host contains whitespace");
          goto cleanup;
        }

      if (strlen(hostPtr) > CEREBRO_MAXNODENAMELEN)
        {
          cerebro_err_debug("hostsfile clusterlist parse error: "
                            "nodename '%s' exceeds maximum length", 
                            hostPtr);
          goto cleanup;
        }
      
      /* Shorten hostname if necessary */
      if ((p = strchr(hostPtr, '.')))
        *p = '\0';

      if (!(str = strdup(hostPtr)))
        {
          cerebro_err_debug("%s(%s:%d): %s clusterlist module: strdup: %s", 
                            __FILE__, __FUNCTION__, __LINE__,
                            HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
          goto cleanup;
        }

      if (!list_append(hosts, str))
        {
          cerebro_err_debug("%s(%s:%d): %s clusterlist module: list_append: %s", 
                            __FILE__, __FUNCTION__, __LINE__,
                            HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
          goto cleanup;
        }
    }
  
  if (len < 0)
    goto cleanup;

  close(fd);
  return 0;

 cleanup:
  close(fd);
  if (hosts)
    list_destroy(hosts);
  hosts = NULL;
  return -1;
}

/* 
 * hostsfile_clusterlist_cleanup
 *
 * hostsfile clusterlist module cleanup function
 */
static int
hostsfile_clusterlist_cleanup(void)
{
  if (!hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return 0;
    }

  list_destroy(hosts);
  free(hostsfile_file);
  hosts = NULL;
  hostsfile_file = NULL;

  return 0;
}

/*
 * hostsfile_clusterlist_get_all_nodes
 *
 * hostsfile clusterlist module get all nodes function
 */
static int
hostsfile_clusterlist_get_all_nodes(char **nodes, unsigned int nodeslen)
{
  char *node;
  ListIterator itr = NULL;
  int numnodes, i = 0;

  if (!hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if (!nodes)
    {     
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: "
                        "invalid nodes parameter",
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME,
                        __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  numnodes = list_count(hosts);

  if (numnodes > nodeslen)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: nodeslen too small",
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      goto cleanup;
    }

  if (!(itr = list_iterator_create(hosts)))
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: list_iterator_create: %s", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
      goto cleanup;
    }

  while ((node = list_next(itr)) && (i < numnodes))
    {
      if (!(nodes[i++] = strdup(node)))
        {
          cerebro_err_debug("%s(%s:%d): %s clusterlist module: strdup: %s", 
                            __FILE__, __FUNCTION__, __LINE__,
                            HOSTSFILE_CLUSTERLIST_MODULE_NAME, strerror(errno));
          goto cleanup;
        }
    }

  if (i > numnodes)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: iterator count error",
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      goto cleanup;
    }

  list_iterator_destroy(itr);

  return numnodes;

 cleanup:
  if (itr)
    list_iterator_destroy(itr);
  return -1;
}

/*
 * hostsfile_clusterlist_numnodes
 *
 * hostsfile clusterlist module numnodes function
 */
static int 
hostsfile_clusterlist_numnodes(void)
{
  if (!hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  return list_count(hosts);
}

/*
 * hostsfile_clusterlist_node_in_cluster
 *
 * hostsfile clusterlist module node in cluster function
 */
static int
hostsfile_clusterlist_node_in_cluster(char *node)
{
  char nodebuf[CEREBRO_MAXNODENAMELEN+1];
  char *nodePtr = NULL;
  void *ret;

  if (!hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if (!node)
    {     
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: "
                        "invalid node parameter",
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME,
                        __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  /* Shorten hostname if necessary */
  if (strchr(node, '.'))
    {
      char *p;

      memset(nodebuf, '\0', CEREBRO_MAXNODENAMELEN+1);
      strncpy(nodebuf, node, CEREBRO_MAXNODENAMELEN);
      p = strchr(nodebuf, '.');
      *p = '\0';
      nodePtr = nodebuf;
    }
  else
    nodePtr = node;

  ret = list_find_first(hosts, (ListFindF)strcmp, nodePtr);

  return ((ret) ? 1: 0);
}

/*
 * hostsfile_clusterlist_get_nodename
 *
 * hostsfile clusterlist module get nodename function
 */
static int
hostsfile_clusterlist_get_nodename(char *node, char *buf, unsigned int buflen)
{
  char nodebuf[CEREBRO_MAXNODENAMELEN+1];
  char *nodePtr = NULL;

  if (!hosts)
    {
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: hosts null", 
                        __FILE__, __FUNCTION__, __LINE__,
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME);
      return -1;
    }

  if (!node)
    {     
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: "
                        "invalid node parameter",
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME,
                        __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  if (!buf)
    {     
      cerebro_err_debug("%s(%s:%d): %s clusterlist module: "
                        "invalid buf parameter",
                        HOSTSFILE_CLUSTERLIST_MODULE_NAME,
                        __FILE__, __FUNCTION__, __LINE__);
      return -1;
    }

  /* Shorten hostname if necessary */
  if (strchr(node, '.'))
    {
      char *p;

      memset(nodebuf, '\0', CEREBRO_MAXNODENAMELEN+1);
      strncpy(nodebuf, node, CEREBRO_MAXNODENAMELEN);
      p = strchr(nodebuf, '.');
      *p = '\0';
      nodePtr = nodebuf;
    }
  else
    nodePtr = node;

  return cerebro_clusterlist_copy_nodename(nodePtr, 
                                           buf, 
                                           buflen, 
                                           HOSTSFILE_CLUSTERLIST_MODULE_NAME);
}

#if WITH_STATIC_MODULES
struct cerebro_clusterlist_module_info hostsfile_clusterlist_module_info =
#else /* !WITH_STATIC_MODULES */
struct cerebro_clusterlist_module_info clusterlist_module_info =
#endif /* !WITH_STATIC_MODULES */
  {
    HOSTSFILE_CLUSTERLIST_MODULE_NAME,
    &hostsfile_clusterlist_parse_options,
    &hostsfile_clusterlist_setup,
    &hostsfile_clusterlist_cleanup,
    &hostsfile_clusterlist_get_all_nodes,
    &hostsfile_clusterlist_numnodes,
    &hostsfile_clusterlist_node_in_cluster,
    &hostsfile_clusterlist_get_nodename,
  };