/******************************************************************************
 *
 * Filename:
 *     dt_value.c
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V6.3
 *
 * Author:
 *     Wilson Snyder
 *
 * Abstract:
 *
 * Modification History:
 *     WPS	20-Jun-93	Created from dt_signal.c
 */


#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/SelectioB.h>
#include <Xm/Text.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>

#include "dinotrace.h"
#include "callbacks.h"


/****************************** UTILITIES ******************************/

#pragma inline (value_to_string)
void    value_to_string (trace, strg, cptr)
    TRACE *trace;
    char *strg;
    unsigned int cptr[];
{
    if (cptr[2]) {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X %08X %08X", cptr[2], cptr[1], cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o %o %o", cptr[2], cptr[1], cptr[0]);
	}
    else if (cptr[1]) {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X %08X", cptr[1], cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o %o", cptr[1], cptr[0]);
	}
    else {
	if (trace->busrep == HBUS)
	    sprintf (strg,"%X", cptr[0]);
	else if (trace->busrep == OBUS)
	    sprintf (strg,"%o", cptr[0]);
	}
    }

void    string_to_value (trace, strg, cptr)
    TRACE *trace;
    char *strg;
    unsigned int cptr[];
{
    register char value;
    unsigned int MSO = (7<<29);		/* Most significant hex digit */
    unsigned int MSH = (15<<28);	/* Most significant octal digit */

    cptr[0] = cptr[1] = cptr[2] = 0;

    for (; *strg; strg++) {
	value = -1;
	if (*strg >= '0' && *strg <= '9')
	    value = *strg - '0';
	else if (*strg >= 'A' && *strg <= 'F')
	    value = *strg - ('A' - 10);
	else if (*strg >= 'a' && *strg <= 'f')
	    value = *strg - ('a' - 10);

	if (trace->busrep == HBUS && value >=0 && value <= 15) {
	    cptr[2] = (cptr[2]<<4) + ((cptr[1] & MSH)>>28);
	    cptr[1] = (cptr[1]<<4) + ((cptr[0] & MSH)>>28);
	    cptr[0] = (cptr[0]<<4) + value;
	    }
	else if (trace->busrep == OBUS && value >=0 && value <= 7) {
	    cptr[2] = (cptr[2]<<3) + ((cptr[1] & MSO)>>29);
	    cptr[1] = (cptr[1]<<3) + ((cptr[0] & MSO)>>29);
	    cptr[0] = (cptr[0]<<3) + value;
	    }
	}
    }

void	val_update_search ()
{
    TRACE	*trace;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    int		found, cursorize;
    register int i;
    CURSOR	*csr_ptr,*new_csr_ptr;

    if (DTPRINT) printf ("In val_update_search\n");

    /* Mark all cursors that are a result of a search as old (-1) */
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	if (csr_ptr->search) csr_ptr->search = -1;
	}

    /* Search every trace for the value, mark the signal if it has it to speed up displaying */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	/* don't do anything if no file is loaded */
	if (!trace->loaded) continue;
	
	for (sig_ptr = trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (sig_ptr->lws == 1) {
		/* Single bit signal, don't search for values */
		continue;
		}
	    
	    found=0;
	    cursorize=0;
	    cptr = (SIGNAL_LW *)(sig_ptr)->bptr;
	    
	    for (; (cptr->sttime.time != EOT); cptr += sig_ptr->lws) {
		switch (cptr->sttime.state) {
		  case STATE_B32:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1] == 0) 
			    && ( global->val_srch[i].value[2] == 0) ) {
			    found |= ( global->val_srch[i].color != 0) ;
			    if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    break;
			    }
			}
		    break;
		    
		  case STATE_B64:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && ( global->val_srch[i].value[2] == 0) ) {
			    found |= ( global->val_srch[i].color != 0) ;
			    if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    break;
			    }
			}
		    break;
		    
		  case STATE_B96:
		    for (i=0; i<MAX_SRCH; i++) {
			if ( ( global->val_srch[i].value[0]== *((unsigned int *)cptr+1) )
			    && ( global->val_srch[i].value[1]== *((unsigned int *)cptr+2) )
			    && ( global->val_srch[i].value[2]== *((unsigned int *)cptr+3) ) ) {
			    found |= ( global->val_srch[i].color != 0) ;
			    if ( global->val_srch[i].cursor != 0) cursorize = global->val_srch[i].cursor;
			    break;
			    }
			}
		    break;
		    } /* switch */

		if (cursorize) {
		    if (NULL != (csr_ptr = time_to_cursor (cptr->sttime.time))) {
			if (csr_ptr->search == -1) {
			    /* mark the old cursor as new so won't be deleted */
			    csr_ptr->search = cursorize;
			    }
			}
		    else {
			/* Make new cursor at this location */
			cur_add (cptr->sttime.time, cursorize, cursorize);
			}
		    cursorize = 0;
		    }

		} /* for cptr */
	    
	    sig_ptr->srch_ena = found;
	    if (found && DTPRINT) printf ("Signal %s matches search string.\n", sig_ptr->signame);
	    
	    } /* for sig */
	} /* for trace */

    /* Delete all old cursors */
    for (csr_ptr = global->cursor_head; csr_ptr; ) {
	new_csr_ptr = csr_ptr;
	csr_ptr = csr_ptr->next;
	if (new_csr_ptr->search==-1) {
	    cur_remove (new_csr_ptr);
	    DFree (new_csr_ptr);
	    }
	}
    }

