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
** $Header: //depot/main/gfx/lib/glu/libtess/dict.h#5 $
*/

#ifndef __dict_list_h_
#define __dict_list_h_

/* Use #define's so that another heap implementation can use this one */

#define DictKey		DictListKey
#define Dict		DictList
#define DictNode	DictListNode

#define dictNewDict(frame,leq)		__gl_dictListNewDict(frame,leq)
#define dictDeleteDict(dict)		__gl_dictListDeleteDict(dict)

#define dictSearch(dict,key)		__gl_dictListSearch(dict,key)
#define dictInsert(dict,key)		__gl_dictListInsert(dict,key)
#define dictInsertBefore(dict,node,key)	__gl_dictListInsertBefore(dict,node,key)
#define dictDelete(dict,node)		__gl_dictListDelete(dict,node)

#define dictKey(n)			__gl_dictListKey(n)
#define dictSucc(n)			__gl_dictListSucc(n)
#define dictPred(n)			__gl_dictListPred(n)
#define dictMin(d)			__gl_dictListMin(d)
#define dictMax(d)			__gl_dictListMax(d)



typedef void *DictKey;
typedef struct Dict Dict;
typedef struct DictNode DictNode;

Dict		*dictNewDict(
			void *frame,
			int (*leq)(void *frame, DictKey key1, DictKey key2) );
			
void		dictDeleteDict( Dict *dict );

/* Search returns the node with the smallest key greater than or equal
 * to the given key.  If there is no such key, returns a node whose
 * key is NULL.  Similarly, Succ(Max(d)) has a NULL key, etc.
 */
DictNode	*dictSearch( Dict *dict, DictKey key );
DictNode	*dictInsertBefore( Dict *dict, DictNode *node, DictKey key );
void		dictDelete( Dict *dict, DictNode *node );

#define		__gl_dictListKey(n)	((n)->key)
#define		__gl_dictListSucc(n)	((n)->next)
#define		__gl_dictListPred(n)	((n)->prev)
#define		__gl_dictListMin(d)	((d)->head.next)
#define		__gl_dictListMax(d)	((d)->head.prev)
#define	       __gl_dictListInsert(d,k) (dictInsertBefore((d),&(d)->head,(k)))


/*** Private data structures ***/

struct DictNode {
  DictKey	key;
  DictNode	*next;
  DictNode	*prev;
};

struct Dict {
  DictNode	head;
  void		*frame;
  int		(*leq)(void *frame, DictKey key1, DictKey key2);
};

#endif
