#ident "$Id$"
/******************************************************************************
 * dt_draw.c --- screen trace drawing
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

#include <Xm/ScrollBar.h>
#include <Xm/ScrollBarP.h>

#include "functions.h"

/**********************************************************************/

#define MAXCNT	1000		/* Maximum number of segment pairs to draw before stopping this signal */
/* Don't make this above 4000, as some servers choke with that many points. */

#define OVERLAPSPACE 2		/* Number of pixels within which two signal transitions are visualized as one */

extern void draw_trace_signame (Trace *trace, const Signal *sig_ptr, Position y);
extern void draw_hscroll (Trace *trace);
extern void draw_vscroll (Trace *trace);

XtExposeProc draw_orig_scroll_expose=NULL;	/* Original function for scroll exposure */

void draw_string_fit (
    Trace	*trace,
    Boolean_t	*textoccupied,
    Position	x,
    Position	y,
    XFontStruct	*font,
    char	*strg)
    /* Draw a string if it will fit on the screen without overlapping something already printed */
    /*** Note textoccupied must be initialized before calling!!! */
{
    Position	len;		/* x width of the string to be printed */
    Position	testx;
    Position	maxx, minx;		/* Max and min position printing on */

    len = XTextWidth (font, strg, strlen (strg));

    minx = (x - len/2 - 2);
    maxx = (x + len/2 + 2);

    /* Test where the text will be printed, if printing occupies it, then abort printing it */
    for (testx = minx;  testx <= maxx; testx++) {
	if ((testx < 0)
	    || (testx >= MAXSCREENWIDTH)
	    || (textoccupied[testx] == TRUE))
	    break;
    }

    if (testx > maxx) {
	/* Fits on screen */
	XDrawString (global->display, trace->pixmap, trace->gc, minx + 2, y, strg, strlen (strg));
	/* Mark these positions as occupied */
	for (testx = minx;  testx <= maxx; testx++) {
	    textoccupied[testx] = TRUE;
	}
    }
}
    
void draw_grid_line (
    Trace	*trace,
    Boolean_t	*textoccupied,
    const Grid	*grid_ptr,		/* Grid information */
    DTime	xtime,
    Position	y0,
    Position	y1,
    Position	y2)
{
    Position	x2;			/* Coordinate of current time */
    char 	strg[MAXTIMELEN];	/* String value to print out */

    x2 = TIME_TO_XPOS (xtime);

    /* if the grid is visible, draw a vertical dashed line */
    XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0, 0);
    XDrawLine     (global->display, trace->pixmap,trace->gc,   x2, y1,   x2, y2);
    if (grid_ptr->wide_line) {
	XDrawLine (global->display, trace->pixmap,trace->gc, 1+x2, y1, 1+x2, y2);
    }

    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
    XDrawLine     (global->display, trace->pixmap,trace->gc, x2,   y0,   x2, y1);
    if (grid_ptr->wide_line) {
	XDrawLine (global->display, trace->pixmap,trace->gc, 1+x2, y0, 1+x2, y1);
    }

    /* compute the time value and draw it if it fits */
    time_to_string (trace, strg, xtime, FALSE);
    draw_string_fit (trace, textoccupied, x2, trace->ygridtime, global->time_font, strg);
}

void draw_grid (
    Trace	*trace,
    Boolean_t	*textoccupied,
    const Grid	*grid_ptr)		/* Grid information */
{         
    char 	primary_dash[4];	/* Dash pattern */
    int		end_time;
    DTime	xtime;
    Position	y0,y1,y2;

    if (grid_ptr->period < 1) return;

    /* Is the grid too small or invisible?  If so, skip it */
    if ((! grid_ptr->visible) || ((grid_ptr->period * global->res) < MIN_GRID_RES)) {
	return;
    }

    /* create the dash pattern for the vertical grid lines */
    primary_dash[0] = PDASH_HEIGHT;
    primary_dash[1] = global->sighgt - primary_dash[0];

    /* Other coordinates */
    y0 = trace->ystart - Y_GRID_TOP;
    y1 = trace->ystart - Y_DASH_TOP;
    y2 = trace->yend + Y_GRID_BOTTOM;
    
    /* set the line attributes as the specified dash pattern */
    XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0, 0);
    XSetDashes (global->display, trace->gc, 0, primary_dash, 2);
    XSetFont (global->display, trace->gc, global->time_font->fid);

    /* Set color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[grid_ptr->color]);

    /* Start to left of right edge */
    xtime = global->time;
    end_time = global->time + TIME_WIDTH (trace);

    /***** Draw the grid lines */

    switch (grid_ptr->period_auto) {
    case PA_EDGE:
	/* Edges not supported yet */
	break;

    case PA_USER:
    case PA_AUTO:
    default:
	/* Do a regular grid every so many grid units */

	/* Move starting point to the right to hit where a grid line is aligned */
	xtime = ((xtime / grid_ptr->period) * grid_ptr->period) + (grid_ptr->alignment % grid_ptr->period);

	/* If possible, put one grid line inside the signal names */
	if (((grid_ptr->period * global->res) < global->xstart)
	    && (xtime >= global->time)
	    && (xtime >= grid_ptr->period)) {
	    xtime -= grid_ptr->period;
	}

	/* Loop through grid times */
	for ( ; xtime <= end_time; xtime += grid_ptr->period) {
	    draw_grid_line (trace, textoccupied, grid_ptr, xtime, y0, y1, y2);
	}

	break;

    }

    /***** End of drawing */
    
    /* Back to default color */
    XSetForeground (global->display, trace->gc, trace->xcolornums[0]);

    /* reset the line attributes */
    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
}

