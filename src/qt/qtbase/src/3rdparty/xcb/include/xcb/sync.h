/*
 * This file generated automatically from sync.xml by c_client.py.
 * Edit at your peril.
 */

/**
 * @defgroup XCB_Sync_API XCB Sync API
 * @brief Sync XCB Protocol Implementation.
 * @{
 **/

#ifndef __SYNC_H
#define __SYNC_H

#include "xcb.h"
#include "xproto.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XCB_SYNC_MAJOR_VERSION 3
#define XCB_SYNC_MINOR_VERSION 0
  
extern xcb_extension_t xcb_sync_id;

typedef uint32_t xcb_sync_alarm_t;

/**
 * @brief xcb_sync_alarm_iterator_t
 **/
typedef struct xcb_sync_alarm_iterator_t {
    xcb_sync_alarm_t *data; /**<  */
    int               rem; /**<  */
    int               index; /**<  */
} xcb_sync_alarm_iterator_t;

typedef enum xcb_sync_alarmstate_t {
    XCB_SYNC_ALARMSTATE_ACTIVE,
    XCB_SYNC_ALARMSTATE_INACTIVE,
    XCB_SYNC_ALARMSTATE_DESTROYED
} xcb_sync_alarmstate_t;

typedef uint32_t xcb_sync_counter_t;

/**
 * @brief xcb_sync_counter_iterator_t
 **/
typedef struct xcb_sync_counter_iterator_t {
    xcb_sync_counter_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_sync_counter_iterator_t;

typedef enum xcb_sync_testtype_t {
    XCB_SYNC_TESTTYPE_POSITIVE_TRANSITION,
    XCB_SYNC_TESTTYPE_NEGATIVE_TRANSITION,
    XCB_SYNC_TESTTYPE_POSITIVE_COMPARISON,
    XCB_SYNC_TESTTYPE_NEGATIVE_COMPARISON
} xcb_sync_testtype_t;

typedef enum xcb_sync_valuetype_t {
    XCB_SYNC_VALUETYPE_ABSOLUTE,
    XCB_SYNC_VALUETYPE_RELATIVE
} xcb_sync_valuetype_t;

typedef enum xcb_sync_ca_t {
    XCB_SYNC_CA_COUNTER = 1,
    XCB_SYNC_CA_VALUE_TYPE = 2,
    XCB_SYNC_CA_VALUE = 4,
    XCB_SYNC_CA_TEST_TYPE = 8,
    XCB_SYNC_CA_DELTA = 16,
    XCB_SYNC_CA_EVENTS = 32
} xcb_sync_ca_t;

/**
 * @brief xcb_sync_int64_t
 **/
typedef struct xcb_sync_int64_t {
    int32_t  hi; /**<  */
    uint32_t lo; /**<  */
} xcb_sync_int64_t;

/**
 * @brief xcb_sync_int64_iterator_t
 **/
typedef struct xcb_sync_int64_iterator_t {
    xcb_sync_int64_t *data; /**<  */
    int               rem; /**<  */
    int               index; /**<  */
} xcb_sync_int64_iterator_t;

/**
 * @brief xcb_sync_systemcounter_t
 **/
typedef struct xcb_sync_systemcounter_t {
    xcb_sync_counter_t counter; /**<  */
    xcb_sync_int64_t   resolution; /**<  */
    uint16_t           name_len; /**<  */
} xcb_sync_systemcounter_t;

/**
 * @brief xcb_sync_systemcounter_iterator_t
 **/
typedef struct xcb_sync_systemcounter_iterator_t {
    xcb_sync_systemcounter_t *data; /**<  */
    int                       rem; /**<  */
    int                       index; /**<  */
} xcb_sync_systemcounter_iterator_t;

/**
 * @brief xcb_sync_trigger_t
 **/
typedef struct xcb_sync_trigger_t {
    xcb_sync_counter_t counter; /**<  */
    uint32_t           wait_type; /**<  */
    xcb_sync_int64_t   wait_value; /**<  */
    uint32_t           test_type; /**<  */
} xcb_sync_trigger_t;

/**
 * @brief xcb_sync_trigger_iterator_t
 **/
typedef struct xcb_sync_trigger_iterator_t {
    xcb_sync_trigger_t *data; /**<  */
    int                 rem; /**<  */
    int                 index; /**<  */
} xcb_sync_trigger_iterator_t;

/**
 * @brief xcb_sync_waitcondition_t
 **/
typedef struct xcb_sync_waitcondition_t {
    xcb_sync_trigger_t trigger; /**<  */
    xcb_sync_int64_t   event_threshold; /**<  */
} xcb_sync_waitcondition_t;

/**
 * @brief xcb_sync_waitcondition_iterator_t
 **/
typedef struct xcb_sync_waitcondition_iterator_t {
    xcb_sync_waitcondition_t *data; /**<  */
    int                       rem; /**<  */
    int                       index; /**<  */
} xcb_sync_waitcondition_iterator_t;

/** Opcode for xcb_sync_counter. */
#define XCB_SYNC_COUNTER 0

/**
 * @brief xcb_sync_counter_error_t
 **/
typedef struct xcb_sync_counter_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
    uint32_t bad_counter; /**<  */
    uint16_t minor_opcode; /**<  */
    uint8_t  major_opcode; /**<  */
} xcb_sync_counter_error_t;

