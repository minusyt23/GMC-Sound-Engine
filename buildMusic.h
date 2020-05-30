//
// Created by minus on 27/05/20.
//

#ifndef GMG_BUILDMUSIC_H
#define GMG_BUILDMUSIC_H

#define FLAG_COMPILE_REBUILD_PU1 0b00000001
#define FLAG_COMPILE_REBUILD_PU2 0b00000010
#define FLAG_COMPILE_REBUILD_WAV 0b00000100
#define FLAG_COMPILE_REBUILD_NOI 0b00001000

#define PU1_INSTRUCTION_PLAY    0b00000000
#define PU1_INSTRUCTION_JUMP    0b01000000
#define PU1_INSTRUCTION_SET     0b10000000
#define PU1_INSTRUCTION_SPECIAL 0b11000000

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
			unsigned short frequency;
		} pu1;
		struct {
			struct {
				unsigned char startingValue : 4;
				unsigned char sign : 1;
				unsigned char shift : 3;
			} volume;
			unsigned char duty : 2;
			unsigned short frequency;
		} pu2;
		struct {
			unsigned char volume : 2;
			unsigned char duty : 2;
			unsigned short frequency;
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

	} soundChip;

	// Internal variables.
	struct {
		struct {
			unsigned short length;
			unsigned char  loop;
		} pu1;
		struct {
			unsigned short length;
			unsigned char  loop;
		} pu2;
		struct {
			unsigned short length;
			unsigned char  loop;
			unsigned char  selectedWave;
		} wav;
		struct {
			unsigned short length;
			unsigned char  loop;
		} noi;
	} internal;

} soundHandler;

char* CompileMusic(MusicFile m, unsigned int flags)
{
	unsigned long long int pu1len = 0, pu2len = 0, wavlen = 0, noilen = 0, totallen = 0;
	unsigned int i, j, k;
	char* rData;

	/* FIRST STEP:
	 * CALC EVERY CHANNELS LENGTH, THEN DO LITTLE TRICKS TO MALLOC THE HUGE AMOUNT OF MEMORY. */
	i = 0; // PU1 ADDR
	while (i < m.pu2addr)
	{
		switch (m.data[i] & 0b11000000) {
			case PU1_INSTRUCTION_PLAY:
				if(m.data[i] & 0b00001000) // Checks if two length bytes
					pu1len += ((m.data[i+2] & 0b00111111) << 8) | m.data[i+3];
				else
					pu1len += (m.data[i+2] & 0b00111111);
				i += 3 + (m.data[i] & 0b00100000 ? 1 : 0) + (m.data[i] & 0b00010000 ? 1 : 0) + (m.data[i] & 0b00001000 ? 1 : 0);
				break;
			case PU1_INSTRUCTION_JUMP:
				if(soundHandler.internal.pu1.loop > 0)
				{
					if(m.data[i] & 0b00100000)
					{
						i -= ((m.data[i] & 0b00011111) << 8) | m.data[i + 1];
						soundHandler.internal.pu1.loop--;
					}
					else
					{
						i += ((m.data[i] & 0b00011111) << 8) | m.data[i+1];
						soundHandler.internal.pu1.loop--;
					}
				} else i += 2;
				break;
			case PU1_INSTRUCTION_SET:
				if (m.data[i] & 0b00100000) // LOOP CONTROL
					soundHandler.internal.pu1.loop = m.data[i + 1];
				i += 1 + (m.data[i] & 0b00100000 ? 1 : 0) + (m.data[i] & 0b00010000 ? 1 : 0);
				break;
			case PU1_INSTRUCTION_SPECIAL:
				i++;
				break;
			default:
				i++;
				break;
		}
	}
	printf("PU1 length in seconds: %f\n", (float)pu1len * 1/60);

	/* REBUILD PULSE 1 */
	/*
	// if(flags & FLAG_COMPILE_REBUILD_PU1)
		for (i = 0; i < m.pu2addr; i++) {
			switch (m.data[i]) {

			}
		}

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
