#ifndef _XUTIL_H_
#define _XUTIL_H_

// Large file support
#define _FILE_OFFSET_BITS 64

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <cassert>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#include <process.h>

typedef int ssize_t;
typedef int socklen_t;

typedef void (*sighandler_t) (int);
#else
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <climits>
#endif

#include "xutil_macros.h"
#include "xfile.h"
#include "xlog.h"

namespace xutil {

const static int MaxLine = 4096;

typedef enum {
    SUCCESS = 0,
    ERR,
    ERR_INVALID_PARM,
    ERR_OUT_OF_RESOURCE,
    ERR_INTERNAL,
    ERR_LOGICAL,
    ERR_SYS,
    ERR_NOT_EXISTS,
    ERR_REMOTE,
    ERR_NOT_IMPLEMENTED,
    ERR_INPUT,
} status_t;

inline const char *strerror_(status_t st)
{
    static const char *info[] = {
        "success",
        "error occurred",
        "invalid param",
        "internal error",
        "logical error",
        "system error",
        "not exists",
        "remote error",
        "not implemented",
        "error input"
    };

    return info[st];
}

#define ERRNOMSG xutil::strerror_(errno)
static inline const char *strerror_(int err)
{
    int errno_save = errno;
    const char *retval = strerror(err);
    errno = errno_save;
    return retval;
}

/////////////////////////////////////////////////////////////

#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 8192
#endif

struct dirent {
    char d_name[PATH_MAX];
};

typedef struct DIR {
    HANDLE   handle;
    WIN32_FIND_DATAW info;
    struct dirent result;
} DIR;

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);

int gettimeofday(struct timeval *tv, void *tz);
#endif

std::string sprintf_(const char *fmt, ...);

bool is_valid_ip(const char *ip);

std::vector<std::string> split(const std::string str, const char *delim);

std::string hostname_to_ip(const char *hostname);

ssize_t readn(int fd, void *buf, size_t n);
ssize_t writen(int fd, const void *buf, size_t n);

char *skip_blank(char *p);

uint64_t get_time_now();

char *strcasechr(const char *s, int c);
bool end_with(const std::string &str, const std::string &sub);
bool start_with(const std::string &str, const std::string &sub);

int system_(const char *fmt, ...);

bool exec_get_int(const char *cmd, int *val);
bool exec_get_str(const char *cmd, char buff[], size_t len);

#ifndef _WIN32
std::string uuid();
#endif

std::string to_upper_str(const char *str);
std::string to_lower_str(const char *str);

std::string time_label();

byte *put_be16(byte *output, uint16_t val);
byte *put_be24(byte *output, uint32_t val);
byte *put_be32(byte *output, uint32_t val);
byte *put_be64(byte *output, uint64_t val);

const std::string dirname_(const std::string &path);
const std::string basename_(const std::string &path);

bool is_dir(const std::string &path);
bool is_file(const std::string &path);

unsigned char *base64_decode(const char *in, unsigned in_size,
        unsigned &result_size, bool trim_trailing_zeros = true);
char *base64_encode(char const *orig_signed, unsigned orig_length);
bool is_base64_encoded(const char *str);

int set_non_blocking(int fd);
int set_close_on_exec(int fd);

void sleep_(uint64_t ms);
void short_snap(uint64_t ms, volatile bool *quit, uint64_t granularity = 100);

bool str2bool(const char *str);

#define TAG_KIND_OF(tag,kind) ((bool)(((tag)&get_tag_mask((kind)))==(kind)))
uint64_t get_tag_mask(uint64_t tag);

#ifdef _WIN32
size_t strnlen(const char *str, size_t max);
char *strndup(const char *str, size_t max);
char *strtok_r(char *s, const char *delim, char **save_ptr);
#endif

char *strdup_(const char *s);

void rmdir_(const std::string &path);

/////////////////////////////////////////////////////////////

class Condition;

class Mutex {
    friend class Condition;
public:
    enum MutexType { NORMAL, RECURSIVE };

public:
    explicit Mutex(MutexType typ = NORMAL);
    ~Mutex();

    void lock() {
#ifdef _WIN32
        EnterCriticalSection(&m_cs);
#else
        pthread_mutex_lock(&m_mutex);
#endif
    }
    void unlock() {
#ifdef _WIN32
        LeaveCriticalSection(&m_cs);
#else
        pthread_mutex_unlock(&m_mutex);
#endif
    }

private:
    DISALLOW_COPY_AND_ASSIGN(Mutex);

#ifdef _WIN32
    CRITICAL_SECTION m_cs;
#else
    pthread_mutex_t m_mutex;
    pthread_mutexattr_t m_attr;
#endif
};

class RecursiveMutex : public Mutex {
public:
    RecursiveMutex() : Mutex(RECURSIVE) { }
};

class AutoLock {
public:
    explicit AutoLock(Mutex &l) : m_mutex(l) {
        m_mutex.lock();
    }
    ~AutoLock() {
        m_mutex.unlock();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(AutoLock);

    Mutex &m_mutex;
};

class Condition {
public:
    Condition(Mutex &m);
    ~Condition();

