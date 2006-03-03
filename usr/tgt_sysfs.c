/*
 * SCSI file and directory handling functions
 *
 * Copyright (C) 2005 FUJITA Tomonori <tomof@acm.org>
 * Copyright (C) 2005 Mike Christie <michaelc@cs.wisc.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "tgt_sysfs.h"
#include "tgtd.h"

enum tgt_fs_op {
	CREATE = 0,
	DELETE,
};

static int tgt_set_string(char *buf, int size, const char *fmt, va_list ap)
{
	int err;

	err = vsnprintf(buf, size, fmt, ap);
	if (err > size)
		return -EINVAL;

	return 0;
}

static int tgt_dir(int op, const char *fmt, ...)
{
	int err;
	char path[PATH_MAX];
	mode_t dmode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

	va_list ap;
	va_start(ap, fmt);

	err = tgt_set_string(path, sizeof(path), fmt, ap);
	if (err)
		goto out;

	if (op)
		err = unlink(path);
	else
		err = mkdir(path, dmode);

	if (err < 0)
		eprintf("fail to create %s %s\n",
			op ? "delete" : "create", path);
out:
	va_end(ap);
	return err;
}

static int tgt_file(int op, const char *fmt, ...)
{
	int err;
	char path[PATH_MAX];
	mode_t fmode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;

	va_list ap;
	va_start(ap, fmt);

	err = tgt_set_string(path, sizeof(path), fmt, ap);
	if (err)
		goto out;

	if (op)
		err = unlink(path);
	else
		err = open(path, O_RDWR|O_CREAT|O_EXCL, fmode);

	if (err < 0)
		eprintf("fail to %s %s\n", op ? "delete" : "create", path);
out:
	va_end(ap);
	return err;
}

static int tgt_write(int fd, const char *fmt, ...)
{
	int err;
	char buf[PATH_MAX];
	va_list ap;

	va_start(ap, fmt);
	err = tgt_set_string(buf, sizeof(buf), fmt, ap);
	if (err)
		goto out;

	err = write(fd, buf, strlen(buf));
	if (err < 0)
		eprintf("fail to write %s\n", buf);
out:
	close(fd);
	va_end(ap);
	return err;
}

int tgt_target_dir_create(int tid)
{
	return tgt_dir(CREATE, TGT_TARGET_SYSFSDIR "/target%d", tid);
}

int tgt_target_dir_delete(int tid)
{
	return tgt_dir(DELETE, TGT_TARGET_SYSFSDIR "/target%d", tid);
}

int tgt_device_dir_create(int tid, uint64_t dev_id)
{
	return tgt_dir(CREATE, TGT_DEVICE_SYSFSDIR
		       "/device%d:%" PRIu64, tid, dev_id);
}

int tgt_device_dir_delete(int tid, uint64_t dev_id)
{
	return tgt_dir(DELETE, TGT_DEVICE_SYSFSDIR
		       "/device%d:%" PRIu64, tid, dev_id);
}
