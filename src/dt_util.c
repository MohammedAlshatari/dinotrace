/******************************************************************************
 *
 * Filename:
 *     dt_util.c
 *
 * Subsystem:
 *     Dinotrace
 *
 * Version:
 *     Dinotrace V4.0
 *
 * Author:
 *     Allen Gallotta
 *
 * Abstract:
 *
 * Modification History:
 *     AAG	14-Aug-90	Original Version
 *     AAG	22-Aug-90	Base Level V4.1
 *
 */


#include stdio
#include descrip

#include <decw$include/DECwDwtApplProg.h>
#include <decw$include/Xlib.h>

#include "dinotrace.h"
#include "callbacks.h"



void
cancel_all_events(w,ptr,cb)
Widget			w;
DISPLAY_SB		*ptr;
DwtAnyCallbackStruct	*cb;
{
    if (DTPRINT) printf("In cancel_all_events - ptr=%d\n",ptr);

    /* remove all events */
    remove_all_events(ptr);

    /* unmanage any widgets left around */
    if ( ptr->signal.customize != NULL )
	XtUnmanageChild(ptr->signal.customize);

    return;
}

void
remove_all_events(ptr)
DISPLAY_SB		*ptr;
{
    if (DTPRINT) printf("In remove_all_events - ptr=%d\n",ptr);

    /* remove all possible events due to cursor options */ 
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,add_cursor,ptr);
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,move_cursor,ptr);
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,delete_cursor,ptr);

    /* remove all possible events due to grid options */ 
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,align_grid,ptr);

    /* remove all possible events due to signal options */ 
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,add_signal,ptr);
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,move_signal,ptr);
    XtRemoveEventHandler(ptr->work,ButtonPressMask,TRUE,delete_signal,ptr);

    return;
}

