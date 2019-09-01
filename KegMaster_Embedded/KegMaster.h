/*
 * Hardware Access interface description for Keg Master
 */

#pragma once

typedef enum
{
	KM_HW,
	KM_SW,
	KM_SWHW // SW drives HW feedback avail
}KegMaster_DataSource;