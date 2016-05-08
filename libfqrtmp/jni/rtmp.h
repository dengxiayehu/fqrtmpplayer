#ifndef _RTMP_H_
#define _RTMP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtmp_t rtmp_t;

rtmp_t *rtmp_init(const char *str);
int rtmp_connect(rtmp_t *r);
int rtmp_disconnect(rtmp_t *r);
void rtmp_destroy(rtmp_t **r);

#ifdef __cplusplus
}
#endif
#endif /* end of _RTMP_H_ */
