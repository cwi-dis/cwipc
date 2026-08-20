#ifndef _cwipc_util_internal_logging_hpp_
#define _cwipc_util_internal_logging_hpp_

#include "cwipc_util/api.h"


extern "C" {
    /** Call to emit a log message.
     * Mainly meant for internal use within cwipc, so messages get forwarded to the correct recipient.
     */
    _CWIPC_UTIL_EXPORT void cwipc_log(cwipc_log_level level, std::string module, std::string message);
    /** Set error capture buffer.
     * Called internally by methods that have a char **errorMessage argument, and
     * cleared at the end of the method.
     * This will capture the most recent error here.
     */
    _CWIPC_UTIL_EXPORT void cwipc_log_set_errorbuf(char **errorbuf);
    /** Get current global log level
     * 
     */
    _CWIPC_UTIL_EXPORT cwipc_log_level cwipc_log_get_level();
};

#endif // _cwipc_util_internal_logging_hpp_
