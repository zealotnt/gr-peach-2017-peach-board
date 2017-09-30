#include "cJson.h"

bool getValueJsonKey(Json json,char * key, char* value)
{
	int keyIndex = json.findKeyIndexIn ( key , 0 );
    if ( keyIndex == -1 )
    {
        // Error handling part ...
        printf("\"%s\" does not exist ... do something!!",key);
        return false;
    }
    else
    {
        // Find the first child index of key-node "city"
        int valueIndex = json.findChildIndexOf ( keyIndex, -1 );
        if ( valueIndex > 0 )
        {
            const char * valueStart  = json.tokenAddress ( valueIndex );
            int          valueLength = json.tokenLength ( valueIndex );
            strncpy ( value, valueStart, valueLength );
            value [ valueLength ] = 0; // NULL-terminate the string
            return true; // success
        }
        return false; // failed
    }
}