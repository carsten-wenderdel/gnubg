/*
* gtkpanels.c
* by Jon Kinsey, 2004
*
* Panels/window code
*
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

#include "config.h"

#include <stdlib.h>
#if HAVE_LIMITS_H
#include <limits.h>
#endif
#include <ctype.h>
#define GTK_ENABLE_BROKEN /* for GtkText */
#include "backgammon.h"
#include <i18n.h>

#if USE_GTK
#include "gtkboard.h"
#include "gtkgame.h"
#include "positionid.h"

extern GtkItemFactory *pif;
extern GtkWidget *pom;

extern GtkWidget* GL_Create();
extern GtkWidget *StatsPixmapButton(GdkColormap *pcmap, char **xpm, void *fn);

static void CreatePanel(gnubgwindow window, GtkWidget* pWidget, char* winTitle, char* windowRole);

#endif

int fDisplayPanels = TRUE;
int fDockPanels = TRUE;

extern void ShowAnalysis();
extern void DeleteAnalysis();
extern void ShowAnnotation();
extern void DeleteAnnotation();
extern void ShowGameWindow();
extern void DeleteGame();
extern void ShowMessage();
extern void DeleteMessage();
extern void ShowTheoryWindow();
extern void DeleteTheoryWindow();
extern void ShowCommandWindow();
extern void DeleteCommandWindow();

typedef void (*panelFun)();

typedef struct _windowobject {
	char* winName;
	int showing;
	int docked;
	int dockable;
	int undockable;
#if USE_GTK
	panelFun showFun;
	panelFun hideFun;
	GtkWidget* pwWin;
#endif
	windowgeometry wg;
} windowobject;

/* Set up window and panel details */
windowobject woPanel[NUM_WINDOWS] =
{
	/* main window */
	{
		"main",
		TRUE, FALSE, FALSE, FALSE,
#if USE_GTK
		NULL, NULL,
		0,
#endif
		{0, 0, 20, 20}
	},
	/* game list */
	{
		"game",
		TRUE, TRUE, TRUE, TRUE,
#if USE_GTK
		ShowGameWindow, DeleteGame,
		0,
#endif
		{ 250, 200, 20, 20 }
	},
	/* analysis */
	{
		"analysis",
		FALSE, TRUE, TRUE, TRUE,
#if USE_GTK
		ShowAnalysis, DeleteAnalysis,
		0,
#endif
		{ 0, 400, 20, 20 }
	},
	/* annotation */
	{
		"annotation",
		FALSE, TRUE, TRUE, FALSE,
#if USE_GTK
		ShowAnnotation, DeleteAnnotation,
		0,
#endif
		{ 0, 400, 20, 20 }
	},
	/* hint */
	{
		"hint",
		FALSE, FALSE, FALSE, FALSE,
#if USE_GTK
		NULL, NULL,
		0,
#endif
		{ 0, 450, 20, 20 }
	},
	/* message */
	{
		"message",
		FALSE, TRUE, TRUE, TRUE,
#if USE_GTK
		ShowMessage, DeleteMessage,
		0,
#endif
		{ 0, 500, 20, 20 }
	},
	/* command */
	{
		"command",
		FALSE, TRUE, TRUE, TRUE,
#if USE_GTK
		ShowCommandWindow, DeleteCommandWindow,
		0,
#endif
		{ 0, 0, 20, 20 }
	}, 
	/* theory */
	{
		"theory",
		FALSE, TRUE, TRUE, TRUE,
#if USE_GTK
		ShowTheoryWindow, DeleteTheoryWindow,
		0,
#endif
		{ 0, 0, 20, 20 }
	}
};

#if USE_GTK

extern void ShowPanel(gnubgwindow window)
{
	setWindowGeometry(window);
	if (!woPanel[window].docked && woPanel[window].pwWin->window)
		gdk_window_raise(woPanel[window].pwWin->window);

	woPanel[window].showing = TRUE;
	/* Avoid showing before main window */
	if (GTK_WIDGET_REALIZED(pwMain))
		gtk_widget_show_all(woPanel[window].pwWin);
}

extern void ShowGameWindow( void )
{
	ShowPanel(WINDOW_GAME);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Game record")), TRUE);
}

extern void ShowAnnotation( void )
{
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Commentary")), TRUE);

	woPanel[WINDOW_ANNOTATION].showing = TRUE;
	/* Avoid showing before main window */
	if( GTK_WIDGET_REALIZED( pwMain ) )
		gtk_widget_show_all( woPanel[WINDOW_ANNOTATION].pwWin );
}

extern void ShowMessage( void )
{
	ShowPanel(WINDOW_MESSAGE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Message")), TRUE);
}

extern void ShowAnalysis( void )
{
	ShowPanel(WINDOW_ANALYSIS);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
		"/Windows/Analysis")), TRUE);
}

extern void ShowTheoryWindow( void )
{
	ShowPanel(WINDOW_THEORY);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Theory")), TRUE);
}

extern void ShowCommandWindow( void )
{
	ShowPanel(WINDOW_COMMAND);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Command")), TRUE);
}

