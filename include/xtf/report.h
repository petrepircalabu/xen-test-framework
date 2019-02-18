#ifndef XTF_REPORT_H
#define XTF_REPORT_H

#include <xtf/types.h>
#include <xtf/compiler.h>

/**
 * @file include/xtf/report.h
 *
 * API for reporting test status.
 *
 * A test is expected to report one of:
 *  - Success
 *  - Skip
 *  - Error
 *  - Failure
 *
 * 'Success' indicates that everything went well, while 'Skip' indicates that
 * the test cannot be completed for configuration reasons.  'Error' indicates
 * a bug in the test code or environment itself, while 'Failure' indicates a
 * bug in the code under test.
 *
 * If multiple statuses are reported, the most severe is the one which is
 * kept.
 */

enum test_status {
    STATUS_RUNNING, /**< Test not yet completed.       */
    STATUS_SUCCESS, /**< Test was successful.          */
    STATUS_SKIP,    /**< Test cannot be completed.     */
    STATUS_ERROR,   /**< Issue with the test itself.   */
    STATUS_FAILURE, /**< Issue with the tested matter. */
};

/**
 * Report test success.
 */
void xtf_success(const char *fmt, ...) __printf(1, 2);

/**
 * Report a test warning.
 *
 * Does not count against a test success, but does indicate that some problem
 * was encountered.
 */
void xtf_warning(const char *fmt, ...) __printf(1, 2);

/**
 * Report a test skip.
 *
 * Indicates that the test cannot be completed.  This may count as a success
 * or failure, depending on the opinion of the tester.
 */
void xtf_skip(const char *fmt, ...) __printf(1, 2);

/**
 * Report a test error.
 *
 * Indicates an error with the test code, or environment, and not with the
 * subject matter under test.
 */
void xtf_error(const char *fmt, ...) __printf(1, 2);

/**
 * Report a test failure.
 *
 * Indicates that the subject matter under test has failed to match
 * expectation.
 */
void xtf_failure(const char *fmt, ...) __printf(1, 2);

/**
 * Print a status report.
 *
 * If a report has not yet been set, an error will occur.
 */
void xtf_report_status(void);

/**
 * Query whether a status has already been reported.
 */
bool xtf_status_reported(void);

/**
 * Exit the test early.
 *
 * Reports the current status.  Does not return.
 */
void __noreturn xtf_exit(void);

#endif /* XTF_REPORT_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
