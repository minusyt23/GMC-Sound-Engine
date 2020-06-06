//
// Created by minus on 27/05/20.
//

#ifndef GMG_BUILDMUSIC_H
#define GMG_BUILDMUSIC_H

#include <string.h>

#define GMC_LOG_INFO "[GMC SOUND ENGINE - INFO]"

#define FLAG_COMPILE_REBUILD_PU1 ((unsigned int)0b00000001)
#define FLAG_COMPILE_REBUILD_PU2 ((unsigned int)0b00000010)
#define FLAG_COMPILE_REBUILD_WAV ((unsigned int)0b00000100)
#define FLAG_COMPILE_REBUILD_NOI ((unsigned int)0b00001000)

#define PU1_INSTRUCTION_PLAY    0b00000000
#define PU1_INSTRUCTION_JUMP    0b01000000
#define PU1_INSTRUCTION_SET     0b10000000
#define PU1_INSTRUCTION_SPECIAL 0b11000000

#define GMC_SAMPLE_RATE 24576
#define GMC_CHANNELS 2

#define GMC_CPU_CLOCK (4194304)
#define GMC_SCREEN_REFRESH (60)
#define GMC_FRAME_SEQUENCER (512)
#define GMC_CLOCK_PER_REFRESH (GMC_CPU_CLOCK / GMC_SCREEN_REFRESH)

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

typedef struct {
	char* trackName;
	char* authorName;
	unsigned short pu2addr, wavaddr, noiaddr, stopaddr;
	unsigned char* data;
} MusicFile;

struct {
	// Emulated Gameboy APU.
	struct {
		struct {
			struct {
				unsigned char period : 3;
				unsigned char sign : 1;
				unsigned char shift : 3;
			} sweep;
			struct {
				unsigned char startingValue : 4;
				unsigned char sign : 1;
				unsigned char shift : 3;
			} volume;
			unsigned char duty : 2;
			unsigned short frequency : 11;
		} pu1;
		struct {
			struct {
				unsigned char startingValue : 4;
				unsigned char sign : 1;
				unsigned char shift : 3;
			} volume;
			unsigned char duty : 2;
			unsigned short frequency : 11;
		} pu2;
		struct {
			unsigned char volume : 2;
			unsigned char duty : 2;
			unsigned short frequency : 11;
		} wav;
		struct {
			struct {
				unsigned char startingValue : 4;
				unsigned char sign : 1;
				unsigned char shift : 3;
			} volume;
			struct {
				unsigned char shift : 4;
				unsigned char width : 1;
				unsigned char ratio : 3;
			} polyCounter;
		} noi;
		struct {
			struct {
				unsigned char leftVin : 1;
				unsigned char leftVolume : 3;
				unsigned char rightVin : 1;
				unsigned char rightVolume : 3;
			} volume;
			struct {
				unsigned char noileft : 1;
				unsigned char wavleft : 1;
				unsigned char pu2left : 1;
				unsigned char pu1left : 1;
				unsigned char noiright : 1;
				unsigned char wavright : 1;
				unsigned char pu2right : 1;
				unsigned char pu1right : 1;
			} soundOutput;
		} control;

		// Not exposed in real gameboy, just for emulating purposes
		struct {
			unsigned char volEnvelope;
			unsigned char sweepEnvelope;
		} frameSequencer;

	} soundChip;

	// Internal variables.
	struct {
		struct {
			unsigned int pointer; // UNSIGNED SHORT IN GAMEBOY VERSION. USED UNSIGNED INT FOR LONGER TRACKS FOR PC.
			unsigned short length;
			unsigned char  loop;
		} pu1;
		struct {
			unsigned int pointer; // UNSIGNED SHORT IN GAMEBOY VERSION. USED UNSIGNED INT FOR LONGER TRACKS FOR PC.
			unsigned short length;
			unsigned char  loop;
		} pu2;
		struct {
			unsigned int pointer; // UNSIGNED SHORT IN GAMEBOY VERSION. USED UNSIGNED INT FOR LONGER TRACKS FOR PC.
			unsigned short length;
			unsigned char  loop;
			unsigned char  selectedWave;
		} wav;
		struct {
			unsigned int pointer; // UNSIGNED SHORT IN GAMEBOY VERSION. USED UNSIGNED INT FOR LONGER TRACKS FOR PC.
			unsigned short length;
			unsigned char  loop;
		} noi;
	} internal;

} soundHandler;