static void CreateMessageWindow( void )
{
	GtkWidget *vscrollbar, *pwhbox;  
	GtkWidget *pwvbox = gtk_vbox_new ( TRUE, 0 ) ;

	CreatePanel(WINDOW_MESSAGE, pwvbox, _("Messages - GNU Backgammon"), "messages");

	gtk_box_pack_start ( GTK_BOX ( pwvbox ), 
						pwhbox = gtk_hbox_new ( FALSE, 0 ), FALSE, TRUE, 0);

	pwMessageText = gtk_text_new ( NULL, NULL );

	gtk_text_set_word_wrap( GTK_TEXT( pwMessageText ), TRUE );
	gtk_text_set_editable( GTK_TEXT( pwMessageText ), FALSE );

	vscrollbar = gtk_vscrollbar_new (GTK_TEXT(pwMessageText)->vadj);
	gtk_box_pack_start(GTK_BOX(pwhbox), pwMessageText, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(pwhbox), vscrollbar, FALSE, FALSE, 0);
}

GtkWidget *pwTheoryList = NULL;

extern char *ReturnHits( int anBoard[ 2 ][ 25 ] );

void UpdateTheoryData(BoardData* bd, int UpdateType, int points[2][25])
{
	char* pc;

	if (!pwTheoryList)
		return;

	if (UpdateType & TT_PIPCOUNT)
	{
		if (ms.gs != GAME_NONE)
		{
			int diff;
			int anPip[ 2 ];
			PipCount(points, anPip);

			diff = anPip[0] - anPip[1];
			if (diff == 0)
    			pc = g_strdup_printf(_("equal"));
			else
    			pc = g_strdup_printf("%d %s", abs(anPip[0] - anPip[1]),
    				(diff > 0) ? _("ahead") : _("behind"));

			gtk_clist_set_text(GTK_CLIST(pwTheoryList), 0, 1, pc);

			g_free(pc);
		}
	}
	if (UpdateType & TT_EPC)
	{
		if (ms.gs != GAME_NONE)
		{
			float arEPC[ 2 ];

			if ( EPC( points, arEPC, NULL, NULL, NULL, TRUE ) )
			{	/* no EPCs avaiable */
				gtk_clist_set_text(GTK_CLIST(pwTheoryList), 1, 1, "");
			}
			else
			{
				pc = g_strdup_printf("%.2f (%+.1f)", 
									arEPC[1], arEPC[1] - arEPC[0]);
				gtk_clist_set_text(GTK_CLIST(pwTheoryList), 1, 1, pc);
				g_free( pc );
			}
		}
	}

	if (UpdateType & TT_RETURNHITS)
	{
		int anBoard[ 2 ][ 25 ];
		pc = NULL;
		if ( bd->valid_move )
		{
			PositionFromKey( anBoard, bd->valid_move->auch );
			pc = ReturnHits( anBoard );
		}
		if (pc)
		{
			gtk_clist_set_text(GTK_CLIST(pwTheoryList), 2, 1, pc);
			g_free( pc );
		}
		else
			gtk_clist_set_text(GTK_CLIST(pwTheoryList), 2, 1, "");
	}

	if (UpdateType & TT_KLEINCOUNT)
	{
		if (ms.gs != GAME_NONE)
		{
			float fKC;
			int anPip[ 2 ];
			PipCount(points, anPip);

			fKC = KleinmanCount(anPip[1], anPip[0]);
			if (fKC != -1)
			{
    			pc = g_strdup_printf("%.4f", fKC);
				gtk_clist_set_text(GTK_CLIST(pwTheoryList), 3, 1, pc);
				g_free(pc);
			}
			else
				gtk_clist_set_text(GTK_CLIST(pwTheoryList), 3, 1, "");
		}
	}
}

static GtkWidget *CreateTheoryWindow( void )
{
	static char *row[] = { NULL, NULL };
	pwTheoryList = gtk_clist_new(2);

	gtk_clist_set_column_auto_resize( GTK_CLIST( pwTheoryList ), 0, TRUE );
	gtk_clist_set_column_auto_resize( GTK_CLIST( pwTheoryList ), 1, TRUE );
	gtk_clist_set_column_justification( GTK_CLIST( pwTheoryList ), 0, GTK_JUSTIFY_RIGHT );

	CreatePanel(WINDOW_THEORY, pwTheoryList, _("Theory - GNU Backgammon"), "theory");

	row[0] = _("Pip count");
	gtk_clist_append(GTK_CLIST(pwTheoryList), row);
	row[0] = _("EPC");
	gtk_clist_append(GTK_CLIST(pwTheoryList), row);
	row[0] = _("Return hits");
	gtk_clist_append(GTK_CLIST(pwTheoryList), row);
	row[0] = _("Kleinman count");
	gtk_clist_append(GTK_CLIST(pwTheoryList), row);

	return woPanel[WINDOW_THEORY].pwWin;
}

extern struct CommandEntryData_T cedPanel;

