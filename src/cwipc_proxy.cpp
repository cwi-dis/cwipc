#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/types.h>
#include <string.h>
#include <inttypes.h>

#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#define closesocket(x) close(x)
#endif

#ifdef _WIN32
#include <Windows.h>

inline void _cwipc_setThreadName(std::thread* thr, const wchar_t* name) {
    HANDLE threadHandle = static_cast<HANDLE>(thr->native_handle());
    SetThreadDescription(threadHandle, name);
}
#else
inline void _cwipc_setThreadName(std::thread* thr, const wchar_t* name) {}
#endif

class cwipc_source_proxy_impl : public cwipc_activesource {
private:
    int m_listen_socket;
    int m_socket;
    std::thread *m_server_thread;
    bool m_running;
    std::mutex m_pc_mutex;
    std::condition_variable m_pc_fresh;
    cwipc_pointcloud *m_pc;

public:
    cwipc_source_proxy_impl(int _socket)
    :   m_listen_socket(_socket),
        m_socket(-1),
        m_running(false),
        m_pc(NULL)
    {
    }

    ~cwipc_source_proxy_impl() {
        free();
    }

    virtual void free() override final {
        if (m_running) {
            stop();
        }

    }

    virtual bool start() override final {
        m_running = true;
        m_server_thread = new std::thread(&cwipc_source_proxy_impl::_server_main, this);
        _cwipc_setThreadName(m_server_thread, L"cwipc_proxy::server_thread");
        return true;
    }

    virtual void stop() override final {
        m_running = false;

        if (m_listen_socket >= 0) {
            closesocket(m_listen_socket);
        }

        m_listen_socket = -1;

        if (m_socket >= 0) {
            closesocket(m_socket);
        }

        m_socket = -1;

        if (m_server_thread) {
            m_server_thread->join();
        }

        m_server_thread = nullptr;    
    }

    virtual bool reload_config(const char* configFile) override final {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_proxy", "reload_config() not implemented");
        return false;
    }

    virtual bool reload_config_fast(const char* configFile) override final {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_proxy", "reload_config_fast() not implemented");
        return false;
    }

    virtual size_t get_config(char* buffer, size_t size) override final {
        return 0;
    }
    
    virtual bool seek(uint64_t timestamp) override final {
        return false;
    }

    virtual bool eof() override final {
        return m_listen_socket < 0 && m_socket < 0;
    }

    virtual bool available(bool wait) override final {
        std::unique_lock<std::mutex> mylock(m_pc_mutex);

        if (wait) {
            m_pc_fresh.wait(mylock, [this]{return m_pc != NULL || !m_running; });
        }

        return m_pc != NULL;
    }

    virtual cwipc_pointcloud* get() override final {
        std::unique_lock<std::mutex> mylock(m_pc_mutex);

        m_pc_fresh.wait(mylock, [this]{
            return m_pc != NULL || !m_running;
        });

        cwipc_pointcloud *rv = m_pc;
        m_pc = NULL;

        return rv;
    }

    virtual int maxtile() override final {
        return 1;
    }

    virtual bool get_tileinfo(int tilenum, struct cwipc_tileinfo *tileinfo) override final {
        static cwipc_tileinfo proxyInfo = {{0, 0, 0}, (char *)"proxy", 1, 0};

        switch(tilenum) {
        case 0:
            if (tileinfo) {
                *tileinfo = proxyInfo;
            }
            return true;
        }

        return false;
    }
private:
    void _server_main() {
        while(m_running) {
            //
            // accept on the socket, if not connected yet
            //
            if (m_socket < 0) {
                cwipc_log(CWIPC_LOG_LEVEL_TRACE, "cwipc_proxy", "waiting for connection...");
                m_socket = accept(m_listen_socket, NULL, 0);

                if (m_socket < 0) {
                    perror("cwipc_proxy: accept");
                    closesocket(m_listen_socket);
                    m_running = false;
                    break;
                }

                cwipc_log(CWIPC_LOG_LEVEL_TRACE, "cwipc_proxy", "connection accepted");
            }
            //
            // Get the header and check it
            //
            struct cwipc_point_packetheader header;
            if (!_recvall((void *)&header, sizeof(header))) {
                closesocket(m_socket);
                m_socket = -1;

                continue;
            }

            if (header.magic != CWIPC_POINT_PACKETHEADER_MAGIC) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", "invalid magic number in packet header");
                break;
            }

            //
            // Get the pointcloud data and convert it
            //
            cwipc_point* points = (cwipc_point *)malloc(header.dataCount);
            if (points == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", "malloc failed");
                break;
            }

            if (!_recvall((void *)points, header.dataCount)) {
                closesocket(m_socket);
                m_socket = -1;
                continue;
            }

            char* errorMessage = NULL;
            cwipc_pointcloud* pc = cwipc_from_points(points, header.dataCount, header.dataCount/sizeof(cwipc_point), header.timestamp, &errorMessage, CWIPC_API_VERSION);
            ::free(points);

            if (pc == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_proxy", std::string("cwipc_from_points: ") + errorMessage);
                break;
            }

            pc->_set_cellsize(header.cellsize);
            //
            // Forward the pointcloud to the consumer
            //
            std::unique_lock<std::mutex> mylock(m_pc_mutex);

            if (m_pc) {
                m_pc->free();
                m_pc = NULL;
            }

            m_pc = pc;
            m_pc_fresh.notify_all();

            //
            // Send acknowledgement (if connection still open)
            //
            if (send(m_socket, (const char *)&header.timestamp, sizeof(header.timestamp), 0) < 0) {
                if (m_socket >= 0) {
                    //
                    // Assume closed from remote. Close this connection socket and accept again.
                    //
                    perror("cwipc_proxy: send");
                    closesocket(m_socket);
                    m_socket = -1;
                    continue;
                }
            }
        }

        if (m_socket >= 0) {
            closesocket(m_socket);
            m_socket = -1;
        }

        std::unique_lock<std::mutex> mylock(m_pc_mutex);
        m_running = false;
        m_pc_fresh.notify_all();
    }

    bool _recvall(void *buffer, size_t size) {
        int status = recv(m_socket, (char *)buffer, size, MSG_WAITALL);
        if (status < 0) {
            perror("cwipc_proxy: recv");
        }
        return status == size;
    }
};

cwipc_activesource* cwipc_proxy(const char *host, int port, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_proxy: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    struct addrinfo hints;
    struct addrinfo *result = NULL;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char portbuf[32];
    snprintf(portbuf, 32, "%d", port);

    if (host != NULL && *host == '\0') {
        host = NULL;
    }

    int status = getaddrinfo(host, portbuf, &hints, &result);

    if (status != 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", std::string("getaddrinfo: ") + gai_strerror(status));
        cwipc_log_set_errorbuf(nullptr);
        return NULL;
    }

    int sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (sock < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", std::string("socket: ") + strerror(errno));
        cwipc_log_set_errorbuf(nullptr);

        return NULL;
    }

    status = bind(sock, result->ai_addr, result->ai_addrlen);

    if (status < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", std::string("bind: ") + strerror(errno));
        cwipc_log_set_errorbuf(nullptr);

        closesocket(sock);
        return NULL;
    }

    status = listen(sock, 1);

    if (status < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", std::string("listen: ") + strerror(errno));
        cwipc_log_set_errorbuf(nullptr);

        closesocket(sock);
        return NULL;
    }

    cwipc_activesource *rv = new cwipc_source_proxy_impl(sock);
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_proxy", "proxy allocation failed without error message");
    }
    cwipc_log_set_errorbuf(nullptr);
    return rv;
}
