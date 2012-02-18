/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/
/*
** Author: Eric Veach, July 1994.
**
** $Date$ $Revision$
** $Header: //depot/main/gfx/lib/glu/libtess/dict.c#5 $
*/

#include <stddef.h>
#include "ThirdParty/glu/libtess/dict-list.h"
#include "ThirdParty/glu/libtess/memalloc.h"

/* really __gl_dictListNewDict */
Dict *dictNewDict( void *frame,
		   int (*leq)(void *frame, DictKey key1, DictKey key2) )
{
  Dict *dict = (Dict *) memAlloc( sizeof( Dict ));
  DictNode *head;

  if (dict == NULL) return NULL;

  head = &dict->head;

  head->key = NULL;
  head->next = head;
  head->prev = head;

  dict->frame = frame;
  dict->leq = leq;

  return dict;
}

/* really __gl_dictListDeleteDict */
void dictDeleteDict( Dict *dict )
{
  DictNode *node;

  for( node = dict->head.next; node != &dict->head; node = node->next ) {
    memFree( node );
  }
  memFree( dict );
}

/* really __gl_dictListInsertBefore */
DictNode *dictInsertBefore( Dict *dict, DictNode *node, DictKey key )
{
  DictNode *newNode;

  do {
    node = node->prev;
  } while( node->key != NULL && ! (*dict->leq)(dict->frame, node->key, key));

  newNode = (DictNode *) memAlloc( sizeof( DictNode ));
  if (newNode == NULL) return NULL;

  newNode->key = key;
  newNode->next = node->next;
  node->next->prev = newNode;
  newNode->prev = node;
  node->next = newNode;

  return newNode;
}

/* really __gl_dictListDelete */
void dictDelete( Dict *dict, DictNode *node ) /*ARGSUSED*/
{
  node->next->prev = node->prev;
  node->prev->next = node->next;
  memFree( node );
}

/* really __gl_dictListSearch */
DictNode *dictSearch( Dict *dict, DictKey key )
{
  DictNode *node = &dict->head;

  do {
    node = node->next;
  } while( node->key != NULL && ! (*dict->leq)(dict->frame, key, node->key));

  return node;
}