/** Opcode for xcb_sync_alarm. */
#define XCB_SYNC_ALARM 1

/**
 * @brief xcb_sync_alarm_error_t
 **/
typedef struct xcb_sync_alarm_error_t {
    uint8_t  response_type; /**<  */
    uint8_t  error_code; /**<  */
    uint16_t sequence; /**<  */
    uint32_t bad_alarm; /**<  */
    uint16_t minor_opcode; /**<  */
    uint8_t  major_opcode; /**<  */
} xcb_sync_alarm_error_t;

/**
 * @brief xcb_sync_initialize_cookie_t
 **/
typedef struct xcb_sync_initialize_cookie_t {
    unsigned int sequence; /**<  */
} xcb_sync_initialize_cookie_t;

/** Opcode for xcb_sync_initialize. */
#define XCB_SYNC_INITIALIZE 0

/**
 * @brief xcb_sync_initialize_request_t
 **/
typedef struct xcb_sync_initialize_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint8_t  desired_major_version; /**<  */
    uint8_t  desired_minor_version; /**<  */
} xcb_sync_initialize_request_t;

/**
 * @brief xcb_sync_initialize_reply_t
 **/
typedef struct xcb_sync_initialize_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint8_t  major_version; /**<  */
    uint8_t  minor_version; /**<  */
    uint8_t  pad1[22]; /**<  */
} xcb_sync_initialize_reply_t;

/**
 * @brief xcb_sync_list_system_counters_cookie_t
 **/
typedef struct xcb_sync_list_system_counters_cookie_t {
    unsigned int sequence; /**<  */
} xcb_sync_list_system_counters_cookie_t;

/** Opcode for xcb_sync_list_system_counters. */
#define XCB_SYNC_LIST_SYSTEM_COUNTERS 1

/**
 * @brief xcb_sync_list_system_counters_request_t
 **/
typedef struct xcb_sync_list_system_counters_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
} xcb_sync_list_system_counters_request_t;

/**
 * @brief xcb_sync_list_system_counters_reply_t
 **/
typedef struct xcb_sync_list_system_counters_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    uint32_t counters_len; /**<  */
    uint8_t  pad1[20]; /**<  */
} xcb_sync_list_system_counters_reply_t;

/** Opcode for xcb_sync_create_counter. */
#define XCB_SYNC_CREATE_COUNTER 2

/**
 * @brief xcb_sync_create_counter_request_t
 **/
typedef struct xcb_sync_create_counter_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_sync_counter_t id; /**<  */
    xcb_sync_int64_t   initial_value; /**<  */
} xcb_sync_create_counter_request_t;

/** Opcode for xcb_sync_destroy_counter. */
#define XCB_SYNC_DESTROY_COUNTER 6

/**
 * @brief xcb_sync_destroy_counter_request_t
 **/
typedef struct xcb_sync_destroy_counter_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_sync_counter_t counter; /**<  */
} xcb_sync_destroy_counter_request_t;

/**
 * @brief xcb_sync_query_counter_cookie_t
 **/
typedef struct xcb_sync_query_counter_cookie_t {
    unsigned int sequence; /**<  */
} xcb_sync_query_counter_cookie_t;

/** Opcode for xcb_sync_query_counter. */
#define XCB_SYNC_QUERY_COUNTER 5

/**
 * @brief xcb_sync_query_counter_request_t
 **/
typedef struct xcb_sync_query_counter_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_sync_counter_t counter; /**<  */
} xcb_sync_query_counter_request_t;