static GtkWidget *CreateCommandWindow( void )
{
    GtkWidget *pwhbox;
    GtkWidget *pwvbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *pwShowHelp;

	CreatePanel(WINDOW_COMMAND, pwvbox, _("Command - GNU Backgammon"), "command");

	cedPanel.cmdString = NULL;
	cedPanel.pwHelpText = pwMessageText;

	cedPanel.cmdEntryCombo = gtk_combo_new();
	gtk_combo_set_value_in_list(GTK_COMBO(cedPanel.cmdEntryCombo), FALSE, TRUE);
	gtk_combo_disable_activate(GTK_COMBO(cedPanel.cmdEntryCombo));
	cedPanel.pwEntry = GTK_COMBO(cedPanel.cmdEntryCombo)->entry;

	PopulateCommandHistory(&cedPanel);

	gtk_signal_connect(GTK_OBJECT(cedPanel.pwEntry), "changed", GTK_SIGNAL_FUNC(CommandTextChange), &cedPanel);
	gtk_signal_connect(GTK_OBJECT(cedPanel.pwEntry), "key-press-event", GTK_SIGNAL_FUNC(CommandKeyPress), &cedPanel);
	gtk_signal_connect(GTK_OBJECT(cedPanel.pwEntry), "activate", GTK_SIGNAL_FUNC(CommandOK), &cedPanel);

	pwhbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start ( GTK_BOX ( pwvbox ), pwhbox, FALSE, FALSE, 0);

	gtk_box_pack_start( GTK_BOX( pwhbox ), cedPanel.cmdEntryCombo, TRUE, TRUE, 10);

	pwShowHelp = gtk_toggle_button_new_with_label( _("Help") );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pwShowHelp), cedPanel.showHelp);
	gtk_signal_connect(GTK_OBJECT(pwShowHelp), "toggled", GTK_SIGNAL_FUNC(ShowHelpToggled), &cedPanel);

	gtk_box_pack_start ( GTK_BOX ( pwhbox ), pwShowHelp, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(pwShowHelp), "focus-in-event", GTK_SIGNAL_FUNC(CommandFocusIn), &cedPanel);

	return woPanel[WINDOW_COMMAND].pwWin;
}

static GtkWidget *CreateAnalysisWindow( void ) {

    GtkWidget *vscrollbar, *pHbox;
	if (!woPanel[WINDOW_ANALYSIS].docked)
	{
		GtkWidget *pwPaned = gtk_vpaned_new() ;

		woPanel[WINDOW_ANALYSIS].pwWin = gtk_window_new( GTK_WINDOW_TOPLEVEL );

		gtk_window_set_title( GTK_WINDOW( woPanel[WINDOW_ANALYSIS].pwWin ),
				  _("Annotation - GNU Backgammon") );
#if GTK_CHECK_VERSION(2,0,0)
	    gtk_window_set_role( GTK_WINDOW( woPanel[WINDOW_ANALYSIS].pwWin ), "annotation" );
#if GTK_CHECK_VERSION(2,2,0)
		gtk_window_set_type_hint( GTK_WINDOW( woPanel[WINDOW_ANALYSIS].pwWin ),
			      GDK_WINDOW_TYPE_HINT_UTILITY );
#endif
#endif

		setWindowGeometry(WINDOW_ANALYSIS);

		gtk_container_add( GTK_CONTAINER( woPanel[WINDOW_ANALYSIS].pwWin ), pwPaned); 
		gtk_window_add_accel_group( GTK_WINDOW( woPanel[WINDOW_ANALYSIS].pwWin ), pagMain );
    
		gtk_paned_pack1( GTK_PANED( pwPaned ),
				 pwAnalysis = gtk_label_new( NULL ), TRUE, FALSE );
    
		gtk_paned_pack2( GTK_PANED( pwPaned ),
				 pHbox = gtk_hbox_new( FALSE, 0 ), FALSE, TRUE );
	}
	else
	{
		pHbox = gtk_hbox_new( FALSE, 0 );
		gtk_box_pack_start( GTK_BOX( pHbox ), pwAnalysis = gtk_label_new( NULL ), TRUE, TRUE, 0 );
		pHbox = gtk_hbox_new( FALSE, 0 );
	}

    pwCommentary = gtk_text_new( NULL, NULL ) ;

    gtk_text_set_word_wrap( GTK_TEXT( pwCommentary ), TRUE );
    gtk_text_set_editable( GTK_TEXT( pwCommentary ), TRUE );
    gtk_widget_set_sensitive( pwCommentary, FALSE );

    vscrollbar = gtk_vscrollbar_new (GTK_TEXT(pwCommentary)->vadj);
    gtk_box_pack_start(GTK_BOX(pHbox), pwCommentary, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(pHbox), vscrollbar, FALSE, FALSE, 0);

    gtk_signal_connect( GTK_OBJECT( pwCommentary ), "changed",
			GTK_SIGNAL_FUNC( CommentaryChanged ), NULL );

	if (!woPanel[WINDOW_ANALYSIS].docked)
	{
	    gtk_signal_connect( GTK_OBJECT( woPanel[WINDOW_ANALYSIS].pwWin ), "delete_event",
			GTK_SIGNAL_FUNC(woPanel[WINDOW_ANALYSIS].hideFun), NULL );
		return woPanel[WINDOW_ANALYSIS].pwWin;
	}
	else
	{
		woPanel[WINDOW_ANALYSIS].pwWin = pwAnalysis->parent;
		return pHbox;
	}
}

static void ButtonCommand( GtkWidget *pw, char *szCommand ) {

    UserCommand( szCommand );
}

static GtkWidget *PixmapButton( GdkColormap *pcmap, char **xpm,
				char *szCommand ) {
    
    GdkPixmap *ppm;
    GdkBitmap *pbm;
    GtkWidget *pw, *pwButton;

    ppm = gdk_pixmap_colormap_create_from_xpm_d( NULL, pcmap, &pbm, NULL,
						 xpm );
    pw = gtk_pixmap_new( ppm, pbm );
    pwButton = gtk_button_new();
    gtk_container_add( GTK_CONTAINER( pwButton ), pw );
    
    gtk_signal_connect( GTK_OBJECT( pwButton ), "clicked",
			GTK_SIGNAL_FUNC( ButtonCommand ), szCommand );
    
    return pwButton;
}

