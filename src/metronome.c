#define IOFUNC_ATTR_T struct io_attr_t
#define IOFUNC_OCB_T struct metro_ocb_t

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <math.h>
#include <sys/types.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>

#define METRO_PULSE_CODE _PULSE_CODE_MINAVAIL
#define PAUSE_PULSE_CODE (METRO_PULSE_CODE + 1)
#define START_PULSE_CODE (PAUSE_PULSE_CODE + 1)
#define STOP_PULSE_CODE  (START_PULSE_CODE + 1)
#define QUIT_PULSE_CODE  (STOP_PULSE_CODE  + 1)
#define SET_PULSE_CODE   (QUIT_PULSE_CODE  + 1)
#define METRO_ATTACH     "metronome"

typedef union {
	struct _pulse pulse;
	char msg[255];
} message_u;

struct DataTableRow {
	int time_sig_top;
	int time_sig_btm;
	int num_intervals;
	char pattern[32];
};

struct DataTableRow time_patterns[] = {
	{ 2,  4,  4,   "|1&2&"         },
	{ 3,  4,  6,   "|1&2&3&"       },
	{ 4,  4,  8,   "|1&2&3&4&"     },
	{ 5,  4,  10,  "|1&2&3&4-5-"   },
	{ 3,  8,  6,   "|1-2-3-"       },
	{ 6,  8,  6,   "|1&a2&a"       },
	{ 9,  8,  9,   "|1&a2&a3&a"    },
	{ 12, 8,  12,  "|1&a2&a3&a4&a" }
};

typedef struct {
	int beats_per_min;
	int time_sig_top;
	int time_sig_btm;
} metro_attr_t;

typedef struct {
	int status;
	double beats_per_sec;
	double measure;
	double interval;
	double nano_sec;
} timer_attr_t;

typedef struct {
	metro_attr_t metro_attr;
	timer_attr_t timer_attr;
} pulse_attr_t;

typedef struct io_attr_t {
	iofunc_attr_t attr;
	int device_id;
} io_attr_t;

typedef struct metro_ocb_t {
	iofunc_ocb_t ocb;
	char buffer[50];
} metro_ocb_t;

name_attach_t *global_attach;
pulse_attr_t pulse_data;
int client_coid;
char response_data[255];

int io_read(resmgr_context_t *ctp, io_read_t *msg, metro_ocb_t *ocb) {
	int sig_index = 0;
	int num_bytes;

	for (int i = 0; i < 8; i++) {
		if (time_patterns[i].time_sig_btm == pulse_data.metro_attr.time_sig_btm && time_patterns[i].time_sig_top == pulse_data.metro_attr.time_sig_top) {
			sig_index = i;
			break;
		}
	}

	sprintf(response_data,
			"[metronome: %d bpm, time signature: %d/%d, interval: %.2fs, nanoseconds: %.0lf]\n",
			pulse_data.metro_attr.beats_per_min,
			time_patterns[sig_index].time_sig_top,
			time_patterns[sig_index].time_sig_btm,
			pulse_data.timer_attr.interval,
			pulse_data.timer_attr.nano_sec);

	num_bytes = strlen(response_data);
	if (ocb->ocb.offset == num_bytes) return 0;

	num_bytes = min(num_bytes, msg->i.nbytes);
	_IO_SET_READ_NBYTES(ctp, num_bytes);
	SETIOV(ctp->iov, response_data, num_bytes);
	ocb->ocb.offset += num_bytes;

	if (num_bytes > 0) ocb->ocb.flags |= IOFUNC_ATTR_ATIME;
	return _RESMGR_NPARTS(1);
}

