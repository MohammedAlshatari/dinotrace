#ident "$Id$"
/******************************************************************************
 * dt_grid.c --- grid drawing and requestors
 *
 * This file is part of Dinotrace.  
 *
 * Author: Wilson Snyder <wsnyder@world.std.com> or <wsnyder@ultranet.com>
 *
 * Code available from: http://www.ultranet.com/~wsnyder/dinotrace
 *
 ******************************************************************************
 *
 * Some of the code in this file was originally developed for Digital
 * Semiconductor, a division of Digital Equipment Corporation.  They
 * gratefuly have agreed to share it, and thus the bas version has been
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
 * the Free Software Foundation; either version 2, or (at your option)
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

#include <Xm/Label.h>
#include <Xm/LabelP.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/CascadeBP.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/BulletinB.h>

#include "functions.h"

/**********************************************************************/

extern void	grid_customize_ok_cb(), grid_customize_apply_cb(), grid_customize_reset_cb(), grid_customize_sensitives_cb(), grid_customize_option_cb();
extern void	grid_customize_align_cb (Widget w, Trace *trace, XmAnyCallbackStruct *cb);


/****************************** AUTO GRIDS ******************************/

void	grid_calc_auto (
    Trace	*trace,
    Grid	*grid_ptr)
{
    Signal	*sig_ptr;
    SignalLW_t	*cptr;
    int		rise1=0, rise2=0;
    int		fall1=0, fall2=0;

    if (DTPRINT_ENTRY) printf ("In grid_calc_auto\n");

    /* Determine period, rise point and fall point of first signal */
    sig_ptr = sig_wildmat_signame (trace, grid_ptr->signal);

    /* Skip phase_count, as it is a CCLI artifact */
    if (sig_ptr && !strncmp(sig_ptr->signame, "phase_count", 11)) sig_ptr=sig_ptr->forward;

    /* Can't do anything if don't have a signal yet */
    if (!sig_ptr) {
	/* Effectively disable grid */
	switch (grid_ptr->period_auto) {
	case PA_AUTO:
	    grid_ptr->period = 0;
	    break;
	case PA_USER:
	case PA_EDGE:
	}
	return;	
    }

    /* Skip to end */
    cptr = cptr_at_time (sig_ptr, EOT);
    /* Ignore last - determined by EOT, not period */
    if ( cptr != sig_ptr->cptr) cptr -= CPTR_SIZE_PREV(cptr);

    /* Move back 3 grids, if we can */
    while ( cptr != sig_ptr->cptr) {
	cptr -= CPTR_SIZE_PREV(cptr);
	switch (cptr->stbits.state) {
	  case STATE_1:
	    if (!rise2) rise2 = CPTR_TIME(cptr);
	    else if (!rise1) rise1 = CPTR_TIME(cptr);
	    break;
	  case STATE_0:
	    if (!fall2) fall2 = CPTR_TIME(cptr);
	    else if (!fall1) fall1 = CPTR_TIME(cptr);
	    break;
	  case STATE_B32:
	  case STATE_B128:
	    if (!rise2) rise2 = CPTR_TIME(cptr);
	    else if (!rise1) rise1 = CPTR_TIME(cptr);
	    if (!fall2) fall2 = CPTR_TIME(cptr);
	    else if (!fall1) fall1 = CPTR_TIME(cptr);
	    break;
	}
    }
    
    /* Set defaults based on changes */
    switch (grid_ptr->period_auto) {
      case PA_AUTO:
	if (rise1 < rise2)	grid_ptr->period = rise2 - rise1;
	else if (fall1 < fall2) grid_ptr->period = fall2 - fall1;
	break;
      default: break;	/* User defined */
    }
    if (grid_ptr->period < 1) grid_ptr->period = 1;	/* Prevents round-down to 0 causing infinite loop */
    
    /* Alignment */
    switch (grid_ptr->align_auto) {
      case AA_ASS:
	if (rise1) grid_ptr->alignment = rise1 % grid_ptr->period;
	break;
      case AA_DEASS:
	if (fall1) grid_ptr->alignment = fall1 % grid_ptr->period;
	break;
      default: break;	/* User defined */
    }
    
    if (DTPRINT_FILE) printf ("grid autoset signal %s align=%d %d\n", sig_ptr->signame,
       grid_ptr->align_auto, grid_ptr->period_auto);
    if (DTPRINT_FILE) printf ("grid rises=%d,%d, falls=%d,%d, period=%d, align=%d\n",
       rise1,rise2, fall1,fall2,
       grid_ptr->period, grid_ptr->alignment);
}