    int wait();
    int signal();
    int broadcast();

private:
#ifdef _WIN32
    HANDLE m_hdl;
    bool m_waited;
#else
    pthread_cond_t m_cond;
    pthread_condattr_t m_cond_attr;
#endif
    Mutex &m_mutex;
};

/////////////////////////////////////////////////////////////

#ifdef _WIN32
#define DECL_THREAD_ROUTINE(Class, func)  \
    class func##Thread : public xutil::Thread { \
    public: \
        func##Thread(Class *arg1, void *arg2, bool detach) \
            : xutil::Thread(detach), m_arg1(arg1), m_arg2(arg2) { \
                m_hdl = (HANDLE) _beginthreadex(NULL, 0, Thread::thrd_router, (void *) this, 0, NULL); \
                if (detach) { \
                    CloseHandle(m_hdl); \
                    m_hdl = INVALID_HANDLE_VALUE; \
                } \
            } \
            virtual ~func##Thread() { } \
            virtual void run() { m_arg1->func(m_arg2); } \
    private: \
        Class *m_arg1; \
        void *m_arg2; \
    }; \
    friend class func##Thread; \
    unsigned int func(void *arg);
#else
#define DECL_THREAD_ROUTINE(Class, func)  \
    class func##Thread : public xutil::Thread { \
    public: \
        func##Thread(Class *arg1, void *arg2, bool detach) \
            : xutil::Thread(detach), m_arg1(arg1), m_arg2(arg2) { \
                pthread_attr_t attr; \
                pthread_attr_init(&attr); \
                pthread_attr_setdetachstate(&attr, \
                        m_detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE); \
                pthread_create(&m_tid, &attr, (void *(*)(void *)) Thread::thrd_router, \
                        reinterpret_cast<void *>(this)); \
                pthread_attr_destroy(&attr); \
            } \
        virtual ~func##Thread() { } \
        virtual void run() { m_arg1->func(m_arg2); } \
    private: \
        Class *m_arg1; \
        void *m_arg2; \
    }; \
    friend class func##Thread; \
    unsigned int func(void *arg);
#endif

#define CREATE_THREAD_ROUTINE(func, arg, detach) \
    new func##Thread(this, reinterpret_cast<void *>(arg), detach)

#define DELETE_THREAD(thrd) \
    SAFE_DELETE(thrd);
#define JOIN_DELETE_THREAD(thrd) \
    if (thrd) { \
        thrd->join(); \
        DELETE_THREAD(thrd); \
    }

class Thread {
public:
    explicit Thread(bool detach = false);
    virtual ~Thread() { }

    status_t join();
    bool is_detach() const { return m_detach; }
    bool is_alive() const;

    virtual void run() = 0;

#ifdef _WIN32
    static bool is_alive(HANDLE hdl);
#else
    static bool is_alive(pthread_t tid);
#endif

protected:
#ifdef _WIN32
    static unsigned int __stdcall thrd_router(void *);
#else
    static unsigned int thrd_router(void *);
#endif

#ifdef _WIN32
    HANDLE m_hdl;
#else
    pthread_t m_tid;
#endif

    bool m_detach;
};

/////////////////////////////////////////////////////////////

class MemHolder {
public:
    MemHolder();
    ~MemHolder();

    void *alloc(uint32_t sz);
    void *calloc(uint32_t sz);
    void *get_buffer() const { return m_buf; }
    void destroy();

private:
    uint32_t m_capacity;
    void *m_buf;
};

/////////////////////////////////////////////////////////////

#define GETAVAILABLEBYTESCOUNT(x) ((x).published - (x).consumed)
#define GETIBPOINTER(x) ((uint8_t *)((x).buffer + (x).consumed))

class IOBuffer {
public:
    uint8_t *buffer;
    uint32_t size;
    uint32_t published;
    uint32_t consumed;
    uint32_t min_chunk_size;

public:
    IOBuffer();
    virtual ~IOBuffer();

    void initialize(uint32_t expected);
    bool ensure_size(uint32_t expected);
    bool move_data();
    bool read_from_buffer(const uint8_t *buffer_, const uint32_t size_);
    void read_from_input_buffer(IOBuffer *input_buffer, uint32_t start, uint32_t size_);
    bool read_from_input_buffer(const IOBuffer &buffer_, uint32_t size_);
    bool read_from_string(std::string binary);
    bool read_from_string(const char *fmt, ...);
    void read_from_byte(uint8_t byte);
    void read_from_repeat(uint8_t byte, uint32_t size_);
    void read_from_file(const std::string &path, const char *mode = "r");
    bool write_to_stdio(int fd, uint32_t size_, int &sent_amount);
    uint32_t get_min_chunk_size();
    void set_min_chunk_size(uint32_t min_chunk_size_);
    uint32_t get_current_write_position();
    uint8_t *get_pointer();
    bool ignore(uint32_t size_);
    bool ignore_all();
    void recycle();
    static std::string dump_buffer(const uint8_t *buffer_, uint32_t length);
    std::string to_string(uint32_t start_index = 0, uint32_t limit = 0);
    operator std::string();

private:
    void cleanup();

private:
    DISALLOW_COPY_AND_ASSIGN(IOBuffer);
};

}

#endif /* end of _XUTIL_H_ */
