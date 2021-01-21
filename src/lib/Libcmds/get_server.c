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

/**
 * @file	get_server.c
 * @brief
 * ------------------------------------
 * As specified in section 5 of the ERS:
 *
 *  5.1.2.  Directing Requests to Correct Server
 *
 *  A  command  shall  perform  its  function  by  sending   the
 *  corresponding  request  for  service  to the a batch server.
 *  The choice of batch servers to which to send the request  is
 *  governed by the following ordered set of rules:
 *
 *  1. For those commands which require or accept a job identif-
 *     ier  operand, if the server is specified in the job iden-
 *     tifier operand as @server, then the batch  requests  will
 *     be sent to the server named by server.
 *
 *  2. For those commands which require or accept a job identif-
 *     ier  operand  and  the @server is not specified, then the
 *     command will attempt to determine the current location of
 *     the  job  by  sending  a  Locate Job batch request to the
 *     server which created the job.
 *
 *  3. If a server component of a destination  is  supplied  via
 *     the  -q  option,  such  as  on  qsub and qselect, but not
 *     qalter, then the server request is sent to that server.
 *
 *  4. The server request is sent to the  server  identified  as
 *     the default server, see section 2.6.3.
 *     [pbs_connect() implements this]
 *
 *  2.6.3.  Default Server
 *
 *  When a server is not specified to a client, the client  will
 *  send  batch requests to the server identified as the default
 *  server.  A client identifies the default server by  (a)  the
 *  setting  of  the environment variable PBS_DEFAULT which con-
 *  tains a destination, or (b) the  destination  in  the  batch
 *  administrator established file {PBS_DIR}/default_destn.
 * ------------------------------------
 *
 * Takes a job_id_in string as input, calls parse_jobid to separate
 * the pieces, then applies the above rules in order
 * If things go OK, the function value is set to 0,
 * if errors, it is set to 1.
 *
 * @par Full legal syntax is:
 *  seq_number[.parent_server[:port]][@current_server[:port]]
 *
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <sys/types.h>
#include <netdb.h>
#include <sys/param.h>
#include "cmds.h"
#include "pbs_ifl.h"
#include "net_connect.h"


/**
 * @brief
 *	processes input jobid according to above mentioned rules
 *
 * @param[in] job_id_in - input job id
 * @param[out] job_id_out - processed job id
 * @param[out] server_out - server name
 *
 * @return	int
 * @retval	0	success
 * @retval	1	error
 *
 */
int
get_server(char *job_id_in, char *job_id_out, char *server_out)
{
	char *seq_number = NULL;
	char *parent_server = NULL;
	char *current_server = NULL;
	char host_server[PBS_MAXSERVERNAME+1];

	if (!job_id_in || !job_id_out || !server_out)
		return 1;

	if (pbs_loadconf(0) != 1)
		return 1;

	/* parse the job_id_in into components */

	if (parse_jobid(job_id_in, &seq_number, &parent_server,
		&current_server)) {
		free(seq_number);
		free(parent_server);
		free(current_server);
		return 1;
	}

	/* Apply the above rules, in order, except for the locate job request.
	 That request is only sent if the job is not found on the local server.
	 */

	server_out[0] = '\0';
	if (notNULL(current_server))		/* @server found */
		strcpy(server_out, current_server);
	free(current_server);

	strcpy(job_id_out, seq_number);
	free(seq_number);

	if (notNULL(parent_server)) {

		/* If parent_server matches PBS_SERVER then use it */
		if (pbs_conf.pbs_server_name) {
			if (strcasecmp(parent_server, pbs_conf.pbs_server_name) == 0) {
				strcat(job_id_out, ".");
				strcat(job_id_out, pbs_conf.pbs_server_name);
				free(parent_server);
				return 0;
			}
		}

		if (get_fullhostname(parent_server, host_server,
				     PBS_MAXSERVERNAME) != 0) {
			free(parent_server);
			return 1;
		}

		strcat(job_id_out, ".");

#ifdef NAS_CANON_JOBID /* localmod 086 */
		strcat(job_id_out, host_server);
#else
		strcat(job_id_out, parent_server);
#endif /* localmod 086 */
		if (server_out[0] == '\0')
			strcpy(server_out, parent_server);
		free(parent_server);
		return 0;
	}

	free(parent_server);

	if (pbs_conf.pbs_server_name) {
		strcat(job_id_out, ".");
		strcat(job_id_out, pbs_conf.pbs_server_name);
	} else {
		return 1;
	}

	return 0;
}
