/*
 * gtkgame.h
 *
 * by Gary Wong <gtw@gnu.org>, 2000, 2002.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 */

#ifndef _GTKGAME_H_
#define _GTKGAME_H_

#include "backgammon.h"
#include "rollout.h"

typedef enum _dialogarea {
    DA_MAIN,
    DA_BUTTONS
} dialogarea;

extern GtkWidget *pwMain, *pwMenuBar;

extern int fEndDelay;

extern void ShowGameWindow( void );

extern void GTKAddMoveRecord( moverecord *pmr );
extern void GTKPopMoveRecord( moverecord *pmr );
extern void GTKSetMoveRecord( moverecord *pmr );
extern void GTKClearMoveRecord( void );
#if WIN32
extern void GTKAddGame( char *sz );
#else
extern void GTKAddGame( moverecord *pmr );
#endif
extern void GTKPopGame( int c );
extern void GTKSetGame( int i );
extern void GTKRegenerateGames( void );

extern void GTKFreeze( void );
extern void GTKThaw( void );

extern int InitGTK( int *argc, char ***argv );
extern void RunGTK( void );
extern void GTKAllowStdin( void );
extern void GTKDisallowStdin( void );
extern void GTKDelay( void );
extern void ShowList( char *asz[], char *szTitle );

extern GtkWidget *CreateDialog( char *szTitle, int fQuestion, GtkSignalFunc pf,
				void *p );
extern GtkWidget *DialogArea( GtkWidget *pw, dialogarea da );
    
extern int GTKGetInputYN( char *szPrompt );
extern void GTKOutput( char *sz );
extern void GTKOutputX( void );
extern void GTKOutputNew( void );
extern void GTKProgressStart( char *sz );
extern void GTKProgress( void );
extern void GTKProgressEnd( void );
extern void
GTKProgressStartValue( char *sz, int iMax );
extern void
GTKProgressValue ( int fValue );
extern void GTKBearoffProgress( int i );

extern void GTKDumpStatcontext( statcontext *psc, char *szTitle );
extern void GTKEval( char *szOutput );
extern void GTKHint( movelist *pml );
extern void GTKDoubleHint( char *sz );
extern void GTKTakeHint( float arDouble[], int fMWC, int fBeaver,
			 cubeinfo *pci );

extern void
GTKRollout( int c, char asz[][ 40 ], int cGames,
	    rolloutstat ars[][ 2 ] );

extern void GTKRolloutRow( int i );

extern int
GTKRolloutUpdate( float aarMu[][ NUM_ROLLOUT_OUTPUTS ],
                  float aarSigma[][ NUM_ROLLOUT_OUTPUTS ],
                  int iGame, int cGames, int fCubeful, int cRows,
                  cubeinfo aci[] );

extern void GTKRolloutDone( void );
extern void GTKSet( void *p );
extern void GTKUpdateAnnotations( void );
extern void GTKShowMatchEquityTable( int n );
extern int GTKGetManualDice( int an[ 2 ] );
extern void GTKShowVersion( void );
extern void GTKDumpRolloutResults(GtkWidget *widget, gpointer data);
extern void GTKViewRolloutStatistics(GtkWidget *widget, gpointer data);
#ifdef WIN32
extern void GTKWinCopy( GtkWidget *widget, gpointer data);
#endif
extern void
GTKResignHint( float arOutput[], float rEqBefore, float rEqAfter,
               cubeinfo *pci, int fMWC );
extern void GTKSaveSettings( void );

extern int fTTY;

#endif
