#pragma once

#include "KegItem.h"

/*=============================================================================
KegItem data processing callback 'cleaner' functions
=============================================================================*/
int KegItem_ProcPourEn(KegItem_obj* self);
int KegItem_ProcPressureDsrd(KegItem_obj* self);
int KegItem_ProcDateAvail(KegItem_obj* self);
int KegItem_ProcAlerts(KegItem_obj* self);