int io_write(resmgr_context_t *ctp, io_write_t *msg, metro_ocb_t *ocb) {
	int num_bytes = 0;

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		char *input_buf = (char *)(msg + 1);
		char *cmd_arg;
		int val = 0;

		if (strstr(input_buf, "pause") != NULL) {
			strsep(&input_buf, " ");
			cmd_arg = strsep(&input_buf, " ");
			val = atoi(cmd_arg);
			if (val >= 1 && val <= 9)
				MsgSendPulse(client_coid, SchedGet(0, 0, NULL), PAUSE_PULSE_CODE, val);
			else printf("\nERROR: Integer is not between 1 and 9\n");
		} else if (strstr(input_buf, "quit") != NULL) {
			MsgSendPulse(client_coid, SchedGet(0, 0, NULL), QUIT_PULSE_CODE, val);
		} else if (strstr(input_buf, "start") != NULL) {
			MsgSendPulse(client_coid, SchedGet(0, 0, NULL), START_PULSE_CODE, val);
		} else if (strstr(input_buf, "stop") != NULL) {
			MsgSendPulse(client_coid, SchedGet(0, 0, NULL), STOP_PULSE_CODE, val);
		} else if (strstr(input_buf, "set") != NULL) {
			strsep(&input_buf, " ");
			pulse_data.metro_attr.beats_per_min = atoi(strsep(&input_buf, " "));
			pulse_data.metro_attr.time_sig_top = atoi(strsep(&input_buf, " "));
			pulse_data.metro_attr.time_sig_btm = atoi(strsep(&input_buf, " "));
			MsgSendPulse(client_coid, SchedGet(0, 0, NULL), SET_PULSE_CODE, val);
		} else {
			printf("\nInvalid Command\n");
		}
		num_bytes = msg->i.nbytes;
	}

	_IO_SET_WRITE_NBYTES(ctp, num_bytes);
	if (msg->i.nbytes > 0) ocb->ocb.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
	return _RESMGR_NPARTS(0);
}

int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle, void *extra) {
	if ((client_coid = name_open(METRO_ATTACH, 0)) == -1) {
		perror("ERROR: name_open() failed");
		return EXIT_FAILURE;
	}
	return iofunc_open_default(ctp, msg, &handle->attr, extra);
}

