/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "output_i2s_f32.h"
//#include "input_i2s_f32.h"
//include "memcpy_audio.h"
//#include "memcpy_interleave.h"
#include <arm_math.h>


//Here's the function to change the sample rate of the system (via changing the clocking of the I2S bus)
//https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate?p=121365&viewfull=1#post121365
//
//And, a post on how to compute the frac and div portions?  I haven't checked the code presented in this post:
//https://forum.pjrc.com/threads/38753-Discussion-about-a-simple-way-to-change-the-sample-rate?p=188812&viewfull=1#post188812
float setI2SFreq(const float freq_Hz) {
	int freq = (int)freq_Hz;
  typedef struct {
    uint8_t mult;
    uint16_t div;
  } __attribute__((__packed__)) tmclk;

  const int numfreqs = 16;
  const int samplefreqs[numfreqs] = { 2000, 8000, 11025, 16000, 22050, 24000, 32000, 44100, (int)44117.64706 , 48000, 88200, (int)(44117.64706 * 2), 96000, 176400, (int)(44117.64706 * 4), 192000};

#if (F_PLL==16000000)
  const tmclk clkArr[numfreqs] = {{4, 125}, {16, 125}, {148, 839}, {32, 125}, {145, 411}, {48, 125}, {64, 125}, {151, 214}, {12, 17}, {96, 125}, {151, 107}, {24, 17}, {192, 125}, {127, 45}, {48, 17}, {255, 83} };
#elif (F_PLL==72000000)
  const tmclk clkArr[numfreqs] = {{832, 1125}, {32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {32, 375}, {128, 1125}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375}, {249, 397}, {32, 51}, {185, 271} };
#elif (F_PLL==96000000)
  const tmclk clkArr[numfreqs] = {{2, 375},{8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {8, 125},  {32, 375}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125}, {151, 321}, {8, 17}, {64, 125} };
#elif (F_PLL==120000000)
  const tmclk clkArr[numfreqs] = {{8, 1875},{32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {32, 625},  {128, 1875}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625}, {178, 473}, {32, 85}, {145, 354} };
#elif (F_PLL==144000000)
  const tmclk clkArr[numfreqs] = {{4, 1125},{16, 1125}, {49, 2500}, {32, 1125}, {49, 1250}, {16, 375},  {64, 1125}, {49, 625}, {4, 51}, {32, 375}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375} };
#elif (F_PLL==180000000)
  const tmclk clkArr[numfreqs] = {{23, 8086}, {46, 4043}, {49, 3125}, {73, 3208}, {98, 3125}, {37, 1084},  {183, 4021}, {196, 3125}, {16, 255}, {128, 1875}, {107, 853}, {32, 255}, {219, 1604}, {214, 853}, {64, 255}, {219, 802} };
#elif (F_PLL==192000000)
  const tmclk clkArr[numfreqs] = {{1, 375}, {4, 375}, {37, 2517}, {8, 375}, {73, 2483}, {4, 125}, {16, 375}, {147, 2500}, {1, 17}, {8, 125}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125} };
#elif (F_PLL==216000000)
  const tmclk clkArr[numfreqs] = {{8, 3375}, {32, 3375}, {49, 3750}, {64, 3375}, {49, 1875}, {32, 1125},  {128, 3375}, {98, 1875}, {8, 153}, {64, 1125}, {196, 1875}, {16, 153}, {128, 1125}, {226, 1081}, {32, 153}, {147, 646} };
#elif (F_PLL==240000000)
  const tmclk clkArr[numfreqs] = {{4, 1875}, {16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {16, 625}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625} };
#endif

  for (int f = 0; f < numfreqs; f++) {
    if ( freq == samplefreqs[f] ) {
      while (I2S0_MCR & I2S_MCR_DUF) ;
		I2S0_MDR = I2S_MDR_FRACT((clkArr[f].mult - 1)) | I2S_MDR_DIVIDE((clkArr[f].div - 1));
		return (float)(F_PLL / 256 * clkArr[f].mult / clkArr[f].div);
    }
  }
  return 0.0f;
}