/**
 * @brief xcb_sync_query_counter_reply_t
 **/
typedef struct xcb_sync_query_counter_reply_t {
    uint8_t          response_type; /**<  */
    uint8_t          pad0; /**<  */
    uint16_t         sequence; /**<  */
    uint32_t         length; /**<  */
    xcb_sync_int64_t counter_value; /**<  */
} xcb_sync_query_counter_reply_t;

/** Opcode for xcb_sync_await. */
#define XCB_SYNC_AWAIT 7

/**
 * @brief xcb_sync_await_request_t
 **/
typedef struct xcb_sync_await_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
} xcb_sync_await_request_t;

/** Opcode for xcb_sync_change_counter. */
#define XCB_SYNC_CHANGE_COUNTER 4

/**
 * @brief xcb_sync_change_counter_request_t
 **/
typedef struct xcb_sync_change_counter_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_sync_counter_t counter; /**<  */
    xcb_sync_int64_t   amount; /**<  */
} xcb_sync_change_counter_request_t;

/** Opcode for xcb_sync_set_counter. */
#define XCB_SYNC_SET_COUNTER 3

/**
 * @brief xcb_sync_set_counter_request_t
 **/
typedef struct xcb_sync_set_counter_request_t {
    uint8_t            major_opcode; /**<  */
    uint8_t            minor_opcode; /**<  */
    uint16_t           length; /**<  */
    xcb_sync_counter_t counter; /**<  */
    xcb_sync_int64_t   value; /**<  */
} xcb_sync_set_counter_request_t;

/** Opcode for xcb_sync_create_alarm. */
#define XCB_SYNC_CREATE_ALARM 8

/**
 * @brief xcb_sync_create_alarm_request_t
 **/
typedef struct xcb_sync_create_alarm_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_sync_alarm_t id; /**<  */
    uint32_t         value_mask; /**<  */
} xcb_sync_create_alarm_request_t;

/** Opcode for xcb_sync_change_alarm. */
#define XCB_SYNC_CHANGE_ALARM 9

/**
 * @brief xcb_sync_change_alarm_request_t
 **/
typedef struct xcb_sync_change_alarm_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_sync_alarm_t id; /**<  */
    uint32_t         value_mask; /**<  */
} xcb_sync_change_alarm_request_t;

/** Opcode for xcb_sync_destroy_alarm. */
#define XCB_SYNC_DESTROY_ALARM 11

/**
 * @brief xcb_sync_destroy_alarm_request_t
 **/
typedef struct xcb_sync_destroy_alarm_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_sync_alarm_t alarm; /**<  */
} xcb_sync_destroy_alarm_request_t;

/**
 * @brief xcb_sync_query_alarm_cookie_t
 **/
typedef struct xcb_sync_query_alarm_cookie_t {
    unsigned int sequence; /**<  */
} xcb_sync_query_alarm_cookie_t;

/** Opcode for xcb_sync_query_alarm. */
#define XCB_SYNC_QUERY_ALARM 10

/**
 * @brief xcb_sync_query_alarm_request_t
 **/
typedef struct xcb_sync_query_alarm_request_t {
    uint8_t          major_opcode; /**<  */
    uint8_t          minor_opcode; /**<  */
    uint16_t         length; /**<  */
    xcb_sync_alarm_t alarm; /**<  */
} xcb_sync_query_alarm_request_t;

/**
 * @brief xcb_sync_query_alarm_reply_t
 **/
typedef struct xcb_sync_query_alarm_reply_t {
    uint8_t            response_type; /**<  */
    uint8_t            pad0; /**<  */
    uint16_t           sequence; /**<  */
    uint32_t           length; /**<  */
    xcb_sync_trigger_t trigger; /**<  */
    xcb_sync_int64_t   delta; /**<  */
    uint8_t            events; /**<  */
    uint8_t            state; /**<  */
    uint8_t            pad1[2]; /**<  */
} xcb_sync_query_alarm_reply_t;

/** Opcode for xcb_sync_set_priority. */
#define XCB_SYNC_SET_PRIORITY 12

/**
 * @brief xcb_sync_set_priority_request_t
 **/
typedef struct xcb_sync_set_priority_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint32_t id; /**<  */
    int32_t  priority; /**<  */
} xcb_sync_set_priority_request_t;

/**
 * @brief xcb_sync_get_priority_cookie_t
 **/