void *metronome_loop(void *arg) {
	struct sigevent event;
	struct itimerspec interval_time;
	message_u recv_msg;
	timer_t timer_id;
	int sig_index = 0;
	int rcvid;
	char *pattern_ptr;

	if ((global_attach = name_attach(NULL, METRO_ATTACH, 0)) == NULL) {
		printf("ERROR: name_attach() failed\n");
		exit(EXIT_FAILURE);
	}

	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, global_attach->chid, _NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = SIGEV_PULSE_PRIO_INHERIT;
	event.sigev_code = METRO_PULSE_CODE;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

	for (int i = 0; i < 8; i++) {
		if (time_patterns[i].time_sig_btm == pulse_data.metro_attr.time_sig_btm && time_patterns[i].time_sig_top == pulse_data.metro_attr.time_sig_top)
			sig_index = i;
	}

	pulse_data.timer_attr.beats_per_sec = 60.0 / pulse_data.metro_attr.beats_per_min;
	pulse_data.timer_attr.measure = pulse_data.timer_attr.beats_per_sec * 2;
	pulse_data.timer_attr.interval = pulse_data.timer_attr.measure / pulse_data.metro_attr.time_sig_btm;
	pulse_data.timer_attr.nano_sec = (pulse_data.timer_attr.interval - (int)pulse_data.timer_attr.interval) * 1e9;

	interval_time.it_value.tv_sec = 1;
	interval_time.it_value.tv_nsec = 0;
	interval_time.it_interval.tv_sec = pulse_data.timer_attr.interval;
	interval_time.it_interval.tv_nsec = pulse_data.timer_attr.nano_sec;
	timer_settime(timer_id, 0, &interval_time, NULL);
	pattern_ptr = time_patterns[sig_index].pattern;

	while (1) {
		rcvid = MsgReceive(global_attach->chid, &recv_msg, sizeof(recv_msg), NULL);
		if (rcvid == 0) {
			switch (recv_msg.pulse.code) {
				case METRO_PULSE_CODE:
					if (*pattern_ptr == '|') {
						printf("%.2s", pattern_ptr);
						pattern_ptr += 2;
					} else if (*pattern_ptr == '\0') {
						printf("\n");
						pattern_ptr = time_patterns[sig_index].pattern;
					} else printf("%c", *pattern_ptr++);
					break;
				case PAUSE_PULSE_CODE:
					if (pulse_data.timer_attr.status == 0) {
						interval_time.it_value.tv_sec = recv_msg.pulse.value.sival_int;
						timer_settime(timer_id, 0, &interval_time, NULL);
					}
					break;
				case QUIT_PULSE_CODE:
					timer_delete(timer_id);
					name_detach(global_attach, 0);
					name_close(client_coid);
					exit(EXIT_SUCCESS);
				case SET_PULSE_CODE:
					for (int i = 0; i < 8; i++) {
						if (time_patterns[i].time_sig_btm == pulse_data.metro_attr.time_sig_btm && time_patterns[i].time_sig_top == pulse_data.metro_attr.time_sig_top)
							sig_index = i;
					}
					pattern_ptr = time_patterns[sig_index].pattern;
					pulse_data.timer_attr.beats_per_sec = 60.0 / pulse_data.metro_attr.beats_per_min;
					pulse_data.timer_attr.measure = pulse_data.timer_attr.beats_per_sec * 2;
					pulse_data.timer_attr.interval = pulse_data.timer_attr.measure / pulse_data.metro_attr.time_sig_btm;
					pulse_data.timer_attr.nano_sec = (pulse_data.timer_attr.interval - (int)pulse_data.timer_attr.interval) * 1e9;
					interval_time.it_value.tv_sec = 1;
					interval_time.it_value.tv_nsec = 0;
					interval_time.it_interval.tv_sec = pulse_data.timer_attr.interval;
					interval_time.it_interval.tv_nsec = pulse_data.timer_attr.nano_sec;
					timer_settime(timer_id, 0, &interval_time, NULL);
					printf("\n");
					break;
				case START_PULSE_CODE:
					if (pulse_data.timer_attr.status == 1) {
						interval_time.it_value.tv_sec = 1;
						interval_time.it_value.tv_nsec = 0;
						interval_time.it_interval.tv_sec = pulse_data.timer_attr.interval;
						interval_time.it_interval.tv_nsec = pulse_data.timer_attr.nano_sec;
						pulse_data.timer_attr.status = 0;
						timer_settime(timer_id, 0, &interval_time, NULL);
					}
					break;
				case STOP_PULSE_CODE:
					if (pulse_data.timer_attr.status == 0) {
						interval_time.it_value.tv_sec = 0;
						pulse_data.timer_attr.status = 1;
						timer_settime(timer_id, 0, &interval_time, NULL);
					}
					break;
			}
			fflush(stdout);
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	dispatch_t *dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	dispatch_context_t *ctp;
	io_attr_t io_attr;
	pthread_attr_t attr;

	if (argc != 4) {
		printf("Usage: %s <bpm> <top> <btm>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	pulse_data.metro_attr.beats_per_min = atoi(argv[1]);
	pulse_data.metro_attr.time_sig_top = atoi(argv[2]);
	pulse_data.metro_attr.time_sig_btm = atoi(argv[3]);

	dpp = dispatch_create();
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS, &io_funcs);
	connect_funcs.open = io_open;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	iofunc_attr_init(&io_attr, S_IFCHR | 0666, NULL, NULL);
	resmgr_attach(dpp, NULL, "/dev/local/metronome", _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &io_attr);

	ctp = dispatch_context_alloc(dpp);
	pthread_attr_init(&attr);
	pthread_create(NULL, &attr, &metronome_loop, &pulse_data);

	while (1) {
		if ((ctp = dispatch_block(ctp)))
			dispatch_handler(ctp);
	}

	return EXIT_SUCCESS;
}
