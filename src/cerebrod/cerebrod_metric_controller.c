/*****************************************************************************\
 *  $Id: cerebrod_metric_controller.c,v 1.2 2005-07-12 00:31:53 achu Exp $
\*****************************************************************************/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <sys/socket.h>

#include <assert.h>

#include "cerebro/cerebro_constants.h"
#include "cerebro/cerebro_metric_control_protocol.h"

#include "cerebrod.h"
#include "cerebrod_config.h"
#include "cerebrod_metric_controller.h"
#include "cerebrod_speaker_data.h"
#include "cerebrod_util.h"

#include "debug.h"
#include "fd.h"
#include "network_util.h"
#include "wrappers.h"

extern struct cerebrod_config conf;
#if CEREBRO_DEBUG
extern pthread_mutex_t debug_output_mutex;
#endif /* CEREBRO_DEBUG */

extern List metric_list;
extern int metric_list_size;
extern pthread_mutex_t metric_list_lock;

#if CEREBRO_DEBUG
#define CEREBROD_METRIC_CONTROLLER_UNIX_PATH  "/tmp/cerebro_metric_cerebrod_metric_controller"
#else  /* !CEREBRO_DEBUG */
#define CEREBROD_METRIC_CONTROLLER_UNIX_PATH  CEREBRO_MODULE_DIR "/cerebro_metric_cerebrod_metric_controller"
#endif  /* !CEREBRO_DEBUG */
#define CEREBROD_METRIC_CONTROLLER_BACKLOG    5

/*
 * metric_controller_init
 * metric_controller_init_cond
 * metric_controller_init_lock
 *
 * variables for synchronizing initialization between different pthreads
 * and signaling when it is complete
 */
int metric_controller_init = 0;
pthread_cond_t metric_controller_init_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t metric_controller_init_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * _metric_controller_initialize
 *
 * perform metric server initialization
 */
static void
_metric_controller_initialize(void)
{
  Pthread_mutex_lock(&metric_controller_init_lock);
  if (metric_controller_init)
    goto out;

  Signal(SIGPIPE, SIG_IGN);

  metric_controller_init++;
  Pthread_cond_signal(&metric_controller_init_cond);
 out:
  Pthread_mutex_unlock(&metric_controller_init_lock);
}

/*
 * _metric_controller_setup_socket
 *
 * Create and setup the controller socket.  Do not use wrappers in this
 * function.  We want to give the controller additional chances to
 * "survive" an error condition.
 *
 * Returns file descriptor on success, -1 on error
 */