void draw_grids (
    Trace	*trace)
{           
    Boolean_t	textoccupied[MAXSCREENWIDTH];
    int	grid_num;

    /* Initalize text array */
    memset (textoccupied, FALSE, sizeof (textoccupied));

    /* Draw each grid */
    for (grid_num=0; grid_num<MAXGRIDS; grid_num++) {
	draw_grid (trace, textoccupied, &(trace->grid[grid_num]));
    }
}


void draw_cursors (
    Trace	*trace)
{         
    int		len,end_time;
    int		last_drawn_xright;
    char 	strg[MAXTIMELEN];		/* String value to print out */
    DCursor 	*csr_ptr;			/* Current cursor being printed */
    Position	x1,mid,x2;
    Position	ytop,ybot,ydelta;
    Dimension m_time_height = global->time_font->ascent;

    if (!global->cursor_vis) return;

    XSetFont (global->display, trace->gc, global->time_font->fid);

    end_time = global->time + TIME_WIDTH (trace);

    /* initial the y colors for drawing */
    ytop = trace->ystart - Y_CURSOR_TOP;
    ybot = trace->ycursortimeabs - m_time_height - Y_TEXT_SPACE;
    ydelta = trace->ycursortimerel - m_time_height/2;
    last_drawn_xright = -1;
    
    for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	
	/* check if cursor is on the screen */
	if ((csr_ptr->time >= global->time) && (csr_ptr->time <= end_time)) {
	    
	    /* Change color */
	    XSetForeground (global->display, trace->gc,
			    trace->xcolornums[csr_ptr->color]);
	    if (csr_ptr->type==USER) {
		XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
	    }
	    else {
		XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0,0);
		XSetDashes (global->display, trace->gc, 0, "\002\002", 2);
	    }
	    
	    /* draw the cursor */
	    x1 = TIME_TO_XPOS (csr_ptr->time);
	    XDrawLine (global->display,trace->pixmap,trace->gc,x1,ytop,x1,ybot);
	    
	    /* draw the cursor value */
	    time_to_string (trace, strg, csr_ptr->time, FALSE);
	    len = XTextWidth (global->time_font,strg,strlen (strg));
	    if (len/2 < (x1 - last_drawn_xright)) {
		last_drawn_xright = x1 + len/2 + 2;
		XDrawString (global->display,trace->pixmap,
			     trace->gc, x1-len/2, trace->ycursortimeabs,
			     strg, strlen (strg));
	    }
	    
	    /* if there is a previous visible cursor, draw delta line */
	    if ( csr_ptr->prev && (csr_ptr->prev->time > global->time) ) {
		
		x2 = TIME_TO_XPOS (csr_ptr->prev->time);
		time_to_string (trace, strg, csr_ptr->time - csr_ptr->prev->time, TRUE);
		len = XTextWidth (global->time_font,strg,strlen (strg));
		
		/* write the delta value if it fits */
		XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
		if ( x1 - x2 >= len + 6 ) {
		    /* calculate the mid pt of the segment */
		    mid = x2 + (x1 - x2)/2;
		    XDrawLine (global->display, trace->pixmap, trace->gc,
			       x2, ydelta, mid-len/2-2, ydelta);
		    XDrawLine (global->display, trace->pixmap, trace->gc,
			       mid+len/2+2, ydelta, x1, ydelta);
		    
		    XDrawString (global->display,trace->pixmap,
				 trace->gc, mid-len/2, trace->ycursortimerel,
				 strg, strlen (strg));
		}
		/* or just draw the delta line */
		else {
		    XDrawLine (global->display, trace->pixmap,
			       trace->gc, x2, ydelta, x1, ydelta);
		}
	    }
	}
    }
    /* Reset */
    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Add a line segement to the drawing list */
