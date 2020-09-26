#pragma once
/*
 *  idea taken from https://forum.arduino.cc/index.php?topic=367842.0
 *
 */

#include <Arduino.h>
#include "config.h"

const int RB_LOG_SIZE = 10;
const int RB_TXT_SIZE = 128;

struct Ringbuffer
{
    //   byte hour;
    //   byte minute;
    //   byte second;
    //   byte day;
    //   byte month;
    //   short year;   // short für 4-stellige Jahreszahlen
    char text[RB_TXT_SIZE];
};
Ringbuffer ringBuffer[RB_LOG_SIZE];
int shadowPtr = 0;

/*
 *
 */
void ringBufferInit()
{
    int i = 0;
    for(i=0; i < RB_LOG_SIZE; ++i) 
    {         
        ringBuffer[i].text[0] = {'\0'};
    }
}

/*
 *
 */
void ringBufferAddItem(const char* text)
{
    static int index;
    strncpy(ringBuffer[index].text, text, sizeof(ringBuffer[index].text));
    // dbgprintf(ico_null, "ringBufferAddItem - index:%d - text: '%s' - bufferItem: '%s'\n", index, text, ringBuffer[index].text);
    index = (index + 1) % RB_LOG_SIZE;
    shadowPtr = index;
}

/*
 *
 */
void ringBufferGetItem(char *text, int id)
{     
    int i = 0;

	// init buffer for return value based on item size 
	size_t size = sizeof(ringBuffer[id].text) / sizeof(ringBuffer[id].text[0]); 

    // get the number of elements in a struct array ringBuffer
    int ringBufferSize = (sizeof(ringBuffer) / sizeof(*ringBuffer));

    if (id > ringBufferSize) 
    {
        strcpy(text, "error, wrong id\0");
    }
	else if (size > RB_TXT_SIZE) 
    {
        strcpy(text, "error, size too big\0");
    } 
    else 
    {        
        for(i=0; i < (int)size; i++) 
        {         
            text[i] = ringBuffer[id].text[i];
        }
        text[i++] = {'\0'};
    }
	// dbgprintf(ico_null, "ringBufferGetItem() - id:%d - size:%d - text:%s - bufferItem: %s\n", id, size, text, ringBuffer[id].text);
}

/*
 *
 */
String ringBufferGetAllItems()
{
	String result = "";

    // get the number of elements in a struct array ringBuffer
    int ringBufferSize = (sizeof(ringBuffer) / sizeof(*ringBuffer));

	for (int i = 0; i < ringBufferSize; i++)
	// for (int i = ringBufferSize-1; i >= 0; i--)
	{
		char txtbuf[RB_TXT_SIZE];
		txtbuf[0] = {'\0'}; // initialize
		ringBufferGetItem(txtbuf, i);

		size_t size = sizeof(txtbuf) / sizeof(txtbuf[0]);

		if (size > 0)
		{
			result += String(txtbuf) + "<br />";
		}
		// dbgprintf(ico_null, "ringBufferGetAllItems() - id:%d - size:%d - text:%s \n", i, size, txtbuf);

	}
	dbgprintln();

	return result;
}

/*
 *
 */
String ringBufferGetAllItemsCircular()
{
	String result = "";

    int ringItem = shadowPtr; // start circle at the current pointer / last added item

	for (int i = 0; i < RB_LOG_SIZE; i++)
	{    
        ringItem++;

        if (ringItem < 0) 
        {
            ringItem = 0; // rotate ring
        }

        if (ringItem > RB_LOG_SIZE-1) 
        {
            ringItem = 0; // rotate ring
        }
         
		char txtbuf[RB_TXT_SIZE];
		txtbuf[0] = {'\0'}; // initialize
		ringBufferGetItem(txtbuf, ringItem);

		// size_t size = sizeof(txtbuf) / sizeof(txtbuf[0]);
		// if (size > 0)
        if (strlen(txtbuf) > 0)
		{
			result += String(txtbuf) + "<br />";
    		// dbgprintf(ico_info, "ringBufferGetAllItems() - id:%d - size:%d - text:%9s - ringItem:%d - shadow:%d\n", i, size, txtbuf, ringItem, shadowPtr);
		}
	}
	// dbgprintln();

	return result;
}

/*
 *
 */
void ringBufferGetAllItemsCircular(char *buffer)
{
	char result[512] = {'\0'};

    int ringItem = shadowPtr; // start circle at the current pointer / last added item

	for (int i = 0; i < RB_LOG_SIZE; i++)
	{    
        ringItem++;

        if (ringItem < 0) 
        {
            ringItem = 0; // rotate ring
        }

        if (ringItem > RB_LOG_SIZE-1) 
        {
            ringItem = 0; // rotate ring
        }
         
		char txtbuf[RB_TXT_SIZE];
		txtbuf[0] = {'\0'}; // initialize
		ringBufferGetItem(txtbuf, ringItem);

		// size_t size = sizeof(txtbuf) / sizeof(txtbuf[0]);
		// if (size > 0)
        if (strlen(txtbuf) > 0)
		{
			// result += String(txtbuf) + "<br />";
            strcat(txtbuf, "<br />");
            strcat(result, txtbuf);
    		dbgprintf(ico_info, "ringBufferGetAllItems() - id:%d - text:%9s - ringItem:%d - shadow:%d\n", i, txtbuf, ringItem, shadowPtr);
		}
	}
	// dbgprintln();
    strcpy(buffer, result);
}