typedef struct xcb_sync_get_priority_cookie_t {
    unsigned int sequence; /**<  */
} xcb_sync_get_priority_cookie_t;

/** Opcode for xcb_sync_get_priority. */
#define XCB_SYNC_GET_PRIORITY 13

/**
 * @brief xcb_sync_get_priority_request_t
 **/
typedef struct xcb_sync_get_priority_request_t {
    uint8_t  major_opcode; /**<  */
    uint8_t  minor_opcode; /**<  */
    uint16_t length; /**<  */
    uint32_t id; /**<  */
} xcb_sync_get_priority_request_t;

/**
 * @brief xcb_sync_get_priority_reply_t
 **/
typedef struct xcb_sync_get_priority_reply_t {
    uint8_t  response_type; /**<  */
    uint8_t  pad0; /**<  */
    uint16_t sequence; /**<  */
    uint32_t length; /**<  */
    int32_t  priority; /**<  */
} xcb_sync_get_priority_reply_t;

/** Opcode for xcb_sync_counter_notify. */
#define XCB_SYNC_COUNTER_NOTIFY 0

/**
 * @brief xcb_sync_counter_notify_event_t
 **/
typedef struct xcb_sync_counter_notify_event_t {
    uint8_t            response_type; /**<  */
    uint8_t            kind; /**<  */
    uint16_t           sequence; /**<  */
    xcb_sync_counter_t counter; /**<  */
    xcb_sync_int64_t   wait_value; /**<  */
    xcb_sync_int64_t   counter_value; /**<  */
    xcb_timestamp_t    timestamp; /**<  */
    uint16_t           count; /**<  */
    uint8_t            destroyed; /**<  */
    uint8_t            pad0; /**<  */
} xcb_sync_counter_notify_event_t;

/** Opcode for xcb_sync_alarm_notify. */
#define XCB_SYNC_ALARM_NOTIFY 1

/**
 * @brief xcb_sync_alarm_notify_event_t
 **/
