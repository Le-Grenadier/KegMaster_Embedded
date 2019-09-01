#pragma once

#include <stdbool.h>
/*-----------------------------------------------------------------------------
 * Functions and definitions to support interface with KegItem Azure SQL DB
 *---------------------------------------------------------------------------*/

/*----- Literal constants -----*/
#define KEGMASTER_NUMKEGS 4



/*----- Functions -----*/ 

/* Get Data */
unsigned int KegMaster_GetFlow(int kegId);
float		KegMaster_GetHumidity();
float		KegMaster_GetPresAtm();
float		KegMaster_GetPresKeg(int kegId);
float		KegMaster_GetScale(int kegId);
float		KegMaster_GetTemp(int kegId);

/* Set Data */
int KegMaster_SetValveGas(int kegId, bool open);
int KegMaster_SetValveLqd(int kegId, bool open);