#ifndef _XUTIL_MACROS_H_
#define _XUTIL_MACROS_H_

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) do { \
    delete (p);             \
    (p) = NULL;             \
} while (0)
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) do { \
    delete [] (p);                \
    (p) = NULL;                   \
} while (0)
#define SAFE_DELETEA SAFE_DELETE_ARRAY
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p) do { \
    free(p);              \
    (p) = NULL;           \
} while (0)
#endif

#ifndef SAFE_CLOSE
#define SAFE_CLOSE(fd) do { \
    if (fd >= 0) {          \
        ::close(fd);        \
        fd = -1;            \
    }                       \
} while (0)
#endif
#ifndef SAFE_CLOSE_SOCKET
#ifdef _WIN32
#define SAFE_CLOSE_SOCKET(sockfd) do { \
    if (sockfd > 0) {                  \
        closesocket(sockfd);           \
        sockfd = -1;                   \
    }                                  \
} while (0)
#else
#define SAFE_CLOSE_SOCKET SAFE_CLOSE
#endif
#endif

#define CHECK_EXPR_EXEC(expr, cmd) do { \
    if (expr) { cmd; }                  \
} while (0)
#define CHECK_EXPR_EXEC_RET(expr, cmd) do { \
    if (expr) { cmd; return; }                 \
} while (0);
#define CHECK_EXPR_EXEC_RETVAL(expr, cmd, retval) do { \
    if (expr) { cmd; return retval; }                      \
} while (0);

#define CHECK_EXPR_EXEC_GOTO(expr, cmd, label) do { \
    if (expr) { cmd; goto label; }                  \
} while (0)

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &);            \
    void operator=(const TypeName &)

#include <sys/stat.h>

#ifndef _WIN32
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

#define MIN(x, y) ({ \
    typeof(x) _min1 = (x); \
    typeof(y) _min2 = (y); \
    (void) (&_min1 == &_min2); \
    _min1 < _min2 ? _min1 : _min2; })
#define MAX(x, y) ({ \
    typeof(x) _min1 = (x); \
    typeof(y) _min2 = (y); \
    (void) (&_min1 == &_min2); \
    _min1 > _min2 ? _min1 : _min2; })

#define foreach(container,it) \
    for (typeof((container).begin()) it = (container).begin(); \
    it != (container).end(); \
    ++it)

#ifndef DIRSEP
#define DIRSEP '/'

#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
    ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
     ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif

#define ABS_PATH(rel, abs, abs_size) realpath((rel), (abs))
#else
#define gettid() GetCurrentThreadId()

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define snprintf sprintf_s

#define __S_ISTYPE(mode, mask) (((mode)&_S_IFMT) == (mask))
#define S_ISDIR(mode) __S_ISTYPE((mode), _S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), _S_IFREG)

#include <io.h>
#include <direct.h>

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#ifndef IN_MULTICAST
#define IN_MULTICAST(a) ((((uint32_t)(a)) & 0xf0000000) == 0xe0000000)
#endif

#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define strdup(s) _strdup(s)
#define getpid() _getpid()
#define mkdir(x, y) _mkdir(x)
#define rmdir(x) _rmdir(x)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#define chdir(x) _chdir(x)
#define getcwd(x, y) _getcwd((x), (y))
#define unlink(x) _unlink(x)

#ifndef DIRSEP
#define DIRSEP '\\'
#endif

#define ABS_PATH(rel, abs, abs_size) _fullpath((abs), (rel), (abs_size))

#if defined(_MSC_VER)
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif
#endif

#define NELEM(arr)  (sizeof(arr)/sizeof(arr[0]))

#define BEGIN   {
#define END     }

#define FOR_VECTOR_ITERATOR(e,v,i) for(vector<e>::iterator i=(v).begin();i!=(v).end();i++)
#define FOR_VECTOR_CONST_ITERATOR(e,v,i) for(vector<e>::const_iterator i=(v).begin();i!=(v).end();i++)
#define FOR_MAP(m,k,v,i) for(map<k , v>::iterator i=(m).begin();i!=(m).end();i++)
#define FOR_MAP_CONST(m,k,v,i) for(map<k , v>::const_iterator i=(m).begin();i!=(m).end();i++)
#define MAP_KEY(i) ((i)->first)
#define MAP_VAL(i) ((i)->second)
#define MAP_HAS(m,k) ((bool)((m).find((k))!=(m).end()))
#define MAP_ERASE(m,k) if(MAP_HAS((m),(k))) (m).erase((k));
#define MAP_ERASE2(m,k1,k2)         \
    if(MAP_HAS((m),(k1))){          \
        MAP_ERASE((m)[(k1)],(k2));  \
        if((m)[(k1)].size()==0)     \
        MAP_ERASE((m),(k1));        \
    }