void	grid_calc_autos (
    Trace		*trace)
{
    int		grid_num;

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_calc_auto (trace, &(trace->grid[grid_num]));
    }

    draw_needed (trace);
}

/****************************** MENU OPTIONS ******************************/

void    grid_align_choose (
    Trace	*trace,
    Grid	*grid_ptr)
{
    DTime	time;
    Position	x1,x2,y1,y2;
    int		last_x;
    XEvent	event;
    XMotionEvent *em;
    XButtonEvent *eb;
    
    if (DTPRINT_ENTRY) printf ("In grid_align_choose\n");
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function to drag the cursor */
    xgcv.function = GXinvert;
    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
    XSync (global->display,0);
    
    /* loop and service events until button is released */
    last_x = -1;	/* Skip first draw */
    y1 = 25;
    while ( 1 ) {
	/* wait for next event */
	XNextEvent (XtDisplay (trace->work),&event);

	/* if the pointer moved, erase previous line and draw new one */
	if (event.type == MotionNotify) {
	    XSetForeground (global->display, trace->gc, trace->xcolornums[grid_ptr->color]);
	    em = (XMotionEvent *)&event;
	    y2 = trace->height - trace->ystart + trace->sighgt;
	    if (last_x >= 0) {
		x1 = x2 = last_x;
		XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    }
	    x1 = x2 = last_x = em->x;
	    XDrawLine (global->display,trace->wind,trace->gc,x1,y1,x2,y2);
	    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);
	}
	
	/* if window was exposed, must redraw it */
	if (event.type == Expose) win_expose_cb (0,trace);
	/* if window was resized, must redraw it */
	if (event.type == ConfigureNotify) win_resize_cb (0,trace);
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease || event.type == ButtonPress) {
	    /* ButtonPress in case user is freaking out, some strange X behavior caused the ButtonRelease to be lost */
	    eb = (XButtonReleasedEvent *)&event;
	    time = posx_to_time_edge (trace, eb->x, eb->y);
	    break;
	}

	if (global->redraw_needed && !XtAppPending (global->appcontext)) {
	    /* change the GC function back to its default */
	    xgcv.function = GXcopy;
	    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
	    draw_perform();
	    /* change the GC function to drag the cursor */
	    xgcv.function = GXinvert;
	    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
	}
    }
    
    /* reset the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* change the GC function back to its default */
    xgcv.function = GXcopy;
    XChangeGC (global->display,trace->gc,GCFunction,&xgcv);
    
    /* make change */
    grid_ptr->alignment = time;
}


void grid_customize_align_cb (
    Widget		w,
    Trace		*trace,
    XmAnyCallbackStruct	*cb)
{
    int			grid_num;

    if (DTPRINT_ENTRY) printf ("In grid_customize_align_cb - trace=%p\n",trace);

    /*   Determine which grid was selected based on which array */
    for (grid_num = 0; grid_num < (MAXGRIDS-1); grid_num++) {	/* -1 as MAXGRID will be the "default if none match*/
	if (w == trace->gridscus.grid[grid_num].align) break;
    }
    global->click_grid = &(trace->grid[grid_num]);

    /* Apply changes */
    grid_customize_ok_cb (w,trace,cb);

    /* remove any previous events */
    remove_all_events (trace);

    /* process all subsequent button presses as grid aligns */
    grid_align_choose (trace, &(trace->grid[grid_num]));

    /* Redraw */
    draw_needed (trace);

    /* Show new menu */
    grid_customize_cb (trace->main);
}


