/******************************************************************************
 * DESCRIPTION: Dinotrace source: display manager, main window
 *
 * This file is part of Dinotrace.
 *
 * Author: Wilson Snyder <wsnyder@wsnyder.org>
 *
 * Code available from: https://www.veripool.org/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the base version has been
 * released to the public with the following provisions:
 *
 *
 * This software is provided 'AS IS'.
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THE INFORMATION
 * (INCLUDING ANY SOFTWARE) PROVIDED, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR ANY PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT. DIGITAL NEITHER WARRANTS NOR REPRESENTS THAT THE USE
 * OF ANY SOURCE, OR ANY DERIVATIVE WORK THEREOF, WILL BE UNINTERRUPTED OR
 * ERROR FREE.  In no event shall DIGITAL be liable for any damages
 * whatsoever, and in particular DIGITAL shall not be liable for special,
 * indirect, consequential, or incidental damages, or damages for lost
 * profits, loss of revenue, or loss of use, arising out of or related to
 * any use of this software or the information contained in it, whether
 * such damages arise in contract, tort, negligence, under statute, in
 * equity, at law or otherwise. This Software is made available solely for
 * use by end users for information and non-commercial or personal use
 * only.  Any reproduction for sale of this Software is expressly
 * prohibited. Any rights not expressly granted herein are reserved.
 *
 ******************************************************************************
 *
 * Changes made over the basic version are covered by the GNU public licence.
 *
 * Dinotrace is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Dinotrace is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dinotrace; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/

#include "dinotrace.h"

#include <assert.h>

#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ArrowB.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrollBarP.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Separator.h>
#include <Xm/MainW.h>

#include "functions.h"

/**********************************************************************/

Widget	dmanage_last;	/* Last DManageChild()'d widget */

/* Application Resources */
/* Colors:
   Black, White, Aquamarine, Blue, BlueViolet, Brown, CadetBlue, Coral,
   CornflowerBlue, Cyan, DarkGreen, DarkOliveGreen, DarkOrchid, DarkSlateBlue,
   DarkSlateGrey, DarkTurquoise, Firebrick, ForestGreen, Gold, Goldenrod, Green,
   GreenYellow, IndianRed, Khaki, LightBlue, LightGrey, LightSteelBlue, LimeGreen,
   Magenta, Maroon, MediumAquamarine, MediumForestGreen, MediumGoldenrod,
   MediumOrchid, MediumSeaGreen, MediumSlateBlue, MediumSpringGreen,
   MediumTurquoise, MidnightBlue, NavyBlue, Orange, OrangeRed, Orchid, PaleGreen,
   Pink, Plum, Red, Salmon, SeaGreen, Sienna, SkyBlue, SlateBlue, SpringGreen,
   SteelBlue, Tan, Thistle, Turquoise, Violet, VioletRed, Wheat, Yellow,
   YellowGreen */

#define Offset(field) XtOffsetOf(Global_t, field)
XtResource resources[] = {
    {"barcolor", "Barcolor", XtRString, sizeof(String), Offset(barcolor_name), XtRImmediate, (XtPointer) NULL},
    {"color1", "Color1", XtRString, sizeof(String), Offset(color_names[1]), XtRImmediate, (XtPointer) "White"},
    {"color2", "Color2", XtRString, sizeof(String), Offset(color_names[2]), XtRImmediate, (XtPointer) "Red"},
    {"color3", "Color3", XtRString, sizeof(String), Offset(color_names[3]), XtRImmediate, (XtPointer) "ForestGreen"},
    {"color4", "Color4", XtRString, sizeof(String), Offset(color_names[4]), XtRImmediate, (XtPointer) "Blue"},
    {"color5", "Color5", XtRString, sizeof(String), Offset(color_names[5]), XtRImmediate, (XtPointer) "Magenta"},
    {"color6", "Color6", XtRString, sizeof(String), Offset(color_names[6]), XtRImmediate, (XtPointer) "Cyan"},
    {"color7", "Color7", XtRString, sizeof(String), Offset(color_names[7]), XtRImmediate, (XtPointer) "Yellow"},
    {"color8", "Color8", XtRString, sizeof(String), Offset(color_names[8]), XtRImmediate, (XtPointer) "Salmon"},
    {"color9", "Color9", XtRString, sizeof(String), Offset(color_names[9]), XtRImmediate, (XtPointer) "NavyBlue"},
    {"signalfont", "SignalFont", XtRString, sizeof(String), Offset(signal_font_name), XtRImmediate, (XtPointer) "-*-Fixed-Medium-R-Normal--*-120-*-*-*-*-*-1"},
    {"timefont",   "TimeFont",   XtRString, sizeof(String), Offset(time_font_name),   XtRImmediate, (XtPointer) "-*-Courier-Medium-R-Normal--*-120-*-*-*-*-*-1"},
    {"valuefont",  "ValueFont",  XtRString, sizeof(String), Offset(value_font_name),  XtRImmediate, (XtPointer) "-*-Fixed-Medium-R-Normal--*-100-*-*-*-*-*-1"}
};
#undef Offset

void debug_event_cb (
    Widget		w,
    Trace_t		*trace,
    XmDrawingAreaCallbackStruct	*cb)
{
    if (trace==NULL) trace = widget_to_trace(w);
    printf ("DEBUG_EVENT_CB %p %p ",w, trace/*(events[cb->event->type]*/);
    printf ("\n");
}

extern void    val_examine_popup_act (Widget w, XButtonPressedEvent* ev, String params, Cardinal* num_params);

/* Any actions must be called with (0) so the callbacks will pass */
/* a null in the trace parameter and thus cause us to search for the right trace */
static XtActionsRec actions[] = {
    {"cancel_all_events",	(XtActionProc)cancel_all_events_cb},
    {"cur_add_current",	(XtActionProc)cur_add_current_cb},
    {"cur_add_next",	(XtActionProc)cur_add_next_cb},
    {"cur_step_back",	(XtActionProc)cur_step_back_cb},
    {"cur_step_back",	(XtActionProc)cur_step_back_cb},
    {"cur_step_fwd",	(XtActionProc)cur_step_fwd_cb},
    {"debug_event",	(XtActionProc)debug_event_cb},
    {"hscroll_pagedec", (XtActionProc)hscroll_pagedec_cb},
    {"hscroll_pageinc", (XtActionProc)hscroll_pageinc_cb},
    {"hscroll_unitdec", (XtActionProc)hscroll_unitdec_cb},
    {"hscroll_unitinc", (XtActionProc)hscroll_unitinc_cb},
    {"sig_high_current",(XtActionProc)sig_highlight_current_cb},
    {"sig_high_next",	(XtActionProc)sig_highlight_next_cb},
    {"sig_search",	(XtActionProc)sig_search_cb},
    {"trace_open",	(XtActionProc)trace_open_cb},
    {"trace_reread_all",(XtActionProc)trace_reread_all_cb},
    {"val_annotate",	(XtActionProc)val_annotate_cb},
    {"val_annotate_do",	(XtActionProc)val_annotate_do_cb},
    {"value_examine_popup", (XtActionProc)val_examine_popup_act},
    {"vscroll_pagedec", (XtActionProc)vscroll_pagedec_cb},
    {"vscroll_pageinc", (XtActionProc)vscroll_pageinc_cb},
    {"vscroll_unitdec", (XtActionProc)vscroll_unitdec_cb},
    {"vscroll_unitinc", (XtActionProc)vscroll_unitinc_cb},
    {"win_begin",	(XtActionProc)win_begin_cb},
    {"win_end",		(XtActionProc)win_end_cb},
    {"win_goto",	(XtActionProc)win_goto_cb}
};

