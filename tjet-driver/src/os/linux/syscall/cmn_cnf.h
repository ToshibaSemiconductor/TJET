/*
    Copyright 2011-2014 Toshiba Corporation.
    All Rights Reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/**
 *  @file     cmn_cnf.h
 *
 *  @brief    This header file manages the configurable values of common
 *            functions.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_CNF_H__)
#define __CMN_CNF_H__


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief for memory pool configuration
 */
#define CMN_MEM_POOL_MAX_NUM 11

//#define USE_IN_INTR_CONTEXT // default no.

/**
 * @brief for semaphore configuration
 */
#define CMN_SEM_MAX_NUM 15


/**
 * @brief for CPU Lock configuration
 */
#define CMN_LOC_MAX_NUM 10


/**
 * @brief for task configuration
 */
#define CMN_TASK_MAX_NUM 2


/**
 * @brief for timer configuration
 */
#define CMN_TIM_MAX_NUM 10


#endif //__CMN_CNF_H__
