# coding: utf-8

# Copyright (C) 1994-2021 Altair Engineering, Inc.
# For more information, contact Altair at www.altair.com.
#
# This file is part of both the OpenPBS software ("OpenPBS")
# and the PBS Professional ("PBS Pro") software.
#
# Open Source License Information:
#
# OpenPBS is free software. You can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# OpenPBS is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
# License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Commercial License Information:
#
# PBS Pro is commercially licensed software that shares a common core with
# the OpenPBS software.  For a copy of the commercial license terms and
# conditions, go to: (http://www.pbspro.com/agreement.html) or contact the
# Altair Legal Department.
#
# Altair's dual-license business model allows companies, individuals, and
# organizations to create proprietary derivative works of OpenPBS and
# distribute them - whether embedded or bundled with other software -
# under a commercial license agreement.
#
# Use of Altair's trademarks, including but not limited to "PBS™",
# "OpenPBS®", "PBS Professional®", and "PBS Pro™" and Altair's logos is
# subject to Altair's trademark licensing policies.


from tests.functional import *


class TestJobArrayComment(TestFunctional):
    """
    Testing job array comment is accurate
    """

    def test_job_array_comment(self):
        """
        Testing job array comment is correct when one or more sub jobs
        are rejected by mom
        """
        attr = {'resources_available.ncpus': 10}
        self.server.manager(MGR_CMD_SET, NODE, attr, id=self.mom.shortname)
        attr = {'job_history_enable': 'true'}
        self.server.manager(MGR_CMD_SET, SERVER, attr)
        # create a mom hook that rejects and deletes subjob 0, 5, and 7
        hook_name = "reject_subjob"
        hook_body = (
            "import pbs\n"
            "import re\n"
            "e = pbs.event()\n"
            "jid = str(e.job.id)\n"
            "if re.match(r'[0-9]*\[[057]\]', jid):\n"
            "    e.job.delete()\n"
            "    e.reject()\n"
            "else:\n"
            "    e.accept()\n"
        )
        attr = {'event': 'execjob_begin', 'enabled': 'True'}
        self.server.create_import_hook(hook_name, attr, hook_body)

        # Check if the hook copy was successful
        self.server.log_match("successfully sent hook file.*" +
                              hook_name + ".PY", regexp=True,
                              max_attempts=60, interval=2)

        test_job_array = Job(TEST_USER, attrs={
            ATTR_J: '0-9',
            'Resource_List.select': 'ncpus=1'
        })
        jid = self.server.submit(test_job_array)
        attr = {
            ATTR_state: 'B',
            ATTR_comment: (MATCH_RE, 'Job Array Began at .*')
        }
        self.server.expect(JOB, attr, id=jid, attrop=PTL_AND)
        self.server.expect(JOB, {ATTR_comment: 'Not Running: PBS Error:' +
                                 ' Execution server rejected request' +
                                 ' and failed'},
                           id=test_job_array.create_subjob_id(jid, 0),
                           extend='x')
        attr = {
            ATTR_state: 'R',
            ATTR_comment: (MATCH_RE, 'Job run at .*')
        }
        self.server.expect(JOB, attr, extend='x',
                           id=test_job_array.create_subjob_id(jid, 1),
                           attrop=PTL_AND)
        self.server.expect(JOB, {ATTR_comment: 'Not Running: PBS Error:' +
                                 ' Execution server rejected request' +
                                 ' and failed'},
                           id=test_job_array.create_subjob_id(jid, 5),
                           extend='x')
        self.server.expect(JOB, {ATTR_comment: 'Not Running: PBS Error:' +
                                 ' Execution server rejected request' +
                                 ' and failed'},
                           id=test_job_array.create_subjob_id(jid, 7),
                           extend='x')
