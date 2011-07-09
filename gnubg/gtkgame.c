/*
 * gtkgame.c
 *
 * by Gary Wong <gtw@gnu.org>, 2000, 2001, 2002.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 or later of the GNU General Public License as
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

#include "config.h"
#undef GTK_DISABLE_DEPRECATED
#undef GSEAL_ENABLE

#include <glib.h>
#include <ctype.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#ifdef WIN32
#include <io.h>
#endif

#include "analysis.h"
#include "backgammon.h"
#include "sgf.h"
#include "dice.h"
#include "drawboard.h"
#include "gtkboard.h"
#include "gtkchequer.h"
#include "gtkcube.h"
#include "gtkgame.h"
#include "gtkmet.h"
#include "gtkmovefilter.h"
#include "gtkprefs.h"
#include "gtksplash.h"
#include "gtkrelational.h"
#include "gtkfile.h"
#include "matchequity.h"
#include "openurl.h"
#include "positionid.h"
#include "sound.h"
#include "gtkoptions.h"
#include "gtktoolbar.h"
#include "format.h"
#include "formatgs.h"
#include "renderprefs.h"
#include "credits.h"
#include "matchid.h"
#include "gtkwindows.h"
#include "export.h"
#include "gtkmovelistctrl.h"
#include "rollout.h"
#include "util.h"
#if USE_BOARD3D
#include "fun3d.h"
#endif
#include "gnubgstock.h"

#define KEY_ESCAPE -229

/* Offset action to avoid predefined values */
#define MENU_OFFSET 50

char *newLang;

/* Hack this for now to stop re-entering - should be fixed when menu switched to actions */
int inCallback = FALSE;

/* Enumeration to be used as index to the table of command strings below
   (since GTK will only let us put integers into a GtkItemFactoryEntry,
   and that might not be big enough to hold a pointer).  Must be kept in
   sync with the string array! */
typedef enum _gnubgcommand {
	CMD_ACCEPT,
    CMD_ANALYSE_CLEAR_MOVE,
    CMD_ANALYSE_CLEAR_GAME,
    CMD_ANALYSE_CLEAR_MATCH,
    CMD_ANALYSE_MOVE,
    CMD_ANALYSE_GAME,
    CMD_ANALYSE_MATCH,
    CMD_ANALYSE_ROLLOUT_CUBE,
    CMD_ANALYSE_ROLLOUT_MOVE,
    CMD_ANALYSE_ROLLOUT_GAME,
    CMD_ANALYSE_ROLLOUT_MATCH,
    CMD_CLEAR_TURN,
    CMD_CMARK_CUBE_CLEAR,
    CMD_CMARK_CUBE_SHOW,
    CMD_CMARK_MOVE_CLEAR,
    CMD_CMARK_MOVE_SHOW,
    CMD_CMARK_GAME_CLEAR,
    CMD_CMARK_GAME_SHOW,
    CMD_CMARK_MATCH_CLEAR,
    CMD_CMARK_MATCH_SHOW,
    CMD_END_GAME,
    CMD_DECLINE,
    CMD_DOUBLE,
    CMD_EVAL,
    CMD_HELP,
    CMD_HINT,
    CMD_LIST_GAME,
    CMD_NEXT,
    CMD_NEXT_GAME,
    CMD_NEXT_MARKED,
    CMD_NEXT_CMARKED,
    CMD_NEXT_ROLL,
    CMD_NEXT_ROLLED,
    CMD_PLAY,
    CMD_PREV,
    CMD_PREV_GAME,
    CMD_PREV_MARKED,
    CMD_PREV_CMARKED,
    CMD_PREV_ROLL,
    CMD_PREV_ROLLED,
    CMD_QUIT,
    CMD_REJECT,
    CMD_RELATIONAL_ADD_MATCH,
    CMD_ROLL,
    CMD_ROLLOUT,
    CMD_SAVE_SETTINGS,
    CMD_SET_ANNOTATION_ON,
    CMD_SET_APPEARANCE,
    CMD_SET_MESSAGE_ON,
    CMD_SET_TURN_0,
    CMD_SET_TURN_1,
    CMD_SHOW_CALIBRATION,
    CMD_SHOW_COPYING,
    CMD_SHOW_ENGINE,
    CMD_SHOW_EXPORT,
    CMD_SHOW_MARKETWINDOW,
    CMD_SHOW_MATCHEQUITYTABLE,
    CMD_SHOW_KLEINMAN,
    CMD_SHOW_MANUAL_ABOUT,
    CMD_SHOW_MANUAL_WEB,
    CMD_SHOW_ROLLS,
    CMD_SHOW_STATISTICS_MATCH,
    CMD_SHOW_TEMPERATURE_MAP,
    CMD_SHOW_TEMPERATURE_MAP_CUBE,
    CMD_SHOW_VERSION,
    CMD_SHOW_WARRANTY,
    CMD_SWAP_PLAYERS,
    NUM_CMDS
} gnubgcommand;

static const char *aszCommands[ NUM_CMDS ] = {
	"accept",
    "analyse clear move",
    "analyse clear game",
    "analyse clear match",
    "analyse move",
    "analyse game",
    "analyse match",
    "analyse rollout cube",
    "analyse rollout move",
    "analyse rollout game",
    "analyse rollout match",
    "clear turn",
    "cmark cube clear",
    "cmark cube show",
    "cmark move clear",
    "cmark move show",
    "cmark game clear",
    "cmark game show",
    "cmark match clear",
    "cmark match show",
    "end game",
    "decline",
    "double",
    "eval",
    "help",
    "hint",
    "list game",
    "next",
    "next game",
    "next marked",
    "next cmarked",
    "next roll",
	"next rolled",
    "play",
    "previous",
    "previous game",
    "previous marked",
    "previous cmarked",
    "previous roll",
	"previous rolled",
    "quit",
    "reject",
    "relational add match",
    "roll",
    "rollout",
    "save settings",
    "set annotation on",
    NULL, /* set appearance */
    "set message on",
    NULL, /* set turn 0 */
    NULL, /* set turn 1 */
    "show calibration",
    "show copying",
    "show engine",
    "show export",
    "show marketwindow",
    "show matchequitytable",
    "show kleinman", /* opens race theory window */
    "show manual about",
    "show manual web",
    "show rolls",
    "show statistics match",
    "show temperaturemap",
    "show temperaturemap =cube",
    "show version",
    "show warranty",
    "swap players",
};
enum { TOGGLE_GAMELIST = NUM_CMDS + 1, TOGGLE_ANALYSIS, TOGGLE_COMMENTARY, TOGGLE_MESSAGE, TOGGLE_THEORY, TOGGLE_COMMAND };

typedef struct _analysiswidget {

  evalsetup esChequer;
  evalsetup esCube; 
  movefilter aamf[ MAX_FILTER_PLIES ][ MAX_FILTER_PLIES ];

  evalsetup esEvalChequer;
  evalsetup esEvalCube; 
  movefilter aaEvalmf[ MAX_FILTER_PLIES ][ MAX_FILTER_PLIES ];

  GtkAdjustment *apadjSkill[3], *apadjLuck[4];
  GtkWidget *pwMoves, *pwCube, *pwLuck, *pwHintSame, *pwCubeSummary;
  GtkWidget *apwAnalysePlayers[ 2 ];
    
} analysiswidget;

/* A dummy widget that can grab events when others shouldn't see them. */
GtkWidget *pwGrab;
GtkWidget *pwOldGrab;

GtkWidget *pwBoard;
GtkWidget *pwMain = NULL;
GtkWidget *pwMenuBar;
GtkWidget *pwToolbar;

static GtkWidget *pwStatus;
GtkWidget *pwIDBox, *pwGnubgID;
static GtkWidget *pwProgress;
GtkWidget *pwMessageText;
GtkWidget *pwPanelVbox;
GtkWidget *pwAnalysis;
GtkWidget *pwCommentary;
static moverecord *pmrAnnotation;
GtkAccelGroup *pagMain;
#if (GTK_MAJOR_VERSION < 3) && (GTK_MINOR_VERSION < 12)
GtkTooltips *ptt;
#endif
GtkItemFactory *pif;
guint nNextTurn = 0; /* GTK idle function */
static guint idOutput, idProgress;
int fTTY = TRUE;
int fGUISetWindowPos = TRUE;
int frozen = FALSE;
static GString *output_str = NULL;
static int fullScreenOnStartup = FALSE;

static guint nStdin, nDisabledCount = 1;

/* Save state of windows for full screen */
int showingPanels, showingIDs, maximised;


static int grabIdSignal;
static int suspendCount = 0;
static GtkWidget* grabbedWidget;

/* Language selection code */
static GtkWidget *curSel;
static GtkWidget *pwLangDialog, *pwLangRadio1, *pwLangRadio2, *pwLangTable;

static GtkWidget *pwHelpTree, *pwHelpLabel;

GtkWidget *hpaned;
static GtkWidget *pwGameBox;
static GtkWidget *pwPanelGameBox;
static GtkWidget *pwEventBox;
static int panelSize = 325;
static GtkWidget *pwStop;

extern void GTKSuspendInput(void)
{
	if (!fX)
		return;

	if (suspendCount == 0)
	{	/* Grab events so that the board window knows this is a re-entrant
		call, and won't allow commands like roll, move or double. */
		grabbedWidget = pwGrab;
		if (pwGrab == pwStop)
		{
			gtk_widget_set_sensitive(pwStop, TRUE);
			gtk_widget_grab_focus(pwStop);
		}
		gtk_grab_add(pwGrab);
		grabIdSignal = g_signal_connect_after(G_OBJECT(pwGrab),
				"key-press-event", G_CALLBACK(gtk_true), NULL);
	}

	/* Don't check stdin here; readline isn't ready yet. */
	GTKDisallowStdin();
	suspendCount++;
}

extern void GTKResumeInput(void)
{
	if (!fX)
		return;
	g_assert(suspendCount > 0);
	suspendCount--;
	if (suspendCount == 0)
	{
		if (GTK_IS_WIDGET(grabbedWidget) && GTK_WIDGET_HAS_GRAB(grabbedWidget))
		{
			if (g_signal_handler_is_connected (G_OBJECT(grabbedWidget), grabIdSignal))
				g_signal_handler_disconnect (G_OBJECT(grabbedWidget), grabIdSignal);
			gtk_grab_remove(grabbedWidget);
		}
		if (pwGrab == pwStop)
			gtk_widget_set_sensitive(pwStop, FALSE);
	}

	GTKAllowStdin();
}

static void StdinReadNotify( gpointer p, gint h, GdkInputCondition cond ) {
    
    char sz[ 2048 ], *pch;
    
#if HAVE_LIBREADLINE
    /* Handle "next turn" processing before more input (otherwise we might
       not even have a readline handler installed!) */
    while( nNextTurn )
            NextTurnNotify( NULL );

    rl_callback_read_char();

    return;
#endif

    while( nNextTurn )
	NextTurnNotify( NULL );
    
    if (fgets( sz, sizeof( sz ), stdin ) == NULL)
    {
	if( !isatty( STDIN_FILENO ) )
	    exit( EXIT_SUCCESS );
	
	PromptForExit();
	return;
    }

    if( ( pch = strchr( sz, '\n' ) ) )
	*pch = 0;
    
	
    fInterrupt = FALSE;

    HandleCommand( sz, acTop );

    ResetInterrupt();

    if( nNextTurn )
	fNeedPrompt = TRUE;
    else
	Prompt();
}


extern void GTKAllowStdin( void ) {

    if( !fTTY || !nDisabledCount )
	return;
    
    if( !--nDisabledCount )
	nStdin = gtk_input_add_full( STDIN_FILENO, GDK_INPUT_READ,
				     StdinReadNotify, NULL, NULL, NULL );
}

extern void GTKDisallowStdin( void ) {

    if( !fTTY )
	return;
    
    nDisabledCount++;
    
    if( nStdin ) {
	gtk_input_remove( nStdin );
	nStdin = 0;
    }
}

int fEndDelay;

extern void GTKDelay( void ) {

    GTKSuspendInput();
    
    while( !fInterrupt && !fEndDelay )
	gtk_main_iteration();
    
    fEndDelay = FALSE;
    
    GTKResumeInput();
}

/* TRUE if gnubg is automatically setting the state of a menu item. */
static int fAutoCommand;

static void Command( gpointer p, guint iCommand, GtkWidget *widget ) {

    char sz[ 80 ];

    if( fAutoCommand )
	return;

    /* FIXME this isn't very good -- if UserCommand fails, the setting
       won't have been changed, but the widget will automatically have
       updated itself. */
    
    if( GTK_IS_RADIO_MENU_ITEM( widget ) ) {
	if( !GTK_CHECK_MENU_ITEM( widget )->active )
	    return;
    } else if( GTK_IS_CHECK_MENU_ITEM( widget ) ) {
	sprintf( sz, "%s %s", aszCommands[ iCommand ],
		 GTK_CHECK_MENU_ITEM( widget )->active ? "on" : "off" );
	UserCommand( sz );
	
	return;
    }

    switch( iCommand ) {
    case CMD_SET_APPEARANCE:
	BoardPreferences( pwBoard );
	return;
	
    case CMD_SET_TURN_0:
    case CMD_SET_TURN_1:
	sprintf( sz, "set turn %s",
		 ap[ iCommand - CMD_SET_TURN_0 ].szName );
	UserCommand( sz );
	return;	
	

    default:
	UserCommand( aszCommands[ iCommand ] );
    }
}

static void gui_clear_turn(GtkWidget *pw, GtkWidget *dialog)
{
	if (dialog)
		gtk_widget_destroy(dialog);
	CommandClearTurn(NULL);
}

extern int GTKGetManualDice(unsigned int an[2])
{
	GtkWidget *dialog;
	GtkWidget *dice;
	GtkWidget *buttons;
	GtkWidget *clear;
	BoardData *bd = BOARD( pwBoard )->board_data;

	manualDiceType mdt;
	if (ToolbarIsEditing(pwToolbar))
		mdt = MT_EDIT;
	else if (plLastMove && ((moverecord*)plLastMove->p)->mt == MOVE_GAMEINFO)
		mdt = MT_FIRSTMOVE;
	else
		mdt = MT_STANDARD;

	dialog = GTKCreateDialog(_("GNU Backgammon - Dice"), DT_INFO, NULL,
			    DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL);
	dice = board_dice_widget(BOARD(pwBoard), mdt);

	gtk_container_add(GTK_CONTAINER(DialogArea(dialog, DA_MAIN)), dice);

	buttons = DialogArea(dialog, DA_BUTTONS);
	clear = gtk_button_new_with_label(_("Clear Dice"));
	gtk_container_add(GTK_CONTAINER(buttons), clear);
	g_signal_connect(G_OBJECT(clear), "clicked", G_CALLBACK(gui_clear_turn), dialog);
	gtk_widget_set_sensitive(GTK_WIDGET(clear), bd->diceShown == DICE_ON_BOARD);

	g_object_set_data(G_OBJECT(dice), "user_data", an);
	an[0] = 0;

	GTKRunDialog(dialog);

	if (mdt == MT_EDIT && an[0])
	{
		if (an[0] > an[1] && bd->turn != -1)
			UserCommand( "set turn 0");
		else if (an[0] < an[1] && bd->turn != 1)
			UserCommand( "set turn 1");
	}

	return an[0] ? 0 : -1;
}

extern void GTKSetDice( gpointer p, guint n, GtkWidget *pw ) {

    unsigned int an[ 2 ];
    char sz[ 13 ]; /* "set dice x y" */

    if( !GTKGetManualDice( an ) ) {
	sprintf( sz, "set dice %d %d", an[ 0 ], an[ 1 ] );
	UserCommand( sz );
    }
}

extern void GTKSetCube( gpointer p, guint n, GtkWidget *pw ) {

    int valChanged;
    int an[ 2 ];
    char sz[ 20 ]; /* "set cube value 4096" */
    GtkWidget *pwDialog, *pwCube;

    if( ms.gs != GAME_PLAYING || ms.fCrawford || !ms.fCubeUse )
	return;
	
    pwDialog = GTKCreateDialog( _("GNU Backgammon - Cube"), DT_INFO,
			NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL );
    pwCube = board_cube_widget( BOARD( pwBoard ) );

    an[ 0 ] = -1;
    
    gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
		       pwCube );
    g_object_set_data( G_OBJECT( pwCube ), "user_data", an );
    
    g_signal_connect( G_OBJECT( pwCube ), "destroy",
			G_CALLBACK( DestroySetCube ), pwDialog );
    
	GTKRunDialog(pwDialog);

    if( an[ 0 ] < 0 )
	return;

    valChanged = (1 << an[ 0 ] != ms.nCube);
    if (valChanged) {
	sprintf( sz, "set cube value %d", 1 << an[ 0 ] );
	UserCommand( sz );
    }

    if( an[ 1 ] != ms.fCubeOwner ) {
	if( an[ 1 ] >= 0 ) {
	    sprintf( sz, "set cube owner %d", an[ 1 ] );
	    UserCommand( sz );
	} else
	    UserCommand( "set cube centre" );
    }
}

static int fAutoCommentaryChange;

extern void CommentaryChanged( GtkWidget *pw, GtkTextBuffer *buffer ) {

    char *pch;
    GtkTextIter begin, end;
    
    if( fAutoCommentaryChange )
	return;

    g_assert( pmrAnnotation );

    /* FIXME Copying the entire text every time it's changed is horribly
       inefficient, but the only alternatives seem to be lazy copying
       (which is much harder to get right) or requiring a specific command
       to update the text (which is probably inconvenient for the user). */

    if( pmrAnnotation->sz )
	g_free( pmrAnnotation->sz );
    
    gtk_text_buffer_get_bounds (buffer, &begin, &end);
    pch = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
	/* This copy is absolutely disgusting, but is necessary because GTK
	   insists on giving us something allocated with g_malloc() instead
	   of malloc(). */
	pmrAnnotation->sz = g_strdup( pch );
	g_free( pch );
}

extern void GTKFreeze( void ) {

	GL_Freeze();
	frozen = TRUE;
}

extern void GTKThaw( void ) {

	GL_Thaw();
	frozen = FALSE;
	/* Make sure analysis window is correct */
	if (plLastMove)
		GTKSetMoveRecord( plLastMove->p );
}

static GtkWidget *
ResignAnalysis ( float arResign[ NUM_ROLLOUT_OUTPUTS ],
                 int nResigned,
                 evalsetup *pesResign ) {

  cubeinfo ci;
  GtkWidget *pwTable = gtk_table_new ( 3, 2, FALSE );
  GtkWidget *pwLabel;

  float rAfter, rBefore;

  char sz [ 64 ];

  if ( pesResign->et == EVAL_NONE ) 
    return NULL;

  GetMatchStateCubeInfo ( &ci, &ms );


  /* First column with text */

  pwLabel = gtk_label_new ( _("Equity before resignation: ") );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 0, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     0, 1, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );

  pwLabel = gtk_label_new ( _("Equity after resignation: ") );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 0, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     0, 1, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );

  pwLabel = gtk_label_new ( _("Difference: ") );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 0, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     0, 1, 2, 3,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );

  /* Second column: equities/mwc */

  getResignEquities ( arResign, &ci, nResigned, &rBefore, &rAfter );

  if( fOutputMWC && ms.nMatchTo )
    sprintf ( sz, "%6.2f%%", eq2mwc ( rBefore, &ci ) * 100.0f );
  else
    sprintf ( sz, "%+6.3f", rBefore );

  pwLabel = gtk_label_new ( sz );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 1, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     1, 2, 0, 1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );


  if( fOutputMWC && ms.nMatchTo )
    sprintf ( sz, "%6.2f%%", eq2mwc ( rAfter, &ci ) * 100.0f );
  else
    sprintf ( sz, "%+6.3f", rAfter );

  pwLabel = gtk_label_new ( sz );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 1, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     1, 2, 1, 2,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );


  if( fOutputMWC && ms.nMatchTo )
    sprintf ( sz, "%+6.2f%%",
              ( eq2mwc ( rAfter, &ci ) - eq2mwc ( rBefore, &ci ) ) * 100.0f );
  else
    sprintf ( sz, "%+6.3f", rAfter - rBefore );

  pwLabel = gtk_label_new ( sz );
  gtk_misc_set_alignment( GTK_MISC( pwLabel ), 1, 0.5 );
  gtk_table_attach ( GTK_TABLE ( pwTable ),
                     pwLabel,
                     1, 2, 2, 3,
                     GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 
                     8, 2 );


  return pwTable;

}



GtkWidget *pwMoveAnalysis = NULL;

static GtkWidget *luck_label(lucktype lt)
{
	GtkWidget *label;
	gchar *markup;
	const gchar *skill;
	gchar *color[N_LUCKS] = {"red", "orange", "black", "green", "white"}; 

	label = gtk_label_new(NULL);
	skill = aszLuckType[lt] ? gettext(aszLuckType[lt]) : "";
	markup = g_strdup_printf("<span foreground=\"%s\" background=\"black\" weight=\"bold\">%s</span>", color[lt], skill);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	return label;
}

static GtkWidget *skill_label(skilltype st)
{
	GtkWidget *label;
	gchar *markup;
	const gchar *skill;
	gchar *color[] = {"red", "orange", "yellow", "black"}; 

	label = gtk_label_new(NULL);
	skill = aszSkillType[st] ? gettext(aszSkillType[st]) : "";
	markup = g_strdup_printf("<span foreground=\"%s\" background=\"black\" weight=\"bold\">%s</span>", color[st], skill);
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free(markup);
	return label;
}

extern void SetAnnotation( moverecord *pmr) {

    GtkWidget *pwParent = pwAnalysis->parent, *pw = NULL, *pwBox, *pwAlign;
    int fMoveOld, fTurnOld;
    listOLD *pl;
    char sz[ 64 ];
    GtkWidget *pwCubeAnalysis = NULL;
    doubletype dt;
    taketype tt;
    cubeinfo ci;
    GtkTextBuffer *buffer;
    pwMoveAnalysis = NULL;

    /* Select the moverecord _after_ pmr.  FIXME this is very ugly! */
    pmrCurAnn = pmr;

    for( pl = plGame->plNext; pl != plGame; pl = pl->plNext )
	if( pl->p == pmr ) {
	    pmr = pl->plNext->p;
	    break;
	}

    if( pl == plGame )
	pmr = NULL;

    pmrAnnotation = pmr;

    /* FIXME optimise by ignoring set if pmr is unchanged */
    
    if( pwAnalysis ) {
	gtk_widget_destroy( pwAnalysis );
	pwAnalysis = NULL;
    }

    gtk_widget_set_sensitive( pwCommentary, pmr != NULL );
    fAutoCommentaryChange = TRUE;
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(pwCommentary));
    gtk_text_buffer_set_text (buffer, "", -1);
    fAutoCommentaryChange = FALSE;
    
    if( pmr ) {
	if( pmr->sz ) {
	    fAutoCommentaryChange = TRUE;
	    gtk_text_buffer_set_text (buffer, pmr->sz, -1);
	    fAutoCommentaryChange = FALSE;
	}

	switch( pmr->mt ) {
	case MOVE_NORMAL:
	    fMoveOld = ms.fMove;
	    fTurnOld = ms.fTurn;

	    pwAnalysis = gtk_vbox_new( FALSE, 0 );

	    pwBox = gtk_table_new( 2, 3, FALSE );
	    gtk_box_pack_start( GTK_BOX( pwAnalysis ), pwBox, FALSE, FALSE,
				4 );

	    ms.fMove = ms.fTurn = pmr->fPlayer;

            /* 
             * Skill and luck
             */

            /* Skill for cube */

            GetMatchStateCubeInfo ( &ci, &ms );
            if ( GetDPEq ( NULL, NULL, &ci ) ) {
              gtk_table_attach_defaults( GTK_TABLE ( pwBox ),
                                   gtk_label_new ( _("Didn't double") ),
                                   0, 1, 0, 1 );
              gtk_table_attach_defaults( GTK_TABLE ( pwBox ),
                                   skill_label(pmr->stCube),
                                   0, 1, 1, 2 );
            }

            /* luck */

	{
    char sz[ 64 ], *pch;
    cubeinfo ci;
    
    pch = sz + sprintf( sz, _("Rolled %d%d"), pmr->anDice[0], pmr->anDice[1] );
    
    if( pmr->rLuck != ERR_VAL ) {
	if( fOutputMWC && ms.nMatchTo ) {
	    GetMatchStateCubeInfo( &ci, &ms );
	    pch += sprintf( pch, " (%+0.3f%%)",
	     100.0f * ( eq2mwc( pmr->rLuck, &ci ) - eq2mwc( 0.0f, &ci ) ) );
	} else
	    pch += sprintf( pch, " (%+0.3f)", pmr->rLuck );
    }
    gtk_table_attach_defaults( GTK_TABLE( pwBox ),
				gtk_label_new( sz ), 1, 2, 0, 1 );
    gtk_table_attach_defaults( GTK_TABLE( pwBox ), luck_label(pmr->lt), 1, 2, 1, 2 );

	}

            /* chequer play skill */
	    strcpy( sz, _("Moved ") );
	    FormatMove( sz + strlen(_("Moved ")), msBoard(), pmr->n.anMove );

	    gtk_table_attach_defaults( GTK_TABLE( pwBox ),
			      gtk_label_new( sz ), 
			      2, 3, 0, 1 );

	    gtk_table_attach_defaults( GTK_TABLE( pwBox ), 
			    skill_label(pmr->n.stMove),
			      2, 3, 1, 2 );


            /* cube */

            pwCubeAnalysis = CreateCubeAnalysis( pmr, &ms, FALSE, -1, TRUE );


            /* move */
			      
	    if( pmr->ml.cMoves ) 
              pwMoveAnalysis = CreateMoveList( pmr,
                                               TRUE, FALSE, !IsPanelDocked(WINDOW_ANALYSIS), TRUE);

            if ( pwMoveAnalysis && pwCubeAnalysis ) {
              /* notebook with analysis */
              pw = gtk_notebook_new ();

              gtk_box_pack_start ( GTK_BOX ( pwAnalysis ), pw, TRUE, TRUE, 0 );

              gtk_notebook_append_page ( GTK_NOTEBOOK ( pw ),
                                         pwMoveAnalysis,
                                         gtk_label_new ( _("Chequer play") ) );

              gtk_notebook_append_page ( GTK_NOTEBOOK ( pw ),
                                         pwCubeAnalysis,
                                         gtk_label_new ( _("Cube decision") ) );


            }
            else if ( pwMoveAnalysis )
			{
				if (IsPanelDocked(WINDOW_ANALYSIS))
					gtk_widget_set_size_request(GTK_WIDGET(pwMoveAnalysis), 0, 200);

				gtk_box_pack_start ( GTK_BOX ( pwAnalysis ),
                                   pwMoveAnalysis, TRUE, TRUE, 0 );
			}
            else if ( pwCubeAnalysis )
              gtk_box_pack_start ( GTK_BOX ( pwAnalysis ),
                                   pwCubeAnalysis, TRUE, TRUE, 0 );


	    if( !g_list_first( GTK_BOX( pwAnalysis )->children ) ) {
		gtk_widget_destroy( pwAnalysis );
		pwAnalysis = NULL;
	    }
		
	    ms.fMove = fMoveOld;
	    ms.fTurn = fTurnOld;
	    break;

	case MOVE_DOUBLE:

            dt = DoubleType ( ms.fDoubled, ms.fMove, ms.fTurn );

	    pwAnalysis = gtk_vbox_new( FALSE, 0 );

	    pwBox = gtk_hbox_new( FALSE, 0 );
	    gtk_box_pack_start( GTK_BOX( pwBox ), 
                                gtk_label_new( 
                                   Q_ ( aszDoubleTypes[ dt ] ) ),
				FALSE, FALSE, 2 );
	    gtk_box_pack_start( GTK_BOX( pwBox ), 
                                   skill_label(pmr->stCube),
				FALSE, FALSE, 2 );
	    gtk_box_pack_start( GTK_BOX( pwAnalysis ), pwBox, FALSE, FALSE,
				0 );

            if ( dt == DT_NORMAL || dt == DT_BEAVER ) {
	    
              if ( ( pw = CreateCubeAnalysis( pmr, &ms, TRUE, -1, TRUE ) ) )
		gtk_box_pack_start( GTK_BOX( pwAnalysis ), pw, FALSE,
				    FALSE, 0 );

            }
            else 
              gtk_box_pack_start ( GTK_BOX ( pwAnalysis ),
                                   gtk_label_new ( _("GNU Backgammon cannot "
                                                     "analyse neither beavers "
                                                     "nor raccoons yet") ),
                                   FALSE, FALSE, 0 );

	    break;

	case MOVE_TAKE:
	case MOVE_DROP:

            tt = (taketype) DoubleType ( ms.fDoubled, ms.fMove, ms.fTurn );

	    pwAnalysis = gtk_vbox_new( FALSE, 0 );

	    pwBox = gtk_vbox_new( FALSE, 0 );
	    gtk_box_pack_start( GTK_BOX( pwBox ),
				gtk_label_new( pmr->mt == MOVE_TAKE ? _("Take") :
				    _("Drop") ), FALSE, FALSE, 2 );
	    gtk_box_pack_start( GTK_BOX( pwBox ), skill_label(pmr->stCube),
				FALSE, FALSE, 2 );
	    gtk_box_pack_start( GTK_BOX( pwAnalysis ), pwBox, FALSE, FALSE,
				0 );

            if ( tt <= TT_NORMAL ) {
              if ( ( pw = CreateCubeAnalysis( pmr, &ms, -1, pmr->mt == MOVE_TAKE, TRUE ) ) )
		gtk_box_pack_start( GTK_BOX( pwAnalysis ), pw, FALSE,
				    FALSE, 0 );
            }
            else
              gtk_box_pack_start ( GTK_BOX ( pwAnalysis ),
                                   gtk_label_new ( _("GNU Backgammon cannot "
                                                     "analyse neither beavers "
                                                     "nor raccoons yet") ),
                                   FALSE, FALSE, 0 );

	    break;
	    
	case MOVE_RESIGN:
	    pwAnalysis = gtk_vbox_new( FALSE, 0 );

            /* equities for resign */
            
	    if( ( pw = ResignAnalysis( pmr->r.arResign,
                                       pmr->r.nResigned,
                                       &pmr->r.esResign ) ) )
              gtk_box_pack_start( GTK_BOX( pwAnalysis ), pw, FALSE,
                                  FALSE, 0 );

            /* skill for resignation */

	    pwBox = gtk_hbox_new( FALSE, 0 );
	    gtk_box_pack_start( GTK_BOX( pwBox ),
				gtk_label_new( _("Resign") ),
				FALSE, FALSE, 2 );

	    pwAlign = gtk_alignment_new( 0.5f, 0.5f, 0.0f, 0.0f );
	    gtk_box_pack_start( GTK_BOX( pwAnalysis ), pwAlign, FALSE, FALSE,
				0 );

	    gtk_container_add( GTK_CONTAINER( pwAlign ), pwBox );

            /* skill for accept */

	    pwBox = gtk_hbox_new( FALSE, 0 );
	    gtk_box_pack_start( GTK_BOX( pwBox ),
				gtk_label_new( _("Accept") ),
				FALSE, FALSE, 2 );

	    pwAlign = gtk_alignment_new( 0.5f, 0.5f, 0.0f, 0.0f );
	    gtk_box_pack_start( GTK_BOX( pwAnalysis ), pwAlign, FALSE, FALSE,
				0 );

	    gtk_container_add( GTK_CONTAINER( pwAlign ), pwBox );
	    
	    break;
	    
	default:
	    break;
	}
    }
	
    if( !pwAnalysis )
	pwAnalysis = gtk_label_new( _("No analysis available.") );
    
	if (!IsPanelDocked(WINDOW_ANALYSIS))
		gtk_paned_pack1( GTK_PANED( pwParent ), pwAnalysis, TRUE, FALSE );
	else
		gtk_box_pack_start( GTK_BOX( pwParent ), pwAnalysis, TRUE, TRUE, 0 );

	gtk_widget_show_all( pwAnalysis );


    if ( pmr && pmr->mt == MOVE_NORMAL && pwMoveAnalysis && pwCubeAnalysis ) {

      if ( badSkill(pmr->stCube) )
        gtk_notebook_set_current_page ( GTK_NOTEBOOK ( pw ), 1 );


    }
}

