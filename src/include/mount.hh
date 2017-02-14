#ifndef HUSH_MOUNT_HH
#define HUSH_MOUNT_HH

#include "utils/optparse.h"

typedef struct _cmdopts {
	_cmdopts() : foreground(false), debug(false), single_threaded(true) {};
	bool foreground;
	bool debug;
	bool single_threaded;
} cmdopts;

int hush_mount(struct optparse *);

#endif /* HUSH_MOUNT_HH */