static void CreateGameWindow( void ) {

    GtkWidget *psw = gtk_scrolled_window_new( NULL, NULL ),
	*pvbox = gtk_vbox_new( FALSE, 0 ),
	*phbox = gtk_hbox_new( FALSE, 0 ),
	*pm = gtk_menu_new(),
	*pw;
    GdkColormap *pcmap;

#include "xpm/prevgame.xpm"
#include "xpm/prevmove.xpm"
#include "xpm/nextmove.xpm"
#include "xpm/nextgame.xpm"
#include "xpm/prevmarked.xpm"
#include "xpm/nextmarked.xpm"

    pcmap = gtk_widget_get_colormap( pwMain );
	if (!woPanel[WINDOW_GAME].docked)
	{
		woPanel[WINDOW_GAME].pwWin = gtk_window_new( GTK_WINDOW_TOPLEVEL );

		gtk_window_set_title( GTK_WINDOW( woPanel[WINDOW_GAME].pwWin ),
			_("Game record - GNU Backgammon") );
#if GTK_CHECK_VERSION(2,0,0)
		gtk_window_set_role( GTK_WINDOW( woPanel[WINDOW_GAME].pwWin ), "game record" );
#if GTK_CHECK_VERSION(2,2,0)
		gtk_window_set_type_hint( GTK_WINDOW( woPanel[WINDOW_GAME].pwWin ),
			      GDK_WINDOW_TYPE_HINT_UTILITY );
#endif
#endif

		setWindowGeometry(WINDOW_GAME);
    
		gtk_container_add( GTK_CONTAINER( woPanel[WINDOW_GAME].pwWin ), pvbox );
		gtk_window_add_accel_group( GTK_WINDOW( woPanel[WINDOW_GAME].pwWin ), pagMain );
	}
    gtk_box_pack_start( GTK_BOX( pvbox ), phbox, FALSE, FALSE, 4 );

    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, prevgame_xpm,
					   "previous game" ),
			FALSE, FALSE, 4 );
    gtk_tooltips_set_tip( ptt, pw, _("Move back to the previous game"), "" );
    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, prevmove_xpm,
					   "previous roll" ),
			FALSE, FALSE, 0 );
    gtk_tooltips_set_tip( ptt, pw, _("Move back to the previous roll"), "" );
    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, nextmove_xpm,
					   "next roll" ),
			FALSE, FALSE, 4 );
    gtk_tooltips_set_tip( ptt, pw, _("Move ahead to the next roll"), "" );
    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, nextgame_xpm,
				      "next game" ),
			FALSE, FALSE, 0 );
    gtk_tooltips_set_tip( ptt, pw, _("Move ahead to the next game"), "" );

    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, prevmarked_xpm,
					   "previous marked" ),
			FALSE, FALSE, 4 );
    gtk_tooltips_set_tip( ptt, pw, _("Move back to the previous marked "
				     "decision" ), "" );
    gtk_box_pack_start( GTK_BOX( phbox ),
			pw = PixmapButton( pcmap, nextmarked_xpm,
					   "next marked" ),
			FALSE, FALSE, 0 );
    gtk_tooltips_set_tip( ptt, pw, _("Move ahead to the next marked "
				     "decision" ), "" );
        
    gtk_menu_append( GTK_MENU( pm ), gtk_menu_item_new_with_label(
	_("(no game)") ) );
    gtk_widget_show_all( pm );
    gtk_option_menu_set_menu( GTK_OPTION_MENU( pom = gtk_option_menu_new() ),
			      pm );
    gtk_option_menu_set_history( GTK_OPTION_MENU( pom ), 0 );
    gtk_box_pack_start( GTK_BOX( phbox ), pom, TRUE, TRUE, 4 );
    
    gtk_container_add( GTK_CONTAINER( pvbox ), psw );
    gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( psw ),
				    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS );

    gtk_container_add( GTK_CONTAINER( psw ), GL_Create());

	if (!woPanel[WINDOW_GAME].docked)
	{
	    gtk_signal_connect( GTK_OBJECT( woPanel[WINDOW_GAME].pwWin ), "delete_event",
			GTK_SIGNAL_FUNC(woPanel[WINDOW_GAME].hideFun), NULL );
	}
	else
		woPanel[WINDOW_GAME].pwWin = pvbox;
}

static void CreateHeadWindow(gnubgwindow panel, const char* sz, GtkWidget* pwWidge)
{
	#include "xpm/x.xpm"
	GtkWidget* pwLab = gtk_label_new( sz );
	GtkWidget* pwVbox = gtk_vbox_new(FALSE, 0);
	GtkWidget* pwHbox = gtk_hbox_new(FALSE, 0);
	GdkColormap *pcmap = gtk_widget_get_colormap( pwMain );
	GtkWidget* pwX = StatsPixmapButton(pcmap, x_xpm, woPanel[panel].hideFun);

	gtk_box_pack_start( GTK_BOX( pwVbox ), pwHbox, FALSE, FALSE, 0 );
	gtk_box_pack_start( GTK_BOX( pwHbox ), pwLab, FALSE, FALSE, 10 );
	gtk_box_pack_end( GTK_BOX( pwHbox ), pwX, FALSE, FALSE, 1 );
	gtk_box_pack_start( GTK_BOX( pwVbox ), pwWidge, TRUE, TRUE, 0 );

	woPanel[panel].pwWin = pwVbox;
}

