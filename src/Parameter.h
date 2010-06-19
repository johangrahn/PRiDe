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

#ifndef _PARAMETER_H_
#define _PARAMETER_H_

/* 
 * Here we define the different type of parameter 
 * that exists 
 */
typedef enum {
	paramTypeInt
} ParamType;

/* 
 * Defines a structure that holds information 
 * about the type of parameter and its value 
 */
typedef struct {
	
	/* Defines the type of the parameter */
	ParamType paramType;
	
	/* Storage for the data to the parameter */
	union {
		int intData;
	} paramData;

} Parameter;

#endif 