#define ADD_SEG(xp1,yp1,xp2,yp2) \
  {segs[cnt].x1=(xp1); segs[cnt].y1=(yp1); segs[cnt].x2=(xp2); segs[cnt].y2=(yp2);\
  cnt++;}
#define ADD_SEG_DASH(xp1,yp1,xp2,yp2) \
  {segsd[cntd].x1=(xp1); segsd[cntd].y1=(yp1); segsd[cntd].x2=(xp2); segsd[cntd].y2=(yp2);\
  cntd++;}

/* Draw segments */
#define DRAW_SEGS \
  {if (cnt) XDrawSegments (global->display, trace->pixmap, trace->gc, segs, cnt);\
   if (cntd) {\
      XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0,0);\
      XDrawSegments (global->display, trace->pixmap, trace->gc, segsd, cntd);\
      XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);\
  }\
  cnt=cntd=0;}

/* Set the foreground color, remember the last setting for faster drawing */
/* If there's a color change, draws all segments in the old color before changing */
#define SET_FOREGROUND(colornum) \
  {int col=(colornum); \
    if (col != colornum_last || (cnt+cntd)>MAXCNT) {colornum_last = col;\
       DRAW_SEGS; \
       XSetForeground (global->display, trace->gc, col);}}

void draw_trace (
    Trace	*trace)
{         
    int cnt=0, cntd=0;
    int ymdpt;		/* Y Midpoint of signal, where Z line is */
    int yvalfntloc, ysigfntloc, yspace;
    int xend;
    int xsigrf,xsigrfleft, xsigrfright;		/* X rise-fall time, left edge and right edge */
    int xleft, xright;
    uint_t numprt;			/* Number of signals printed out on screen */
    XSegment segs[MAXCNT+100];		/* Array of line segments to plot */
    XSegment segsd[MAXCNT+100];		/* Array of line segments to plot */
    char strg[MAXVALUELEN];		/* String value to print out */
    const Value_t *cptr,*nptr;		/* Current value pointer and next value pointer */
    const Signal *sig_ptr;		/* Current signal being printed */
    int star_width;			/* Width of '*' character */
    int colornum_last = -1;
    int colornum_sig;
    
    if (DTPRINT_ENTRY) printf ("In draw_trace, xstart=%d\n", global->xstart);
    
    /* don't draw anything if no file is loaded */
    if (!trace->loaded) return;
    /* check for all signals being deleted */
    if (trace->dispsig == NULL) return;
    
    if (DTPRINT_DRAW) printf ("global->res=%f time=%d\n",global->res, global->time);
    
    /* Draw greenbars first... It's pre-printed on the paper :-) */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {
	int yhigh, ylow;
	yhigh = trace->ystart + numprt * global->sighgt;
	ylow = yhigh + global->sighgt - Y_SIG_SPACE;
	if ( (numprt & 1) ^ (trace->numsigstart & 1) ) {
	    SET_FOREGROUND (trace->barcolornum);
	    XFillRectangle (global->display, trace->pixmap, trace->gc,
			    0, yhigh, trace->width - XMARGIN, ylow-yhigh);
	}
    }

    /* Draw grid "under" the signal lines */
    draw_grids (trace);

    /* Overall coordinates */
    star_width = XTextWidth (global->value_font,"*",1);
    xend = trace->width - XMARGIN;
    xsigrf = MAX(1,global->sigrf);
    
    /* Preset dash pattern for STATE_Z's */
    XSetDashes (global->display, trace->gc, 0, "\001\001", 2);

    /* Loop and draw each signal individually */
    for (sig_ptr = trace->dispsig, numprt = 0; sig_ptr && numprt<trace->numsigvis;
	 sig_ptr = sig_ptr->forward, numprt++) {
	int yhigh, ylow;
	/*if (DTPRINT_DRAW) printf ("draw %s\n",sig_ptr->signame);*/

	/* All drawing is from the midpoint of the X in _B32s ( ===X=== ) */

	/* Calculate line position */
	yhigh = trace->ystart + numprt * global->sighgt;
	ylow = yhigh + global->sighgt - Y_SIG_SPACE;
	/* Steal space from Y_SIG_SPACE if we can, down to 2 pixels */
	yspace = global->sighgt - global->signal_font->max_bounds.ascent;
	yspace = MIN( yspace, Y_SIG_SPACE);  /* Bound reasonably */
	yspace = MAX( yspace, 2);  /* Bound reasonably */
	ymdpt = (yhigh + ylow)/2;
	ysigfntloc = ymdpt + (global->signal_font->max_bounds.ascent / 2);
	yvalfntloc = ymdpt + (global->value_font->max_bounds.ascent / 2);

	/* Grab the signal color and font, draw the signal*/
	XSetFont (global->display, trace->gc, global->signal_font->fid);
	colornum_sig = trace->xcolornums[sig_ptr->color];
	SET_FOREGROUND (colornum_sig);
	draw_trace_signame (trace, sig_ptr, ysigfntloc);

	/* Prepare for value drawing */
	XSetFont (global->display, trace->gc, global->value_font->fid);
	
	/* Compute starting points for signal */
	cptr = sig_ptr->cptr;
	cnt = 0;
	xright = global->xstart;

	/* Loop as long as the time and end of trace are in current screen */
	for (; (CPTR_TIME(cptr) != EOT && xright < xend);
	     cptr = nptr) {
	    uint_t dr_mask = 0;	/* Bitmask of how to draw transitions */
#define DR_HIGH		0x01
#define DR_HIGHHIGH	0x02	/* wide high line */
#define DR_LOW		0x04
#define DR_Z		0x08
#define DR_U		0x10
	    int len;
	    int color_value;

	    /* find the next transition */
	    nptr = CPTR_NEXT(cptr);
	    
	    /* if next transition is the end, don't draw */
	    if (CPTR_TIME(nptr) == EOT) break;

	    /* find the x location for the left and right of this segment */
	    xleft = TIME_TO_XPOS (CPTR_TIME(cptr));
	    xleft = MIN (xleft, xend);
	    xleft = MAX (xleft, xright);

	    xright = TIME_TO_XPOS (CPTR_TIME(nptr));
	    xright = MIN (xright, xend);

	    color_value = cptr->siglw.stbits.color;
	    if (cptr->siglw.stbits.allhigh) dr_mask |= DR_HIGHHIGH;
	    switch (cptr->siglw.stbits.state) {
	    case STATE_0:	dr_mask |= DR_LOW; break;
	    case STATE_1:	dr_mask |= DR_HIGHHIGH; break;
	    case STATE_U:	dr_mask |= DR_U; break;
	    case STATE_F32:	dr_mask |= DR_U; break;
	    case STATE_F128:	dr_mask |= DR_U; break;
	    case STATE_Z:	dr_mask |= DR_Z; break;
	    default:		dr_mask |= DR_LOW | DR_HIGH; break;
	    }

	    /*printf ("cptr %s t %d x %d xl %d xr %d xe %d\n",
	      sig_ptr->signame, (int)CPTR_TIME(cptr), (int)TIME_TO_XPOS(CPTR_TIME(cptr)),
	      xleft, xright, xend );
	      printf (" nptr    t %d x %d xl %d xr %d xe %d\n",
	      (int)CPTR_TIME(nptr), (int)TIME_TO_XPOS(CPTR_TIME(nptr)),
	      xleft, xright, xend ); */

	    /* Compress invisible transitions into a glitch */
	    if ((xright - xleft) <= (xsigrf*2)) {
		int xleft_ok_next = MIN(xend, xleft + xsigrf*2);	/* May pass xend.. looks better then truncating */
		const Value_t *nnptr;
		xright = xleft + xsigrf*2;
		/*printf (" glitch %d\n", xleft_ok_next);*/
		/* Too tight for drawing, compress transitions in this space */
		/* Scan forward nptrs till find end of compression point */
		while (1) {
		    nnptr = CPTR_NEXT(nptr);
		    if ((CPTR_TIME(nnptr) == EOT
			 || (TIME_TO_XPOS (CPTR_TIME(nnptr)) > xleft_ok_next))) {
			break;
		    }
		    /*printf ("nnptr    t %d %s x %d xl %d xr %d xe %d  *hunt %x\n",
		      (int)CPTR_TIME(nnptr), (int)TIME_TO_XPOS(CPTR_TIME(nnptr)),
		      xleft, xright, xend, xleft_ok_next );*/
		    /* Build combination image, which is overlay of all values in space */
		    switch (nptr->siglw.stbits.state) {
		    case STATE_0:	dr_mask |= DR_LOW; break;
		    case STATE_1:	dr_mask |= DR_HIGHHIGH; break;
		    case STATE_U:	dr_mask |= DR_U; break;
		    case STATE_F32:	dr_mask |= DR_U; break;
		    case STATE_F128:	dr_mask |= DR_U; break;
		    case STATE_Z:	dr_mask |= DR_Z; break;
		    default:		dr_mask |= DR_LOW | DR_HIGH; break;
		    }
		    color_value = MAX(color_value, nptr->siglw.stbits.color);
		    xright = MAX(xright, TIME_TO_XPOS (CPTR_TIME(nnptr)));
		    nptr = nnptr;
		}
		xright = MIN (xright, xend);
	    }
	    
	    /* Color selection */
	    if (color_value == 0) {SET_FOREGROUND (colornum_sig);}
	    else {SET_FOREGROUND (trace->xcolornums[color_value]);}

	    /* Draw the transition lines */
	    xsigrfleft = (xleft==global->xstart)?0:xsigrf;
	    xsigrfright = (xright==xend)?0:xsigrf;
	    if (dr_mask & DR_U) {
		XPoint pts[10];
		pts[0].x = xleft;		pts[0].y = ymdpt;
		pts[1].x = xleft+xsigrfleft;	pts[1].y = ylow;
		pts[2].x = xright-xsigrfright;	pts[2].y = ylow;
		pts[3].x = xright;		pts[3].y = ymdpt;
		pts[4].x = xright-xsigrfright;	pts[4].y = yhigh;
		pts[5].x = xleft+xsigrfleft;	pts[5].y = yhigh;
		pts[6].x = xleft;		pts[6].y = ymdpt;
		XFillPolygon (global->display, trace->pixmap, trace->gc,
			      pts, 7, Convex, CoordModeOrigin);
		/* Don't need to draw lines, since we illed region */
	    } else {
		if (dr_mask & DR_LOW) {
		    if (xsigrfleft)
			ADD_SEG (xleft, ymdpt, xleft+xsigrfleft, ylow);
		    ADD_SEG (xleft+xsigrfleft, ylow,  xright-xsigrfright, ylow);
		    if (xsigrfright)
			ADD_SEG (xright-xsigrfright, ylow,  xright, ymdpt);
		}
		if (dr_mask & (DR_HIGH | DR_HIGHHIGH)) {
		    if (xsigrfleft)
			ADD_SEG (xleft, ymdpt, xleft+xsigrfleft, yhigh);
		    ADD_SEG (xleft+xsigrfleft, yhigh, xright-xsigrfright, yhigh);
		    if (xsigrfright)
			ADD_SEG (xright-xsigrfright, yhigh, xright, ymdpt);
		    if (dr_mask & DR_HIGHHIGH)
			ADD_SEG (xleft+xsigrfleft, yhigh+1, xright-xsigrfright, yhigh+1);
		}
		if (dr_mask & DR_Z) {
		    ADD_SEG_DASH (xleft, ymdpt, xright, ymdpt);
		}
	    }

	    /* Plot value */
	    if (sig_ptr->bits>1
		&& cptr->siglw.stbits.state != STATE_U
		&& cptr->siglw.stbits.state != STATE_Z
		) {
		uint_t num0 = 0;
		if (cptr->siglw.stbits.state != STATE_0) num0 = cptr->number[0];
		/* Below evaluation left to right important to prevent error */
		if ((cptr->siglw.stbits.state == STATE_B32
		     || cptr->siglw.stbits.state == STATE_0) &&
		    (sig_ptr->decode != NULL) &&
		    (num0 < sig_ptr->decode->numstates) &&
		    (sig_ptr->decode->statename[num0][0] != '\0')) {
		    strcpy (strg, sig_ptr->decode->statename[num0]);
		    len = XTextWidth (global->value_font,strg,strlen(strg));
		    if ( len < xright-xleft-2 ) {
			/* fits, don't try number */
			goto state_plot;
		    }
		}
		if (cptr->siglw.stbits.state != STATE_0) {

		    val_to_string (sig_ptr->radix, strg, cptr, TRUE);

		  state_plot:
		    /* calculate positional parameters */
		    if (star_width < (xright-xleft-2)) {  /* if less definately no space for value */
			int charlen = MIN ((xright-xleft-2) / star_width, strlen(strg));
			char *plotstrg;
			while (1) {
			    plotstrg = strg + strlen(strg) - charlen;
			    if (plotstrg > strg) *plotstrg = '*';
			    len = XTextWidth (global->value_font,plotstrg,charlen);
			    if (len < (xright-xleft-2)) {
				/* Fits */
				break;
			    }
			    charlen--;
			}

			/* write the bus value if it fits */
			if (charlen>0) {
			    int mid = xleft + (int)( (xright-xleft)/2 );
			    XDrawString (global->display, trace->pixmap,
					 trace->gc, mid-len/2, yvalfntloc, plotstrg, charlen );
			}
		    }
		}
	    } /* if bus */
	} /* for cptr */
	
	/* draw the lines, if any to be done */
	DRAW_SEGS;
    } /* for sig_ptr */
    
    if (DTPRINT_DRAW) printf ("Draw done.\n");
    /* draw the cursors if they are visible */
    draw_cursors (trace);

    /* Draw the scroll bar */
    if (DTPRINT_DRAW) printf ("Draw %d.\n",__LINE__);
    draw_hscroll (trace);
    draw_vscroll (trace);
    
    /* Back to default color */
    SET_FOREGROUND (trace->xcolornums[0]);

    if (DTPRINT_DRAW) printf ("Draw done.\n");
} /* End of DRAW */