static void CreatePanels()
{
   CreateGameWindow();
   gtk_box_pack_start( GTK_BOX( pwPanelVbox ), woPanel[WINDOW_GAME].pwWin, TRUE, TRUE, 0 );

   CreateHeadWindow(WINDOW_ANNOTATION, _("Commentary"), CreateAnalysisWindow());
   gtk_box_pack_start( GTK_BOX( pwPanelVbox ), woPanel[WINDOW_ANALYSIS].pwWin, FALSE, FALSE, 0 );
   gtk_box_pack_start( GTK_BOX( pwPanelVbox ), woPanel[WINDOW_ANNOTATION].pwWin, FALSE, FALSE, 0 );

   CreateMessageWindow();
   CreateHeadWindow(WINDOW_MESSAGE, _("Messages"), woPanel[WINDOW_MESSAGE].pwWin);
   gtk_box_pack_start(GTK_BOX(pwPanelVbox), woPanel[WINDOW_MESSAGE].pwWin, FALSE, FALSE, 0 );

   CreateHeadWindow(WINDOW_COMMAND, _("Command"), CreateCommandWindow());
   gtk_box_pack_start(GTK_BOX(pwPanelVbox), woPanel[WINDOW_COMMAND].pwWin, FALSE, FALSE, 0 );

   CreateHeadWindow(WINDOW_THEORY, _("Theory"), CreateTheoryWindow());
   gtk_box_pack_start(GTK_BOX(pwPanelVbox), woPanel[WINDOW_THEORY].pwWin, FALSE, FALSE, 0 );
}

extern void DeleteMessage ( void )
{
	HidePanel(WINDOW_MESSAGE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Message")), FALSE);
}

extern void DeleteAnalysis( void )
{
	HidePanel(WINDOW_ANALYSIS);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Analysis")), FALSE);
}

extern void DeleteAnnotation( void )
{
	HidePanel(WINDOW_ANNOTATION);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Commentary")), FALSE);
}

extern void DeleteGame( void )
{
	HidePanel(WINDOW_GAME);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Game record")), FALSE);
}

extern void DeleteTheoryWindow ( void )
{
	HidePanel(WINDOW_THEORY);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Theory")), FALSE);
}

extern void DeleteCommandWindow ( void )
{
	HidePanel(WINDOW_COMMAND);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif,
			  "/Windows/Command")), FALSE);
}

static void GetGeometryString(char* buf, windowobject* pwo)
{
	sprintf(buf, "set geometry %s width %d\n"
                  "set geometry %s height %d\n"
                  "set geometry %s xpos %d\n" 
                  "set geometry %s ypos %d\n", 
                  pwo->winName, pwo->wg.nWidth,
                  pwo->winName, pwo->wg.nHeight,
                  pwo->winName, pwo->wg.nPosX,
                  pwo->winName, pwo->wg.nPosY );
}

extern void SaveWindowSettings(FILE* pf)
{
	char szTemp[1024];
	int i;

	fprintf(pf, "set annotation %s\n", woPanel[WINDOW_ANNOTATION].showing ? "yes" : "no");
	fprintf(pf, "set message %s\n", woPanel[WINDOW_MESSAGE].showing ? "yes" : "no");
	fprintf(pf, "set gamelist %s\n", woPanel[WINDOW_GAME].showing ? "yes" : "no");
	fprintf(pf, "set analysis window %s\n", woPanel[WINDOW_ANALYSIS].showing ? "yes" : "no");
	fprintf(pf, "set theorywindow %s\n", woPanel[WINDOW_THEORY].showing ? "yes" : "no");
	fprintf(pf, "set commandwindow %s\n", woPanel[WINDOW_COMMAND].showing ? "yes" : "no");

	fprintf(pf, "set panels %s\n", fDisplayPanels ? "yes" : "no");

	for (i = 0; i < NUM_WINDOWS; i++)
	{
		if (i != WINDOW_ANNOTATION)
		{
			GetGeometryString(szTemp, &woPanel[i]);
			fputs(szTemp, pf);
		}
	}
	/* Save docked slider position */
	fprintf(pf, "set panelwidth %d\n", GetPanelSize());

	/* Save panel dock state (if not docked - default is docked) */
	if (!fDockPanels)
		fputs("set dockpanels off\n", pf);
}

extern void HidePanel(gnubgwindow window)
{
	if (GTK_WIDGET_VISIBLE(woPanel[window].pwWin))
	{
		getWindowGeometry(window);
		woPanel[window].showing = FALSE;
		gtk_widget_hide(woPanel[window].pwWin);
	}
}

extern void getWindowGeometry(gnubgwindow window)
{
	windowobject * pwo = &woPanel[window];
	if (pwo->docked || !pwo->pwWin)
		return;

#if GTK_CHECK_VERSION(2,0,0)

  gtk_window_get_position ( GTK_WINDOW ( pwo->pwWin ),
                            &pwo->wg.nPosX, &pwo->wg.nPosY );

  gtk_window_get_size ( GTK_WINDOW ( pwo->pwWin ),
                        &pwo->wg.nWidth, &pwo->wg.nHeight );

#else

  if (!pwo->pwWin->window)
    return;

  gdk_window_get_position ( pwo->pwWin->window,
                            &pwo->wg.nPosX, &pwo->wg.nPosY );

  gdk_window_get_size ( pwo->pwWin->window,
                        &pwo->wg.nWidth, &pwo->wg.nHeight );

#endif /* ! GTK 2.0 */
}