void grid_reset_cb (
    Widget		w)
{
    Trace *trace = widget_to_trace(w);
    int		grid_num;
    Grid	*grid_ptr;

    if (DTPRINT_ENTRY) printf ("In grid_reset_cb - trace=%p\n",trace);

    /* reset the alignment back to zero and resolution to 100 */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	strcpy (grid_ptr->signal, "*");
	grid_ptr->color = grid_num;
	grid_ptr->period = 100;
	grid_ptr->alignment = 0;
	grid_ptr->period_auto = PA_AUTO;
	grid_ptr->align_auto = AA_ASS;
	if (grid_num == 0) {
	    /* Enable only one grid by default */
	    grid_ptr->visible = TRUE;
	    grid_ptr->wide_line = TRUE;
	}
	else {
	    grid_ptr->visible = FALSE;
	    grid_ptr->wide_line = FALSE;
	}
     }

    grid_calc_autos (trace);

    /* cancel the button actions */
    remove_all_events (trace);
}

/****************************** CUSTOMIZE ******************************/

void    grid_customize_sensitives_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    int		grid_num;
    int		opt;

    if (DTPRINT_ENTRY) printf ("In grid_customize_sensitives_cb\n");

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	/* Period value, desensitize if automatic */
	opt = option_to_number(trace->gridscus.grid[grid_num].autoperiod_options, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton, 2);
	XtSetArg (arglist[0], XmNsensitive, (opt==0));
	XtSetValues (trace->gridscus.grid[grid_num].period, arglist, 1);

	/* Edge Mode */
	opt = option_to_number(trace->gridscus.grid[grid_num].autoalign_options, trace->gridscus.grid[grid_num].autoalign_pulldownbutton, 2);
	XtSetArg (arglist[0], XmNsensitive, (opt==0));
	XtSetValues (trace->gridscus.grid[grid_num].align, arglist, 1);
    }
}

void    grid_customize_widget_update_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    int		grid_num;
    Grid	*grid_ptr;
    char	strg[MAXTIMELEN];
    int		opt;

    if (DTPRINT_ENTRY) printf ("In grid_customize_widget_update\n");

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	XmTextSetString (trace->gridscus.grid[grid_num].signal, grid_ptr->signal);

	XmToggleButtonSetState (trace->gridscus.grid[grid_num].visible, (grid_ptr->visible), TRUE);
	XmToggleButtonSetState (trace->gridscus.grid[grid_num].wide_line, (grid_ptr->wide_line), TRUE);

	/* Period value */
	time_to_string (trace, strg, grid_ptr->period, TRUE);
	XmTextSetString (trace->gridscus.grid[grid_num].period, strg);

	/* Color */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].pulldownbutton[grid_ptr->color]);
	XtSetValues (trace->gridscus.grid[grid_num].options, arglist, 1);
	/* Must redraw color box on any exposures */
	XtAddEventHandler (trace->gridscus.dialog, ExposureMask, TRUE, grid_customize_option_cb, trace);

	/* period Mode */
	opt=(int)grid_ptr->period_auto;	/* Weedy, enums are defined to be equiv to descriptions */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[opt]);
	XtSetValues (trace->gridscus.grid[grid_num].autoperiod_options, arglist, 1);

	/* Edge Mode */
	opt=(int)grid_ptr->align_auto;	/* Weedy, enums are defined to be equiv to descriptions */
	XtSetArg (arglist[0], XmNmenuHistory, trace->gridscus.grid[grid_num].autoalign_pulldownbutton[opt]);
	XtSetValues (trace->gridscus.grid[grid_num].autoalign_options, arglist, 1);
    }

    /* Update sensitivity buttons too */
    grid_customize_sensitives_cb (NULL, trace, NULL);
}