extern void GTKSaveSettings( void ) {

    char *sz = g_build_filename(szHomeDirectory, "gnubgmenurc", NULL);
    gtk_accel_map_save( sz );
    g_free(sz);
}

static gboolean main_delete( GtkWidget *pw ) {

    getWindowGeometry(WINDOW_MAIN);
    
    PromptForExit();

    return TRUE;
}

/* The brain-damaged gtk_statusbar_pop interface doesn't return a value,
   so we have to use a signal to see if anything was actually popped. */
static int fFinishedPopping;

static void TextPopped( GtkWidget *pw, guint id, gchar *text, void *p ) {

    if( !text )
	fFinishedPopping = TRUE;
}

extern int GetPanelSize(void)
{
    if (!fFullScreen && fX && gtk_widget_get_realized(pwMain))
	{
		int pos = gtk_paned_get_position(GTK_PANED(hpaned));
		return pwMain->allocation.width - pos;
	}
	else
		return panelSize;
}

extern void SetPanelWidth(int size)
{
	panelSize = size;
    if( gtk_widget_get_realized( pwMain ) )
	{
		if (panelSize > pwMain->allocation.width * .8)
			panelSize = (int)(pwMain->allocation.width * .8);
	}
}

extern void SwapBoardToPanel(int ToPanel, int updateEvents)
{	/* Show/Hide panel on right of screen */
	if (ToPanel)
	{
		gtk_widget_reparent(pwEventBox, pwPanelGameBox);
		gtk_widget_show(hpaned);
		if (updateEvents)
			ProcessEvents();
		gtk_widget_hide(pwGameBox);
		gtk_paned_set_position(GTK_PANED(hpaned), pwMain->allocation.width - panelSize);

{	/* Hack to sort out widget positions - may be removed if works in later version of gtk */
		GtkAllocation temp = pwMain->allocation;
		temp.height++;
		gtk_widget_size_allocate(pwMain, &temp);
		temp.height--;
		gtk_widget_size_allocate(pwMain, &temp);
}
	}
	else
	{
		/* Need to hide these, as handle box seems to be buggy and gets confused */
		gtk_widget_hide(gtk_widget_get_parent(pwMenuBar));
		if (fToolbarShowing)
			gtk_widget_hide(gtk_widget_get_parent(pwToolbar));

		gtk_widget_reparent(pwEventBox, pwGameBox);
		gtk_widget_show(pwGameBox);
		if (updateEvents)
			ProcessEvents();
		if (GTK_WIDGET_VISIBLE(hpaned))
		{
			panelSize = GetPanelSize();
			gtk_widget_hide(hpaned);
		}
		gtk_widget_show(gtk_widget_get_parent(pwMenuBar));
		if (fToolbarShowing)
			gtk_widget_show(gtk_widget_get_parent(pwToolbar));
	}
}

static void MainSize( GtkWidget *pw, GtkRequisition *preq, gpointer p ) {

    /* Give the main window a size big enough that the board widget gets
       board_size=4, if it will fit on the screen. */
    
	int width;

    if( gtk_widget_get_realized( pw ) )
	g_signal_handlers_disconnect_by_func( G_OBJECT( pw ), G_CALLBACK( MainSize ), p );

    else if (!SetMainWindowSize())
		gtk_window_set_default_size( GTK_WINDOW( pw ),
				     MAX( 480, preq->width ),
				     MIN( preq->height + 79 * 3,
					  gdk_screen_height() - 20 ) );

	width = GetPanelWidth(WINDOW_MAIN);
	if (width)
		gtk_paned_set_position(GTK_PANED(hpaned), width - panelSize);
	else
		gtk_paned_set_position(GTK_PANED(hpaned), preq->width - panelSize);
}


static gchar *GTKTranslate ( const gchar *path, gpointer func_data ) {

  return (gchar *) gettext ( (const char *) path );

}

static void ToolbarStyle(gpointer    callback_data,
                       guint       callback_action,
                       GtkWidget  *widget)
{
	if(GTK_CHECK_MENU_ITEM(widget)->active)
	{	/* If radio button has been selected set style */
		SetToolbarStyle(callback_action - TOOLBAR_ACTION_OFFSET);
	}
}

GtkClipboard *clipboard = NULL;

static void PasteIDs(void)
{
	char *text = gtk_clipboard_wait_for_text(clipboard);
	int editing = ToolbarIsEditing(pwToolbar);
	char *sz;

	if (!text)
		return;

	if (editing)
		click_edit();

	sz = g_strdup_printf("set gnubgid %s", text);
	g_free(text);

	UserCommand(sz);
	g_free(sz);

	strcpy(ap[0].szName,default_names[0]);
	strcpy(ap[1].szName,default_names[1]);

#if USE_GTK
	if( fX )
		GTKSet(ap);
#endif /* USE_GTK */

	if (editing)
		click_edit();
}

extern void GTKTextToClipboard(const char *text)
{
  gtk_clipboard_set_text(clipboard, text, -1);
}

static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *eCon, void* null)
{	/* Maintain panel size */
	if (DockedPanelsShowing())
		gtk_paned_set_position(GTK_PANED(hpaned), eCon->width - GetPanelSize());

	return FALSE;
}

static void NewClicked(gpointer  p, guint n, GtkWidget * pw)
{
	GTKNew();
}

static void CopyAsBGbase(gpointer p, guint n, GtkWidget * pw)
{

	UserCommand("export position backgammonbase2clipboard");

}

static void CopyAsGOL(gpointer p, guint n, GtkWidget * pw)
{

	UserCommand("export position gol2clipboard");

}

static void CopyIDs(gpointer p, guint n, GtkWidget * pw)
{				/* Copy the position and match ids to the clipboard */
	char buffer[1024];

	if( ms.gs == GAME_NONE )
	{
		output( _("No game in progress.") );
		outputx();
		return;
	}

	sprintf(buffer, "%s %s\n%s %s\n", _("Position ID:"),
		PositionID(msBoard()), _("Match ID:"),
		MatchIDFromMatchState(&ms));

	GTKTextToClipboard(buffer);

	gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, _("Position and Match IDs copied to the clipboard") );
}

static void CopyMatchID(gpointer p, guint n, GtkWidget * pw)
{				/* Copy the position and match ids to the clipboard */
	char buffer[1024];

	if( ms.gs == GAME_NONE )
	{
		output( _("No game in progress.") );
		outputx();
		return;
	}

	sprintf(buffer, "%s %s\n",_("Match ID:"),
		MatchIDFromMatchState(&ms));

	GTKTextToClipboard(buffer);

	gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, _("Match ID copied to the clipboard") );
}

static void CopyPositionID(gpointer p, guint n, GtkWidget * pw)
{				/* Copy the position and match ids to the clipboard */
	char buffer[1024];

	if( ms.gs == GAME_NONE )
	{
		output( _("No game in progress.") );
		outputx();
		return;
	}

	sprintf(buffer, "%s %s\n", _("Position ID:"),
		PositionID(msBoard()));

	GTKTextToClipboard(buffer);

	gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, _("Position ID copied to the clipboard") );
}

static void TogglePanel(gpointer p, guint n, GtkWidget * pw)
{
	int f;
	gnubgwindow panel = 0;

	g_assert(GTK_IS_CHECK_MENU_ITEM(pw));

	f = GTK_CHECK_MENU_ITEM(pw)->active;
	switch (n) {
	case TOGGLE_ANALYSIS:
		panel = WINDOW_ANALYSIS;
		break;
	case TOGGLE_COMMENTARY:
		panel = WINDOW_ANNOTATION;
		break;
	case TOGGLE_GAMELIST:
		panel = WINDOW_GAME;
		break;
	case TOGGLE_MESSAGE:
		panel = WINDOW_MESSAGE;
		break;
	case TOGGLE_THEORY:
		panel = WINDOW_THEORY;
		break;
	case TOGGLE_COMMAND:
		panel = WINDOW_COMMAND;
		break;
	default:
		g_assert(FALSE);
	}
	if (f)
		PanelShow(panel);
	else
		PanelHide(panel);

	/* Resize screen */
	SetMainWindowSize();
}

extern void GTKUndo(void)
{
	BoardData *bd = BOARD( pwBoard )->board_data;

#if USE_BOARD3D
	RestrictiveRedraw();
#endif

	if (bd->drag_point >= 0)
	{	/* Drop piece */
		GdkEventButton dummyEvent;
		dummyEvent.x = dummyEvent.y = 0;
		board_button_release(pwBoard, &dummyEvent, bd);
	}

	ShowBoard();

	UpdateTheoryData(bd, TT_RETURNHITS, msBoard());
}

#if USE_BOARD3D

extern void SetSwitchModeMenuText(void)
{	/* Update menu text */
	BoardData *bd = BOARD( pwBoard )->board_data;
	GtkWidget *pMenuItem = gtk_item_factory_get_widget_by_action(pif, TOOLBAR_ACTION_OFFSET + MENU_OFFSET);
	char *text;
	if (display_is_2d(bd->rd))
		text = _("Switch to 3D view");
	else
		text = _("Switch to 2D view");
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(pMenuItem))), text);
	gtk_widget_set_sensitive(pMenuItem, gtk_gl_init_success);
}

static void
SwitchDisplayMode( gpointer p, guint n, GtkWidget *pw )
{
	BoardData *bd = BOARD( pwBoard )->board_data;
	BoardData3d *bd3d = bd->bd3d;
	renderdata *prd = bd->rd;

	if (display_is_2d(prd))
	{
		prd->fDisplayType = DT_3D;
		/* Reset 3d settings */
		MakeCurrent3d(bd3d);
		preDraw3d(bd, bd3d, prd);
		UpdateShadows(bd->bd3d);
		if (bd->diceShown == DICE_ON_BOARD)
			setDicePos(bd, bd3d);	/* Make sure dice appear ok */
		RestrictiveRedraw();

		/* Needed for 2d dice+chequer widgets */
		board_free_pixmaps(bd);
		board_create_pixmaps(pwBoard, bd);
	}
	else
	{
		prd->fDisplayType = DT_2D;
		/* Make sure 2d pixmaps are correct */
		board_free_pixmaps(bd);
		board_create_pixmaps(pwBoard, bd);
		/* Make sure dice are visible if rolled */
		if (bd->diceShown == DICE_ON_BOARD && bd->x_dice[0] <= 0)
			RollDice2d(bd);
	}

	DisplayCorrectBoardType(bd, bd3d, prd);
	SetSwitchModeMenuText();
	/* Make sure chequers correct below board */
	gtk_widget_queue_draw(bd->table);
}

#endif

static void ToggleShowingIDs( gpointer p, guint n, GtkWidget *pw )
{
	int newValue = GTK_CHECK_MENU_ITEM( pw )->active;
	char *sz = g_strdup_printf("set gui showids %s", newValue ? "on" :
			"off");
	UserCommand(sz);
	g_free(sz);
	UserCommand("save settings");
}

int fToolbarShowing = TRUE;

extern void ShowToolbar(void)
{
	GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
	gtk_widget_show(pwToolbar);
	gtk_widget_show(pwHandle);

	gtk_widget_show(gtk_item_factory_get_widget(pif, "/View/Toolbar/Hide Toolbar"));
	gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Text only"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Icons only"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Both"), TRUE);

	fToolbarShowing = TRUE;
}

extern void HideToolbar(void)
{
	GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
	gtk_widget_hide(pwToolbar);
	gtk_widget_hide(pwHandle);

	gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Hide Toolbar"));
	gtk_widget_show(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Text only"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Icons only"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/View/Toolbar/Both"), FALSE);

	fToolbarShowing = FALSE;
}

static gboolean EndFullScreen(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	short k = (short)event->keyval;

	if (k == KEY_ESCAPE)
		FullScreenMode(FALSE);

	return FALSE;
}

static void SetFullscreenWindowSettings(int panels, int ids, int maxed)
{
	showingPanels = panels;
	showingIDs = ids;
	maximised = maxed;
}

static void DoFullScreenMode(gpointer p, guint n, GtkWidget * pw)
{
	BoardData *bd = BOARD(pwBoard)->board_data;
	GtkWindow *ptl = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(bd->table)));
	GtkWidget *pwHandle = gtk_widget_get_parent(pwToolbar);
	int showingPanels;
	int maximised;
	static gulong id;
	static int changedRP, changedDP;

	GtkWidget *pmiRP = gtk_item_factory_get_widget(pif, "/View/Restore panels");
	GtkWidget *pmiDP = gtk_item_factory_get_widget(pif, "/View/Dock panels");

#if USE_BOARD3D
    if (display_is_3d(bd->rd))
	{	/* Stop any 3d animations */
		StopIdle3d(bd, bd->bd3d);
	}
#endif

	fFullScreen = GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif, "/View/Full screen"))->active;

	if (fFullScreen)
	{
		if (!fullScreenOnStartup)
			GTKShowWarning(WARN_FULLSCREEN_EXIT, NULL);
		else
			fullScreenOnStartup = FALSE;

		bd->rd->fShowGameInfo = FALSE;

		if (pmiRP && GTK_WIDGET_VISIBLE(pmiRP)
		    && GTK_WIDGET_IS_SENSITIVE(pmiRP))
			changedRP = TRUE;
		if (pmiDP && GTK_WIDGET_VISIBLE(pmiDP)
		    && GTK_WIDGET_IS_SENSITIVE(pmiDP))
			changedDP = TRUE;

		/* Check if window is maximized */
		{
			GdkWindowState state =
			    gdk_window_get_state(GTK_WIDGET(ptl)->window);
			maximised =
			    ((state & GDK_WINDOW_STATE_MAXIMIZED) ==
			     GDK_WINDOW_STATE_MAXIMIZED);
			SetFullscreenWindowSettings(ArePanelsShowing(),
						    fShowIDs,
						    maximised);
		}

		id = g_signal_connect(G_OBJECT(ptl), "key-press-event",
				      G_CALLBACK(EndFullScreen), 0);

		gtk_widget_hide(GTK_WIDGET(bd->table));
		gtk_widget_hide(GTK_WIDGET(bd->dice_area));
		gtk_widget_hide(pwStatus);
		gtk_widget_hide(pwProgress);
		gtk_widget_hide(pwStop);

		fFullScreen = FALSE;

		DoHideAllPanels(FALSE);

		fFullScreen = TRUE;
		fShowIDs = FALSE;

		gtk_widget_hide(pwToolbar);
		gtk_widget_hide(pwHandle);

		gtk_window_fullscreen(ptl);
		gtk_window_set_decorated(ptl, FALSE);

		if (pmiRP)
			gtk_widget_set_sensitive(pmiRP, FALSE);
		if (pmiDP)
			gtk_widget_set_sensitive(pmiDP, FALSE);
	} else {
		bd->rd->fShowGameInfo = TRUE;
		gtk_widget_show(pwMenuBar);
		if (fToolbarShowing)
		{
			gtk_widget_show(pwToolbar);
			gtk_widget_show(pwHandle);
		}
		gtk_widget_show(GTK_WIDGET(bd->table));
#if USE_BOARD3D
		/* Only show 2d dice below board if in 2d */
		if (display_is_2d(bd->rd))
#endif
			gtk_widget_show(GTK_WIDGET(bd->dice_area));
		gtk_widget_show(pwStatus);
		gtk_widget_show(pwProgress);
		gtk_widget_show(pwStop);

		GetFullscreenWindowSettings(&showingPanels,
					    &fShowIDs, &maximised);

		if (g_signal_handler_is_connected(G_OBJECT(ptl), id))
			g_signal_handler_disconnect(G_OBJECT(ptl), id);

		gtk_window_unfullscreen(ptl);
		gtk_window_set_decorated(ptl, TRUE);

		if (showingPanels)
		{
			fFullScreen = TRUE;	/* Avoid panel sizing code */
			ShowAllPanels(NULL, 0, NULL);
			fFullScreen = FALSE;
		}

		if (changedRP) {
			gtk_widget_set_sensitive(pmiRP, TRUE);
			changedRP = FALSE;
		}
		if (changedDP) {
			gtk_widget_set_sensitive(pmiDP, TRUE);
			changedDP = FALSE;
		}
	}
	UpdateSetting(&fShowIDs);
}

extern void FullScreenMode(int state)
{
	BoardData *bd = BOARD( pwBoard )->board_data;
	GtkWidget *pw = gtk_item_factory_get_widget(pif, "/View/Full screen");
	if (gtk_widget_get_realized(bd->table))
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pw), state);
}

void GetFullscreenWindowSettings(int *panels, int *ids, int *maxed)
{
	*panels = showingPanels;
	*ids = showingIDs;
	*maxed = maximised;
}

static void FinishMove(gpointer p, guint n, GtkWidget * pw)
{

	int anMove[8];
	char sz[65];

	if (!GTKGetMove(anMove))
		/* no valid move */
		return;

	UserCommand(FormatMove(sz, msBoard(), anMove));

}


typedef struct _evalwidget {
	evalcontext *pec;
	movefilter *pmf;
	GtkWidget *pwCubeful, *pwUsePrune, *pwDeterministic;
	GtkAdjustment *padjPlies, *padjSearchCandidates, *padjSearchTolerance, *padjNoise;
	int *pfOK;
	GtkWidget *pwOptionMenu;
	int fMoveFilter;
	GtkWidget *pwMoveFilter;
} evalwidget;

static void EvalGetValues ( evalcontext *pec, evalwidget *pew ) {

  pec->nPlies = (int)pew->padjPlies->value;
  pec->fCubeful =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON( pew->pwCubeful ) );

  pec->fUsePrune =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON( pew->pwUsePrune ) );

  pec->rNoise = (float)pew->padjNoise->value;
  pec->fDeterministic =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON( pew->pwDeterministic ) );
}


static void EvalChanged ( GtkWidget *pw, evalwidget *pew ) {

  int i;
  evalcontext ecCurrent;
  int fFound = FALSE;
  int fEval, fMoveFilter;

  EvalGetValues ( &ecCurrent, pew );

  /* update predefined settings menu */

  for ( i = 0; i < NUM_SETTINGS; i++ ) {

    fEval = ! cmp_evalcontext ( &aecSettings[ i ], &ecCurrent );
    fMoveFilter = ! aecSettings[ i ].nPlies ||
      ( ! pew->fMoveFilter || 
        equal_movefilters ( (movefilter (*)[MAX_FILTER_PLIES]) pew->pmf, 
                            aaamfMoveFilterSettings[ aiSettingsMoveFilter[ i ] ] ) );

    if ( fEval && fMoveFilter ) {

      /* current settings equal to a predefined setting */

      gtk_combo_box_set_active(GTK_COMBO_BOX ( pew->pwOptionMenu ), i );
      fFound = TRUE;
      break;

    }

  }


  /* user defined setting */

  if ( ! fFound )
    gtk_combo_box_set_active( GTK_COMBO_BOX ( pew->pwOptionMenu ),
                                  NUM_SETTINGS );

  
  if ( pew->fMoveFilter )
    gtk_widget_set_sensitive ( GTK_WIDGET ( pew->pwMoveFilter ),
                               ecCurrent.nPlies );

}


static void EvalNoiseValueChanged( GtkAdjustment *padj, evalwidget *pew ) {

    gtk_widget_set_sensitive( pew->pwDeterministic, padj->value != 0.0f );

    EvalChanged ( NULL, pew );
    
}

static void EvalPliesValueChanged( GtkAdjustment *padj, evalwidget *pew ) {
    gtk_widget_set_sensitive( pew->pwUsePrune, padj->value > 0 );
    EvalChanged ( NULL, pew );

}

static void SettingsMenuActivate ( GtkComboBox *box, evalwidget *pew ) {
  
  evalcontext *pec;
  int iSelected;
  

  iSelected = gtk_combo_box_get_active(box);
  if ( iSelected == NUM_SETTINGS )
    return; /* user defined */

  /* set all widgets to predefined values */

  pec = &aecSettings[ iSelected ];
 
  gtk_adjustment_set_value ( pew->padjPlies, pec->nPlies );
  gtk_adjustment_set_value ( pew->padjNoise, pec->rNoise );

  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwUsePrune ),
                                pec->fUsePrune );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwCubeful ),
                                pec->fCubeful );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwDeterministic ),
                                pec->fDeterministic );

  if ( pew->fMoveFilter )
    MoveFilterSetPredefined ( pew->pwMoveFilter, 
                              aiSettingsMoveFilter[ iSelected ] );

}

/*
 * Create widget for displaying evaluation settings
 *
 */

static GtkWidget *EvalWidget( evalcontext *pec, movefilter *pmf, 
                              int *pfOK, const int fMoveFilter ) {

    evalwidget *pew;
    GtkWidget *pwEval, *pw;

    GtkWidget *pwFrame, *pwFrame2;
    GtkWidget *pw2, *pw3;


    GtkWidget *pwev;
    int i;

    if( pfOK )
	*pfOK = FALSE;

    pwEval = gtk_vbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( pwEval ), 8 );
    
    pew = malloc( sizeof *pew );

    /*
     * Frame with prefined settings 
     */

    pwev = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add ( GTK_CONTAINER ( pwEval ), pwev );

    pwFrame = gtk_frame_new ( _("Predefined settings") );
    gtk_container_add ( GTK_CONTAINER ( pwev ), pwFrame );

    pw2 = gtk_vbox_new ( FALSE, 8 );
    gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw2 );
    gtk_container_set_border_width ( GTK_CONTAINER ( pw2 ), 8 );

    /* option menu with selection of predefined settings */

    gtk_container_add ( GTK_CONTAINER ( pw2 ),
                        gtk_label_new ( _("Select a predefined setting:") ) );

    gtk_widget_set_tooltip_text(pwev,
                          _("Select a predefined setting, ranging from "
                            "beginner's play to the grandmaster setting "
                            "that will test your patience"));

    pew->pwOptionMenu = gtk_combo_box_new_text();

    for ( i = 0; i < NUM_SETTINGS; i++ )
	    gtk_combo_box_append_text(GTK_COMBO_BOX(pew->pwOptionMenu), Q_(aszSettings[i]));
    gtk_combo_box_append_text(GTK_COMBO_BOX(pew->pwOptionMenu), _("user defined"));
    g_signal_connect(G_OBJECT(pew->pwOptionMenu), "changed", G_CALLBACK(SettingsMenuActivate), pew);
    gtk_container_add ( GTK_CONTAINER ( pw2 ), pew->pwOptionMenu );
                                                               


    /*
     * Frame with user settings 
     */

    pwFrame = gtk_frame_new ( _("User defined settings") );
    gtk_container_add ( GTK_CONTAINER ( pwEval ), pwFrame );
    
    pw2 = gtk_vbox_new ( FALSE, 8 );
    gtk_container_set_border_width( GTK_CONTAINER( pw2 ), 8 );
    gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw2 );

    /* lookahead */

    pwev = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add ( GTK_CONTAINER ( pw2 ), pwev );

    gtk_widget_set_tooltip_text(pwev,
                          _("Specify how many rolls GNU Backgammon should "
                            "lookahead. Each ply costs approximately a factor "
                            "of 21 in computational time. Also note that "
                            "2-ply is equivalent to Snowie's 3-ply setting."));

    pwFrame2 = gtk_frame_new ( _("Lookahead") );
    gtk_container_add ( GTK_CONTAINER ( pwev ), pwFrame2 );

    pw = gtk_hbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( pwFrame2 ), pw );

    pew->padjPlies = GTK_ADJUSTMENT( gtk_adjustment_new( pec->nPlies, 0, 7,
							 1, 1, 0 ) );
    gtk_container_add( GTK_CONTAINER( pw ),
		       gtk_label_new( _("Plies:") ) );
    gtk_container_add( GTK_CONTAINER( pw ),
		       gtk_spin_button_new( pew->padjPlies, 1, 0 ) );
	
    /* Use pruning neural nets */
    
    pwFrame2 = gtk_frame_new ( _("Pruning neural nets") );
    gtk_container_add ( GTK_CONTAINER ( pw2 ), pwFrame2 );

    gtk_container_add( GTK_CONTAINER( pwFrame2 ),
		       pew->pwUsePrune = gtk_check_button_new_with_label(
			   _("Use neural net pruning") ) );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwUsePrune ),
				  pec->fUsePrune );
    /* FIXME This needs a tool tip */

    /* cubeful */
    
    pwFrame2 = gtk_frame_new ( _("Cubeful evaluations") );
    gtk_container_add ( GTK_CONTAINER ( pw2 ), pwFrame2 );

    gtk_container_add( GTK_CONTAINER( pwFrame2 ),
		       pew->pwCubeful = gtk_check_button_new_with_label(
			   _("Cubeful chequer evaluation") ) );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwCubeful ),
				  pec->fCubeful );

    if ( fMoveFilter ) 
      /* checker play */
      gtk_widget_set_tooltip_text(pew->pwCubeful,
                            _("Instruct GNU Backgammon to use cubeful "
                              "evaluations, i.e., include the value of "
                              "cube ownership in the evaluations. It is "
                              "recommended to enable this option."));
    else
      /* checker play */
      gtk_widget_set_tooltip_text(pew->pwCubeful,
                            _("GNU Backgammon will always perform "
                              "cubeful evaluations for cube decisions. "
                              "Disabling this option will make GNU Backgammon "
                              "use cubeless evaluations in the interval nodes "
                              "of higher ply evaluations. It is recommended "
                              "to enable this option"));

    /* noise */

    pwev = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
    gtk_container_add ( GTK_CONTAINER ( pw2 ), pwev );

    gtk_widget_set_tooltip_text(pwev,
                          _("You can use this option to introduce noise "
                            "or errors in the evaluations. This is useful for "
                            "introducing levels below 0-ply. The lower rated "
                            "bots (e.g., GGotter) on the GamesGrid backgammon "
                            "server uses this technique. "
                            "The introduced noise can be "
                            "deterministic, i.e., always the same noise for "
                            "the same position, or it can be random"));

    pwFrame2 = gtk_frame_new ( _("Noise") );
    gtk_container_add ( GTK_CONTAINER ( pwev ), pwFrame2 );

    pw3 = gtk_vbox_new ( FALSE, 0 );
    gtk_container_add ( GTK_CONTAINER ( pwFrame2 ), pw3 );


    pew->padjNoise = GTK_ADJUSTMENT( gtk_adjustment_new(
	pec->rNoise, 0, 1, 0.001, 0.001, 0.0 ) );
    pw = gtk_hbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( pw3 ), pw );
    gtk_container_add( GTK_CONTAINER( pw ),
		       gtk_label_new( _("Noise:") ) );
    gtk_container_add( GTK_CONTAINER( pw ), gtk_spin_button_new(
	pew->padjNoise, 0.001, 3 ) );
    
    gtk_container_add( GTK_CONTAINER( pw3 ),
		       pew->pwDeterministic = gtk_check_button_new_with_label(
			   _("Deterministic noise") ) );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pew->pwDeterministic ),
				  pec->fDeterministic );

    /* misc data */

    pew->fMoveFilter = fMoveFilter;
    pew->pec = pec;
    pew->pmf = pmf;
    pew->pfOK = pfOK;

    /* move filter */

    if ( fMoveFilter ) {
      
      pew->pwMoveFilter = MoveFilterWidget ( pmf, pfOK, 
                                             G_CALLBACK ( EvalChanged ), 
                                             pew );

      pwev = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwev), FALSE);
      gtk_container_add ( GTK_CONTAINER ( pwEval ), pwev ); 
      gtk_container_add ( GTK_CONTAINER ( pwev ), pew->pwMoveFilter );

      gtk_widget_set_tooltip_text(pwev,
                            _("GNU Backgammon will evaluate all moves at "
                              "0-ply. The move filter controls how many "
                              "moves to be evaluted at higher plies. "
                              "A \"smaller\" filter will be faster, but "
                              "GNU Backgammon may not find the best move. "
                              "Power users may set up their own filters "
                              "by clicking on the [Modify] button"));

    }
    else
      pew->pwMoveFilter = NULL;

    /* setup signals */

    g_signal_connect( G_OBJECT( pew->padjPlies ), "value-changed",
			G_CALLBACK( EvalPliesValueChanged ), pew );
    EvalPliesValueChanged( pew->padjPlies, pew );
    
    g_signal_connect( G_OBJECT( pew->padjNoise ), "value-changed",
			G_CALLBACK( EvalNoiseValueChanged ), pew );
    EvalNoiseValueChanged( pew->padjNoise, pew );

    g_signal_connect( G_OBJECT( pew->pwDeterministic ), "toggled",
                         G_CALLBACK( EvalChanged ), pew );

    g_signal_connect( G_OBJECT( pew->pwCubeful ), "toggled",
                         G_CALLBACK( EvalChanged ), pew );
    
    g_signal_connect( G_OBJECT( pew->pwUsePrune ), "toggled",
                         G_CALLBACK( EvalChanged ), pew );
    
    g_object_set_data_full( G_OBJECT( pwEval ), "user_data", pew, free );

    return pwEval;
}

static void EvalOK( GtkWidget *pw, void *p ) {

    GtkWidget *pwEval = GTK_WIDGET( p );
    evalwidget *pew = g_object_get_data( G_OBJECT( pwEval ), "user_data" );

    if( pew->pfOK )
	*pew->pfOK = TRUE;

    EvalGetValues ( pew->pec, pew );
	
    if( pew->pfOK )
	gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static void SetEvalCommands( const char *szPrefix, evalcontext *pec,
			     evalcontext *pecOrig ) {

    char sz[ 256 ];

    outputpostpone();

    if( pec->nPlies != pecOrig->nPlies ) {
	sprintf( sz, "%s plies %d", szPrefix, pec->nPlies );
	UserCommand( sz );
    }

    if( pec->fUsePrune != pecOrig->fUsePrune ) {
	sprintf( sz, "%s prune %s", szPrefix, pec->fUsePrune ? "on" : "off" );
	UserCommand( sz );
    }

    if( pec->fCubeful != pecOrig->fCubeful ) {
	sprintf( sz, "%s cubeful %s", szPrefix, pec->fCubeful ? "on" : "off" );
	UserCommand( sz );
    }

    if( pec->rNoise != pecOrig->rNoise ) {
        gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
	sprintf( sz, "%s noise %s", szPrefix, 
                                  g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE,
			           "%.3f", pec->rNoise ));
	UserCommand( sz );
    }
    
    if( pec->fDeterministic != pecOrig->fDeterministic ) {
	sprintf( sz, "%s deterministic %s", szPrefix, pec->fDeterministic ?
		 "on" : "off" );
	UserCommand( sz );
    }

    outputresume();
}

typedef struct _AnalysisDetails
{
	const char *title;
	evalcontext *esChequer;
	movefilter *mfChequer;
	evalcontext *esCube;
	movefilter *mfCube;
	GtkWidget *pwCube, *pwChequer, *pwOptionMenu, *pwSettingWidgets;
	int cubeDisabled;
} AnalysisDetails;

