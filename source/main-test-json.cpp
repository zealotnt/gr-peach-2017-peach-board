/*
* @Author: zealotnt
* @Date:   2017-09-27 09:10:31
* @Last Modified by:   Tran Tam
* @Last Modified time: 2017-09-27 09:14:35
*/
#include "Json.h"
#include "stdio.h"

#define logError(...)       printf(__VA_ARGS__); printf("\r\n");
#define logInfo(...)        printf(__VA_ARGS__); printf("\r\n");

int main_test_json() {
    // Note that the JSON object is 'escaped'.  One doesn't get escaped JSON
    // directly from the webservice, if the response type is APPLICATION/JSON
    // Just a little thing to keep in mind.
    const char * jsonSource = "{\"team\":\"Night Crue\",\"company\":\"TechShop\",\"city\":\"San Jose\",\"state\":\"California\",\"country\":\"USA\",\"zip\":95113,\"active\":true,\"members\":[{\"firstName\":\"John\",\"lastName\":\"Smith\",\"active\":false,\"hours\":18.5,\"age\":21},{\"firstName\":\"Foo\",\"lastName\":\"Bar\",\"active\":true,\"hours\":25,\"age\":21},{\"firstName\":\"Peter\",\"lastName\":\"Jones\",\"active\":false}]}";

    Json json ( jsonSource, strlen ( jsonSource ), 100 );

    if ( !json.isValidJson () )
    {
        logError ( "Invalid JSON: %s", jsonSource );
        return -1;
    }

    if ( json.type (0) != JSMN_OBJECT )
    {
        logError ( "Invalid JSON.  ROOT element is not Object: %s", jsonSource );
        return -1;
    }

    // Let's get the value of key "city" in ROOT object, and copy into
    // cityValue
    char cityValue [ 32 ];

    logInfo ( "Finding \"city\" Key ... " );
    // ROOT object should have '0' tokenIndex, and -1 parentIndex
    int cityKeyIndex = json.findKeyIndexIn ( "city", 0 );
    if ( cityKeyIndex == -1 )
    {
        // Error handling part ...
        logError ( "\"city\" does not exist ... do something!!" );
    }
    else
    {
        // Find the first child index of key-node "city"
        int cityValueIndex = json.findChildIndexOf ( cityKeyIndex, -1 );
        if ( cityValueIndex > 0 )
        {
            const char * valueStart  = json.tokenAddress ( cityValueIndex );
            int          valueLength = json.tokenLength ( cityValueIndex );
            strncpy ( cityValue, valueStart, valueLength );
            cityValue [ valueLength ] = 0; // NULL-terminate the string

            //let's print the value.  It should be "San Jose"
            logInfo ( "city: %s", cityValue );
        }
    }

    // More on this example to come, later.
    return 0;
}