void draw_trace_signame (
    Trace *trace,
    const Signal *sig_ptr,
    Position y)
{	
    Position x1;
    char *basename;
    Dimension m_sig_width = XTextWidth (global->signal_font,"m",1);
    int truncchars;

    basename = strrchr ((sig_ptr->signame_buspos ?
			 sig_ptr->signame_buspos : sig_ptr->signame),
			trace->hierarchy_separator);
    if (NULL==basename) {
	basename = sig_ptr->signame;
    }

    /* calculate the location to draw the signal name and draw it */
    x1 = global->xstart - XSTART_MARGIN	/* leftmost character position */
	- m_sig_width * strlen (sig_ptr->signame)  /* fit in whole signal */
	- m_sig_width * (global->namepos_base - strlen (basename)) /* extra chars to align basename */
	+ m_sig_width * global->namepos;	/* Scroll position */
    truncchars = global->namepos - (global->namepos_base - strlen (basename));
    if (truncchars < 0) truncchars = 0;

    /*printf ("m_sig_width %d  npos %d nbase %d nhier %d nvis %d x1 %d tc %d\n", m_sig_width, global->namepos,
      global->namepos_base, global->namepos_hier, global->namepos_visible, x1, truncchars);*/

    XDrawString (global->display, trace->pixmap, trace->gc, x1, y,
		 sig_ptr->signame,
		 strlen (sig_ptr->signame) - truncchars);
}