static void DetailedAnalysisOK(GtkWidget *pw, AnalysisDetails *pDetails)
{
  EvalOK(pDetails->pwChequer, pDetails->pwChequer);
  EvalOK(pDetails->pwCube, pDetails->pwCube);
  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static int EvalDefaultSetting(evalcontext *pec, movefilter *pmf)
{
	int i;
	int fEval, fMoveFilter;

	/* Look for predefined settings */
	for ( i = 0; i < NUM_SETTINGS; i++ )
	{
		fEval = ! cmp_evalcontext ( &aecSettings[ i ], pec );
		fMoveFilter = ! aecSettings[ i ].nPlies ||
			( !pmf || 
					equal_movefilters ( (movefilter (*)[MAX_FILTER_PLIES]) pmf, 
								aaamfMoveFilterSettings[ aiSettingsMoveFilter[ i ] ] ) );

		if (fEval && fMoveFilter)
			return i;
	}

	return NUM_SETTINGS;
}

static void UpdateSummaryEvalMenuSetting(AnalysisDetails *pAnalDetails )
{
	int chequerDefault = EvalDefaultSetting(pAnalDetails->esChequer, pAnalDetails->mfChequer);
	int cubeDefault = EvalDefaultSetting(pAnalDetails->esCube, pAnalDetails->mfCube);
	int setting = NUM_SETTINGS;
	if (chequerDefault == cubeDefault
		/* Special case as cube_supremo==cube_worldclass */
		|| (chequerDefault == SETTINGS_SUPREMO && cubeDefault == SETTINGS_WORLDCLASS))
		setting = chequerDefault;

    gtk_combo_box_set_active(GTK_COMBO_BOX ( pAnalDetails->pwOptionMenu ), setting );
}

static void ShowDetailedAnalysis(GtkWidget *button, AnalysisDetails *pDetails)
{
	GtkWidget *pwvbox, *pwFrame, *pwDialog, *hbox;
	pwDialog = GTKCreateDialog(pDetails->title,
				   DT_INFO, button, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON,
				   G_CALLBACK(DetailedAnalysisOK), pDetails);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), hbox);

	pwFrame = gtk_frame_new (_("Chequer play"));
	gtk_box_pack_start (GTK_BOX (hbox), pwFrame, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

	gtk_container_add(GTK_CONTAINER (pwFrame ), 
		  pDetails->pwChequer = EvalWidget( pDetails->esChequer, pDetails->mfChequer,
												   NULL, pDetails->mfChequer != NULL ));

	pwFrame = gtk_frame_new (_("Cube decisions"));
	gtk_box_pack_start (GTK_BOX (hbox), pwFrame, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

	pwvbox = gtk_vbox_new ( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER( pwFrame ), pwvbox );

	gtk_box_pack_start ( GTK_BOX ( pwvbox ), 
					   pDetails->pwCube = EvalWidget( pDetails->esCube, (movefilter*)&pDetails->mfCube[9],
														NULL, pDetails->mfCube != NULL ),
					   FALSE, FALSE, 0 );

	if (pDetails->cubeDisabled)
		gtk_widget_set_sensitive(pDetails->pwCube, FALSE);

	GTKRunDialog(pwDialog);
	UpdateSummaryEvalMenuSetting(pDetails);
}

static void SummaryMenuActivate(GtkComboBox *box, AnalysisDetails *pAnalDetails)
{
  int selected = gtk_combo_box_get_active(box);
  if (selected == NUM_SETTINGS)
    return; /* user defined */

  /* set eval settings to predefined values */
  *pAnalDetails->esChequer = aecSettings[selected];
  *pAnalDetails->esCube = aecSettings[selected];

  if (pAnalDetails->mfChequer)
    memcpy(pAnalDetails->mfChequer, aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]], sizeof(aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]]));
  if (pAnalDetails->mfCube)
    memcpy(pAnalDetails->mfCube, aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]], sizeof(aaamfMoveFilterSettings[aiSettingsMoveFilter[selected]]));
}

static GtkWidget *AddLevelSettings(GtkWidget *pwFrame, AnalysisDetails *pAnalDetails)
{
	GtkWidget *vbox, *hbox, *pw2, *pwDetails, *vboxSpacer;
	int i;

	vboxSpacer = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width ( GTK_CONTAINER ( vboxSpacer ), 8 );
	gtk_container_add ( GTK_CONTAINER ( pwFrame ), vboxSpacer );

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add ( GTK_CONTAINER ( vboxSpacer ), vbox );

	/*
	 * Frame with prefined settings 
	 */

	pw2 = gtk_vbox_new ( FALSE, 4 );
	gtk_box_pack_start(GTK_BOX(vbox), pw2, FALSE, FALSE, 0 );

	/* option menu with selection of predefined settings */

	pAnalDetails->pwOptionMenu = gtk_combo_box_new_text();

	for ( i = 0; i < NUM_SETTINGS; i++ )
		gtk_combo_box_append_text(GTK_COMBO_BOX(pAnalDetails->pwOptionMenu), Q_(aszSettings[i]));
	gtk_combo_box_append_text(GTK_COMBO_BOX(pAnalDetails->pwOptionMenu), _("user defined"));
	g_signal_connect(G_OBJECT(pAnalDetails->pwOptionMenu), "changed", G_CALLBACK(SummaryMenuActivate), pAnalDetails);
	gtk_container_add ( GTK_CONTAINER ( pw2 ), pAnalDetails->pwOptionMenu );

	pwDetails = gtk_button_new_with_label( _("Advanced Settings...") );
	g_signal_connect( G_OBJECT( pwDetails ), "clicked", G_CALLBACK( ShowDetailedAnalysis ), (void *)pAnalDetails );
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), pwDetails, FALSE, FALSE, 0 );
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4 );
	UpdateSummaryEvalMenuSetting(pAnalDetails);
	return vboxSpacer;	/* Container */
}

#define CHECKUPDATE(button,flag,string) \
   n = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( (button) ) ); \
   if ( n != (flag)){ \
           sprintf(sz, (string), n ? "on" : "off"); \
           UserCommand(sz); \
   }

#define ADJUSTSKILLUPDATE(button,flag,string) \
  if( paw->apadjSkill[(button)]->value != arSkillLevel[(flag)] ) \
  { \
     gchar buf[G_ASCII_DTOSTR_BUF_SIZE]; \
     sprintf(sz,  (string), g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE, \
			           "%0.3f", (gdouble) paw->apadjSkill[(button)]->value )); \
     UserCommand(sz); \
  }

#define ADJUSTLUCKUPDATE(button,flag,string) \
  if( paw->apadjLuck[(button)]->value != arLuckLevel[(flag)] ) \
  { \
     gchar buf[G_ASCII_DTOSTR_BUF_SIZE]; \
     sprintf(sz,  (string), g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE, \
			           "%0.3f", (gdouble) paw->apadjLuck[(button)]->value )); \
     UserCommand(sz); \
  }
     
static void AnalysisOK( GtkWidget *pw, analysiswidget *paw ) {

  char sz[128]; 
  int n;

  gtk_widget_hide( gtk_widget_get_toplevel( pw ) );

  CHECKUPDATE(paw->pwMoves,fAnalyseMove, "set analysis moves %s")
  CHECKUPDATE(paw->pwCube,fAnalyseCube, "set analysis cube %s")
  CHECKUPDATE(paw->pwLuck,fAnalyseDice, "set analysis luck %s")
  CHECKUPDATE(paw->apwAnalysePlayers[ 0 ], afAnalysePlayers[ 0 ],
              "set analysis player 0 analyse %s")
  CHECKUPDATE(paw->apwAnalysePlayers[ 1 ], afAnalysePlayers[ 1 ],
              "set analysis player 1 analyse %s")

  ADJUSTSKILLUPDATE( 0, SKILL_DOUBTFUL, "set analysis threshold doubtful %s" )
  ADJUSTSKILLUPDATE( 1, SKILL_BAD, "set analysis threshold bad %s" )
  ADJUSTSKILLUPDATE( 2, SKILL_VERYBAD, "set analysis threshold verybad %s" )

  ADJUSTLUCKUPDATE( 0, LUCK_VERYGOOD, "set analysis threshold verylucky %s" )
  ADJUSTLUCKUPDATE( 1, LUCK_GOOD, "set analysis threshold lucky %s" )
  ADJUSTLUCKUPDATE( 2, LUCK_BAD, "set analysis threshold unlucky %s" )
  ADJUSTLUCKUPDATE( 3, LUCK_VERYBAD, "set analysis threshold veryunlucky %s" )

  /* Group output in one batch */
  outputpostpone();

  SetEvalCommands( "set analysis chequerplay eval", &paw->esChequer.ec,
		  &esAnalysisChequer.ec );
  SetMovefilterCommands ( "set analysis movefilter", paw->aamf, aamfAnalysis );
  SetEvalCommands( "set analysis cubedecision eval", &paw->esCube.ec,
		  &esAnalysisCube.ec );

  n = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->pwHintSame));
  if (n != fEvalSameAsAnalysis)
  {
	sprintf( sz, "set eval sameasanalysis %s", n ? "yes" : "no" );
	UserCommand( sz );
  }

  if (!fEvalSameAsAnalysis)
  {
	  SetEvalCommands( "set evaluation chequer eval", &paw->esEvalChequer.ec,
			  &GetEvalChequer()->ec );
	  SetMovefilterCommands ( "set evaluation movefilter",
			  paw->aaEvalmf, *GetEvalMoveFilter() );
	  SetEvalCommands( "set evaluation cubedecision eval", &paw->esEvalCube.ec,
			  &GetEvalCube()->ec );
  }
  UserCommand("save settings");
  outputresume();

  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );

}
#undef CHECKUPDATE
#undef ADJUSTSKILLUPDATE
#undef ADJUSTLUCKUPDATE

static void AnalysisSet( analysiswidget *paw) {

  int i;

  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( paw->pwMoves ),
                                fAnalyseMove );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( paw->pwCube ),
                                fAnalyseCube );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( paw->pwLuck ),
                                fAnalyseDice );

  for ( i = 0; i < 2; ++i )
    gtk_toggle_button_set_active( 
        GTK_TOGGLE_BUTTON( paw->apwAnalysePlayers[ i ] ), 
        afAnalysePlayers[ i ] );

  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjSkill[0] ),
		 arSkillLevel[SKILL_DOUBTFUL] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjSkill[1] ),
		 arSkillLevel[SKILL_BAD] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjSkill[2] ),
		 arSkillLevel[SKILL_VERYBAD] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjLuck[0] ),
		 arLuckLevel[LUCK_VERYGOOD] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjLuck[1] ),
		 arLuckLevel[LUCK_GOOD] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjLuck[2] ),
		 arLuckLevel[LUCK_BAD] );
  gtk_adjustment_set_value ( GTK_ADJUSTMENT( paw->apadjLuck[3] ),
		 arLuckLevel[LUCK_VERYBAD] );
}

static void HintSameToggled( GtkWidget *notused, analysiswidget *paw )
{
	int active = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(paw->pwHintSame));
	gtk_widget_set_sensitive(paw->pwCubeSummary, active);
}

static AnalysisDetails *CreateEvalSettings(GtkWidget *pwParent, const char *title, evalcontext *pechequer, movefilter *pmfchequer, evalcontext *pecube, movefilter *pmfcube)
{
	AnalysisDetails *pAnalDetail = malloc(sizeof(AnalysisDetails));
	pAnalDetail->title = title;
	pAnalDetail->esChequer = pechequer;
	pAnalDetail->mfChequer = pmfchequer;
	pAnalDetail->esCube = pecube;
	pAnalDetail->mfCube = pmfcube;
	pAnalDetail->cubeDisabled = FALSE;

	pAnalDetail->pwSettingWidgets = AddLevelSettings(pwParent, pAnalDetail);
	return pAnalDetail;
}

extern void SetAnalysis(gpointer p, guint n, GtkWidget * pw)
{
	const char *aszSkillLabel[3] = { N_("Doubtful:"), N_("Bad:"), N_("Very bad:") };
	const char *aszLuckLabel[4] = { N_("Very lucky:"), N_("Lucky:"),
	  N_("Unlucky:"), N_("Very unlucky:") };
	int i;
	AnalysisDetails *pAnalDetailSettings1, *pAnalDetailSettings2;
	GtkWidget *pwDialog, *pwPage, *pwFrame, *pwLabel, *pwSpin, *pwTable; 
	GtkWidget *hboxTop, *hboxBottom, *vbox1, *vbox2, *hbox;
	analysiswidget aw;

	memcpy(&aw.esCube, &esAnalysisCube, sizeof(aw.esCube));
	memcpy(&aw.esChequer, &esAnalysisChequer, sizeof(aw.esChequer));
	memcpy(&aw.aamf, aamfAnalysis, sizeof(aw.aamf));

    memcpy( &aw.esEvalChequer, &esEvalChequer, sizeof esEvalChequer );
    memcpy( &aw.esEvalCube, &esEvalCube, sizeof esEvalCube );
    memcpy( aw.aaEvalmf, aamfEval, sizeof ( aamfEval ) );

	pwDialog = GTKCreateDialog(_("GNU Backgammon - Analysis Settings"),
				   DT_QUESTION, NULL, DIALOG_FLAG_MODAL,
				   G_CALLBACK(AnalysisOK), &aw);

  pwPage = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width( GTK_CONTAINER( pwPage ), 8 );

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pwPage), vbox1, TRUE, TRUE, 0);

  hboxTop = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hboxTop, TRUE, TRUE, 0);
  hboxBottom = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hboxBottom, TRUE, TRUE, 0);

  pwFrame = gtk_frame_new (_("Analysis"));
  gtk_box_pack_start (GTK_BOX (hboxTop), pwFrame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (pwFrame), vbox2);

  aw.pwMoves = gtk_check_button_new_with_label (_("Chequer play"));
  gtk_box_pack_start (GTK_BOX (vbox2), aw.pwMoves, FALSE, FALSE, 0);

  aw.pwCube = gtk_check_button_new_with_label (_("Cube decisions"));
  gtk_box_pack_start (GTK_BOX (vbox2), aw.pwCube, FALSE, FALSE, 0);

  aw.pwLuck = gtk_check_button_new_with_label (_("Luck"));
  gtk_box_pack_start (GTK_BOX (vbox2), aw.pwLuck, FALSE, FALSE, 0);

  for ( i = 0; i < 2; ++i ) {

    char *sz = g_strdup_printf( _("Analyse player %s"), ap[ i ].szName );

    aw.apwAnalysePlayers[ i ] = gtk_check_button_new_with_label ( sz );
    gtk_box_pack_start (GTK_BOX (vbox2), aw.apwAnalysePlayers[ i ], 
                        FALSE, FALSE, 0);

  }

  pwFrame = gtk_frame_new (_("Skill thresholds"));
  gtk_box_pack_start (GTK_BOX (hboxBottom), pwFrame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

  pwTable = gtk_table_new (5, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (pwFrame), pwTable);

  for (i = 0; i < 3; i++){
    pwLabel = gtk_label_new ( gettext ( aszSkillLabel[i] ) );
    gtk_table_attach (GTK_TABLE (pwTable), pwLabel, 0, 1, i, i+1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (pwLabel), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment (GTK_MISC (pwLabel), 0, 0.5);
  }
  
  for (i = 0; i < 3; i++){
    aw.apadjSkill[i] = 
	  GTK_ADJUSTMENT( gtk_adjustment_new( 1, 0, 1, 0.01, 0.05, 0 ) );

    pwSpin = 
	    gtk_spin_button_new (GTK_ADJUSTMENT (aw.apadjSkill[i]), 1, 2);
    gtk_table_attach (GTK_TABLE (pwTable), pwSpin, 1, 2, i, i+1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (pwSpin), TRUE);
  }

  pwFrame = gtk_frame_new (_("Luck thresholds"));
  gtk_box_pack_start (GTK_BOX (hboxBottom), pwFrame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

  pwTable = gtk_table_new (4, 2, FALSE);
  gtk_container_add (GTK_CONTAINER (pwFrame), pwTable);
 
  for (i = 0; i < 4; i++){
    pwLabel = gtk_label_new ( gettext ( aszLuckLabel[i] ) );
    gtk_table_attach (GTK_TABLE (pwTable), pwLabel, 0, 1, i, i+1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify (GTK_LABEL (pwLabel), GTK_JUSTIFY_RIGHT);
    gtk_misc_set_alignment (GTK_MISC (pwLabel), 0, 0.5);
  }
  
  for (i = 0; i < 4; i++){
    aw.apadjLuck[i] = 
	  GTK_ADJUSTMENT( gtk_adjustment_new( 1, 0, 1, 0.01, 0.05, 0 ) );

    pwSpin = 
	    gtk_spin_button_new (GTK_ADJUSTMENT (aw.apadjLuck[i]), 1, 2);
    
    gtk_table_attach (GTK_TABLE (pwTable), pwSpin, 1, 2, i, i+1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (pwSpin), TRUE);
  }

  pwFrame = gtk_frame_new (_("Analysis Level"));
  gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hboxTop), vbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), pwFrame, FALSE, FALSE, 0);

  pAnalDetailSettings1 = CreateEvalSettings(pwFrame, _("Analysis settings"),
										  &aw.esChequer.ec, (movefilter*)&aw.aamf, &aw.esCube.ec, NULL);

  gtk_box_pack_start (GTK_BOX (pwPage), gtk_vseparator_new(), TRUE, TRUE, 0);

  pwFrame = gtk_frame_new (_("Eval Hint/Tutor Level"));
  gtk_container_set_border_width (GTK_CONTAINER (pwFrame), 4);
  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), pwFrame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (pwPage), vbox2, FALSE, FALSE, 0);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), vbox1 );

  aw.pwHintSame = gtk_check_button_new_with_label(_("Same as analysis"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(aw.pwHintSame), fEvalSameAsAnalysis);
  g_signal_connect(G_OBJECT(aw.pwHintSame), "toggled", G_CALLBACK(HintSameToggled), &aw);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), aw.pwHintSame, FALSE, FALSE, 8);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0 );

  pAnalDetailSettings2 = CreateEvalSettings(vbox1, _("Hint/Tutor settings"),
										  &aw.esEvalChequer.ec, (movefilter*)&aw.aaEvalmf, &aw.esEvalCube.ec, NULL);
  aw.pwCubeSummary = pAnalDetailSettings2->pwSettingWidgets;

	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)),
			  pwPage);

	AnalysisSet(&aw);


	HintSameToggled(NULL, &aw);
	GTKRunDialog(pwDialog);
	free(pAnalDetailSettings1);
	free(pAnalDetailSettings2);
}

typedef struct _playerswidget {
	int *pfOK;
	player *ap;
	GtkWidget *apwName[ 2 ], *apwRadio[ 2 ][ 3 ], *apwSocket[ 2 ], *apwExternal[ 2 ];
	char aszSocket[ 2 ][ 128 ];
	evalsetup esChequer[2];
	evalsetup esCube[2]; 
	AnalysisDetails *pLevelSettings[2];
} playerswidget;

static void PlayerTypeToggled( GtkWidget *pw, playerswidget *ppw )
{
	int i;

	for( i = 0; i < 2; i++ )
	{
		gtk_widget_set_sensitive(ppw->pLevelSettings[i]->pwSettingWidgets,
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON( ppw->apwRadio[ i ][ 1 ] ) ) );
		gtk_widget_set_sensitive(ppw->apwExternal[ i ],
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON( ppw->apwRadio[ i ][ 2 ] ) ) );
	}
}

static GtkWidget *PlayersPage( playerswidget *ppw, int i, const char *title )
{
    GtkWidget *pw, *pwFrame, *pwVBox;

	pwFrame = gtk_frame_new ( title );
    gtk_container_set_border_width( GTK_CONTAINER( pwFrame ), 4 );
	pwVBox = gtk_vbox_new(FALSE, 0);
    gtk_container_add( GTK_CONTAINER( pwFrame ), pwVBox );

    pw = gtk_hbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( pw ), 4 );
    gtk_container_add( GTK_CONTAINER( pwVBox ), pw );
    gtk_container_add( GTK_CONTAINER( pw ),
		       gtk_label_new( _("Default Name:") ) );
    gtk_container_add( GTK_CONTAINER( pw ),
		       ppw->apwName[ i ] = gtk_entry_new() );
    gtk_entry_set_text( GTK_ENTRY( ppw->apwName[ i ] ),
			(ppw->ap[ i ].szName) );
    
    gtk_container_add( GTK_CONTAINER( pwVBox ),
		       ppw->apwRadio[ i ][ 0 ] =
		       gtk_radio_button_new_with_label( NULL, _("Human") ) );
    
    gtk_container_add( GTK_CONTAINER( pwVBox ),
		       ppw->apwRadio[ i ][ 1 ] =
		       gtk_radio_button_new_with_label_from_widget(
			   GTK_RADIO_BUTTON( ppw->apwRadio[ i ][ 0 ] ),
			   _("GNU Backgammon") ) );

	memcpy(&ppw->esCube[i], &ppw->ap[ i ].esChequer.ec, sizeof(ppw->esCube[i]));
	memcpy(&ppw->esChequer[i], &ppw->ap[ i ].esCube.ec, sizeof(ppw->esChequer[i]));

	ppw->pLevelSettings[i] = CreateEvalSettings(pwVBox, _("GNU Backgammon settings"),
								&ppw->ap[ i ].esChequer.ec, (movefilter*)ppw->ap[ i ].aamf, &ppw->ap[ i ].esCube.ec, NULL);
    
    gtk_container_add( GTK_CONTAINER( pwVBox ),
		       ppw->apwRadio[ i ][ 2 ] =
		       gtk_radio_button_new_with_label_from_widget(
			   GTK_RADIO_BUTTON( ppw->apwRadio[ i ][ 0 ] ),
			   _("External") ) );
    ppw->apwExternal[ i ] = pw = gtk_hbox_new( FALSE, 0 );
    gtk_container_set_border_width( GTK_CONTAINER( pw ), 4 );
    gtk_widget_set_sensitive( pw, ap[ i ].pt == PLAYER_EXTERNAL );
    gtk_container_add( GTK_CONTAINER( pwVBox ), pw );
    gtk_container_add( GTK_CONTAINER( pw ),
		       gtk_label_new( _("Socket:") ) );
    gtk_container_add( GTK_CONTAINER( pw ),
		       ppw->apwSocket[ i ] = gtk_entry_new() );
    if( ap[ i ].szSocket )
	gtk_entry_set_text( GTK_ENTRY( ppw->apwSocket[ i ] ),
			    ap[ i ].szSocket );
    
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(
	ppw->apwRadio[ i ][ ap[ i ].pt ] ), TRUE );

    g_signal_connect( G_OBJECT( ppw->apwRadio[ i ][ 1 ] ), "toggled",
			G_CALLBACK( PlayerTypeToggled ), ppw );
    g_signal_connect( G_OBJECT( ppw->apwRadio[ i ][ 2 ] ), "toggled",
			G_CALLBACK( PlayerTypeToggled ), ppw );
    
    return pwFrame;
}

static void PlayersOK( GtkWidget *pw, playerswidget *pplw ) {

    int i,j ;
    *pplw->pfOK = TRUE;

    for( i = 0; i < 2; i++ ) {
	strcpyn( pplw->ap[ i ].szName, gtk_entry_get_text(
	    GTK_ENTRY( pplw->apwName[ i ] ) ), MAX_NAME_LEN );
	
	for( j = 0; j < 3; j++ )
	    if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(
		pplw->apwRadio[ i ][ j ] ) ) ) {
		pplw->ap[ i ].pt = j;
		break;
	    }
	g_assert( j < 4 );

	strcpyn( pplw->aszSocket[ i ], gtk_entry_get_text(
	    GTK_ENTRY( pplw->apwSocket[ i ] ) ), 128 );
    }

    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static void SetPlayers(gpointer p, guint n, GtkWidget *pw)
{
	GtkWidget *pwDialog, *pwHBox;
	int i, fOK = FALSE;
	player apTemp[2];
	playerswidget plw;
	char sz[256];

	memcpy(apTemp, ap, sizeof ap);
	plw.ap = apTemp;
	plw.pfOK = &fOK;

	pwDialog =
	    GTKCreateDialog(_("GNU Backgammon - Players"), DT_QUESTION,
			    NULL, DIALOG_FLAG_MODAL, G_CALLBACK(PlayersOK),
			    &plw);

	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)),
			pwHBox = gtk_hbox_new(FALSE, 0));
	gtk_box_pack_start(GTK_BOX(pwHBox), PlayersPage(&plw, 0, _("Player 0")), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pwHBox), PlayersPage(&plw, 1, _("Player 1")), FALSE, FALSE, 0);
	PlayerTypeToggled(NULL, &plw);

	GTKRunDialog(pwDialog);

	free(plw.pLevelSettings[0]);
	free(plw.pLevelSettings[1]);

	if (fOK)
	{
		outputpostpone();

		sprintf(sz, "set defaultnames \"%s\" \"%s\"", apTemp[0].szName, apTemp[1].szName);
	       	UserCommand(sz);

		for (i = 0; i < 2; i++) {
			/* NB: this comparison is case-sensitive, and does not use
			   CompareNames(), so that the user can modify the case of
			   names. */
			switch (apTemp[i].pt) {
			case PLAYER_HUMAN:
				if (ap[i].pt != PLAYER_HUMAN) {
					sprintf(sz, "set player %d human",
						i);
					UserCommand(sz);
				}
				break;

			case PLAYER_GNU:
				if (ap[i].pt != PLAYER_GNU) {
					sprintf(sz, "set player %d gnubg",
						i);
					UserCommand(sz);
				}

				/* FIXME another temporary hack (should be some way to set
				   chequer and cube parameters independently) */
				sprintf(sz,
					"set player %d chequer evaluation",
					i);
				SetEvalCommands(sz,
						&apTemp[i].esChequer.ec,
						&ap[i].esChequer.ec);
				sprintf(sz, "set player %d movefilter", i);
				SetMovefilterCommands(sz, apTemp[i].aamf,
						      ap[i].aamf);
				sprintf(sz,
					"set player %d cube evaluation",
					i);
				SetEvalCommands(sz, &apTemp[i].esCube.ec,
						&ap[i].esCube.ec);
				break;

			case PLAYER_EXTERNAL:
				if (ap[i].pt != PLAYER_EXTERNAL ||
				    strcmp(ap[i].szSocket,
					   plw.aszSocket[i])) {
					sprintf(sz,
						"set player %d external %s",
						i, plw.aszSocket[i]);
					UserCommand(sz);
				}
				break;
			}
		}

		UserCommand("save settings");
		outputresume();
	}
}

static void SetOptions(gpointer p, guint n, GtkWidget * pw)
{

	GTKSetOptions();

}

/* Language screen name, code and flag name */
static char *aaszLang[][ 3 ] = {
    { N_("System default"), "system", NULL },
    { N_("Czech"),	    "cs_CZ", "flags/czech.png" },
    { N_("Danish"),	    "da_DK", "flags/denmark.png" },
    { N_("English (GB)"),   "en_GB", "flags/england.png" },
    { N_("English (US)"),   "en_US", "flags/usa.png" },
    { N_("French"),	    "fr_FR", "flags/france.png" },
    { N_("German"),	    "de_DE", "flags/germany.png" },
    { N_("Icelandic"),      "is_IS", "flags/iceland.png" },
    { N_("Italian"),	    "it_IT", "flags/italy.png" },
    { N_("Japanese"),	    "ja_JP", "flags/japan.png" },
    { N_("Romanian"),       "ro_RO", "flags/romania.png" },
    { N_("Russian"),	    "ru_RU", "flags/russia.png" },
    { N_("Spanish"),	    "es_ES", "flags/spain.png" },
    { N_("Turkish"),	    "tr_TR", "flags/turkey.png" },
    { NULL, NULL, NULL }
};

static void TranslateWidgets(GtkWidget *p)
{
	if (GTK_IS_CONTAINER(p))
	{
		GList *pl = gtk_container_get_children(GTK_CONTAINER(p));
		while(pl)
		{
			TranslateWidgets(pl->data);
			pl = pl->next;
		}
		g_list_free(pl);
	}
	if (GTK_IS_LABEL(p))
	{
		char *name = (char*)g_object_get_data(G_OBJECT(p), "lang");
		gtk_label_set_text(GTK_LABEL(p), _(name));
	}
}

static void SetLangDialogText(void)
{
   gtk_window_set_title(GTK_WINDOW(pwLangDialog), _("Select language"));
   TranslateWidgets(pwLangDialog);
}

static void SetLangOk(void)
{
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pwLangRadio1)))
		newLang = aaszLang[0][1];
	else
		newLang = (char*)g_object_get_data(G_OBJECT(curSel), "lang");

	gtk_widget_destroy(pwLangDialog);
}

static gboolean FlagClicked(GtkWidget *pw, GdkEventButton *event, void* dummy)
{
	/* Manually highlight clicked flag */
	GtkWidget *frame, *eb;

	if (event && event->type == GDK_2BUTTON_PRESS && curSel == pw)
		SetLangOk();

	if (curSel == pw)
		return FALSE;

	if (curSel)
	{	/* Reset old item */
		frame = gtk_bin_get_child(GTK_BIN(curSel));
		eb = gtk_bin_get_child(GTK_BIN(frame));
		gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
		gtk_widget_modify_bg(eb, GTK_STATE_NORMAL, NULL);
	}

	curSel = pw;
	frame = gtk_bin_get_child(GTK_BIN(pw));
	eb = gtk_bin_get_child(GTK_BIN(frame));

	gtk_frame_set_shadow_type(GTK_FRAME(gtk_bin_get_child(GTK_BIN(pw))), GTK_SHADOW_ETCHED_OUT);
	gtk_widget_modify_bg(eb, GTK_STATE_NORMAL, &pwMain->style->bg[GTK_STATE_SELECTED]);

	if (SetupLanguage((char *) g_object_get_data(G_OBJECT(curSel), "lang")))
		/* Immediately translate this dialog */
		SetLangDialogText();
	else
		outputerrf(_("Locale '%s' not supported by C library."), (char *) g_object_get_data(G_OBJECT(curSel), "lang"));

	gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), TRUE);

	return FALSE;
}

static GtkWidget *GetFlagWidget(char *language, char *langCode, const char *flagfilename)
{	/* Create a flag */
	GtkWidget *eb, *eb2, *vbox, *lab1;
	GtkWidget *frame;
	char *file;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GError *pix_error = NULL;

	eb = gtk_event_box_new();
	gtk_widget_modify_bg(eb, GTK_STATE_INSENSITIVE, &pwMain->style->bg[GTK_STATE_NORMAL]);
	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_add( GTK_CONTAINER( eb ), frame );

	eb2 = gtk_event_box_new();
	gtk_widget_modify_bg(eb2, GTK_STATE_INSENSITIVE, &pwMain->style->bg[GTK_STATE_NORMAL]);
	gtk_container_add( GTK_CONTAINER( frame ), eb2 );

	vbox = gtk_vbox_new( FALSE, 5 );
	gtk_container_set_border_width( GTK_CONTAINER(vbox), 5);
	gtk_container_add( GTK_CONTAINER( eb2 ), vbox );

	if (flagfilename)
	{
		file = BuildFilename(flagfilename);
		pixbuf = gdk_pixbuf_new_from_file(file, &pix_error);

		if (pix_error)
			outputerrf("Failed to open flag: %s\n", file);
		else
		{
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
		}
                g_free(file);
	}
	lab1 = gtk_label_new(NULL);
	gtk_widget_set_size_request(lab1, 80, -1);
	gtk_box_pack_start (GTK_BOX (vbox), lab1, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(lab1), "lang", language);

	g_signal_connect(G_OBJECT(eb), "button_press_event", G_CALLBACK(FlagClicked), NULL);

	g_object_set_data(G_OBJECT(eb), "lang", langCode);

	return eb;
}

static int defclick(GtkWidget *pw, void *dummy, GtkWidget *table)
{
	gtk_widget_set_sensitive(table, FALSE);
	SetupLanguage("");
	SetLangDialogText();
	gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), TRUE);
	return FALSE;
}

