/*****************************************************************************\
 *  $Id: metric_util.h,v 1.3 2005-07-19 23:41:13 achu Exp $
\*****************************************************************************/

#ifndef _METRIC_UTIL_H
#define _METRIC_UTIL_H

#include <sys/types.h>

#define check_metric_type_len(t,l) \
        _check_metric_type_len(t,l,__FUNCTION__)

#define check_metric_type_len_value(t,l,v) \
        _check_metric_type_len_value(t,l,v,__FUNCTION__)

#define marshall_metric(t,l,v,b,bl,e) \
        _marshall_metric(t,l,v,b,bl,e,__FUNCTION__)

/* 
 * _check_metric_type_len
 *
 * Check if metric type and len are reasonable
 *
 * Returns 0 if data is sane, -1 if not
 */
int _check_metric_type_len(u_int32_t mtype, 
                           u_int32_t mlen, 
                           const char *caller);

/* 
 * _check_metric_type_len_value
 *
 * Check if metric type, len, and value are reasonable
 *
 * Returns 0 if data is sane, -1 if not
 */
int _check_metric_type_len_value(u_int32_t mtype, 
                                 u_int32_t mlen, 
                                 void *mvalue,
                                 const char *caller);

/* 
 * _marshall_metric
 *
 * Marshall metric type, len, and if appropriate the value
 *
 * Returns bytes written to buffer on success, -1 on error
 */
int _marshall_metric(u_int32_t mtype,
                           u_int32_t mlen,
                           void *mvalue,
                           char *buf,
                           unsigned int buflen,
                           int *errnum,
                           const char *caller);
     
#endif /* _METRIC_UTIL_H */