void    grid_customize_cb (
    Widget	w)
{
    Trace *trace = widget_to_trace(w);
    int		grid_num;
    int		y=10, xb=0, ymax=0;
    char	strg[30];
    int		i;
    
    if (DTPRINT_ENTRY) printf ("In grid_customize_cb - trace=%p\n",trace);
    
    if (!trace->gridscus.dialog) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Grid Customization") );
	trace->gridscus.dialog = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);
	
	XtSetArg (arglist[0], XmNverticalSpacing, 7);
	trace->gridscus.form = XmCreateForm (trace->gridscus.dialog, "form", arglist, 1);
	DManageChild (trace->gridscus.form, trace, MC_NOKEYS);

	for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	    /* Create label for this grid */
	    sprintf (strg, "Grid #%d", grid_num);
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (strg));
	    XtSetArg (arglist[1], XmNx, xb+10);
	    XtSetArg (arglist[2], XmNy, y);
	    trace->gridscus.grid[grid_num].label1 = XmCreateLabel (trace->gridscus.form,"label",arglist,3);
	    DManageChild (trace->gridscus.grid[grid_num].label1, trace, MC_NOKEYS);
	
	    /* signal name */
	    y += 20;
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNx, xb+20);
	    XtSetArg (arglist[3], XmNy, y);
	    XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->gridscus.grid[grid_num].signal = XmCreateText (trace->gridscus.form,"textn",arglist,6);
	    DManageChild (trace->gridscus.grid[grid_num].signal, trace, MC_NOKEYS);
	    
	    /* visible button */
	    y += 40;
	    XtSetArg (arglist[0], XmNx, xb+20);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple ("Visible"));
	    XtSetArg (arglist[3], XmNshadowThickness, 1);
	    trace->gridscus.grid[grid_num].visible = XmCreateToggleButton (trace->gridscus.form,"visn",arglist,4);
	    DManageChild (trace->gridscus.grid[grid_num].visible, trace, MC_NOKEYS);
	    
	    /* double button */
	    XtSetArg (arglist[0], XmNx, xb+100);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNlabelString, XmStringCreateSimple ("Wide Line"));
	    XtSetArg (arglist[3], XmNshadowThickness, 1);
	    trace->gridscus.grid[grid_num].wide_line = XmCreateToggleButton (trace->gridscus.form,"wideln",arglist,4);
	    DManageChild (trace->gridscus.grid[grid_num].wide_line, trace, MC_NOKEYS);
	    
	    /*Color options */
	    trace->gridscus.grid[grid_num].pulldown = XmCreatePulldownMenu (trace->gridscus.form,"pulldown",arglist,0);

	    for (i=0; i<=MAX_SRCH; i++) {
		XtSetArg (arglist[0], XmNbackground, trace->xcolornums[i] );
		XtSetArg (arglist[1], XmNmarginRight, 20);
		XtSetArg (arglist[2], XmNmarginBottom, 2);
		XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple ("Color"));
		trace->gridscus.grid[grid_num].pulldownbutton[i] =
		    XmCreatePushButton (trace->gridscus.grid[grid_num].pulldown,"",arglist,4);
		DAddCallback (trace->gridscus.grid[grid_num].pulldownbutton[i], XmNactivateCallback, grid_customize_option_cb, trace);
		DManageChild (trace->gridscus.grid[grid_num].pulldownbutton[i], trace, MC_NOKEYS);
	    }
	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].pulldown);
	    XtSetArg (arglist[1], XmNx, xb+200);
	    XtSetArg (arglist[2], XmNy, y-4);
	    trace->gridscus.grid[grid_num].options = XmCreateOptionMenu (trace->gridscus.form,"options",arglist,3);
	    DManageChild (trace->gridscus.grid[grid_num].options, trace, MC_NOKEYS);
	
	    /* auto button */
	    y += 40;
	    trace->gridscus.grid[grid_num].autoperiod_pulldown = XmCreatePulldownMenu (trace->gridscus.form,"autoperiod_pulldown",arglist,0);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Periodic") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[0], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Auto Periodic") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[1], trace, MC_NOKEYS);

	    /*
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Auto Edges Only") );
	    trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoperiod_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2], XmNactivateCallback, grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_pulldownbutton[2], trace, MC_NOKEYS);
	    */

	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].autoperiod_pulldown);
	    XtSetArg (arglist[1], XmNx, xb+15);
	    XtSetArg (arglist[2], XmNy, y-3);
	    trace->gridscus.grid[grid_num].autoperiod_options = XmCreateOptionMenu (trace->gridscus.form,"options",arglist,3);
	    DManageChild (trace->gridscus.grid[grid_num].autoperiod_options, trace, MC_NOKEYS);

	    /* Period */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 10);
	    XtSetArg (arglist[2], XmNx, xb+195);
	    XtSetArg (arglist[3], XmNy, y);
	    XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->gridscus.grid[grid_num].period = XmCreateText (trace->gridscus.form,"textn",arglist,6);
	    DManageChild (trace->gridscus.grid[grid_num].period, trace, MC_NOKEYS);

	    /* auto button */
	    y += 40;
	    trace->gridscus.grid[grid_num].autoalign_pulldown = XmCreatePulldownMenu (trace->gridscus.form,"autoalign_pulldown",arglist,0);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Manual Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[0], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Assertion Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[1], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Deassertion Edge") );
	    trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2] =
		XmCreatePushButtonGadget (trace->gridscus.grid[grid_num].autoalign_pulldown,"pdbutton0",arglist,1);
	    DAddCallback (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2], XmNactivateCallback, (XtCallbackProc)grid_customize_sensitives_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_pulldownbutton[2], trace, MC_NOKEYS);

	    XtSetArg (arglist[0], XmNsubMenuId, trace->gridscus.grid[grid_num].autoalign_pulldown);
	    XtSetArg (arglist[1], XmNx, xb+15);
	    XtSetArg (arglist[2], XmNy, y-3);
	    trace->gridscus.grid[grid_num].autoalign_options = XmCreateOptionMenu (trace->gridscus.form,"options",arglist,3);
	    DManageChild (trace->gridscus.grid[grid_num].autoalign_options, trace, MC_NOKEYS);

	    /* Edge */
	    XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Set Alignment") );
	    XtSetArg (arglist[1], XmNx, xb+195);
	    XtSetArg (arglist[2], XmNy, y);
	    trace->gridscus.grid[grid_num].align = XmCreatePushButton (trace->gridscus.form,"align",arglist,3);
	    DAddCallback (trace->gridscus.grid[grid_num].align, XmNactivateCallback, (XtCallbackProc)grid_customize_align_cb, trace);
	    DManageChild (trace->gridscus.grid[grid_num].align, trace, MC_NOKEYS);

	    y += 60;
	    ymax = MAX (y, ymax);
	    if (y > 400) { xb += 350; y = 10; }
	}

	/* Create Separator */
	XtSetArg (arglist[0], XmNy, ymax );
	XtSetArg (arglist[1], XmNleftAttachment, XmATTACH_FORM );
	XtSetArg (arglist[2], XmNrightAttachment, XmATTACH_FORM );
	trace->gridscus.sep = XmCreateSeparator (trace->gridscus.form, "sep",arglist,3);
	DManageChild (trace->gridscus.sep, trace, MC_NOKEYS);
	
	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.sep );
	trace->gridscus.ok = XmCreatePushButton (trace->gridscus.form,"ok",arglist,4);
	DAddCallback (trace->gridscus.ok, XmNactivateCallback, grid_customize_ok_cb, trace);
	DManageChild (trace->gridscus.ok, trace, MC_NOKEYS);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.sep );
	trace->gridscus.apply = XmCreatePushButton (trace->gridscus.form,"apply",arglist,4);
	DAddCallback (trace->gridscus.apply, XmNactivateCallback, grid_customize_apply_cb, trace);
	DManageChild (trace->gridscus.apply, trace, MC_NOKEYS);
	
	/* create reset button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Reset") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.sep );
	trace->gridscus.apply = XmCreatePushButton (trace->gridscus.form,"reset",arglist,4);
	DAddCallback (trace->gridscus.apply, XmNactivateCallback, grid_customize_reset_cb, trace);
	DManageChild (trace->gridscus.apply, trace, MC_NOKEYS);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 200);
	XtSetArg (arglist[2], XmNtopAttachment, XmATTACH_WIDGET );
	XtSetArg (arglist[3], XmNtopWidget, trace->gridscus.sep );
	trace->gridscus.cancel = XmCreatePushButton (trace->gridscus.form,"cancel",arglist,4);
	DAddCallback (trace->gridscus.cancel, XmNactivateCallback, unmanage_cb, trace->gridscus.dialog);
	DManageChild (trace->gridscus.cancel, trace, MC_NOKEYS);
    }
    
    /* Copy settings to local area to allow cancel to work */
    grid_customize_widget_update_cb (NULL, trace, NULL);

    /* manage the dialog on the screen */
    DManageChild (trace->gridscus.dialog, trace, MC_NOKEYS);
}