/* Translations for work area only */
char *work_translations = "<Btn2Down> : value_examine_popup()\n";

/* Translations for all main widgets */
/* As noted above: All actions must have (0), and be designed to take a null TRACE */
char *key_translations = "\
:Shift<Key>osfUp:   	vscroll_pagedec(0)\n\
:Shift<Key>osfDown: 	vscroll_pageinc(0)\n\
:<Key>osfUp:		vscroll_unitdec(0)\n\
:<Key>osfDown:		vscroll_unitinc(0)\n\
:<Key>osfPageUp:   	vscroll_pagedec(0)\n\
:<Key>osfPageDown: 	vscroll_pageinc(0)\n\
:Shift<Key>osfLeft:	hscroll_pagedec(0)\n\
:Shift<Key>osfRight:	hscroll_pageinc(0)\n\
:<Key>osfLeft:		hscroll_unitdec(0)\n\
:<Key>osfRight:		hscroll_unitinc(0)\n\
:<Key>osfBeginLine:	win_begin(0)\n\
:<Key>osfEndLine:	win_end(0)\n\
:<Key>Escape:		cancel_all_events(0)\n\
:<Key>a:		val_annotate_do(0)\n\
:<Key>c:		cur_add_current(0)\n\
:<Key>f:		sig_search(0)\n\
:<Key>s:		sig_high_current(0)\n\
:<Key>g:		win_goto(0)\n\
Ctrl<Key>f:		sig_search(0)\n\
Ctrl<Key>o:		trace_open(0)\n\
Ctrl<Key>r:		trace_reread_all(0)\n\
Shift<Key>a:		val_annotate(0)\n\
Shift<Key>c:		cur_add_next(0)\n\
Shift<Key>s:		sig_high_next(0)\n\
:<Key>F2:		val_annotate_do(0)\n\
:<Key><:		cur_step_back(0)\n\
:<Key>>:		cur_step_fwd(0)\n\
";

static int last_set_cursor_num = DC_NORMAL;
int  last_set_cursor ()
{return (last_set_cursor_num);}

void set_cursor (
    int		cursor_num)		/* Entry in xcursors to display */
{
    Trace_t	*trace;			/* Display information */
    if (DTPRINT_ENTRY) printf ("in set_cursor(%d)\n", cursor_num);
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (trace->wind) {
	    XDefineCursor (global->display, trace->wind, global->xcursors[cursor_num]);
	}
    }
    last_set_cursor_num = cursor_num;
}

/* Make the close menu options on all of the menus be active */
static void dm_set_menu_closes ()
{
    Trace_t	*trace;
    int		sensitive;

    sensitive = (global->trace_head && global->trace_head->next_trace)?TRUE:FALSE;

    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	XtSetArg (arglist[0], XmNsensitive, sensitive);
	XtSetValues (trace->menu.menu_close, arglist, 1);
    }
}

/* Split the current trace, return new trace */
Trace_t *trace_create_split_window (
    Trace_t	*trace)
{
    Position x,y,width,height;
    Position new_x,new_y,new_width,new_height;
    Trace_t	*trace_new;

    if (DTPRINT_ENTRY) printf ("In trace_open_split_window - trace=%p\n",trace);

    /* Get orignal sizes */
    XtSetArg (arglist[0], XmNheight,&height);
    XtSetArg (arglist[1], XmNwidth, &width);
    XtSetArg (arglist[2], XmNx, &x);
    XtSetArg (arglist[3], XmNy, &y);
    XtGetValues (trace->toplevel, arglist, 4);
    if (DTPRINT_DISPLAY) printf ("Old size: %dx%d+%d+%d\n", width, height, x, y);

    /* Shrink this window */
    new_x = x; new_y = y; new_width=width; new_height=height;
    if (global->shrink_geometry.xp)	new_x = x + (( width * global->shrink_geometry.x ) / 100);
    if (global->shrink_geometry.yp) 	new_y = y + (( height * global->shrink_geometry.y ) / 100);
    if (global->shrink_geometry.widthp) new_width = ( width * global->shrink_geometry.width ) / 100;
    if (global->shrink_geometry.heightp) new_height = ( height * global->shrink_geometry.height ) / 100;
    if (DTPRINT_DISPLAY) printf ("Shrink size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
    XtSetArg (arglist[0], XmNheight, new_height);
    XtSetArg (arglist[1], XmNwidth, new_width);
    XtSetArg (arglist[2], XmNx, new_x);
    XtSetArg (arglist[3], XmNy, new_y);
    XtSetValues (trace->toplevel, arglist, 4);

    /* Create new window */
    new_x = x; new_y = y; new_width=width; new_height=height;
    if (global->open_geometry.xp)	new_x = x + (( width * global->open_geometry.x ) / 100);
    else	new_x = global->open_geometry.x;
    if (global->open_geometry.yp) 	new_y = y + (( height * global->open_geometry.y ) / 100);
    else	new_y = global->open_geometry.y;
    if (global->open_geometry.widthp)	new_width = ( width * global->open_geometry.width ) / 100;
    else	new_width = global->open_geometry.width;
    if (global->open_geometry.heightp)	new_height = ( height * global->open_geometry.height ) / 100;
    else	new_height = global->open_geometry.height;
    if (DTPRINT_DISPLAY) printf ("New size: %dx%d+%d+%d\n", new_width, new_height, new_x, new_y);
    trace_new = create_trace (new_width, new_height, new_x, new_y);

    return (trace_new);
}

void trace_open_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    Trace_t	*trace_new;

    trace_new = trace_create_split_window (trace);

    /* Ask for a file in the new window */
    XSync (global->display,0);
    trace_read_cb (NULL, trace_new);
}

