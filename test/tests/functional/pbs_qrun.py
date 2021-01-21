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
import os
import signal


class TestQrun(TestFunctional):

    def setUp(self):
        TestFunctional.setUp(self)
        # set ncpus to a known value, 2 here
        a = {'resources_available.ncpus': 2}
        self.server.manager(MGR_CMD_SET, NODE, a, self.mom.shortname)
        self.pbs_exec = self.server.pbs_conf['PBS_EXEC']
        self.qrun = os.path.join(self.pbs_exec, 'bin', 'qrun')

    def test_invalid_host_val(self):
        """
        Tests that pbs_server should not crash when the node list in
        qrun is ill-formed
        """
        j1 = Job(TEST_USER)
        # submit a multi-chunk job
        j1 = Job(attrs={'Resource_List.select':
                        'ncpus=2:host=%s+ncpus=2:host=%s' %
                        (self.mom.shortname, self.mom.shortname)})
        jid1 = self.server.submit(j1)
        self.server.expect(JOB, {ATTR_state: 'Q'}, jid1)
        exec_vnode = '"\'(%s)+(%s)\'"' % \
                     (self.mom.shortname, self.mom.shortname)
        err_msg = 'qrun: Unknown node  "\'(%s)+(%s)\'"' % \
            (self.mom.shortname, self.mom.shortname)
        try:
            self.server.runjob(jobid=jid1, location=exec_vnode)
        except PbsRunError as e:
            self.assertIn(err_msg, e.msg[0])
            self.logger.info('As expected qrun throws error: ' + err_msg)
        else:
            msg = "Able to run job successfully"
            self.assertTrue(False, msg)
        msg = "Server is not up"
        self.assertTrue(self.server.isUp(), msg)
        self.logger.info("As expected server is up and running")
        j2 = Job(TEST_USER)
        # submit a sleep job
        j2 = Job(attrs={'Resource_List.select': 'ncpus=3'})
        jid2 = self.server.submit(j2)
        self.server.expect(JOB, {ATTR_state: 'Q'}, jid2)
        try:
            self.server.runjob(jobid=jid2, location=exec_vnode)
        except PbsRunError as e:
            self.assertIn(err_msg, e.msg[0])
            self.logger.info('As expected qrun throws error: ' + err_msg)
        else:
            msg = "Able to run job successfully"
            self.assertTrue(False, msg)
        msg = "Server is not up"
        self.assertTrue(self.server.isUp(), msg)
        self.logger.info("As expected server is up and running")

    def test_qrun_hangs(self):
        """
        This test submit 500 jobs with differnt equivalence class,
        turn of scheduling and qrun job to
        verify whether qrun hangs.
        """
        node = self.mom.shortname
        self.server.manager(MGR_CMD_SET, SCHED,
                            {'scheduling': 'False'})
        self.server.manager(MGR_CMD_SET, NODE,
                            {'resources_available.ncpus': 1}, id=node)
        for walltime in range(1, 501):
            j = Job(TEST_USER)
            a = {'Resource_List.walltime': walltime}
            j.set_attributes(a)
            if walltime == 500:
                jid = self.server.submit(j)
            else:
                self.server.submit(j)
        self.logger.info("Submitted 500 jobs with different walltime")
        self.server.manager(MGR_CMD_SET, SERVER,
                            {'scheduling': 'True'})
        self.server.manager(MGR_CMD_SET, SERVER, {'scheduling': 'False'})
        time.sleep(1)
        now = time.time()
        pid = os.fork()
        if pid == 0:
            try:
                self.server.runjob(jobid=jid)
                self.logger.info("Successfully runjob. Child process exit.")
                os._exit(0)
            except PbsRunError as e:
                self.logger.info("Runjob throws error: " + e.msg[0])
        else:
            try:
                self.scheduler.log_match("Starting Scheduling Cycle",
                                         interval=5, starttime=now,
                                         max_attempts=10)
                self.logger.info("No hangs. Parent process exit")
            except PtlLogMatchError:
                os.kill(pid, signal.SIGKILL)
                os.waitpid(pid, 0)
                self.logger.info("Runjob hung. Child process exit.")
                self.fail("Qrun didn't start another sched cycle")

    def test_qrun_subjob(self):
        """
        This test tests if PBS is able to qrun an array subjob
        """
        a = {'resources_available.ncpus': 1}
        self.server.manager(MGR_CMD_SET, NODE, a, id=self.mom.shortname)
        jid = self.server.submit(Job())
        self.server.expect(JOB, {'job_state': 'R'}, jid)
        j = Job(TEST_USER, {ATTR_J: '1-5'})
        arr_jid = self.server.submit(j)
        self.server.expect(JOB, {'job_state': 'Q'}, arr_jid)
        subj2 = j.create_subjob_id(arr_jid, 2)
        self.server.runjob(jobid=subj2)
        self.server.expect(JOB, {'job_state': 'R'}, subj2)
        self.server.expect(JOB, {'job_state': 'S'}, jid)