void    grid_customize_option_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)	/* OR     XButtonPressedEvent	*ev; */
    /* Puts the color in the option menu, since Xm does not copy colors on selection */
    /* Also used as an event callback for exposures */
{
    int		i;
    Widget 	button;
    Position 	x,y,height,width;
    int		grid_num;

    if (DTPRINT_ENTRY) printf ("In grid_customize_option_cb - trace=%p\n",trace);

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	i = option_to_number(trace->gridscus.grid[grid_num].options, trace->gridscus.grid[grid_num].pulldownbutton, MAX_SRCH);

	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetValues ( XmOptionButtonGadget (trace->gridscus.grid[grid_num].options), arglist, 1);

	/* Find the coords of the button */
	button = XmOptionButtonGadget (trace->gridscus.grid[grid_num].options);
	x = (((XmCascadeButtonWidget) button)->core.x);
	y = (((XmCascadeButtonWidget) button)->core.y);
	width = (((XmCascadeButtonWidget) button)->core.width);
	height = (((XmCascadeButtonWidget) button)->core.height);
    
	/* Fill the button with the color */
	XSetForeground (global->display, trace->gc, trace->xcolornums[i]);
	XFillRectangle (global->display, XtWindow (button), trace->gc,
			x, y, width, height);
    }

    if (DTPRINT_ENTRY) printf ("Done grid_customize_option_cb - trace=%p\n",trace);
}