void	draw_update_sigstart ()
    /* Update the starting coodinate of the signals. */
    /* Don't call directly, instead use draw_update_sig_start() it will log a request for the update */
{
    Trace *trace;
    Signal *sig_ptr;
    Dimension widest_hier;
    Dimension widest_base;
    Dimension xstart_sig, xstart_base;
    char *basename;
    Dimension smallest_width;
    Dimension m_sig_width = XTextWidth (global->signal_font,"m",1);

    if (DTPRINT_ENTRY) printf ("In draw_update_sigstart\n");

    /* What's the smallest window */
    smallest_width = global->trace_head->width;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	smallest_width = MIN (smallest_width,trace->width);
    }

    /* Calculate xstart from longest signal name */
    widest_hier = 0;
    widest_base = 0;
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	for (sig_ptr = (Signal *)trace->firstsig; sig_ptr; sig_ptr = sig_ptr->forward) {
	    if (NULL==(basename = strrchr (sig_ptr->signame, '.'))) {
		basename = sig_ptr->signame;
	    }
 	    /* if (DTPRINT) printf ("Signal = '%s'  xstart=%d\n",t1,widest_sig); */
	    widest_hier = MAX(widest_hier, (strlen (sig_ptr->signame) - strlen(basename)));
	    widest_base = MAX(widest_base, (strlen (basename)));
	}
	    
	/* Don't waste more then 1/3 the screen area on signame */
	xstart_sig = XMARGIN + MIN (m_sig_width * (widest_hier+widest_base), (smallest_width/3)) + XSTART_MARGIN;
	xstart_base = XMARGIN + m_sig_width * widest_base + XSTART_MARGIN;
	global->xstart = MAX (xstart_sig, xstart_base);

	/* Remember position of text */
	global->namepos_hier = widest_hier;
	global->namepos_base = widest_base;
	global->namepos_visible = (global->xstart - XSTART_MARGIN) / m_sig_width;
    }
}


