#include "common.h"

extern struct retro_audio_callback audio_callback;

static fifo_t *speaker_buffer;
static SDL_AudioDeviceID g_pcm = 0;

static char snd[SNDBUFFLEN]={0};
static bool snd_is_paused=true;

static void sdl_audio_playback_cb(void *data, Uint8 *stream, int len)
{
   
   size_t      avail = FIFO_READ_AVAIL(speaker_buffer);
   size_t write_size = (len > (int)avail) ? avail : (size_t)len;
   //printf("av:%d wc:%d\n",avail,write_size);
   fifo_read(speaker_buffer, stream, write_size);

   /* If underrun, fill rest with silence. */
   memset(stream + write_size, 0, len - write_size);
}

void audio_init(int frequency) {
	
	void * tmp;
	
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;

    SDL_zero(desired);
    SDL_zero(obtained);

    desired.format = AUDIO_S16SYS;
    desired.freq   = frequency;
    desired.channels = 2;
    desired.samples = 1024;	

    desired.callback = sdl_audio_playback_cb;

	speaker_buffer=malloc(sizeof(fifo_t));
	memset(snd,0,SNDBUFFLEN);
	
	fifo_init(speaker_buffer,snd,SNDBUFFLEN);
	
	tmp             = calloc(1,SNDBUFFLEN);
	if (tmp){
		fifo_write(speaker_buffer, tmp,SNDBUFFLEN );
		free(tmp);
	}
	 
    g_pcm = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	
    if (!g_pcm){
        die("Failed to open playback device: %s", SDL_GetError());
	}
	
 	SDL_PauseAudioDevice(g_pcm, 0);
 	
     if (audio_callback.set_state) {
        audio_callback.set_state(true);
    }
    
    snd_is_paused = false;
    
    printf("audio init %d \n",frequency);
    
	return;	
}

void audio_deinit() {

	SDL_CloseAudioDevice(g_pcm);
	free(speaker_buffer);
	return;	
}

 size_t audio_write(const void *buf, unsigned frames) {
	
      size_t ret = 0,written = 0,size=frames*4;
 
 	if(snd_is_paused == true) return 0;
 
      while (written < size)
      { /* Until we've written all the sample data we have available... */
         size_t avail;

         SDL_LockAudioDevice(g_pcm); /* Stop the SDL speaker thread from running */
         avail = FIFO_WRITE_AVAIL(speaker_buffer);

         if (avail == 0)
         { /* If the outgoing sample queue is full... */
            SDL_UnlockAudioDevice(g_pcm);
            /* Let the SDL speaker thread run so it can play the enqueued samples,
             * which will free up space for us to write new ones. */
         }
         else
         {
            size_t write_amt = size - written > avail ? avail : size - written;
            fifo_write(speaker_buffer, (const char*)buf + written, write_amt);
            /* Enqueue as many samples as we have available without overflowing the queue */
            SDL_UnlockAudioDevice(g_pcm); /* Let the SDL speaker thread run again */
            written += write_amt;
         }
      }
      ret = written;
      //printf("w:%d\n",written);
      return ret/4;

	 return frames;
}

bool audio_stop()
{
   if(snd_is_paused == true) return true;
   
   snd_is_paused = true;
   SDL_PauseAudioDevice(g_pcm, true);

   return true;
}

bool audio_alive()
{
   return !snd_is_paused;
}

bool audio_start()
{
   if(snd_is_paused == false) return true;
   
   snd_is_paused = false;
   SDL_PauseAudioDevice(g_pcm, false);

   return true;
}