static int selclick(GtkWidget *pw, void *dummy, GtkWidget *table)
{
	gtk_widget_set_sensitive(table, TRUE);
	if (curSel)
	{
		SetupLanguage((char*)g_object_get_data(G_OBJECT(curSel), "lang"));
		SetLangDialogText();
	}
	else
		gtk_widget_set_sensitive(DialogArea(pwLangDialog, DA_OK), FALSE);

	return FALSE;
}

static void SetWidgetLabelLang(GtkWidget *pw, char *text)
{
	if (GTK_IS_CONTAINER(pw))
	{
		GList *pl = gtk_container_get_children(GTK_CONTAINER(pw));
		while(pl)
		{
			SetWidgetLabelLang(pl->data, text);
			pl = pl->next;
		}
		g_list_free(pl);
	}
	if (GTK_IS_LABEL(pw))
		g_object_set_data(G_OBJECT(pw), "lang", text);
}

static void AddLangWidgets(GtkWidget *cont)
{
	int i, numLangs;
	GtkWidget *pwVbox, *pwHbox, *selLang = NULL;
	pwVbox = gtk_vbox_new( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER( cont ), pwVbox );

	pwLangRadio1 = gtk_radio_button_new_with_label(NULL, "");
	SetWidgetLabelLang(pwLangRadio1, N_("System default"));
	pwLangRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(pwLangRadio1), "");
	SetWidgetLabelLang(pwLangRadio2, N_("Select language"));
	gtk_box_pack_start (GTK_BOX (pwVbox), pwLangRadio1, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (pwVbox), pwLangRadio2, FALSE, FALSE, 0);

	numLangs = 0;
	while (*aaszLang[numLangs])
		numLangs++;
	numLangs--;	/* Don't count system default */

#define NUM_COLS 4	/* Display in 4 columns */
	pwLangTable = gtk_table_new(numLangs / NUM_COLS + 1, NUM_COLS, TRUE);

	for (i = 0; i < numLangs; i++)
	{
		GtkWidget *flag = GetFlagWidget(aaszLang[i + 1][0], aaszLang[i + 1][1], aaszLang[i + 1][2]);
		int row = i / NUM_COLS;
		int col = i - row * NUM_COLS;
		gtk_table_attach(GTK_TABLE(pwLangTable), flag, col, col + 1, row, row + 1, 0, 0, 0, 0);
		if (!StrCaseCmp(szLang, aaszLang[i + 1][1]))
		selLang = flag;
	}
	pwHbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pwVbox), pwHbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pwHbox), pwLangTable, FALSE, FALSE, 20);

	if (selLang == NULL)
		defclick(0, 0, pwLangTable);
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwLangRadio2), TRUE);
		FlagClicked(selLang, 0, 0);
	}
	g_signal_connect(G_OBJECT(pwLangRadio1), "button_press_event", G_CALLBACK(defclick), pwLangTable);
	g_signal_connect(G_OBJECT(pwLangRadio2), "button_press_event", G_CALLBACK(selclick), pwLangTable);
}


static void SetLanguage( gpointer p, guint n, GtkWidget *pw )
{
	GList *pl;

	pwLangDialog = GTKCreateDialog( NULL, DT_QUESTION, NULL, DIALOG_FLAG_MODAL, SetLangOk, NULL );

	pl = gtk_container_get_children( GTK_CONTAINER( DialogArea( pwLangDialog, DA_BUTTONS ) ) );
	SetWidgetLabelLang(GTK_WIDGET(pl->data), N_("Cancel"));
	SetWidgetLabelLang(GTK_WIDGET(pl->next->data), N_("OK"));
	g_list_free(pl);

	curSel = NULL;
	newLang = NULL;

	AddLangWidgets(DialogArea(pwLangDialog, DA_MAIN));

	GTKRunDialog(pwLangDialog);

	if (newLang)
		CommandSetLang(newLang);	/* Set new language (after dialog has closed) */
	else
		SetupLanguage(szLang);	/* If cancelled make sure language stays the same */
}


static void ReportBug(gpointer p, guint n, GtkWidget * pwEvent)
{
	OpenURL("http://savannah.gnu.org/bugs/?func=additem&group=gnubg");
}

GtkItemFactoryEntry aife[] = {
	{ N_("/_File"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_File/_New..."), "<control>N", NewClicked, 0,
		"<StockItem>", GTK_STOCK_NEW
	},
	{ N_("/_File/_Open..."), "<control>O", GTKOpen, 0, 
		"<StockItem>", GTK_STOCK_OPEN
	},
	{ N_("/_File/_Save..."), "<control>S", GTKSave, 0, 
		"<StockItem>", GTK_STOCK_SAVE
	},
	{ N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_File/Open _Commands..."), NULL, GTKCommandsOpen, 0, 
		NULL, NULL
	},
	{ N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_File/Match information..."), NULL, GTKMatchInfo, 0, NULL,
		NULL },
	{ N_("/_File/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ 
#ifdef WIN32
		N_("/_File/E_xit"),
#else
		N_("/_File/_Quit"),
#endif
		"<control>Q", Command, CMD_QUIT, "<StockItem>", GTK_STOCK_QUIT
	},
	{ N_("/_Edit"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Edit/_Undo"), "<control>Z", GTKUndo, 0, 
		"<StockItem>", GTK_STOCK_UNDO
	},
	{ N_("/_Edit/-"), NULL, NULL, 0, "<Separator>", NULL },

	{ N_("/_Edit/_Copy ID to Clipboard"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Edit/_Copy ID to Clipboard/GNUBG ID"), "<control>C", CopyIDs, 0, NULL, NULL }, 
	{ N_("/_Edit/_Copy ID to Clipboard/Match ID"), "<control>M", CopyMatchID, 0, NULL, NULL },
	{ N_("/_Edit/_Copy ID to Clipboard/Position ID"), "<control>P", CopyPositionID, 0, NULL, NULL },

	{ N_("/_Edit/Copy as"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Edit/Copy as/Position as ASCII"), NULL,
	  CommandCopy, 0, NULL, NULL },
	{ N_("/_Edit/Copy as/GammOnLine (HTML)"), NULL,
	  CopyAsGOL, 0, NULL, NULL },
	{ N_("/_Edit/Copy as/BackgammonBase.com (URL)"), NULL,
	  CopyAsBGbase, 0, NULL, NULL },

	{ N_("/_Edit/_Paste ID"), "<control>V", PasteIDs, 0,
		"<StockItem>", GTK_STOCK_PASTE},

	{ N_("/_Edit/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Edit/_Edit Position"), NULL, click_edit, 0,
		"<CheckItem>", NULL},

	{ N_("/_View"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_View/_Panels"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_View/_Panels/_Game record"), NULL, TogglePanel, TOGGLE_GAMELIST,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Panels/_Analysis"), NULL, TogglePanel, TOGGLE_ANALYSIS,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Panels/_Commentary"), NULL, TogglePanel, TOGGLE_COMMENTARY,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Panels/_Message"), NULL, TogglePanel, TOGGLE_MESSAGE,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Panels/_Theory"), NULL, TogglePanel, TOGGLE_THEORY,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Panels/_Command"), NULL, TogglePanel, TOGGLE_COMMAND,
	  "<CheckItem>", NULL },
	{ N_("/_View/_Dock panels"), NULL, ToggleDockPanels, 0, "<CheckItem>", NULL },
	{ N_("/_View/Restore panels"), NULL, ShowAllPanels, 0, NULL, NULL },
	{ N_("/_View/Hide panels"), NULL, HideAllPanels, 0, NULL, NULL },
	{ N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_View/Show _ID in status bar"), NULL, ToggleShowingIDs, 0, "<CheckItem>", NULL },
	{ N_("/_View/_Toolbar"), NULL, NULL, 0, "<Branch>", NULL},
	{ N_("/_View/_Toolbar/_Hide Toolbar"), NULL, HideToolbar, 0, NULL, NULL },
	{ N_("/_View/_Toolbar/_Show Toolbar"), NULL, ShowToolbar, 0, NULL, NULL },
	{ N_("/_View/_Toolbar/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_View/_Toolbar/_Text only"), NULL, ToolbarStyle, TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_TEXT,
	  "<RadioItem>", NULL },
	{ N_("/_View/_Toolbar/_Icons only"), NULL, ToolbarStyle, TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_ICONS,
	  "/View/Toolbar/Text only", NULL },
	{ N_("/_View/_Toolbar/_Both"), NULL, ToolbarStyle, TOOLBAR_ACTION_OFFSET + GTK_TOOLBAR_BOTH,
	  "/View/Toolbar/Text only", NULL },
	{ N_("/_View/Full screen"), "F11", DoFullScreenMode, 0, "<CheckItem>", NULL },
	{ N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_View/Play _Clockwise"), NULL, click_swapdirection, 0, "<CheckItem>", NULL },
#if USE_BOARD3D
	{ N_("/_View/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_View/Switch to xD view"), NULL, SwitchDisplayMode, TOOLBAR_ACTION_OFFSET + MENU_OFFSET, NULL, NULL },
#endif
	{ N_("/_Game"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Game/_Roll"), "<control>R", Command, CMD_ROLL, NULL, NULL },
	{ N_("/_Game/_Finish move"), "<control>F", FinishMove, 0, NULL, NULL },
	{ N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Game/_Double"), "<control>D", Command, CMD_DOUBLE, "<StockItem>", GNUBG_STOCK_DOUBLE },
	{ N_("/_Game/Re_sign"), NULL, GTKResign, 0, "<StockItem>", GNUBG_STOCK_RESIGN },
	{ N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Game/_Accept"), NULL, Command, CMD_ACCEPT, "<StockItem>", GNUBG_STOCK_ACCEPT },
	{ N_("/_Game/Re_ject"), NULL, Command, CMD_REJECT, "<StockItem>", GNUBG_STOCK_REJECT },
	{ N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Game/Play computer turn"), NULL, Command, CMD_PLAY, NULL,
		NULL },
	{ N_("/_Game/_End Game"), "<control>G", Command, CMD_END_GAME, "<StockItem>", GNUBG_STOCK_END_GAME },
	{ N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Game/Swap players"), NULL, Command, CMD_SWAP_PLAYERS, NULL,
		NULL },
	{ N_("/_Game/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Game/Set cube..."), NULL, GTKSetCube, 0, NULL, NULL },
	{ N_("/_Game/Set _dice..."), NULL, GTKSetDice, 0, NULL, NULL },
	{ N_("/_Game/Set _turn"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Game/Set turn/0"), 
          NULL, Command, CMD_SET_TURN_0, "<RadioItem>", NULL },
	{ N_("/_Game/Set turn/1"), NULL, Command, CMD_SET_TURN_1,
	  "/Game/Set turn/0", NULL },
	{ N_("/_Game/Clear turn"), NULL, Command, CMD_CLEAR_TURN, NULL, NULL },
	{ N_("/_Analyse"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Analyse/_Evaluate"), "<control>E", Command, CMD_EVAL, NULL, NULL },
	{ N_("/_Analyse/_Hint"), "<control>H", Command, CMD_HINT,
		"<StockItem>", GNUBG_STOCK_HINT},
	{ N_("/_Analyse/_Rollout"), NULL, Command, CMD_ROLLOUT, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/Analyse move"), 
          NULL, Command, CMD_ANALYSE_MOVE, NULL, NULL },
	{ N_("/_Analyse/Analyse game"), 
          NULL, Command, CMD_ANALYSE_GAME, NULL, NULL },
	{ N_("/_Analyse/Analyse match or session"), 
          NULL, Command, CMD_ANALYSE_MATCH, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
        { N_("/_Analyse/Clear analysis"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/Clear analysis/Move"), 
          NULL, Command, CMD_ANALYSE_CLEAR_MOVE, 
		"<StockItem>", GTK_STOCK_CLEAR
	},
        { N_("/_Analyse/Clear analysis/_Game"), 
          NULL, Command, CMD_ANALYSE_CLEAR_GAME,
		"<StockItem>", GTK_STOCK_CLEAR
	},
        { N_("/_Analyse/Clear analysis/_Match or session"), 
          NULL, Command, CMD_ANALYSE_CLEAR_MATCH,
		"<StockItem>", GTK_STOCK_CLEAR
	},
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
        { N_("/_Analyse/CMark"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/CMark/Cube"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/CMark/Cube/Clear"), NULL, Command, CMD_CMARK_CUBE_CLEAR, NULL, NULL },
        { N_("/_Analyse/CMark/Cube/Show"), NULL, Command, CMD_CMARK_CUBE_SHOW, NULL, NULL },
        { N_("/_Analyse/CMark/Move"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/CMark/Move/Clear"), NULL, Command, CMD_CMARK_MOVE_CLEAR, NULL, NULL },
        { N_("/_Analyse/CMark/Move/Show"), NULL, Command, CMD_CMARK_MOVE_SHOW, NULL, NULL },
        { N_("/_Analyse/CMark/Game"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/CMark/Game/Clear"), NULL, Command, CMD_CMARK_GAME_CLEAR, NULL, NULL },
        { N_("/_Analyse/CMark/Game/Show"), NULL, Command, CMD_CMARK_GAME_SHOW, NULL, NULL },
        { N_("/_Analyse/CMark/Match"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/CMark/Match/Clear"), NULL, Command, CMD_CMARK_MATCH_CLEAR, NULL, NULL },
        { N_("/_Analyse/CMark/Match/Show"), NULL, Command, CMD_CMARK_MATCH_SHOW, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
        { N_("/_Analyse/Rollout"), NULL, NULL, 0, "<Branch>", NULL },
        { N_("/_Analyse/Rollout/Cube"), NULL, Command, CMD_ANALYSE_ROLLOUT_CUBE, NULL, NULL },
        { N_("/_Analyse/Rollout/Move"), NULL, Command, CMD_ANALYSE_ROLLOUT_MOVE, NULL, NULL },
        { N_("/_Analyse/Rollout/Game"), NULL, Command, CMD_ANALYSE_ROLLOUT_GAME, NULL, NULL },
        { N_("/_Analyse/Rollout/Match"), NULL, Command, CMD_ANALYSE_ROLLOUT_MATCH, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/Batch analyse..."), NULL, GTKBatchAnalyse, 0, NULL,
		NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/Match or session statistics"), NULL, Command,
          CMD_SHOW_STATISTICS_MATCH, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
    { N_("/_Analyse/Add match or session to database"), NULL,
        GtkRelationalAddMatch, 0,
	"<StockItem>", GTK_STOCK_ADD},
    { N_("/_Analyse/Show Records"), NULL,
        GtkShowRelational, 0, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/Distribution of rolls"), NULL, Command, 
          CMD_SHOW_ROLLS, NULL, NULL },
	{ N_("/_Analyse/Temperature Map"), NULL, Command, 
          CMD_SHOW_TEMPERATURE_MAP, NULL, NULL },
	{ N_("/_Analyse/Temperature Map (cube decision)"), NULL, Command, 
          CMD_SHOW_TEMPERATURE_MAP_CUBE, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/_Race Theory"), 
          NULL, Command, CMD_SHOW_KLEINMAN, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/_Market window"), NULL, Command, CMD_SHOW_MARKETWINDOW,
	  NULL, NULL },
	{ N_("/_Analyse/M_atch equity table"), NULL, Command,
	  CMD_SHOW_MATCHEQUITYTABLE, NULL, NULL },
	{ N_("/_Analyse/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Analyse/Evaluation speed"), NULL, Command,
	  CMD_SHOW_CALIBRATION, NULL, NULL },
	{ N_("/_Settings"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Settings/_Analysis..."), NULL, SetAnalysis, 0, NULL, NULL },
	{ N_("/_Settings/_Board Appearance..."), NULL, Command, CMD_SET_APPEARANCE,
	  NULL, NULL },
    { N_("/_Settings/E_xport..."), NULL, Command, CMD_SHOW_EXPORT,
      NULL, NULL },
	{ N_("/_Settings/_Players..."), NULL, SetPlayers, 0, NULL, NULL },
	{ N_("/_Settings/_Rollouts..."), NULL, SetRollouts, 0, NULL, NULL },
	{ N_("/_Settings/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Settings/_Options..."), NULL, SetOptions, 0, NULL, NULL },
	{ N_("/_Settings/_Language..."), NULL, SetLanguage, 0, NULL, NULL },
	{ N_("/G_o"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/G_o/Previous marked move"), "<shift><control>Page_Up", Command, CMD_PREV_MARKED, "<StockItem>", GNUBG_STOCK_GO_PREV_MARKED },
	{ N_("/G_o/Previous cmarked move"), "<shift>Page_Up", Command, CMD_PREV_CMARKED, "<StockItem>", GNUBG_STOCK_GO_PREV_CMARKED },
	{ N_("/G_o/Previous rol_l"), "Page_Up", Command, CMD_PREV_ROLL, "<StockItem>", GNUBG_STOCK_GO_PREV },
	{ N_("/G_o/Pre_vious game"), "<control>Page_Up", Command, CMD_PREV_GAME, "<StockItem>", GNUBG_STOCK_GO_PREV_GAME },
	{ N_("/G_o/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/G_o/Next _game"), "<control>Page_Down", Command, CMD_NEXT_GAME, "<StockItem>", GNUBG_STOCK_GO_NEXT_GAME },
	{ N_("/G_o/Next _roll"), "Page_Down", Command, CMD_NEXT_ROLL, "<StockItem>", GNUBG_STOCK_GO_NEXT },
	{ N_("/G_o/Next cmarked move"), "<shift>Page_Down", Command, CMD_NEXT_CMARKED, "<StockItem>", GNUBG_STOCK_GO_NEXT_CMARKED },
	{ N_("/G_o/Next marked move"), "<shift><control>Page_Down", Command, CMD_NEXT_MARKED, "<StockItem>", GNUBG_STOCK_GO_NEXT_MARKED },
	{ N_("/_Help"), NULL, NULL, 0, "<Branch>", NULL },
	{ N_("/_Help/_Commands"), NULL, Command, CMD_HELP, NULL, NULL },
	{ N_("/_Help/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Help/_Manual (all about)"), NULL, Command, 
          CMD_SHOW_MANUAL_ABOUT, NULL, NULL },
	{ N_("/_Help/Manual (_web)"), NULL, Command, 
          CMD_SHOW_MANUAL_WEB, NULL, NULL },
	{ N_("/_Help/-"), NULL, NULL, 0, "<Separator>", NULL },
	{ N_("/_Help/_About gnubg"), NULL, Command, CMD_SHOW_VERSION,
		"<StockItem>", GTK_STOCK_ABOUT
	}
};

extern int automaticTask;

static void Stop( GtkWidget *pw, gpointer unused )
{
	if (automaticTask)
		StopAutomaticPlay();
	else if (!GTKShowWarning(WARN_STOP, pw))
		return;

	fInterrupt = TRUE;
#if USE_BOARD3D
{
	BoardData *bd = BOARD( pwBoard )->board_data;
	if (display_is_3d(bd->rd))
	{
		StopIdle3d(bd, bd->bd3d);
		RestrictiveRedraw();
	}
}
#endif
	gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, _("Process interrupted") );
}

static gboolean StopAnyAnimations(void)
{
#if USE_BOARD3D
	BoardData *bd = BOARD( pwBoard )->board_data;

	if (display_is_3d(bd->rd))
	{
		if (Animating3d(bd->bd3d))
		{
			StopIdle3d(bd, bd->bd3d);
			RestrictiveRedraw();
			return TRUE;
		}
	}
	else
#endif
	if (!animation_finished)
	{
		fInterrupt = TRUE;
		return TRUE;
	}
	return FALSE;
}

static void StopNotButton( GtkWidget *pw, gpointer unused )
{	/* Interrupt any animations or show message in status bar */
	if (!StopAnyAnimations())
		gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, _("Press the stop button to interrupt the current process") );
}

static void FileDragDropped(GtkWidget *widget, GdkDragContext * drag_context,
			    gint x, gint y, GtkSelectionData * data, guint info, guint time)
{
	if (data->length > 0) {
		char *next, *file, *quoted;
		char *uri = (char *)data->data;
		if (StrNCaseCmp("file:", uri, 5) != 0) {
			outputerrf(_("Only local files supported in dnd"));
			return;
		}

		file = g_filename_from_uri(uri, NULL, NULL);

		if (!file) {
			outputerrf(_("Failed to parse uri"));
			return;
		}

		next = strchr(file, '\r');
		if (next)
			*next = 0;
		quoted = g_strdup_printf("\"%s\"", file);
		CommandImportAuto(quoted);
		g_free(quoted);
		g_free(file);
	}
}


static gboolean ContextMenu(GtkWidget *widget, GdkEventButton *event, GtkWidget* menu)
{
	if (event->type != GDK_BUTTON_PRESS || event->button != 3)
		return FALSE;

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);

	return TRUE;
}

static void CreateMainWindow(void)
{
	GtkWidget *pwVbox, *pwHbox, *pwHbox2, *pwHandle, *pwPanelHbox, *pwStopButton, *idMenu, *menu_item, *pwFrame;
#ifdef GTK_TARGET_OTHER_APP	/* gtk 2.12+ */
	GtkTargetEntry fileDrop = {"text/uri-list", GTK_TARGET_OTHER_APP, 1};
#else
	GtkTargetEntry fileDrop = {"text/uri-list", 0, 1};
#endif

    pwMain = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_window_maximize(GTK_WINDOW(pwMain));
	SetPanelWidget(WINDOW_MAIN, pwMain);
    gtk_window_set_role( GTK_WINDOW( pwMain ), "main" );
    gtk_window_set_type_hint( GTK_WINDOW( pwMain ),
			      GDK_WINDOW_TYPE_HINT_NORMAL );
    gtk_window_set_title( GTK_WINDOW( pwMain ), _("GNU Backgammon") );
	/* Enable dropping of files on to main window */
	gtk_drag_dest_set(pwMain, GTK_DEST_DEFAULT_ALL, &fileDrop, 1, GDK_ACTION_DEFAULT);
	g_signal_connect(G_OBJECT(pwMain), "drag_data_received", G_CALLBACK(FileDragDropped), NULL);

    gtk_container_add( GTK_CONTAINER( pwMain ), pwVbox = gtk_vbox_new( FALSE, 0 ) );

    pagMain = gtk_accel_group_new();
    pif = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<main>", pagMain );

    gtk_item_factory_set_translate_func ( pif, GTKTranslate, NULL, NULL );

    gtk_item_factory_create_items( pif, sizeof( aife ) / sizeof( aife[ 0 ] ), aife, NULL );

	/* Tick default toolbar style */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget_by_action(pif, nToolbarStyle + TOOLBAR_ACTION_OFFSET)), TRUE);
    gtk_window_add_accel_group( GTK_WINDOW( pwMain ), pagMain );

    gtk_box_pack_start( GTK_BOX( pwVbox ),
			pwHandle = gtk_handle_box_new(),
			FALSE, FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( pwHandle ),
			pwMenuBar = gtk_item_factory_get_widget( pif,
								 "<main>" ));

   gtk_box_pack_start( GTK_BOX( pwVbox ),
                       pwHandle = gtk_handle_box_new(), FALSE, TRUE, 0 );
   gtk_container_add( GTK_CONTAINER( pwHandle ),
                       pwToolbar = ToolbarNew() );
    
   gtk_box_pack_start( GTK_BOX( pwVbox ),
			pwGameBox = gtk_hbox_new(FALSE, 0),
			TRUE, TRUE, 0 );
   gtk_box_pack_start( GTK_BOX( pwVbox ),
			hpaned = gtk_hpaned_new(),
			TRUE, TRUE, 0 );

   gtk_paned_add1(GTK_PANED(hpaned), pwPanelGameBox = gtk_hbox_new(FALSE, 0));
   gtk_container_add(GTK_CONTAINER(pwPanelGameBox), pwEventBox = gtk_event_box_new());
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(pwEventBox), FALSE);

   gtk_container_add(GTK_CONTAINER(pwEventBox), pwBoard = board_new(GetMainAppearance()));
   g_signal_connect(G_OBJECT(pwEventBox), "button-press-event", G_CALLBACK(board_button_press),
	   BOARD(pwBoard)->board_data);

   pwPanelHbox = gtk_hbox_new(FALSE, 0);
   gtk_paned_pack2(GTK_PANED(hpaned), pwPanelHbox, FALSE, FALSE);
   gtk_box_pack_start( GTK_BOX( pwPanelHbox ), pwPanelVbox = gtk_vbox_new( FALSE, 1 ), TRUE, TRUE, 0);

   DockPanels();

   /* Status bar */
					
   gtk_box_pack_end( GTK_BOX( pwVbox ), pwHbox = gtk_hbox_new( FALSE, 0 ), FALSE, FALSE, 0 );

    gtk_box_pack_start( GTK_BOX( pwHbox ), pwStatus = gtk_statusbar_new(),
		      TRUE, TRUE, 0 );

    gtk_statusbar_set_has_resize_grip( GTK_STATUSBAR( pwStatus ), FALSE );
    /* It's a bit naughty to access pwStatus->label, but its default alignment
       is ugly, and GTK gives us no other way to change it. */
    gtk_misc_set_alignment( GTK_MISC( GTK_STATUSBAR( pwStatus )->label ),
			    0.0f, 0.5f );
    idOutput = gtk_statusbar_get_context_id( GTK_STATUSBAR( pwStatus ),
					     "gnubg output" );
    idProgress = gtk_statusbar_get_context_id( GTK_STATUSBAR( pwStatus ),
					       "progress" );
    g_signal_connect( G_OBJECT( pwStatus ), "text-popped",
			G_CALLBACK( TextPopped ), NULL );

	idMenu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label (_("Copy Position ID"));
	gtk_menu_shell_append (GTK_MENU_SHELL (idMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( CopyIDs ), NULL );

	menu_item = gtk_menu_item_new_with_label (_("Paste Position ID"));
	gtk_menu_shell_append (GTK_MENU_SHELL (idMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( PasteIDs ), NULL );

    pwIDBox = gtk_event_box_new();
	gtk_box_pack_start( GTK_BOX( pwHbox ), pwIDBox, FALSE, FALSE, 0 );

	pwHbox2 = gtk_hbox_new( FALSE, 0 );
	gtk_container_add(GTK_CONTAINER(pwIDBox), pwHbox2);

	gtk_box_pack_start( GTK_BOX( pwHbox2 ), gtk_label_new("Gnubg id:"), FALSE, FALSE, 0 );
	pwFrame = gtk_frame_new(NULL);
	gtk_box_pack_start( GTK_BOX( pwHbox2 ), pwFrame, FALSE, FALSE, 0 );

	pwGnubgID = gtk_label_new("");
	gtk_container_add(GTK_CONTAINER(pwFrame), pwGnubgID);
	gtk_container_set_border_width(GTK_CONTAINER(pwFrame), 2);

	gtk_widget_set_tooltip_text(pwIDBox, _("This is a unique id for this position."
		" Ctrl+C copies the current ID and Ctrl+V pastes an ID from the clipboard"));
	g_signal_connect( G_OBJECT( pwIDBox ), "button-press-event", G_CALLBACK( ContextMenu ), idMenu );

    pwStop = gtk_event_box_new();
    pwStopButton = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(pwStop), pwStopButton);
    gtk_container_add(GTK_CONTAINER(pwStopButton), gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_box_pack_start( GTK_BOX( pwHbox ), pwStop, FALSE, FALSE, 2 );
	g_signal_connect(G_OBJECT(pwStop), "button-press-event", G_CALLBACK( StopNotButton ), NULL );
	g_signal_connect(G_OBJECT(pwStopButton), "button-press-event", G_CALLBACK( Stop ), NULL );

	pwGrab = pwStop;

    gtk_box_pack_start( GTK_BOX( pwHbox ),
			pwProgress = gtk_progress_bar_new(),
			FALSE, FALSE, 0 );
    gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( pwProgress ), 0.0 );
    /* This is a kludge to work around an ugly bug in GTK: we don't want to
       show text in the progress bar yet, but we might later.  So we have to
       pretend we want text in order to be sized correctly, and then set the
       format string to something so we don't get the default text. */
    gtk_progress_bar_set_text( GTK_PROGRESS_BAR( pwProgress ), " " );

    g_signal_connect(G_OBJECT(pwMain), "configure_event",
			G_CALLBACK(configure_event), NULL);
    g_signal_connect( G_OBJECT( pwMain ), "size-request",
			G_CALLBACK( MainSize ), NULL );
    g_signal_connect( G_OBJECT( pwMain ), "delete_event",
			G_CALLBACK( main_delete ), NULL );
    g_signal_connect( G_OBJECT( pwMain ), "destroy",
			G_CALLBACK( gtk_main_quit ), NULL );
}

static void gnubg_set_default_icon(void)
{
	/* win32 uses the ico file for this */
	/* adapted from pidgin */
#ifndef WIN32
	GList *icons = NULL;
	GdkPixbuf *icon = NULL;
	char *ip;
	guint i;
	struct {
		const char *dir;
		const char *fn;
	} is[] = {
	       	{"16x16", "gnubg.png"},
	       	{"24x24", "gnubg.png"},
	       	{"32x32", "gnubg.png"},
	       	{"48x48", "gnubg.png"}
	};

	for (i = 0; i < G_N_ELEMENTS(is); i++) {
		ip = g_build_filename(getDataDir(), "icons", "hicolor", is[i].dir, "apps", is[i].fn, NULL);
		icon = gdk_pixbuf_new_from_file(ip, NULL);
		g_free(ip);
		if (icon)
			icons = g_list_append(icons, icon);
		/* fail silently */

	}
	gtk_window_set_default_icon_list(icons);

	g_list_foreach(icons, (GFunc) g_object_unref, NULL);
	g_list_free(icons);
#endif
}
extern void InitGTK(int *argc, char ***argv)
{
	char *sz;
	GtkIconFactory *pif;
	GdkAtom cb;

	sz = BuildFilename("gnubg.gtkrc");
	gtk_rc_add_default_file(sz);
	g_free(sz);

	sz = g_build_filename(szHomeDirectory, "gnubg.gtkrc", NULL);
	gtk_rc_add_default_file(sz);
	g_free(sz);

	sz = g_build_filename(szHomeDirectory, "gnubgmenurc", NULL);
	gtk_accel_map_load(sz);
	g_free(sz);

	fX = gtk_init_check(argc, argv);
	if (!fX)
		return;

	gnubg_stock_init();

#if USE_BOARD3D
	InitGTK3d(argc, argv);
#endif

	/*add two xpm based icons*/
	pif = gtk_icon_factory_new();
	gtk_icon_factory_add_default(pif);

#if (GTK_MAJOR_VERSION < 3) && (GTK_MINOR_VERSION < 12)
	ptt = gtk_tooltips_new();
#endif
	
	gnubg_set_default_icon();

	CreateMainWindow();

	/*Create string for handling messages from output* functions*/
	output_str = g_string_new(NULL);

	cb = gdk_atom_intern("CLIPBOARD", TRUE);
	clipboard = gtk_clipboard_get(cb);
}


#ifndef WIN32
static gint python_run_file (gpointer file)
{ 
	char *pch;
        g_assert(file);
        pch = g_strdup_printf(">import sys;"
		   "sys.argv=['','-n', '%s'];"
		   "import idlelib.PyShell;" "idlelib.PyShell.main()", (char *)file);
	UserCommand(pch);
	g_free(pch);
        g_free(file);
        return FALSE;
}
#endif

enum {RE_NONE, RE_LANGUAGE_CHANGE};

int reasonExited;

extern void RunGTK( GtkWidget *pwSplash, char *commands, char *python_script, char *match )
{
#if USE_BOARD3D
	/* Use 1st predefined board settings if none have been loaded */
	Default3dSettings(BOARD(pwBoard)->board_data);
#endif

	do
	{
		reasonExited = RE_NONE;

		GTKSet( &ms.fCubeOwner );
		GTKSet( &ms.nCube );
		GTKSet( ap );
		GTKSet( &ms.fTurn );
		GTKSet( &ms.gs );
		GTKSet( &ms.fJacoby );
	    
		PushSplash ( pwSplash, _("Rendering"), _("Board") );

		GTKAllowStdin();
	    
		if( fTTY ) {
#if HAVE_LIBREADLINE
			fReadingCommand = TRUE;
			rl_callback_handler_install( FormatPrompt(), ProcessInput );
			atexit( rl_callback_handler_remove );
#else
			Prompt();
#endif
		}

		/* Show everything */
		gtk_widget_show_all( pwMain );

		GTKSet( &fShowIDs);

		/* Set the default arrow cursor in the stop window so obvious it can be clicked */
		gdk_window_set_cursor(pwStop->window, gdk_cursor_new(GDK_ARROW));

		/* Make sure toolbar looks correct */
		{
			int style = nToolbarStyle;
			nToolbarStyle = 2;	/* Default style is fine */
			SetToolbarStyle(style);
		}

#if USE_BOARD3D
	{
		BoardData *bd = BOARD( pwBoard )->board_data;
		BoardData3d *bd3d = bd->bd3d;
		renderdata *prd = bd->rd;

		SetSwitchModeMenuText();
		DisplayCorrectBoardType(bd, bd3d, prd);
	}
#endif

		DestroySplash ( pwSplash );
		pwSplash = NULL;

		/* Display any other windows now */
		DisplayWindows();

		/* Make sure some things stay hidden */
		if (!ArePanelsDocked())
		{
			gtk_widget_hide(hpaned);
			gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Panels/Commentary"));
			gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Hide panels"));
			gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Restore panels"));
		}
		else 
		{
			if (ArePanelsShowing())
			{
				gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Restore panels"));
				gtk_widget_hide(pwGameBox);
			}
			else
				gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Hide panels"));
		}

		/* Make sure main window is on top */
		gdk_window_raise(pwMain->window);

		/* force update of board; needed to display board correctly if user
			has special settings, e.g., clockwise or nackgammon */
		ShowBoard();

		if (fToolbarShowing)
			gtk_widget_hide(gtk_item_factory_get_widget(pif, "/View/Toolbar/Show Toolbar"));

		if (fFullScreen)
		{	/* Change to full screen (but hide warning) */
			fullScreenOnStartup = TRUE;
			FullScreenMode(TRUE);
		}
		else if (!fToolbarShowing)
			HideToolbar();

        if (match)
        {
                CommandImportAuto(match);
                g_free(match);
                match = NULL;
        }

        if (commands)
        {
                CommandLoadCommands(commands);
                g_free(commands);
                commands = NULL;
        }

        if (python_script)
        {
#ifdef WIN32
			outputerrf(_("The MS windows GTK interface doesn't support the '-p' option. Use the cl interface instead"));
#else
			g_idle_add( python_run_file, g_strdup(python_script) );
#endif
			g_free(python_script);
			python_script = NULL;
		}

		gtk_main();

		if (reasonExited == RE_LANGUAGE_CHANGE)
		{	/* Recreate main window with new language */
			CreateMainWindow();
			setWindowGeometry(WINDOW_MAIN);
		}
	} while (reasonExited != RE_NONE);
}

extern void GtkChangeLanguage(void)
{
	gtk_set_locale();
	if (pwMain && gtk_widget_get_realized(pwMain))
	{
		reasonExited = RE_LANGUAGE_CHANGE;
		custom_cell_renderer_invalidate_size();	/* Recalulate widget sizes */
		ClosePanels();
		getWindowGeometry(WINDOW_MAIN);
		DestroyPanel(WINDOW_MAIN);
		GTKGameSelectDestroy();
		pwProgress = NULL;

	}
}

extern void ShowList(char *psz[], const char *szTitle, GtkWidget * parent)
{

 	GString *gst = g_string_new(NULL);
 	while (*psz)
 		g_string_append_printf(gst, "%s\n", *psz++);
 	GTKTextWindow(gst->str, szTitle, DT_INFO, parent);
	g_string_free(gst, TRUE);
}

extern void OK( GtkWidget *pw, int *pf ) {

    if( pf )
	*pf = TRUE;
    
    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

/*
 * put up a tutor window with a message about how bad the intended
 * play is. Allow selections:
 * Continue (play it anyway)
 * Cancel   (rethink)
 * returns TRUE if play it anyway
 */

static void TutorEnd( GtkWidget *pw, int *pf ) {

    if( pf )
	*pf = TRUE;
    
	fTutor = FALSE;
    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static void
TutorHint ( GtkWidget *pw, void *unused ) {

  gtk_widget_destroy ( gtk_widget_get_toplevel( pw ) );
  UserCommand ( "hint" );

}

static void
TutorRethink ( GtkWidget *pw, void *unused ) {

#if USE_BOARD3D
	RestrictiveRedraw();
#endif
  gtk_widget_destroy ( gtk_widget_get_toplevel( pw ) );
  ShowBoard ();

}

extern int GtkTutor ( char *sz )
{
    int f = FALSE;
    GtkWidget *pwTutorDialog, *pwOK, *pwCancel, *pwEndTutor,
          *pwButtons, *pwPrompt, *pwHint;

	pwTutorDialog = GTKCreateDialog( _("GNU Backgammon - Tutor"),
		DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(OK), (void*)&f);

	pwOK = DialogArea(pwTutorDialog, DA_OK);
	gtk_button_set_label(GTK_BUTTON(pwOK), _("Play Anyway"));

	pwCancel = gtk_button_new_with_label( _("Rethink") );
	pwEndTutor = gtk_button_new_with_label ( _("End Tutor Mode") );
	pwHint = gtk_button_new_with_label ( _("Hint") );
	pwButtons = DialogArea(pwTutorDialog, DA_BUTTONS);

	gtk_container_add( GTK_CONTAINER( pwButtons ), pwCancel );
	g_signal_connect( G_OBJECT( pwCancel ), "clicked",
				   G_CALLBACK( TutorRethink ),
				   (void *) &f );

	gtk_container_add( GTK_CONTAINER( pwButtons ), pwEndTutor );
	g_signal_connect( G_OBJECT( pwEndTutor ), "clicked",
				   G_CALLBACK( TutorEnd ), (void *) &f );

	gtk_container_add( GTK_CONTAINER( pwButtons ), pwHint );
	g_signal_connect( G_OBJECT( pwHint ), "clicked",
				   G_CALLBACK( TutorHint ), (void *) &f );
    
    pwPrompt = gtk_label_new( sz );

    gtk_misc_set_padding( GTK_MISC( pwPrompt ), 8, 8 );
    gtk_label_set_justify( GTK_LABEL( pwPrompt ), GTK_JUSTIFY_LEFT );
    gtk_label_set_line_wrap( GTK_LABEL( pwPrompt ), TRUE );
    gtk_container_add( GTK_CONTAINER( DialogArea( pwTutorDialog, DA_MAIN ) ),
		       pwPrompt );
    
    gtk_window_set_resizable( GTK_WINDOW( pwTutorDialog ), FALSE);
    
    /* This dialog should be REALLY modal -- disable "next turn" idle
       processing and stdin handler, to avoid reentrancy problems. */
    if( nNextTurn ) 
      g_source_remove( nNextTurn );
    
	GTKRunDialog(pwTutorDialog);
    
    if( nNextTurn ) 
      nNextTurn = g_idle_add( NextTurnNotify, NULL );
    
    /* if tutor mode was disabled, update the checklist */
    if ( !fTutor) {
      GTKSet ( (void *) &fTutor);
    }
    
    return f;
}

extern void GTKOutput( const char *sz ) {
    if( !sz || !*sz )
	return;
    g_string_append(output_str, sz);
}

extern void GTKOutputX( void ) {
	gchar *str;
	if (output_str->len == 0)
		return;
	str = g_strchomp(output_str->str);
	if (PanelShowing(WINDOW_MESSAGE))
	{
		GtkTextBuffer *buffer;
		GtkTextIter iter;
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwMessageText));
		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_buffer_insert( buffer, &iter, "\n", -1);
		gtk_text_buffer_insert( buffer, &iter, g_strchomp(str), -1);
		gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW(pwMessageText),
				gtk_text_buffer_create_mark (buffer, "last", &iter, FALSE),
				0.0, TRUE, 0.0, 1.0 );
	}
	else if( output_str->len > 80 || strchr( str, '\n' )) {
		GTKMessage( str, DT_INFO );
	}
	else
	{
		gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idOutput, str );
	}
	g_string_set_size(output_str, 0);
}

extern void GTKOutputErr( const char *sz ) {

    GtkTextBuffer *buffer;
    GtkTextIter iter;
    GTKMessage( (char*)sz, DT_ERROR );
    
    if (PanelShowing(WINDOW_MESSAGE)) {
      buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwMessageText));
      gtk_text_buffer_get_end_iter (buffer, &iter);
      gtk_text_buffer_insert( buffer, &iter, sz, -1);
      gtk_text_buffer_insert( buffer, &iter, "\n", -1);
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW(pwMessageText), &iter, 0.0, FALSE, 0.0, 1.0 );
    }
}

extern void GTKOutputNew( void ) {

    /* This is horribly ugly, but fFinishedPopping will never be set if
       the progress bar leaves a message in the status stack.  There should
       be at most one message, so we get rid of it here. */
    gtk_statusbar_pop( GTK_STATUSBAR( pwStatus ), idProgress );
    
    fFinishedPopping = FALSE;
    
    do
	gtk_statusbar_pop( GTK_STATUSBAR( pwStatus ), idOutput );
    while( !fFinishedPopping );
}

typedef struct _newwidget {
  GtkWidget *pwCPS, *pwML, *pwGNUvsHuman,
      *pwHumanHuman, *pwManualDice, *pwTutorMode;
} newwidget;

static GtkWidget *
button_from_image ( GtkWidget *pwImage ) {

  GtkWidget *pw = gtk_button_new ();

  gtk_container_add ( GTK_CONTAINER ( pw ), pwImage );

  return pw;

}
static void UpdatePlayerSettings( newwidget *pnw ) {
	
  int fManDice = 
	  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwManualDice));
  int fTM = 
	  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwTutorMode));
  int fCPS = 
	  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwCPS));
  int fGH = 
	  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwGNUvsHuman));
  int fHH = 
	  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pnw->pwHumanHuman));
  
  if(!fCPS){
	  if (fGH){
		  UserCommand("set player 0 gnubg");
		  UserCommand("set player 1 human");
	  }
	  if (fHH){
		  UserCommand("set player 0 human");
		  UserCommand("set player 1 human");
	  }
  }

  
  if((fManDice) && (rngCurrent != RNG_MANUAL))
	  UserCommand("set rng manual");
  
  if((!fManDice) && (rngCurrent == RNG_MANUAL))
	  UserCommand("set rng mersenne");
  
  if((fTM) && (!fTutor))
	  UserCommand("set tutor mode on");

  if((!fTM) && (fTutor))
	  UserCommand("set tutor mode off");

  UserCommand("save settings");
}

static void SettingsPressed( GtkWidget *pw, gpointer data )
{
	GTKSetCurrentParent(pw);
	SetPlayers( NULL, 0, NULL);
}

static void ToolButtonPressedMS( GtkWidget *pw, newwidget *pnw ) {
  UpdatePlayerSettings( pnw ); 
  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
  UserCommand("new session");
}

static void ToolButtonPressed( GtkWidget *pw, newwidget *pnw ) {
  char sz[40];
  int *pi;

  pi = (int *) g_object_get_data ( G_OBJECT ( pw ), "user_data" );
  sprintf(sz, "new match %d", *pi);
  UpdatePlayerSettings( pnw );
  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
  UserCommand(sz);
}

extern int edit_new(unsigned int length)
{
	matchstate ms;

	ms.anDice[0] = ms.anDice[1] = 0;
	ms.fTurn = ms.fMove = 1;
	ms.fResigned = 0;
	ms.fDoubled = 0;
	ms.fCubeOwner = -1;
	ms.fCrawford = FALSE;
	ms.fJacoby = fJacoby;
	ms.anScore[0] = ms.anScore[1] = 0; 
	ms.nCube = 0;
	ms.gs = GAME_PLAYING;

	ms.nMatchTo = length;

	CommandSetMatchID(MatchIDFromMatchState(&ms));

	return 0;
}

static void edit_new_clicked(GtkWidget * pw, newwidget * pnw)
{
	unsigned int length = (unsigned int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(pnw->pwML));
	gtk_widget_destroy(gtk_widget_get_toplevel(pw));
	edit_new(length);
	if (!ToolbarIsEditing(NULL))
		click_edit();
}

static GtkWidget *NewWidget( newwidget *pnw)
{
  int i, j = 1 ;
  GtkWidget *pwVbox, *pwHbox, *pwLabel, *pwToolbar;
  GtkWidget *pwButtons, *pwFrame, *pwVbox2; 
  pwVbox = gtk_vbox_new(FALSE, 0);
  pwToolbar = gtk_toolbar_new ();

  gtk_toolbar_set_orientation ( GTK_TOOLBAR ( pwToolbar ),
                                GTK_ORIENTATION_HORIZONTAL );
  gtk_toolbar_set_style ( GTK_TOOLBAR ( pwToolbar ),
                          GTK_TOOLBAR_ICONS );
  pwFrame = gtk_frame_new(_("Shortcut buttons"));
  gtk_box_pack_start ( GTK_BOX ( pwVbox ), pwFrame, TRUE, TRUE, 0);
  gtk_container_add( GTK_CONTAINER( pwFrame ), pwToolbar);
  gtk_container_set_border_width( GTK_CONTAINER( pwToolbar ), 4);
  
  /* Edit button */
  pwButtons = gtk_button_new();
  gtk_container_add ( GTK_CONTAINER ( pwButtons ), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_widget_show(pwButtons);

  gtk_toolbar_append_widget( GTK_TOOLBAR( pwToolbar ),
                   pwButtons, _("Edit position"), NULL );
  g_signal_connect(pwButtons, "clicked", G_CALLBACK(edit_new_clicked), pnw);

  gtk_toolbar_append_space(GTK_TOOLBAR(pwToolbar));

  pwButtons = button_from_image(gtk_image_new_from_stock(GNUBG_STOCK_NEW0, GTK_ICON_SIZE_LARGE_TOOLBAR));
  gtk_toolbar_append_widget( GTK_TOOLBAR( pwToolbar ),
                   pwButtons, _("Start a new money game session"), NULL );
  g_signal_connect( G_OBJECT( pwButtons ), "clicked",
	    G_CALLBACK( ToolButtonPressedMS ), pnw );

  gtk_toolbar_append_space(GTK_TOOLBAR(pwToolbar));

  for(i = 1; i < 19; i=i+2, j++ ){			     
     gchar *sz;
     gchar stock[50];
     int *pi;

     sz = g_strdup_printf(_("Start a new %d point match"), i);
     sprintf(stock, "gnubg-stock-new%d", i);
     pwButtons = button_from_image(gtk_image_new_from_stock(stock, GTK_ICON_SIZE_LARGE_TOOLBAR));
     gtk_toolbar_append_widget( GTK_TOOLBAR( pwToolbar ),
                             pwButtons, sz, NULL );
     g_free(sz);
     
      pi = malloc ( sizeof ( int ) );
      *pi = i;
     g_object_set_data_full( G_OBJECT( pwButtons ), "user_data",
                                  pi, free );

     g_signal_connect( G_OBJECT( pwButtons ), "clicked",
		    G_CALLBACK( ToolButtonPressed ), pnw );
  }

  pwFrame = gtk_frame_new(_("Match settings"));
  pwHbox = gtk_hbox_new(FALSE, 0);

  pwLabel = gtk_label_new(_("Length:"));
  gtk_label_set_justify (GTK_LABEL (pwLabel), GTK_JUSTIFY_RIGHT);
  pnw->pwML = gtk_spin_button_new_with_range (0, MAXSCORE, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (pnw->pwML), TRUE);

  gtk_box_pack_start ( GTK_BOX ( pwHbox ), pwLabel, FALSE, TRUE, 0);
  gtk_box_pack_start ( GTK_BOX ( pwHbox ), pnw->pwML, FALSE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(pwFrame), pwHbox);
  gtk_container_add(GTK_CONTAINER(pwVbox), pwFrame);

  /* Here the simplified player settings starts */
  
  pwFrame = gtk_frame_new(_("Player settings"));
  
  pwHbox = gtk_hbox_new(FALSE, 0);
  pwVbox2 = gtk_vbox_new(FALSE, 0);

  gtk_container_add(GTK_CONTAINER(pwHbox), pwVbox2);

  pnw->pwCPS = gtk_radio_button_new_with_label( NULL, _("Current player settings"));
  gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwCPS, FALSE, FALSE, 0);
  
  pnw->pwGNUvsHuman = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(pnw->pwCPS), _("GNU Backgammon vs. Human"));
  pnw->pwHumanHuman = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(pnw->pwCPS), _("Human vs. Human"));
  gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwGNUvsHuman, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwHumanHuman, FALSE, FALSE, 0);
  
  pwButtons = gtk_button_new_with_label(_("Modify player settings..."));
  gtk_container_set_border_width(GTK_CONTAINER(pwButtons), 10);
  
  gtk_container_add(GTK_CONTAINER(pwVbox2), pwButtons );

  g_signal_connect(G_OBJECT(pwButtons), "clicked",
      G_CALLBACK( SettingsPressed ), NULL );

  pwVbox2 = gtk_vbox_new( FALSE, 0);
  
  gtk_container_add(GTK_CONTAINER(pwHbox), pwVbox2);

  pnw->pwManualDice = gtk_check_button_new_with_label(_("Manual dice"));
  pnw->pwTutorMode = gtk_check_button_new_with_label(_("Tutor mode"));

  gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwManualDice, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(pwVbox2), pnw->pwTutorMode, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(pwFrame), pwHbox);
  gtk_container_add(GTK_CONTAINER(pwVbox), pwFrame);
  
  return pwVbox;
} 

static void NewOK( GtkWidget *pw, newwidget *pnw )
{
  char sz[40];
  unsigned int Mlength = (unsigned int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(pnw->pwML));

  UpdatePlayerSettings(pnw);

  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );

	if (ToolbarIsEditing(NULL))
		click_edit();	/* Come out of editing mode */

  sprintf(sz, "new match %d", Mlength );
  UserCommand(sz);
}

static void NewSet( newwidget *pnw)
{
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pnw->pwTutorMode ),
                                fTutor );
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pnw->pwManualDice), rngCurrent == RNG_MANUAL);
  gtk_spin_button_set_value( GTK_SPIN_BUTTON( pnw->pwML ), nDefaultLength );
}

