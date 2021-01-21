/*
 * Copyright (C) 1994-2021 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *
 * This file is part of both the OpenPBS software ("OpenPBS")
 * and the PBS Professional ("PBS Pro") software.
 *
 * Open Source License Information:
 *
 * OpenPBS is free software. You can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * OpenPBS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * PBS Pro is commercially licensed software that shares a common core with
 * the OpenPBS software.  For a copy of the commercial license terms and
 * conditions, go to: (http://www.pbspro.com/agreement.html) or contact the
 * Altair Legal Department.
 *
 * Altair's dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of OpenPBS and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair's trademarks, including but not limited to "PBS™",
 * "OpenPBS®", "PBS Professional®", and "PBS Pro™" and Altair's logos is
 * subject to Altair's trademark licensing policies.
 */

/*
 * @file	req_locate.c
 * @brief
 * 	Functions relating to the Locate Job Batch Request.
 *
 * Included funtions are:
 *	req_locatejob()
 *
 */
#include <pbs_config.h>   /* the master config generated by configure */

#include <sys/types.h>
#include "libpbs.h"
#include <signal.h>
#include <string.h>
#include "server_limits.h"
#include "list_link.h"
#include "attribute.h"
#include "server.h"
#include "credential.h"
#include "batch_request.h"
#include "job.h"
#include "work_task.h"
#include "tracking.h"
#include "pbs_error.h"
#include "log.h"


/* Global Data Items: */

extern struct server server;
extern char   server_name[];

/* External functions */
extern int svr_chk_histjob(job *);
extern int is_job_array(char *);

/**
 * @brief
 * 		req_locatejob - service the Locate Job Request
 *
 *		This request attempts to locate a job.
 *
 * @param[in]	preq	- Job Request
 */

void
req_locatejob(struct batch_request *preq)
{
	char 		 *at;
	int		  i;
	job		 *pjob;
	char		 *location = NULL;

	if ((at = strchr(preq->rq_ind.rq_locate, (int)'@')) != NULL)
		*at = '\0';			/* strip off @server_name */
	pjob = find_job(preq->rq_ind.rq_locate);

	/*
	 * Reject request for history jobs:
	 *	i) jobs with state FINISHED
	 *	ii) jobs with state MOVED and substate FINISHED
	 */
	if (pjob && svr_chk_histjob(pjob) == PBSE_HISTJOBID) {
		req_reject(PBSE_HISTJOBID, 0, preq);
		return;
	}

	/*
	 * return the location if job is not history (i.e. state is not
	 * JOB_STATE_LTR_MOVED) else search in tracking table.
	 */
	if (pjob && (!check_job_state(pjob, JOB_STATE_LTR_MOVED)))
		location = pbs_server_name;
	else {
		int	job_array_ret;
		job_array_ret = is_job_array(preq->rq_ind.rq_locate);
		if ((job_array_ret == IS_ARRAY_Single) || (job_array_ret == IS_ARRAY_Range)) {
			int i;
			char idbuf[PBS_MAXSVRJOBID+1]={'\0'};
			char *pc;
			for (i=0; i<PBS_MAXSVRJOBID; i++) {
				idbuf[i] = *(preq->rq_ind.rq_locate + i);
				if (idbuf[i] == '[')
					break;
			}
			idbuf[++i] = ']';
			idbuf[++i] = '\0';
			pc = strchr(preq->rq_ind.rq_locate,(int)'.');
			if (pc)
				strcat(idbuf, pc);
			strncpy(preq->rq_ind.rq_locate,idbuf,sizeof(preq->rq_ind.rq_locate));
		}
		for (i=0; i < server.sv_tracksize; i++) {
			if ((server.sv_track+i)->tk_mtime &&
				!strcmp((server.sv_track+i)->tk_jobid, preq->rq_ind.rq_locate)) {
				location = (server.sv_track+i)->tk_location;
				break;
			}
		}
	}
	if (location) {
		preq->rq_reply.brp_code = 0;
		preq->rq_reply.brp_auxcode = 0;
		preq->rq_reply.brp_choice = BATCH_REPLY_CHOICE_Locate;
		(void)strcpy(preq->rq_reply.brp_un.brp_locate, location);
		reply_send(preq);
	} else
		req_reject(PBSE_UNKJOBID, 0, preq);
	return;
}