/****************************** MENU OPTIONS ******************************/

void    val_examine_cb (w, trace, cb)
    Widget		w;
    TRACE		*trace;
    XmAnyCallbackStruct	*cb;
{
    
    if (DTPRINT) printf ("In val_examine_cb - trace=%d\n",trace);
    
    /* remove any previous events */
    remove_all_events (trace);
    
    /* process all subsequent button presses as cursor moves */
    set_cursor (trace, DC_VAL_EXAM);
    add_event (ButtonPressMask, val_examine_ev);
    }


/****************************** EVENTS ******************************/

void    val_examine_popup (trace, x, y, ev)
    /* Create the popup menu for val_examine, based on cursor position x,y */
    TRACE		*trace;
    int			x,y;
    XButtonPressedEvent	*ev;
{
    int		time;
    SIGNAL	*sig_ptr;
    SIGNAL_LW	*cptr;
    char	strg[2000];
    char	strg2[2000];
    XmString	xs;
    int		value[3];
    int		rows, cols, bit, bit_value, row, col;
    
    time = posx_to_time (trace, x);
    sig_ptr = posy_to_signal (trace, y);
    
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	/* XtUnmanageChild (trace->examine.label); */
	trace->examine.popup = NULL;
	}
    
    if (time && sig_ptr) {
	/* Get information */
	for (cptr = (SIGNAL_LW *)(sig_ptr)->cptr;
	     (cptr->sttime.time != EOT) && (((cptr + sig_ptr->lws)->sttime.time) < time);
	     cptr += sig_ptr->lws) ;
	
	strcpy (strg, sig_ptr->signame);
	
	if (cptr->sttime.time == EOT) {
	    strcat (strg, "\nValue at EOT:\n");
	    }
	else {
	    strcat (strg, "\nValue at times ");
	    time_to_string (trace, strg2, cptr->sttime.time, FALSE);
	    strcat (strg, strg2);
	    strcat (strg, " - ");
	    if (((cptr + sig_ptr->lws)->sttime.time) == EOT) {
		strcat (strg, "EOT:\n");
		}
	    else {
		time_to_string (trace, strg2, (cptr + sig_ptr->lws)->sttime.time, FALSE);
		strcat (strg, strg2);
		strcat (strg, ":\n");
		}
	    }
	
	value[0] = value[1] = value[2] = 0;
	switch (cptr->sttime.state) {
	  case STATE_1:
	    value[0] = 1;
	    break;
	    
	  case STATE_B32:
	    value[0] = *((unsigned int *)cptr+1);
	    break;
	
	  case STATE_B64:
	    value[0] = *((unsigned int *)cptr+1);
	    value[1] = *((unsigned int *)cptr+2);
	    break;

	  case STATE_B96:
	    value[0] = *((unsigned int *)cptr+1);
	    value[1] = *((unsigned int *)cptr+2);
	    value[2] = *((unsigned int *)cptr+3);
	    break;
	    } /* switch */

	switch (cptr->sttime.state) {
	  case STATE_0:
	  case STATE_1:
	    sprintf (strg2, "= %d\n", value[0]);
	    strcat (strg, strg2);
	    break;
	  
	  case STATE_Z:
	    sprintf (strg2, "= Z\n");
	    strcat (strg, strg2);
	    break;
	  
	  case STATE_U:
	    sprintf (strg2, "= U\n");
	    strcat (strg, strg2);
	    break;
	  
	  case STATE_B32:
	  case STATE_B64:
	  case STATE_B96:
	    strcat (strg, "= ");
	    value_to_string (trace, strg2, value);
	    strcat (strg, strg2);
	    if ( (sig_ptr->decode != NULL) 
		&& (cptr->sttime.state == STATE_B32)
		&& (value[0] < MAXSTATENAMES)
                && (sig_ptr->decode->statename[value[0]][0] != '\0') ) {
		sprintf (strg2, " = %s\n", sig_ptr->decode->statename[value[0]] );
		strcat (strg, strg2);
		}
	    else strcat (strg, "\n");

	    /* Bitwise information */
	    rows = ceil (sqrt ((double)(sig_ptr->bits + 1)));
	    cols = ceil ((double)(rows) / 4.0) * 4;
	    rows = ceil ((double)(sig_ptr->bits + 1)/ (double)cols);

	    bit = 0;
	    for (row=rows - 1; row >= 0; row--) {
		for (col = cols - 1; col >= 0; col--) {
		    bit = (row * cols + col);

		    if (bit<32) bit_value = ( value[0] >> bit ) & 1;
			else if (bit<64) bit_value = ( value[1] >> (bit-32) ) & 1;
			    else  bit_value = ( value[2] >> (bit-64) ) & 1;

		    if ((bit>=0) && (bit <= sig_ptr->bits)) {
			sprintf (strg2, "<%02d>=%d ", sig_ptr->msb_index +
				 ((sig_ptr->msb_index >= sig_ptr->lsb_index)
				  ? (bit - sig_ptr->bits) : (sig_ptr->bits - bit)),
				 bit_value);
			strcat (strg, strg2);
			if (col==4) strcat (strg, "  ");
			}
		    }
		strcat (strg, "\n");
		}

            break;
            } /* Case */

	/* Debugging information */
	if (DTDEBUG) {
	    sprintf (strg2, "\nState %d\n", cptr->sttime.state);
	    strcat (strg, strg2);
	    sprintf (strg2, "Type %d   Lws %d   Blocks %d\n",
		     sig_ptr->type, sig_ptr->lws, sig_ptr->blocks);
	    strcat (strg, strg2);
	    sprintf (strg2, "Bits %d   Index %d - %d  Srch_ena %d\n",
		     sig_ptr->bits, sig_ptr->msb_index, sig_ptr->lsb_index, sig_ptr->srch_ena);
	    strcat (strg, strg2);
	    sprintf (strg2, "File_type %d   File_Pos %d-%d  Mask %08lx\n",
		     sig_ptr->file_type.flags, sig_ptr->file_pos, sig_ptr->file_end_pos, sig_ptr->pos_mask);
	    strcat (strg, strg2);
	    sprintf (strg2, "Value_mask %08lx %08lx %08lx\n",
		     sig_ptr->value_mask[2], sig_ptr->value_mask[1], sig_ptr->value_mask[0]);
	    strcat (strg, strg2);
	    }
	
        /* Create */
        if (DTPRINT) printf ("\ttime = %d, signal = %s\n", time, sig_ptr->signame);

	XtSetArg (arglist[0], XmNentryAlignment, XmALIGNMENT_BEGINNING);
	trace->examine.popup = XmCreatePopupMenu (trace->main, "examinepopup", arglist, 1);

	xs = string_create_with_cr (strg);
	XtSetArg (arglist[0], XmNlabelString, xs);
	trace->examine.label = XmCreateLabel (trace->examine.popup,"popuplabel",arglist,1);
	XtManageChild (trace->examine.label);
	XmStringFree (xs);

	XmMenuPosition (trace->examine.popup, ev);
	XtManageChild (trace->examine.popup);
	}
    }
	
