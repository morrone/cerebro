/*****************************************************************************\
 *  $Id: cerebro_metric_protocol.h,v 1.6 2005-05-26 18:23:38 achu Exp $
\*****************************************************************************/

#ifndef _CEREBRO_METRIC_PROTOCOL_H
#define _CEREBRO_METRIC_PROTOCOL_H

#include <sys/types.h>
#include <cerebro/cerebro_constants.h>

/* Metric server protocol
 *
 * Client -> Server
 * - Metric request.
 * 
 * Server -> Client
 * - Stream of node and node data responses indicating metric values.
 *   Stream of nodes returned will depend on the request.  After the
 *   stream of nodes is complete, an "end of responses" response will
 *   indicate the end of stream and completion of the request.
 *   This "end of responses" response will be sent even if no nodes are
 *   returned (i.e. only down nodes are requested, but all nodes are
 *   up, or no nodes are monitoring the indicated metric).

 * - On "normal" errors, the "end of responses" packet will contain 
 *   the error code.
 * - On version incompatability errors, older version responses
 *   will be sent from the server containing the error code.
 * - On version incompatability errors in which an older version does
 *   not exist and invalid packet format errors, an eight byte packet
 *   containing only the version and error code will be the response.
 *
 * Notes:
 * - Require a smaller timeout len for the server than the client, 
 *   so the server can respond to the client with a meaningful error
 *   message.
 */

#define CEREBRO_METRIC_PROTOCOL_VERSION                    1
#define CEREBRO_METRIC_PROTOCOL_SERVER_TIMEOUT_LEN         3
#define CEREBRO_METRIC_PROTOCOL_CLIENT_TIMEOUT_LEN         5
#define CEREBRO_METRIC_PROTOCOL_CONNECT_TIMEOUT_LEN        5

#define CEREBRO_METRIC_CLUSTER_NODES                       "cluster_nodes"
#define CEREBRO_METRIC_UP_NODES                            "up_nodes"
#define CEREBRO_METRIC_DOWN_NODES                          "down_nodes"
#define CEREBRO_METRIC_UPDOWN_STATE                        "updown_state"
#define CEREBRO_METRIC_STARTTIME                           "starttime"
#define CEREBRO_METRIC_BOOTTIME                            "boottime"

#define CEREBRO_METRIC_PROTOCOL_ERR_SUCCESS                0
#define CEREBRO_METRIC_PROTOCOL_ERR_VERSION_INVALID        1
#define CEREBRO_METRIC_PROTOCOL_ERR_METRIC_UNKNOWN         2
#define CEREBRO_METRIC_PROTOCOL_ERR_PARAMETER_INVALID      3
#define CEREBRO_METRIC_PROTOCOL_ERR_PACKET_INVALID         4
#define CEREBRO_METRIC_PROTOCOL_ERR_INTERNAL_SYSTEM_ERROR  5

#define CEREBRO_METRIC_PROTOCOL_IS_LAST_RESPONSE           1
#define CEREBRO_METRIC_PROTOCOL_IS_NOT_LAST_RESPONSE       0

/* 
 * Updown Definitions
 */
#if 0
#define CEREBRO_UPDOWN_STATE_NODE_UP              1
#define CEREBRO_UPDOWN_STATE_NODE_DOWN            0
#endif

#define CEREBRO_METRIC_SERVER_PORT           8852

#define CEREBRO_METRIC_MAX                   16

#define CEREBRO_METRIC_NAME_MAXLEN           32

#define CEREBRO_METRIC_STRING_MAXLEN         64

#define CEREBRO_METRIC_STRING_PARAM_MAXLEN   64

#define CEREBRO_METRIC_VALUE_LEN             CEREBRO_METRIC_STRING_MAXLEN

/*
 * cerebro_metric_type_t
 *
 * type for metric type
 */
typedef enum {
  CEREBRO_METRIC_TYPE_NONE = 0,
  CEREBRO_METRIC_TYPE_BOOL = 1,
  CEREBRO_METRIC_TYPE_INT32 = 2,
  CEREBRO_METRIC_TYPE_UNSIGNED_INT32 = 3,
  CEREBRO_METRIC_TYPE_FLOAT = 4,
  CEREBRO_METRIC_TYPE_DOUBLE = 5,
  CEREBRO_METRIC_TYPE_STRING = 6
} cerebro_metric_type_t;

/* 
 * cerebro_metric_value_t
 *
 * metric value
 */
typedef union {
  char      val_bool;
  int32_t   val_int32;
  u_int32_t val_unsigned_int32;
  float     val_float;
  double    val_double;
  char      val_string[CEREBRO_METRIC_STRING_MAXLEN];
} cerebro_metric_value_t;

/*
 * struct cerebro_metric_request
 *
 * defines a metric server data request
 */
struct cerebro_metric_request
{
  int32_t version;
  char metric_name[CEREBRO_METRIC_NAME_MAXLEN];
  int32_t int32_param1;
  u_int32_t unsigned_int32_param1;
  char string_param1[CEREBRO_METRIC_STRING_PARAM_MAXLEN];
};
  
#define CEREBRO_METRIC_REQUEST_LEN  (sizeof(int32_t) \
                                     + CEREBRO_METRIC_NAME_MAXLEN \
                                     + sizeof(int32_t) \
                                     + sizeof(u_int32_t) \
                                     + CEREBRO_METRIC_STRING_PARAM_MAXLEN)

/*
 * struct cerebro_metric_response
 *
 * defines a metric server data response
 */
struct cerebro_metric_response
{
  int32_t version;
  u_int32_t metric_err_code;
  u_int8_t end_of_responses;
  char nodename[CEREBRO_MAXNODENAMELEN];
  cerebro_metric_type_t metric_type;
  cerebro_metric_value_t metric_value;
};
  
#define CEREBRO_METRIC_RESPONSE_LEN  (sizeof(int32_t) \
                                      + sizeof(u_int32_t) \
                                      + sizeof(u_int8_t) \
                                      + CEREBRO_MAXNODENAMELEN \
                                      + sizeof(u_int32_t) \
                                      + CEREBRO_METRIC_VALUE_LEN)

/*
 * struct cerebro_metric_err_response
 *
 * defines a metric server invalid version or packet response
 */
struct cerebro_metric_err_response
{
  int32_t version;
  u_int32_t metric_err_code;
};
  
#define CEREBRO_METRIC_ERR_RESPONSE_LEN  (sizeof(int32_t) \
                                          + sizeof(u_int32_t))

#endif /* _CEREBRO_METRIC_PROTOCOL_H */