audio_block_f32_t * AudioOutputI2S_F32::block_left_1st = NULL;
audio_block_f32_t * AudioOutputI2S_F32::block_right_1st = NULL;
audio_block_f32_t * AudioOutputI2S_F32::block_left_2nd = NULL;
audio_block_f32_t * AudioOutputI2S_F32::block_right_2nd = NULL;
uint16_t  AudioOutputI2S_F32::block_left_offset = 0;
uint16_t  AudioOutputI2S_F32::block_right_offset = 0;
bool AudioOutputI2S_F32::update_responsibility = false;
//DMAMEM static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES]; //local audio_block_samples should be no larger than global AUDIO_BLOCK_SAMPLES
DMAMEM static int32_t i2s_tx_buffer[2*AUDIO_BLOCK_SAMPLES]; //2 channels at 32-bits per sample.  Local "audio_block_samples" should be no larger than global "AUDIO_BLOCK_SAMPLES"
DMAChannel AudioOutputI2S_F32::dma(false);

float AudioOutputI2S_F32::sample_rate_Hz = AUDIO_SAMPLE_RATE;
int AudioOutputI2S_F32::audio_block_samples = AUDIO_BLOCK_SAMPLES;

//#for 16-bit transfers
//#define I2S_BUFFER_TO_USE_BYTES (AudioOutputI2S_F32::audio_block_samples*sizeof(i2s_tx_buffer[0]))

//#for 32-bit transfers
#define I2S_BUFFER_TO_USE_BYTES (AudioOutputI2S_F32::audio_block_samples*2*sizeof(i2s_tx_buffer[0]))


void AudioOutputI2S_F32::begin(void)
{
	bool transferUsing32bit = true;
	begin(transferUsing32bit);
}
void AudioOutputI2S_F32::begin(bool transferUsing32bit) {

	dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_i2s(transferUsing32bit);

	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	//setup DMA parameters
	//if (transferUsing32bit) {
		sub_begin_i32();
	//} else {
	//	sub_begin_i16();
	//}
	
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
	dma.attachInterrupt(isr_32);
	
	// change the I2S frequencies to make the requested sample rate
	setI2SFreq(AudioOutputI2S_F32::sample_rate_Hz);
	
	enabled = 1;
	
	//AudioInputI2S_F32::begin_guts();
}

void AudioOutputI2S_F32::sub_begin_i16(void) {
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	//dma.TCD->SLAST = -sizeof(i2s_tx_buffer);	//original
	dma.TCD->SLAST = -I2S_BUFFER_TO_USE_BYTES;
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;	//original
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 2;
	dma.TCD->DLASTSGA = 0;
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;	//original
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
}

void AudioOutputI2S_F32::sub_begin_i32(void) {
	dma.TCD->SADDR = i2s_tx_buffer; //here's where to get the data from
	
	//let's assume that we'll transfer each sample (left or right) independently.  So 4-byte (32bit) transfers.
	dma.TCD->SOFF = 4;	   //step forward pointer for source data by 4 bytes (ie, 32 bits) after each read
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_32BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT); //each read is 32 bits
	dma.TCD->NBYTES_MLNO = 4;   //how many bytes to send per minor loop. Do each sample (left or right) independently.  So, 4 bytes?   Should be 4 or 8?
	
	//dma.TCD->SLAST = -sizeof(i2s_tx_buffer);	//original
	dma.TCD->SLAST = -I2S_BUFFER_TO_USE_BYTES;  //jump back to beginning of source data when hit the end
	dma.TCD->DADDR = &I2S0_TDR0;  //destination of DMA transfers
	dma.TCD->DOFF = 0;  //do not increment the destination pointer
	//dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;	//original
	dma.TCD->CITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4;   //number of minor loops in a major loop.  I2S_BUFFER_TO_USE_BYTES/NBYTES_MLNO?  Should be 4 or 8?
	dma.TCD->DLASTSGA = 0;
	//dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;	//original
	dma.TCD->BITER_ELINKNO = I2S_BUFFER_TO_USE_BYTES / 4;   //number of minor loops in a major loop.  I2S_BUFFER_TO_USE_BYTES/NBYTES_MLNO? should be 4 or 8?
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
}


