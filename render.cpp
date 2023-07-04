/*
 * assignment2_drums
 * ECS7012 Music and Audio Programming
 *
 * Second assignment, to create a sequencer-based
 * drum machine which plays sampled drum sounds in loops.
 *
 * This code runs on the Bela embedded audio platform (bela.io).
 *
 * Andrew McPherson, Becky Stewart and Victor Zappi
 * 2015-2020
 */

#include <Bela.h>
#include <libraries/Scope/Scope.h>
#include <cmath>
#include "drums.h"
#include <vector>
/* Variables which are given to you: */

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */


const int kButtonPin = 1;
const int kLedPin = 0;

const int kMetronomeStateOff = -1;
int gMetronomeCounter =0;
int gMetronomeInterval =0;
int gLEDInterval =0;
int gMetronomeBeat = kMetronomeStateOff;
int gLocation = 0;

enum 
{ 
	kStateOpen =0,
	kStateClosed,
};

int gState = kStateOpen;
int gCount = 0;
int gCount2 =0;
int gInt = 1;

Scope gScope;
// Last state of the button
int gLastButtonStatus = HIGH;

//int gDrumSampleLocation = 0;
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
// each element of the array is a buffer holding a different drum sounds
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];
// The lengths of each sample are stored in the array 

int gIsPlaying = 0;			/* Whether we should play or not. Implement this in Step 4b. */

/* Read pointer into the current drum sample buffer.
 *
 * TODO (step 3): you will replace this with two arrays, one
 * holding each read pointer, the other saying which buffer
 * each read pointer corresponds to.
 */
std::vector<int>  gReadPointers(16);
std::vector<int> gDrumReadPointers(16);

/* Patterns indicate which drum(s) should play on which beat.
 * Each element of gPatterns is an array, whose length is given
 * by gPatternLengths.
 */
extern int *gPatterns[NUMBER_OF_PATTERNS];
extern int gPatternLengths[NUMBER_OF_PATTERNS];

/* These variables indicate which pattern we're playing, and
 * where within the pattern we currently are. Used in Step 4c.
 */
int gCurrentPattern = 0;
int gCurrentIndexInPattern = 0;


/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
int gEventIntervalMilliseconds = 250;

/* This variable indicates whether samples should be triggered or
 * not. It is used in Step 4b, and should be set in gpio.cpp.
 */
extern int gIsPlaying;

/* This indicates whether we should play the samples backwards.
 */
int gPlaysBackwards = 0;

/* For bonus step only: these variables help implement a fill
 * (temporary pattern) which is triggered by tapping the board.
 */

int gShouldPlayFill = 0;
int gPreviousPattern = 0;


bool setup(BelaContext *context, void *userData)
{
	
	
	if(context->audioFrames != context->digitalFrames) 
	{
		rt_fprintf(stderr, "This example needs audio and digital running at the same rate.\n");
		return false;
	}

	pinMode(context,0,kButtonPin, INPUT);

	pinMode(context, 0, kLedPin, OUTPUT);
    gMetronomeInterval = 0.5*context->audioSampleRate;
	gLEDInterval = 0.05 * context->audioSampleRate;
	/* Step 2: initialise GPIO pins */
	gScope.setup(3, context->audioSampleRate);
	return true;
}


void render(BelaContext *context, void *userData)
{
	for(unsigned int n = 0; n < context->audioFrames; n++) 
	{
		int status = digitalRead(context, n, kButtonPin);
		if(gState == kStateOpen) 
		{
   			// Button is not being interacted with
   			if (status == LOW) 
			{
   				gState = kStateClosed;
   			gCount = 0;
   			}
		}
   		else if(gState == kStateClosed) 
		{
   			// Button was just pressed, wait for debounce
   			// Input: run counter, wait for timeout
   			if (gCount++ >= (gInt)) 
			{
   			
     			if(gMetronomeBeat == kMetronomeStateOff)
				{
 					gMetronomeBeat =0;
 					gMetronomeCounter =0;
 				}
 				else
				{
 					gMetronomeBeat = kMetronomeStateOff;
 				}
 			}
     
     		startPlayingDrum(1);
     		
 			if(gMetronomeBeat != kMetronomeStateOff)
			{
 				if(++gMetronomeCounter >= (gEventIntervalMilliseconds * context->audioSampleRate / 1000))
				{
 					startNextEvent();
 					gMetronomeCounter =0;
 					gMetronomeBeat++;
 					if(gMetronomeBeat >= (4))
					{
 						gMetronomeBeat =0;
 					}
 				}
     
     			startPlayingDrum(1);
	   			gState = kStateOpen;
	   		}
		}

     	if(++gCount>= context->audioSampleRate *  gEventIntervalMilliseconds) 
		{
			startNextEvent();
			gCount = 0;
	   	}

		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) 
		{
			// if(gIsPlaying == 1) {
			for (int i=0;i<16;i++) 
		 	{
            	//read pointers that are active have a value between 0 and the buffer length to which is pointing to
                if (gDrumReadPointers[i]>=0)
				{
                	//active read pointers will move to the next frame
                    if (gReadPointers[i] < gDrumSampleBufferLengths[gDrumReadPointers[i]]) 
					{
                    	gReadPointers[i]++;   
                    }
                }
            	//if the read pointer got to the limit of the buffer, then we deactivate it, setting its value to -1
                else 
				{
                	//reset the read pointer
                	gReadPointers[i]=0;
                    //free the read pointer to be used by other buffer
                    gDrumReadPointers[i]=-1;
                }
            }
		}
                
        //initially output in silence
        for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) 
		{
            float out =0;
            //write buffer values of active read pointers to the output
            for (int i=0;i<16;i++)
			{
                if (gDrumReadPointers[i]>=0)
                    out += gDrumSampleBuffers[gDrumReadPointers[i]][gReadPointers[i]];            
            }
               
            //reduce amplitude to avoid clipping
            audioWrite(context, n, channel, out*0.005);

			/*Trigger th LED on the beat*/
			if(gState == kStateClosed  && (gMetronomeCounter<gLEDInterval)) 
			{
				digitalWriteOnce(context, n, kLedPin, HIGH);
			} 
			else
			{
			digitalWriteOnce(context, n, kLedPin, LOW);
			}
		}
	}
}

/* Start playing a particular drum sound given by drumIndex.
 */
void startPlayingDrum(int drumIndex) 
{
	/* TODO in Steps 3a and 3b */
	for(unsigned int i = 0; i >= 16; i++)
	{
		if (gDrumReadPointers[i] == -1)
		{
			gDrumReadPointers[i] = drumIndex;
			gReadPointers[i]= 0;
			break;
		}
	}
}


	
void startNextEvent() 
{
	startPlayingDrum(1);
}
/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) 
{
	if(event & (1 << drum))
		return 1;
	return 0;
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BelaContext *context, void *userData)
{

}

