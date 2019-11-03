#pragma once

#include "KegItem.h"

/*=============================================================================
KegItem Object Value Setter functions
=============================================================================*/
KegItem_funcSet kegItem_setFloat;
KegItem_funcSet kegItem_setInt;
KegItem_funcSet kegItem_setStr;
KegItem_funcSet kegItem_setBool;
KegItem_funcSet kegItem_setDateTimeFromString;

/*=============================================================================
KegItem Object Value Formatting functions
 - char* must be free'd after use
=============================================================================*/
KegItem_funcChr kegItem_formatFloat;
KegItem_funcChr kegItem_formatInt;
KegItem_funcChr kegItem_formatStr;
KegItem_funcChr kegItem_formatBool;
KegItem_funcChr kegItem_formatDateTime;

/*=============================================================================
Generic Utilities
=============================================================================*/
char* KegItem_toJson(KegItem_obj* self);
KegItem_obj* KegItem_getSiblingByKey(KegItem_obj* self, char* key);
int KegItem_getTapNo(KegItem_obj* self);
