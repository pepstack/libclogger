/****************************************************************************
* RTree.H
*
* MODULE:       R-Tree library
*
* AUTHOR(S):    Antonin Guttman - original code
*               Daniel Green (green@superliminal.com) - major clean-up
*                               and implementation of bounding spheres
*
* PURPOSE:      Multi Dimensional Index
*
* COPYRIGHT:    (C) 2001 by the GRASS Development Team
*
*               This program is free software under the GNU General Public
*               License (>=v2). Read the file COPYING that comes with GRASS
*               for details.
*
* LAST MODIFY:
*    2007-11:   Initially created by ZhangLiang (cheungmine@gmail.com)
*    2008-5:    Multiple threads safe supports
*    2021-8:    Update codes and renames
*****************************************************************************/
#ifndef  RTREE_H_API_INCLUDED
#define  RTREE_H_API_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTREE_ASSERT
  #define  RTREE_ASSERT(x)  assert(x)
#endif

#define RTREE_PTR_TO_INT(pv)      ((int) (uintptr_t) (void*) (pv))
#define RTREE_INT_TO_PTR(iv)      ((void*) (uintptr_t) (int) (iv))

#define RTREE_PTR_TO_INT64(pv)    ((int64_t) (uint64_t) (void*) (pv))
#define RTREE_INT64_TO_PTR(iv)    ((void*) (uint64_t) (int64_t) (iv))


/* RTREE_PAGESZ is normally the natural page size of the machine */
#ifndef RTREE_PAGESZ
  #define  RTREE_PAGESZ  4096
#endif

/* number of dimensions: 2, 3, 4, 5, 6 ... 20 */
#ifndef RTREE_DIMS
  #define  RTREE_DIMS    2
#endif

#define  RTREE_SIDES   ((RTREE_DIMS)*2)

#ifndef RTREE_REAL
  #define RTREE_REAL   double
#endif

/* do not change the following */
#define  RTREE_TRUE	   1
#define  RTREE_FALSE   0

#if RTREE_DIMS > 20
  #error "not enough precomputed sphere volumes"
#endif

/**
 * public type definitions
 */
typedef struct _RTreeNode*  RTREE_NODE;
typedef struct _RTreeRoot*  RTREE_ROOT;


typedef struct
{
    /* [Min1, Min2, ..., MinN, Max1, Max2, ..., MaxN] */
    RTREE_REAL bound[RTREE_SIDES];
} RTREE_MBR;


typedef struct
{
    RTREE_MBR	mbr;

    /* mbr id */
    RTREE_NODE child;
} RTREE_BRANCH;


/**
 * Initialize a rectangle to have all 0 coordinates.
 */
void RTreeMbrInit(RTREE_MBR *mbr);


/**
 * Return a mbr whose first low side is higher than its opposite side -
 *   interpreted as an undefined mbr.
 */
RTREE_MBR RTreeMbrNull(void);


/**
 * Print out the data for a rectangle.
 */
void RTreeMbrPrint(RTREE_MBR *mbr, int depth);


/**
 * Calculate the 2-dimensional area of a rectangle
 */
RTREE_REAL RTreeMbrArea(RTREE_MBR *mbr);


/**
 * Calculate the n-dimensional volume of a rectangle
 */
RTREE_REAL RTreeMbrVolume(RTREE_MBR *mbr);


/**
 * Calculate the n-dimensional volume of the bounding sphere of a rectangle
 * The exact volume of the bounding sphere for the given RTREE_MBR.
 */
RTREE_REAL RTreeMbrSpherVolume(RTREE_MBR *mbr);


/**
 * Calculate the n-dimensional surface area of a rectangle
 */
RTREE_REAL RTreeMbrSurfaceArea(RTREE_MBR *mbr);


/**
 * Combine two rectangles, make one that includes both.
 */
RTREE_MBR RTreeMbrCombine(RTREE_MBR *rc1, RTREE_MBR *rc2);


/**
 * Decide whether two rectangles overlap.
 */
int RTreeMbrOverlapped(const RTREE_MBR *rc1, const RTREE_MBR *rc2);


/**
 * Decide whether rectangle r is contained in rectangle s.
 */
int RTreeMbrContained(const RTREE_MBR *r, const RTREE_MBR *s);


/**
 * Split a node.
 * Divides the nodes branches and the extra one between two nodes.
 * Old node is one of the new ones, and one really new one is created.
 * Tries more than one method for choosing a partition, uses best result.
 */
void RTreeSplitNode(RTREE_ROOT root, RTREE_NODE node, RTREE_BRANCH *br, RTREE_NODE *new_node);


/**
 * Initialize a RTreeNode structure.
 */
void RTreeInitNode( RTREE_NODE node );


/**
 * Make a new node and initialize to have all branch cells empty.
 */
RTREE_NODE RTreeNewNode(void);


void RTreeFreeNode( RTREE_NODE node );


/**
 * Print out the data in a node.
 */
void RTreePrintNode(RTREE_NODE node, int depth);


/**
 * Find the smallest rectangle that includes all rectangles in branches of a node.
 */
RTREE_MBR RTreeNodeCover(RTREE_NODE node);


/**
 * Pick a branch.  Pick the one that will need the smallest increase
 * in area to accomodate the new rectangle.  This will result in the
 * least total area for the covering rectangles in the current node.
 * In case of a tie, pick the one which was smaller before, to get
 * the best resolution when searching.
 */
int RTreePickBranch(RTREE_MBR *mbr, RTREE_NODE node);


/**
 * Add a branch to a node.  Split the node if necessary.
 * Returns 0 if node not split.  Old node updated.
 * Returns 1 if node split, sets *new_node to address of new node.
 * Old node updated, becomes one of two.
 */
int RTreeAddBranch(RTREE_ROOT root, RTREE_BRANCH *br, RTREE_NODE node, RTREE_NODE *new_node);


/**
 * Disconnect a dependent node.
 */
void RTreeCutBranch(RTREE_NODE node, int i);


/**
 * Destroy (free) node recursively.
 */
void RTreeDelNode(RTREE_NODE node);


/**
 * Create a new rtree index, empty. Consists of a single node.
 */
RTREE_ROOT RTreeCreate(int (*RTreeSearchCallback)(void*, void*));


/**
 * Destroy a rtree root must be a root of rtree. Free all memory.
 */
void RTreeDestroy(RTREE_ROOT root);


/**
 * Search in an index tree for all data rectangles that overlap the argument rectangle.
 * Return the number of qualifying data rects.
 */
int RTreeSearchMbr(RTREE_ROOT root, const RTREE_MBR *mbr, int (*searchCallback)(void*, void*), void* cbarg);


/**
 * Insert a data rectangle into an index structure.
 * RTreeInsertRect provides for splitting the root;
 * returns 1 if root was split, 0 if it was not.
 * The level argument specifies the number of steps up from the leaf
 * level to insert; e.g. a data rectangle goes in at level = 0.
 * _RTreeInsertRect does the recursion.
 */
int RTreeInsertMbr(RTREE_ROOT root, RTREE_MBR *data_mbr, void* data_id, int level);


/**
 * Delete a data rectangle from an index structure.
 * Pass in a pointer to a RTREE_MBR, the tid of the record, ptr to ptr to root node.
 * Returns 1 if record not found, 0 if success.
 * RTreeDeleteRect provides for eliminating the root.
 */
int RTreeDropMbr(RTREE_ROOT root, RTREE_MBR *data_mbr, void* data_id);

#ifdef __cplusplus
}
#endif

#endif /* RTREE_H_API_INCLUDED */
