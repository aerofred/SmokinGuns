/*
===========================================================================
SDL2 audio for iOS
===========================================================================
*/
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"

qboolean snd_inited = qfalse;

cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;

static int dmapos = 0;
static int dmasize = 0;
static SDL_AudioDeviceID audioDev = 0;

static void SNDDMA_AudioCallback( void *userdata, Uint8 *stream, int len )
{
	int pos;
	int tobufend;
	int len1, len2;

	(void)userdata;

	if ( !snd_inited || !dma.buffer )
	{
		memset( stream, 0, len );
		return;
	}

	pos = dmapos * ( dma.samplebits / 8 );
	if ( pos >= dmasize )
		dmapos = pos = 0;

	tobufend = dmasize - pos;
	len1 = len;
	if ( len1 > tobufend )
		len1 = tobufend;
	len2 = len - len1;

	memcpy( stream, dma.buffer + pos, len1 );
	if ( len2 > 0 )
		memcpy( stream + len1, dma.buffer, len2 );

	dmapos += ( len / ( dma.samplebits / 8 ) );
}

qboolean SNDDMA_Init( void )
{
	SDL_AudioSpec desired, obtained;

	if ( snd_inited )
		return qtrue;

	s_sdlBits = Cvar_Get( "s_sdlBits", "16", CVAR_ARCHIVE );
	s_sdlSpeed = Cvar_Get( "s_sdlSpeed", "44100", CVAR_ARCHIVE );
	s_sdlChannels = Cvar_Get( "s_sdlChannels", "2", CVAR_ARCHIVE );
	s_sdlDevSamps = Cvar_Get( "s_sdlDevSamps", "0", CVAR_ARCHIVE );
	s_sdlMixSamps = Cvar_Get( "s_sdlMixSamps", "0", CVAR_ARCHIVE );

	if ( !SDL_WasInit( SDL_INIT_AUDIO ) )
		SDL_Init( SDL_INIT_AUDIO );

	memset( &desired, 0, sizeof( desired ) );
	desired.freq = s_sdlSpeed->integer;
	desired.format = ( s_sdlBits->integer == 8 ) ? AUDIO_U8 : AUDIO_S16LSB;
	desired.channels = (Uint8)s_sdlChannels->integer;
	desired.samples = 1024;
	desired.callback = SNDDMA_AudioCallback;

	audioDev = SDL_OpenAudioDevice( NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_ANY_CHANGE );
	if ( !audioDev )
	{
		Com_Printf( "SDL_OpenAudioDevice failed: %s\n", SDL_GetError() );
		return qfalse;
	}

	dma.speed = obtained.freq;
	dma.samplebits = ( obtained.format & 0xFF ) == 8 ? 8 : 16;
	dma.channels = obtained.channels;
	dma.samples = obtained.samples * obtained.channels;
	dma.submission_chunk = 1;
	dma.buffer = calloc( 1, dma.samples * ( dma.samplebits / 8 ) );
	if ( !dma.buffer )
		return qfalse;

	dmasize = dma.samples * ( dma.samplebits / 8 );
	dmapos = 0;

	SDL_PauseAudioDevice( audioDev, 0 );
	snd_inited = qtrue;
	Com_Printf( "SDL2 audio: %d Hz, %d bits, %d channels\n",
		dma.speed, dma.samplebits, dma.channels );
	return qtrue;
}

int SNDDMA_GetDMAPos( void )
{
	return dmapos;
}

void SNDDMA_Shutdown( void )
{
	if ( audioDev )
	{
		SDL_CloseAudioDevice( audioDev );
		audioDev = 0;
	}
	if ( dma.buffer )
	{
		free( dma.buffer );
		dma.buffer = NULL;
	}
	snd_inited = qfalse;
}

void SNDDMA_BeginPainting( void ) {}

void SNDDMA_Submit( void )
{
	dmapos = 0;
}