#define FOR_SET(e,s,i) for(set<e>::iterator i=(s).begin();i!=(s).end();i++)

#define FATAL(fmt, ...) do {                \
    fprintf(stderr, fmt, ##__VA_ARGS__);    \
    fprintf(stderr, "\n");                  \
} while (0)

#define STR(x) (((std::string)(x)).c_str())

#define UNUSED(x)   ((void) (x))

#include "xtype.h"

#define REVERSE_BYTES(bytes_arr, n)   do { \
    for (uint32_t idx = 0; idx < n/2; ++idx) { \
        byte tmp = *bytes_arr[idx]; \
        *bytes_arr[idx] = *bytes_arr[n - idx - 1]; \
        *bytes_arr[n - idx - 1] = tmp; \
    } \
} while (0)

#define REVERSE_BYTE(b) do { \
    byte x = 0; \
    for (uint8_t idx = 0; idx < 8; ++idx) { \
    x |= ((b & 0x01) << (8 - idx - 1)); \
    b >>= 1; \
    } \
    b = x; \
} while (0)

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

// 16 bit
#define EHTONS(x) htons(x)
#define ENTOHS(x) ntohs(x)

// 24 bit
#define ENDIAN_CHANGE_UI24(x)   REVERSE_BYTES((byte *) &(x), 3)
#define VALUI24(x)              \
    ((uint32_t)((x)[2]<<16) + (uint32_t)((x)[1]<<8) + (uint32_t)(x)[0])

#define ENTOH24(x)              (((x)[0] << 16) + ((x)[1] << 8) + (x)[2])
#define INITUI24(x, val) do {   \
    x[2] = (val&0xFF0000)>>16;  \
    x[1] = (val&0xFF00)>>8;     \
    x[0] = val&0xFF;            \
} while (0)

// 32 bit
#define EHTONL(x) htonl(x)
#define ENTOHL(x) ntohl(x)

// 64 bit
#ifndef DONT_DEFINE_HTONLL
#define htonll(x) \
    ((uint64_t)( \
    ((((uint64_t)(x)) & 0xff00000000000000LL) >> 56) | \
    ((((uint64_t)(x)) & 0x00ff000000000000LL) >> 40) | \
    ((((uint64_t)(x)) & 0x0000ff0000000000LL) >> 24) | \
    ((((uint64_t)(x)) & 0x000000ff00000000LL) >> 8)  | \
    ((((uint64_t)(x)) & 0x00000000ff000000LL) << 8)  | \
    ((((uint64_t)(x)) & 0x0000000000ff0000LL) << 24) | \
    ((((uint64_t)(x)) & 0x000000000000ff00LL) << 40) | \
    ((((uint64_t)(x)) & 0x00000000000000ffLL) << 56) \
    ))
#define ntohll(x)   htonll(x)
#endif
#define EHTONLL(x) htonll(x)
#define ENTOHLL(x) ntohll(x)

#define MAKE_TAG8(a,b,c,d,e,f,g,h) ((uint64_t)(((uint64_t)(a))<<56)|(((uint64_t)(b))<<48)|(((uint64_t)(c))<<40)|(((uint64_t)(d))<<32)|(((uint64_t)(e))<<24)|(((uint64_t)(f))<<16)|(((uint64_t)(g))<<8)|((uint64_t)(h)))
#define MAKE_TAG7(a,b,c,d,e,f,g) MAKE_TAG8(a,b,c,d,e,f,g,0)
#define MAKE_TAG6(a,b,c,d,e,f) MAKE_TAG7(a,b,c,d,e,f,0)
#define MAKE_TAG5(a,b,c,d,e) MAKE_TAG6(a,b,c,d,e,0)
#define MAKE_TAG4(a,b,c,d) MAKE_TAG5(a,b,c,d,0)
#define MAKE_TAG3(a,b,c) MAKE_TAG4(a,b,c,0)
#define MAKE_TAG2(a,b) MAKE_TAG3(a,b,0)
#define MAKE_TAG1(a) MAKE_TAG2(a,0)

#endif /* end of _XUTIL_MACROS_H_ */