void trace_close (
    Trace_t	*trace)
{
    Trace_t	*trace_ptr;

    if (DTPRINT_ENTRY) printf ("In trace_close - trace=%p\n",trace);

    assert (trace!=global->deleted_trace_head);

    /* clear the screen */
    XClearWindow (global->display, trace->wind);
    /* Nail the child */
    /* XtUnmanageChild (trace->toplevel); */
    /* free memory associated with the data */
    free_data (trace);
    /* destroy all the widgets created for the screen */
    XtDestroyWidget (trace->toplevel);

    /* relink pointers to ignore this trace */
    if (trace == global->trace_head)
	global->trace_head = trace->next_trace;
    for (trace_ptr = global->deleted_trace_head; trace_ptr; trace_ptr = trace_ptr->next_trace) {
	if (trace_ptr->next_trace == trace) trace_ptr->next_trace = trace->next_trace;
    }
    if (trace == global->anno_last_trace) global->anno_last_trace = NULL;

    /* free the display structure */
    DFree (trace);

    /* Update menus */
    dm_set_menu_closes ();
}

void trace_close_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    trace_close (trace);
}

void trace_clear_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    Trace_t	*trace_ptr;
    Trace_t	*trace_next;

    if (DTPRINT_ENTRY) printf ("In clear_trace - trace=%p\n",trace);

    /* nail all traces except for this window's */
    for (trace_ptr = global->trace_head; trace_ptr; ) {
	trace_next = trace_ptr->next_trace;
	if (trace_ptr != trace) {
	    trace_close (trace_ptr);
	}
	trace_ptr = trace_next;
    }

    /* clear the screen */
    XClearWindow (global->display, trace->wind);

    /* free memory associated with the data */
    free_data (trace);

    /* change the name on title bar back to the trace */
    change_title (trace);
}

void trace_exit_cb (
    Widget		w)
{
    Trace_t *trace = widget_to_trace(w);
    Trace_t		*trace_next;

    if (DTPRINT_ENTRY) printf ("In trace_exit_cb - trace=%p\n",trace);

    for (trace = global->trace_head; trace; ) {
	trace_next = trace->next_trace;
	trace_close (trace);
	trace = trace_next;
    }

    DFree (global);

    /* all done */
    exit (0);
}