void draw_perform ()
{
    Trace		*trace;
    int bgdcolor;

    if (DTPRINT_ENTRY) printf ("In draw_perform %d %d\n", global->redraw_needed, global->trace_head->redraw_needed);

    /* do not unroll this loop, as it will make the refresh across windows */
    /* appear out of sync */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & GRD_ALL) || (trace->redraw_needed & TRD_REDRAW)) {
	    get_geometry (trace);
	}
    }

    /* Update xstart, etc.  They may care about geometry (xstart does at least) */
    draw_update();

    XtSetArg (arglist[0], XmNbackground, &(bgdcolor));
    XtGetValues (global->trace_head->work, arglist, 1);
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & GRD_ALL) || trace->redraw_needed) {
	    XSetForeground (global->display, trace->gc, bgdcolor);
	    XFillRectangle (global->display, trace->pixmap, trace->gc,
			    0,0,XtWidth(trace->work), XtHeight(trace->work));
	    draw_trace (trace);
	}
    }

    /* Install backing store */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if ((global->redraw_needed & GRD_ALL) || trace->redraw_needed) {
	    XCopyArea(global->display,
		      trace->pixmap,
		      XtWindow(trace->work),
		      trace->gc,
		      0, 0,
		      XtWidth(trace->work), XtHeight(trace->work),
		      0, 0);	
	    trace->redraw_needed = FALSE;
	}
    }

    global->redraw_needed = FALSE;
}


