/* Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef MYSQL_COMPONENT_H
#define MYSQL_COMPONENT_H

#include <vector>
#include <mysql/components/services/dynamic_loader.h>
#include "my_metadata.h"

/**
  Wraps st_mysql_component_t component data conforming ABI into C++ object.
*/
class mysql_component : public my_metadata
{
public:
  mysql_component(mysql_component_t* component_data, my_string urn);

  const char* name_c_str() const;
  const char* urn_c_str() const;
  const my_string& get_urn() const;

  std::vector<const mysql_service_ref_t*> get_provided_services() const;
  std::vector<mysql_service_placeholder_ref_t*> get_required_services() const;

  const mysql_component_t* get_data() const;

private:
  mysql_component_t* m_component_data;
  my_string m_urn;
};

#endif /* MYSQL_COMPONENT_H */