void init_globals (void)
{
    int i;
    char *pchar;
    int		cfg_num;
    char	filename[MAXFNAMELEN];

    if (DTPRINT_ENTRY) printf ("in init_globals\n");

    global = DNewCalloc (Global_t);
    global->trace_head = NULL;
    global->directory[0] = '\0';
    global->res = RES_SCALE/ (float)(250);
    global->res_default = TRUE;

    global->save_duplicates = TRUE;
    global->preserved_trace = NULL;
    global->selected_sig = NULL;
    global->cursor_head = NULL;
    global->signalstate_head = NULL;
    global->xstart = 200;
    global->time = 0;
    global->time_precision = TIMEREP_NS;
    global->tempest_time_mult = 2;
    global->click_to_edge = 1;
    global->start_geometry.width = 800;
    global->start_geometry.height = 600;
    global->start_geometry.x = 100;
    global->start_geometry.y = 100;
    global->open_geometry.width = 100;
    global->open_geometry.height = 50;
    global->open_geometry.x = 0;
    global->open_geometry.y = 50;
    global->open_geometry.xp = global->open_geometry.yp = TRUE;
    global->open_geometry.heightp = global->open_geometry.widthp = TRUE;
    global->shrink_geometry.width = 100;
    global->shrink_geometry.height = 50;
    global->shrink_geometry.x = 0;
    global->shrink_geometry.y = 0;
    global->shrink_geometry.xp = global->shrink_geometry.yp = TRUE;
    global->shrink_geometry.heightp = global->shrink_geometry.widthp = TRUE;

    global->goto_color = -1;

    global->cuswr_item[CUSWRITEM_COMMENT] = FALSE;
    global->cuswr_item[CUSWRITEM_PERSONAL] = FALSE;
    global->cuswr_item[CUSWRITEM_VALSEARCH] = TRUE;
    global->cuswr_item[CUSWRITEM_CURSORS] = TRUE;
    global->cuswr_item[CUSWRITEM_GRIDS] = TRUE;
    global->cuswr_item[CUSWRITEM_FORMAT] = TRUE;
    global->cuswr_item[CUSWRITEM_SIGHIGHLIGHT] = TRUE;
    global->cuswr_item[CUSWRITEM_SIGORDER] = TRUE;

    /* Annotate stuff */
    global->anno_traces = TRACESEL_ALL;
    global->anno_last_trace = NULL;
    for (i=0; i <= MAX_SRCH; i++) {
	global->anno_ena_signal[i] = (i!=0);
	global->anno_ena_cursor[i] = (i!=0);
	global->anno_ena_cursor_dotted[i] = 0;
    }
#ifdef VMS
    strcpy (global->anno_filename, "sys$login:dinotrace.danno");
#else
    global->anno_filename[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (global->anno_filename, pchar);
    if (global->anno_filename[0]) strcat (global->anno_filename, "/");
    strcat (global->anno_filename, "dinotrace.danno");
#endif

#ifdef VMS
    global->cuswr_filename = "sys$login:written.dino";
#else
    filename[0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) strcpy (filename, pchar);
    if (filename[0]) strcat (filename, "/");
    strcat (filename, "written.dino");
    global->cuswr_filename = strdup (filename);
#endif

    val_radix_init ();

    /* Search stuff */
    for (i=0; i<MAX_SRCH; i++) {
	/* Colors */
	global->color_names[i] = NULL;

	/* Value */
	memset ((char *)&global->val_srch[i], 0, sizeof (ValSearch_t));
	strcpy (global->val_srch[i].signal, "*");
	global->val_srch[i].radix = global->radixs[0];

	/* Signal */
	memset ((char *)&global->sig_srch[i], 0, sizeof (SigSearch_t));
    }

    /* Config stuff */
    for (cfg_num=0; cfg_num<MAXCFGFILES; cfg_num++) {
	global->config_enable[cfg_num] = TRUE;
	global->config_filename[cfg_num][0] = '\0';
    }

#ifdef VMS
    strcpy (global->config_filename[0], "DINODISK:DINOTRACE.DINO");
    strcpy (global->config_filename[1], "DINOCONFIG:");
    strcpy (global->config_filename[2], "SYS$LOGIN:DINOTRACE.DINO");
#else
    dinodisk_directory (global->config_filename[0]);
    if (global->config_filename[0][0]) {
	strcat (global->config_filename[0], "/dinotrace.dino");
    }

    global->config_filename[1][0] = '\0';
    if (NULL != (pchar = getenv ("DINOCONFIG"))) {
	strcpy (global->config_filename[1], pchar);
    }

    global->config_filename[2][0] = '\0';
    if (NULL != (pchar = getenv ("HOME"))) {
	strcpy (global->config_filename[2], pchar);
	strcat (global->config_filename[2], "/dinotrace.dino");
    }
#endif
}

void create_globals (
    int		argc,
    char	**argv,
    Boolean_t	sync)
{
    int		argc_copy;
    char	**argv_copy;
    char	display_name[512];

    /* alloc variables */
    global->argc = argc;
    global->argv = argv;

    XtToolkitInitialize ();

    global->appcontext = XtCreateApplicationContext ();

    /* Save parameters and open display */
    argc_copy = global->argc;
    argv_copy = (char **)XtMalloc (global->argc * sizeof (char *));
    memcpy (argv_copy, global->argv, global->argc * sizeof (char *));

    global->display = XtOpenDisplay (global->appcontext, NULL, NULL, "Dinotrace",
				    NULL, 0, &argc_copy, argv_copy);

    DFree(argv_copy);

    if (global->display==NULL) {
	display_name[0] = '\0';
	printf ("Can't open display '%s'\n", XDisplayName (display_name));
	exit (0);
    }

    XSynchronize (global->display, sync);
    if (DTPRINT_ENTRY) printf ("in create_globals, syncronization is %d\n", sync);

    /*** create dino pixmaps from data ***/
    icon_dinos ();

    /* Define cursors */
    global->xcursors[0] = XCreateFontCursor (global->display, XC_top_left_arrow);
    global->xcursors[1] = XCreateFontCursor (global->display, XC_watch);
    global->xcursors[2] = XCreateFontCursor (global->display, XC_sb_left_arrow);
    global->xcursors[3] = XCreateFontCursor (global->display, XC_sb_right_arrow);
    global->xcursors[4] = XCreateFontCursor (global->display, XC_hand1);
    global->xcursors[5] = XCreateFontCursor (global->display, XC_center_ptr);
    global->xcursors[6] = XCreateFontCursor (global->display, XC_sb_h_double_arrow);
    global->xcursors[7] = XCreateFontCursor (global->display, XC_X_cursor);
    global->xcursors[8] = XCreateFontCursor (global->display, XC_left_side);
    global->xcursors[9] = XCreateFontCursor (global->display, XC_right_side);
    global->xcursors[10] = XCreateFontCursor (global->display, XC_spraycan);
    global->xcursors[11] = XCreateFontCursor (global->display, XC_question_arrow);
    global->xcursors[12] = XCreateFontCursor (global->display, XC_cross);
    global->xcursors[13] = XCreateFontCursor (global->display, XC_dotbox);

    config_global_defaults ();
}

Trace_t *malloc_trace (void)
    /* Allocate a trace structure and return it */
    /* This should NOT do any windowing initialization */
{
    Trace_t	*trace;

    /*** alloc space for trace to display state block ***/
    trace = DNewCalloc (Trace_t);
    trace->next_trace = global->trace_head;
    global->trace_head = trace;
    if (global->deleted_trace_head) global->deleted_trace_head->next_trace = global->trace_head;

    /* Initialize Various Parameters */
    trace->ystart = 30;

    return (trace);
}


static void dm_menu_title (
    Trace_t *trace,
    char *title,
    char key		/* Or '\0' for none */
    )
    /*** create a pulldownmenu on the top bar ***/
{
    int arg=0;

    if (++trace->menu.pde >= MENU_PDE_MAX) printf ("Out of menu space\n");
    trace->menu.pdmenu[trace->menu.pde] = XmCreatePulldownMenu (trace->menu.menu,"",NULL,0);
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );		arg++;
    XtSetArg (arglist[arg], XmNsubMenuId, trace->menu.pdmenu[trace->menu.pde] );	arg++;
    if (key !='\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    trace->menu.pdmenubutton[trace->menu.pde] = XmCreateCascadeButton (trace->menu.menu, "mt", arglist, arg);
    DManageChild (trace->menu.pdmenubutton[trace->menu.pde], trace, MC_NOKEYS);
}

static void dm_menu_entry (
    Trace_t *trace,
    char *title,
    char key,		/* Or '\0' for none */
    char *accel,	/* Accelerator, or NULL */
    char *accel_string,	/* Accelerator string, or NULL */
    MenuCallback_t callback
    )
    /*** create a pulldownmenu entry under the top bar ***/
{
    int arg=0;

    if (++trace->menu.pdm >= MENU_PDM_MAX) printf ("Out of pdm menu space\n");
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    if (accel != NULL) { XtSetArg (arglist[arg], XmNacceleratorText, XmStringCreateSimple (accel_string) );	arg++; }
    if (accel_string != NULL) { XtSetArg (arglist[arg], XmNaccelerator, accel );	arg++; }
     trace->menu.pdentrybutton[trace->menu.pdm] = XmCreatePushButtonGadget (trace->menu.pdmenu[trace->menu.pde], "me", arglist, arg);
    DAddCallback (trace->menu.pdentrybutton[trace->menu.pdm], XmNactivateCallback, callback, trace);
    DManageChild (trace->menu.pdentrybutton[trace->menu.pdm], trace, MC_NOKEYS);
}

static void dm_menu_separator (
    Trace_t *trace
    )
    /*** create a separator menu entry under the top bar ***/
{
    if (++trace->menu.pdmsep >= MENU_PDMSEP_MAX) printf ("Out of pdmsep menu space\n");
    trace->menu.pdsep[trace->menu.pdmsep] = XmCreateSeparator (trace->menu.pdmenu[trace->menu.pde], "msep", arglist, 0);
    DManageChild (trace->menu.pdsep[trace->menu.pdmsep], trace, MC_NOKEYS);
}

static void dm_menu_subtitle (Trace_t *trace,
		       char *title,
		       char key			/* Or '\0' for none */
		       )
    /*** create a pulldownmenu entry under the top bar ***/
{
    int arg=0;

    if (++trace->menu.pdm >= MENU_PDM_MAX) printf ("Out of pdm menu space\n");
    trace->menu.pdentry[trace->menu.pdm] = XmCreatePulldownMenu (trace->menu.pdmenu[trace->menu.pde],"",NULL,0);
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    XtSetArg (arglist[arg], XmNsubMenuId, trace->menu.pdentry[trace->menu.pdm] );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    trace->menu.pdentrybutton[trace->menu.pdm] = XmCreateCascadeButton (trace->menu.pdmenu[trace->menu.pde], "mst", arglist, arg);
    DManageChild (trace->menu.pdentrybutton[trace->menu.pdm], trace, MC_NOKEYS);
}

static void dm_menu_subentry (
    Trace_t *trace,
    char *title,
    char key,		/* Or '\0' for none */
    char *accel,	/* Accelerator, or NULL */
    char *accel_string,	/* Accelerator string, or NULL */
    MenuCallback_t callback
    )
    /*** create a pulldownmenu entry under a subtitle ***/
{
    int arg=0;

    if (++trace->menu.pds >= MENU_PDS_MAX) printf ("Out of pds menu space\n");
    XtSetArg (arglist[arg], XmNlabelString, XmStringCreateSimple (title) );	arg++;
    if (key != '\0') { XtSetArg (arglist[arg], XmNmnemonic, key );	arg++; }
    if (accel_string != NULL) { XtSetArg (arglist[arg], XmNacceleratorText, XmStringCreateSimple (accel_string) );	arg++; }
    if (accel != NULL) { XtSetArg (arglist[arg], XmNaccelerator, accel );	arg++; }
    trace->menu.pdsubbutton[trace->menu.pds] = XmCreatePushButtonGadget (trace->menu.pdentry[trace->menu.pdm], "mse", arglist, arg);
    DAddCallback (trace->menu.pdsubbutton[trace->menu.pds], XmNactivateCallback, callback, trace);
    DManageChild (trace->menu.pdsubbutton[trace->menu.pds], trace, MC_NOKEYS);
}

static void dm_menu_subentry_colors (
    Trace_t *trace,
    char *cur_accel,	/* Accelerator, or NULL */
    char *cur_accel_string,	/* Accelerator string, or NULL */
    char *next_accel,	/* Accelerator, or NULL */
    char *next_accel_string,	/* Accelerator string, or NULL */
    MenuCallback_t callback
    )
    /*** create a pulldownmenu entry under a subtitle (uses special colors) ***/
{
    int color;

    for (color=0; color <= MAX_SRCH; color++) {
	++trace->menu.pds;
	XtSetArg (arglist[0], XmNbackground, trace->xcolornums[color]);
	XtSetArg (arglist[1], XmNmarginBottom, 8);
	/*XtSetArg (arglist[2], XmNmarginRight, 50);*/
	trace->menu.pdsubbutton[trace->menu.pds] = XmCreatePushButton (trace->menu.pdentry[trace->menu.pdm], "", arglist, 2);
	DAddCallback (trace->menu.pdsubbutton[trace->menu.pds], XmNactivateCallback, callback, trace);
	DManageChild (trace->menu.pdsubbutton[trace->menu.pds], trace, MC_NOKEYS);
    }
    dm_menu_subentry (trace, (char*)(cur_accel ? "Curr":"Current"), 'C', cur_accel, cur_accel_string, callback);
    dm_menu_subentry (trace, "Next", 'N', next_accel, next_accel_string, callback);
}


static void dm_menu_subentry_radixs (
    Trace_t *trace,
    MenuCallback_t callback
    )
{
    int radixnum;
    Radix_t *radix_ptr;
    char name[100];

    for (radixnum=0; radixnum<RADIX_MAX; radixnum++) {
	radix_ptr=global->radixs[radixnum];
	if (radix_ptr) {
	    strcpy (name, radix_ptr->name);
	    if (radix_ptr->prefix[0]) {
		strcat (name, " (");
		strcat (name, radix_ptr->prefix);
		strcat (name, ")");
	    }
	    dm_menu_subentry (trace, name, '\0', NULL, NULL, callback);
	}
    }
}


/* Try to allocate the given font.  If it doesn't exist, use a default font, rather
*  than printing a error.  Roughly equivelent to:
*  return (XLoadQueryFont (global->display, global->signal_font_name))
*  but that crashes if a font isn't found.
*/
static XFontStruct *grab_font (
    Trace_t	*trace,
    char	*font_name)		/* Name of the font */
{
    int	num;
    char **list;
    const char* default_font_name = "-*-*-medium-r-*-*-*-*-*-*-*-*-*-*";

    list = XListFonts (global->display, font_name, 1, &num);
    XFreeFontNames(list);

    XFontStruct* fontp;

    if (num > 0) {
	fontp = XLoadQueryFont (global->display, font_name);
    }
    else {
	fontp = XLoadQueryFont (global->display, default_font_name);
    }

    if (!fontp) {
	fprintf(stderr,"%%Error: Can't allocate any fonts, even '%s'\n", default_font_name);
	exit(10);
    }

    return fontp;
}

/* Manage a dinotrace widget */
/* Insure that the user data is setup correctly to point to a trace */
/* Optionally install standard keymap defines */
void DManageChild (Widget w, Trace_t *trace, MCKeys_t keys)
{
    XtSetArg (arglist[0], XmNuserData, trace);
    XtSetValues (w, arglist, 1);
    if (keys) XtOverrideTranslations (w, XtParseTranslationTable (key_translations));
    XtManageChild (w);
    dmanage_last = w;
}

/* Create a trace display and link it into the global information */
Trace_t *create_trace (
    int		xs,
    int		ys,
    int		xp,
    int		yp)
{
    /*    int		x1,x2;
	  unsigned int junk; */
    int		i;
    Trace_t	*trace;
    int		argc_copy;
    char	**argv_copy;
    XColor	xcolor,xcolor2;
    Colormap	cmap;

    /*** alloc space for trace to display state block ***/
    trace = malloc_trace ();

    /*** create trace->toplevel widget ***/
    argc_copy = global->argc;
    argv_copy = (char **)XtMalloc (global->argc * sizeof (char *));
    memcpy (argv_copy, global->argv, global->argc * sizeof (char *));
    XtSetArg (arglist[0], XtNargc, argc_copy);
    XtSetArg (arglist[1], XtNargv, argv_copy);
    XtSetArg (arglist[2], XmNallowShellResize, FALSE);
    trace->toplevel = XtAppCreateShell
	(NULL, "Dinotrace", applicationShellWidgetClass,
	 global->display, arglist, 3);
    /* printf ("&& && trace %d, top=%d\n", trace, trace->toplevel); */

    DFree(argv_copy);

    change_title (trace);

    XtGetApplicationResources (trace->toplevel, (XtPointer) global, resources,
			      XtNumber (resources), (Arg *) NULL, 0);

    XtAppAddActions (global->appcontext, actions, XtNumber(actions));

    /*** add pixmap and position trace->toplevel widget ***/
    XtSetArg (arglist[0], XtNallowShellResize, FALSE);
    XtSetArg (arglist[1], XtNiconPixmap, global->bdpm);
    XtSetArg (arglist[2], "iconifyPixmap", global->dpm);
    XtSetArg (arglist[3], XmNx, xp);
    XtSetArg (arglist[4], XmNy, yp);
    XtSetArg (arglist[5], XmNheight, ys);
    XtSetArg (arglist[6], XmNwidth, xs);
    XtSetValues (trace->toplevel, arglist, 7);

    /****************************************
     * create the main window
     ****************************************/
    XtSetArg (arglist[0], XmNx, 20);
    XtSetArg (arglist[1], XmNy, 350);
    trace->main = XmCreateMainWindow (trace->toplevel,"main", arglist, 2);
    /*DAddCallback (trace->main, XmNfocusCallback, win_focus_cb, trace);*/

    /* Find the colors to use */
    XtSetArg (arglist[0], XmNcolormap, &cmap);
    XtSetArg (arglist[1], XmNforeground, &(trace->xcolornums[0]));
    XtSetArg (arglist[2], XmNbackground, &(trace->barcolornum));
    XtGetValues (trace->main, arglist, 3);

    for (i=1; i<=9; i++) {
	if (DTPRINT_DISPLAY) printf ("%d = '%s'\n", i, global->color_names[i] ? global->color_names[i]:"NULL");
	if ( (global->color_names[i] != NULL)
	    && (XAllocNamedColor (global->display, cmap, global->color_names[i], &xcolor, &xcolor2)))
	    trace->xcolornums[i] = xcolor.pixel;
	else trace->xcolornums[i] = XWhitePixel (global->display, 0);
    }

    if (global->barcolor_name == NULL || global->barcolor_name[0]=='\0') {
	/* Default is 7% green above background */
	xcolor.pixel = trace->barcolornum;
	XQueryColor (global->display, cmap, &xcolor);
	if (xcolor.green < 58590)
	    xcolor.green  += (unsigned short)(xcolor.green * 0.07);
	else xcolor.green -= (unsigned short)(xcolor.green * 0.07);
	if (DTPRINT_DISPLAY) printf (" = %x, %x, %x \n", xcolor.red, xcolor.green, xcolor.blue);
	if (XAllocColor (global->display, cmap, &xcolor))
	    trace->barcolornum = xcolor.pixel;
    }
    else {
	if (XAllocNamedColor (global->display, cmap, global->barcolor_name, &xcolor, &xcolor2))
	    trace->barcolornum = xcolor.pixel;
    }

    /****************************************
     * create the menu bar
     ****************************************/

    /*** begin with -1 pde, since new menu will increment ***/
    trace->menu.pdm = trace->menu.pde = trace->menu.pds = trace->menu.pdmsep = -1;
    trace->menu.menu = XmCreateMenuBar (trace->main,"menu", NULL, 0);
    DManageChild (trace->menu.menu, trace, MC_NOKEYS);

    dm_menu_title (trace, "Trace", 'T');
    dm_menu_entry (trace, 	"Read...",	'R',	NULL, NULL,	(MenuCallback_t)trace_read_cb);
    dm_menu_entry (trace, 	"ReRead",	'e',	NULL, NULL,	(MenuCallback_t)trace_reread_cb);
    dm_menu_entry (trace, 	"ReRead All",	'A', "!Ctrl<Key>R:", "C-r", (MenuCallback_t)trace_reread_all_cb);
    dm_menu_entry (trace, 	"Open...",	'O', "!Ctrl<Key>O:", "C-o", (MenuCallback_t)trace_open_cb);
    dm_menu_entry (trace, 	"Close",	'C',	NULL, NULL,	(MenuCallback_t)trace_close_cb);
    trace->menu.menu_close = trace->menu.pdentrybutton[trace->menu.pdm];
    dm_menu_entry (trace, 	"Clear",	'l',	NULL, NULL,	(MenuCallback_t)trace_clear_cb);
    dm_menu_separator (trace);
    dm_menu_entry (trace, 	"Print...",	'i',	NULL, NULL,	(MenuCallback_t)print_dialog_cb);
    dm_menu_entry (trace, 	"Print",	'P',	NULL, NULL,	(MenuCallback_t)print_direct_cb);
    dm_menu_separator (trace);
    dm_menu_entry (trace, 	"Exit", 	'x',	NULL, NULL,	(MenuCallback_t)trace_exit_cb);

    dm_menu_title (trace, "Customize", 'u');
    dm_menu_entry (trace, 	"Change...",	'C',	NULL, NULL,	(MenuCallback_t)cus_dialog_cb);
    dm_menu_entry (trace, 	"Grid...",	'G',	NULL, NULL,	(MenuCallback_t)grid_customize_cb);
    dm_menu_entry (trace, 	"Read...",	'R',	NULL, NULL,	(MenuCallback_t)cus_read_cb);
    dm_menu_entry (trace, 	"ReRead",	'e',	NULL, NULL,	(MenuCallback_t)cus_reread_cb);
    dm_menu_entry (trace, 	"Save As...",	'S',	NULL, NULL,	(MenuCallback_t)cus_write_cb);
    dm_menu_entry (trace, 	"Restore",	'R',	NULL, NULL,	(MenuCallback_t)cus_restore_cb);

    dm_menu_title (trace, "Cursor", 'C');
    dm_menu_subtitle (trace, 	"Add",		'A');
    trace->menu.cur_add_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "!<Key>C:", "c", "!Shift<Key>C:", "S-c", (MenuCallback_t)cur_add_cb);
    if (global->simview_info_ptr) {
	dm_menu_subtitle (trace, "Open View",	'V');
	trace->menu.cur_add_simview_pds = trace->menu.pds+1;
	dm_menu_subentry_colors (trace, NULL, NULL, NULL, NULL, (MenuCallback_t)cur_add_simview_cb);
    }
    dm_menu_subtitle (trace,	 "Highlight",	'H');
    trace->menu.cur_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, 		NULL, NULL, NULL, NULL,	(MenuCallback_t)cur_highlight_cb);
    dm_menu_entry (trace, 	"Move",		'M',	NULL, NULL,	(MenuCallback_t)cur_mov_cb);
    dm_menu_entry (trace, 	"Delete",	'D',	NULL, NULL,	(MenuCallback_t)cur_del_cb);
    dm_menu_entry (trace, 	"Note",		'N',	NULL, NULL,	(MenuCallback_t)cur_note_cb);
    dm_menu_entry (trace, 	"Step Forward",	'F',	"!Shift<Key>greater:", ">",	(MenuCallback_t)cur_step_fwd_cb);
    dm_menu_entry (trace, 	"Step Backward",'B',	"!Shift<Key>less:", "<",	(MenuCallback_t)cur_step_back_cb);
    dm_menu_entry (trace, 	"Clear", 	'C',	NULL, NULL,	(MenuCallback_t)cur_clr_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	"!<Key>Escape:", "esc",	(MenuCallback_t)cancel_all_events_cb);

    dm_menu_title (trace, "Signal", 'S');
    dm_menu_entry (trace, 	"Add...",	'A',	NULL, NULL,	(MenuCallback_t)sig_add_cb);
    dm_menu_subtitle (trace, 	"Highlight",	'H');
    trace->menu.sig_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "!<Key>s:", "s", "!Shift<Key>s:", "S-s",(MenuCallback_t)sig_highlight_cb);
    dm_menu_entry (trace, 	"Move",		'M',	NULL, NULL,	(MenuCallback_t)sig_mov_cb);
    dm_menu_entry (trace, 	"Copy",		'C',	NULL, NULL,	(MenuCallback_t)sig_copy_cb);
    dm_menu_entry (trace, 	"Delete",	'D',	NULL, NULL,	(MenuCallback_t)sig_del_cb);
    dm_menu_entry (trace, 	"Note",		'N',	NULL, NULL,	(MenuCallback_t)sig_note_cb);
    dm_menu_subtitle (trace, 	"Radix",	'R');
    trace->menu.sig_radix_pds = trace->menu.pds+1;
    dm_menu_subentry_radixs (trace, (MenuCallback_t)sig_radix_cb);
    dm_menu_subtitle (trace, 	"Waveform",	'W');
    trace->menu.sig_waveform_pds = trace->menu.pds+1;
    dm_menu_subentry (trace, 		"Digital", 'D', NULL, NULL,	(MenuCallback_t)sig_waveform_cb);
    dm_menu_subentry (trace, 		"Analog", 'A', NULL, NULL,	(MenuCallback_t)sig_waveform_cb);
    dm_menu_subentry (trace, 		"Analog Signed", 'S', NULL, NULL,	(MenuCallback_t)sig_waveform_cb);
    dm_menu_entry (trace, 	"Search...",	'S', "<Key>F:", "f/C-f", (MenuCallback_t)sig_search_cb);
    dm_menu_entry (trace, 	"Select...",	'e',	NULL, NULL,	(MenuCallback_t)sig_select_cb);
    dm_menu_entry (trace, 	"Clear Highlight",'i',	NULL, NULL,	(MenuCallback_t)sig_highlight_clear_cb);
    dm_menu_entry (trace, 	"Keep Highlighted",'K',	NULL, NULL,	(MenuCallback_t)sig_highlight_keep_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	"!<Key>Escape:", "esc",	(MenuCallback_t)cancel_all_events_cb);

    dm_menu_title (trace, "Value", 'V');
    dm_menu_entry (trace,	"Annotate", 	'A',	"!<Key>A:", "a", 	(MenuCallback_t)val_annotate_do_cb);
    dm_menu_entry (trace, 	"Annotate...",	'n', 	"!Shift<Key>A:", "S-a", (MenuCallback_t)val_annotate_cb);
    dm_menu_subtitle (trace, 	"Highlight",	'H');
    trace->menu.val_highlight_pds = trace->menu.pds+1;
    dm_menu_subentry_colors (trace, "!<Key>v:", "v", "!Shift<Key>v:", "S-v",	(MenuCallback_t)val_highlight_cb);
    dm_menu_entry (trace, 	"Examine",	'E',	NULL, NULL,		(MenuCallback_t)val_examine_cb);
    XtSetArg (arglist[0], XmNacceleratorText, XmStringCreateSimple ("MB2") );
    XtSetValues (trace->menu.pdentrybutton[trace->menu.pdm], arglist, 1);
    dm_menu_entry (trace, 	"Search...",	'S',	NULL, NULL,		(MenuCallback_t)val_search_cb);
    dm_menu_entry (trace, 	"Cancel", 	'l',	"!<Key>Escape:", "esc",	(MenuCallback_t)cancel_all_events_cb);


    if (DTDEBUG) {
	dm_menu_title (trace, "Debug", 'D');
	dm_menu_entry	(trace, "Toggle Print",		'P', NULL, NULL,	(MenuCallback_t)debug_toggle_print_cb);
	dm_menu_entry	(trace, "Integrity Check",	'I', NULL, NULL,	(MenuCallback_t)debug_integrity_check_cb);
	dm_menu_entry	(trace, "Statistics",		'S', NULL, NULL,	(MenuCallback_t)debug_statistics_cb);
	dm_menu_entry	(trace, "Print Signal Info (Screen Only)", 'I', NULL, NULL,	(MenuCallback_t)debug_print_screen_traces_cb);
	dm_menu_entry	(trace, "Increase DebugTemp",	'+', NULL, NULL,	(MenuCallback_t)debug_increase_debugtemp_cb);
	dm_menu_entry	(trace, "Decrease DebugTemp",	'-', NULL, NULL,	(MenuCallback_t)debug_decrease_debugtemp_cb);
    }

    dm_menu_title (trace, "Help", 'H');
    dm_menu_entry (trace, 	"On Version",	'V',	NULL, NULL,	(MenuCallback_t)help_cb);
    dm_menu_entry (trace, 	"On Trace",	'T',	NULL, NULL,	(MenuCallback_t)help_trace_cb);
    dm_menu_entry (trace, 	"On Documentation",	'D',	NULL, NULL,	(MenuCallback_t)help_doc_cb);
    XtSetArg (arglist[0], XmNmenuHelpWidget, trace->menu.pdmenubutton[trace->menu.pde]);
    XtSetValues (trace->menu.menu, arglist, 1);

    /****************************************
     * create the command widget
     ****************************************/
    XtSetArg (arglist[0], XmNresizePolicy, XmRESIZE_NONE);
    trace->command.form = XmCreateForm (trace->main, "form", arglist, 1);

    /* For every manage, also call override translations to force our keys. */
    /* Accelerators should have the same effect, but a bug in lessTif prevents */
    /* them from working */

    /* create the vertical scroll bar */
    XtSetArg (arglist[0], XmNorientation, XmVERTICAL );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNbottomOffset, 40);
    XtSetArg (arglist[5], XmNwidth, 18);
    trace->vscroll = XmCreateScrollBar ( trace->command.form, "vscroll", arglist, 6);
    DAddCallback (trace->vscroll, XmNincrementCallback, vscroll_unitinc_cb, trace);
    DAddCallback (trace->vscroll, XmNdecrementCallback, vscroll_unitdec_cb, trace);
    DAddCallback (trace->vscroll, XmNdragCallback, vscroll_drag_cb, trace);
    DAddCallback (trace->vscroll, XmNpageIncrementCallback, vscroll_pageinc_cb, trace);
    DAddCallback (trace->vscroll, XmNpageDecrementCallback, vscroll_pagedec_cb, trace);
    DManageChild (trace->vscroll, trace, MC_GLOBALKEYS);

    /*** create begin button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Begin") );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNleftOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.begin_but = XmCreatePushButton (trace->command.form, "begin", arglist, 4);
    DAddCallback (trace->command.begin_but, XmNactivateCallback, win_begin_cb, trace);
    DManageChild (trace->command.begin_but, trace, MC_GLOBALKEYS);

    /*** create goto button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Goto") );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNleftOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNleftWidget, trace->command.begin_but);
    trace->command.goto_but = XmCreatePushButton (trace->command.form, "goto", arglist, 5);
    DAddCallback (trace->command.goto_but, XmNactivateCallback, win_goto_cb, trace);
    DManageChild (trace->command.goto_but, trace, MC_GLOBALKEYS);

    /*** create end button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("End") );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNrightOffset, 3);
    XtSetArg (arglist[3], XmNrightWidget, trace->vscroll);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.end_but = XmCreatePushButton (trace->command.form, "end", arglist, 5);
    DAddCallback (trace->command.end_but, XmNactivateCallback, win_end_cb, trace);
    DManageChild (trace->command.end_but, trace, MC_GLOBALKEYS);

    /*** create refresh button in command region ***/
    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Refresh") );
    XtSetArg (arglist[1], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNrightOffset, 7);
    XtSetArg (arglist[3], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNrightWidget, trace->command.end_but);
    trace->command.refresh_but = XmCreatePushButton (trace->command.form, "refresh", arglist, 5);
    DAddCallback (trace->command.refresh_but, XmNactivateCallback, win_refresh_cb, trace);
    DManageChild (trace->command.refresh_but, trace, MC_GLOBALKEYS);

    /*** create resolution button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_POSITION );
    XtSetArg (arglist[1], XmNleftPosition, 45);
    XtSetArg (arglist[2], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Res=123456.7 units") );
    trace->command.reschg_but = XmCreatePushButton (trace->command.form, "res", arglist, 4);
    DAddCallback (trace->command.reschg_but, XmNactivateCallback, win_chg_res_cb, trace);
    DManageChild (trace->command.reschg_but, trace, MC_GLOBALKEYS);
    /* No more size changes */
    XtSetArg (arglist[0], XmNrecomputeSize, FALSE );
    XtSetValues (trace->command.reschg_but,arglist,1);

    /* create the horizontal scroll bar */
    XtSetArg (arglist[0], XmNorientation, XmHORIZONTAL );
    XtSetArg (arglist[1], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNbottomWidget, trace->command.end_but);
    XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNleftOffset, global->xstart - XSTART_MARGIN);
    XtSetArg (arglist[5], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[6], XmNrightWidget, trace->vscroll);
    XtSetArg (arglist[7], XmNheight, 18);
    trace->hscroll = XmCreateScrollBar ( trace->command.form, "hscroll", arglist, 8);
    DAddCallback (trace->hscroll, XmNincrementCallback, hscroll_unitinc_cb, trace);
    DAddCallback (trace->hscroll, XmNdecrementCallback, hscroll_unitdec_cb, trace);
    DAddCallback (trace->hscroll, XmNdragCallback, hscroll_drag_cb, trace);
    DAddCallback (trace->hscroll, XmNpageIncrementCallback, hscroll_pageinc_cb, trace);
    DAddCallback (trace->hscroll, XmNpageDecrementCallback, hscroll_pagedec_cb, trace);
    DManageChild (trace->hscroll, trace, MC_GLOBALKEYS);

    /* create the signal name horizontal scroll bar */
    XtSetArg (arglist[0], XmNorientation, XmHORIZONTAL );
    XtSetArg (arglist[1], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[2], XmNbottomWidget, trace->command.end_but);
    XtSetArg (arglist[3], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[4], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[5], XmNrightWidget, trace->hscroll);
    XtSetArg (arglist[6], XmNheight, 18);
    XtSetArg (arglist[7], XmNwidth, 18);
    trace->command.namescroll = XmCreateScrollBar ( trace->command.form, "namescroll", arglist, 8);
    DAddCallback (trace->command.namescroll, XmNvalueChangedCallback, win_namescroll_change_cb, trace);
    DAddCallback (trace->command.namescroll, XmNdragCallback,  win_namescroll_change_cb, trace);
    DManageChild (trace->command.namescroll, trace, MC_GLOBALKEYS);

    /*** create full button in command region ***/
    XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNrightWidget, trace->command.reschg_but);
    XtSetArg (arglist[2], XmNrightOffset, 2);
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Full") );
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.resfull_but = XmCreatePushButton (trace->command.form, "full", arglist, 5);
    DAddCallback (trace->command.resfull_but, XmNactivateCallback, win_full_res_cb, trace);
    DManageChild (trace->command.resfull_but, trace, MC_GLOBALKEYS);

    /*** create zoom button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNleftWidget, trace->command.reschg_but);
    XtSetArg (arglist[2], XmNleftOffset, 2);
    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Zoom") );
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    trace->command.reszoom_but = XmCreatePushButton (trace->command.form, "zoom", arglist, 5);
    DAddCallback (trace->command.reszoom_but, XmNactivateCallback, win_zoom_res_cb, trace);
    DManageChild (trace->command.reszoom_but, trace, MC_GLOBALKEYS);

    /*** create resolution decrease button in command region ***/
    XtSetArg (arglist[0], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNrightWidget, trace->command.resfull_but);
    XtSetArg (arglist[2], XmNrightOffset, 2);
    XtSetArg (arglist[3], XmNheight, 25);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[5], XmNarrowDirection, XmARROW_LEFT );
    trace->command.resdec_but = XmCreateArrowButton (trace->command.form, "decres", arglist, 6);
    DAddCallback (trace->command.resdec_but, XmNactivateCallback, win_dec_res_cb, trace);
    DManageChild (trace->command.resdec_but, trace, MC_GLOBALKEYS);

    /*** create resolution increase button in command region ***/
    XtSetArg (arglist[0], XmNleftAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[1], XmNleftWidget, trace->command.reszoom_but);
    XtSetArg (arglist[2], XmNleftOffset, 2);
    XtSetArg (arglist[3], XmNheight, 25);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_FORM );
    XtSetArg (arglist[5], XmNarrowDirection, XmARROW_RIGHT );
    trace->command.resinc_but = XmCreateArrowButton (trace->command.form, "incres", arglist, 6);
    DAddCallback (trace->command.resinc_but, XmNactivateCallback, win_inc_res_cb, trace);
    DManageChild (trace->command.resinc_but, trace, MC_GLOBALKEYS);

    /****************************************
     * create the work area window
     ****************************************/

    XtSetArg (arglist[0], XmNtopAttachment, XmATTACH_FORM );
    XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
    XtSetArg (arglist[2], XmNrightAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[3], XmNrightWidget, trace->vscroll);
    XtSetArg (arglist[4], XmNbottomAttachment, XmATTACH_WIDGET );
    XtSetArg (arglist[5], XmNbottomWidget, trace->hscroll);
    XtSetArg (arglist[6], XmNtranslations, XtParseTranslationTable (work_translations));
    trace->work = XmCreateDrawingArea (trace->command.form,"work", arglist, 7);

    DAddCallback (trace->work, XmNexposeCallback, win_expose_cb, trace);
    DAddCallback (trace->work, XmNresizeCallback, win_resize_cb, trace);
    DManageChild (trace->work, trace, MC_GLOBALKEYS);

    DManageChild (trace->main, trace, MC_GLOBALKEYS);
    DManageChild (trace->command.form, trace, MC_GLOBALKEYS);
    XtRealizeWidget (trace->toplevel);

    /* Display parameters */
    trace->wind = XtWindow (trace->work);
    trace->gc = XCreateGC (global->display, trace->wind, 0, NULL);
    trace->pixmap = XCreatePixmap (global->display, XtWindow (trace->work),
				   XtWidth(trace->work), XtHeight(trace->work),
				   DefaultDepthOfScreen(XtScreen(trace->work)));

    /* Choose fonts */
    global->signal_font = grab_font (trace, global->signal_font_name);
    global->time_font   = grab_font (trace, global->time_font_name);
    global->value_font  = grab_font (trace, global->value_font_name);

    XSetFont (global->display, trace->gc, global->signal_font->fid);

    /* Hack into scroll bar intrinsics */
    draw_scroll_hook_cb_expose ();

    /* Reset requestors and prepare a redraw */
    print_reset (trace);

    config_trace_defaults (trace);

    new_res (trace, global->res);

    set_cursor (DC_NORMAL);

    get_geometry (trace);

    dm_set_menu_closes ();

    return (trace);
}


