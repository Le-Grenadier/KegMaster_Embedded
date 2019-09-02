/*
 * Hardware Access interface for Keg Master
 */

#include <signal.h>
#include <string.h>

#include "ConnectionStringPrv.h"
#include "KegMaster.h"
#include "KegItem.h"

#include "azure_iot_utilities.h"
#include "azureiot/iothub_device_client_ll.h"
#include "epoll_timerfd_utilities.h"

/* Available Data  */
//const char* KegItem_Fields[] =
//	{
//	{ KM_SW, "Alerts",				},
//	{ KM_HW, "TapNo",				},
//	{ KM_SW, "Name",				},
//	{ KM_SW, "Style",				},
//	{ KM_SW, "Description",			},
//	{ KM_SW, "DateKegged",			},
//	{ KM_SW, "DateAvail",			},
//	{ KM_SW, "PourEn",				},
//	{ KM_SW, "PourNotification",	},
//	{ KM_SW, "PourQtyGlass",		},
//	{ KM_SW, "PourQtySample",		},
//	{ KM_HW, "PressureCrnt",		},
//	{ KM_SW, "PressureDsrd",		},
//	{ KM_HW, "PressureDwellTime",	},
//	{ KM_HW, "PressureEn",			},
//	{ KM_HW, "QtyAvailable",		},
//	{ KM_SW, "QtyReserve",			},
//	{ KM_SW, "Version",				},
//	{ KM_SW, "CreatedAt",			},
//	{ KM_SW, "UpdatedAt",			},
//	{ KM_SW, "Deleted"				}
//	};


int KegMaster_initRemote()
{
	// Tell the system about the callback function that gets called when we receive a device twin update message from Azure
	//AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

	return 0;
}

int KegMaster_initLocal()
{


	//if (initI2c() == -1) {
	//	return -1;
	//}
	return(0);
}

int KegMaster_initProcs()
{
	/* Custom error handling */
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	//action.sa_handler = TerminationHandler;
	//sigaction(SIGTERM, &action, NULL);


	return(0);
}

int KegMaster_dbGetKegData(void)
{
	return(true);
}