static int
_metric_controller_setup_socket(void)
{
  struct sockaddr_un addr;
  int fd;
  
  if ((fd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    {
      CEREBRO_DBG(("socket: %s", strerror(errno)));
      return -1;
    }

  if (strlen(CEREBROD_METRIC_CONTROLLER_UNIX_PATH) >= sizeof(addr.sun_path))
    {
      CEREBRO_DBG(("path '%s' too long", CEREBROD_METRIC_CONTROLLER_UNIX_PATH));
      goto cleanup;
    }

  /* unlink is allowed to fail */
  unlink(CEREBROD_METRIC_CONTROLLER_UNIX_PATH);
  
  memset(&addr, '\0', sizeof(struct sockaddr_un));
  addr.sun_family = AF_LOCAL;
  strncpy(addr.sun_path, 
          CEREBROD_METRIC_CONTROLLER_UNIX_PATH, 
          sizeof(addr.sun_path));
  
  if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0)
    {
      CEREBRO_DBG(("bind: %s", strerror(errno)));
      goto cleanup;
    }
  
  if (listen(fd, CEREBROD_METRIC_CONTROLLER_BACKLOG) < 0)
    {
      CEREBRO_DBG(("listen: %s", strerror(errno)));
      goto cleanup;
    }
  
  return fd;

 cleanup:
  close(fd);
  return -1;
}

/*
 * _metric_control_request_check_version
 *
 * Check that the version is correct prior to unmarshalling
 *
 * Returns 0 if version is correct, -1 if not
 */
static int
_metric_control_request_check_version(const char *buf, 
                                      unsigned int buflen, 
                                      int32_t *version)
{
  assert(buflen >= sizeof(int32_t) && version);
                                                                                      
  if (!Unmarshall_int32(version, buf, buflen))
    {
      CEREBRO_DBG(("version could not be unmarshalled"));
      return -1;
    }
                                                                                      
  if (*version != CEREBRO_METRIC_CONTROL_PROTOCOL_VERSION)
    return -1;
                                                                                      
  return 0;
}

/*
 * _metric_control_request_header_unmarshall
 *
 * unmarshall contents of a metric control request packet buffer
 *
 * Returns 0 on success, -1 on error
 */
static int
_metric_control_request_header_unmarshall(struct cerebro_metric_control_request *req,
                                          const char *buf,
                                          unsigned int buflen)
{
  int bufPtrlen, c = 0;
  char *bufPtr;
  
  assert(req && buf);
  
  bufPtr = req->metric_name;
  bufPtrlen = sizeof(req->metric_name);
  c += Unmarshall_int32(&(req->version), buf + c, buflen - c);
  c += Unmarshall_int32(&(req->command), buf + c, buflen - c);
  c += Unmarshall_buffer(bufPtr, bufPtrlen, buf + c, buflen - c);
  c += Unmarshall_u_int32(&(req->metric_value_type), buf + c, buflen - c);
  c += Unmarshall_u_int32(&(req->metric_value_len), buf + c, buflen - c);
  
  if (c != CEREBRO_METRIC_CONTROL_REQUEST_HEADER_LEN)
    return -1;
  
  return 0;
}

/*
 * _metric_control_response_marshall
 *
 * marshall contents of a metric err response packet buffer
 *
 * Returns length written to buffer on success, -1 on error
 */
static int
_metric_control_response_marshall(struct cerebro_metric_control_response *res,
                                  char *buf,
                                  unsigned int buflen)
{
  int len = 0;
                                                                                      
  assert(res && buf && buflen >= CEREBRO_METRIC_CONTROL_RESPONSE_LEN);
                                                                                      
  memset(buf, '\0', buflen);
  len += Marshall_int32(res->version, buf + len, buflen - len);
  len += Marshall_u_int32(res->err_code, buf + len, buflen - len);
  return len;
}


/*  
 * _metric_control_request_dump
 *
 * dump contents of a metric controller request
 */
static void
_metric_control_request_dump(struct cerebro_metric_control_request *req)
{
#if CEREBRO_DEBUG
  char metric_name_buf[CEREBRO_MAX_METRIC_NAME_LEN+1];

  assert(req);

  if (!(conf.debug && conf.metric_controller_debug))
    return;
  
  Pthread_mutex_lock(&debug_output_mutex);
  fprintf(stderr, "**************************************\n");
  fprintf(stderr, "* Metric Controller Request Received:\n");
  fprintf(stderr, "* ------------------------\n");
  fprintf(stderr, "* Version: %d\n", req->version);
  fprintf(stderr, "* Command: %d\n", req->command);
  /* Guarantee ending '\0' character */
  memset(metric_name_buf, '\0', CEREBRO_MAX_METRIC_NAME_LEN+1);
  memcpy(metric_name_buf, req->metric_name, CEREBRO_MAX_METRIC_NAME_LEN);
  fprintf(stderr, "* Metric_Name: %s\n", metric_name_buf);
  fprintf(stderr, "* Metric_Value_Type: %x\n", req->metric_value_type);
  fprintf(stderr, "* metric_value_len: %d\n", req->metric_value_len);
  fprintf(stderr, "**************************************\n");
  Pthread_mutex_unlock(&debug_output_mutex);
#endif /* CEREBRO_DEBUG */
}

/* 
 * _send_metric_control_response
 *
 * Send metric control responses with the appropriate err code
 *
 * Returns 0 on success, -1 on error
 */
static int
_send_metric_control_response(int fd, int32_t version, u_int32_t err_code)
{
  struct cerebro_metric_control_response res;
  char buf[CEREBRO_MAX_PACKET_LEN];
  int res_len, buflen;
                                                                                      
  assert(fd >= 0
         && err_code >= CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_VERSION_INVALID
         && err_code <= CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_INTERNAL_ERROR);
                                                                                      
  memset(&res, '\0', CEREBRO_METRIC_SERVER_ERR_RESPONSE_LEN);
  res.version = version;
  res.err_code = err_code;
                                                                                      
  buflen = CEREBRO_MAX_PACKET_LEN;
  if ((res_len = _metric_control_response_marshall(&res, buf, buflen)) < 0)
    return -1;
  
  if (fd_write_n(fd, buf, res_len) < 0)
    {
      CEREBRO_DBG(("fd_write_n: %s", strerror(errno)));
      return -1;
    }
                                                                                      
  return 0;
}

/*
 * _metric_controller_service_connection
 *
 * Thread to service a connection from a client to retrieve metric
 * data.  Use wrapper functions minimally, b/c we want to return
 * errors to the user instead of exitting with errors.
 *
 * Passed int * pointer to client TCP socket file descriptor
 *
 * Executed in detached state, no return value.
 */
static void *
_metric_controller_service_connection(void *arg)
{
  int fd, recv_len;
  struct cerebro_metric_control_request req;
  char buf[CEREBRO_MAX_PACKET_LEN];
  int32_t version;

  fd = *((int *)arg);
  
  memset(&req, '\0', sizeof(struct cerebro_metric_control_request));
  
  if ((recv_len = receive_data(fd,
                               CEREBRO_METRIC_CONTROL_REQUEST_HEADER_LEN,
                               buf,
                               CEREBRO_MAX_PACKET_LEN,
                               CEREBRO_METRIC_CONTROL_PROTOCOL_CLIENT_TIMEOUT_LEN,
                               NULL)) < 0)
    goto cleanup;
  
  if (recv_len < sizeof(version))
    goto cleanup;

  if (_metric_control_request_check_version(buf, recv_len, &version) < 0)
    {
      _send_metric_control_response(fd,
                                    version,
                                    CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_VERSION_INVALID);
      goto cleanup;
    }
  
  if (recv_len < CEREBRO_METRIC_CONTROL_REQUEST_HEADER_LEN)
    {
      _send_metric_control_response(fd,
                                    version,
                                    CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_PACKET_INVALID);
      goto cleanup;
    }
  
  if (_metric_control_request_header_unmarshall(&req, buf, recv_len) < 0)
    {
      _send_metric_control_response(fd,
                                    version,
                                    CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_PACKET_INVALID);
      goto cleanup;
    }

  _metric_control_request_dump(&req);

  if (req.command == CEREBRO_METRIC_CONTROL_PROTOCOL_CMD_REGISTER)
    {
      struct cerebrod_speaker_metric_info *metric_info;
      char metric_name_buf[CEREBRO_MAX_METRIC_NAME_LEN+1];
      ListIterator itr = NULL;

      /* Guarantee ending '\0' character */
      memset(metric_name_buf, '\0', CEREBRO_MAX_METRIC_NAME_LEN+1);
      memcpy(metric_name_buf, req.metric_name, CEREBRO_MAX_METRIC_NAME_LEN);

      Pthread_mutex_lock(&metric_list_lock);
      itr = List_iterator_create(metric_list);
      while ((metric_info = list_next(itr)))
        {
          if (!strcmp(metric_info->metric_name, metric_name_buf))
            {
              _send_metric_control_response(fd,
                                            version,
                                            CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_METRIC_INVALID);
              List_iterator_destroy(itr);
              Pthread_mutex_unlock(&metric_list_lock);
              goto cleanup;
            }
        }
      metric_info = NULL;
      List_iterator_destroy(itr);

      metric_info = Malloc(sizeof(struct cerebrod_speaker_metric_info));
      metric_info->metric_name = Strdup(metric_name_buf);
      metric_info->metric_origin = CEREBROD_METRIC_ORIGIN_USERSPACE;
      /* 
       * Setting next_call_time to UINT_MAX means the data will never
       * be sent.
       */
      metric_info->next_call_time = UINT_MAX;
      List_append(metric_list, metric_info);
      cerebrod_speaker_data_metric_list_sort();
      Pthread_mutex_unlock(&metric_list_lock);
    }
  else if (req.command == CEREBRO_METRIC_CONTROL_PROTOCOL_CMD_UNREGISTER)
    {
      struct cerebrod_speaker_metric_info *metric_info;
      char metric_name_buf[CEREBRO_MAX_METRIC_NAME_LEN+1];
      ListIterator itr = NULL;
      int found = 0;
      /* Guarantee ending '\0' character */
      memset(metric_name_buf, '\0', CEREBRO_MAX_METRIC_NAME_LEN+1);
      memcpy(metric_name_buf, req.metric_name, CEREBRO_MAX_METRIC_NAME_LEN);

      Pthread_mutex_lock(&metric_list_lock);
      itr = List_iterator_create(metric_list);
      while ((metric_info = list_next(itr)))
        {
          if (!strcmp(metric_info->metric_name, metric_name_buf))
            {
              found++;
              break;
            }
        }

      if (!found || 
          !(metric_info->metric_origin & CEREBROD_METRIC_ORIGIN_USERSPACE))
        {
          _send_metric_control_response(fd,
                                        version,
                                        CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_METRIC_INVALID);
          List_iterator_destroy(itr);
          Pthread_mutex_unlock(&metric_list_lock);
          goto cleanup;
        }

      List_delete(itr);
      List_iterator_destroy(itr);
      Pthread_mutex_unlock(&metric_list_lock);
    }
  else if (req.command == CEREBRO_METRIC_CONTROL_PROTOCOL_CMD_UPDATE)
    {
      /* XXX 
       *
       * need to sort 
       *
       * check for name and userspace flag
       */
    }
  else
    {
      _send_metric_control_response(fd,
                                    version,
                                    CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_PARAMETER_INVALID);
      goto cleanup;
    }

  _send_metric_control_response(fd,
                                version,
                                CEREBRO_METRIC_CONTROL_PROTOCOL_ERR_SUCCESS);

 cleanup:
  Free(arg);
  Close(fd);
  return NULL;
}

void *
cerebrod_metric_controller(void *arg)
{
  int controller_fd;

  _metric_controller_initialize();

  if ((controller_fd = _metric_controller_setup_socket()) < 0)
    CEREBRO_EXIT(("fd setup failed"));

  for (;;)
    {
      pthread_t thread;
      pthread_attr_t attr;
      int fd, client_addr_len, *arg;
      struct sockaddr_un client_addr;
      
      client_addr_len = sizeof(struct sockaddr_un);
      if ((fd = accept(controller_fd,
                       (struct sockaddr *)&client_addr,
                       &client_addr_len)) < 0)
        controller_fd = cerebrod_reinit_socket(controller_fd,
                                               _metric_controller_setup_socket,
                                               "metric_controller: accept");
      
      if (fd < 0)
        continue;
                                                                                      
      /* Pass off connection to thread */
      Pthread_attr_init(&attr);
      Pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      Pthread_attr_setstacksize(&attr, CEREBROD_THREAD_STACKSIZE);
      arg = Malloc(sizeof(int));
      *arg = fd;
      Pthread_create(&thread,
                     &attr,
                     _metric_controller_service_connection,
                     (void *)arg);
      Pthread_attr_destroy(&attr);
    }

  return NULL;			/* NOT REACHED */
}