new_time(ptr)
DISPLAY_SB *ptr;
{
int		i,inc;
short		*pshort;
SIGNAL_SB	*sig_ptr;

    if ( ptr->time > ptr->end_time - (int)((ptr->width-XMARGIN-ptr->xstart)/ptr->res) )
    {
        if (DTPRINT) printf("At end of trace...\n");
        ptr->time = ptr->end_time - (int)((ptr->width-XMARGIN-ptr->xstart)/ptr->res);
    }

    if ( ptr->time < ptr->start_time )
    {
        if (DTPRINT) printf("At beginning of trace...\n");
        ptr->time = ptr->start_time;
    }

    pshort = ptr->bus;
    sig_ptr = ptr->sig.forward;

    for (i=0; i<ptr->numsig-ptr->numsigdel; i++)
    {

if (DTPRINT)
  printf("next time=%d\n",(*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time);

        if ( ptr->time >= (*(SIGNAL_LW *)(sig_ptr->cptr)).time )
	{
            while (ptr->time>(*(SIGNAL_LW *)((sig_ptr->cptr)+sig_ptr->inc)).time)
	    {
		(sig_ptr->cptr)+=sig_ptr->inc;
	    }
	}
	else
        {
            while (ptr->time < (*(SIGNAL_LW *)(sig_ptr->cptr)).time)
	    {
                (sig_ptr->cptr)-=sig_ptr->inc;
            }
        }

/* increment to next signal */

    sig_ptr = sig_ptr->forward;
    }
}

get_geometry( ptr )
DISPLAY_SB	*ptr;
{
    int		temp,x,y,width,height,dret,max_y;

    XGetGeometry( XtDisplay(toplevel), XtWindow(ptr->work), &dret,
         &x, &y, &width, &height, &dret, &dret);

    ptr->width = width;
    ptr->height = height;

    /* calulate the number of signals possibly visible on the screen */
    max_y = (int)((ptr->height-ptr->ystart)/ptr->sighgt);
    ptr->numsigvis = MIN(ptr->numsig - ptr->numsigdel,max_y);

    /* if there are cursors showing, subtract one to make room for cursor */
    if (ptr->numcursors > 0 && ptr->cursor_vis && ptr->numsigvis > 1)
	ptr->numsigvis--;

    XtSetArg(arglist[0], DwtNminValue, ptr->start_time);
    XtSetArg(arglist[1], DwtNmaxValue, ptr->end_time);
    XtSetArg(arglist[2], DwtNvalue, ptr->time);
    XtSetArg(arglist[3], DwtNinc,
	(int)((ptr->width-ptr->xstart)/ptr->res/ptr->pageinc));
    XtSetArg(arglist[4], DwtNshown,
	(int)((ptr->width-ptr->xstart)/ptr->res));
    XtSetValues(ptr->hscroll, arglist, 5);

    XtSetArg(arglist[0], DwtNminValue, 0);
    XtSetArg(arglist[1], DwtNmaxValue, ptr->numsig);
    XtSetArg(arglist[2], DwtNvalue, 0);
    XtSetArg(arglist[3], DwtNinc, 1);
    XtSetArg(arglist[4], DwtNshown,
	(int)((ptr->height-ptr->ystart)/ptr->sighgt));
    XtSetValues(ptr->vscroll, arglist, 5);

    if (DTPRINT)
    {
	printf("In get_geometry\n");
	printf("x=%d y=%d width=%d height=%d\n",x,y,width,height);
    }
}

static DwtCallback fil_ok_cb[2] =
{
    {cb_fil_ok,  NULL},
    {NULL,       NULL}
};

static DwtCallback fil_can_cb[2] =
{
    {cb_fil_can, NULL},
    {NULL,       NULL}
};

void 
get_file_name( ptr )
DISPLAY_SB	*ptr;
{
    if (DTPRINT) printf("In get_file_name ptr=%d\n",ptr);

    if (!ptr->fileselect)
    {
	XtSetArg(arglist[0], DwtNactivateCallback, fil_ok_cb);
	XtSetArg(arglist[1], DwtNcancelCallback, fil_can_cb);
	fil_ok_cb[0].tag = ptr;
	fil_can_cb[0].tag = ptr;
	XtSetArg(arglist[2], DwtNdirMask, DwtLatin1String("*.tra") );
	XtSetArg(arglist[3], DwtNdirectionRToL, TRUE);
	XtSetArg(arglist[4], DwtNdefaultPosition, TRUE);
	XtSetArg(arglist[5], DwtNcols, 50);
	ptr->fileselect = DwtFileSelectionCreate( ptr->main, "", arglist, 6);

	XSync(ptr->disp,0);
    }

    XtManageChild(ptr->fileselect);

    return;
}

void
cb_fil_ok(widget, ptr, reason)
Widget		widget;
DISPLAY_SB	*ptr;
DwtFileSelectionCallbackStruct *reason;
{
    int d;
    char *tmp, title[100];
    DwtCompStringContext context;

    if (DTPRINT) printf("In cb_fil_ok ptr=%d\n",ptr);

    DwtInitGetSegment(&context, reason->value);
    DwtGetNextSegment(&context, &tmp, &d, &d, &d, &d);
    sprintf(ptr->filename,"%s",tmp);

    if (DTPRINT) printf("In cb_fil_ok Filename=%s\n",ptr->filename);

    /* remove the file select widget */
    XtUnmanageChild(ptr->fileselect);

    /* read in the DECSIM trace file */
    read_DECSIM(ptr);

    /* change the name on title bar to filename */
    sprintf(title,DTVERSION);
    strcat(title," - ");
    strcat(title,ptr->filename);
    XtSetArg(arglist[0], DwtNtitle, title);
    XtSetValues(toplevel,arglist,1);

    /* clear the number of deleted signals */
    ptr->numsigdel = 0;

    /* draw the screen */
    get_geometry(ptr);
    XClearWindow(ptr->disp,ptr->wind);
    draw(ptr);
    drawsig(ptr);
}

void
cb_fil_can( widget, ptr, reason )
Widget		widget;
DISPLAY_SB	*ptr;
DwtFileSelectionCallbackStruct *reason;
{
    if (DTPRINT) printf("In cb_fil_can ptr=%d\n",ptr);

    /* remove the file select widget */
    XtUnmanageChild( ptr->fileselect );
}


Widget popup_wid, ok_wid, can_wid, text_wid, label_wid;

void get_data_popup(), ok(), can(), hit_return();

static char io_trans_table[100]; 

    static XtActionsRec io_action_table[] =
{
        {"hit_return", (XtActionProc)hit_return},
        {NULL,        NULL}
};

    XtTranslations io_trans_parsed;

static DwtCallback ok_cb[2]=
{
        {ok,    NULL},
        {NULL,  NULL}
};

static DwtCallback can_cb[2]=
{
        {can,   NULL},
        {NULL,  NULL}
};
/*
static DwtCallback get_data_cb[2]=
{
        {get_data_popup,   NULL},
        {NULL,  NULL}
};
*/

void 
get_data_popup(ptr,string,type)
DISPLAY_SB	*ptr;
char		*string;
int		type;
{
    static	MANAGED=FALSE;

    if (DTPRINT) 
	printf("In get_data ptr=%d string=%s type=%d\n",ptr,string,type);

    /* parse the translation table - do this each time for diff types */
    sprintf(io_trans_table,"<KeyPress>0xff0d: hit_return(%d,%d)",ptr,type); 
    XtAddActions(io_action_table, 1);
    io_trans_parsed = XtParseTranslationTable(io_trans_table);

    if (!MANAGED)
    {
	/* create the dialog box popup window */
	XtSetArg(arglist[0], DwtNstyle, DwtModal );
	XtSetArg(arglist[1], DwtNwidth, DIALOG_WIDTH);
	XtSetArg(arglist[2], DwtNheight, DIALOG_HEIGHT);
	XtSetArg(arglist[3], DwtNdefaultPosition, TRUE);
	popup_wid = DwtDialogBoxPopupCreate(ptr->work,"", arglist, 4);

	/* create the label widget */
	XtSetArg(arglist[0], DwtNx, 5 );
	XtSetArg(arglist[1], DwtNy, 5 );
	label_wid = DwtLabelCreate( popup_wid,"",arglist,2);
	XtManageChild( label_wid );

	/* create the OK push button */
	XtSetArg(arglist[0], DwtNlabel, DwtLatin1String(" OK ") );
	XtSetArg(arglist[1], DwtNx, 5);
	XtSetArg(arglist[2], DwtNy, 40);
	XtSetArg(arglist[3], DwtNsensitive, TRUE);
	ok_wid = DwtPushButtonCreate(popup_wid,"",arglist,4);
	XtManageChild( ok_wid );
    
	/* create the CANCEL push button */
	XtManageChild(can_wid=DwtPushButton(popup_wid,"",30,40,
               DwtLatin1String(" Cancel "), can_cb, NULL));

	/* only need to create the widget once - now just manage/unmanage */
	MANAGED = TRUE;
    }

    XtSetArg(arglist[0], DwtNlabel, DwtLatin1String(string));
    XtSetValues(label_wid,arglist,1);

    XtSetArg(arglist[0], DwtNtextMergeTranslations, io_trans_parsed );
    XtSetValues(popup_wid,arglist,1);

    ok_cb[0].tag = type;
    XtSetArg(arglist[0], DwtNactivateCallback, ok_cb );
    XtSetValues(ok_wid,arglist,1);

    /* create the simple text widget */
    XtSetArg(arglist[0], DwtNrows, 1 );
    XtSetArg(arglist[1], DwtNcols, 10 );
    XtSetArg(arglist[2], DwtNx, 5 );
    XtSetArg(arglist[3], DwtNy, 20 );
    text_wid = DwtSTextCreate( popup_wid, "", arglist, 4);
    XtManageChild( text_wid );

    /* manage the popup window */
    XtManageChild(popup_wid);
}

void
ok(w, type, reason)
Widget		w;
int		type;
DwtFileSelectionCallbackStruct *reason;
{
    int		num;
    char	*list[2],string1[10],string2[10];

    if (DTPRINT) printf("In ok_cb type=%d\n",type);

    sprintf(string1,"%d",curptr);
    sprintf(string2,"%d",type);

    list[0] = string1;
    list[1] = string2;

    num = 2;
 
    hit_return(w,NULL,&list,&num);

    return;
}

void
can( widget, tag, reason )
Widget		widget;
DISPLAY_SB	*tag;
DwtFileSelectionCallbackStruct *reason;
{

    if (DTPRINT) printf("In can_cb\n");
    XtUnmanageChild(popup_wid);

}

void
hit_return(w,event,params,numparams)
Widget		w;
XEvent		*event;
char		**params;
int		*numparams;
{
    char	*tmp,data[20],string[20]={"\0"};
    DISPLAY_SB	*ptr;
    int		type,tempi;
    float	tempf;

    if (DTPRINT) printf("in hit return!\n");

    if (DTPRINT) printf("*numparams=%d\n",*numparams);

    /* obtain the ptr */
    ptr = atoi(params[0]);
    if (DTPRINT) printf("ptr=%d\n",ptr);

    /* obtain the type */
    type = atoi(params[1]);
    if (DTPRINT) printf("type=%d\n",type);

    /* do stuff depending on type of data */
    switch(type)
    {
	case IO_GRIDRES:
	    /* get the data and store in display structure */
	    strcpy(data, tmp = DwtSTextGetString(text_wid));
	    tempi = atoi(data);
	    if (tempi > 0)
		ptr->grid_res = tempi;
	    else
	    {
		sprintf(message,"Value %d out of range",tempi);
		dino_message_ack(ptr,message);
		return;
	    }
    
	    /* validate the geometry */
	    get_geometry( ptr );
	    break;

	case IO_RES:
	    /* get the data and store in display structure */
	    strcpy(data, tmp = DwtSTextGetString(text_wid) );
	    tempi = atoi(data);
	    if (tempi <= 0)
	    {
		sprintf(message,"Value %d out of range",tempi);
		dino_message_ack(ptr,message);
		return;
	    }
	    else
	    {
		ptr->res = ((float)(ptr->width-ptr->xstart))/(float)tempi;
    
	        /* change the resolution string on display */
	 	strcat(string,"Res=");
		strcat(string,data);
		strcat(string," ns");
		XtSetArg(arglist[0],DwtNlabel, DwtLatin1String(string));
		XtSetValues(ptr->command.reschg_but,arglist,1);
	    }
	    /* validate the geometry */
	    get_geometry( ptr );
	    break;

	case IO_READCUSTOM:
	    /* get the data and store in display structure */
	    strcpy(data, tmp = DwtSTextGetString(text_wid) );
	    printf("Read Custom File: %s\n",data);
	    break;

	case IO_SAVECUSTOM:
	    /* get the data and store in display structure */
	    strcpy(data, tmp = DwtSTextGetString(text_wid) );
	    printf("Save Custom File: %s\n",data);
	    break;

	default:
	    printf("Error - bad type %d\n",type);
    }

    /* unmanage the popup window */
    XtUnmanageChild(popup_wid);

    /* destroy the text widget */
    XtDestroyWidget(text_wid);

    /* redraw the screen */
    XClearWindow(ptr->disp, ptr->wind);
    draw(ptr);
    drawsig(ptr);
}


void 
dino_message_info(ptr,msg)
DISPLAY_SB	*ptr;
char		*msg;
{
    short	wx,wy,wh,ww;
    static	MAPPED=FALSE;
    static Widget message;

    if (DTPRINT) printf("In dino_message_info msg=%s\n",msg);

    /* display message at terminal */
    printf("DINO_MESSAGE_INFO: %s\n",msg);

    /* display message in main_wid window */


    return;
}

void 
dino_message_ack(ptr,msg)
DISPLAY_SB	*ptr;
char		*msg;
{
    short	wx,wy,wh,ww;
    static	MAPPED=FALSE;
    static Widget message;

    if (DTPRINT) printf("In dino_message_ack msg=%s\n",msg);

    /* create the widget if it hasn't already been */
    if (!MAPPED)
    {
	cb_arglist[0].proc = message_ack;
	cb_arglist[0].tag = message;
	cb_arglist[1].proc = NULL;
	XtSetArg(arglist[0], DwtNyesCallback, cb_arglist);
	XtSetArg(arglist[1], DwtNdefaultPosition, TRUE);
	message = DwtMessageBoxCreate(ptr->work, "", arglist, 2);
	MAPPED=TRUE;
    }

    /* change the label value and location */
    XtSetArg(arglist[0], DwtNlabel, DwtLatin1String(msg));
    XtSetValues(message,arglist,1);

    /* manage the widget */
    XtManageChild(message);
}

void
message_ack(widget, tag, reason )
Widget		widget;
Widget		*tag;
DwtFileSelectionCallbackStruct *reason;
{
    if (DTPRINT) printf("In message_ack\n");

    /* unmanage the widget */
    XtUnmanageChild(widget);
}

/******************************************************************************
 *
 *			Dinotrace Debugging Routines
 *
 *****************************************************************************/

DINO_NUMBER_TO_VALUE(num)
int	num;
{
    switch(num)
    {
	case STATE_1:
		printf("1");
		return;

	case STATE_0:
		printf("0");
		return;

	case STATE_U:
		printf("U");
		return;

	case STATE_Z:
		printf("Z");
		return;

	case STATE_B32:
		printf("B32");
		return;

	default:
		printf("UNKNOWN VALUE");
		return;
    }
}

void
print_sig_names(w,ptr)
Widget		w;
DISPLAY_SB	*ptr;
{
    int		i;
    short	*pbus;

    printf("There are %d signals in this trace.\n",ptr->numsig);
    printf("Num:\tType:\tName:\n");

    for (i=0; i< ptr->numsig; i++)
    {
	printf("%d)\t\t%s\n",i,(*(ptr->signame)).array[i]);
    }

    printf("Bus array: ptr->bus[]=");
    pbus = ptr->bus;
    for (i=0; i< ptr->numsig; i++)
    {
	printf("%d|",*(pbus+i));
    }

    
}

void
print_all_traces(w,ptr)
Widget		w;
DISPLAY_SB	*ptr;
{
    printf("In print_all_traces.\n");
}

void
print_screen_traces(w,ptr)
Widget		w;
DISPLAY_SB	*ptr;
{
    int		i,adj,num;
    SIGNAL_SB	*sig_ptr;
    SIGNAL_LW	*cptr;

    printf("There are %d signals currently visible.\n",ptr->numsigvis);
    printf("Which signal do you wish to view (0-%d): ",ptr->numsigvis-1);
    scanf("%d",&num);
    if ( num < 0 || num > ptr->numsigvis-1 )
    {
	printf("Illegal Value.\n");
	return;
    }

    adj = ptr->time * ptr->res - ptr->xstart;
    printf("Adjustment value is %d\n",adj);

    sig_ptr = ptr->startsig;
    for (i=0; i<num; i++)
    {
	sig_ptr = sig_ptr->forward;	
    }

    cptr = sig_ptr->cptr;

    printf("Signal %s starts at %d with a value of ",sig_ptr->signame,cptr->time);
    DINO_NUMBER_TO_VALUE(cptr->state);
    printf("\n");

    while( cptr->time != 0x1FFFFFFF &&
	   (cptr->time*ptr->res-adj) < ptr->width - XMARGIN)
    {
	DINO_NUMBER_TO_VALUE(cptr->state);
	printf(" at time %d ns",cptr->time);

	if ( cptr->state >= STATE_B32 )
	{
	    switch(sig_ptr->type)
	    {
		case STATE_B32:
		    cptr++;
		    printf(" with a value of %x\n",*((unsigned int *)cptr));
		    break;

		case STATE_B64:
		    cptr++;
		    printf(" with a value of %x ",*((unsigned int *)cptr));
		    cptr++;
		    printf("%x\n",*((unsigned int *)cptr));
		    break;

		case STATE_B96:
		    cptr++;
		    printf(" with a value of %x ",*((unsigned int *)cptr));
		    cptr++;
		    printf("%x ",*((unsigned int *)cptr));
		    cptr++;
		    printf("%x\n",*((unsigned int *)cptr));
		    break;

		default:
		    printf("Error: Bad sig_ptr->type=%d\n",sig_ptr->type);
		    break;
	    }
	    cptr++;
	}
	else
	{
	    printf("\n");
	    cptr += sig_ptr->inc;
	}
    }
}