void    val_examine_ev (w, trace, ev)
    Widget		w;
    TRACE		*trace;
    XButtonPressedEvent	*ev;
{
    XEvent	event;
    XMotionEvent *em;
    int		update_pending = FALSE;
    
    if (DTPRINT) printf ("In val_examine_ev\n");
    
    /* not sure why this has to be done but it must be done */
    XUngrabPointer (XtDisplay (trace->work),CurrentTime);
    
    /* select the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonReleaseMask|PointerMotionMask|StructureNotifyMask|ExposureMask);
    
    /* Create */
    val_examine_popup (trace, ev->x, ev->y, ev);
    
    /* loop and service events until button is released */
    while ( 1 ) {
	/* wait for next event */
	XNextEvent (XtDisplay (trace->work),&event);
	
	/* Mark an update as needed */
	if (event.type == MotionNotify) {
	    update_pending = TRUE;
	    }

	/* if window was exposed or resized, must redraw it */
	if (event.type == Expose ||
	    event.type == ConfigureNotify) {
	    win_expose_cb (0,trace);
	    }
	
	/* button released - calculate cursor position and leave the loop */
	if (event.type == ButtonRelease) {
	    break;
	    }

	/* If a update is needed, redraw the menu. */
	/* Do it later if events pending, otherwise dragging is SLOWWWW */
	if (update_pending && !XPending (global->display)) {
	    update_pending = FALSE;
	    em = (XMotionEvent *)&event;
	    val_examine_popup (trace, em->x, em->y, em);
	    }
	}
    
    /* unmanage popup */
    if (trace->examine.popup) {
	XtUnmanageChild (trace->examine.popup);
	XtUnmanageChild (trace->examine.label);
	trace->examine.popup = NULL;
	}

    /* reset the events the widget will respond to */
    XSelectInput (XtDisplay (trace->work),XtWindow (trace->work),
		 ButtonPressMask|StructureNotifyMask|ExposureMask);
    
    /* redraw the screen as popup may have mangled widgets */
    redraw_all (trace);
    }

