/* 
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include "iscsi.h"
#include "scsi-lowlevel.h"
#include "iscsi-test.h"

int T0101_read10_beyond_eol(const char *initiator, const char *url)
{ 
	struct iscsi_context *iscsi;
	struct scsi_task *task;
	int ret, i, lun;

	printf("0101_read10_beyond_eol:\n");
	printf("=======================\n");
	if (show_info) {
		printf("Test that READ10 fails when reading beyond end-of-lun.\n");
		printf("This test is skipped for LUNs with more than 2^31 blocks\n");
		printf("1, Read 1-256 blocks one block beyond end-of-lun.\n");
		printf("2, Read 1-256 blocks at LBA 2^31\n");
		printf("3, Read 1-256 blocks at LBA -1\n");
		printf("4, Read 2-256 blocks all but one beyond end-of-lun.\n");
		printf("\n");
		return 0;
	}

	iscsi = iscsi_context_login(initiator, url, &lun);
	if (iscsi == NULL) {
		printf("Failed to login to target\n");
		return -1;
	}

	ret = 0;

	if (num_blocks >= 0x80000000) {
		printf("[SKIPPED]\n");
		printf("LUN is too big for read-beyond-eol tests with READ10. Skipping test.\n");
		ret = -2;
		goto finished;
	}

	/* read 1-256 blocks, one block beyond the end-of-lun */
	printf("Reading last 1-256 blocks one block beyond eol ... ");
	for (i=1; i<=256; i++) {
		task = iscsi_read10_sync(iscsi, lun, num_blocks + 2 - i, i * block_size, block_size, 0, 0, 0, 0, 0);
		if (task == NULL) {
			printf("[FAILED]\n");
			printf("Failed to send READ10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
			printf("[FAILED]\n");
			printf("READ10 beyond end-of-lun did not fail with sense.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->status        != SCSI_STATUS_CHECK_CONDITION
		    || task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		    || task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		        printf("[FAILED]\n");
			printf("READ10 failed but ascq was wrong. Should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


	/* Reading 1 - 256 blocks at LBA 2^31 */
	printf("Reaing 1-256 blocks at LBA 2^31 ... ");
	for (i = 1; i <= 256; i++) {
		task = iscsi_read10_sync(iscsi, lun, 0x80000000, i * block_size, block_size, 0, 0, 0, 0, 0);
		if (task == NULL) {
		        printf("[FAILED]\n");
			printf("Failed to send READ10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
		        printf("[FAILED]\n");
			printf("READ10 command should fail when reading from LBA 2^31\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->status        != SCSI_STATUS_CHECK_CONDITION
			|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
			|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
			printf("[FAILED]\n");
			printf("READ10 failed but with the wrong sense code. It should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


	/* read 1 - 256 blocks at LBA -1 */
	printf("Read 1-256 blocks at LBA -1 ... ");
	for (i = 1; i <= 256; i++) {
		task = iscsi_read10_sync(iscsi, lun, -1, i * block_size, block_size, 0, 0, 0, 0, 0);
		if (task == NULL) {
		        printf("[FAILED]\n");
			printf("Failed to send READ10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
		        printf("[FAILED]\n");
			printf("READ10 command should fail when reading at LBA -1\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->status        != SCSI_STATUS_CHECK_CONDITION
			|| task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
			|| task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
			printf("[FAILED]\n");
			printf("READ10 failed but with the wrong sense code. It should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


	/* read 2-256 blocks, all but one block beyond the eol */
	printf("Reading 1-255 blocks beyond eol starting at last block ... ");
	for (i=2; i<=256; i++) {
		task = iscsi_read10_sync(iscsi, lun, num_blocks, i * block_size, block_size, 0, 0, 0, 0, 0);
		if (task == NULL) {
			printf("[FAILED]\n");
			printf("Failed to send READ10 command: %s\n", iscsi_get_error(iscsi));
			ret = -1;
			goto finished;
		}
		if (task->status == SCSI_STATUS_GOOD) {
			printf("[FAILED]\n");
			printf("READ10 beyond end-of-lun did not return sense.\n");
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		if (task->status        != SCSI_STATUS_CHECK_CONDITION
		    || task->sense.key  != SCSI_SENSE_ILLEGAL_REQUEST
		    || task->sense.ascq != SCSI_SENSE_ASCQ_LBA_OUT_OF_RANGE) {
		        printf("[FAILED]\n");
			printf("READ10 failed but ascq was wrong. Should have failed with ILLEGAL_REQUEST/LBA_OUT_OF_RANGE. Sense:%s\n", iscsi_get_error(iscsi));
			ret = -1;
			scsi_free_scsi_task(task);
			goto finished;
		}
		scsi_free_scsi_task(task);
	}
	printf("[OK]\n");


finished:
	iscsi_logout_sync(iscsi);
	iscsi_destroy_context(iscsi);
	return ret;
}