extern void GTKNew( void )
{
  GtkWidget *pwDialog, *pwPage;
  newwidget nw;

  pwDialog = GTKCreateDialog( _("GNU Backgammon - New"),
			   DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK( NewOK ), &nw );
  gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
 		        pwPage = NewWidget(&nw));

  gtk_widget_grab_focus(DialogArea(pwDialog, DA_OK));

  NewSet( &nw );
 
  GTKRunDialog(pwDialog);

}

extern void
SetMET (GtkWidget * pw, gpointer p)
{
  gchar *filename, *command;

  gchar *met_dir = BuildFilename("met");
  filename =
    GTKFileSelect (_("Set match equity table"), "*.xml",
		   met_dir, NULL, GTK_FILE_CHOOSER_ACTION_OPEN);
  g_free(met_dir);

  if (filename)
    {
      command = g_strconcat ("set matchequitytable \"", filename, "\"", NULL);
      UserCommand (command);
      g_free (command);
      g_free (filename);
      /* update filename on option page */
      if (p && GTK_WIDGET_VISIBLE (p))
	gtk_label_set_text (GTK_LABEL (p), (char *) miCurrent.szFileName);
    }
  UserCommand("save settings");
}

typedef struct _rolloutpagewidget {
  int *pfOK;
  evalcontext *precCube, *precCheq;
  movefilter *pmf;
} rolloutpagewidget;

typedef struct _rolloutpagegeneral {
  int *pfOK;
  GtkWidget *pwCubeful, *pwVarRedn, *pwInitial, *pwRotate, *pwDoLate;
  GtkWidget *pwDoTrunc, *pwCubeEqualChequer, *pwPlayersAreSame;
  GtkWidget *pwTruncEqualPlayer0;
  GtkWidget *pwTruncBearoff2, *pwTruncBearoffOS, *pwTruncBearoffOpts;
  GtkWidget *pwAdjLatePlies, *pwAdjTruncPlies, *pwAdjMinGames;
  GtkWidget *pwDoSTDStop, *pwAdjMaxError;
  GtkWidget *pwJsdDoStop;
  GtkWidget *pwJsdMinGames, *pwJsdAdjMinGames, *pwJsdAdjLimit;
  GtkAdjustment *padjTrials, *padjTruncPlies, *padjLatePlies;
  GtkAdjustment *padjSeed, *padjMinGames, *padjMaxError;
  GtkAdjustment *padjJsdMinGames, *padjJsdLimit;
  GtkWidget *arpwGeneral;
  GtkWidget *pwMinGames, *pwMaxError;
} rolloutpagegeneral;

typedef struct _rolloutwidget {
  rolloutcontext  rcRollout;
  rolloutpagegeneral  *prwGeneral;
  rolloutpagewidget *prpwPages[4], *prpwTrunc;
  GtkWidget *RolloutNotebook, *frame[2];
  AnalysisDetails *analysisDetails[5];
  int  fCubeEqualChequer, fPlayersAreSame, fTruncEqualPlayer0;
  int *pfOK;
  int *psaveAs;
  int *ploadRS;
} rolloutwidget;

/***************************************************************************
 *****
 *****  Change SGF_ROLLOUT_VER in eval.h if rollout settings change 
 *****  such that previous .sgf files won't be able to extend rollouts
 *****
 ***************************************************************************/
static void GetRolloutSettings( GtkWidget *pw, rolloutwidget *prw ) {
  int   p0, p1, i;
  int fCubeEqChequer, fPlayersAreSame;

  prw->rcRollout.nTrials = (int)prw->prwGeneral->padjTrials->value;
  prw->rcRollout.nTruncate = (unsigned short)prw->prwGeneral->padjTruncPlies->value;

  prw->rcRollout.nSeed = (int)prw->prwGeneral->padjSeed->value;

  prw->rcRollout.fCubeful = gtk_toggle_button_get_active(
                                                         GTK_TOGGLE_BUTTON( prw->prwGeneral->pwCubeful ) );

  prw->rcRollout.fVarRedn = gtk_toggle_button_get_active(
                                                         GTK_TOGGLE_BUTTON( prw->prwGeneral->pwVarRedn ) );

  prw->rcRollout.fRotate = gtk_toggle_button_get_active(
                                                        GTK_TOGGLE_BUTTON( prw->prwGeneral->pwRotate ) );

  prw->rcRollout.fInitial = gtk_toggle_button_get_active(
                                                         GTK_TOGGLE_BUTTON( prw->prwGeneral->pwInitial ) );

  prw->rcRollout.fDoTruncate = gtk_toggle_button_get_active(
                                                            GTK_TOGGLE_BUTTON( prw->prwGeneral->pwDoTrunc ) );

  prw->rcRollout.fTruncBearoff2 = gtk_toggle_button_get_active(
                                                               GTK_TOGGLE_BUTTON( prw->prwGeneral->pwTruncBearoff2 ) );

  prw->rcRollout.fTruncBearoffOS = gtk_toggle_button_get_active(
                                                                GTK_TOGGLE_BUTTON( prw->prwGeneral->pwTruncBearoffOS ) );

  if (prw->rcRollout.nTruncate == 0) 
    prw->rcRollout.fDoTruncate = FALSE;

  prw->rcRollout.fLateEvals = gtk_toggle_button_get_active(
                                                           GTK_TOGGLE_BUTTON( prw->prwGeneral->pwDoLate ) );

  prw->rcRollout.nLate = (unsigned short)prw->prwGeneral->padjLatePlies->value;

  fCubeEqChequer = 
    gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( prw->prwGeneral->pwCubeEqualChequer ) );

  fPlayersAreSame = 
    gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( prw->prwGeneral->pwPlayersAreSame ) );

  prw->rcRollout.fStopOnSTD = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(
                                            prw->prwGeneral->pwDoSTDStop));
  prw->rcRollout.nMinimumGames = (unsigned int) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prw->prwGeneral->pwMinGames));
  prw->rcRollout.rStdLimit = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (prw->prwGeneral->pwMaxError));

  prw->rcRollout.fStopOnJsd = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(prw->prwGeneral->pwJsdDoStop));
  prw->rcRollout.nMinimumJsdGames = (unsigned int) gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prw->prwGeneral->pwJsdMinGames));
  prw->rcRollout.rJsdLimit = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (prw->prwGeneral->pwJsdAdjLimit));

  /* if the players are the same, copy player 0 settings to player 1 */
  if (fPlayersAreSame) {
    for (p0 = 0, p1 = 1; p0 < 4; p0 += 2, p1 += 2) {
      memcpy (prw->prpwPages[p1]->precCheq, prw->prpwPages[p0]->precCheq, 
              sizeof (evalcontext));
      memcpy (prw->prpwPages[p1]->precCube, prw->prpwPages[p0]->precCube, 
              sizeof (evalcontext));

      memcpy (prw->prpwPages[p1]->pmf, prw->prpwPages[p0]->pmf,
              MAX_FILTER_PLIES * MAX_FILTER_PLIES * sizeof ( movefilter ) );
    }
  }

  /* if cube is same as chequer, copy the chequer settings to the
     cube settings */

  if (fCubeEqChequer) {
    for (i = 0; i < 4; ++i) {
      memcpy (prw->prpwPages[i]->precCube, prw->prpwPages[i]->precCheq,
              sizeof (evalcontext));
    }
	  
    memcpy (prw->prpwTrunc->precCube, prw->prpwTrunc->precCheq,
            sizeof (evalcontext));
  }

  if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON ( 
													   prw->prwGeneral->pwTruncEqualPlayer0))) {
	memcpy (prw->prpwTrunc->precCube, prw->prpwPages[0]->precCheq,
			sizeof (evalcontext));
	memcpy (prw->prpwTrunc->precCheq, prw->prpwPages[0]->precCube,
			sizeof (evalcontext));
  }

  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );

}

static void SetRolloutsOK( GtkWidget *pw, rolloutwidget *prw ) {
  *prw->pfOK = TRUE;
  GetRolloutSettings(pw, prw);
}

static void save_rollout_as_clicked (GtkWidget *pw, rolloutwidget *prw ) {
  *prw->psaveAs = TRUE;
  GetRolloutSettings(pw, prw);
}

static void load_rs_clicked (GtkWidget *pw, rolloutwidget *prw ) {
  *prw->ploadRS = TRUE;
  gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

/* create one page for rollout settings  for playes & truncation */

static AnalysisDetails *RolloutPage( rolloutpagewidget *prpw, const char *title, const int fMoveFilter, GtkWidget **frameRet )
{
	GtkWidget *pwFrame;
	pwFrame = gtk_frame_new ( title );
	if (frameRet)
		*frameRet = pwFrame;

	return CreateEvalSettings(pwFrame, title, prpw->precCheq, prpw->pmf, prpw->precCube, NULL);
}

static void LateEvalToggled( GtkWidget *pw, rolloutwidget *prw) {

  int do_late = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
                                                 prw->prwGeneral->pwDoLate ) );
  int   are_same = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( prw->prwGeneral->pwPlayersAreSame ) );

  /* turn on/off the late pages */
  gtk_widget_set_sensitive(prw->analysisDetails[2]->pwSettingWidgets, do_late);
  gtk_widget_set_sensitive(prw->analysisDetails[3]->pwSettingWidgets, do_late && !are_same);

  /* turn on/off the ply setting in the general page */
  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwAdjLatePlies),
                            do_late);
}

static void STDStopToggled( GtkWidget *pw, rolloutwidget *prw) {

  int do_std_stop = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
                       prw->prwGeneral->pwDoSTDStop ) );

  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwAdjMinGames),
                            do_std_stop);
  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwAdjMaxError),
                            do_std_stop);
}

static void JsdStopToggled( GtkWidget *pw, rolloutwidget *prw) {

  int do_jsd_stop = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (prw->prwGeneral->pwJsdDoStop ) );
  
  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwJsdAdjLimit), do_jsd_stop);

  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwJsdAdjMinGames), do_jsd_stop);

}

static void TruncEnableToggled( GtkWidget *pw, rolloutwidget *prw)
{
  int do_trunc = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
                                                                   prw->prwGeneral->pwDoTrunc ) );

  int sameas_p0 = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
			prw->prwGeneral->pwTruncEqualPlayer0));

  /* turn on/off the truncation page */
  gtk_widget_set_sensitive(prw->analysisDetails[4]->pwSettingWidgets, do_trunc && !sameas_p0);

  /* turn on/off the truncation ply setting */
  gtk_widget_set_sensitive (GTK_WIDGET (prw->prwGeneral->pwAdjTruncPlies ),
                            do_trunc);

}

static void TruncEqualPlayer0Toggled( GtkWidget *pw, rolloutwidget *prw)
{
  int do_trunc = 
	gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
									 prw->prwGeneral->pwDoTrunc ) );
  int sameas_p0 = 
	gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (
									prw->prwGeneral->pwTruncEqualPlayer0));

  prw->fTruncEqualPlayer0 = sameas_p0;
  /* turn on/off the truncation page */
  gtk_widget_set_sensitive(prw->analysisDetails[4]->pwSettingWidgets, do_trunc && !sameas_p0);
}

static void CubeEqCheqToggled( GtkWidget *pw, rolloutwidget *prw)
{
  int i, are_same = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON (prw->prwGeneral->pwCubeEqualChequer ) );

  prw->fCubeEqualChequer = are_same;

  for (i = 0; i < 5; ++i)
	  prw->analysisDetails[i]->cubeDisabled = are_same;
}

static void
CubefulToggled ( GtkWidget *pw, rolloutwidget *prw ) {

  int f = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( 
                                                            prw->prwGeneral->pwCubeful ) );
  
  gtk_widget_set_sensitive ( GTK_WIDGET ( 
                                         prw->prwGeneral->pwTruncBearoffOS ), ! f );

}

static void PlayersSameToggled( GtkWidget *pw, rolloutwidget *prw)
{
  int   are_same = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( prw->prwGeneral->pwPlayersAreSame ) );
  int do_late = gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON ( prw->prwGeneral->pwDoLate ) );

  prw->fPlayersAreSame = are_same;

  gtk_widget_set_sensitive(prw->analysisDetails[1]->pwSettingWidgets, !are_same);
  gtk_widget_set_sensitive(prw->analysisDetails[3]->pwSettingWidgets, !are_same && do_late);
  prw->analysisDetails[0]->title = are_same ? _("First Play Both") : _("First Play (0) ");
  gtk_frame_set_label(GTK_FRAME(prw->frame[0]), prw->analysisDetails[0]->title);
  prw->analysisDetails[2]->title = are_same ? _("Later Play Both") : _("Later Play (0) ");
  gtk_frame_set_label(GTK_FRAME(prw->frame[1]), prw->analysisDetails[2]->title);
}

/* create the General page for rollouts */

