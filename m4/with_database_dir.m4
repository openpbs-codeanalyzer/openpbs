
#
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

#

AC_DEFUN([PBS_AC_WITH_DATABASE_DIR],
[
  AC_MSG_CHECKING([for PBS database directory])
  AC_ARG_WITH([database-dir],
    AS_HELP_STRING([--with-database-dir=DIR],
      [Specify the directory where the PBS database is installed.]
    )
  )
  [database_dir="$with_database_dir"]
  AS_IF(
    [test "$database_dir" = ""],
    AC_CHECK_HEADER([libpq-fe.h], [], [database_dir="/usr"])
  )
  AS_IF(
    [test "$database_dir" != ""],
    AS_IF(
      [test -r "$database_dir/include/libpq-fe.h"],
      [database_inc="-I$database_dir/include"],
      [test -r "$database_dir/include/pgsql/libpq-fe.h"],
      [database_inc="-I$database_dir/include/pgsql"],
      [test -r "$database_dir/include/postgresql/libpq-fe.h"],
      [database_inc="-I$database_dir/include/postgresql"],
      AC_MSG_ERROR([Database headers not found.])
    )
  )
  AS_IF(
    # Using system installed PostgreSQL
    [test "$with_database_dir" = ""],
    AC_CHECK_LIB([pq], [PQconnectdb],
      [database_lib="-lpq"],
      AC_MSG_ERROR([PBS database shared object library not found.])),
    # Using developer installed PostgreSQL
    [test -r "$database_dir/lib64/libpq.a"],
    [database_lib="$database_dir/lib64/libpq.a"],
    [test -r "$database_dir/lib/libpq.a"],
    [database_lib="$database_dir/lib/libpq.a"],
    AC_MSG_ERROR([PBS database library not found.])
  )
  AC_MSG_RESULT([$database_dir])
  AC_SUBST([database_dir])
  AC_SUBST([database_inc])
  AC_SUBST([database_lib])
  AC_DEFINE([DATABASE], [], [Defined when PBS database is available])
])
