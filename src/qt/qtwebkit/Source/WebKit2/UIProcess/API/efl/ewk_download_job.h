/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_download_job.h
 * @brief   Describes the Download Job API.
 *
 * @note Ewk_Download_Job encapsulates a WebKit download job in order to provide
 * information about it and interact with it (e.g. set the destination
 * path, cancel the download, ...).
 */

#ifndef ewk_download_job_h
#define ewk_download_job_h

#include "ewk_url_request.h"
#include "ewk_url_response.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare Ewk_Download_Job as Ewk_Object.
 *
 * @see Ewk_Object
 */
typedef struct EwkObject Ewk_Download_Job;

/// Defines the possible states of a download.
enum Ewk_Download_Job_State {
    /// The download state is unknown
    EWK_DOWNLOAD_JOB_STATE_UNKNOWN = -1,
    /// The download has not started yet
    EWK_DOWNLOAD_JOB_STATE_NOT_STARTED,
    /// The download has started
    EWK_DOWNLOAD_JOB_STATE_DOWNLOADING,
    /// The download stopped because of a failure
    EWK_DOWNLOAD_JOB_STATE_FAILED,
    /// The download is being cancelled
    EWK_DOWNLOAD_JOB_STATE_CANCELLING,
    /// The download stopped because it was cancelled
    EWK_DOWNLOAD_JOB_STATE_CANCELLED,
    /// The download completed successfully.
    EWK_DOWNLOAD_JOB_STATE_FINISHED
};
/// Creates a type name for @a Ewk_Download_Job_State.
typedef enum Ewk_Download_Job_State Ewk_Download_Job_State;

/**
 * Query the state for this download.
 *
 * @param download a #Ewk_Download_Job to query.
 *
 * @return the download state.
 */
EAPI Ewk_Download_Job_State ewk_download_job_state_get(const Ewk_Download_Job *download);

/**
 * Query the URL request for this download.
 *
 * @param download a #Ewk_Download_Job to query.
 *
 * @return the #Ewk_Url_Request for this download.
 */
EAPI Ewk_Url_Request *ewk_download_job_request_get(const Ewk_Download_Job *download);

/**
 * Query the URL response for this download.
 *
 * @param download a #Ewk_Download_Job to query.
 *
 * @return the #Ewk_Url_Response for this download or @c NULL if it was not received yet.
 */
EAPI Ewk_Url_Response *ewk_download_job_response_get(const Ewk_Download_Job *download);

/**
 * Query the URL to which the downloaded file will be written.
 *
 * @param download a #Ewk_Download_Job to query.
 *
 * @return the destination pointer, that may be @c NULL. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 */
EAPI const char *ewk_download_job_destination_get(const Ewk_Download_Job *download);

/**
 * Sets the destination path for this download.
 *
 * Sets the path to which the downloaded file will be written.
 *
 * This method needs to be called before the download transfer
 * starts, by connecting to the "download,new" signal on the
 * Ewk_View and setting the destination in the callback. To set
 * the destination using the filename suggested by the server
 * use ewk_download_job_suggested_filename_get().
 *
 * @param download #Ewk_Download_Job to update.
 * @param destination the destination path.
 *
 * @return @c EINA_TRUE if successful, @c EINA_FALSE otherwise.
 *
 * @see ewk_download_job_suggested_filename_get
 */
EAPI Eina_Bool ewk_download_job_destination_set(Ewk_Download_Job *download, const char *destination);

/**
 * Queries the suggested file name for this download.
 *
 * It can be useful to use the value returned by this function to construct
 * the destination path to pass to ewk_download_job_destination_set().
 *
 * @param download #Ewk_Download_Job to query.
 *
 * @return The suggested file name for this download. This pointer is
 *         guaranteed to be eina_stringshare, so whenever possible
 *         save yourself some cpu cycles and use
 *         eina_stringshare_ref() instead of eina_stringshare_add() or
 *         strdup().
 *
 * @see ewk_download_job_destination_set
 */
EAPI const char *ewk_download_job_suggested_filename_get(const Ewk_Download_Job *download);

/**
 * Cancels the download asynchronously.
 *
 * When the ongoing download operation is effectively cancelled a "download,cancelled"
 * signal will be emitted on the view.
 *
 * @param download a #Ewk_Download_Job to cancel.
 *
 * @return @c EINA_TRUE if the cancellation request was taken into account, or
 *         @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_download_job_cancel(Ewk_Download_Job *download);

/**
 * Query the estimated progress for this download.
 *
 * @param download a #Ewk_Download_Job to query.
 *
 * @return an estimate of the of the percent complete for a download
 *         as a range from 0.0 to 1.0.
 */
EAPI double ewk_download_job_estimated_progress_get(const Ewk_Download_Job *download);

/**
 * Gets the elapsed time in seconds, including any fractional part.
 *
 * If the download finished, had an error or was cancelled this is
 * the time between its start and the event.
 *
 * @param download a #Ewk_Download_Job
 *
 * @return seconds since the download was started or 0.0 in case of failure.
 */
EAPI double ewk_download_job_elapsed_time_get(const Ewk_Download_Job *download);

#ifdef __cplusplus
}
#endif

#endif // ewk_download_job_h