static GtkWidget *
RolloutPageGeneral (rolloutpagegeneral *prpw, rolloutwidget *prw) {
  GtkWidget *pwPage, *pw, *pwv;
  GtkWidget *pwHBox;
  GtkWidget *pwTable, *pwFrame;

  pwPage = gtk_vbox_new( FALSE, 0 );
  gtk_container_set_border_width( GTK_CONTAINER( pwPage ), 8 );

  prpw->padjSeed = GTK_ADJUSTMENT( gtk_adjustment_new(
                                                      abs( prw->rcRollout.nSeed ), 0, INT_MAX, 1, 1, 0 ) );
 
  prpw->padjTrials = GTK_ADJUSTMENT(gtk_adjustment_new(
                                                       prw->rcRollout.nTrials, 1,
                                                       1296 * 1296, 36, 36, 0 ) );
  pw = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pwPage), pw );

  gtk_container_add( GTK_CONTAINER( pw ),
                     gtk_label_new( _("Seed:") ) );
  gtk_container_add( GTK_CONTAINER( pw ),
		     gtk_spin_button_new( prpw->padjSeed, 1, 0 ) );

  gtk_container_add( GTK_CONTAINER( pw ),
                     gtk_label_new( _("Trials:") ) );
  gtk_container_add( GTK_CONTAINER( pw ),
                     gtk_spin_button_new( prpw->padjTrials, 36, 0 ) );

  pwFrame = gtk_frame_new ( _("Truncation") );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwFrame );

  pw = gtk_hbox_new( FALSE, 8 );
  gtk_container_set_border_width( GTK_CONTAINER( pw ), 8 );
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw);
   
  prpw->pwDoTrunc = gtk_check_button_new_with_label (
                                                     _( "Truncate Rollouts" ) );
  gtk_container_add( GTK_CONTAINER( pw ), prpw->pwDoTrunc );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( prpw->pwDoTrunc ),
                                 prw->rcRollout.fDoTruncate);
  g_signal_connect( G_OBJECT( prpw->pwDoTrunc ), "toggled",
                      G_CALLBACK (TruncEnableToggled), prw);

  prpw->pwAdjTruncPlies = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pw ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), 
                     gtk_label_new( _("Truncate at ply:" ) ) );

  prpw->padjTruncPlies = GTK_ADJUSTMENT( gtk_adjustment_new( 
                                                            prw->rcRollout.nTruncate, 0, 1000, 1, 1, 0 ) );
  gtk_container_add( GTK_CONTAINER( pwHBox ), gtk_spin_button_new( 
                                                                  prpw->padjTruncPlies, 1, 0 ) );


  pwFrame = gtk_frame_new ( _("Evaluation for later plies") );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwFrame );

  pw = gtk_hbox_new( FALSE, 8 );
  gtk_container_set_border_width( GTK_CONTAINER( pw ), 8 );
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw);
   
  prpw->pwDoLate = gtk_check_button_new_with_label (
                                        _( "Enable separate evaluations " ) );
  gtk_container_add( GTK_CONTAINER( pw ), prpw->pwDoLate );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( 
                                      prw->prwGeneral->pwDoLate ), 
                                 prw->rcRollout.fLateEvals);
  g_signal_connect( G_OBJECT( prw->prwGeneral->pwDoLate ), 
                      "toggled", G_CALLBACK (LateEvalToggled), prw);

  prpw->pwAdjLatePlies = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pw ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), 
                     gtk_label_new( _("Change eval after ply:" ) ) );

  prpw->padjLatePlies = GTK_ADJUSTMENT( gtk_adjustment_new( 
                                       prw->rcRollout.nLate, 0, 1000, 1, 1, 0 ) );
  gtk_container_add( GTK_CONTAINER( pwHBox ), gtk_spin_button_new( 
                                                prpw->padjLatePlies, 1, 0 ) );

  pwFrame = gtk_frame_new ( _("Stop when result is accurate") );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwFrame );

  /* an hbox for the pane */
  pw = gtk_hbox_new( FALSE, 8 );
  gtk_container_set_border_width( GTK_CONTAINER( pw ), 8 );
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw);
   
  prpw->pwDoSTDStop = gtk_check_button_new_with_label (
                               _( "Stop when STD is small enough " ) );
  gtk_container_add( GTK_CONTAINER( pw ), prpw->pwDoSTDStop );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( 
                                      prw->prwGeneral->pwDoSTDStop ), 
                                 prw->rcRollout.fStopOnSTD);
  g_signal_connect( G_OBJECT( prw->prwGeneral->pwDoSTDStop ), 
                      "toggled", G_CALLBACK (STDStopToggled), prw);

  /* a vbox for the adjusters */
  pwv = gtk_vbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pw ), pwv);

  
  prpw->pwAdjMinGames = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pwv ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), 
                     gtk_label_new( _("Minimum Trials:" ) ) );

  prpw->padjMinGames = GTK_ADJUSTMENT( gtk_adjustment_new( 
                                       prw->rcRollout.nMinimumGames, 1, 1296 * 1296, 36, 36, 0 ) );

  prpw->pwMinGames = gtk_spin_button_new(prpw->padjMinGames, 1, 0 ) ;

  gtk_container_add( GTK_CONTAINER( pwHBox ), prpw->pwMinGames);

  prpw->pwAdjMaxError = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pwv ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), 
                   gtk_label_new( _("Equity Standard Deviation:" ) ) );

  prpw->padjMaxError = GTK_ADJUSTMENT( gtk_adjustment_new( 
                       prw->rcRollout.rStdLimit, 0, 1, .0001, .0001, 0 ) );

  prpw->pwMaxError = gtk_spin_button_new(prpw->padjMaxError, .0001, 4 );

  gtk_container_add( GTK_CONTAINER( pwHBox ), prpw->pwMaxError);


  pwFrame = gtk_frame_new ( _("Stop Rollouts of multiple moves based on JSD") );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwFrame );

  /* an hbox for the frame */
  pw = gtk_hbox_new( FALSE, 8 );
  gtk_container_set_border_width( GTK_CONTAINER( pw ), 8 );
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw);
  
  /* a vbox for the check boxes */
  pwv = gtk_vbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pw ), pwv);

  prpw->pwJsdDoStop = gtk_check_button_new_with_label (_( "Enable Stop on JSD" ) );
  gtk_container_add( GTK_CONTAINER( pwv ), prpw->pwJsdDoStop );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( prw->prwGeneral->pwJsdDoStop ), prw->rcRollout.fStopOnJsd);
  g_signal_connect( G_OBJECT( prw->prwGeneral->pwJsdDoStop ), "toggled", G_CALLBACK (JsdStopToggled), prw);

  /* a vbox for the adjusters */
  pwv = gtk_vbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pw ), pwv);

  prpw->pwJsdAdjMinGames = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pwv ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), gtk_label_new( _("Minimum Trials:" ) ) );

  prpw->padjJsdMinGames = GTK_ADJUSTMENT( gtk_adjustment_new( prw->rcRollout.nMinimumJsdGames, 1, 1296 * 1296, 36, 36, 0 ) );

  prpw->pwJsdMinGames = gtk_spin_button_new(prpw->padjJsdMinGames, 1, 0 ) ;

  gtk_container_add( GTK_CONTAINER( pwHBox ), prpw->pwJsdMinGames);

  prpw->pwJsdAdjLimit = pwHBox = gtk_hbox_new( FALSE, 0 );
  gtk_container_add( GTK_CONTAINER( pwv ), pwHBox);
  gtk_container_add( GTK_CONTAINER( pwHBox ), 
                   gtk_label_new( _("JSDs from best choice" ) ) );

  prpw->padjJsdLimit = GTK_ADJUSTMENT( gtk_adjustment_new( 
                       prw->rcRollout.rJsdLimit, 0, 8, .0001, .0001, 0 ) );

  prpw->pwJsdAdjLimit = gtk_spin_button_new(prpw->padjJsdLimit, .0001, 4 );

  gtk_container_add( GTK_CONTAINER( pwHBox ), prpw->pwJsdAdjLimit);

  pwFrame = gtk_frame_new ( _("Bearoff Truncation") );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwFrame );
  prpw->pwTruncBearoffOpts = pw = gtk_vbox_new( FALSE, 8 );
  gtk_container_set_border_width( GTK_CONTAINER( pw ), 8 );
  gtk_container_add ( GTK_CONTAINER ( pwFrame ), pw);

  prpw->pwTruncBearoff2 = gtk_check_button_new_with_label (
                                                           _( "Truncate cubeless (and cubeful money) at exact bearoff database" ) );

  gtk_container_add( GTK_CONTAINER( pw ), prpw->pwTruncBearoff2 );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON ( prpw->pwTruncBearoff2 ),
                                 prw->rcRollout.fTruncBearoff2 );

  prpw->pwTruncBearoffOS = gtk_check_button_new_with_label (
                                                            _( "Truncate cubeless at one-sided bearoff database" ) );

  gtk_container_add( GTK_CONTAINER( pw ), prpw->pwTruncBearoffOS );
  gtk_toggle_button_set_active ( GTK_TOGGLE_BUTTON 
                                 (prpw->pwTruncBearoffOS ),
                                 prw->rcRollout.fTruncBearoffOS );


  pwTable = gtk_table_new ( 2, 2, TRUE );
  gtk_container_add ( GTK_CONTAINER (pwPage ), pwTable );

  prpw->pwCubeful = gtk_check_button_new_with_label ( _("Cubeful") );
  gtk_table_attach ( GTK_TABLE ( pwTable ), prpw->pwCubeful,
                     0, 1, 0, 1,
                     GTK_FILL,
                     GTK_FILL,
                     2, 2 );

  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( prpw->pwCubeful ),
                                prw->rcRollout.fCubeful );

  g_signal_connect( G_OBJECT( prpw->pwCubeful ), "toggled",
                      G_CALLBACK( CubefulToggled ), prw );


  prpw->pwVarRedn = gtk_check_button_new_with_label ( 
                                                     _("Variance reduction") );
  gtk_table_attach ( GTK_TABLE ( pwTable ), prpw->pwVarRedn,
                     1, 2, 0, 1,
                     GTK_FILL,
                     GTK_FILL,
                     2, 2 );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( prpw->pwVarRedn ),
                                prw->rcRollout.fVarRedn );

  prpw->pwRotate = gtk_check_button_new_with_label ( 
                                                    _("Use quasi-random dice") );
  gtk_table_attach ( GTK_TABLE ( pwTable ), prpw->pwRotate,
                     0, 1, 1, 2,
                     GTK_FILL,
                     GTK_FILL,
                     2, 2 );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( prpw->pwRotate ),
                                prw->rcRollout.fRotate );

  prpw->pwInitial = gtk_check_button_new_with_label ( 
                                                     _("Rollout as initial position") );
  gtk_table_attach ( GTK_TABLE ( pwTable ), prpw->pwInitial,
                     1, 2, 1, 2,
                     GTK_FILL,
                     GTK_FILL,
                     2, 2 );
  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( prpw->pwInitial ),
                                prw->rcRollout.fInitial );

  return pwPage;
}

static void gtk_save_rollout_settings(void)
{
	char *filename;
	char *folder;
	FILE *pf;
	folder = g_build_filename(szHomeDirectory, "rol", NULL);
	if (!g_file_test(folder, G_FILE_TEST_IS_DIR)) {
		g_mkdir(folder, 0700);
		if (!g_file_test(folder, G_FILE_TEST_IS_DIR)) {
			outputerrf(_("Failed to create %s"), folder);
			return;
		}
	}
	filename =
	    GTKFileSelect(_("Save Rollout Settings As (*.rol)"), "*.rol", folder,
			  NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
	if (!filename)
		return;

	if (!g_str_has_suffix(filename, ".rol")) {
		char *tmp = g_strdup(filename);
		g_free(filename);
		filename = g_strconcat(tmp, ".rol", NULL);
                g_free(tmp);
	}

	errno = 0;
	pf = g_fopen(filename, "w");
	if (!pf) {
		outputerr(filename);
		g_free(filename);
		return;
	}
	SaveRolloutSettings(pf, "set rollout", &rcRollout);
	fclose(pf);
	if (errno)
		outputerr(filename);
	g_free(filename);
	return;
}

static void gtk_load_rollout_settings(void)
{

    gchar *filename, *command, *folder;
    folder = g_build_filename(szHomeDirectory, "rol", NULL);
    filename =
	GTKFileSelect(_("Open rollout settings (*.rol)"), "*.rol", folder, NULL,
		      GTK_FILE_CHOOSER_ACTION_OPEN);
    if (filename) {
	command = g_strconcat("load commands \"", filename, "\"", NULL);
        outputoff();
	UserCommand(command);
        outputon();
	g_free(command);
	g_free(filename);
    }
}


extern void SetRollouts( gpointer p, guint n, GtkWidget *pwIgnore )
{
  GtkWidget *pwDialog, *pwTable, *pwVBox;
  GtkWidget *saveAsButton;
  GtkWidget *loadRSButton;
  int fOK = FALSE;
  int saveAs = TRUE;
  int loadRS = TRUE;
  rolloutwidget rw;
  rolloutpagegeneral RPGeneral;
  rolloutpagewidget  RPPlayer0, RPPlayer1, RPPlayer0Late, RPPlayer1Late,
    RPTrunc;
  char sz[ 256 ];
  int  i;
  const float epsilon = 1.0e-6f;

  while (saveAs || loadRS)
  {
  memcpy (&rw.rcRollout, &rcRollout, sizeof (rcRollout));
  rw.prwGeneral = &RPGeneral;
  rw.prpwPages[0] = &RPPlayer0;
  rw.prpwPages[1] = &RPPlayer1;
  rw.prpwPages[2] = &RPPlayer0Late;
  rw.prpwPages[3] = &RPPlayer1Late;
  rw.prpwTrunc = &RPTrunc;
  rw.fCubeEqualChequer = fCubeEqualChequer;
  rw.fPlayersAreSame = fPlayersAreSame;
  rw.fTruncEqualPlayer0 = fTruncEqualPlayer0;
  rw.pfOK = &fOK;
  rw.psaveAs = &saveAs;
  rw.ploadRS = &loadRS;

  RPPlayer0.precCube = &rw.rcRollout.aecCube[0];
  RPPlayer0.precCheq = &rw.rcRollout.aecChequer[0];
  RPPlayer0.pmf = (movefilter *) rw.rcRollout.aaamfChequer[ 0 ];

  RPPlayer0Late.precCube = &rw.rcRollout.aecCubeLate[0];
  RPPlayer0Late.precCheq = &rw.rcRollout.aecChequerLate[0];
  RPPlayer0Late.pmf = (movefilter *) rw.rcRollout.aaamfLate[ 0 ];

  RPPlayer1.precCube = &rw.rcRollout.aecCube[1];
  RPPlayer1.precCheq = &rw.rcRollout.aecChequer[1];
  RPPlayer1.pmf = (movefilter *) rw.rcRollout.aaamfChequer[ 1 ];

  RPPlayer1Late.precCube = &rw.rcRollout.aecCubeLate[1];
  RPPlayer1Late.precCheq = &rw.rcRollout.aecChequerLate[1];
  RPPlayer1Late.pmf = (movefilter *) rw.rcRollout.aaamfLate[ 1 ];

  RPTrunc.precCube = &rw.rcRollout.aecCubeTrunc;
  RPTrunc.precCheq = &rw.rcRollout.aecChequerTrunc;

  saveAs = FALSE;
  loadRS = FALSE;
  pwDialog = GTKCreateDialog( _("GNU Backgammon - Rollouts"), DT_QUESTION,
                           NULL, DIALOG_FLAG_MODAL, G_CALLBACK( SetRolloutsOK ), &rw );

  gtk_container_add(GTK_CONTAINER( DialogArea(pwDialog, DA_BUTTONS ) ),
                  saveAsButton = gtk_button_new_with_label(_("Save As")));
  g_signal_connect(saveAsButton, "clicked", G_CALLBACK(save_rollout_as_clicked), &rw);

  gtk_container_add(GTK_CONTAINER( DialogArea(pwDialog, DA_BUTTONS ) ),
                  loadRSButton = gtk_button_new_with_label(_("Load")));
  g_signal_connect(loadRSButton, "clicked", G_CALLBACK(load_rs_clicked), &rw);

  gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
                     rw.RolloutNotebook = gtk_notebook_new() );
  gtk_container_set_border_width( GTK_CONTAINER( rw.RolloutNotebook ), 4 );


  gtk_notebook_append_page( GTK_NOTEBOOK( rw.RolloutNotebook ), 
                            RolloutPageGeneral (rw.prwGeneral, &rw),
                            gtk_label_new ( _("General Settings" ) ) );

	pwVBox = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK( rw.RolloutNotebook ), pwVBox, gtk_label_new (_("Play settings")));

	pwTable = gtk_table_new( 3, 2, FALSE );
	gtk_box_pack_start(GTK_BOX(pwVBox), pwTable, FALSE, FALSE, 0);
	rw.analysisDetails[0] = RolloutPage (rw.prpwPages[0], _("First Play (0) "), TRUE, &rw.frame[0] );
	gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[0]->pwSettingWidgets), 0, 1, 0, 1, 0, 0, 4, 4 );
	rw.analysisDetails[1] = RolloutPage (rw.prpwPages[1], _("First Play (1) "), TRUE, NULL );
	gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[1]->pwSettingWidgets), 1, 2, 0, 1, 0, 0, 4, 4 );
	rw.analysisDetails[2] = RolloutPage (rw.prpwPages[2], _("Later Play (0) "), TRUE, &rw.frame[1] );
	gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[2]->pwSettingWidgets), 0, 1, 1, 2, 0, 0, 4, 4 );
	rw.analysisDetails[3] = RolloutPage (rw.prpwPages[3], _("Later Play (1) "), TRUE, NULL );
	gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[3]->pwSettingWidgets), 1, 2, 1, 2, 0, 0, 4, 4 );
	rw.prpwTrunc->pmf = NULL;
	rw.analysisDetails[4] = RolloutPage (rw.prpwTrunc, _("Truncation Pt."), FALSE, NULL );
	gtk_table_attach(GTK_TABLE(pwTable), gtk_widget_get_parent(rw.analysisDetails[4]->pwSettingWidgets), 0, 1, 2, 3, 0, 0, 4, 4 );

	RPGeneral.pwPlayersAreSame = gtk_check_button_new_with_label ( _("Use same settings for both players") );
	gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwPlayersAreSame, FALSE, FALSE, 0);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( RPGeneral.pwPlayersAreSame), rw.fPlayersAreSame);
	g_signal_connect( G_OBJECT( RPGeneral.pwPlayersAreSame ), "toggled", G_CALLBACK ( PlayersSameToggled ), &rw);

	RPGeneral.pwCubeEqualChequer = gtk_check_button_new_with_label (
															  _("Cube decisions use same settings as Chequer play") );
	gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwCubeEqualChequer, FALSE, FALSE, 0);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( RPGeneral.pwCubeEqualChequer),
								rw.fCubeEqualChequer);
	g_signal_connect( G_OBJECT( RPGeneral.pwCubeEqualChequer ), "toggled",
					  G_CALLBACK ( CubeEqCheqToggled ), &rw);

	RPGeneral.pwTruncEqualPlayer0 = gtk_check_button_new_with_label (
			_("Use player0 setting for truncation point") );
	gtk_box_pack_start(GTK_BOX(pwVBox), RPGeneral.pwTruncEqualPlayer0, FALSE, FALSE, 0);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( RPGeneral.pwTruncEqualPlayer0),
								rw.fTruncEqualPlayer0);
	g_signal_connect( G_OBJECT( RPGeneral.pwTruncEqualPlayer0 ), "toggled",
					  G_CALLBACK ( TruncEqualPlayer0Toggled), &rw);

	/* Set things up correctly */
	LateEvalToggled (NULL, &rw);
	STDStopToggled (NULL, &rw);
	JsdStopToggled (NULL, &rw);
	TruncEnableToggled (NULL, &rw);
	CubeEqCheqToggled (NULL, &rw);
	PlayersSameToggled (NULL, &rw);
	CubefulToggled (NULL, &rw);

	GTKRunDialog(pwDialog);

	for (i = 0; i < 5; i++)
		free(rw.analysisDetails[i]);

  if( fOK || saveAs ) 
  {
    unsigned int fCubeful;
    outputoff();

    if((fCubeful = rw.rcRollout.fCubeful) != rcRollout.fCubeful ) {
      sprintf( sz, "set rollout cubeful %s",
              fCubeful ? "on" : "off" );
      UserCommand( sz );

		for (i = 0; i < 2; ++i) {
		  /* replicate main page cubeful on/of in all evals */
		  rw.rcRollout.aecChequer[i].fCubeful = fCubeful;
		  rw.rcRollout.aecChequerLate[i].fCubeful = fCubeful;
		  rw.rcRollout.aecCube[i].fCubeful = fCubeful;
		  rw.rcRollout.aecCubeLate[i].fCubeful = fCubeful;
		}
		rw.rcRollout.aecCubeTrunc.fCubeful = 
		  rw.rcRollout.aecChequerTrunc.fCubeful = fCubeful;
	
    }

    for (i = 0; i < 2; ++i) {


      if (EvalCmp (&rw.rcRollout.aecCube[i], &rcRollout.aecCube[i], 1) ) {
        sprintf (sz, "set rollout player %d cubedecision", i);
        SetEvalCommands( sz, &rw.rcRollout.aecCube[i], 
                         &rcRollout.aecCube[ i ] );
      }
      if (EvalCmp (&rw.rcRollout.aecChequer[i], 
                   &rcRollout.aecChequer[i], 1)) {
        sprintf (sz, "set rollout player %d chequer", i);
        SetEvalCommands( sz, &rw.rcRollout.aecChequer[i], 
                         &rcRollout.aecChequer[ i ] );
      }
      
      sprintf ( sz, "set rollout player %d movefilter", i );
      SetMovefilterCommands ( sz, 
                              rw.rcRollout.aaamfChequer[ i ], 
                              rcRollout.aaamfChequer[ i ] );

      if (EvalCmp (&rw.rcRollout.aecCubeLate[i], 
                   &rcRollout.aecCubeLate[i], 1 ) ) {
        sprintf (sz, "set rollout late player %d cube", i);
        SetEvalCommands( sz, &rw.rcRollout.aecCubeLate[i], 
                         &rcRollout.aecCubeLate[ i ] );
      }

      if (EvalCmp (&rw.rcRollout.aecChequerLate[i], 
                   &rcRollout.aecChequerLate[i], 1 ) ) {
        sprintf (sz, "set rollout late player %d chequer", i);
        SetEvalCommands( sz, &rw.rcRollout.aecChequerLate[i], 
                         &rcRollout.aecChequerLate[ i ] );
      }

      sprintf ( sz, "set rollout late player %d movefilter", i );
      SetMovefilterCommands ( sz, 
                              rw.rcRollout.aaamfLate[ i ], 
                              rcRollout.aaamfLate[ i ] );

    }

    if (EvalCmp (&rw.rcRollout.aecCubeTrunc, &rcRollout.aecCubeTrunc, 1) ) {
      SetEvalCommands( "set rollout truncation cube", 
                       &rw.rcRollout.aecCubeTrunc, &rcRollout.aecCubeTrunc );
    }

    if (EvalCmp (&rw.rcRollout.aecChequerTrunc, 
                 &rcRollout.aecChequerTrunc, 1) ) {
      SetEvalCommands( "set rollout truncation chequer", 
                       &rw.rcRollout.aecChequerTrunc, 
                       &rcRollout.aecChequerTrunc );
    }

    if( rw.rcRollout.fCubeful != rcRollout.fCubeful ) {
      sprintf( sz, "set rollout cubeful %s",
               rw.rcRollout.fCubeful ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fVarRedn != rcRollout.fVarRedn ) {
      sprintf( sz, "set rollout varredn %s",
               rw.rcRollout.fVarRedn ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fInitial != rcRollout.fInitial ) {
      sprintf( sz, "set rollout initial %s",
               rw.rcRollout.fInitial ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fRotate != rcRollout.fRotate ) {
      sprintf( sz, "set rollout quasirandom %s",
               rw.rcRollout.fRotate ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fLateEvals != rcRollout.fLateEvals ) {
      sprintf( sz, "set rollout late enable  %s",
               rw.rcRollout.fLateEvals ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fDoTruncate != rcRollout.fDoTruncate ) {
      sprintf( sz, "set rollout truncation enable  %s",
               rw.rcRollout.fDoTruncate ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.nTruncate != rcRollout.nTruncate ) {
      sprintf( sz, "set rollout truncation plies %d", 
               rw.rcRollout.nTruncate );
      UserCommand( sz );
    }

    if( rw.rcRollout.nTrials != rcRollout.nTrials ) {
      sprintf( sz, "set rollout trials %d", rw.rcRollout.nTrials );
      UserCommand( sz );
    }

    if( rw.rcRollout.fStopOnSTD != rcRollout.fStopOnSTD ) {
      sprintf( sz, "set rollout limit enable %s", rw.rcRollout.fStopOnSTD ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.nMinimumGames != rcRollout.nMinimumGames ) {
      sprintf( sz, "set rollout limit minimumgames %d", rw.rcRollout.nMinimumGames );
      UserCommand( sz );
    }

    if( fabs( rw.rcRollout.rStdLimit - rcRollout.rStdLimit ) > epsilon ) {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
      sprintf( sz, "set rollout limit maxerr %s",
                                  g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE,
			           "%5.4f", rw.rcRollout.rStdLimit ));
      UserCommand( sz );
    }

    /* ======================= */

    if( rw.rcRollout.fStopOnJsd != rcRollout.fStopOnJsd ) {
      sprintf( sz, "set rollout jsd stop %s", rw.rcRollout.fStopOnJsd ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.nMinimumJsdGames != rcRollout.nMinimumJsdGames ) {
      sprintf( sz, "set rollout jsd minimumgames %d", rw.rcRollout.nMinimumJsdGames );
      UserCommand( sz );
    }

    if( fabs( rw.rcRollout.rJsdLimit - rcRollout.rJsdLimit ) > epsilon ) {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
      sprintf( sz, "set rollout jsd limit %s",
                                  g_ascii_formatd( buf, G_ASCII_DTOSTR_BUF_SIZE,
			           "%5.4f", rw.rcRollout.rJsdLimit ));
      UserCommand( sz );
    }


    if( rw.rcRollout.nLate != rcRollout.nLate ) {
      sprintf( sz, "set rollout late plies %d", rw.rcRollout.nLate );
      UserCommand( sz );
    }

    if( abs(rw.rcRollout.nSeed) != abs(rcRollout.nSeed) ) {
	    /* seed may be unsigned long int */
      sprintf( sz, "set rollout seed %lu", rw.rcRollout.nSeed );
      UserCommand( sz );
    }

    if( rw.rcRollout.fTruncBearoff2 != rcRollout.fTruncBearoff2 ) {
      sprintf( sz, "set rollout bearofftruncation exact %s", 
               rw.rcRollout.fTruncBearoff2 ? "on" : "off" );
      UserCommand( sz );
    }

    if( rw.rcRollout.fTruncBearoffOS != rcRollout.fTruncBearoffOS ) {
      sprintf( sz, "set rollout bearofftruncation onesided %s", 
               rw.rcRollout.fTruncBearoffOS ? "on" : "off" );
      UserCommand( sz );
    }

	if ( rw.fCubeEqualChequer != fCubeEqualChequer) {
	  sprintf( sz, "set rollout cube-equal-chequer %s",
               rw.fCubeEqualChequer ? "on" : "off" );
	  UserCommand( sz );
	}

	if ( rw.fPlayersAreSame != fPlayersAreSame) {
	  sprintf( sz, "set rollout players-are-same %s",
               rw.fPlayersAreSame ? "on" : "off" );
	  UserCommand( sz );
	}

	if ( rw.fTruncEqualPlayer0 != fTruncEqualPlayer0) {
	  sprintf( sz, "set rollout truncate-equal-player0 %s",
               rw.fTruncEqualPlayer0 ? "on" : "off" );
	  UserCommand( sz );
	}

    UserCommand("save settings");
    outputon();
    if (saveAs)
            gtk_save_rollout_settings();
    else if (fOK) 
            break;
  }
  if (loadRS)
          gtk_load_rollout_settings();
  }	
}

void
GTKTextWindow (const char *szOutput, const char *title, const int type, GtkWidget *parent)
{

  GtkWidget *pwDialog = GTKCreateDialog (title, type, parent, 0, NULL, NULL);
  GtkWidget *pwText;
  GtkWidget *sw;
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  GtkRequisition req;

  pwText = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (pwText), GTK_WRAP_NONE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(pwText), FALSE);

  buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_create_tag (buffer, "monospace", "family", "monospace",
			      NULL);
  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, szOutput, -1,
					    "monospace", NULL);
  gtk_text_view_set_buffer (GTK_TEXT_VIEW (pwText), buffer);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (sw), pwText);

  gtk_widget_size_request( GTK_WIDGET( pwText ), &req );
  gtk_window_set_default_size( GTK_WINDOW( pwDialog ), -1, MIN(500, req.height+200) );
  gtk_container_add (GTK_CONTAINER (DialogArea (pwDialog, DA_MAIN)), sw);
  gtk_window_set_modal (GTK_WINDOW (pwDialog), TRUE);
  gtk_window_set_transient_for (GTK_WINDOW (pwDialog), GTK_WINDOW (pwMain));
  g_signal_connect(G_OBJECT (pwDialog), "destroy",
		      G_CALLBACK (gtk_main_quit), NULL);

  GTKRunDialog(pwDialog);
}

extern void GTKEval( char *szOutput ) {
    GTKTextWindow( szOutput,  _("GNU Backgammon - Evaluation"), DT_INFO, NULL);
}

static void DestroyHint( gpointer p, GObject *obj ) {

  movelist *pml = p;

  if ( pml ) {
    if ( pml->amMoves )
      free ( pml->amMoves );
    
    free ( pml );
  }

  SetPanelWidget(WINDOW_HINT, NULL);
}

static void
HintOK ( GtkWidget *pw, void *unused )
{
	getWindowGeometry(WINDOW_HINT);
	DestroyPanel(WINDOW_HINT);
}

extern void GTKCubeHint(moverecord *pmr, const matchstate *pms, int did_double, int did_take, int hist ) {
    
    GtkWidget *pw, *pwHint;

    if (GetPanelWidget(WINDOW_HINT))
	gtk_widget_destroy(GetPanelWidget(WINDOW_HINT));

	pwHint = GTKCreateDialog( _("GNU Backgammon - Hint"), DT_INFO,
			   NULL, DIALOG_FLAG_NOTIDY, G_CALLBACK( HintOK ), NULL );
	SetPanelWidget(WINDOW_HINT, pwHint);

    pw = CreateCubeAnalysis (pmr, pms, did_double, did_take, hist);

    gtk_container_add( GTK_CONTAINER( DialogArea( pwHint, DA_MAIN ) ),
                       pw );

    gtk_widget_grab_focus( DialogArea( pwHint, DA_OK ) );
    
    setWindowGeometry(WINDOW_HINT);
    g_object_weak_ref( G_OBJECT( pwHint ), DestroyHint, NULL );
    
    gtk_window_set_default_size(GTK_WINDOW(pwHint), 400, 300);
    
    gtk_widget_show_all( pwHint );
}

/*
 * Give hints for resignation
 *
 * Input:
 *    rEqBefore: equity before resignation
 *    rEqAfter: equity after resignation
 *    pci: cubeinfo
 *    fOUtputMWC: output in MWC or equity
 *
 * FIXME: include arOutput in the dialog, so the the user
 *        can see how many gammons/backgammons she'll win.
 */

extern void
GTKResignHint( float arOutput[], float rEqBefore, float rEqAfter,
               cubeinfo *pci, int fMWC )
{
    GtkWidget *pwDialog = GTKCreateDialog( _("GNU Backgammon - Hint"), DT_INFO,
			NULL, DIALOG_FLAG_MODAL, NULL, NULL );
    GtkWidget *pw;
    GtkWidget *pwTable = gtk_table_new( 2, 3, FALSE );

    char *pch, sz[ 16 ];

    /* equity before resignation */
    
    gtk_table_attach( GTK_TABLE( pwTable ), pw = gtk_label_new(
	fMWC ? _("MWC before resignation") : _("Equity before resignation") ), 0, 1, 0, 1,
		      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0 );
    gtk_misc_set_alignment( GTK_MISC( pw ), 0, 0.5 );

    if( fMWC )
      sprintf( sz, "%6.2f%%", 100.0 * ( eq2mwc ( - rEqBefore, pci ) ) );
    else
      sprintf( sz, "%+6.3f", -rEqBefore );

    gtk_table_attach( GTK_TABLE( pwTable ), pw = gtk_label_new( sz ),
		      1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
		      GTK_EXPAND | GTK_FILL, 4, 0 );
    gtk_misc_set_alignment( GTK_MISC( pw ), 1, 0.5 );

    /* equity after resignation */

    gtk_table_attach( GTK_TABLE( pwTable ), pw = gtk_label_new(
	fMWC ? _("MWC after resignation") : _("Equity after resignation") ), 0, 1, 1, 2,
		      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 0 );
    gtk_misc_set_alignment( GTK_MISC( pw ), 0, 0.5 );
    
    if( fMWC )
      sprintf( sz, "%6.2f%%",
               100.0 * eq2mwc ( - rEqAfter, pci ) );
    else
      sprintf( sz, "%+6.3f", - rEqAfter );

    gtk_table_attach( GTK_TABLE( pwTable ), pw = gtk_label_new( sz ),
		      1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
		      GTK_EXPAND | GTK_FILL, 4, 0 );
    gtk_misc_set_alignment( GTK_MISC( pw ), 1, 0.5 );

    if ( -rEqAfter >= -rEqBefore )
	pch = _("You should accept the resignation!");
    else
	pch = _("You should reject the resignation!");
    
    gtk_table_attach( GTK_TABLE( pwTable ), pw = gtk_label_new( pch ),
		      0, 2, 2, 3, GTK_EXPAND | GTK_FILL,
		      GTK_EXPAND | GTK_FILL, 0, 8 );

    gtk_container_set_border_width( GTK_CONTAINER( pwTable ), 8 );
    
    gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
		       pwTable );

	GTKRunDialog(pwDialog);
}

extern void 
GTKHint( moverecord *pmr, int hist)
{
    GtkWidget *pwMoves, *pwHint;
    if (!pmr || pmr->ml.cMoves < 1)
    {
	    outputerrf(_("There are no legal moves. Figure it out yourself."));
	    return;
    }

    if (GetPanelWidget(WINDOW_HINT))
	gtk_widget_destroy(GetPanelWidget(WINDOW_HINT));

    pwMoves = CreateMoveList( pmr, TRUE, TRUE, TRUE, hist );

    /* create dialog */

	pwHint = GTKCreateDialog( _("GNU Backgammon - Hint"), DT_INFO,
			   NULL, DIALOG_FLAG_NONE, G_CALLBACK( HintOK ), NULL );
	SetPanelWidget(WINDOW_HINT, pwHint);
    
    gtk_container_add( GTK_CONTAINER( DialogArea( pwHint, DA_MAIN ) ), 
                       pwMoves );

    setWindowGeometry(WINDOW_HINT);
    g_object_weak_ref( G_OBJECT( pwHint ), DestroyHint, NULL );

	if (!IsPanelDocked(WINDOW_HINT))
		gtk_window_set_default_size(GTK_WINDOW(pwHint), 400, 300);

    gtk_widget_show_all( pwHint );
}

static void SetMouseCursor(GdkCursorType cursorType)
{
	if (!GDK_IS_WINDOW(pwMain->window))
	{
		g_print("no window\n");
		return;
	}
	if (cursorType)
	{
		GdkCursor *cursor;
		cursor = gdk_cursor_new(cursorType);
		gdk_window_set_cursor(pwMain->window, cursor);
		gdk_cursor_unref(cursor);
	}
	else
		gdk_window_set_cursor(pwMain->window, NULL);
}

extern void GTKProgressStart( const char *sz )
{
	GTKSuspendInput();

	if( sz )
		gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idProgress, sz );

	SetMouseCursor(GDK_WATCH);
}

