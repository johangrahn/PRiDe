/*
 * Copyright 2009 Johan Grahn
 *
 * This file is part of the PRiDe project 
 *
 * PRiDe is free software: you can distribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the 
 * Free Software Foundation, version 1. 
 *
 * PRiDe is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warrenty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details. 
 *
 * You should have received a copy of the GNU General Public License along with
 * PRiDe. If not, see http://www.gnu.org/licenses/
 */

#include "Config.h"

#ifndef NDEBUG
	#define __DEBUG(fmt, ...) fprintf (__conf.log, "[%d] " fmt "\n", __conf.id, ##__VA_ARGS__ ); fflush( __conf.log )
	#define __WARNING(fmt, ...) fprintf (__conf.log, "\n[%d] WARNING: " fmt "\n\n", __conf.id, ##__VA_ARGS__ ) ; fflush( __conf.log )
	#define __ERROR(fmt, ...) fprintf (__conf.log, "[%d] ERROR: " fmt "\n", __conf.id, ##__VA_ARGS__ ) ; fflush( __conf.log )
#else
	#define __DEBUG(fmt, ...) 
	#define __ERROR(fmt, ...) 
#endif