/* void AudioOutputI2S_F32::isr_16(void)
{
#if defined(KINETISK)
	int16_t *dest;
	audio_block_t *blockL, *blockR;
	uint32_t saddr, offsetL, offsetR;

	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	//if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {	//original
	if (saddr < (uint32_t)i2s_tx_buffer + I2S_BUFFER_TO_USE_BYTES / 2) {	
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		//dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];	//original
		dest = (int16_t *)&i2s_tx_buffer[audio_block_samples/2];
		if (AudioOutputI2S_F32::update_responsibility) AudioStream_F32::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
	}

	blockL = AudioOutputI2S_F32::block_left_1st;
	blockR = AudioOutputI2S_F32::block_right_1st;
	offsetL = AudioOutputI2S_F32::block_left_offset;
	offsetR = AudioOutputI2S_F32::block_right_offset;

	
	int16_t *d = dest;
	if (blockL && blockR) {
		//memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
		//memcpy_tointerleaveLRwLen(dest, blockL->data + offsetL, blockR->data + offsetR, audio_block_samples/2);
		int16_t *pL = blockL->data + offsetL;
		int16_t *pR = blockR->data + offsetR;
		for (int i=0; i < audio_block_samples/2; i++) {	*d++ = *pL++; *d++ = *pR++; } //interleave
		offsetL += audio_block_samples / 2;
		offsetR += audio_block_samples / 2;
	} else if (blockL) {
		//memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
		int16_t *pL = blockL->data + offsetL;
		for (int i=0; i < audio_block_samples / 2 * 2; i+=2) { *(d+i) = *pL++; } //interleave
		offsetL += audio_block_samples / 2;
	} else if (blockR) {
		int16_t *pR = blockR->data + offsetR;
		for (int i=0; i < audio_block_samples /2 * 2; i+=2) { *(d+i) = *pR++; } //interleave
		offsetR += audio_block_samples / 2;
	} else {
		//memset(dest,0,AUDIO_BLOCK_SAMPLES * 2);
		memset(dest,0,audio_block_samples * 2);
		return;
	}

	//if (offsetL < AUDIO_BLOCK_SAMPLES) { //original
	if (offsetL < (uint16_t)audio_block_samples) {
		AudioOutputI2S_F32::block_left_offset = offsetL;
	} else {
		AudioOutputI2S_F32::block_left_offset = 0;
		AudioStream::release(blockL);
		AudioOutputI2S_F32::block_left_1st = AudioOutputI2S_F32::block_left_2nd;
		AudioOutputI2S_F32::block_left_2nd = NULL;
	}
	//if (offsetR < AUDIO_BLOCK_SAMPLES) {
	if (offsetR < (uint16_t)audio_block_samples) {
		AudioOutputI2S_F32::block_right_offset = offsetR;
	} else {
		AudioOutputI2S_F32::block_right_offset = 0;
		AudioStream::release(blockR);
		AudioOutputI2S_F32::block_right_1st = AudioOutputI2S_F32::block_right_2nd;
		AudioOutputI2S_F32::block_right_2nd = NULL;
	}
#else
	const int16_t *src, *end;
	int16_t *dest;
	audio_block_t *block;
	uint32_t saddr, offset;

	saddr = (uint32_t)(dma.CFG->SAR);
	dma.clearInterrupt();
	if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
		if (AudioOutputI2S_F32::update_responsibility) AudioStream_F32::update_all();
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = (int16_t *)i2s_tx_buffer;
		end = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];
	}

	block = AudioOutputI2S_F32::block_left_1st;
	if (block) {
		offset = AudioOutputI2S_F32::block_left_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S_F32::block_left_offset = offset;
		} else {
			AudioOutputI2S_F32::block_left_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S_F32::block_left_1st = AudioOutputI2S_F32::block_left_2nd;
			AudioOutputI2S_F32::block_left_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
	dest -= AUDIO_BLOCK_SAMPLES - 1;
	block = AudioOutputI2S_F32::block_right_1st;
	if (block) {
		offset = AudioOutputI2S_F32::block_right_offset;
		src = &block->data[offset];
		do {
			*dest = *src++;
			dest += 2;
		} while (dest < end);
		offset += AUDIO_BLOCK_SAMPLES/2;
		if (offset < AUDIO_BLOCK_SAMPLES) {
			AudioOutputI2S_F32::block_right_offset = offset;
		} else {
			AudioOutputI2S_F32::block_right_offset = 0;
			AudioStream::release(block);
			AudioOutputI2S_F32::block_right_1st = AudioOutputI2S_F32::block_right_2nd;
			AudioOutputI2S_F32::block_right_2nd = NULL;
		}
	} else {
		do {
			*dest = 0;
			dest += 2;
		} while (dest < end);
	}
#endif
} */