extern void GTKProgressStartValue( char *sz, int iMax )
{
	GTKSuspendInput();

	if( sz )
		gtk_statusbar_push( GTK_STATUSBAR( pwStatus ), idProgress, sz );

	SetMouseCursor(GDK_WATCH);
}

extern void GTKProgressValue ( int iValue, int iMax )
{
    gchar *gsz;
    gdouble frac = 1.0 * iValue / (1.0 * iMax );
    gsz = g_strdup_printf("%d/%d (%.0f%%)", iValue, iMax, 100 * frac);
    gtk_progress_bar_set_text( GTK_PROGRESS_BAR( pwProgress ), gsz);
    gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( pwProgress ), frac);
    g_free(gsz);

	ProcessEvents();
}

extern void GTKProgress( void )
{
    gtk_progress_bar_pulse( GTK_PROGRESS_BAR( pwProgress ) );

	ProcessEvents();
}

extern void GTKProgressEnd( void )
{
    GTKResumeInput();

	if (!pwProgress) /*safe guard on language change*/
		return;

    gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( pwProgress ), 0.0 );
    gtk_progress_bar_set_text( GTK_PROGRESS_BAR( pwProgress ), " " );
    gtk_statusbar_pop( GTK_STATUSBAR( pwStatus ), idProgress );

	SetMouseCursor(0);
}

int colWidth;

extern void GTKShowScoreSheet( void )
{
	GtkWidget *pwDialog, *pwBox;
	GtkWidget *hbox;
	GtkWidget *view;
	GtkWidget* pwScrolled;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	int numRows = 0;
	char title[100];
	listOLD *pl;

	sprintf(title, _("Score Sheet - "));
	if (ms.nMatchTo > 0)
		sprintf(title + strlen(title), _("%d point match"), ms.nMatchTo);
	else
		strcat(title, _("Money Session"));

	pwDialog = GTKCreateDialog(title, DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL );
	pwBox = gtk_vbox_new( FALSE, 0);
	gtk_container_set_border_width( GTK_CONTAINER(pwBox), 8);

	gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ), pwBox);

	gtk_container_set_border_width(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), 4);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), hbox);

	store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_INT);

	for (pl = lMatch.plNext; pl->p; pl = pl->plNext )
	{
		int score[2];
		GtkTreeIter iter;
		listOLD *plGame = pl->plNext->p;

		if (plGame)
		{
			moverecord *pmr = plGame->plNext->p;
			score[0] = pmr->g.anScore[0];
			score[1] = pmr->g.anScore[1];
		}
		else
		{
			moverecord *pmr;
			listOLD *plGame = pl->p;
			if (!plGame)
				continue;
			pmr = plGame->plNext->p;
			score[0] = pmr->g.anScore[0];
			score[1] = pmr->g.anScore[1];
			if (pmr->g.fWinner == -1)
			{
				if (pl == lMatch.plNext)
				{	/* First game */
					score[0] = score[1] = 0;
				}
				else
					continue;
			}
			else
				score[pmr->g.fWinner] += pmr->g.nPoints;
		}
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, score[0], 1, score[1], -1);
		numRows++;
	}

	pwScrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(ap[0].szName, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_min_width(column, 75);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(ap[1].szName, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_min_width(column, 75);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

	gtk_container_add(GTK_CONTAINER(pwScrolled), view);
	gtk_box_pack_start(GTK_BOX(hbox), pwScrolled, TRUE, TRUE, 0);

	GTKRunDialog(pwDialog);
}

static void GtkShowCopying(GtkWidget *parent)
{
	ShowList( aszCopying, _("Copying"), parent);
}

static void GtkShowWarranty(GtkWidget *parent)
{
	ShowList( aszWarranty, _("Warranty"), parent);
}

static void GtkShowEngine(GtkWidget *parent)
{
    char *szBuffer[2];
    szBuffer[0] = g_new0(char, 4096);
    szBuffer[1] = NULL;
    EvalStatus(szBuffer[0]);
    ShowList(szBuffer, _("Evaluation engine"), parent);
}

extern void GTKShowVersion( void )
{
	GtkWidget *pwDialog, *pwButtonBox, *pwButton;
	GtkWidget *image;
	gchar *fn;

	pwDialog = GTKCreateDialog(_("About GNU Backgammon"), DT_CUSTOM, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_CLOSEBUTTON, NULL, NULL);
	gtk_window_set_resizable(GTK_WINDOW(pwDialog), FALSE);

	fn = g_build_filename(getPkgDataDir(), "pixmaps", "gnubg-big.png", NULL);
	image = gtk_image_new_from_file(fn);
	g_free(fn);
	gtk_misc_set_padding(GTK_MISC(image), 8, 8 );
	gtk_box_pack_start(GTK_BOX(DialogArea(pwDialog, DA_MAIN)), image, FALSE, FALSE, 0 );

	/* Buttons on right side */
	pwButtonBox = gtk_vbox_new( FALSE, 0 );
	gtk_box_pack_start( GTK_BOX(DialogArea(pwDialog, DA_MAIN)), pwButtonBox, FALSE, FALSE, 8 );

	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Credits") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( GTKCommandShowCredits ), pwDialog );
	
	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Build Info") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( GTKShowBuildInfo ), pwDialog );
	
	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Copying conditions") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( GtkShowCopying ), pwDialog );
	
	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Warranty") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( GtkShowWarranty ), NULL );

	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Report Bug") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( ReportBug ), NULL );

	gtk_box_pack_start( GTK_BOX( pwButtonBox ), 
	pwButton = gtk_button_new_with_label(_("Evaluation Engine") ),
	FALSE, FALSE, 8 );
	g_signal_connect( G_OBJECT( pwButton ), "clicked",
		G_CALLBACK( GtkShowEngine ), pwDialog );
	GTKRunDialog(pwDialog);
}

static GtkWidget* SelectableLabel(GtkWidget* reference, const char* text)
{
	GtkWidget* pwLabel = gtk_label_new(text);
	gtk_label_set_selectable(GTK_LABEL(pwLabel), TRUE);
	return pwLabel;
}

extern void GTKShowBuildInfo(GtkWidget *pw, GtkWidget *pwParent)
{
	GtkWidget *pwDialog, *pwBox, *pwPrompt;
	const char* pch;

	pwDialog = GTKCreateDialog( _("GNU Backgammon - Build Info"),
		DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL );
	pwBox = gtk_vbox_new( FALSE, 0);
	gtk_container_set_border_width( GTK_CONTAINER(pwBox), 8);

	gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ), pwBox);

	gtk_box_pack_start( GTK_BOX( pwBox ), SelectableLabel(pwDialog, "Version " VERSION_STRING), FALSE, FALSE, 4 );

	gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);

    while((pch = GetBuildInfoString()))
		gtk_box_pack_start( GTK_BOX( pwBox ), pwPrompt = gtk_label_new( gettext(pch) ),
			FALSE, FALSE, 0 );

	gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);

	gtk_box_pack_start( GTK_BOX( pwBox ),
			gtk_label_new( _(aszCOPYRIGHT) ), FALSE, FALSE, 4 );

	pwPrompt = gtk_label_new( _(intro_string));
	gtk_box_pack_start( GTK_BOX( pwBox ), pwPrompt, FALSE, FALSE, 4 );
	gtk_label_set_line_wrap( GTK_LABEL( pwPrompt ), TRUE );
	
	GTKRunDialog(pwDialog);
}

/* Stores names in credits so not duplicated in list at bottom */
listOLD names;

static void AddTitle(GtkWidget* pwBox, char* Title)
{
	GtkRcStyle *ps = gtk_rc_style_new();
	GtkWidget* pwTitle = gtk_label_new(Title),
		*pwHBox = gtk_hbox_new( TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, FALSE, FALSE, 4);

	ps->font_desc = pango_font_description_new();
	pango_font_description_set_family_static( ps->font_desc, "serif" );
	pango_font_description_set_size( ps->font_desc, 16 * PANGO_SCALE );    
	gtk_widget_modify_style( pwTitle, ps );
	g_object_unref( ps );

	gtk_box_pack_start(GTK_BOX(pwHBox), pwTitle, TRUE, FALSE, 0);
}

static void AddName(GtkWidget* pwBox, char* name, const char* type)
{
	char buf[255];
	if (type)
		sprintf(buf, "%s: %s", type, name);
	else
		strcpy(buf, name);

	gtk_box_pack_start(GTK_BOX(pwBox), gtk_label_new(buf), FALSE, FALSE, 0);
	ListInsert(&names, name);
}

static int FindName(listOLD* pList, const char* name)
{
	listOLD *pl;
	for (pl = pList->plNext; pl != pList; pl = pl->plNext )
	{
		if (!strcmp(pl->p, name))
			return TRUE;
	}
	return FALSE;
}

extern void GTKCommandShowCredits(GtkWidget * pw, GtkWidget * pwParent)
{
	GtkWidget *pwDialog;
	GtkWidget *pwBox;
	GtkWidget *pwMainHBox;
	GtkWidget *pwHBox = 0;
	GtkWidget *pwVBox;
	GtkWidget *pwScrolled;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkTreeIter iter;
	int i = 0;
	credits *credit = &creditList[0];
	credEntry *ce;

	pwScrolled = gtk_scrolled_window_new(NULL, NULL);
	ListCreate(&names);

	pwDialog = GTKCreateDialog(_("GNU Backgammon - Credits"), DT_INFO, pwParent, DIALOG_FLAG_MODAL, NULL, NULL);

	pwMainHBox = gtk_hbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwMainHBox);

	pwBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pwMainHBox), pwBox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(pwBox), 8);

	while (credit->Title) {
		/* Two columns, so new hbox every-other one */
		if (i / 2 == (i + 1) / 2) {
			pwHBox = gtk_hbox_new(TRUE, 0);
			gtk_box_pack_start(GTK_BOX(pwBox), pwHBox, TRUE, FALSE, 0);
		}

		pwVBox = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(pwHBox), pwVBox, FALSE, FALSE, 0);

		AddTitle(pwVBox, _(credit->Title));

		ce = credit->Entry;
		while (ce->Name) {
			AddName(pwVBox, ce->Name, _(ce->Type));
			ce++;
		}
		if (i == 1)
			gtk_box_pack_start(GTK_BOX(pwBox), gtk_hseparator_new(), FALSE, FALSE, 4);
		credit++;
		i++;
	}

	pwVBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pwMainHBox), pwVBox, FALSE, FALSE, 0);

	AddTitle(pwVBox, _("Special thanks"));

	/* create list store */
	store = gtk_list_store_new(1, G_TYPE_STRING);

	/* add data to the list store */
	for (i = 0; ceCredits[i].Name; i++) {
		if (!FindName(&names, ceCredits[i].Name)) {
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter, 0, ceCredits[i].Name, -1);
		}
	}

	/* create tree view */
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Contributers"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	while (names.plNext->p)
		ListDelete(names.plNext);
	gtk_container_set_border_width(GTK_CONTAINER(pwVBox), 8);
	gtk_box_pack_start(GTK_BOX(pwVBox), pwScrolled, TRUE, TRUE, 0);
	gtk_widget_set_size_request(pwScrolled, 150, -1);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pwScrolled), treeview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pwScrolled), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);


	GTKRunDialog(pwDialog);
}

static void GTKHelpAdd( GtkTreeStore *pts, GtkTreeIter *ptiParent,
			command *pc ) {
    GtkTreeIter ti;
    
    for( ; pc->sz; pc++ )
	if( pc->szHelp ) {
	    gtk_tree_store_append( pts, &ti, ptiParent );
	    gtk_tree_store_set( pts, &ti, 0, pc->sz, 1, pc->szHelp,
				2, pc, -1 );
	    if( pc->pc && pc->pc->sz )
		GTKHelpAdd( pts, &ti, pc->pc );
	}
}

static void GTKHelpSelect( GtkTreeSelection *pts, gpointer p ) {

    GtkTreeModel *ptm;
    GtkTreeIter ti;
    GtkTreePath *ptp;
    command **apc;
    int i, c;
    char szCommand[ 128 ], *pchCommand = szCommand,
	szUsage[ 128 ], *pchUsage = szUsage, *pLabel;
	const char *pch;
    
    if( gtk_tree_selection_get_selected( pts, &ptm, &ti ) ) {
	ptp = gtk_tree_model_get_path( ptm, &ti );
	c = gtk_tree_path_get_depth( ptp );
	apc = malloc( c * sizeof( command * ) );
	for( i = c - 1; ; i-- ) {
	    gtk_tree_model_get( ptm, &ti, 2, apc + i, -1 );
	    if( !i )
		break;
	    gtk_tree_path_up( ptp );
	    gtk_tree_model_get_iter( ptm, &ti, ptp );
	}

	for( i = 0; i < c; i++ ) {
	    /* accumulate command and usage strings from path */
	    /* FIXME use markup a la gtk_label_set_markup for this */
	    pch = apc[ i ]->sz;
	    while( *pch )
		*pchCommand++ = *pchUsage++ = *pch++;
	    *pchCommand++ = ' '; *pchCommand = 0;
	    *pchUsage++ = ' '; *pchUsage = 0;

	    if( ( pch = apc[ i ]->szUsage ) ) {
		while( *pch )
		    *pchUsage++ = *pch++;
		*pchUsage++ = ' '; *pchUsage = 0;	
	    }		
	}

	pLabel = g_strdup_printf( _("%s- %s\n\nUsage: %s%s\n"), szCommand,
			       apc[ c - 1 ]->szHelp, szUsage,
			       ( apc[ c - 1 ]->pc && apc[ c - 1 ]->pc->sz ) ?
			       " <subcommand>" : "" );
	gtk_label_set_text( GTK_LABEL( pwHelpLabel ), pLabel );
	g_free( pLabel );
	
	free( apc );
	gtk_tree_path_free( ptp );
    } else
	gtk_label_set_text( GTK_LABEL( pwHelpLabel ), NULL );
}

extern void GTKHelp( char *sz )
{
    static GtkWidget *pw = NULL;
    GtkWidget *pwPaned, *pwScrolled;
    GtkTreeStore *pts;
    GtkTreeIter ti, tiSearch;
    GtkTreePath *ptp, *ptpExpand;
	GtkTreeSelection *treeSelection;
    char *pch;
    command *pc, *pcTest, *pcStart;
    int cch, i, c, *pn;
    void ( *pf )( char * );
    
    if( pw )
	{
		gtk_window_present( GTK_WINDOW( pw ) );
		return;
	}

	pts = gtk_tree_store_new( 3, G_TYPE_STRING, G_TYPE_STRING,
				  G_TYPE_POINTER );

	GTKHelpAdd( pts, NULL, acTop );
	
	pw = GTKCreateDialog(_("Help - command reference"), DT_INFO, NULL, DIALOG_FLAG_NONE, NULL, NULL);
	g_object_add_weak_pointer( G_OBJECT( pw ), (void*) &pw );
	gtk_window_set_title( GTK_WINDOW( pw ), _("Help - command reference") );
	gtk_window_set_default_size( GTK_WINDOW( pw ), 500, 400 );

	g_signal_connect_swapped (pw, "response",
			G_CALLBACK (gtk_widget_destroy), pw);

	gtk_container_add( GTK_CONTAINER(DialogArea( pw, DA_MAIN )), pwPaned = gtk_vpaned_new() );

	gtk_paned_pack1( GTK_PANED( pwPaned ),
			 pwScrolled = gtk_scrolled_window_new( NULL, NULL ),
			 TRUE, FALSE );
	gtk_scrolled_window_set_shadow_type( GTK_SCROLLED_WINDOW(
	    pwScrolled ), GTK_SHADOW_IN );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( pwScrolled ),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC );
	gtk_container_add( GTK_CONTAINER( pwScrolled ),
			   pwHelpTree = gtk_tree_view_new_with_model(
			       GTK_TREE_MODEL( pts ) ) );
	treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW( pwHelpTree ));

	g_object_unref( G_OBJECT( pts ) );
	gtk_tree_view_set_headers_visible( GTK_TREE_VIEW( pwHelpTree ),
					   FALSE );
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW(
	    pwHelpTree ), 0, NULL, gtk_cell_renderer_text_new(),
						     "text", 0, NULL );
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW(
	    pwHelpTree ), 1, NULL, gtk_cell_renderer_text_new(),
						     "text", 1, NULL );
	g_signal_connect( G_OBJECT( treeSelection ), "changed",
			  G_CALLBACK( GTKHelpSelect ), NULL );
	
	gtk_paned_pack2( GTK_PANED( pwPaned ),
			 pwHelpLabel = gtk_label_new( NULL ), FALSE, FALSE );
	gtk_label_set_selectable( GTK_LABEL( pwHelpLabel ), TRUE );
	
	gtk_widget_show_all( pw );

    gtk_tree_model_get_iter_first( GTK_TREE_MODEL( pts ), &ti );
    tiSearch = ti;
    pc = acTop;
    c = 0;
    while( pc && sz && ( pch = NextToken( &sz ) ) ) {
	pcStart = pc;
	cch = strlen( pch );
	for( ; pc->sz; pc++ )
	    if( !StrNCaseCmp( pch, pc->sz, cch ) )
		break;

	if( !pc->sz )
	    break;

	if( !pc->szHelp ) {
	    /* they gave a synonym; find the canonical version */
	    pf = pc->pf;
	    for( pc = pcStart; pc->sz; pc++ )
		if( pc->pf == pf && pc->szHelp )
		    break;

	    if( !pc->sz )
		break;
	}

	do
	    gtk_tree_model_get( GTK_TREE_MODEL( pts ), &tiSearch, 2,
				&pcTest, -1 );
	while( pcTest != pc &&
	       gtk_tree_model_iter_next( GTK_TREE_MODEL( pts ), &tiSearch ) );

	if( pcTest == pc ) {
	    /* found!  now try the next level down... */
	    c++;
	    ti = tiSearch;
	    pc = pc->pc;
	    gtk_tree_model_iter_children( GTK_TREE_MODEL( pts ), &tiSearch,
					  &ti );
	} else
	    break;
    }
    
    ptp = gtk_tree_model_get_path( GTK_TREE_MODEL( pts ), &ti );
    pn = gtk_tree_path_get_indices( ptp );
    ptpExpand = gtk_tree_path_new();
    for( i = 0; i < c; i++ ) {
	gtk_tree_path_append_index( ptpExpand, pn[ i ] );
	gtk_tree_view_expand_row( GTK_TREE_VIEW( pwHelpTree ), ptpExpand,
				  FALSE );
    }
    gtk_tree_selection_select_iter(treeSelection, &ti );
    gtk_tree_view_scroll_to_cell( GTK_TREE_VIEW( pwHelpTree ), ptp,
				  NULL, TRUE, 0.5, 0 );
	gtk_tree_view_set_cursor(GTK_TREE_VIEW( pwHelpTree ), ptp, NULL, FALSE);
    gtk_tree_path_free( ptp );
    gtk_tree_path_free( ptpExpand );
}

static void GTKBearoffProgressCancel( void ) {

#ifdef SIGINT
    raise( SIGINT );
#endif
    exit( EXIT_FAILURE );
}

/* Show a dialog box with a progress bar to be used during initialisation
   if a heuristic bearoff database must be created.  Most of gnubg hasn't
   been initialised yet, so this function is restricted in many ways. */
extern void GTKBearoffProgress( int i ) {

    static GtkWidget *pwDialog, *pw, *pwAlign;
    gchar *gsz;
    
    if( !pwDialog ) {
		pwDialog = GTKCreateDialog( _("GNU Backgammon"), DT_INFO, NULL, DIALOG_FLAG_MODAL|DIALOG_FLAG_NOTIDY, NULL, NULL );
	gtk_window_set_role( GTK_WINDOW( pwDialog ), "progress" );
	gtk_window_set_type_hint( GTK_WINDOW(pwDialog), GDK_WINDOW_TYPE_HINT_DIALOG );
	g_signal_connect( G_OBJECT( pwDialog ), "destroy",
			    G_CALLBACK( GTKBearoffProgressCancel ),
			    NULL );

	gtk_box_pack_start( GTK_BOX( DialogArea( pwDialog, DA_MAIN ) ),
			    pwAlign = gtk_alignment_new( 0.5, 0.5, 1, 0 ),
			    TRUE, TRUE, 8 );
	gtk_container_add( GTK_CONTAINER( pwAlign ),
			   pw = gtk_progress_bar_new() );
	
	gtk_widget_show_all( pwDialog );
    }

    gsz = g_strdup_printf("Generating bearoff database (%.0f %%)", i / 542.64);
    gtk_progress_bar_set_text( GTK_PROGRESS_BAR( pw ), gsz );
    gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( pw ), i / 54264.0 );
    g_free(gsz);

    if( i >= 54000 ) {
	g_signal_handlers_disconnect_by_func(
	    G_OBJECT( pwDialog ),
	    G_CALLBACK( GTKBearoffProgressCancel ), NULL );

	gtk_widget_destroy( pwDialog );
    }

    ProcessEvents();
}

static void enable_sub_menu( GtkWidget *pw, int f ); /* for recursion */

static void enable_menu( GtkWidget *pw, int f ) {

    GtkMenuItem *pmi = GTK_MENU_ITEM( pw );

    if( pmi->submenu )
	enable_sub_menu( pmi->submenu, f );
    else
	gtk_widget_set_sensitive( pw, f );
}

static void enable_sub_menu( GtkWidget *pw, int f ) {

    GtkMenuShell *pms = GTK_MENU_SHELL( pw );

    g_list_foreach( pms->children, (GFunc) enable_menu, GINT_TO_POINTER(f) );
}

/* A global setting has changed; update entry in Settings menu if necessary. */
extern void GTKSet( void *p ) {
	
    BoardData *bd = BOARD( pwBoard )->board_data;

    if( p == ap ) {
	/* Handle the player names. */
	gtk_label_set_text( GTK_LABEL( GTK_BIN(
	    gtk_item_factory_get_widget_by_action( pif, CMD_SET_TURN_0 )
	    )->child ), (ap[ 0 ].szName) );
	gtk_label_set_text( GTK_LABEL( GTK_BIN(
	    gtk_item_factory_get_widget_by_action( pif, CMD_SET_TURN_1 )
	    )->child ), (ap[ 1 ].szName) );

    GL_SetNames();

	GTKRegenerateGames();

    } else if( p == &ms.fJacoby ) {
        bd->jacoby_flag = ms.fJacoby;
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( bd->jacoby ),
                                      bd->jacoby_flag );
  	ShowBoard();

    } else if( p == &ms.fTurn ) {
	/* Handle the player on roll. */
	fAutoCommand = TRUE;
	
	if( ms.fTurn >= 0 )
	    gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(
		gtk_item_factory_get_widget_by_action( pif, CMD_SET_TURN_0 +
						       ms.fTurn ) ), TRUE );

        enable_menu ( gtk_item_factory_get_widget ( pif, "/Game/Roll" ),
                      ms.fMove == ms.fTurn && 
                      ap[ ms.fMove ].pt == PLAYER_HUMAN );

	fAutoCommand = FALSE;
    } else if( p == &ms.gs ) {
	/* Handle the game state. */
	fAutoCommand = TRUE;

	board_set_playing( BOARD( pwBoard ), plGame != NULL );
        ToolbarSetPlaying( pwToolbar, plGame != NULL );

	gtk_widget_set_sensitive( gtk_item_factory_get_widget( pif,
				"/File/Save..." ), plGame != NULL );
	
	enable_sub_menu( gtk_item_factory_get_widget( pif, "/Game" ),
			 ms.gs == GAME_PLAYING );

        enable_menu ( gtk_item_factory_get_widget ( pif, "/Game/Roll" ),
                      ms.fMove == ms.fTurn && 
                      ap[ ms.fMove ].pt == PLAYER_HUMAN );

	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_NEXT_ROLL ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_PREV_ROLL ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_NEXT_MARKED ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_PREV_MARKED ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_NEXT_GAME ), !ListEmpty( &lMatch ) );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_PREV_GAME ), !ListEmpty( &lMatch ) );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget(
				      pif, "/File/Match information..." ),
				  !ListEmpty( &lMatch ) );
	
	enable_sub_menu( gtk_item_factory_get_widget( pif, "/Analyse" ),
			 ms.gs == GAME_PLAYING );

    gtk_widget_set_sensitive( gtk_item_factory_get_widget( pif,
                              "/Analyse/Batch analyse..." ), TRUE );

	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
            pif, CMD_ANALYSE_MOVE ), 
            plLastMove && plLastMove->plNext && plLastMove->plNext->p );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_ANALYSE_GAME ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_ANALYSE_MATCH ), !ListEmpty( &lMatch ) );

	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
            pif, CMD_ANALYSE_CLEAR_MOVE ), 
            plLastMove && plLastMove->plNext && plLastMove->plNext->p );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_ANALYSE_CLEAR_GAME ), plGame != NULL );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_ANALYSE_CLEAR_MATCH ), !ListEmpty( &lMatch ) );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_SHOW_STATISTICS_MATCH ), !ListEmpty ( &lMatch ) );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_SHOW_MATCHEQUITYTABLE ), TRUE );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_SHOW_CALIBRATION ), TRUE );
	gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
	    pif, CMD_SWAP_PLAYERS ), !ListEmpty( &lMatch ) );
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_CUBE_CLEAR), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_CUBE_SHOW), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_MOVE_CLEAR), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_MOVE_SHOW), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_GAME_CLEAR), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_GAME_SHOW), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_MATCH_CLEAR), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_CMARK_MATCH_SHOW), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_ANALYSE_ROLLOUT_CUBE), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_ANALYSE_ROLLOUT_MOVE), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_ANALYSE_ROLLOUT_GAME), !ListEmpty(&lMatch));
    gtk_widget_set_sensitive( gtk_item_factory_get_widget_by_action(
			    pif, CMD_ANALYSE_ROLLOUT_MATCH), !ListEmpty(&lMatch));

    gtk_widget_set_sensitive( 
       gtk_item_factory_get_widget( pif,
                                    "/Analyse/Add match or session to database" ), 
       !ListEmpty( &lMatch ) );

    gtk_widget_set_sensitive( 
          gtk_item_factory_get_widget( pif,
                                       "/Analyse/Show Records" ), 
          TRUE );

	fAutoCommand = FALSE;
    } else if( p == &ms.fCrawford ) {
        bd->crawford_game = ms.fCrawford;
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( bd->crawford ), 
                                      ms.fCrawford );
	ShowBoard(); /* this is overkill, but it works */
    } else if( p == &ms.nCube ) {
	ShowBoard(); /* this is overkill, but it works */ 
    } else if (IsPanelShowVar(WINDOW_ANNOTATION, p)) {
	if (PanelShowing(WINDOW_ANNOTATION))
		ShowHidePanel(WINDOW_ANNOTATION);
    } else if (IsPanelShowVar(WINDOW_GAME, p)) {
		ShowHidePanel(WINDOW_GAME);
    } else if (IsPanelShowVar(WINDOW_ANALYSIS, p)) {
		ShowHidePanel(WINDOW_ANALYSIS);
    } else if (IsPanelShowVar(WINDOW_MESSAGE, p)) {
		ShowHidePanel(WINDOW_MESSAGE);
    } else if (IsPanelShowVar(WINDOW_THEORY, p)) {
		ShowHidePanel(WINDOW_THEORY);
    } else if (IsPanelShowVar(WINDOW_COMMAND, p)) {
		ShowHidePanel(WINDOW_COMMAND);
	} else if( p == &bd->rd->fDiceArea ) {
	if( gtk_widget_get_realized( pwBoard ) )
	{
#if USE_BOARD3D
		/* If in 3d mode may need to update sizes */
		if (display_is_3d(bd->rd))
			SetupViewingVolume3d(bd, bd->bd3d, bd->rd);
		else
#endif
		{    
			if( gtk_widget_get_realized( pwBoard ) ) {
			    if( GTK_WIDGET_VISIBLE( bd->dice_area ) && !bd->rd->fDiceArea )
				gtk_widget_hide( bd->dice_area );
			    else if( ! GTK_WIDGET_VISIBLE( bd->dice_area ) && bd->rd->fDiceArea )
				gtk_widget_show_all( bd->dice_area );
			}
		}}
    }
	else if( p == &fShowIDs )
	{
		inCallback = TRUE;
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif, "/View/Show ID in status bar" )), fShowIDs);
		inCallback = FALSE;

		if (!fShowIDs)
			gtk_widget_hide(pwIDBox);
		else
			gtk_widget_show_all(pwIDBox);
	}
	else if( p == &gui_show_pips )
		ShowBoard(); /* this is overkill, but it works */
	else if (p == &fOutputWinPC)
	{
		MoveListRefreshSize();
	}
	else if (p == &showMoveListDetail)
	{
		if (pwMoveAnalysis && pwDetails)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwDetails), showMoveListDetail);
	}
}


