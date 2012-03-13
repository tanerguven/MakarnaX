/*
 * Copyright (C) 2011,2012 Taner Guven <tanerguven@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WMC_LIST_H_
#define _WMC_LIST_H_

typedef char * wmc_ptr_t;

#define USE_LIST_1
namespace __wmc_list1 {
#include "__list.h"
};
#undef USE_LIST_1

namespace __wmc_list2 {
#include "__list.h"
};

#define List_1 __wmc_list1::List
#define List_2 __wmc_list2::List


#define define_list(type_t,list_t) \
struct LI_##list_t { static const  wmc_ptr_t offset_node_value; }; \
typedef List_2<type_t, LI_##list_t> list_t;

#define set_list_offset(type_t,list_t,node_name) \
const wmc_ptr_t LI_##list_t::offset_node_value =(wmc_ptr_t)offsetof(type_t,node_name);

/**
 * type_t icerisinde tanimlandiginda kullanmak icin
 * type_t::LI_list_t seklinde
 */
#define set_list_offset_2(type_t,list_t,node_name)					\
const wmc_ptr_t type_t::LI_##list_t::offset_node_value =(wmc_ptr_t)offsetof(type_t,node_name);

#endif /* _WMC_LIST_H_ */