static void CreatePanel(gnubgwindow window, GtkWidget* pWidget, char* winTitle, char* windowRole)
{
	if (!woPanel[window].docked)
	{
		woPanel[window].pwWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(woPanel[window].pwWin), winTitle);
#if GTK_CHECK_VERSION(2,0,0)
		gtk_window_set_role( GTK_WINDOW( woPanel[window].pwWin ), windowRole );
#if GTK_CHECK_VERSION(2,2,0)
		gtk_window_set_type_hint(GTK_WINDOW(woPanel[window].pwWin), GDK_WINDOW_TYPE_HINT_UTILITY);
#endif
#endif

		setWindowGeometry(window);
		gtk_container_add(GTK_CONTAINER(woPanel[window].pwWin), pWidget);
		gtk_window_add_accel_group(GTK_WINDOW(woPanel[window].pwWin), pagMain);

		gtk_signal_connect(GTK_OBJECT(woPanel[window].pwWin), "delete_event", GTK_SIGNAL_FUNC(woPanel[window].hideFun), NULL);
	}
	else
		woPanel[window].pwWin = pWidget;
}

void DockPanels()
{
	int i;
	int currentSelectedGame = -1;
	
	if (pom)
		currentSelectedGame = gtk_option_menu_get_history( GTK_OPTION_MENU( pom ) );

	if (fDockPanels)
	{
		RefreshGeometries();	/* Get the current window positions */

		gtk_widget_show(gtk_item_factory_get_widget(pif, "/Windows/Commentary"));
		gtk_widget_show(gtk_item_factory_get_widget(pif, "/Windows/Hide panels"));
		gtk_widget_show(gtk_item_factory_get_widget(pif, "/Windows/Restore panels"));

		for (i = 0; i < NUM_WINDOWS; i++)
		{
			if (woPanel[i].undockable && woPanel[i].pwWin)
				gtk_widget_destroy(woPanel[i].pwWin);

			if (woPanel[i].dockable)
				woPanel[i].docked = TRUE;
		}
		CreatePanels();
		if (fDisplayPanels)
			SwapBoardToPanel(TRUE);
	}
	else
	{
		if (fDisplayPanels)
			SwapBoardToPanel(FALSE);

		gtk_widget_hide(gtk_item_factory_get_widget(pif, "/Windows/Commentary"));
		gtk_widget_hide(gtk_item_factory_get_widget(pif, "/Windows/Hide panels"));
		gtk_widget_hide(gtk_item_factory_get_widget(pif, "/Windows/Restore panels"));

		for (i = 0; i < NUM_WINDOWS; i++)
		{
			if (woPanel[i].dockable && woPanel[i].pwWin)
			{
				gtk_widget_destroy(woPanel[i].pwWin);
				woPanel[i].pwWin = NULL;
				woPanel[i].docked = FALSE;
			}
		}
		CreateGameWindow();
		CreateAnalysisWindow();
		CreateMessageWindow();
		CreateTheoryWindow();
		CreateCommandWindow();
	}
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Message"), !fDockPanels || fDisplayPanels);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Game record"), !fDockPanels || fDisplayPanels);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Commentary"), !fDockPanels || fDisplayPanels);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Analysis"), !fDockPanels || fDisplayPanels);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Command"), !fDockPanels || fDisplayPanels);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Theory"), !fDockPanels || fDisplayPanels);

	if (!fDockPanels || fDisplayPanels)
	{
		for (i = 0; i < NUM_WINDOWS; i++)
		{
			if (woPanel[i].undockable && woPanel[i].showing)
				woPanel[i].showFun();
		}
	}
	/* Refresh panel contents */
	GTKRegenerateGames();
	GTKUpdateAnnotations();
	if (currentSelectedGame != -1)
	    GTKSetGame(currentSelectedGame);

	/* Make sure check item is correct */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gtk_item_factory_get_widget(pif, "/Windows/Dock panels")), fDockPanels);

	/* Resize screen */
	SetMainWindowSize();
}


extern void
ShowAllPanels ( gpointer *p, guint n, GtkWidget *pw )
{
	int i;
	/* Only valid if panels docked */
	if (!fDockPanels)
		return;

	fDisplayPanels = 1;

	for (i = 0; i < NUM_WINDOWS; i++)
	{
		if (woPanel[i].dockable && woPanel[i].showing)
			woPanel[i].showFun();
	}

	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Hide panels"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Restore panels"), FALSE);

	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Message"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Game record"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Commentary"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Analysis"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Command"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Theory"), TRUE);

	SwapBoardToPanel(TRUE);
}

extern void
HideAllPanels ( gpointer *p, guint n, GtkWidget *pw )
{
	int i;
	/* Only valid if panels docked */
	if (!fDockPanels)
		return;

	fDisplayPanels = 0;

	for (i = 0; i < NUM_WINDOWS; i++)
	{
		if (woPanel[i].dockable && woPanel[i].showing)
		{
			woPanel[i].hideFun();
			woPanel[i].showing = TRUE;
		}
	}

	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Restore panels"), TRUE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Hide panels"), FALSE);

	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Message"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Game record"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Commentary"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Analysis"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Theory"), FALSE);
	gtk_widget_set_sensitive(gtk_item_factory_get_widget(pif, "/Windows/Command"), FALSE);

	SwapBoardToPanel(FALSE);

	/* Resize screen */
	SetMainWindowSize();
}

extern void ToggleDockPanels( gpointer *p, guint n, GtkWidget *pw )
{
	int newValue = GTK_CHECK_MENU_ITEM( pw )->active;
	if (fDockPanels != newValue)
	{
		fDockPanels = newValue;
		DockPanels();
	}
}