void AudioOutputI2S_F32::isr_32(void)  //should be called every half of an audio block
{
	int32_t *dest;
	audio_block_f32_t *blockL, *blockR;
	uint32_t saddr; 
	uint32_t offsetL, offsetR;
	
	saddr = (uint32_t)(dma.TCD->SADDR);
	dma.clearInterrupt();
	//if (saddr < (uint32_t)i2s_tx_buffer + sizeof(i2s_tx_buffer) / 2) {	//original 16-bit
	if (saddr < (uint32_t)i2s_tx_buffer + I2S_BUFFER_TO_USE_BYTES / 2) {	//are we transmitting the first half or second half of the buffer?
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		//dest = (int16_t *)&i2s_tx_buffer[AUDIO_BLOCK_SAMPLES/2];	//original, half-way through buffer (buffer is 32-bit elements filled with 16-bit stereo samples)
		dest = (int32_t *)&i2s_tx_buffer[2*(audio_block_samples/2)];  //half-way through the buffer..remember, buffer is 32-bit elements filled with 32-bit stereo samples)
		if (AudioOutputI2S_F32::update_responsibility) AudioStream_F32::update_all();
	} else {
		// DMA is transmitting the second half of the buffer so we must fill the first half
		dest = (int32_t *)i2s_tx_buffer;  //beginning of the buffer
	}

	blockL = AudioOutputI2S_F32::block_left_1st;
	blockR = AudioOutputI2S_F32::block_right_1st;
	offsetL = AudioOutputI2S_F32::block_left_offset;
	offsetR = AudioOutputI2S_F32::block_right_offset;

	
	int32_t *d = dest;
	if (blockL && blockR) {
		//memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
		//memcpy_tointerleaveLRwLen(dest, blockL->data + offsetL, blockR->data + offsetR, audio_block_samples/2);
		float32_t *pL = blockL->data + offsetL;
		float32_t *pR = blockR->data + offsetR;
		for (int i=0; i < audio_block_samples/2; i++) {	//loop over half of the audio block (this routine gets called every half an audio block)
			*d++ = (int32_t) (*pL++); 	
			*d++ = (int32_t) (*pR++); //cast and interleave
		}
		offsetL += (audio_block_samples / 2);
		offsetR += (audio_block_samples / 2);
	} else if (blockL) {
		//memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
		float32_t *pL = blockL->data + offsetL;
		for (int i=0; i < audio_block_samples /2; i++) { 
			*d++ = (int32_t) *pL++;  //cast and interleave
			*d++ = 0;
		}
		offsetL += (audio_block_samples / 2);
	} else if (blockR) {
		float32_t *pR = blockR->data + offsetR;
		for (int i=0; i < audio_block_samples /2; i++) { 
			*d++ = 0;
			*d++ = (int32_t) *pR++;  //cast and interleave
		} 
		offsetR += (audio_block_samples / 2);
	} else {
		//memset(dest,0,AUDIO_BLOCK_SAMPLES * 2);  //half buffer (AUDIO_BLOCK_SAMPLES/2), 16-bits per sample (AUDIO_BLOCK_SAMPLES/2*2), stereo (AUDIO_BLOCK_SAMPLES/2*2*2)
		//memset(dest,0,audio_block_samples * 2 * 4 / 2);//half buffer (AUDIO_BLOCK_SAMPLES/2), 32-bits per sample (AUDIO_BLOCK_SAMPLES/2*4), stereo (AUDIO_BLOCK_SAMPLES/2*4*2)
		for (int i=0; i < audio_block_samples/2; i++) {	//loop over half of the audio block (this routine gets called every half an audio block)
			*d++ = (int32_t) 0; 	
			*d++ = (int32_t) 0; 
			//*d++ = (int32_t) (-200000000L);
		}
		return;
	}

	//if (offsetL < AUDIO_BLOCK_SAMPLES) { //original
	if (offsetL < (uint16_t)audio_block_samples) {
		AudioOutputI2S_F32::block_left_offset = offsetL;
	} else {
		AudioOutputI2S_F32::block_left_offset = 0;
		AudioStream_F32::release(blockL);
		AudioOutputI2S_F32::block_left_1st = AudioOutputI2S_F32::block_left_2nd;
		AudioOutputI2S_F32::block_left_2nd = NULL;
	}
	//if (offsetR < AUDIO_BLOCK_SAMPLES) {
	if (offsetR < (uint16_t)audio_block_samples) {
		AudioOutputI2S_F32::block_right_offset = offsetR;
	} else {
		AudioOutputI2S_F32::block_right_offset = 0;
		AudioStream_F32::release(blockR);
		AudioOutputI2S_F32::block_right_1st = AudioOutputI2S_F32::block_right_2nd;
		AudioOutputI2S_F32::block_right_2nd = NULL;
	}

}

