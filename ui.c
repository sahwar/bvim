/* Bvim - BVi IMproved, binary analysis framework
 *
 * Copyright 1996-2004 by Gerhard Buergmann <gerhard@puon.at>
 * Copyright 2011 by Anton Kochkov <anton.kochkov@gmail.com>
 *
 * This file is part of Bvim.
 *
 * Bvim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bvim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bvim.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#include <stdio.h>
#include "bvim.h"
#include "blocks.h"
#include "set.h"
#include "ui.h"
#include "keys.h"
#include "bscript.h"

extern struct MARKERS_ markers[MARK_COUNT];

char tmpbuf[10];
char linbuf[256];

/* ----------------------------------------------------------------------
 *                  Some internal functions
 * ----------------------------------------------------------------------
 */

int outmsg(core_t *core, char *s)
{
	char *poi;
	int cnt = 0;

	move(core->screen.maxy, 0);
	poi = strchr(s, '|');

	if (P(P_TE)) {
		poi = s;
		while (*poi != '\0' && *poi != '@' && *poi != '|') {
			addch(*poi++);
			cnt++;
		}
	} else {
		if (poi)
			poi++;
		else
			poi = s;
		while (*poi) {
			if (*poi == '@')
				addch(' ');
			else
				addch(*poi);
			poi++;
			cnt++;
		}
	}
	return cnt;
}


/* --------------------------------------------------------------------------
 *                         Colors handling
 * --------------------------------------------------------------------------
 */

struct color colors[] = {	/* RGB definitions and default value, if have no support of 256 colors */
	{"background", "bg", 50, 50, 50, COLOR_BLACK},
	{"addresses", "addr", 335, 506, 700, COLOR_BLUE},
	{"hex", "hex", 600, 600, 600, COLOR_MAGENTA},
	{"data", "data", 0, 800, 400, COLOR_GREEN},
	{"error", "err", 999, 350, 0, COLOR_RED},
	{"status", "stat", 255, 255, 255, COLOR_WHITE},
	{"command", "comm", 255, 255, 255, COLOR_WHITE},
	{"window", "win", 0, 800, 900, COLOR_YELLOW},
	{"addrbg", "addrbg", 0, 0, 0, COLOR_CYAN},
	{"", "", 0, 0, 0, 0}	/* end marker */
};

struct {
	short r;
	short g;
	short b;
} original_colors[8];

struct {
	short f;
	short b;
} original_colorpairs[8];

void ColorsSave()
{
	int i;
	for (i = 0; colors[i].fullname[0] != '\0'; i++) {
		color_content(colors[i].short_value, &original_colors[i].r,
			      &original_colors[i].g, &original_colors[i].b);
	}
	for (i = 1; i < 8; i++) {
		pair_content(i, &original_colorpairs[i].f,
			     &original_colorpairs[i].b);
	}
}

void ColorsLoad()
{
	int i;
	for (i = 0; colors[i].fullname[0] != '\0'; i++) {
		init_color(colors[i].short_value, original_colors[i].r,
			   original_colors[i].g, original_colors[i].b);
	}
	for (i = 1; i < 8; i++) {
		init_pair(i, original_colorpairs[i].f,
			  original_colorpairs[i].b);
	}
}

void ui__Colors_Set()
{
	int i;
	if (can_change_color()) {
		for (i = 0; colors[i].fullname[0] != '\0'; i++) {
			if (init_color
			    (colors[i].short_value, C_r(i), C_g(i),
			     C_b(i)) == ERR)
				fprintf(stderr, "Failed to set [%d] color!\n",
					i);
			if (C_s(i) <= 7) {
				init_pair(i + 1, C_s(i), C_s(0));
			} else {
				colors[i].short_value = COLOR_WHITE;
				init_pair(i + 1, C_s(i), C_s(0));
			}
		}
		init_pair(C_AD + 1, C_s(C_AD), COLOR_CYAN);
	} else {		/* if have no support of changing colors */
		for (i = 0; colors[i].fullname[0] != '\0'; i++) {
			if (C_s(i) <= 7) {
				init_pair(i + 1, C_s(i), C_s(0));
			} else {
				colors[i].short_value = COLOR_WHITE;
				init_pair(i + 1, C_s(i), C_s(0));
			}
		}
	}
}

