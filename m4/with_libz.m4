
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

AC_DEFUN([PBS_AC_WITH_LIBZ],
[
  AC_ARG_WITH([libz],
    AS_HELP_STRING([--with-libz=DIR],
      [Specify the directory where libz is installed.]
    )
  )
  [libz_dir="$with_libz"]
  AC_MSG_CHECKING([for libz])
  AS_IF(
    [test "$libz_dir" = ""],
    AC_CHECK_HEADER([zlib.h], [], AC_MSG_ERROR([libz headers not found.])),
    [test -r "$libz_dir/include/zlib.h"],
    [libz_inc="-I$libz_dir/include"],
    AC_MSG_ERROR([libz headers not found.])
  )
  AS_IF(
    # Using system installed libz
    [test "$libz_dir" = ""],
    AC_CHECK_LIB([z], [deflateInit_],
      [libz_lib="-lz"],
      AC_MSG_ERROR([libz shared object library not found.])),
	  # Using developer installed libz
    [test -r "${libz_dir}/lib64/libz.a"],
    [libz_lib="${libz_dir}/lib64/libz.a"],
    [test -r "${libz_dir}/lib/libz.a"],
    [libz_lib="${libz_dir}/lib/libz.a"],
    AC_MSG_ERROR([libz not found.])
  )
  AC_MSG_RESULT([$libz_dir])
  AC_SUBST(libz_inc)
  AC_SUBST(libz_lib)
  AC_DEFINE([PBS_COMPRESSION_ENABLED], [], [Defined when libz is available])
])