void AudioOutputI2S_F32::convert_f32_to_i16(float32_t *p_f32, int16_t *p_i16, int len) {
	for (int i=0; i<len; i++) { *p_i16++ = max(-32768,min(32768,(int16_t)((*p_f32++) * 32768.f))); }
}
#define F32_TO_I24_NORM_FACTOR (8388607)   //which is 2^23-1
void AudioOutputI2S_F32::convert_f32_to_i24( float32_t *p_f32, float32_t *p_i24, int len) {
	for (int i=0; i<len; i++) { *p_i24++ = max(-F32_TO_I24_NORM_FACTOR,min(F32_TO_I24_NORM_FACTOR,(*p_f32++) * F32_TO_I24_NORM_FACTOR)); }
}
#define F32_TO_I32_NORM_FACTOR (2147483647)   //which is 2^31-1
//define F32_TO_I32_NORM_FACTOR (8388607)   //which is 2^23-1
void AudioOutputI2S_F32::convert_f32_to_i32( float32_t *p_f32, float32_t *p_i32, int len) {
	for (int i=0; i<len; i++) { *p_i32++ = max(-F32_TO_I32_NORM_FACTOR,min(F32_TO_I32_NORM_FACTOR,(*p_f32++) * F32_TO_I32_NORM_FACTOR)); }
	//for (int i=0; i<len; i++) { *p_i32++ = (*p_f32++) * F32_TO_I32_NORM_FACTOR + 512.f*8388607.f; }
}

void AudioOutputI2S_F32::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

	audio_block_f32_t *block_f32;
	audio_block_f32_t *block_f32_scaled;
	block_f32 = receiveReadOnly_f32(0); // input 0 = left channel
	if (block_f32) {
		if (block_f32->length != audio_block_samples) {
			Serial.print("AudioOutputI2S_F32: *** WARNING ***: audio_block says len = ");
			Serial.print(block_f32->length);
			Serial.print(", but I2S settings want it to be = ");
			Serial.println(audio_block_samples);
		}
		//Serial.print("AudioOutputI2S_F32: audio_block_samples = ");
		//Serial.println(audio_block_samples);
	
		//convert F32 to Int16
		block_f32_scaled = AudioStream_F32::allocate_f32();
		convert_f32_to_i32(block_f32->data, block_f32_scaled->data, audio_block_samples);
		AudioStream_F32::release(block_f32);
		
		//now process the data blocks
		__disable_irq();
		if (block_left_1st == NULL) {
			block_left_1st = block_f32_scaled;
			block_left_offset = 0;
			__enable_irq();
		} else if (block_left_2nd == NULL) {
			block_left_2nd = block_f32_scaled;
			__enable_irq();
		} else {
			audio_block_f32_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block_f32_scaled;
			block_left_offset = 0;
			__enable_irq();
			AudioStream_F32::release(tmp);
		}
	}
	
	block_f32 = receiveReadOnly_f32(1); // input 1 = right channel
	if (block_f32) {
		//convert F32 to Int16
		block_f32_scaled = AudioStream_F32::allocate_f32();
		convert_f32_to_i32(block_f32->data, block_f32_scaled->data, audio_block_samples);
		AudioStream_F32::release(block_f32);
		
		__disable_irq();
		if (block_right_1st == NULL) {
			block_right_1st = block_f32_scaled;
			block_right_offset = 0;
			__enable_irq();
		} else if (block_right_2nd == NULL) {
			block_right_2nd = block_f32_scaled;
			__enable_irq();
		} else {
			audio_block_f32_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block_f32_scaled;
			block_right_offset = 0;
			__enable_irq();
			AudioStream_F32::release(tmp);
		}
	}
}


// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 4
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 8
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 16
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 1
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 8
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 4
  #define MCLK_DIV  85
#elif F_CPU == 16000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
	#if (F_CPU >= 20000000)
	  #define MCLK_SRC  3  // the PLL
	#else
	  #define MCLK_SRC  0  // system clock
	#endif
#endif

void AudioOutputI2S_F32::config_i2s(void) {	config_i2s(false); }
void AudioOutputI2S_F32::config_i2s(bool transferUsing32bit) {
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;
	
	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;
	
	//if (transferUsing32bit) {
		config_i2s_i32();
	//} else {
	//	config_i2s_i16();
	//}

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK	
}

void AudioOutputI2S_F32::config_i2s_i16(void)
{

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(3);  //for 32-bit, use I2S_TCR2_DIV(1)
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;   //for 32-bit use  I2S_TCR4_SYWD(31)
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15); //for 32-bit, change all 15 to 31

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(3); //for 32-bit, change I2S_RCR2_DIV(3) to I2S_RCR2_DIV(1)
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD; //for 32-bit, change I2S_RCR4_SYWD(15) to I2S_RCR4_SYWD(31)
	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15); //for 32-bit, change all 15 to 31


}

//32-bit transfers.  Taken from: https://github.com/WMXZ-EU/BasicAudioLogger/blob/master/I2S_32.h
void AudioOutputI2S_F32::config_i2s_i32(void)
{

  // enable MCLK output
  I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
  while (I2S0_MCR & I2S_MCR_DUF) ;
  I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));
  //I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV/2-1)); //For 32-bit?

  // configure transmitter
  I2S0_TMR = 0;
  I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size.  should be 1 or 2?
  I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
    | I2S_TCR2_BCD | I2S_TCR2_DIV(1);  //transmitter must be set to asynchronous mode, 
  I2S0_TCR3 = I2S_TCR3_TCE;
  I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
    | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
  I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

  // configure receiver (sync'd to transmitter clocks)
  I2S0_RMR = 0;
  I2S0_RCR1 = I2S_RCR1_RFW(1); // watermark at half fifo size.  should be 1 or 2?
  I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
    | I2S_RCR2_BCD | I2S_RCR2_DIV(1);  //receiver set to syncrhonous
  I2S0_RCR3 = I2S_RCR3_RCE;
  I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
    | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
  I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

}
/******************************************************************/

/*
void AudioOutputI2Sslave::begin(void)
{
	dma.begin(true); // Allocate the DMA channel first

	//pinMode(2, OUTPUT);
	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2Sslave::config_i2s();
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0
#if defined(KINETISK)
	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
#endif
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	dma.attachInterrupt(isr);
}

void AudioOutputI2Sslave::config_i2s(void)
{
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// Select input clock 0
	// Configure to input the bit-clock from pin, bypasses the MCLK divider
	I2S0_MCR = I2S_MCR_MICS(0);
	I2S0_MDR = 0;

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP;

	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSP;

	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(1);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP;

	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;

	I2S0_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}
*/