typedef struct xcb_sync_alarm_notify_event_t {
    uint8_t          response_type; /**<  */
    uint8_t          kind; /**<  */
    uint16_t         sequence; /**<  */
    xcb_sync_alarm_t alarm; /**<  */
    xcb_sync_int64_t counter_value; /**<  */
    xcb_sync_int64_t alarm_value; /**<  */
    xcb_timestamp_t  timestamp; /**<  */
    uint8_t          state; /**<  */
    uint8_t          pad0[3]; /**<  */
} xcb_sync_alarm_notify_event_t;

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_alarm_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_alarm_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_alarm_next
 ** 
 ** @param xcb_sync_alarm_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_alarm_next (xcb_sync_alarm_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_alarm_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_alarm_end
 ** 
 ** @param xcb_sync_alarm_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_alarm_end (xcb_sync_alarm_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_counter_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_counter_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_counter_next
 ** 
 ** @param xcb_sync_counter_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_counter_next (xcb_sync_counter_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_counter_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_counter_end
 ** 
 ** @param xcb_sync_counter_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_counter_end (xcb_sync_counter_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_int64_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_int64_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_int64_next
 ** 
 ** @param xcb_sync_int64_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_int64_next (xcb_sync_int64_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_int64_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_int64_end
 ** 
 ** @param xcb_sync_int64_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_int64_end (xcb_sync_int64_iterator_t i  /**< */);


/*****************************************************************************
 **
 ** char * xcb_sync_systemcounter_name
 ** 
 ** @param const xcb_sync_systemcounter_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_sync_systemcounter_name (const xcb_sync_systemcounter_t *R  /**< */);


/*****************************************************************************
 **
 ** int xcb_sync_systemcounter_name_length
 ** 
 ** @param const xcb_sync_systemcounter_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_sync_systemcounter_name_length (const xcb_sync_systemcounter_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_systemcounter_name_end
 ** 
 ** @param const xcb_sync_systemcounter_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_systemcounter_name_end (const xcb_sync_systemcounter_t *R  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_systemcounter_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_systemcounter_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_systemcounter_next
 ** 
 ** @param xcb_sync_systemcounter_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_systemcounter_next (xcb_sync_systemcounter_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_systemcounter_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_systemcounter_end
 ** 
 ** @param xcb_sync_systemcounter_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_systemcounter_end (xcb_sync_systemcounter_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_trigger_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_trigger_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_trigger_next
 ** 
 ** @param xcb_sync_trigger_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_trigger_next (xcb_sync_trigger_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_trigger_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_trigger_end
 ** 
 ** @param xcb_sync_trigger_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_trigger_end (xcb_sync_trigger_iterator_t i  /**< */);

/**
 * Get the next element of the iterator
 * @param i Pointer to a xcb_sync_waitcondition_iterator_t
 *
 * Get the next element in the iterator. The member rem is
 * decreased by one. The member data points to the next
 * element. The member index is increased by sizeof(xcb_sync_waitcondition_t)
 */

/*****************************************************************************
 **
 ** void xcb_sync_waitcondition_next
 ** 
 ** @param xcb_sync_waitcondition_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_sync_waitcondition_next (xcb_sync_waitcondition_iterator_t *i  /**< */);

/**
 * Return the iterator pointing to the last element
 * @param i An xcb_sync_waitcondition_iterator_t
 * @return  The iterator pointing to the last element
 *
 * Set the current element in the iterator to the last element.
 * The member rem is set to 0. The member data points to the
 * last element.
 */

/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_sync_waitcondition_end
 ** 
 ** @param xcb_sync_waitcondition_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_sync_waitcondition_end (xcb_sync_waitcondition_iterator_t i  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_sync_initialize_cookie_t xcb_sync_initialize
 ** 
 ** @param xcb_connection_t *c
 ** @param uint8_t           desired_major_version
 ** @param uint8_t           desired_minor_version
 ** @returns xcb_sync_initialize_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_initialize_cookie_t
xcb_sync_initialize (xcb_connection_t *c  /**< */,
                     uint8_t           desired_major_version  /**< */,
                     uint8_t           desired_minor_version  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_sync_initialize_cookie_t xcb_sync_initialize_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint8_t           desired_major_version
 ** @param uint8_t           desired_minor_version
 ** @returns xcb_sync_initialize_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_initialize_cookie_t
xcb_sync_initialize_unchecked (xcb_connection_t *c  /**< */,
                               uint8_t           desired_major_version  /**< */,
                               uint8_t           desired_minor_version  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_sync_initialize_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_sync_initialize_reply_t * xcb_sync_initialize_reply
 ** 
 ** @param xcb_connection_t              *c
 ** @param xcb_sync_initialize_cookie_t   cookie
 ** @param xcb_generic_error_t          **e
 ** @returns xcb_sync_initialize_reply_t *
 **
 *****************************************************************************/
 
xcb_sync_initialize_reply_t *
xcb_sync_initialize_reply (xcb_connection_t              *c  /**< */,
                           xcb_sync_initialize_cookie_t   cookie  /**< */,
                           xcb_generic_error_t          **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_sync_list_system_counters_cookie_t xcb_sync_list_system_counters
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_sync_list_system_counters_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_list_system_counters_cookie_t
xcb_sync_list_system_counters (xcb_connection_t *c  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_sync_list_system_counters_cookie_t xcb_sync_list_system_counters_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @returns xcb_sync_list_system_counters_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_list_system_counters_cookie_t
xcb_sync_list_system_counters_unchecked (xcb_connection_t *c  /**< */);


/*****************************************************************************
 **
 ** int xcb_sync_list_system_counters_counters_length
 ** 
 ** @param const xcb_sync_list_system_counters_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_sync_list_system_counters_counters_length (const xcb_sync_list_system_counters_reply_t *R  /**< */);


/*****************************************************************************
 **
 ** xcb_sync_systemcounter_iterator_t xcb_sync_list_system_counters_counters_iterator
 ** 
 ** @param const xcb_sync_list_system_counters_reply_t *R
 ** @returns xcb_sync_systemcounter_iterator_t
 **
 *****************************************************************************/
 
xcb_sync_systemcounter_iterator_t
xcb_sync_list_system_counters_counters_iterator (const xcb_sync_list_system_counters_reply_t *R  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_sync_list_system_counters_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_sync_list_system_counters_reply_t * xcb_sync_list_system_counters_reply
 ** 
 ** @param xcb_connection_t                        *c
 ** @param xcb_sync_list_system_counters_cookie_t   cookie
 ** @param xcb_generic_error_t                    **e
 ** @returns xcb_sync_list_system_counters_reply_t *
 **
 *****************************************************************************/
 
xcb_sync_list_system_counters_reply_t *
xcb_sync_list_system_counters_reply (xcb_connection_t                        *c  /**< */,
                                     xcb_sync_list_system_counters_cookie_t   cookie  /**< */,
                                     xcb_generic_error_t                    **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_create_counter_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  id
 ** @param xcb_sync_int64_t    initial_value
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_create_counter_checked (xcb_connection_t   *c  /**< */,
                                 xcb_sync_counter_t  id  /**< */,
                                 xcb_sync_int64_t    initial_value  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_create_counter
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  id
 ** @param xcb_sync_int64_t    initial_value
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_create_counter (xcb_connection_t   *c  /**< */,
                         xcb_sync_counter_t  id  /**< */,
                         xcb_sync_int64_t    initial_value  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_destroy_counter_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_destroy_counter_checked (xcb_connection_t   *c  /**< */,
                                  xcb_sync_counter_t  counter  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_destroy_counter
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_destroy_counter (xcb_connection_t   *c  /**< */,
                          xcb_sync_counter_t  counter  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_sync_query_counter_cookie_t xcb_sync_query_counter
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @returns xcb_sync_query_counter_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_query_counter_cookie_t
xcb_sync_query_counter (xcb_connection_t   *c  /**< */,
                        xcb_sync_counter_t  counter  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_sync_query_counter_cookie_t xcb_sync_query_counter_unchecked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @returns xcb_sync_query_counter_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_query_counter_cookie_t
xcb_sync_query_counter_unchecked (xcb_connection_t   *c  /**< */,
                                  xcb_sync_counter_t  counter  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_sync_query_counter_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_sync_query_counter_reply_t * xcb_sync_query_counter_reply
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_sync_query_counter_cookie_t   cookie
 ** @param xcb_generic_error_t             **e
 ** @returns xcb_sync_query_counter_reply_t *
 **
 *****************************************************************************/
 
xcb_sync_query_counter_reply_t *
xcb_sync_query_counter_reply (xcb_connection_t                 *c  /**< */,
                              xcb_sync_query_counter_cookie_t   cookie  /**< */,
                              xcb_generic_error_t             **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_await_checked
 ** 
 ** @param xcb_connection_t               *c
 ** @param uint32_t                        wait_list_len
 ** @param const xcb_sync_waitcondition_t *wait_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_await_checked (xcb_connection_t               *c  /**< */,
                        uint32_t                        wait_list_len  /**< */,
                        const xcb_sync_waitcondition_t *wait_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_await
 ** 
 ** @param xcb_connection_t               *c
 ** @param uint32_t                        wait_list_len
 ** @param const xcb_sync_waitcondition_t *wait_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_await (xcb_connection_t               *c  /**< */,
                uint32_t                        wait_list_len  /**< */,
                const xcb_sync_waitcondition_t *wait_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_change_counter_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @param xcb_sync_int64_t    amount
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_change_counter_checked (xcb_connection_t   *c  /**< */,
                                 xcb_sync_counter_t  counter  /**< */,
                                 xcb_sync_int64_t    amount  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_change_counter
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @param xcb_sync_int64_t    amount
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_change_counter (xcb_connection_t   *c  /**< */,
                         xcb_sync_counter_t  counter  /**< */,
                         xcb_sync_int64_t    amount  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_set_counter_checked
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @param xcb_sync_int64_t    value
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_set_counter_checked (xcb_connection_t   *c  /**< */,
                              xcb_sync_counter_t  counter  /**< */,
                              xcb_sync_int64_t    value  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_set_counter
 ** 
 ** @param xcb_connection_t   *c
 ** @param xcb_sync_counter_t  counter
 ** @param xcb_sync_int64_t    value
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_set_counter (xcb_connection_t   *c  /**< */,
                      xcb_sync_counter_t  counter  /**< */,
                      xcb_sync_int64_t    value  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_create_alarm_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  id
 ** @param uint32_t          value_mask
 ** @param const uint32_t   *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_create_alarm_checked (xcb_connection_t *c  /**< */,
                               xcb_sync_alarm_t  id  /**< */,
                               uint32_t          value_mask  /**< */,
                               const uint32_t   *value_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_create_alarm
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  id
 ** @param uint32_t          value_mask
 ** @param const uint32_t   *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_create_alarm (xcb_connection_t *c  /**< */,
                       xcb_sync_alarm_t  id  /**< */,
                       uint32_t          value_mask  /**< */,
                       const uint32_t   *value_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_change_alarm_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  id
 ** @param uint32_t          value_mask
 ** @param const uint32_t   *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_change_alarm_checked (xcb_connection_t *c  /**< */,
                               xcb_sync_alarm_t  id  /**< */,
                               uint32_t          value_mask  /**< */,
                               const uint32_t   *value_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_change_alarm
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  id
 ** @param uint32_t          value_mask
 ** @param const uint32_t   *value_list
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_change_alarm (xcb_connection_t *c  /**< */,
                       xcb_sync_alarm_t  id  /**< */,
                       uint32_t          value_mask  /**< */,
                       const uint32_t   *value_list  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_destroy_alarm_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  alarm
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_destroy_alarm_checked (xcb_connection_t *c  /**< */,
                                xcb_sync_alarm_t  alarm  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_destroy_alarm
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  alarm
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_destroy_alarm (xcb_connection_t *c  /**< */,
                        xcb_sync_alarm_t  alarm  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_sync_query_alarm_cookie_t xcb_sync_query_alarm
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  alarm
 ** @returns xcb_sync_query_alarm_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_query_alarm_cookie_t
xcb_sync_query_alarm (xcb_connection_t *c  /**< */,
                      xcb_sync_alarm_t  alarm  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_sync_query_alarm_cookie_t xcb_sync_query_alarm_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param xcb_sync_alarm_t  alarm
 ** @returns xcb_sync_query_alarm_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_query_alarm_cookie_t
xcb_sync_query_alarm_unchecked (xcb_connection_t *c  /**< */,
                                xcb_sync_alarm_t  alarm  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_sync_query_alarm_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_sync_query_alarm_reply_t * xcb_sync_query_alarm_reply
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_sync_query_alarm_cookie_t   cookie
 ** @param xcb_generic_error_t           **e
 ** @returns xcb_sync_query_alarm_reply_t *
 **
 *****************************************************************************/
 
xcb_sync_query_alarm_reply_t *
xcb_sync_query_alarm_reply (xcb_connection_t               *c  /**< */,
                            xcb_sync_query_alarm_cookie_t   cookie  /**< */,
                            xcb_generic_error_t           **e  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will not cause
 * a reply to be generated. Any returned error will be
 * saved for handling by xcb_request_check().
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_set_priority_checked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          id
 ** @param int32_t           priority
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_set_priority_checked (xcb_connection_t *c  /**< */,
                               uint32_t          id  /**< */,
                               int32_t           priority  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_sync_set_priority
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          id
 ** @param int32_t           priority
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_sync_set_priority (xcb_connection_t *c  /**< */,
                       uint32_t          id  /**< */,
                       int32_t           priority  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 */

/*****************************************************************************
 **
 ** xcb_sync_get_priority_cookie_t xcb_sync_get_priority
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          id
 ** @returns xcb_sync_get_priority_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_get_priority_cookie_t
xcb_sync_get_priority (xcb_connection_t *c  /**< */,
                       uint32_t          id  /**< */);

/**
 * Delivers a request to the X server
 * @param c The connection
 * @return A cookie
 *
 * Delivers a request to the X server.
 * 
 * This form can be used only if the request will cause
 * a reply to be generated. Any returned error will be
 * placed in the event queue.
 */

/*****************************************************************************
 **
 ** xcb_sync_get_priority_cookie_t xcb_sync_get_priority_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint32_t          id
 ** @returns xcb_sync_get_priority_cookie_t
 **
 *****************************************************************************/
 
xcb_sync_get_priority_cookie_t
xcb_sync_get_priority_unchecked (xcb_connection_t *c  /**< */,
                                 uint32_t          id  /**< */);

/**
 * Return the reply
 * @param c      The connection
 * @param cookie The cookie
 * @param e      The xcb_generic_error_t supplied
 *
 * Returns the reply of the request asked by
 * 
 * The parameter @p e supplied to this function must be NULL if
 * xcb_sync_get_priority_unchecked(). is used.
 * Otherwise, it stores the error if any.
 *
 * The returned value must be freed by the caller using free().
 */

/*****************************************************************************
 **
 ** xcb_sync_get_priority_reply_t * xcb_sync_get_priority_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_sync_get_priority_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_sync_get_priority_reply_t *
 **
 *****************************************************************************/
 
xcb_sync_get_priority_reply_t *
xcb_sync_get_priority_reply (xcb_connection_t                *c  /**< */,
                             xcb_sync_get_priority_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */
