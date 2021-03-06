/* Copyright (c) 2017 Oracle and/or its affiliates. All rights reserved.

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

#ifndef DD_SYSTEM_VIEWS__EVENTS_INCLUDED
#define DD_SYSTEM_VIEWS__EVENTS_INCLUDED

#include "dd/impl/system_views/system_view_definition_impl.h"
#include "dd/impl/system_views/system_view_impl.h"

namespace dd {
namespace system_views {

/*
  The class representing INFORMATION_SCHEMA.EVENTS system view definition.
*/
class Events : public System_view_impl<System_view_select_definition_impl>
{
public:
  enum enum_fields
  {
    FIELD_EVENT_CATALOG,
    FIELD_EVENT_SCHEMA,
    FIELD_EVENT_NAME,
    FIELD_DEFINER,
    FIELD_TIME_ZONE,
    FIELD_EVENT_BODY,
    FIELD_EVENT_DEFINITION,
    FIELD_EVENT_TYPE,
    FIELD_EXECUTE_AT,
    FIELD_INTERVAL_VALUE,
    FIELD_INTERVAL_FIELD,
    FIELD_SQL_MODE,
    FIELD_STARTS,
    FIELD_ENDS,
    FIELD_STATUS,
    FIELD_ON_COMPLETION,
    FIELD_CREATED,
    FIELD_LAST_ALTERED,
    FIELD_LAST_EXECUTED,
    FIELD_EVENT_COMMENT,
    FIELD_ORIGINATOR,
    FIELD_CHARACTER_SET_CLIENT,
    FIELD_COLLATION_CONNECTION,
    FIELD_DATABASE_COLLATION
  };

  Events();

  static const Events &instance();

  static const String_type &view_name()
  {
    static String_type s_view_name("EVENTS");
    return s_view_name;
  }

  virtual const String_type &name() const
  { return Events::view_name(); }
};

}
}

#endif // DD_SYSTEM_VIEWS__EVENTS_INCLUDED
