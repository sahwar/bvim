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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <sys/stat.h>
#include <setjmp.h>

#include "data.h"
#include "patchlevel.h"
#include "../config.h"

#if HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define SKIP_WHITE  while(*cmd!='\0'&&isspace(*cmd))cmd++;

#ifdef DEBUG
extern FILE *debug_fp;
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#endif

struct MARKERS_ {
	long address;
	char marker;		/* Usually we use '+' character, but can be another */
};

#define BVI_VISUAL_SELECTION_ID 1999

extern char addr_form[];
extern char pattern[];
extern char rep_buf[];

extern int x, y;

extern int filemode;
extern int edits, new;
extern int addr_flag;
extern int ignore_case, magic;
extern int screen, status;
extern PTR undo_start;
extern PTR current_start;

extern PTR start_addr;
extern PTR end_addr;

extern char *name, cmdstr[];
extern off_t filesize, memsize;
extern PTR markbuf[];
extern PTR last_motion;
extern off_t undo_count;
extern off_t yanked;
extern off_t undosize;
extern char *copyright, *notfound;
extern char *terminal;
extern char *undo_buf;
extern char *yank_buf;
extern int repl_count;
extern char string[];
extern char *shell;
extern char *poi;
extern int smode;
extern int again;
extern int block_flag;
extern off_t block_begin, block_end, block_size;

#ifndef S_ISDIR			/* POSIX 1003.1 file type tests. */
#define	S_ISDIR(m)	((m & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	((m & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	((m & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	((m & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	((m & 0170000) == 0010000)	/* fifo */
#endif

/* ================= Debug utilities ================ */

void bvim_error(core_t *core, buf_t *buf, char* fmt, ...);
void bvim_info(core_t *core, buf_t *buf, char* fmt, ...);
void bvim_debug(core_t *core, buf_t *buf, char* fmt, ...);

/* ================= Exports ================ */

char *bvim_substr(const char *, size_t, size_t);

off_t alloc_buf(core_t*, buf_t*, off_t, char **);
off_t yd_addr(core_t*, buf_t*);
off_t range(core_t *, buf_t*, int);
void do_dot(void);
void do_exit(core_t*);
void do_shell(core_t*, buf_t*);
void do_undo(core_t *core, buf_t *buf);
void do_tilde(core_t *core, buf_t *buf, off_t);
void trunc_cur(core_t *core, buf_t *buf);
void do_back(core_t*, buf_t*, off_t, PTR);
void do_ins_chg(core_t*, buf_t*, PTR, char*, int);
void do_mark(buf_t*, int, PTR);
void badcmd(char *);
void movebyte(core_t*, buf_t*);
void do_over(core_t *core, buf_t *buf, PTR, off_t, PTR);
void do_put(core_t *, buf_t *, PTR, off_t, PTR);
void jmpproc(int);
void printline(PTR, int);

int addfile(core_t*, buf_t*, char *);
int bregexec(core_t*, buf_t*, PTR, char *);
int doecmd(core_t*, buf_t*, char *, int);
int do_append(core_t*, buf_t*, int, char *);
int do_delete(core_t*, buf_t*, off_t, PTR);
int doset(core_t*, char *);
int do_substitution(core_t*, buf_t*, int, char *, PTR, PTR);
int hexchar(void);

PTR searching(core_t*, buf_t*, int, char *, PTR, PTR, int);
PTR wordsearch(core_t*, buf_t*, PTR, char);
PTR backsearch(core_t*, buf_t*, PTR, char);
PTR fsearch(core_t*, buf_t*, PTR, PTR, char *);
PTR rsearch(core_t*, buf_t*, PTR, PTR, char *);
PTR end_word(core_t*, buf_t*, PTR);
PTR calc_addr(core_t*, buf_t*, char **, PTR);
PTR do_ft(core_t*, buf_t*, int, int);
char *patcpy(char *, char *, char);
void setpage(core_t*, buf_t*, PTR);

void usage(void);
void bvim_init(core_t*, char *);
void statpos(core_t*, buf_t*);

void setcur(void);

void showparms(core_t*, int);
void toggle(core_t*, buf_t*);
void scrolldown(core_t*, buf_t*, int);
void scrollup(core_t*, buf_t*, int);
void fileinfo(core_t*, buf_t*, char *);
void clear_marks(buf_t*);

void quit(core_t*, buf_t *);

void do_z(core_t*, buf_t*, int);
void stuffin(char *);
off_t edit(core_t*, buf_t*, int);
off_t load(core_t*, buf_t*, char *);
off_t calc_size(char *);
int ascii_comp(char *, char *);
int hex_comp(char *, char *);
int cur_forw(core_t*, buf_t*, int);
int cur_back(core_t*, buf_t*);
int lineout(void);
int save(core_t*, buf_t*, char *, int);
int at_least(char *, char *, int);
int vgetc(void);
int xpos(core_t*, buf_t*);
int enlarge(core_t*, buf_t*, off_t);
int getcmdstr(core_t*, char *, int);
int read_rc(core_t*, char*);
int wait_return(core_t*, buf_t *buf, int);
int get_cursor_position();

int read_history(core_t*, char*);
void record_cmd(core_t*, char*);

/* ========= Event handlers ======== */

int handler__goto_HEX();
int handler__goto_ASCII();
int handler__toggle();
int handler__tilda();
int handler__goto_home();
int handler__M();
int handler__L();
int handler__goto_left();
int handler__goto_right();
int handler__goto_up();
int handler__goto_EOL();
int handler__goto_down();
int handler__line();
int handler__toolwin_toggle();
int handler__cmdstring();
int handler__previous_page();
int handler__scrolldown();
int handler__scrollup();
int handler__linescroll_down();
int handler__nextpage();
int handler__fileinfo();
int handler__screen_redraw();
int handler__linescroll_up();
int handler__luarepl();
int handler__toggle_selection();
int handler__append_mode();
int handler__backsearch();
int handler__setpage();
int handler__doft1();
int handler__doft2();
int handler__doft3_F();
int handler__doft3_f();
int handler__doft3_t();
int handler__doft3_T();
int handler__goto1();
int handler__goto2();
int handler__search_string1();
int handler__search_string2();
int handler__search_string3();
int handler__search_string4();
int handler__search_next();
int handler__mark();
int handler__goto_mark();
int handler__trunc();
int handler__overwrite();
int handler__paste();
int handler__redo();
int handler__undo();
int handler__visual();
int handler__wordsearch();
int handler__yank();
int handler__doz();
int handler__exit();
int handler__stuffin();
int handler__insert();
int handler__s();
int handler__append2();
int handler__insert2();
int handler__paste2();
int handler__c_or_d();
int handler__x();
int handler__x2();