void    grid_customize_ok_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    int			grid_num;
    Grid	*grid_ptr;

    if (DTPRINT_ENTRY) printf ("In grid_customize_ok_cb - trace=%p\n",trace);

    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	grid_ptr = &(trace->grid[grid_num]);

	strcpy (grid_ptr->signal, XmTextGetString (trace->gridscus.grid[grid_num].signal));
	grid_ptr->visible = XmToggleButtonGetState (trace->gridscus.grid[grid_num].visible);
	grid_ptr->wide_line = XmToggleButtonGetState (trace->gridscus.grid[grid_num].wide_line);
	if (grid_ptr->period_auto == PA_USER) {
	    grid_ptr->period = string_to_time (trace, XmTextGetString (trace->gridscus.grid[grid_num].period));
	}

	grid_ptr->color = option_to_number(trace->gridscus.grid[grid_num].options, trace->gridscus.grid[grid_num].pulldownbutton, MAX_SRCH);
	grid_ptr->period_auto = option_to_number(trace->gridscus.grid[grid_num].autoperiod_options, trace->gridscus.grid[grid_num].autoperiod_pulldownbutton, 2);
	grid_ptr->align_auto = option_to_number(trace->gridscus.grid[grid_num].autoalign_options, trace->gridscus.grid[grid_num].autoalign_pulldownbutton, 2);
    }
    
    XtUnmanageChild (trace->gridscus.dialog);

    grid_calc_autos (trace);
}

void    grid_customize_apply_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In grid_customize_apply_cb - trace=%p\n",trace);

    grid_customize_ok_cb (w,trace,cb);
    grid_customize_cb (trace->main);
}

void    grid_customize_reset_cb (
    Widget	w,
    Trace	*trace,
    XmAnyCallbackStruct *cb)
{
    if (DTPRINT_ENTRY) printf ("In grid_customize_resize_cb - trace=%p\n",trace);

    XtUnmanageChild (trace->gridscus.dialog);
    grid_reset_cb (trace->main);
    grid_customize_cb (trace->main);
}

