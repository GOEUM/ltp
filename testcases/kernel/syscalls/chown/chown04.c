/*
 *
 *   Copyright (c) International Business Machines  Corp., 2001
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Test Name: chown04
 *
 * Test Description:
 *   Verify that,
 *   1) chown(2) returns -1 and sets errno to EPERM if the effective user id
 *		 of process does not match the owner of the file and the process
 *		 is not super user.
 *   2) chown(2) returns -1 and sets errno to EACCES if search permission is
 *		 denied on a component of the path prefix.
 *   3) chown(2) returns -1 and sets errno to EFAULT if pathname points
 *		 outside user's accessible address space.
 *   4) chown(2) returns -1 and sets errno to ENAMETOOLONG if the pathname
 *		 component is too long.
 *   5) chown(2) returns -1 and sets errno to ENOTDIR if the directory
 *		 component in pathname is not a directory.
 *   6) chown(2) returns -1 and sets errno to ENOENT if the specified file
 *		 does not exists.
 *
 * Expected Result:
 *  chown() should fail with return value -1 and set expected errno.
 *
 * Algorithm:
 *  Setup:
 *   Setup signal handling.
 *   Create temporary directory.
 *   Pause for SIGUSR1 if option specified.
 *
 *  Test:
 *   Loop if the proper options are given.
 *   Execute system call
 *   Check return code, if system call failed (return=-1)
 *	if errno set == expected errno
 *		Issue sys call fails with expected return value and errno.
 *	Otherwise,
 *		Issue sys call fails with unexpected errno.
 *   Otherwise,
 *		Issue sys call returns unexpected value.
 *
 *  Cleanup:
 *   Print errno log and/or timing stats if options given
 *   Delete the temporary directory(s)/file(s) created.
 *
 * Usage:  <for command-line>
 *  chown04 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -e   : Turn on errno logging.
 *		        -i n : Execute test n times.
 *		        -I x : Execute test for x seconds.
 *		        -P x : Pause for x seconds between iterations.
 *		        -t   : Turn on syscall timing.
 *
 * HISTORY
 *		 07/2001 Ported by Wayne Boyer
 *
 * RESTRICTIONS:
 *  This test should be executed by 'non-super-user' only.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include "test.h"
#include "usctest.h"
#include "safe_macros.h"

#define MODE_RWX		 (S_IRWXU|S_IRWXG|S_IRWXO)
#define FILE_MODE		 (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define DIR_MODE		 (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP| \
				 S_IXGRP|S_IROTH|S_IXOTH)
#define DIR_TEMP		 "testdir_1"
#define TEST_FILE1		 "tfile_1"
#define TEST_FILE2		 (DIR_TEMP "/tfile_2")
#define TEST_FILE3		 "t_file/tfile_3"
#define TEST_FILE4		 "test_eloop1"
#define TEST_FILE5		 "mntpoint"

void setup1(void);
void setup2(void);
void setup3(void);
void longpath_setup(void);
static void help(void);

char Longpathname[PATH_MAX + 2];
char high_address_node[64];
static char *fstype = "ext2";
static char *device;
static int dflag;
static int mount_flag;

static option_t options[] = {
	{"T:", NULL, &fstype},
	{"D:", &dflag, &device},
	{NULL, NULL, NULL}
};

struct test_case_t {
	char *pathname;
	int exp_errno;
	void (*setupfunc) (void);
} test_cases[] = {
	{
	TEST_FILE1, EPERM, setup1}, {
	TEST_FILE2, EACCES, setup2}, {
	high_address_node, EFAULT, NULL}, {
	(char *)-1, EFAULT, NULL}, {
	Longpathname, ENAMETOOLONG, longpath_setup}, {
	"", ENOENT, NULL}, {
	TEST_FILE3, ENOTDIR, setup3}, {
	TEST_FILE4, ELOOP, NULL}, {
	TEST_FILE5, EROFS, NULL},};

char *TCID = "chown04";
int TST_TOTAL = sizeof(test_cases) / sizeof(*test_cases);
int exp_enos[] = { EPERM, EACCES, EFAULT, ENAMETOOLONG, ENOENT, ENOTDIR,
		   ELOOP, EROFS, 0 };

struct passwd *ltpuser;

char *bad_addr = 0;

void setup(void);
void cleanup(void);

int main(int ac, char **av)
{
	int lc;
	char *msg;
	char *file_name;
	int i;
	uid_t user_id;
	gid_t group_id;

	msg = parse_opts(ac, av, options, help);
	if (msg != NULL)
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);
	if (!dflag) {
		tst_brkm(TBROK, NULL, "you must specify the device used for "
			 "mounting with -D option");
	}

	setup();

	TEST_EXP_ENOS(exp_enos);

	user_id = geteuid();
	group_id = getegid();

	for (lc = 0; TEST_LOOPING(lc); lc++) {

		tst_count = 0;

		for (i = 0; i < TST_TOTAL; i++) {
			file_name = test_cases[i].pathname;

			if (file_name == high_address_node)
				file_name = get_high_address();

			TEST(chown(file_name, user_id, group_id));

			if (TEST_RETURN == 0) {
				tst_resm(TFAIL, "chown succeeded unexpectedly");
				continue;
			} else if (TEST_ERRNO == test_cases[i].exp_errno)
				tst_resm(TPASS | TTERRNO, "chown failed");
			else {
				tst_resm(TFAIL | TTERRNO,
					 "chown failed; expected: %d - %s",
					 test_cases[i].exp_errno,
					 strerror(test_cases[i].exp_errno));
			}
		}
	}

	cleanup();
	tst_exit();

}

void setup(void)
{
	int i;
	int fd;

	tst_require_root(NULL);

	tst_mkfs(NULL, device, fstype, NULL);

	tst_sig(FORK, DEF_HANDLER, cleanup);

	ltpuser = getpwnam("nobody");
	if (ltpuser == NULL)
		tst_brkm(TBROK | TERRNO, NULL, "getpwnam(\"nobody\") failed");
	if (seteuid(ltpuser->pw_uid) == -1)
		tst_brkm(TBROK | TERRNO, NULL, "seteuid(%d) failed",
			 ltpuser->pw_uid);

	TEST_PAUSE;

	tst_tmpdir();

	bad_addr = mmap(0, 1, PROT_NONE,
			MAP_PRIVATE_EXCEPT_UCLINUX | MAP_ANONYMOUS, 0, 0);
	if (bad_addr == MAP_FAILED)
		tst_brkm(TBROK | TERRNO, cleanup, "mmap failed");

	test_cases[3].pathname = bad_addr;

	SAFE_SYMLINK(cleanup, "test_eloop1", "test_eloop2");
	SAFE_SYMLINK(cleanup, "test_eloop2", "test_eloop1");

	SAFE_SETEUID(cleanup, 0);
	SAFE_MKDIR(cleanup, "mntpoint", DIR_MODE);
	if (mount(device, "mntpoint", fstype, MS_RDONLY, NULL) < 0) {
		tst_brkm(TBROK | TERRNO, cleanup,
			 "mount device:%s failed", device);
	}
	mount_flag = 1;

	SAFE_SETEUID(cleanup, ltpuser->pw_uid);

	for (i = 0; i < TST_TOTAL; i++)
		if (test_cases[i].setupfunc != NULL)
			test_cases[i].setupfunc();
}

void setup1(void)
{
	int fd;
	uid_t old_uid;

	old_uid = geteuid();

	if ((fd = open(TEST_FILE1, O_RDWR | O_CREAT, 0666)) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "opening \"%s\" failed",
			 TEST_FILE1);

	if (seteuid(0) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "seteuid(0) failed");

	if (fchown(fd, 0, 0) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "fchown failed");

	if (close(fd) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "closing \"%s\" failed",
			 TEST_FILE1);

	if (seteuid(old_uid) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "seteuid(%d) failed",
			 old_uid);

}

void setup2(void)
{
	int fd;
	uid_t old_uid;

	old_uid = geteuid();

	if (seteuid(0) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "seteuid(0) failed");

	if (mkdir(DIR_TEMP, S_IRWXU) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "mkdir failed");

	if ((fd = open(TEST_FILE2, O_RDWR | O_CREAT, 0666)) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "fchown failed");

	if (close(fd) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "closing \"%s\" failed",
			 TEST_FILE2);

	if (seteuid(old_uid) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "seteuid(%d) failed",
			 old_uid);

}

void setup3(void)
{
	int fd;

	if ((fd = open("t_file", O_RDWR | O_CREAT, MODE_RWX)) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "opening \"t_file\" failed");
	if (close(fd) == -1)
		tst_brkm(TBROK | TERRNO, cleanup, "closing \"t_file\" failed");
}

void longpath_setup(void)
{
	int i;

	for (i = 0; i <= (PATH_MAX + 1); i++)
		Longpathname[i] = 'a';
}

void cleanup(void)
{
	TEST_CLEANUP;

	if (seteuid(0) == -1)
		tst_resm(TWARN | TERRNO, "seteuid(0) failed");
	if (mount_flag && umount("mntpoint") < 0) {
		tst_brkm(TBROK | TERRNO, NULL,
			 "umount device:%s failed", device);
	}

	tst_rmdir();

}

static void help(void)
{
	printf("-T type   : specifies the type of filesystem to be mounted. "
	       "Default ext2.\n");
	printf("-D device : device used for mounting.\n");
}