void DisplayWindows()
{
	int i;
	/* Display any other windows now */
	for (i = 0; i < NUM_WINDOWS; i++)
	{
		if (woPanel[i].pwWin && woPanel[i].dockable)
		{
			if (woPanel[i].showing)
				gtk_widget_show_all(woPanel[i].pwWin);
			else
				gtk_widget_hide(woPanel[i].pwWin);
		}
	}
	if (!fDisplayPanels)
		HideAllPanels (0, 0, 0);
}

void DestroyPanel(gnubgwindow window)
{
	if (woPanel[window].pwWin)
	{
		gtk_widget_destroy(woPanel[window].pwWin);
		woPanel[window].pwWin = NULL;
		woPanel[window].showing = FALSE;
	}
}

GtkWidget* GetPanelWidget(gnubgwindow window)
{
    return woPanel[window].pwWin;
}

void SetPanelWidget(gnubgwindow window, GtkWidget* pWin)
{
	woPanel[window].pwWin = pWin;
	woPanel[WINDOW_HINT].showing = pWin ? TRUE : FALSE;
}

extern void
setWindowGeometry(gnubgwindow window)
{
	windowobject* pwo = &woPanel[window];

	if (pwo->docked || !pwo->pwWin || !fGUISetWindowPos)
		return;

#if GTK_CHECK_VERSION(2,0,0)

  gtk_window_set_default_size ( GTK_WINDOW ( pwo->pwWin ),
                      ( pwo->wg.nWidth > 0 ) ? pwo->wg.nWidth : -1,
                      ( pwo->wg.nHeight > 0 ) ? pwo->wg.nHeight : -1 );

  gtk_window_move ( GTK_WINDOW ( pwo->pwWin ),
                    ( pwo->wg.nPosX >= 0 ) ? pwo->wg.nPosX : 0, 
                    ( pwo->wg.nPosY >= 0 ) ? pwo->wg.nPosY : 0 );

#else

  gtk_window_set_default_size( GTK_WINDOW( pwo->pwWin ), 
                               ( pwo->wg.nWidth > 0 ) ? pwo->wg.nWidth : -1,
                               ( pwo->wg.nHeight > 0 ) ? pwo->wg.nHeight : -1 );
  
  gtk_widget_set_uposition ( pwo->pwWin, 
                             ( pwo->wg.nPosX >= 0 ) ? pwo->wg.nPosX : 0, 
                             ( pwo->wg.nPosY >= 0 ) ? pwo->wg.nPosY : 0 );

#endif /* ! GTK 2.0 */

}

void ShowHidePanel(gnubgwindow panel)
{
	if (woPanel[panel].showing)
		woPanel[panel].showFun();
	else
		woPanel[panel].hideFun();
}

int SetMainWindowSize()
{
	if (woPanel[WINDOW_MAIN].wg.nWidth && woPanel[WINDOW_MAIN].wg.nHeight)
	{
		gtk_window_set_default_size(GTK_WINDOW(pwMain),
                                     woPanel[WINDOW_MAIN].wg.nWidth,
                                     woPanel[WINDOW_MAIN].wg.nHeight);
		return 1;
	}
	else
		return 0;
}

void PanelShow(gnubgwindow panel)
{
	woPanel[panel].showFun();
}

void PanelHide(gnubgwindow panel)
{
	woPanel[panel].hideFun();
}

extern void
RefreshGeometries ( void )
{
	int i;
	for (i = 0; i < NUM_WINDOWS; i++)
		getWindowGeometry(i);
}

#endif

extern void CommandSetAnnotation( char *sz ) {

    SetToggle( "annotation", &woPanel[WINDOW_ANNOTATION].showing, sz,
		   _("Move analysis and commentary will be displayed."),
		   _("Move analysis and commentary will not be displayed."));
}

extern void CommandSetMessage( char *sz ) {

	SetToggle("message", &woPanel[WINDOW_MESSAGE].showing, sz,
		_("Show window with messages"),
		_("Do not show window with message."));
}

extern void CommandSetTheoryWindow( char *sz ) {

	SetToggle("theorywindow", &woPanel[WINDOW_THEORY].showing, sz,
		_("Show window with theory"),
		_("Do not show window with theory."));
}

extern void CommandSetCommandWindow( char *sz ) {

	SetToggle("commandwindow", &woPanel[WINDOW_COMMAND].showing, sz,
		_("Show window to enter commands"),
		_("Do not show window to enter commands."));
}

extern void CommandSetGameList( char *sz ) {

    SetToggle( "gamelist", &woPanel[WINDOW_GAME].showing, sz,
		   _("Show game window with moves"),
		   _("Do not show game window with moves.") );
}

extern void CommandSetAnalysisWindows( char *sz ) {

    SetToggle( "analysis window", &woPanel[WINDOW_ANALYSIS].showing, sz,
		   _("Show window with analysis"),
		   _("Do not show window with analysis.") );
}

gnubgwindow pwoSetPanel;

extern void
CommandSetGeometryAnalysis ( char *sz )
{
	pwoSetPanel = WINDOW_ANALYSIS;
	HandleCommand ( sz, acSetGeometryValues );
}

extern void
CommandSetGeometryHint ( char *sz )
{
	pwoSetPanel = WINDOW_HINT;
	HandleCommand ( sz, acSetGeometryValues );
}

extern void
CommandSetGeometryGame ( char *sz )
{
	pwoSetPanel = WINDOW_GAME;
	HandleCommand ( sz, acSetGeometryValues );
}

