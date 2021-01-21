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

#include <pbs_config.h>   /* the master config generated by configure */

#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pbs_ifl.h>
#include "list_link.h"
#include "attribute.h"
#include "pbs_error.h"


/**
 * @file	attr_fn_time.c
 * @brief
 * 	This file contains functions for manipulating attributes of type
 *	time:	[[hh:]mm:]ss[.sss]
 * @details
 * 	Each set has functions for:
 *	Decoding the value string to the machine representation.
 *	Encoding the internal attribute to external form
 *	Setting the value by =, + or - operators.
 *	Comparing a (decoded) value with the attribute value.
 *
 * Some or all of the functions for an attribute type may be shared with
 * other attribute types.
 *
 * The prototypes are declared in "attribute.h"
 *
 * --------------------------------------------------
 * The Set of Attribute Functions for attributes with
 * value type "long"
 * --------------------------------------------------
 */


/**
 * @brief
 *	decode time into attribute structure of type ATR_TYPE_LONG
 *
 * @param[in/out]   patr - pointer to attribute structure
 * @param[in]   name - attribute name
 * @param[in]   rescn - resource name
 * @param[in]   val - attribute value
 *
 * @return int
 * @retval  0	if ok
 * @retval >0   error
 * @retval *patr elements set
 *
 */

#define PBS_MAX_TIME (LONG_MAX - 1)
int
decode_time(struct attribute *patr, char *name, char *rescn, char *val)
{
	int   i;
	char  msec[4] = {'\0'};
	int   ncolon = 0;
	char *pc;
	long  rv = 0;
	char *workval;
	char *workvalsv;
	long strtol_ret = 0;
	int index = -1;

	if ((val == NULL) || (strlen(val) == 0)) {
		ATR_UNSET(patr);
		patr->at_val.at_long = 0;
		return (0);
	}

	workval = strdup(val);
	if (workval == NULL)
		return (PBSE_SYSTEM);
	workvalsv = workval;

	for (i = 0; i < 3; ++i)
		msec[i] = '0';

	for (pc = workval; *pc; ++pc) {
		index++;
		if (*pc == ':') {
			if ((++ncolon > 2) || (index == 0) || (!isdigit(val[index-1])))
				goto badval;
			*pc = '\0';
			errno = 0;
			strtol_ret = strtol(workval, NULL, 10);
			if ((strtol_ret < 0) || (errno != 0))
				goto badval;
			rv = (rv * 60) + strtol_ret;
			workval = pc + 1;

		} else if (*pc == '.') {
			*pc++ = '\0';
			if ((index == 0) || (!isdigit(val[index-1])))
				goto badval;
			for (i = 0; *pc; ++pc) {
				if (!isdigit((int)*pc)) {
					goto badval;
				}
				if (i < 3) {
					msec[i++] = *pc;
				}
			}
			break;
		} else if (!isdigit((int)*pc)) {
			goto badval;	/* bad value */
		}
	}
	errno = 0;
	strtol_ret = strtol(workval, NULL, 10);
	if ((strtol_ret < 0) || (errno != 0))
		goto badval;
	rv = (rv * 60) + strtol_ret;
	if ((rv > PBS_MAX_TIME) || (rv < 0))
		goto badval;
	if (atoi(msec) >= 500)
		rv++;
	patr->at_val.at_long = rv;
	patr->at_flags |= ATR_SET_MOD_MCACHE;
	(void)free(workvalsv);
	return (0);

	badval:	(void)free(workvalsv);
	return (PBSE_BADATVAL);
}

/**
 * @brief
 *	encode_time - encode attribute of type long into attr_extern
 *	with value in form of hh:mm:ss
 *
 * @param[in] attr - ptr to attribute to encode
 * @param[in] phead - ptr to head of attrlist list
 * @param[in] atname - attribute name
 * @param[in] rsname - resource name or null
 * @param[in] mode - encode mode
 * @param[out] rtnl - ptr to svrattrl
 *
 * @retval      int
 * @retval      >0      if ok, entry created and linked into list
 * @retval      =0      no value to encode, entry not created
 * @retval      -1      if error
 *
 */
/*ARGSUSED*/

#define CVNBUFSZ 24

int
encode_time(const attribute *attr, pbs_list_head *phead, char *atname, char *rsname, int mode, svrattrl **rtnl)
{
	size_t ct;
	unsigned long n;
	unsigned long hr;
	unsigned int min;
	unsigned int sec;
	svrattrl *pal;
	char *pv;
	char cvnbuf[CVNBUFSZ] = {'\0'};

	if (!attr)
		return (-1);
	if (!(attr->at_flags & ATR_VFLAG_SET))
		return (0);
	if (attr->at_val.at_long < 0)
		return (-1);

	n = attr->at_val.at_long;
	hr = n / 3600;
	n %= 3600;
	min = n / 60;
	sec = n % 60;

	pv = cvnbuf;
	(void)sprintf(pv, "%02lu:%02u:%02u", hr, min, sec);
	pv += strlen(pv);

	ct = strlen(cvnbuf) + 1;

	pal = attrlist_create(atname, rsname, ct);
	if (pal == NULL)
		return (-1);

	(void)memcpy(pal->al_value, cvnbuf, ct);
	pal->al_flags = attr->at_flags;
	if (phead)
		append_link(phead, &pal->al_link, pal);
	if (rtnl)
		*rtnl = pal;

	return (1);
}

/*
 * set_time  - use the function set_l()
 *
 * comp_time - use the funttion comp_l()
 *
 * free_l - use free_null to (not) free space
 */

/**
 * @brief
 *	Action routine for attributes of type time (or long) where a zero
 *	value is to be disallowed.
 *
 * @param[in]   pattr - pointer to the changed attribute
 * @param[in]   pobject - pointer to parent object of the attribute - unused
 * @param[in]   actmode - if being set/altered - unused
 *
 * @return      int - a PBSE_ defined error
 * @retval	PBSE_NONE - no error
 * @retval	PBSE_BADATVAL - if being set to zero
 *
 */

int
at_non_zero_time(attribute *pattr, void *pobject, int actmode)
{
	if ((pattr->at_flags & ATR_VFLAG_SET) == 0)
		return PBSE_NONE;
	if (pattr->at_val.at_long == 0)
		return PBSE_BADATVAL;
	else
		return PBSE_NONE;
}