char* CompileMusic(MusicFile m, unsigned int flags)
{
	unsigned long long int pu1len = 0, pu2len = 0, wavlen = 0, noilen = 0;
	long double dpu1len = 0.0, dpu2len = 0.0, dwavlen = 0.0, dnoilen = 0.0, dtotallen = 0.0;
	unsigned long long int i, j, k;
	char* rData;
	char processedSample;

	/* FIRST STEP:
	 * CALC EVERY CHANNELS LENGTH, THEN DO LITTLE TRICKS TO MALLOC THE HUGE AMOUNT OF MEMORY. */

	/* CALC PU1 LEN */

	soundHandler.internal.pu1.pointer = 0; // PU1 ADDR
	while (soundHandler.internal.pu1.pointer < m.pu2addr)
	{
		switch (m.data[soundHandler.internal.pu1.pointer] & 0b11000000)
		{
			case PU1_INSTRUCTION_PLAY:
				if(m.data[soundHandler.internal.pu1.pointer] & 0b00001000) // Checks if two length bytes
					pu1len += ((m.data[soundHandler.internal.pu1.pointer + 2] & 0b00111111) << 8) | m.data[soundHandler.internal.pu1.pointer + 3];
				else
					pu1len += (m.data[soundHandler.internal.pu1.pointer + 2] & 0b00111111);
				soundHandler.internal.pu1.pointer += 3 + (m.data[soundHandler.internal.pu1.pointer] & 0b00100000 ? 1 : 0) + (m.data[soundHandler.internal.pu1.pointer] & 0b00010000 ? 1 : 0) + (m.data[soundHandler.internal.pu1.pointer] & 0b00001000 ? 1 : 0);
				break;
			case PU1_INSTRUCTION_JUMP:
				if(soundHandler.internal.pu1.loop > 0)
				{
					if(m.data[soundHandler.internal.pu1.pointer] & 0b00100000)
					{
						soundHandler.internal.pu1.pointer -= ((m.data[soundHandler.internal.pu1.pointer] & 0b00011111) << 8) | m.data[soundHandler.internal.pu1.pointer + 1];
						soundHandler.internal.pu1.loop--;
					}
					else
					{
						soundHandler.internal.pu1.pointer += ((m.data[soundHandler.internal.pu1.pointer] & 0b00011111) << 8) | m.data[soundHandler.internal.pu1.pointer + 1];
						soundHandler.internal.pu1.loop--;
					}
				} else soundHandler.internal.pu1.pointer += 2;
				break;
			case PU1_INSTRUCTION_SET:
				if (m.data[soundHandler.internal.pu1.pointer] & 0b00100000) // LOOP CONTROL
					soundHandler.internal.pu1.loop = m.data[soundHandler.internal.pu1.pointer + 1];
				soundHandler.internal.pu1.pointer += 1 + (m.data[soundHandler.internal.pu1.pointer] & 0b00100000 ? 1 : 0) + (m.data[soundHandler.internal.pu1.pointer] & 0b00010000 ? 1 : 0);
				break;
			case PU1_INSTRUCTION_SPECIAL:
				soundHandler.internal.pu1.pointer++;
				break;
			default:
				soundHandler.internal.pu1.pointer++;
				break;
		}
	}

	printf(GMC_LOG_INFO " PU1 length in frames: %u\n", pu1len);

	/* CALC THE OTHERS, FOR TESTING PURPOSES WE'RE NOT GONNA WRITE THE CODE */

	dpu1len = (double)pu1len * (1.0 / 60.0) * GMC_SAMPLE_RATE * GMC_CHANNELS;
	dpu2len = (double)pu2len * (1.0 / 60.0) * GMC_SAMPLE_RATE * GMC_CHANNELS;
	dwavlen = (double)wavlen * (1.0 / 60.0) * GMC_SAMPLE_RATE * GMC_CHANNELS;
	dnoilen = (double)noilen * (1.0 / 60.0) * GMC_SAMPLE_RATE * GMC_CHANNELS;

	dtotallen = max(max(dpu1len, dpu2len),max(dwavlen, dnoilen));

	printf(GMC_LOG_INFO " Total length in samples: %.2f\n", (float)dtotallen);

	rData = (char*)malloc( dtotallen + 255 ); // ADD 255 BYTES TO BE SURE IT DOESN'T SIGSEGV LATER.

	/* REBUILD PULSE 1
	 * HOW, YOU MAY ASK?
	 * UPDATE FRAME SEQ → UPDATE CPU → BUILD SAMPLE → MIX SAMPLE → PUT SAMPLE */

	if(flags & FLAG_COMPILE_REBUILD_PU1)
	{
		// CLEAR SOUND CHIP
		memset(&(soundHandler.soundChip), 0, sizeof(soundHandler.soundChip));

		soundHandler.soundChip.frameSequencer.volEnvelope = 0;
		soundHandler.soundChip.frameSequencer.sweepEnvelope = 1;

		soundHandler.internal.pu1.pointer = 0;

		for (i = 0; i < (unsigned long long int)dpu1len; i++)
		{
			processedSample = 0;

			if(!(i % 48) && i != 0)
			{
				// FRAME SEQUENCER - 512 HERTZ (CUZ SAMPLE RATE / 512 = 48)
				soundHandler.soundChip.frameSequencer.volEnvelope++;
				if (soundHandler.soundChip.frameSequencer.volEnvelope >= 7)
				{
					soundHandler.soundChip.frameSequencer.volEnvelope = 0;
					// DO STUFF
				}

				soundHandler.soundChip.frameSequencer.sweepEnvelope++;
				if (soundHandler.soundChip.frameSequencer.sweepEnvelope >= 3)
				{
					soundHandler.soundChip.frameSequencer.sweepEnvelope = 0;
					// DO STUFF
				}
			}

			if(!(i % 410) && i != 0)
			{
				// UPDATE - ROUGHLY 60 HERTZ (CUZ SAMPLE RATE / 410 = 59.94, CONSIDER THAT THE ACTUAL REFRESH RATE OF THE GAMEBOY IS 59.73 HERTZ)
				if(soundHandler.internal.pu1.length <= 0)
				{
					// READ NEW INSTRUCTION
					switch (m.data[soundHandler.internal.pu1.pointer] & 0b11000000)
					{
						case PU1_INSTRUCTION_PLAY:
							break;
						case PU1_INSTRUCTION_JUMP:
							break;
						case PU1_INSTRUCTION_SET:
							break;
						case PU1_INSTRUCTION_SPECIAL:
							break;
					}
				}
				else
				{
					soundHandler.internal.pu1.length--;
				}

			}

			rData[i] += processedSample;
		}
	}
	/*
	// if (flags & FLAG_COMPILE_REBUILD_PU2)
		for (i = m.pu2addr; i < m.wavaddr; i++) {
			switch (m.data[i]) {

			}
		}
	// if (flags & FLAG_COMPILE_REBUILD_WAV)
		for (i = m.wavaddr; i < m.noiaddr; i++) {
			switch (m.data[i]) {

			}
		}
	// if (flags & FLAG_COMPILE_REBUILD_NOI)
		for (i = m.noiaddr; i < m.stopaddr; i++) {
			switch (m.data[i]) {

			}
		}
	 */
}
#endif //GMG_BUILDMUSIC_H
