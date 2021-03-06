/* Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#ifndef PFS_ERROR_H
#define PFS_ERROR_H

/**
  @file storage/perfschema/pfs_error.h
  server error instrument data structures (declarations).
*/

#include <sys/types.h>

#include "lf.h"
#include "mysqld_error.h" /* For lookup */
#include "pfs_server.h"
#include "derror.h"

static const int NUM_SECTIONS =
  sizeof(errmsg_section_start) / sizeof(errmsg_section_start[0]);

extern uint max_server_errors;
extern uint pfs_to_server_error_map[];

struct PFS_thread;
struct PFS_account;
struct PFS_user;
struct PFS_host;

extern server_error error_names_array[total_error_count + 2];

void reset_events_errors_by_thread();
void reset_events_errors_by_account();
void reset_events_errors_by_user();
void reset_events_errors_by_host();
void reset_events_errors_global();
void aggregate_account_errors(PFS_account *account);
void aggregate_user_errors(PFS_user *user);
void aggregate_host_errors(PFS_host *host);

int init_error(const PFS_global_param *param);
void cleanup_error();
uint lookup_error_stat_index(uint mysql_errno);
#endif
