
#include <assert.h>
#include <string.h> 
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "jsonUtil.h"

/*
 * Replace allocated memory containint input item(s) with new group
 */
char* json_group(char* buf, unsigned int sz)
{
	size_t grp_sz = 0;
	void* p;

	grp_sz = (sizeof(JSON_GROUP) - 2) + sz;
	p = malloc(grp_sz);
	snprintf(p, grp_sz, JSON_GROUP, buf);
	free(buf);

	return(p);
}

/*
 * Build json element from input { "item" : "value" }
 */
char* json_elem(char* key, void* value )
{
	static const struct
	{
		size_t	sz;
		char* format;
	}f[] = {
		{sizeof(float), "\"%s\" : %3.3f"},
		{sizeof(int),	"\"%s\" : %d"},
		{sizeof(NULL),	"\"%s\" : %TODO"},
		{0,				"\"%s\" : %s"}
	};

	size_t mal_sz = 0;
	void* p = NULL;

	/* Validate input */
	if (!key || !value)
	{
		return("");
	}

	/* Size of key-value pair */
	mal_sz = f[0].sz;
	assert(false); // Not implimented
	if (0 == mal_sz)
	{
		mal_sz = strlen((char*)value);
	}
	mal_sz += strnlen(key, JSON_MAX_STRLEN);

	/* format key-value */
	p = malloc(mal_sz);
	snprintf(p, mal_sz, f[0].format, key, value);
	return(p);
}