int ui__Color_Set(core_t *core, buf_t *buf, char *arg)
{
	int i;
	char *s;
	for (i = 0; colors[i].fullname[0] != '\0'; i++) {
		s = colors[i].fullname;
		if (strncmp(arg, s, strlen(s)) == 0)
			break;
		s = colors[i].shortname;
		if (strncmp(arg, s, strlen(s)) == 0)
			break;
	}
	if (i == 0) {
		ui__ErrorMsg(core, buf, "Wrong color name!");
		return -1;
	} else {
		colors[i].r = atoi(bvim_substr(arg, strlen(s) + 1, 3));
		colors[i].g = atoi(bvim_substr(arg, strlen(s) + 5, 3));
		colors[i].b = atoi(bvim_substr(arg, strlen(s) + 9, 3));
		/*set_palette(); */
		ui__Colors_Set();
		ui__Screen_Repaint(core, buf);
	}
	return 0;
}


void ui__Init(core_t *core, buf_t *buf)
{
	// Initialization of curses
	initscr();
	if (has_colors() != FALSE) {
		start_color();
		ColorsSave();
		ui__Colors_Set(core, buf);
	}
	attrset(A_NORMAL);
	ui__MainWin_Resize(core, LINES);
}

void ui__MainWin_Resize(core_t *core, int lines_count)
{
	core->screen.maxy = lines_count;
	if (params[P_LI].flags & P_CHANGED)
		core->screen.maxy = P(P_LI);
	core->curbuf->state.scrolly = core->screen.maxy / 2;
	P(P_SS) = core->curbuf->state.scrolly;
	P(P_LI) = core->screen.maxy;
	core->screen.maxy--;

	keypad(stdscr, TRUE);
	scrollok(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();

	core->params.COLUMNS_ADDRESS = 10;
	strcpy(addr_form, "%08lX%c:");

	core->params.COLUMNS_DATA =
	    ((COLS - core->params.COLUMNS_ADDRESS - 1) / 16) * 4;
	P(P_CM) = core->params.COLUMNS_DATA;
	core->screen.maxx =
	    core->params.COLUMNS_DATA * 4 + core->params.COLUMNS_ADDRESS + 1;
	core->params.COLUMNS_HEX = core->params.COLUMNS_DATA * 3;
	status = core->params.COLUMNS_HEX + core->params.COLUMNS_DATA - 17;
	core->curbuf->state.screen = core->params.COLUMNS_DATA * (core->screen.maxy - 1);

	ui__Screen_New(core, core->curbuf);
}

/* ==================== REPL window ======================= */

static WINDOW *repl_win = NULL;

struct repl_t repl;

// TODO: Improve REPL window to make it scrollable
// TODO: Tab autocompletion and commands history in REPL window

/* Check if tools window already exist */
short ui__REPLWin_Exist()
{
	if (repl_win == NULL)
		return 0;
	else
		return 1;
}

/* Show tools window with <lines_count> height */
int ui__REPLWin_Show(core_t *core, buf_t *buf)
{
	if (repl_win == NULL) {
		refresh();
		attron(COLOR_PAIR(C_WN + 1));
		repl_win = newwin(LINES - 1, core->screen.maxx + 1, 0, 0);
		box(repl_win, 0, 0);
		wrefresh(repl_win);
		//attroff(COLOR_PAIR(C_WN + 1));
		return 0;
	} else {
		//ui__REPLWin_Hide();
		//ui__REPLWin_Show();
		return 0;
	}
}

/* Print string in REPL */
int ui__REPLWin_print(core_t *core, buf_t* buf, char *str)
{
	char *saved = NULL;
	char *sptr = NULL;

	if (repl_win != NULL) {
		sptr = strtok_r(str, "\n", &saved);
		while (sptr != NULL) {
			repl.current_x = 1;
			mvwaddstr(repl_win, repl.current_y, repl.current_x, sptr);
			wrefresh(repl_win);
			repl.current_y++;
			sptr = strtok_r(NULL, "\n", &saved);
		}
		return 0;
	} else {
		ui__ErrorMsg(core, buf, "print_repl_window: repl window not exist!");
		return -1;
	}
}

// FIXME: clear repl window
int ui__REPLWin_clear(core_t *core, buf_t* buf)
{
	repl.current_y = 1;
	repl.current_x = 1;
	mvwaddch(repl_win, repl.current_y, repl.current_x, '>');
	wrefresh(repl_win);
	return 0;
}

int ui__REPL_Main(core_t *core, buf_t *buf)
{
	int c;
	int i = 0;
	char p[1024];

	p[0] = '\0';

	signal(SIGINT, jmpproc);
	buf->state.mode = BVI_MODE_REPL;
	ui__REPLWin_Show(core, buf);
	repl.current_y = 1;
	repl.current_x = 1;
	mvwaddch(repl_win, repl.current_y, repl.current_x, '>');
	wrefresh(repl_win);
	do {
		switch(c = vgetc()) {
			case KEY_ENTER:
			case NL:
			case CR:
				// end of line - evaluate
				p[i++] = '\0';
				if (strlen(p) != 0) 
				{
					repl.current_y++;
					repl.current_x = 1;
					bvim_repl_eval(p);
					mvwaddch(repl_win, repl.current_y, repl.current_x, '>');
				}
				p[0] = '\0';
				i = 0;
				break;
			// FIXME: fix moving border from right side when deleting character
			case KEY_BACKSPACE:
				if (repl.current_x > 1) {
					i--;
					wmove(repl_win, repl.current_y, repl.current_x);
					waddch(repl_win, ' ');
					wmove(repl_win, repl.current_y, repl.current_x--);
					p[i] = '\0';
				}
				break;
			case BVI_CTRL('D'):
				break;
			default:
				repl.current_x++;
				mvwaddch(repl_win, repl.current_y, repl.current_x, c);
				p[i++] = c;
				break;
		}
		wrefresh(repl_win);
	} while (c != BVI_CTRL('D'));
	p[0] = '\0';
	ui__REPLWin_Hide(core, buf);
	buf->state.mode = BVI_MODE_EDIT;
	signal(SIGINT, SIG_IGN);
	return 0;
}

int ui__REPLWin_ScrollUp(int lines)
{
	return 0;
}

int ui__REPLWin_ScrollDown(int lines)
{
	return 0;
}

/* Hides tools window */
int ui__REPLWin_Hide(core_t *core, buf_t *buf)
{
	if (repl_win != NULL) {
		//attron(COLOR_PAIR(C_WN + 1));
		wborder(repl_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
		wrefresh(repl_win);
		delwin(repl_win);
		repl_win = NULL;
		attroff(COLOR_PAIR(C_WN + 1));
		ui__Screen_Repaint(core, buf);
		return 0;
	} else {
		ui__ErrorMsg(core, buf, "hide_tools_window: tools window not exist!\n");
		return -1;
	}
}


/* ==================== Tools window ======================= */

WINDOW *tools_win = NULL;

// TODO: Improve tools window to show information from 
// TODO: buffer and make it scrollable

/* Check if tools window already exist */
short ui__ToolWin_Exist(core_t *core, buf_t *buf)
{
	if (tools_win == NULL)
		return 0;
	else
		return 1;
}

/* Show tools window with <lines_count> height */
int ui__ToolWin_Show(core_t *core, buf_t *buf, int lines_count)
{
	if (tools_win == NULL) {
		lines_count = LINES - lines_count - 2;
		ui__MainWin_Resize(core, lines_count);
		refresh();
		attron(COLOR_PAIR(C_WN + 1));
		tools_win = newwin(LINES - lines_count + 2, core->screen.maxx + 1, lines_count, 0);
		box(tools_win, 0, 0);
		wrefresh(tools_win);
		attroff(COLOR_PAIR(C_WN + 1));
		return 0;
	} else {
		ui__ToolWin_Hide(core, buf);
		ui__ToolWin_Show(core, buf, lines_count);
		return 0;
	}
}

/* Print string in tools window in <line> line */
int ui__ToolWin_Print(core_t *core, buf_t *buf, char *str, int line)
{
	if (tools_win != NULL) {
		attron(COLOR_PAIR(C_WN + 1));
		mvwaddstr(tools_win, line, 1, str);
		wrefresh(tools_win);
		attroff(COLOR_PAIR(C_WN + 1));
		return 0;
	} else {
		ui__ErrorMsg(core, buf, "print_tools_window: tools window not exist!\n");
		return -1;
	}
}

int ui__ToolWin_ScrollUp(int lines)
{
	return 0;
}

int ui__ToolWin_ScrollDown(int lines)
{
	return 0;
}

/* Hides tools window */
int ui__ToolWin_Hide(core_t *core, buf_t *buf)
{
	if (tools_win != NULL) {
		ui__MainWin_Resize(core, LINES);
		attron(COLOR_PAIR(C_WN + 1));
		wborder(tools_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
		wrefresh(tools_win);
		delwin(tools_win);
		attroff(COLOR_PAIR(C_WN + 1));
		ui__Screen_Repaint(core, buf);
		tools_win = NULL;
		return 0;
	} else {
		ui__ErrorMsg(core, buf, "hide_tools_window: tools window not exist!\n");
		return -1;
	}
}

/* -----------------------------------------------------------------
 *                    Highlighting engine 
 * -----------------------------------------------------------------
 */

hl_link hl = NULL;
char* tmp_mem;

/* ========== Abstracts ========== */

int HighlightAdd(struct hl_item i)
{
	hl_link t = NULL;
	t = (hl_link)malloc(sizeof(*t));
	if (t != NULL) {
		t->item = i;
		if (hl != NULL) {
			t->next = hl->next;
			hl->next = t;
		} else {
			hl = t;
			hl->next = NULL;
		}
	} else {
		return -1;
	}
	return 0;
}

int ui__BlockHighlightAdd(struct block_item *i)
{
	struct hl_item t;
	t.id = i->id;
	HighlightAdd(t);
	return 0;
}

/* Iterator of any functions on highlights list,
 * where result - expected result for function
 * All blocks are unique */
int HighlightIterator(int (*(func))(), int result)
{
	hl_link t;
	t = hl;
	while (t != NULL)
	{
		if ((*(func))(&(t->item)) == result) {
			return 0;
		}
		t = t->next;
	}
	return -1;
}

struct hl_item* HighlightGetByID(unsigned int id) {
	hl_link t;
	
	t = hl;
	
	while (t != NULL)
	{
		if (t->item.id == id) return &(t->item);
		t = t->next;
	}
	return NULL;
}


/* =========== Implementation ========= */

int highlight_block(core_t *core, buf_t* buf, struct block_item *tmp_blk) {
	int i = 0;
	struct hl_item *thl;
// TODO: Implement recursive handling for highlights
	if (tmp_blk->hl_toggle == 1) {
		thl = HighlightGetByID(tmp_blk->id);
		thl->id = tmp_blk->id;
		thl->toggle = 1;
		thl->hex_start = 0;
		thl->dat_start = 0;
		thl->palette = tmp_blk->palette;
		if (thl->flg == 1) {
			thl->hex_end = core->params.COLUMNS_DATA * 3;
			thl->dat_end = core->params.COLUMNS_DATA;
		} else {
			thl->hex_end = 0;
			thl->dat_end = 0;
		}
		for (i = 0; i < core->params.COLUMNS_DATA * 3; i += 3) {
			if (((long)(tmp_mem - buf->mem + (i / 3)) == tmp_blk->pos_start) & (thl->flg != 1)) {
				thl->hex_start = i;
				thl->dat_start = i / 3;
				thl->hex_end = core->params.COLUMNS_DATA * 3;
				thl->dat_end = core->params.COLUMNS_DATA;
				thl->flg = 1;
			} else
			    if (((long)(tmp_mem - buf->mem + (i / 3)) < tmp_blk->pos_end) & ((long)(tmp_mem - buf->mem + (i / 3)) > tmp_blk->pos_start) & (thl->flg != 1)) {
				thl->hex_start = i;
				thl->dat_start = i / 3;
				thl->hex_end = core->params.COLUMNS_DATA * 3;
				thl->dat_end = core->params.COLUMNS_DATA;
				thl->flg = 1;
			} else
			    if (((long)(tmp_mem - buf->mem + (i / 3)) == tmp_blk->pos_end) & (thl->flg == 1)) {
				thl->hex_end = i + 2;
				thl->dat_end = i / 3 + 1;
				thl->flg = 0;
			} else
			    if (((long)(tmp_mem - buf->mem + (i / 3)) > tmp_blk->pos_end)) {
				thl->flg = 0;
			}
		}
	}
	return 0;
}

/* ========================== lines print interface ====================== */

void printcolorline(int y, int x, int palette, char *string)
{
	palette++;
	attron(COLOR_PAIR(palette));
	mvaddstr(y, x, string);
	attroff(COLOR_PAIR(palette));
}

void printcolorline_hexhl(int y, int x, int base_palette, char *string)
{
	hl_link t;
	
	printcolorline(y, x, base_palette, string);
	t = hl;
	while (t != NULL) {
		if ((t->item.hex_start < t->item.hex_end) & (t->item.toggle == 1)) {
			attron(COLOR_PAIR(t->item.palette) | A_STANDOUT | A_BOLD);
			mvaddstr(y, x + t->item.hex_start, bvim_substr(string, t->item.hex_start, t->item.hex_end - t->item.hex_start));
			attroff(COLOR_PAIR(t->item.palette) | A_STANDOUT | A_BOLD);
		}
		t = t->next;
	}
}

void printcolorline_dathl(int y, int x, int base_palette, char *string)
{
	hl_link t;

	printcolorline(y, x, base_palette, string);
	t = hl;
	while (t != NULL) {
		if ((t->item.dat_start < t->item.dat_end) & (t->item.toggle == 1)) {
			attron(COLOR_PAIR(t->item.palette) | A_STANDOUT | A_BOLD);
			mvaddstr(y, x + t->item.dat_start, bvim_substr(string, t->item.dat_start, t->item.dat_end - t->item.dat_start));
			attroff(COLOR_PAIR(t->item.palette) | A_STANDOUT | A_BOLD);
		}
		t = t->next;
	}
}

void ui__Line_Print(core_t *core, buf_t *buf, PTR mempos, int scpos)
{
	char hl_msg[256];
	unsigned int k = 0;
	unsigned int print_pos;
	int nxtpos = 0;
	unsigned char Zeichen;
	long address;
	char marker = ' ';

	hl_msg[0] = '\0';
	*linbuf = '\0';

	if (mempos > buf->maxpos) {
		strcpy(linbuf, "~         ");
	} else {
		address = (long)(mempos - buf->mem + P(P_OF));
		while (markers[k].address != 0) {
			if (markers[k].address == address) {
				marker = markers[k].marker;
				break;
			}
			k++;
		}
		sprintf(linbuf, addr_form, address, marker);
		*string = '\0';
	}
	strcat(linbuf, " ");
	/* load color from C(C_AD) */
	printcolorline(scpos, 0, C_AD, linbuf);
	nxtpos = 11;
	*linbuf = '\0';

	tmp_mem = mempos;
	// check if buf has blocks!
	if (buf->blocks != NULL) {
		blocks__Iterator(buf, highlight_block, 1);
	}
	mempos = tmp_mem;

	for (print_pos = 0; print_pos < core->params.COLUMNS_DATA; print_pos++) {
		if (mempos + print_pos >= buf->maxpos) {
			sprintf(tmpbuf, "   ");
			Zeichen = ' ';
		} else {
			Zeichen = *(mempos + print_pos);
			sprintf(tmpbuf, "%02X ", Zeichen);
		}
		strcat(linbuf, tmpbuf);
		if (isprint(Zeichen))
			*(string + print_pos) = Zeichen;
		else
			*(string + print_pos) = '.';
	}
	*(string + core->params.COLUMNS_DATA) = '\0';

	/* load color from C(C_HX) */
	strcat(linbuf, "|");
	printcolorline_hexhl(scpos, nxtpos, C_HX, linbuf);

	/* strcat(linbuf, string); */
	nxtpos += strlen(linbuf);
	/* load color from C(C_DT) */
	printcolorline_dathl(scpos, nxtpos, C_DT, string);
}

/* TODO: add hex-data folding feature */

/* displays a line on screen
 * at state.pagepos + line y
 */
int ui__lineout(core_t *core, buf_t *buf)
{
	off_t Address;

	Address = buf->state.pagepos - buf->mem + y * core->params.COLUMNS_DATA;
	ui__Line_Print(core, buf, buf->mem + Address, y);
	move(y, x);
	/*if (k != 0) 
	   y = k;
	 */
	return (0);
}

void ui__Screen_New(core_t *core, buf_t *buf)
{
	buf->state.screen = core->params.COLUMNS_DATA * (core->screen.maxy - 1);
	clear();
	ui__Screen_Repaint(core, buf);
}

int ui__line(core_t *core, buf_t *buf, int line, int address)
{
	ui__Line_Print(core, buf, buf->mem + address, line);
	move(line, x);
	return (0);
}

off_t buf_address;

int fold_block(struct block_item tmp_blk) {
/*
	if ((tmp_blk.folding == 1) & (tmp_blk.pos_start < Address) & (tmp_blk.pos_end > Address))
	{
		fold_start = tmp_blk.pos_start;
		fold_end = tmp_blk.pos_end;
		state.pagepos += (fold_end - fold_start) / core.params.COLUMNS_DATA;
		// Address = state.pagepos - core.editor.mem + y * core.params.COLUMNS_DATA;
		return 0;
	}
*/
	return 0;
}

// Redraw screen
void ui__Screen_Repaint(core_t *core, buf_t *buf)
{
	int save_y;
	int fold_start = 0;
	int fold_end = 0;
	int i = 0;

	save_y = y;
	for (y = 0; y < core->screen.maxy; y++) {
		buf_address = buf->state.pagepos - buf->mem + y * core->params.COLUMNS_DATA;
		ui__line(core, buf, y, buf_address);
	}
	y = save_y;
	if (repl_win != NULL)
		wrefresh(repl_win);
	if (tools_win != NULL)
		wrefresh(tools_win);
}

void ui__clearstr(core_t *core, buf_t *buf)
{
	int n;

	move(core->screen.maxy, 0);
	for (n = 0; n < core->screen.maxx; n++)
		addch(' ');
	move(core->screen.maxy, 0);
}

// Displays an error message
void ui__ErrorMsg(core_t *core, buf_t* buf, char *s)
{
	int cnt;

	if (P(P_EB))
		beep();
	ui__clearstr(core, buf);
	attron(COLOR_PAIR(C_ER + 1));
	/* attrset(A_REVERSE); */
	cnt = outmsg(core, s);
	/* attrset(A_NORMAL); */
	attroff(COLOR_PAIR(C_ER + 1));
	if (cnt >= (core->screen.maxx - 25)) {	/* 25 = status */
		addch('\n');
		wait_return(core, buf, TRUE);
	}
}

// System error message
void ui__SystemErrorMsg(core_t *core, buf_t* buf, char* s)
{
	char string[256];

#ifdef HAVE_STRERROR
	sprintf(string, "%s: %s", s, strerror(errno));
#else
	sprintf(string, "%s: %s", s, sys_errlist[errno]);
#endif

	ui__ErrorMsg(core, buf, string);
}

/*** displays mode if showmode set *****/
void ui__smsg(core_t *core, buf_t *buf, char* s)
{
	if (P(P_MO)) {
		ui__StatusMsg(core, buf, s);
		setcur();
	}
}

// Display simple message window
void ui__MsgWin_Show(core_t *core, buf_t* buf, char* s, int height, int width)
{
	WINDOW *msg_win;
	int starty = (LINES - height) / 2;	/* Calculating for a center placement */
	int startx = (COLS - width) / 2;	/* of the window                */
	int ch;
	refresh();
	attron(COLOR_PAIR(C_WN + 1));
	msg_win = newwin(height, width, starty, startx);
	box(msg_win, 0, 0);
	wrefresh(msg_win);
	mvwaddstr(msg_win, 1, 2, s);
	wrefresh(msg_win);
	while ((ch = getch()) == -1) {
	}
	wborder(msg_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	wrefresh(msg_win);
	delwin(msg_win);
	attroff(COLOR_PAIR(C_WN + 1));
	if (buf->state.mode != BVI_MODE_REPL)
		ui__Screen_Repaint(core, buf);
	/*
	else
		wrefresh(repl_win);
	*/
}

// Display message in the status line
void ui__StatusMsg(core_t *core, buf_t *buf, char* msg)
{
	ui__clearstr(core, buf);
	if (outmsg(core, msg) >= (core->screen.maxx - 25)) {	/* 25 = status */
		addch('\n');
		wait_return(core, buf, TRUE);
	}
}


int ui__Destroy()
{
	endwin();
	ColorsLoad();
	return 0;
}