extern void
CommandSetGeometryMain ( char *sz )
{
	pwoSetPanel = WINDOW_MAIN;
	HandleCommand ( sz, acSetGeometryValues );
}

extern void
CommandSetGeometryMessage ( char *sz )
{
	pwoSetPanel = WINDOW_MESSAGE;
	HandleCommand(sz, acSetGeometryValues);
}

extern void
CommandSetGeometryCommand ( char *sz )
{
	pwoSetPanel = WINDOW_COMMAND;
	HandleCommand(sz, acSetGeometryValues);
}

extern void
CommandSetGeometryTheory ( char *sz )
{
	pwoSetPanel = WINDOW_THEORY;
	HandleCommand(sz, acSetGeometryValues);
}

extern void
CommandSetGeometryWidth ( char *sz ) {

  int n;

  if ( ( n = ParseNumber( &sz ) ) == INT_MIN )
    outputf ( _("Illegal value. "
                "See 'help set geometry %s width'.\n"), woPanel[pwoSetPanel].winName );
  else {

    woPanel[pwoSetPanel].wg.nWidth = n;
    outputf ( _("Width of %s window set to %d.\n"), woPanel[pwoSetPanel].winName, n );

#if USE_GTK
    if ( fX )
		setWindowGeometry(pwoSetPanel);
#endif

  }

}

extern void
CommandSetGeometryHeight ( char *sz ) {

  int n;

  if ( ( n = ParseNumber( &sz ) ) == INT_MIN )
    outputf ( _("Illegal value. "
                "See 'help set geometry %s height'.\n"), woPanel[pwoSetPanel].winName );
  else {

    woPanel[pwoSetPanel].wg.nHeight = n;
    outputf ( _("Height of %s window set to %d.\n"), woPanel[pwoSetPanel].winName, n );

#if USE_GTK
    if ( fX )
		setWindowGeometry(pwoSetPanel);
#endif

  }

}

extern void
CommandSetGeometryPosX ( char *sz ) {

  int n;

  if ( ( n = ParseNumber( &sz ) ) == INT_MIN )
    outputf ( _("Illegal value. "
                "See 'help set geometry %s xpos'.\n"), woPanel[pwoSetPanel].winName );
  else {

    woPanel[pwoSetPanel].wg.nPosX = n;
    outputf ( _("X-position of %s window set to %d.\n"), woPanel[pwoSetPanel].winName, n );

#if USE_GTK
    if ( fX )
		setWindowGeometry(pwoSetPanel);
#endif

  }

}

extern void
CommandSetGeometryPosY ( char *sz ) {

  int n;

  if ( ( n = ParseNumber( &sz ) ) == INT_MIN )
    outputf ( _("Illegal value. "
                "See 'help set geometry %s ypos'.\n"), woPanel[pwoSetPanel].winName );
  else {

    woPanel[pwoSetPanel].wg.nPosY = n;
    outputf ( _("Y-position of %s window set to %d.\n"), woPanel[pwoSetPanel].winName, n );

#if USE_GTK
    if ( fX )
		setWindowGeometry(pwoSetPanel);
#endif

  }

}

extern void CommandSetDisplayPanels( char *sz ) {

  SetToggle ("panels", &fDisplayPanels, sz, 
  _("Game list, Annotation and Message panels/windows will be displayed."),
  _("Game list, Annotation and Message panels/windows will not be displayed.")
	     );

#if USE_GUI && USE_GTK
  if (fX) {
    if (fDisplayPanels)
      ShowAllPanels (0, 0, 0);
    else
      HideAllPanels (0, 0, 0);
  }
#endif
    
}

extern void CommandShowDisplayPanels( char *sz ) {
	if (fDisplayPanels)
	  outputf( _("Game list, Annotation and Message panels/windows "
		        "will be displayed."));
	else
	  outputf( _("Game list, Annotation and Message panels/windows "
		        "will not be displayed."));
}

extern void CommandSetDockPanels( char *sz ) {

    SetToggle( "dockdisplay", &fDockPanels, sz, _("Windows will be docked."),
		_("Windows will be detached.") );
#if USE_GTK
	DockPanels();
#endif
}

static void GetGeometryDisplayString(char* buf, windowobject* pwo)
{
	char dispName[50];
	sprintf(dispName, "%c%s %s", toupper(pwo->winName[0]), &pwo->winName[1], _("window"));

	sprintf(buf, "%-17s : size %dx%d, position (%d,%d)\n",
		dispName, pwo->wg.nWidth, pwo->wg.nHeight, pwo->wg.nPosX, pwo->wg.nPosY);
}

extern void
CommandShowGeometry ( char *sz )
{
	int i;
	char szBuf[1024];
	output(_("Default geometries:\n\n"));

	for (i = 0; i < NUM_WINDOWS; i++)
	{
		GetGeometryDisplayString(szBuf, &woPanel[i]);
		output(szBuf);
	}
}

int DockedPanelsShowing()
{
	return fDockPanels && fDisplayPanels;
}

int ArePanelsShowing()
{
	return fDisplayPanels;
}

int ArePanelsDocked()
{
	return fDockPanels;
}

int IsPanelDocked(gnubgwindow window)
{
	return woPanel[window].docked;
}

int GetPanelWidth(gnubgwindow panel)
{
	return woPanel[panel].wg.nWidth;
}

int IsPanelShowVar(gnubgwindow panel, void *p)
{
	return (p == &woPanel[panel].showing);
}

int PanelShowing(gnubgwindow window)
{
	return woPanel[window].showing && (fDisplayPanels || !woPanel[window].docked);
}