/* Match stats variables */
#define FORMATGS_ALL -1
#define NUM_STAT_TYPES 4
char *aszStatHeading [ NUM_STAT_TYPES ] = {
  N_("Chequer Play Statistics:"), 
  N_("Cube Statistics:"),
  N_("Luck Statistics:"),
  N_("Overall Statistics:")};
static GtkWidget* statViews[NUM_STAT_TYPES], *statView;
static int numStatGames;
GtkWidget *pwStatDialog;
int fGUIUseStatsPanel = TRUE;
GtkWidget *pswList;
GtkWidget *pwNotebook;

static void AddList(char *pStr, GtkWidget *view, const char *pTitle)
{
	gchar *sz;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	sprintf(strchr(pStr, 0), "%s\n", pTitle);
	if (gtk_tree_model_get_iter_first(model, &iter))
		do {
			gtk_tree_model_get(model, &iter, 0, &sz, -1);
			sprintf(strchr(pStr, 0), "%-37s ", sz ? sz : "");
			gtk_tree_model_get(model, &iter, 1, &sz, -1);
			sprintf(strchr(pStr, 0), "%-20s ", sz ? sz : "");
			gtk_tree_model_get(model, &iter, 2, &sz, -1);
			sprintf(strchr(pStr, 0), "%-20s\n", sz ? sz : "");
		} while (gtk_tree_model_iter_next(model, &iter));
	sprintf(strchr(pStr, 0), "\n");
}

static void CopyData(GtkWidget *pwNotebook, int page)
{
	char szOutput[4096];

	sprintf(szOutput, "%-37s %-20s %-20s\n", "", ap[ 0 ].szName, ap[ 1 ].szName);

	if (page == FORMATGS_CHEQUER || page == FORMATGS_ALL)
		AddList(szOutput, statViews[FORMATGS_CHEQUER], aszStatHeading[FORMATGS_CHEQUER]);
	if (page == FORMATGS_LUCK || page == FORMATGS_ALL)
		AddList(szOutput, statViews[FORMATGS_LUCK], aszStatHeading[FORMATGS_LUCK]);
	if (page == FORMATGS_CUBE || page == FORMATGS_ALL)
		AddList(szOutput, statViews[FORMATGS_CUBE], aszStatHeading[FORMATGS_CUBE]);
	if (page == FORMATGS_OVERALL || page == FORMATGS_ALL)
		AddList(szOutput, statViews[FORMATGS_OVERALL], aszStatHeading[FORMATGS_OVERALL]);

	TextToClipboard(szOutput);
}

static void CopyPage( GtkWidget *pwWidget, GtkWidget *pwNotebook )
{
	switch(gtk_notebook_get_current_page(GTK_NOTEBOOK(pwNotebook)))
	{
	case 0:
		CopyData(pwNotebook, FORMATGS_OVERALL);
		break;
	case 1:
		CopyData(pwNotebook, FORMATGS_CHEQUER);
		break;
	case 2:
		CopyData(pwNotebook, FORMATGS_CUBE);
		break;
	case 3:
		CopyData(pwNotebook, FORMATGS_LUCK);
		break;
	}
}

static void CopyAll( GtkWidget *pwWidget, GtkWidget *pwNotebook )
{
	CopyData(pwNotebook, FORMATGS_ALL);
}

static void FillStats(const statcontext *psc, const matchstate *pms,
		      const enum _formatgs gs, GtkWidget *statView)
{

	GList *list = formatGS(psc, pms->nMatchTo, gs);
	GList *pl;
	GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(statView)));

	for (pl = g_list_first(list); pl; pl = g_list_next(pl)) {
		GtkTreeIter iter;
		char **aasz = pl->data;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, aasz[0], 1, aasz[1], 2, aasz[2], -1);
	}
	freeGS(list);
}

static void SetStats(const statcontext *psc)
{
	char *aszLine[] = { NULL, NULL, NULL };
	int i;
	GtkListStore *store;
	GtkTreeIter iter;

	for (i = 0; i < NUM_STAT_TYPES; ++i)
	{
		store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(statViews[i]), GTK_TREE_MODEL(store));
		FillStats(psc, &ms, i, statViews[i]);
		g_object_unref(store);
	}

	store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(statView), GTK_TREE_MODEL(store));
	g_object_unref(store);
	aszLine[0] = aszStatHeading[FORMATGS_CHEQUER];
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
	FillStats(psc, &ms, FORMATGS_CHEQUER, statView);
	FillStats(psc, &ms, FORMATGS_LUCK, statView);

	aszLine[0] = aszStatHeading[FORMATGS_CUBE];
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
	FillStats(psc, &ms, FORMATGS_CUBE, statView);

	aszLine[0] = aszStatHeading[FORMATGS_OVERALL];
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, aszLine[0], -1);
	FillStats(psc, &ms, FORMATGS_OVERALL, statView);
}

static const statcontext *GetStatContext(int game)
{
	xmovegameinfo *pmgi;
	int i;

	if (!game)
		return &scMatch;
	else
	{
		listOLD *plGame, *pl = lMatch.plNext;
		for (i = 1; i < game; i++)
			pl = pl->plNext;

		plGame = pl->p;
                pmgi = &((moverecord *) plGame->plNext->p)->g;
		return &pmgi->sc;
	}
}

static void StatsSelectGame(GtkWidget *box, int i)
{
	int curStatGame = gtk_combo_box_get_active(GTK_COMBO_BOX(box));
	if (!curStatGame) {
		gtk_window_set_title(GTK_WINDOW(pwStatDialog), _("Statistics for all games"));
	} else {
		char sz[100];
		strcpy(sz, _("Statistics for game "));
		sprintf(sz + strlen(sz), "%d", curStatGame);
		gtk_window_set_title(GTK_WINDOW(pwStatDialog), sz);
	}
	SetStats(GetStatContext(curStatGame));
}

static void StatsPreviousGame( GtkWidget *button, GtkWidget *combo )
{
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	if (i > 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i -1 );
}

static void StatsNextGame( GtkWidget *button, GtkWidget *combo)
{
	int i = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	if (i < numStatGames)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), gtk_combo_box_get_active(GTK_COMBO_BOX(combo))+1);
}

static GtkWidget *AddNavigation(GtkWidget *pvbox)
{
	GtkWidget *phbox, *pw, *box;
	char sz[128];
	int anFinalScore[2];
	listOLD *pl;

	box = gtk_combo_box_new_text();

	if (getFinalScore(anFinalScore))
		sprintf(sz, _("All games: %s %d, %s %d"), ap[0].szName,
			anFinalScore[0], ap[1].szName, anFinalScore[1]);
	else
		sprintf(sz, _("All games: %s, %s"), ap[0].szName, ap[1].szName);
	phbox = gtk_hbox_new(FALSE, 0), gtk_box_pack_start(GTK_BOX(pvbox), phbox, FALSE, FALSE, 4);
	pw = button_from_image(gtk_image_new_from_stock
			       (GNUBG_STOCK_GO_PREV_GAME, GTK_ICON_SIZE_LARGE_TOOLBAR));
	g_signal_connect(G_OBJECT(pw), "clicked", G_CALLBACK(StatsPreviousGame), box);
	gtk_box_pack_start(GTK_BOX(phbox), pw, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(pw, _("Move back to the previous game"));

	pw = button_from_image(gtk_image_new_from_stock
			       (GNUBG_STOCK_GO_NEXT_GAME, GTK_ICON_SIZE_LARGE_TOOLBAR));
	g_signal_connect(G_OBJECT(pw), "clicked", G_CALLBACK(StatsNextGame), box);
	gtk_box_pack_start(GTK_BOX(phbox), pw, FALSE, FALSE, 4);
	gtk_widget_set_tooltip_text(pw, _("Move ahead to the next game"));

	gtk_combo_box_append_text(GTK_COMBO_BOX(box), sz);
	numStatGames = 0;
	for (pl = lMatch.plNext; pl->p; pl = pl->plNext) {
		listOLD *plGame = pl->p;
		moverecord *pmr = plGame->plNext->p;
		numStatGames++;

		sprintf(sz, _("Game %d: %s %d, %s %d"), pmr->g.i + 1, ap[0].szName,
			pmr->g.anScore[0], ap[1].szName, pmr->g.anScore[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(box), sz);
	}
	g_signal_connect(G_OBJECT(box), "changed", G_CALLBACK(StatsSelectGame), NULL);
	gtk_box_pack_start(GTK_BOX(phbox), box, TRUE, TRUE, 4);

	return box;
}

static void toggle_fGUIUseStatsPanel(GtkWidget *widget, GtkWidget *pw)
{
	fGUIUseStatsPanel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (fGUIUseStatsPanel)
	{
		gtk_widget_hide(pswList);
		gtk_widget_show(pwNotebook);
	}
	else
	{
		gtk_widget_hide(pwNotebook);
		gtk_widget_show(pswList);
	}
}

static void StatcontextCopy(GtkWidget *pw, GtkTreeView *view)
{
	static char szOutput[4096];
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *row;
	GList *rows;

	selection = gtk_tree_view_get_selection(view);

	if (gtk_tree_selection_count_selected_rows(selection) < 1)
		return;

	sprintf(szOutput, "%-37s %-20s %-20s\n", "", ap[0].szName, ap[1].szName);

	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (row = rows ; row; row = row->next) {
		GtkTreeIter iter;
		gchar *sz;
		GtkTreePath *path = row->data;
		gchar *pc;
		gtk_tree_model_get_iter(model, &iter, path);

		gtk_tree_model_get(model, &iter, 0, &sz, -1);
		sprintf(pc = strchr(szOutput, 0), "%-37s ", sz ? sz : "");
		g_free(sz);

		gtk_tree_model_get(model, &iter, 1, &sz, -1);
		sprintf(pc = strchr(szOutput, 0), "%-20s ", sz ? sz : "");
		g_free(sz);

		gtk_tree_model_get(model, &iter, 2, &sz, -1);
		sprintf(pc = strchr(szOutput, 0), "%-20s\n", sz ? sz : "");
		g_free(sz);

		gtk_tree_path_free(path);
	}
	g_list_free(rows);
	GTKTextToClipboard(szOutput);
}


static GtkWidget *CreateList(void)
{
	int i;
	GtkWidget *view;
	GtkWidget *copyMenu;
	GtkWidget *menu_item;
       
	view = gtk_tree_view_new();
	
	for (i=0; i < 3; i++) {
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		renderer = gtk_cell_renderer_text_new();
		g_object_set(renderer, "xalign", 1.0, NULL);
		column = gtk_tree_view_column_new_with_attributes("", renderer, "text", i, NULL);
		gtk_tree_view_column_set_alignment  (column, 0.97);
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
	}
	gtk_tree_view_column_set_title (gtk_tree_view_get_column (GTK_TREE_VIEW(view), 1), ap[0].szName); 
	gtk_tree_view_column_set_title (gtk_tree_view_get_column (GTK_TREE_VIEW(view), 2), ap[1].szName); 
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_MULTIPLE);
	/* list view (selections) */
	copyMenu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label ("Copy selection");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( StatcontextCopy ), view );

	menu_item = gtk_menu_item_new_with_label ("Copy all");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( CopyAll ), pwNotebook );

	g_signal_connect( G_OBJECT( view ), "button-press-event", G_CALLBACK( ContextMenu ), copyMenu );

	return view;
}

static void stat_dialog_map(GtkWidget *window, GtkWidget *pwUsePanels)
{
	toggle_fGUIUseStatsPanel(pwUsePanels, 0);
}

extern void GTKDumpStatcontext( int game )
{
	GtkWidget *copyMenu, *menu_item, *pvbox, *pwUsePanels;
	GtkWidget *navi_combo;
#if USE_BOARD3D
	int i;
	GtkWidget *pw;
	listOLD *pl;
	GraphData *gd = CreateGraphData();
#endif
	pwStatDialog = GTKCreateDialog( "", DT_INFO, NULL, DIALOG_FLAG_MODAL, NULL, NULL );

	pwNotebook = gtk_notebook_new();
	gtk_notebook_set_scrollable( GTK_NOTEBOOK( pwNotebook ), TRUE );
	gtk_notebook_popup_disable( GTK_NOTEBOOK( pwNotebook ) );

	pvbox = gtk_vbox_new( FALSE, 0 ),
    gtk_box_pack_start( GTK_BOX( pvbox ), pwNotebook, TRUE, TRUE, 0);

	gtk_notebook_append_page( GTK_NOTEBOOK( pwNotebook ), statViews[FORMATGS_OVERALL] = CreateList(),
					  gtk_label_new(_("Overall")));

	gtk_notebook_append_page( GTK_NOTEBOOK( pwNotebook ), statViews[FORMATGS_CHEQUER] = CreateList(),
					  gtk_label_new(_("Chequer play")));

	gtk_notebook_append_page( GTK_NOTEBOOK( pwNotebook ), statViews[FORMATGS_CUBE] = CreateList(),
					  gtk_label_new(_("Cube decisions")));

	gtk_notebook_append_page( GTK_NOTEBOOK( pwNotebook ), statViews[FORMATGS_LUCK] = CreateList(),
					  gtk_label_new(_("Luck")));

	statView = CreateList();

	pswList = gtk_scrolled_window_new( NULL, NULL );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( pswList ),
				GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
	gtk_container_add( GTK_CONTAINER( pswList ), statView );

	gtk_box_pack_start (GTK_BOX (pvbox), pswList, TRUE, TRUE, 0);

	navi_combo = AddNavigation(pvbox);
	gtk_container_add( GTK_CONTAINER( DialogArea( pwStatDialog, DA_MAIN ) ), pvbox );

#if USE_BOARD3D
	SetNumGames(gd, numStatGames);

	pl = lMatch.plNext;
	for (i = 0; i < numStatGames; i++)
	{
		listOLD *plGame = pl->p;
		moverecord *mr = plGame->plNext->p;
		xmovegameinfo *pmgi = &mr->g;
		AddGameData(gd, i, &pmgi->sc);
		pl = pl->plNext;
	}
	/* Total values */
	AddGameData(gd, i, &scMatch);

	pw = StatGraph(gd);
	gtk_notebook_append_page( GTK_NOTEBOOK( pwNotebook ), pw,
					  gtk_label_new(_("Graph")));
    gtk_widget_set_tooltip_text(pw, _("This graph shows the total error rates per game for each player."
		" The games are along the bottom and the error rates up the side."
		" Chequer error in green, cube error in blue."));
#endif

	pwUsePanels = gtk_check_button_new_with_label(_("Split statistics into panels"));
	gtk_widget_set_tooltip_text(pwUsePanels, "Show data in a single list or split other several panels");
	gtk_box_pack_start (GTK_BOX (pvbox), pwUsePanels, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pwUsePanels), fGUIUseStatsPanel);
	g_signal_connect(G_OBJECT(pwUsePanels), "toggled", G_CALLBACK(toggle_fGUIUseStatsPanel), NULL);

	/* list view (selections) */
	copyMenu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label ("Copy selection");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( StatcontextCopy ), statView );

	menu_item = gtk_menu_item_new_with_label ("Copy all");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( CopyAll ), pwNotebook );

	g_signal_connect( G_OBJECT( statView ), "button-press-event", G_CALLBACK( ContextMenu ), copyMenu );

	/* dialog size */
	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( pwUsePanels ) ) )
	    gtk_window_set_default_size( GTK_WINDOW( pwStatDialog ), 0, 300 );
	else {
	    GtkRequisition req;
	    gtk_widget_size_request( GTK_WIDGET( pwStatDialog ), &req );
	    if ( req.height < 600 )
		gtk_window_set_default_size( GTK_WINDOW( pwStatDialog ), 0, 600 );
	}

	copyMenu = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_label ("Copy page");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( CopyPage ), pwNotebook );

	menu_item = gtk_menu_item_new_with_label ("Copy all pages");
	gtk_menu_shell_append (GTK_MENU_SHELL (copyMenu), menu_item);
	gtk_widget_show (menu_item);
	g_signal_connect( G_OBJECT( menu_item ), "activate", G_CALLBACK( CopyAll ), pwNotebook );

	g_signal_connect( G_OBJECT( pwNotebook ), "button-press-event", G_CALLBACK( ContextMenu ), copyMenu );

	gtk_combo_box_set_active(GTK_COMBO_BOX(navi_combo), game);

	g_signal_connect(pwStatDialog, "map", G_CALLBACK(stat_dialog_map), pwUsePanels);

	GTKRunDialog(pwStatDialog);

#if USE_BOARD3D
	TidyGraphData(gd);
#endif
}

extern int
GTKGetMove ( int anMove[ 8 ] ) {

  BoardData *bd = BOARD ( pwBoard )->board_data;

  if ( !bd->valid_move )
    return 0;
  
  memcpy ( anMove, bd->valid_move->anMove, 8 * sizeof ( int ) );

  return 1;

}

typedef struct _recordwindowinfo {
    GtkWidget *pwList, *pwTable, *apwStats[ 22 ];
    int nRow;
} recordwindowinfo;

static void UpdateMatchinfo( const char *pch, const char *szParam, char **ppch ) {

    char *szCommand; 
    const char *pchOld = *ppch ? *ppch : "";
    
    if( !strcmp( pch, pchOld ) )
	/* no change */
	return;

    szCommand = g_strdup_printf( "set matchinfo %s %s", szParam, pch );
    UserCommand( szCommand );
    g_free( szCommand );
}

/* Variables for match info dialog */
GtkWidget *apwRating[ 2 ], *pwDate, *pwEvent,
		*pwRound, *pwPlace, *pwAnnotator;
GtkTextBuffer *buffer;

static void MatchInfoOK( GtkWidget *pw, int *pf )
{
    GtkTextIter begin, end;
	unsigned int nYear, nMonth, nDay;
	char *pch;
	
	outputpostpone();

	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( apwRating[ 0 ] ) ),
			 "rating 0", &mi.pchRating[ 0 ] );
	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( apwRating[ 1 ] ) ),
			 "rating 1", &mi.pchRating[ 1 ] );
	
	gtk_calendar_get_date( GTK_CALENDAR( pwDate ), &nYear, &nMonth,
			       &nDay );
	nMonth++;
	if( mi.nYear && !nDay )
	    UserCommand( "set matchinfo date" );
	else if( nDay && ( !mi.nYear || mi.nYear != nYear ||
			   mi.nMonth != nMonth || mi.nDay != nDay ) ) {
	    char sz[ 64 ];
	    sprintf( sz, "set matchinfo date %04d-%02d-%02d", nYear, nMonth,
		     nDay );
	    UserCommand( sz );
	}
	    
	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( pwEvent ) ),
			 "event", &mi.pchEvent );
	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( pwRound ) ),
			 "round", &mi.pchRound );
	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( pwPlace ) ),
			 "place", &mi.pchPlace );
	UpdateMatchinfo( gtk_entry_get_text( GTK_ENTRY( pwAnnotator ) ),
			 "annotator", &mi.pchAnnotator );

        gtk_text_buffer_get_bounds (buffer, &begin, &end);
        pch = gtk_text_buffer_get_text(buffer, &begin, &end, FALSE);
	UpdateMatchinfo( pch, "comment", &mi.pchComment );
	g_free( pch );

	outputresume();

    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static void AddToTable(GtkWidget* pwTable, char* str, int x, int y)
{
	GtkWidget* pw = gtk_label_new(str);
	/* Right align */
	gtk_misc_set_alignment(GTK_MISC(pw), 1, .5);
	gtk_table_attach(GTK_TABLE(pwTable), pw, x, x + 1, y, y + 1,
		      GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
}

extern void GTKMatchInfo( void )
{
    int fOK = FALSE;
    GtkWidget *pwDialog, *pwTable, *pwComment;
    char sz[ 128 ];
    
    pwDialog = GTKCreateDialog( _("GNU Backgammon - Match information"),
		DT_QUESTION, NULL, DIALOG_FLAG_MODAL, G_CALLBACK(MatchInfoOK), &fOK ),

    pwTable = gtk_table_new( 5, 7, FALSE );
    gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
		       pwTable );

    sprintf( sz, _("%s's rating:"), ap[ 0 ].szName );
	AddToTable(pwTable, sz, 0, 0);

    sprintf( sz, _("%s's rating:"), ap[ 1 ].szName );
	AddToTable(pwTable, sz, 0, 1);

	AddToTable(pwTable, _("Date:"), 0, 2);
	AddToTable(pwTable, _("Event:"), 0, 3);
	AddToTable(pwTable, _("Round:"), 0, 4);
	AddToTable(pwTable, _("Place:"), 0, 5);
	AddToTable(pwTable, _("Annotator:"), 0, 6);

    gtk_table_attach( GTK_TABLE( pwTable ),
		      gtk_label_new( _("Comments:") ),
		      2, 3, 0, 1, 0, 0, 0, 0 );

    apwRating[ 0 ] = gtk_entry_new();
    if( mi.pchRating[ 0 ] )
	gtk_entry_set_text( GTK_ENTRY( apwRating[ 0 ] ), mi.pchRating[ 0 ] );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), apwRating[ 0 ], 1, 2,
			       0, 1 );
    
    apwRating[ 1 ] = gtk_entry_new();
    if( mi.pchRating[ 1 ] )
	gtk_entry_set_text( GTK_ENTRY( apwRating[ 1 ] ), mi.pchRating[ 1 ] );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), apwRating[ 1 ], 1, 2,
			       1, 2 );
    
    pwDate = gtk_calendar_new();
    if( mi.nYear ) {
	gtk_calendar_select_month( GTK_CALENDAR( pwDate ), mi.nMonth - 1,
				   mi.nYear );
	gtk_calendar_select_day( GTK_CALENDAR( pwDate ), mi.nDay );
    } else
	gtk_calendar_select_day( GTK_CALENDAR( pwDate ), 0 );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwDate, 1, 2, 2, 3 );

    pwEvent = gtk_entry_new();
    if( mi.pchEvent )
	gtk_entry_set_text( GTK_ENTRY( pwEvent ), mi.pchEvent );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwEvent, 1, 2, 3, 4 );
    
    pwRound = gtk_entry_new();
    if( mi.pchRound )
	gtk_entry_set_text( GTK_ENTRY( pwRound ), mi.pchRound );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwRound, 1, 2, 4, 5 );
    
    pwPlace = gtk_entry_new();
    if( mi.pchPlace )
	gtk_entry_set_text( GTK_ENTRY( pwPlace ), mi.pchPlace );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwPlace, 1, 2, 5, 6 );
    
    pwAnnotator = gtk_entry_new();
    if( mi.pchAnnotator )
	gtk_entry_set_text( GTK_ENTRY( pwAnnotator ), mi.pchAnnotator );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwAnnotator, 1, 2, 6, 7 );
        
    pwComment = gtk_text_view_new() ;
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pwComment));
    if( mi.pchComment ) {
	gtk_text_buffer_set_text(buffer, mi.pchComment, -1);
    }
    gtk_text_view_set_wrap_mode( GTK_TEXT_VIEW( pwComment ), GTK_WRAP_WORD );
    gtk_table_attach_defaults( GTK_TABLE( pwTable ), pwComment, 2, 5, 1, 7 );

    gtk_window_set_default_size( GTK_WINDOW( pwDialog ), 500, 0 );

	GTKRunDialog(pwDialog);
}

static void CalibrationOK( GtkWidget *pw, GtkWidget *ppw ) {

    char sz[ 128 ];
    GtkAdjustment *padj = gtk_spin_button_get_adjustment(
	GTK_SPIN_BUTTON( ppw ) );
    
    if( GTK_WIDGET_IS_SENSITIVE( ppw ) ) {
	if( padj->value != rEvalsPerSec ) {
	    sprintf( sz, "set calibration %.0f", padj->value );
	    UserCommand( sz );
	}
    } else if( rEvalsPerSec > 0 )
	UserCommand( "set calibration" );
    
    UserCommand("save settings");
    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
}

static void CalibrationEnable( GtkWidget *pw, GtkWidget *pwspin ) {

    gtk_widget_set_sensitive( pwspin, gtk_toggle_button_get_active(
				  GTK_TOGGLE_BUTTON( pw ) ) );
}

static void CalibrationGo( GtkWidget *pw, GtkWidget *apw[ 2 ] )
{
	GTKSetCurrentParent(pw);
    UserCommand( "calibrate" );

    fInterrupt = FALSE;
    
    if( rEvalsPerSec > 0 ) {
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( apw[ 0 ] ), TRUE );
	gtk_adjustment_set_value( gtk_spin_button_get_adjustment(
				      GTK_SPIN_BUTTON( apw[ 1 ] ) ),
				  rEvalsPerSec );
    }
}

extern void GTKShowCalibration( void )
{
    GtkAdjustment *padj;
    GtkWidget *pwDialog, *pwvbox, *pwhbox, *pwenable, *pwspin, *pwbutton,
	*apw[ 2 ];
    
    padj = GTK_ADJUSTMENT( gtk_adjustment_new( rEvalsPerSec > 0 ?
					       rEvalsPerSec : 10000,
					       2, G_MAXFLOAT, 100,
					       1000, 0 ) );
    pwspin = gtk_spin_button_new( padj, 100, 0 );
    /* FIXME should be modal but presently causes crash and/or killing of the main window */
    pwDialog = GTKCreateDialog( _("GNU Backgammon - Speed estimate"), DT_QUESTION,
		NULL, 0, G_CALLBACK( CalibrationOK ), pwspin );
    gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
		       pwvbox = gtk_vbox_new( FALSE, 8 ) );
    gtk_container_set_border_width( GTK_CONTAINER( pwvbox ), 8 );
    gtk_container_add( GTK_CONTAINER( pwvbox ),
		       pwhbox = gtk_hbox_new( FALSE, 8 ) );
    gtk_container_add( GTK_CONTAINER( pwhbox ),
		       pwenable = gtk_check_button_new_with_label(
			   _("Speed recorded:") ) );
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( pwenable ),
				  rEvalsPerSec > 0 );
    
    gtk_container_add( GTK_CONTAINER( pwhbox ), pwspin );
    gtk_widget_set_sensitive( pwspin, rEvalsPerSec > 0 );
    
    gtk_container_add( GTK_CONTAINER( pwhbox ), gtk_label_new(
			   _("static evaluations/second") ) );

    gtk_container_add( GTK_CONTAINER( pwvbox ),
		       pwbutton = gtk_button_new_with_label(
			   _("Calibrate") ) );
    apw[ 0 ] = pwenable;
    apw[ 1 ] = pwspin;
    g_signal_connect( G_OBJECT( pwbutton ), "clicked",
			G_CALLBACK( CalibrationGo ), apw );

    g_signal_connect( G_OBJECT( pwenable ), "toggled",
			G_CALLBACK( CalibrationEnable ), pwspin );
    
	GTKRunDialog(pwDialog);
}

static gboolean CalibrationCancel( GtkObject *po, gpointer p ) {

    fInterrupt = TRUE;

    return TRUE;
}

extern void *GTKCalibrationStart( void ) {

    GtkWidget *pwDialog, *pwhbox, *pwResult;
    
    pwDialog = GTKCreateDialog( _("GNU Backgammon - Calibration"), DT_INFO,
		NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOTIDY, G_CALLBACK( CalibrationCancel ), NULL );
    gtk_container_add( GTK_CONTAINER( DialogArea( pwDialog, DA_MAIN ) ),
		       pwhbox = gtk_hbox_new( FALSE, 8 ) );
    gtk_container_set_border_width( GTK_CONTAINER( pwhbox ), 8 );
    gtk_container_add( GTK_CONTAINER( pwhbox ),
		       gtk_label_new( _("Calibrating:") ) );
    gtk_container_add( GTK_CONTAINER( pwhbox ),
		       pwResult = gtk_label_new( _("       (n/a)       ") ) );
    gtk_container_add( GTK_CONTAINER( pwhbox ),
		       gtk_label_new( _("static evaluations/second") ) );
    
    pwOldGrab = pwGrab;
    pwGrab = pwDialog;
    
    g_signal_connect( G_OBJECT( pwDialog ), "delete_event",
			G_CALLBACK( CalibrationCancel ), NULL );
    
    gtk_widget_show_all( pwDialog );

	ProcessEvents();

    gtk_widget_ref( pwResult );
    
    return pwResult;
}

extern void GTKCalibrationUpdate( void *context, float rEvalsPerSec ) {

    char sz[ 32 ];

    sprintf( sz, "%.0f", rEvalsPerSec );
    gtk_label_set_text( GTK_LABEL( context ), sz );
    
	ProcessEvents();
}

extern void GTKCalibrationEnd( void *context ) {

    gtk_widget_unref( GTK_WIDGET( context ) );
    
    gtk_widget_destroy( gtk_widget_get_toplevel( GTK_WIDGET( context ) ) );

    pwGrab = pwOldGrab;
}

static void CallbackResign(GtkWidget *pw, gpointer data)
{
    int i = GPOINTER_TO_INT(data);
    const char *asz[3]= { "normal", "gammon", "backgammon" };
    char sz[20];

    sprintf(sz, "resign %s", asz[i]);
    gtk_widget_destroy( gtk_widget_get_toplevel( pw ) );
    UserCommand(sz);

    return;
}
    
extern void GTKResign(gpointer p, guint n, GtkWidget * pw)
{
	GtkWidget *pwDialog, *pwVbox, *pwHbox, *pwButtons;
	int i;
	const char *asz[3] = { N_("Resign normal"),
		N_("Resign gammon"),
		N_("Resign backgammon")
	};
	const char *resign_stocks[3] = {GNUBG_STOCK_RESIGNSN, GNUBG_STOCK_RESIGNSG, GNUBG_STOCK_RESIGNSB};

	if (ap[ !ms.fTurn ].pt != PLAYER_HUMAN && check_resigns(NULL) != -1
		&& GTKShowWarning(WARN_RESIGN, NULL))
	{	/* Automatically resign for computer */
		UserCommand("resign -1");
		while (nNextTurn)
			NextTurnNotify(NULL);
		if (!ms.fResignationDeclined)
			return;
	}
	pwDialog = GTKCreateDialog(_("Resign"), DT_QUESTION, NULL, DIALOG_FLAG_MODAL | DIALOG_FLAG_NOOK, NULL, NULL);

	pwVbox = gtk_vbox_new(TRUE, 5);

	for (i = 0; i < 3; i++) {
		pwButtons = gtk_button_new();
		pwHbox = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(pwButtons), pwHbox);
		gtk_box_pack_start(GTK_BOX(pwHbox), gtk_image_new_from_stock(resign_stocks[i], GTK_ICON_SIZE_LARGE_TOOLBAR), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(pwHbox), gtk_label_new(_(asz[i])), TRUE, TRUE, 10);
		gtk_container_add(GTK_CONTAINER(pwVbox), pwButtons);
		g_signal_connect(G_OBJECT(pwButtons), "clicked", G_CALLBACK(CallbackResign), GINT_TO_POINTER(i));
	}

	gtk_container_add(GTK_CONTAINER(DialogArea(pwDialog, DA_MAIN)), pwVbox);

	GTKRunDialog(pwDialog);
}

extern void MoveListDestroy(void)
{
	if (pwMoveAnalysis)
	{
		gtk_widget_destroy(pwMoveAnalysis);
		pwMoveAnalysis = NULL;
	}
}
