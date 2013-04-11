#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

typedef long time_t;
typedef long suseconds_t;

struct timeval {
	time_t tv_sec; /* seconds */
	suseconds_t tv_usec; /* microseconds */
};

struct timezone {
	int tz_minuteswest;     /* minutes west of Greenwich */
	int tz_dsttime;         /* type of DST correction */
};

#endif /* _SYS_TIME_H_ */
