#pragma once

/*----- Literal constants -----*/
#define JSON_GROUP "{%s}"
#define JSON_MAX_STRLEN 400

/*----- types -----*/

/*----- functions -----*/
char* json_group(char* buf, unsigned int sz);
char* json_elem(char* key, void* value );