#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"

qboolean snd_inited = qfalse;
cvar_t *s_sdlBits;
cvar_t *s_sdlSpeed;
cvar_t *s_sdlChannels;
cvar_t *s_sdlDevSamps;
cvar_t *s_sdlMixSamps;

static SDL_AudioDeviceID audioDevice;
static int dmapos;
static int dmasize;

static void SNDDMA_AudioCallback( void *userdata, Uint8 *stream, int len )
{
	int pos = dmapos * ( dma.samplebits / 8 );
	int toEnd;
	int len1;
	int len2;

	(void)userdata;

	if( !snd_inited || !dma.buffer || dmasize <= 0 )
	{
		memset( stream, 0, len );
		return;
	}

	if( pos >= dmasize )
		dmapos = pos = 0;

	toEnd = dmasize - pos;
	len1 = len > toEnd ? toEnd : len;
	len2 = len - len1;

	memcpy( stream, dma.buffer + pos, len1 );
	if( len2 > 0 )
	{
		memcpy( stream + len1, dma.buffer, len2 );
		dmapos = len2 / ( dma.samplebits / 8 );
	}
	else
	{
		dmapos += len1 / ( dma.samplebits / 8 );
	}
}

qboolean SNDDMA_Init( void )
{
	SDL_AudioSpec desired;
	SDL_AudioSpec obtained;
	int samples;
	int allowedChanges;

	if( snd_inited )
		return qtrue;

	if( !s_sdlBits )
	{
		s_sdlBits = Cvar_Get( "s_sdlBits", "16", CVAR_ARCHIVE );
		s_sdlSpeed = Cvar_Get( "s_sdlSpeed", "22050", CVAR_ARCHIVE );
		s_sdlChannels = Cvar_Get( "s_sdlChannels", "2", CVAR_ARCHIVE );
		s_sdlDevSamps = Cvar_Get( "s_sdlDevSamps", "0", CVAR_ARCHIVE );
		s_sdlMixSamps = Cvar_Get( "s_sdlMixSamps", "0", CVAR_ARCHIVE );
	}

	SDL_SetHint( SDL_HINT_AUDIODRIVER, "coreaudio" );
	SDL_SetHint( SDL_HINT_AUDIO_CATEGORY, "playback" );

	if( SDL_InitSubSystem( SDL_INIT_AUDIO ) < 0 )
	{
		Com_Printf( "SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s\n", SDL_GetError() );
		return qfalse;
	}

	memset( &desired, 0, sizeof( desired ) );
	desired.freq = s_sdlSpeed->integer > 0 ? s_sdlSpeed->integer : 22050;
	desired.format = s_sdlBits->integer == 8 ? AUDIO_U8 : AUDIO_S16SYS;
	desired.channels = s_sdlChannels->integer > 0 ? s_sdlChannels->integer : 2;
	desired.samples = s_sdlDevSamps->integer > 0 ? s_sdlDevSamps->integer : 1024;
	desired.callback = SNDDMA_AudioCallback;
	allowedChanges = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE;

	audioDevice = SDL_OpenAudioDevice( NULL, 0, &desired, &obtained, allowedChanges );
	if( !audioDevice )
	{
		Com_Printf( "SDL_OpenAudioDevice failed: %s\n", SDL_GetError() );
		SDL_QuitSubSystem( SDL_INIT_AUDIO );
		return qfalse;
	}

	samples = s_sdlMixSamps->integer > 0 ? s_sdlMixSamps->integer : ( obtained.samples * obtained.channels ) * 10;
	if( samples & ( samples - 1 ) )
	{
		int powerOfTwo = 1;
		while( powerOfTwo < samples )
			powerOfTwo <<= 1;
		samples = powerOfTwo;
	}
	dma.samplebits = obtained.format & 0xFF;
	dma.channels = obtained.channels;
	dma.speed = obtained.freq;
	dma.samples = samples;
	dma.submission_chunk = 1;
	dmasize = dma.samples * ( dma.samplebits / 8 );
	dma.buffer = calloc( 1, dmasize );
	dmapos = 0;

	snd_inited = qtrue;
	SDL_PauseAudioDevice( audioDevice, 0 );
	Com_Printf( "SDL2 audio: %d Hz, %d bits, %d channels\n", dma.speed, dma.samplebits, dma.channels );
	return qtrue;
}

void SNDDMA_Activate( void )
{
	if( snd_inited && audioDevice )
		SDL_PauseAudioDevice( audioDevice, 0 );
}

int SNDDMA_GetDMAPos( void )
{
	return dmapos;
}

void SNDDMA_Shutdown( void )
{
	if( !snd_inited )
		return;
	SDL_PauseAudioDevice( audioDevice, 1 );
	SDL_CloseAudioDevice( audioDevice );
	audioDevice = 0;
	free( dma.buffer );
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	SDL_QuitSubSystem( SDL_INIT_AUDIO );
}

void SNDDMA_Submit( void )
{
	if( audioDevice )
		SDL_UnlockAudioDevice( audioDevice );
}

void SNDDMA_BeginPainting( void )
{
	if( audioDevice )
		SDL_LockAudioDevice( audioDevice );
}

void SNDDMA_EndPainting( void )
{
}