void    val_search_cb (w,trace,cb)
    Widget		w;
    TRACE	*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    int		i;
    int		y=10;
    char	strg[40];
    
    if (DTPRINT) printf ("In val_search_cb - trace=%d\n",trace);
    
    if (!trace->value.search) {
	XtSetArg (arglist[0], XmNdefaultPosition, TRUE);
	XtSetArg (arglist[1], XmNdialogTitle, XmStringCreateSimple ("Value Search Requester") );
	/* XtSetArg (arglist[2], XmNwidth, 500);
	   XtSetArg (arglist[3], XmNheight, 400); */
	trace->value.search = XmCreateBulletinBoardDialog (trace->work,"search",arglist,2);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Color"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label1 = XmCreateLabel (trace->value.search,"label1",arglist,3);
	XtManageChild (trace->value.label1);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Place"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label2 = XmCreateLabel (trace->value.search,"label2",arglist,3);
	XtManageChild (trace->value.label2);
	
	y += 15;
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Value"));
	XtSetArg (arglist[1], XmNx, 5);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label4 = XmCreateLabel (trace->value.search,"label4",arglist,3);
	XtManageChild (trace->value.label4);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cursor"));
	XtSetArg (arglist[1], XmNx, 60);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label5 = XmCreateLabel (trace->value.search,"label5",arglist,3);
	XtManageChild (trace->value.label5);
	
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple
		 ( (trace->busrep == HBUS)? "Search value in HEX":"Search value in OCTAL" ) );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.label3 = XmCreateLabel (trace->value.search,"label3",arglist,3);
	XtManageChild (trace->value.label3);
	
	y += 25;

	for (i=0; i<MAX_SRCH; i++) {
	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 15);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->value.enable[i] = XmCreateToggleButton (trace->value.search,"togglen",arglist,4);
	    XtManageChild (trace->value.enable[i]);

	    /* enable button */
	    XtSetArg (arglist[0], XmNx, 70);
	    XtSetArg (arglist[1], XmNy, y);
	    XtSetArg (arglist[2], XmNselectColor, trace->xcolornums[i+1]);
	    XtSetArg (arglist[3], XmNlabelString, XmStringCreateSimple (""));
	    trace->value.cursor[i] = XmCreateToggleButton (trace->value.search,"cursorn",arglist,4);
	    XtManageChild (trace->value.cursor[i]);

	    /* create the file name text widget */
	    XtSetArg (arglist[0], XmNrows, 1);
	    XtSetArg (arglist[1], XmNcolumns, 30);
	    XtSetArg (arglist[2], XmNx, 120);
	    XtSetArg (arglist[3], XmNy, y);
	    XtSetArg (arglist[4], XmNresizeHeight, FALSE);
	    XtSetArg (arglist[5], XmNeditMode, XmSINGLE_LINE_EDIT);
	    trace->value.text[i] = XmCreateText (trace->value.search,"textn",arglist,6);
	    XtAddCallback (trace->value.text[i], XmNactivateCallback, val_search_ok_cb, trace);
	    XtManageChild (trace->value.text[i]);
	    
	    y += 40;
	    }

	y+= 15;

	/* Create OK button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple (" OK ") );
	XtSetArg (arglist[1], XmNx, 10);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.ok = XmCreatePushButton (trace->value.search,"ok",arglist,3);
	XtAddCallback (trace->value.ok, XmNactivateCallback, val_search_ok_cb, trace);
	XtManageChild (trace->value.ok);
	
	/* create apply button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Apply") );
	XtSetArg (arglist[1], XmNx, 70);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.apply = XmCreatePushButton (trace->value.search,"apply",arglist,3);
	XtAddCallback (trace->value.apply, XmNactivateCallback, val_search_apply_cb, trace);
	XtManageChild (trace->value.apply);
	
	/* create cancel button */
	XtSetArg (arglist[0], XmNlabelString, XmStringCreateSimple ("Cancel") );
	XtSetArg (arglist[1], XmNx, 140);
	XtSetArg (arglist[2], XmNy, y);
	trace->value.cancel = XmCreatePushButton (trace->value.search,"cancel",arglist,3);
	XtAddCallback (trace->value.cancel, XmNactivateCallback, unmanage_cb, trace->value.search);
	XtManageChild (trace->value.cancel);
	}
    
    /* Copy settings to local area to allow cancel to work */
    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	XtSetArg (arglist[0], XmNset, (global->val_srch[i].color != 0));
	XtSetValues (trace->value.enable[i], arglist, 1);

	/* Update with current cursor enables */
	XtSetArg (arglist[0], XmNset, (global->val_srch[i].cursor != 0));
	XtSetValues (trace->value.cursor[i], arglist, 1);

	/* Update with current search values */
	value_to_string (trace, strg, global->val_srch[i].value);
	XmTextSetString (trace->value.text[i], strg);
	}

    /* manage the popup on the screen */
    XtManageChild (trace->value.search);
    }