void draw_update ()
    /* Update any asyncronous elements that aren't up to date */
    /* Call this before showing the user any data */
{
    /* calculate anything out of date */
    if (global->updates_needed & GUD_SIG_START) {
	draw_update_sigstart();
	global->updates_needed &= ~GUD_SIG_START;
    }
    if (global->updates_needed & GUD_SIG_SEARCH) {
	sig_update_search ();
	global->updates_needed &= ~GUD_SIG_SEARCH;
    }
    if (global->updates_needed & GUD_VAL_SEARCH) {
	val_update_search ();
	global->updates_needed &= ~GUD_VAL_SEARCH;
    }
    if (global->updates_needed & GUD_VAL_STATES) {
	val_states_update ();
	global->updates_needed &= ~GUD_VAL_STATES;
    }
}


/**********************************************************************
 *	Scroll bar hacks (dependent on Motif internals)
 *	Yes, these should just be a new widget class, but it's too late,
 *	it already works.
 **********************************************************************/

/* If set, the scroll pixmap is double buffered ala LessTIF */
/* If clear, modify the scrollbar directly and ignore double buffering (vms???) */
#define BUFFERED_PIXMAP 1

void draw_hscroll (
    Trace *trace)
    /* Draw on the horizontal scroll bar - needs guts of the ScrollBarWidget */
{
    int ymin,ymax,x1,xmin,xmax,slider_xmin,slider_xmax;
    float xscale;
    DCursor *csr_ptr;			/* Current cursor being printed */
    Pixmap pixmap;

    if (DTPRINT_ENTRY) printf ("In draw_hscroll\n");

    if (!trace->loaded || (trace->end_time == trace->start_time)) return;

    /* if this causes a compile error, then change BUFFERED_PIXMAP to 0 */
#if BUFFERED_PIXMAP
    pixmap = ((XmScrollBarRec *)trace->hscroll)->scrollBar.pixmap;
#else
    pixmap = XtWindow (trace->hscroll);
#endif

    /* initial the y colors for drawing */
    xmin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_x;
    xmax = xmin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_width;
    ymin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_y;
    ymax = ymin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_height;
    slider_xmin = ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_x;
    slider_xmax = slider_xmin + ((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_width;
    xscale =
	(float)((XmScrollBarRec *)trace->hscroll)->scrollBar.slider_area_width	/* sc width */
	/ (float)(trace->end_time - trace->start_time);		/* range of times */

    /* Blank area to either side of slider */
    XSetForeground (global->display, trace->gc,
		    ((XmScrollBarRec *)trace->hscroll)->scrollBar.trough_color );
    XFillRectangle (global->display, pixmap, trace->gc,
		   xmin, ymin, slider_xmin - xmin, ymax-ymin);
    XFillRectangle (global->display, pixmap, trace->gc,
		   slider_xmax, ymin, xmax - slider_xmax , ymax-ymin);

    if ( global->cursor_vis ) {
	for (csr_ptr = global->cursor_head; csr_ptr; csr_ptr = csr_ptr->next) {
	    /* draw the cursor */
	    x1 = xmin + xscale * (csr_ptr->time - trace->start_time);

	    /* Don't plot if it would overwrite the slider */
	    if ((x1 < slider_xmin) || (x1 > slider_xmax)) {
		/* Change color */
		XSetForeground (global->display, trace->gc, trace->xcolornums[csr_ptr->color]);

		if (csr_ptr->type==USER) {
		    XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
		}
		else {
		    XSetLineAttributes (global->display, trace->gc, 0, LineOnOffDash, 0,0);
		    XSetDashes (global->display, trace->gc, 0, "\002\002", 2);
		}

		XDrawLine (global->display, pixmap, trace->gc,
			   x1,ymin,x1,ymax);
	    }
	}
	/* Reset */
	XSetLineAttributes (global->display,trace->gc,0,LineSolid,0,0);
    }

#if BUFFERED_PIXMAP
    /* Expose the modified pixmap to the user */
    /* Event and region believed to be don't cares */
    if (draw_orig_scroll_expose) {
	(draw_orig_scroll_expose) (trace->hscroll, NULL, (Region)NULL);
    }
#endif
}

void draw_vscroll (
    Trace *trace)
    /* Draw on the vertical scroll bar - needs guts of the ScrollBarWidget */
{
    int ymin,ymax,y1,xmin,xmax,slider_ymin,slider_ymax;
    uint_t signum;
    float yscale;
    Signal	*sig_ptr;
    Pixmap pixmap;

    if (DTPRINT_ENTRY) printf ("In draw_vscroll\n");

    if (!trace->loaded || !trace->numsig) return;

    /* if this causes a compile error, then change BUFFERED_PIXMAP to 0 */
#if BUFFERED_PIXMAP
    pixmap = ((XmScrollBarRec *)trace->vscroll)->scrollBar.pixmap;
#else
    pixmap = XtWindow (trace->vscroll);
#endif

    /* initial the y colors for drawing */
    xmin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_x;
    xmax = xmin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_width;
    ymin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_y;
    ymax = ymin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_height;
    slider_ymin = ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_y;
    slider_ymax = slider_ymin + ((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_height;

    /*
    if (DTPRINT_DRAW) printf (">>X %d - %d   Y %d - %d  Sli %d - %d\n", 
			xmin,xmax,ymin,ymax,slider_ymin,slider_ymax);
			*/

    /* Blank area to either side of slider */
    XSetForeground (global->display, trace->gc,
		    ((XmScrollBarRec *)trace->vscroll)->scrollBar.trough_color );
    XFillRectangle (global->display, pixmap, trace->gc,
		   xmin, ymin, xmax - xmin, slider_ymin - ymin);
    XFillRectangle (global->display, pixmap, trace->gc,
		   xmin, slider_ymax, xmax - xmin, ymax - slider_ymax);

    yscale =
	(float)((XmScrollBarRec *)trace->vscroll)->scrollBar.slider_area_height	/* sc height */
	    / (float)(trace->numsig);		/* number of signals */

    for (sig_ptr = trace->firstsig, signum=0; sig_ptr; sig_ptr = sig_ptr->forward, signum++) {
	if (sig_ptr->color && ((signum < trace->numsigstart) ||
			       (signum >= ( trace->numsigstart + trace->numsigvis)))) {
	    y1 = ymin + yscale * signum;

	    /* If plotting it would overwrite the slider move it to one side or the other */
	    if (y1 < slider_ymax && y1 > slider_ymin) {
		if ((y1 - slider_ymin) < ((slider_ymax - slider_ymin ) / 2))
		    y1 = slider_ymin - 1;	/* Adjust up */
		else y1 = slider_ymax + 1;	/* Adjust down */
	    }
	    /* Change color */
	    XSetForeground (global->display, trace->gc, trace->xcolornums[sig_ptr->color]);
	    XDrawLine (global->display, pixmap, trace->gc,
		       xmin,y1,xmax,y1);
	}
    }

#if BUFFERED_PIXMAP
    /* Expose the modified pixmap to the user */
    /* Event and region believed to be don't cares */
    if (draw_orig_scroll_expose) {
	(draw_orig_scroll_expose) (trace->vscroll, NULL, (Region)NULL);
    }
#endif
}

void hscroll_expose (Widget w, XEvent *event, Region region)
{
    Trace *trace;
    /* Draw the scroll bar normally */
    (draw_orig_scroll_expose) (w, event, region);
    /* See if it's a special widget of ours */
    for (trace = global->trace_head; trace; trace = trace->next_trace) {
	if (w == trace->vscroll) {
	    draw_vscroll (trace);
	    break;
	}
	else if (w == trace->hscroll) {
	    draw_hscroll (trace);
	    break;
	}
    }
}

void draw_scroll_hook_cb_expose ()
    /* Change the callback for exposing scroll bars to the above hook
       function.  It will look for our special hscroll and vscroll widgets
       and draw appropriately */
{
    XtExposeProc unchanged, changeto;
    changeto = hscroll_expose;
    unchanged = xmScrollBarClassRec.core_class.expose;
    if (unchanged != changeto) {
	draw_orig_scroll_expose = unchanged;
	xmScrollBarClassRec.core_class.expose = changeto;
    } /* else already hooked */
}

