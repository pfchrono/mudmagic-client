/**************************************************************************
*  Mud Magic Client                                                       *
*  Copyright (C) 2005 MudMagic.Com  ( hosting@mudmagic.com )              *
*                                                                         *
***************************************************************************/
/**************************************************************************
* msp.c:                                                                  *
*                2004  Calvin Ellis  ( kyndig@mudmagic.com )              *
*									  *
***************************************************************************/
/***************************************************************************
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef HAVE_WINDOWS
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#endif // HAVE_WINDOWS
#include <mudmagic.h>
#include <interface.h>
#include "protocols.h"

extern CONFIGURATION *config;

enum {
	MSP_STATE_SEP,			// between parameters
	MSP_STATE_FNAME, 		// after fname parameter begins
	MSP_STATE_PNAME,		// after parameter name
	MSP_STATE_EQUAL, 		// after parameter name and equal
	MSP_STATE_DONE			// after )
};
enum {
	MSP_OK,		// trigger may begin , after \n	
	MSP_NOTOK, 	// if a not space character have receive after \n
	MSP_EX1,  	// after first  "!"
	MSP_EX2,  	// after second "!"
	MSP_SN1,  	// after !!S
	MSP_SN2,  	// after !!SO
	MSP_SN3,  	// after !!SOU
	MSP_SN4,  	// after !!SOUN
	MSP_SN5,  	// after !!SOUND
	MSP_MS1,  	// after !!M
	MSP_MS2,  	// after !!MU
	MSP_MS3,  	// after !!MUS
	MSP_MS4,  	// after !!MUSI
	MSP_MS5,  	// after !!MUSIC
	MSP_BP,   	// begin parameters , after !!SOUND( or !!MUSIC(
	MSP_EP   	// end   parameters ; after !!SOUND(...) or !!MUSIC(...)
};


static void msp_trigger_reset( MSP_TRIGGER *trigger){
	gint i;
	if ( trigger->fname ) {
		g_free( trigger->fname );
		trigger->fname = NULL; 
	}	
	if ( trigger->url ) {
		g_free( trigger->url );
		trigger->url = NULL; 
	}	
	trigger->volume = 100;
	trigger->priority = 50;
	trigger->loop = 1;
	if ( trigger->sound_type ) {
		g_free( trigger->sound_type );
		trigger->sound_type = NULL; 
	}	
	trigger->pid = 0;
	trigger->type = MSP_NULL;	
	for ( i = 0 ; i < MSPMAXFILES ; i ++ ) 
		if ( trigger->filenames[i] != NULL ) {
			g_free( trigger->filenames[i] ); 
			trigger->filenames[i] = NULL;
		}
}

// msp_trigger_dump() Called below to provide debug output
void msp_trigger_dump( MSP_TRIGGER * trigger ) {
	gint i;

	g_return_if_fail( trigger != NULL );
	mdebug (DBG_SOUND, 1, "MSP Dump  :\n" );
	mdebug (DBG_SOUND, 1, "type      : %d\n", trigger->type );
	mdebug (DBG_SOUND, 1, "fname     : %s\n", (trigger->fname != NULL) ? trigger->fname : "NULL" );
	mdebug (DBG_SOUND, 1, "loop      : %d\n", trigger->loop );
	mdebug (DBG_SOUND, 1, "url       : %s\n", (trigger->url != NULL) ? trigger->url : "NULL" );
	mdebug (DBG_SOUND, 1, "volume    : %d\n", trigger->volume );
	mdebug (DBG_SOUND, 1, "priority  : %d\n", trigger->priority );
	mdebug (DBG_SOUND, 1, "sound_type: %s\n", (trigger->sound_type != NULL) ? trigger->sound_type : "NULL" );

	for ( i = 0 ; i < MSPMAXFILES ; i ++ ) 
		if ( trigger->filenames[i] != NULL ) {
			mdebug(DBG_SOUND, 1, "file %d   : %s\n", i, trigger->filenames[i] );
		}
	mdebug (DBG_SOUND, 1, "pid       : %d\n", trigger->pid );
	mdebug (DBG_SOUND, 1, "End MSP dump\n" );
}

MSP_TRIGGER *msp_trigger_new( ) {
	MSP_TRIGGER *ret;
	ret = g_new0( MSP_TRIGGER, 1 );
	msp_trigger_reset( ret );
	return ret;
}

void msp_trigger_free( MSP_TRIGGER *trigger ) {
	msp_trigger_reset( trigger );
	g_free( trigger );
}

MSP_INFO* msp_new(gpointer link){
	MSP_INFO *ret;
	ret = g_new0( MSP_INFO, 1 ); 
	ret->sound   = msp_trigger_new();
	ret->music   = msp_trigger_new();
	ret->trigger = msp_trigger_new();
	ret->link = link;
	return ret;
}

void msp_free(MSP_INFO *msp){
	msp_trigger_free( msp->sound );
	msp_trigger_free( msp->music );
	msp_trigger_free( msp->trigger );
	g_free( msp );
}

void msp_stop( MSP_TRIGGER *trigger ) {
	trigger->loop = 0; // reset the other iterations
		// mciSendString
		if ( trigger->pid != 0 ) {
#ifdef HAVE_WINDOWS
			if ( trigger->type == MSP_SOUND ) {
				mciSendString( "stop  s1", NULL, 0, NULL );
				mciSendString( "close s1", NULL, 0, NULL );
			}
			if ( trigger->type == MSP_SOUND ) {
				mciSendString( "stop  m1", NULL, 0, NULL );
				mciSendString( "close m1", NULL, 0, NULL );
			}
			trigger->pid = 0;
#else 
			g_message( "kill process with pid %d.", trigger->pid );
			kill( trigger->pid, 2 );
#endif
		}
}

void msp_play( MSP_TRIGGER *trigger ) {
	
	
	g_return_if_fail( trigger != NULL );
	g_return_if_fail( trigger->n > 0  );
#ifdef HAVE_WINDOWS
#else
	gchar **argv=NULL;
	gchar **p, *t;

	if ( g_str_has_suffix( trigger->fname, ".mp3" ) )
		argv = g_strsplit( config->mp3cmd, " ", 10 );
	if ( g_str_has_suffix( trigger->fname, ".wav" )) 
		argv = g_strsplit( config->wavcmd, " ", 10 );
	if ( g_str_has_suffix( trigger->fname, ".mid" ) ) 
		argv = g_strsplit( config->midcmd, " ", 10 );
	p = argv;
	// maybe there is a better way to do this ...
	while ( *p ) {
		if ( strstr( *p, "%d" ) != NULL ) {
			t = *p; *p = g_strdup_printf( t, trigger->volume ); g_free( t );
		}
		if ( strstr( *p, "%s" ) != NULL ) {
			t = *p; 
			*p = g_strdup_printf( 
				t, trigger->filenames[ g_random_int_range(0, trigger->n) ] 
			); 
			g_free( t );
		}
		g_print(">>> %s\n", *p );
		p++;
	}
#endif // HAVE_WINDOWS
	
	
	while ( trigger->loop != 0 ) {
#ifdef HAVE_WINDOWS
	// mciSendString
	gchar *cmd;

	if ( trigger->type == MSP_SOUND ) {
		cmd = g_strdup_printf( 
			"open %s as s1",  
			trigger->filenames[g_random_int_range(0, trigger->n)]
		); 
		mciSendString( cmd, NULL, 0, NULL );
		g_free( cmd );
		mciSendString( "play s1 wait", NULL, 0, NULL );
		trigger->pid = 1;
	}
	if ( trigger->type == MSP_SOUND ) {
		cmd = g_strdup_printf( 
			"open %s as m1",  
			trigger->filenames[g_random_int_range(0, trigger->n)]
		); 
		mciSendString( cmd, NULL, 0, NULL );
		g_free( cmd );
		mciSendString( "play m1 wait", NULL, 0, NULL );
		trigger->pid = 2;
	}
#else 
		g_spawn_async( 
			".", argv, NULL, 
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, 
			NULL, NULL, &trigger->pid, NULL
		);
		waitpid( trigger->pid, NULL, 0 );
#endif // HAVE_WINDOWS
		if ( trigger->loop > 0  ) 
			trigger->loop--;
	}
	trigger->pid = 0;
}

void msp_handle( MSP_INFO *msp ){
	GDir  *dir;
	const gchar *file;
	gchar *t=NULL;
	GPatternSpec *pattern;
	MSP_TRIGGER *temp;
	
	gchar *sounddir; 
	gchar *fullname;
	gchar *basename;
	gchar *dirname;
        int fd;

	// sanity check 
	g_return_if_fail( msp != NULL );

	if  ( strcmp( msp->trigger->fname, "Off"  ) == 0 ) {
		if ( msp->trigger->type == MSP_SOUND ) {
			if ( msp->trigger->url == NULL ) {
				msp_stop( msp->sound );
			} else {
				strcpy( msp->urls, msp->trigger->url );
			}
		} 
		if ( msp->trigger->type == MSP_MUSIC ) {
			if ( msp->trigger->url == NULL ) {
				msp_stop( msp->music );
			} else {
				strcpy( msp->urlm, msp->trigger->url );
			}
		}
		return;
	} 
	
	if ( strchr( msp->trigger->fname, '.' ) == NULL) { // we have an extension ?
		// if not append one 
		if ( msp->trigger->type == MSP_SOUND ) {
			t = g_strconcat( msp->trigger->fname, ".wav", NULL );
		}
		if ( msp->trigger->type == MSP_MUSIC ) {
			t = g_strconcat( msp->trigger->fname, ".mid", NULL );
		}
		g_free( msp->trigger->fname ); msp->trigger->fname = t;
	}

	sounddir = g_build_filename( 
		((SESSION_STATE*)msp->link)->slot, "sounds", NULL 
	);
	utils_mkdir( sounddir );

	fullname = g_strdup_printf("%s/%s", sounddir, msp->trigger->fname );
	
	basename = g_path_get_basename( fullname );
	
	dirname = g_path_get_dirname( fullname );

        mdebug (DBG_SOUND, 1, "sounddir: %s\n", sounddir);
        mdebug (DBG_SOUND, 1, "fullname: %s\n", fullname);
        mdebug (DBG_SOUND, 1, "basename: %s\n", basename);
        mdebug (DBG_SOUND, 1, "dirname: %s\n", dirname);

	// fill matches 
	dir = g_dir_open( dirname, 0, NULL );
	if ( dir == NULL ) {
		g_warning( "msp_handle : directory \"%s\" couldn't be opened", dirname);
		utils_mkdir( dirname );
	} else {
		pattern = g_pattern_spec_new( basename );
		while ( (file = g_dir_read_name( dir )) != NULL ) {
			if ( g_pattern_match_string( pattern, file ) ) {
				msp->trigger->filenames[msp->trigger->n++] = 			
					g_strdup_printf("%s/%s", dirname, file );
			}
		}
		g_pattern_spec_free( pattern );
		g_dir_close( dir );
	}
	
	
	// if no matches ...
	if ( msp->trigger->n == 0  && config->download ) { 
		// ... then try to download 
		g_message( "MSP: no file matches '%s' ;", msp->trigger->fname );
		gchar *fullurl = NULL;
		if ( msp->trigger->url != NULL) { // have url specified ? 
			fullurl = g_strdup_printf(
				"%s/%s", msp->trigger->url, msp->trigger->fname  // use it ...
			);
		} else {  
			if ( msp->trigger->type == MSP_SOUND && *msp->urls != '\0' ) {
				fullurl = g_strdup_printf( // ... or use default 
					"%s/%s", msp->urls, msp->trigger->fname 
				);
			}
			if ( msp->trigger->type == MSP_MUSIC && *msp->urlm != '\0' ) {
				fullurl = g_strdup_printf( // ... or use dafault  
					"%s/%s", msp->urlm, msp->trigger->fname 
				);
			}
		}
		if ( fullurl != NULL )
		{
			HttpHelper* hh = httphelper_new (msp->trigger->url);
                        fd = open (fullname, O_WRONLY | O_CREAT | O_TRUNC, MUD_NEW_FILE_MODE);
                        if (fd != -1)
                          {
                            if ( http_download( fullurl, fd, hh ) == CONNECT_OK )
				msp->trigger->filenames[msp->trigger->n++] = g_strdup(fullname);
                            close (fd);
                          }
                        else
                          {
                            g_error (strerror (errno));
                          }
			httphelper_free (hh);
		} else {
			g_message("MSP: download aborted: no valid url");
		}
	} else {
		if ( !config->download )
			g_message("MPS: download not allowed !" ) ;
	}
	
	g_free( sounddir );
	g_free( fullname );
	g_free( basename );
	g_free( dirname );

	if ( msp->trigger->n == 0 ) // still don't have a file to play ?
	{
		mdebug (DBG_SOUND, 1, "MSP: No file to play\n" );
		return;
	}
	
	if ( msp->trigger->type == MSP_SOUND ) {
		if ( msp->sound->pid != 0 ) {// there is an active sound ?
			if ( msp->sound->priority < msp->trigger->priority ) {
				msp_stop( msp->sound );
			} else {
				// return if the new sound has a lower priority 
				return;
			}
		}
		temp = msp->sound ; msp->sound = msp->trigger; msp->trigger = temp;
		msp->tsound = g_thread_create( 
			(GThreadFunc)msp_play, msp->sound, TRUE, NULL 
		);
	}
	if ( msp->trigger->type == MSP_MUSIC ) {
		if ( msp->sound->pid != 0 ) {// there is an active sound ?
			if ( strcmp( msp->music->fname,  msp->trigger->fname ) == 0 ) {
				if ( msp->trigger->cont == 1 ) {
					msp->sound->loop = msp->trigger->loop;
					return;
				}
			}
			msp_stop( msp->sound );
		}
		temp = msp->music ; msp->music = msp->trigger; msp->trigger = temp;
		msp->tmusic = g_thread_create( 
			(GThreadFunc)msp_play, msp->music, TRUE, NULL 
		);
	}
}

// the protocol said that the trigger must be on a sigle line
// followed by a CR/LF; return TRUE if msp->buff is a valid trigger
// and fill msp->trigger with values from trigger

gboolean msp_fill_trigger( MSP_INFO *msp ){
	gint state, i, pos, k ;
	
	g_return_val_if_fail( msp->buff != NULL && msp->len > 9, FALSE ); 

	msp_trigger_reset( msp->trigger );

	// get trigger type
	if ( g_str_has_prefix( msp->buff, "!!SOUND(" ) ){
		msp->trigger->type = MSP_SOUND;
		mdebug (DBG_SOUND, 1, "MSP: Trigger is !!SOUND()\n" );
	}
	if ( g_str_has_prefix( msp->buff, "!!MUSIC(" ) ){
		msp->trigger->type = MSP_MUSIC;
		mdebug (DBG_SOUND, 1, "MSP: Trigger is !!MUSIC\n" );
	}

	g_return_val_if_fail( msp->trigger->type != MSP_NULL, FALSE );

	pos = i = 8 ; // we'll start parsing after "("
	state = MSP_STATE_SEP;
	while ( i < msp->len  ) {

		if (msp->buff[i]=='\n') {
			if ( state != MSP_STATE_DONE ) {
				for ( k = 0 ; k < msp->len ; k++ ) printf("%c", msp->buff[k] );
				g_warning( "invalid triger: \\n in the middle of the trigger ");
				return FALSE;
			} else {
				if ( msp->trigger->fname == NULL ) {
					g_warning( "invalid triger: first parameter is NULL");
					return FALSE;
				}
				return TRUE;
			}
		} else {
			switch ( state ) {
				case MSP_STATE_SEP : {
					if ( msp->buff[i] != ' ' ) {
						if ( msp->buff[i] == ')' ) {
							state = MSP_STATE_DONE; 
						} else {
							if ( msp->trigger->fname == NULL ) {
								state = MSP_STATE_FNAME;	
							} else {
								// FIXME check for upper letter
								state = MSP_STATE_PNAME;
							}
							pos = i;
						}
					}
				} break;
				case MSP_STATE_FNAME : {
					if ( msp->buff[i] == ' '  || msp->buff[i] == ')' ) {
						msp->trigger->fname = g_strndup( msp->buff + pos, i - pos ); 
						if ( msp->buff[i] == ' ' )
							state = MSP_STATE_SEP;
						else 
							state = MSP_STATE_DONE;
					}
				} break;
				case MSP_STATE_PNAME : {
					if ( msp->buff[i] == '=' ) {
						state = MSP_STATE_EQUAL;
					} else {
						// invalid parameter 
						g_warning( 
							"invalid trigger: not '=' after param name (%d)", i
						);
						return FALSE;
					}
				} break;
				case MSP_STATE_EQUAL : {
					if ( msp->buff[i] == ' '  || msp->buff[i] == ')' ) {
						if ( i - pos == 2 ) {
							g_warning( 
								"invalid trigger: incomplet parameter (%d)", i 
							);
							return FALSE;
						}
						switch ( msp->buff[pos] ) {
							case 'V' : {
								msp->trigger->volume = atoi( msp->buff+pos+2 );
							} break;
							case 'L' : {
								msp->trigger->loop   = atoi( msp->buff+pos+2);
							} break;
							case 'C' : {
								msp->trigger->cont   = atoi( msp->buff+pos+2);
							} break;
							case 'P' : {
								msp->trigger->priority = atoi( msp->buff+pos+2);
							} break;
							case 'U' : {
								gchar *p,*q;
								msp->trigger->url = g_strndup( 
									msp->buff+pos+2, i - pos - 2 
								); 
								// strip \"
								for ( p = q = msp->trigger->url ; *q ; q++ ) {
									if ( *q != '"' ) *p++ = *q; 
								}
								*p = '\0';
							} break;
							case 'T' : {
								msp->trigger->sound_type = g_strndup( 
									msp->buff+pos+2, i - pos - 2 
								); 
							} break;
						}
						if ( msp->buff[i] == ' ' )
							state = MSP_STATE_SEP;
						else 
							state = MSP_STATE_DONE;
					}
				} break;
			}
		}
		i++;
	}
	g_warning( "invalid triger: not \\n at the end of trigger ");
	return FALSE;
}

void msp_process( MSP_INFO *msp, gchar **buff, gsize *len ){
	gint i,  pos, new_len ;
	gchar *s;
	g_return_if_fail( msp != NULL && buff != NULL && len != NULL );
	if ( *buff == NULL || *len == 0 ) return ;
	pos = i = 0; 
	new_len = *len;
	s = *buff;
	while ( i < new_len ) {
		if ( s[i] == '\n' ) { 
			if ( msp->state == MSP_EP ) {
				// we have a valid trigger line ... fill msp->trigger with it
				msp->buff[msp->len++] = s[i]; // add \n to trigger 
				if ( msp_fill_trigger( msp ) ) {
					msp_trigger_dump( msp->trigger );
					if (msp->link) { // we have a session configuration ?
						if ( ((SESSION_STATE*)msp->link)->sound ) { // allow 
							 msp_handle( msp );
						}
					}
				}
				memmove( s + pos, s + i + 1 , new_len - i - 1 );
				new_len -= ( i - pos + 1 );
				i = pos - 1;
			} 
			//remove trigger and last \n from buffer 
			msp->state = MSP_OK; msp->len = 0; 
			// clear the buffer 
			memset( msp->buff, 0, MSPBUFFSIZE );
		} else 
			switch ( msp->state ) {
				case MSP_OK :  {
					if ( s[i] == '!' ) {
						msp->state = MSP_EX1;
						msp->buff[msp->len++] = s[i]; 
						pos = i;
					} else {
						msp->state = MSP_NOTOK;
					}
				} break;
				case MSP_EX1 : {
					if ( s[i] == '!' ) {	
						msp->state = MSP_EX2;
						msp->buff[msp->len++] = s[i]; 
					}
				} break;
				case MSP_EX2 : {
					switch( s[i] ) {
						case 'S' : {
							msp->state = MSP_SN1;
							msp->buff[msp->len++] = s[i]; 
						} break;
						case 'M' : {
							msp->state = MSP_MS1;
							msp->buff[msp->len++] = s[i]; 
						} break;
						default : {
							msp->state = MSP_NOTOK;
							msp->len = 0;
						}
					}
				} break;
				case MSP_SN1 : {
					if ( s[i] == 'O' ) {
						msp->state = MSP_SN2; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_SN2 : {
					if ( s[i] == 'U' ) {
						msp->state = MSP_SN3; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_SN3 : {
					if ( s[i] == 'N' ) {
						msp->state = MSP_SN4; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_SN4 : {
					if ( s[i] == 'D' ) {
						msp->state = MSP_SN5; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_SN5 : {
					if ( s[i] == '(' ) {
						msp->state = MSP_BP; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_MS1 : {
					if ( s[i] == 'U' ) {
						msp->state = MSP_MS2; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_MS2 : {
					if ( s[i] == 'S' ) {
						msp->state = MSP_MS3; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_MS3 : {
					if ( s[i] == 'I' ) {
						msp->state = MSP_MS4; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_MS4 : {
					if ( s[i] == 'C' ) {
						msp->state = MSP_MS5; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_MS5 : {
					if ( s[i] == '(' ) {
						msp->state = MSP_BP; msp->buff[msp->len++] = s[i]; 
					} else {
						msp->state = MSP_NOTOK; msp->len = 0;
					}
				} break;
				case MSP_BP : {
					if ( s[i] == ')' ) {
						msp->state = MSP_EP; 
					} 
					msp->buff[msp->len++] = s[i]; 
				} break;
			}
			i++;
			if ( msp->len == MSPBUFFSIZE ) { // this should never happen
				g_error("sound trigger too long");	
			}
	}
	if ( msp->state != MSP_OK && msp->state != MSP_NOTOK ) // incomplet trigger
	{
		new_len = pos;
		mdebug (DBG_SOUND, 1, "MSP: msp_process() msp state is odd: %d:\n", msp->state );
	}
	memset( s + new_len, 0, *len - new_len );
	*len = new_len;
}