void    val_search_ok_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    char		*strg;
    int			i;

    if (DTPRINT) printf ("In val_search_ok_cb - trace=%d\n",trace);

    for (i=0; i<MAX_SRCH; i++) {
	/* Update with current search enables */
	if (XmToggleButtonGetState (trace->value.enable[i]))
	    global->val_srch[i].color = i+1;
	else global->val_srch[i].color = 0;
	
	/* Update with current cursor enables */
	if (XmToggleButtonGetState (trace->value.cursor[i]))
	    global->val_srch[i].cursor = i+1;
	else global->val_srch[i].cursor = 0;
	
	/* Update with current search values */
	strg = XmTextGetString (trace->value.text[i]);
	string_to_value (trace, strg, global->val_srch[i].value);

	if (DTPRINT) {
	    char strg2[40];
	    value_to_string (trace, strg2, global->val_srch[i].value);
	    printf ("Search %d) %d   '%s' -> '%s'\n", i, global->val_srch[i].color, strg, strg2);
	    }
	}
    
    XtUnmanageChild (trace->value.search);

    val_update_search ();

    /* redraw the display */
    redraw_all (trace);
    }

void    val_search_apply_cb (w,trace,cb)
    Widget				w;
    TRACE			*trace;
    XmSelectionBoxCallbackStruct *cb;
{
    if (DTPRINT) printf ("In val_search_apply_cb - trace=%d\n",trace);

    val_search_ok_cb (w,trace,cb);
    val_search_cb (w,trace,cb);
    }