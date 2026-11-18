#ifndef _cwipc_util_internal_logging_hpp_
#define _cwipc_util_internal_logging_hpp_

#include "cwipc_util/api.h"


extern "C" {
    /** Call to emit a log message.
     * Mainly meant for internal use within cwipc, so messages get forwarded to the correct recipient.
     */
    _CWIPC_UTIL_EXPORT void cwipc_log(cwipc_log_level level, std::string module, std::string message);
    /** Set error capture buffer.
     * Deprecated: use cwipc_errorbuf_scope (below) instead. Pairs of set/clear calls left the
     * buffer active on return paths that forgot the clear, which corrupted memory later (#384).
     * Still exported for compatibility.
     */
    [[deprecated("use cwipc_errorbuf_scope instead (cwi-dis/cwipc#384)")]]
    _CWIPC_UTIL_EXPORT void cwipc_log_set_errorbuf(char **errorbuf);
    /** Set error capture buffer, and return the previous one.
     * Used by cwipc_errorbuf_scope, which restores the previous buffer at the end of its scope.
     */
    _CWIPC_UTIL_EXPORT char **cwipc_log_exchange_errorbuf(char **errorbuf);
    /** Get current global log level
     * 
     */
    _CWIPC_UTIL_EXPORT cwipc_log_level cwipc_log_get_level();
};

/** Activates an error buffer for the rest of the enclosing scope (#384).
 * Use it at the top of every API function with a char **errorMessage argument, instead of
 * pairs of cwipc_log_set_errorbuf() calls: the buffer is deactivated on every return path,
 * and a nested call (an entry point calling another one) restores the outer buffer.
 * The active buffer is per thread.
 */
class cwipc_errorbuf_scope {
public:
    explicit cwipc_errorbuf_scope(char **errorbuf) : m_previous(cwipc_log_exchange_errorbuf(errorbuf)) {}
    ~cwipc_errorbuf_scope() { cwipc_log_exchange_errorbuf(m_previous); }
    cwipc_errorbuf_scope(const cwipc_errorbuf_scope&) = delete;
    cwipc_errorbuf_scope& operator=(const cwipc_errorbuf_scope&) = delete;
private:
    char **m_previous;
};

#endif // _cwipc_util_internal_logging_hpp_
