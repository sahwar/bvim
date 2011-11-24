#include <stdlib.h>
#include <string.h>
#include "bvim.h"
#include "blocks.h"
#include "bscript.h"

extern core_t core;

/* =============== Blocks list/buffer abstractions =============== */

/* Preallocate some memory for future blocks allocation */
int InitBlocksList(int N)
{
	int i = 0;
	core.blocks = (block_link)malloc((N + 1)*(sizeof *(core.blocks)));
	for (i = 0; i < N + 1; i++) {
		core.blocks[i].next = &(core.blocks[i + 1]);
	}
	core.blocks[N].next = NULL;
	return 0;
}

int BlockAdd(struct block_item i)
{
	block_link t = NULL;
	t = (block_link)malloc(sizeof(*t));
	if (t != NULL) {
		t->item = i;
		if (core.blocks != NULL) {
			t->next = core.blocks->next;
			core.blocks->next = t;
		} else {
			core.blocks = t;
			core.blocks->next = NULL;
		}
	} else {
		return -1;
	}
	return 0;
}

void BlockFree(block_link x)
{
	BlockInsertNext(core.blocks, x);
}

void BlockInsertNext(block_link x, block_link t)
{
	t->next = x->next;
	x->next = t;
}

block_link BlockDeleteNext(block_link x)
{
	block_link t =  NULL;
	if (x != NULL) {
		t = x->next;
		x->next = t->next;
	}
	return t;
}

block_link BlockNext(block_link x)
{
	return x->next;
}

struct block_item BlockGet(block_link x)
{
	return x->item;
}

/* ============= Blocks interface/handlers ================ */

/* Iterator of any functions on blocks list,
 * where result - expected result for function
 * All blocks are unique */
int blocks__Iterator(int (*(func))(), int result)
{
	block_link t;
	t = core.blocks;
	while (t != NULL)
	{
		if ((*(func))(&(t->item)) == result) {
			return 0;
		}
		t = t->next;
	}
	return -1;
}

int blocks__Add(struct block_item b) {
	
	BlockAdd(b);

	return 0;
}

int blocks__DelByID(int id) {
	return 0;
}

int blocks__DelByName(char* name) {
	return 0;
}

struct block_item* blocks__GetByID(unsigned int id) {
	block_link t;
	
	t = core.blocks;
	
	while (t != NULL)
	{
		if (t->item.id == id) return &(t->item);
		t = t->next;
	}
	return NULL;
}

struct block_item* blocks__GetByName(char* name) {
	block_link t;

	t = core.blocks;

	while (t != NULL)
	{
		if (!strcmp(t->item.name, name)) return &(t->item);
		t = t->next;
	}
	return NULL;
}

int blocks__Init()
{
	//InitBlocksList(10);
	return 0;
}

int blocks__Destroy()
{
	block_link t;
	t = core.blocks;
	while (t != NULL)
	{
		free(t);
		t = t->next;
	}
	return 0;
}

