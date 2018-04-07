/* Updating of data structures for redisplay.

Copyright (C) 1985-1988, 1993-1995, 1997-2018 Free Software Foundation,
Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.  */

#include "config.h"

#include "sysstdio.h"
#include <stdlib.h>
#include <unistd.h>

#include "lisp.h"
#include "ptr-bounds.h"
#include "termchar.h"
/* cm.h must come after dispextern.h on Windows.  */
#include "dispextern.h"
#include "cm.h"
#include "buffer.h"
#include "keyboard.h"
#include "frame.h"
#include "termhooks.h"
#include "window.h"
#include "commands.h"
#include "disptab.h"
#include "blockinput.h"
#include "syssignal.h"
#include "systime.h"
#include "tparam.h"
#include "xwidget.h"

#ifdef HAVE_WINDOW_SYSTEM
#include TERM_HEADER
#endif /* HAVE_WINDOW_SYSTEM */

#include <errno.h>

#include <fpending.h>

#ifdef WINDOWSNT
#include "w32.h"
#endif

/* Structure to pass dimensions around.  Used for character bounding
   boxes, glyph matrix dimensions and alike.  */

struct dim
{
  int width;
  int height;
};


/* Function prototypes.  */

static void update_frame_line (struct frame *, int, bool);
static int required_matrix_height (struct window *);
static int required_matrix_width (struct window *);
static void increment_row_positions (struct glyph_row *, ptrdiff_t, ptrdiff_t);
static void build_frame_matrix_from_window_tree (struct glyph_matrix *,
                                                 struct window *);
static void build_frame_matrix_from_leaf_window (struct glyph_matrix *,
                                                 struct window *);
static void adjust_decode_mode_spec_buffer (struct frame *);
static void fill_up_glyph_row_with_spaces (struct glyph_row *);
static void clear_window_matrices (struct window *, bool);
static void fill_up_glyph_row_area_with_spaces (struct glyph_row *, int);
static int scrolling_window (struct window *, bool);
static bool update_window_line (struct window *, int, bool *);
static void mirror_make_current (struct window *, int);
#ifdef GLYPH_DEBUG
static void check_matrix_pointers (struct glyph_matrix *,
                                   struct glyph_matrix *);
#endif
static void mirror_line_dance (struct window *, int, int, int *, char *);
static bool update_window_tree (struct window *, bool);
static bool update_window (struct window *, bool);
static bool update_frame_1 (struct frame *, bool, bool, bool, bool);
static bool scrolling (struct frame *);
static void set_window_cursor_after_update (struct window *);
static void adjust_frame_glyphs_for_window_redisplay (struct frame *);
static void adjust_frame_glyphs_for_frame_redisplay (struct frame *);
static void set_window_update_flags (struct window *w, bool on_p);

/* True means last display completed.  False means it was preempted.  */

bool display_completed;

/* True means SIGWINCH happened when not safe.  */

static bool delayed_size_change;

/* A glyph for a space.  */

struct glyph space_glyph;

#if defined GLYPH_DEBUG && defined ENABLE_CHECKING

/* Counts of allocated structures.  These counts serve to diagnose
   memory leaks and double frees.  */

static int glyph_matrix_count;
static int glyph_pool_count;

#endif /* GLYPH_DEBUG and ENABLE_CHECKING */

/* If non-null, the frame whose frame matrices are manipulated.  If
   null, window matrices are worked on.  */

static struct frame *frame_matrix_frame;

/* Convert vpos and hpos from frame to window and vice versa.
   This may only be used for terminal frames.  */

#ifdef GLYPH_DEBUG

static int window_to_frame_vpos (struct window *, int);
static int window_to_frame_hpos (struct window *, int);
#define WINDOW_TO_FRAME_VPOS(W, VPOS) window_to_frame_vpos ((W), (VPOS))
#define WINDOW_TO_FRAME_HPOS(W, HPOS) window_to_frame_hpos ((W), (HPOS))

/* One element of the ring buffer containing redisplay history
   information.  */

struct redisplay_history
{
  char trace[512 + 100];
};

/* The size of the history buffer.  */

#define REDISPLAY_HISTORY_SIZE	30

/* The redisplay history buffer.  */

static struct redisplay_history redisplay_history[REDISPLAY_HISTORY_SIZE];

/* Next free entry in redisplay_history.  */

static int history_idx;

/* A tick that's incremented each time something is added to the
   history.  */

static uprintmax_t history_tick;

/* Add to the redisplay history how window W has been displayed.
   MSG is a trace containing the information how W's glyph matrix
   has been constructed.  PAUSED_P means that the update
   has been interrupted for pending input.  */

static void
add_window_display_history (struct window *w, const char *msg, bool paused_p)
{
  char *buf;
  void *ptr = w;

  if (history_idx >= REDISPLAY_HISTORY_SIZE)
    history_idx = 0;
  buf = redisplay_history[history_idx].trace;
  ++history_idx;

  snprintf (buf, sizeof redisplay_history[0].trace,
	    "%"pMu": window %p (%s)%s\n%s",
	    history_tick++,
	    ptr,
	    ((BUFFERP (w->contents)
	      && STRINGP (BVAR (XBUFFER (w->contents), name)))
	     ? SSDATA (BVAR (XBUFFER (w->contents), name))
	     : "???"),
	    paused_p ? " ***paused***" : "",
	    msg);
}


/* Add to the redisplay history that frame F has been displayed.
   PAUSED_P means that the update has been interrupted for
   pending input.  */

static void
add_frame_display_history (struct frame *f, bool paused_p)
{
  char *buf;
  void *ptr = f;

  if (history_idx >= REDISPLAY_HISTORY_SIZE)
    history_idx = 0;
  buf = redisplay_history[history_idx].trace;
  ++history_idx;

  sprintf (buf, "%"pMu": update frame %p%s",
	   history_tick++,
	   ptr, paused_p ? " ***paused***" : "");
}


DEFUN ("dump-redisplay-history", Fdump_redisplay_history,
       Sdump_redisplay_history, 0, 0, "",
       doc: /* Dump redisplay history to stderr.  */)
  (void)
{
  int i;

  for (i = history_idx - 1; i != history_idx; --i)
    {
      if (i < 0)
	i = REDISPLAY_HISTORY_SIZE - 1;
      fprintf (stderr, "%s\n", redisplay_history[i].trace);
    }

  return Qnil;
}


#else /* not GLYPH_DEBUG */

#define WINDOW_TO_FRAME_VPOS(W, VPOS) ((VPOS) + WINDOW_TOP_EDGE_LINE (W))
#define WINDOW_TO_FRAME_HPOS(W, HPOS) ((HPOS) + WINDOW_LEFT_EDGE_COL (W))

#endif /* GLYPH_DEBUG */


#if defined PROFILING && !HAVE___EXECUTABLE_START
/* This function comes first in the Emacs executable and is used only
   to estimate the text start for profiling.  */
void
__executable_start (void)
{
  emacs_abort ();
}
#endif

/***********************************************************************
			    Glyph Matrices
 ***********************************************************************/

/* Allocate and return a glyph_matrix structure.  POOL is the glyph
   pool from which memory for the matrix should be allocated, or null
   for window-based redisplay where no glyph pools are used.  The
   member `pool' of the glyph matrix structure returned is set to
   POOL, the structure is otherwise zeroed.  */

static struct glyph_matrix *
new_glyph_matrix (struct glyph_pool *pool)
{
  struct glyph_matrix *result = xzalloc (sizeof *result);

#if defined GLYPH_DEBUG && defined ENABLE_CHECKING
  /* Increment number of allocated matrices.  This count is used
     to detect memory leaks.  */
  ++glyph_matrix_count;
#endif

  /* Set pool and return.  */
  result->pool = pool;
  return result;
}


/* Free glyph matrix MATRIX.  Passing in a null MATRIX is allowed.

   If GLYPH_DEBUG and ENABLE_CHECKING are in effect, the global counter
   glyph_matrix_count is decremented when a matrix is freed.  If the count
   gets negative, more structures were freed than allocated, i.e. one matrix
   was freed more than once or a bogus pointer was passed to this function.

   If MATRIX->pool is null, this means that the matrix manages its own
   glyph memory---this is done for matrices on X frames.  Freeing the
   matrix also frees the glyph memory in this case.  */

static void
free_glyph_matrix (struct glyph_matrix *matrix)
{
  if (matrix)
    {
      int i;

#if defined GLYPH_DEBUG && defined ENABLE_CHECKING
      /* Detect the case that more matrices are freed than were
	 allocated.  */
      --glyph_matrix_count;
      eassert (glyph_matrix_count >= 0);
#endif

      /* Free glyph memory if MATRIX owns it.  */
      if (matrix->pool == NULL)
	for (i = 0; i < matrix->rows_allocated; ++i)
	  xfree (matrix->rows[i].glyphs[LEFT_MARGIN_AREA]);

      /* Free row structures and the matrix itself.  */
      xfree (matrix->rows);
      xfree (matrix);
    }
}


/* Return the number of glyphs to reserve for a marginal area of
   window W.  TOTAL_GLYPHS is the number of glyphs in a complete
   display line of window W.  MARGIN gives the width of the marginal
   area in canonical character units.  */

static int
margin_glyphs_to_reserve (struct window *w, int total_glyphs, int margin)
{
  if (margin > 0)
    {
      int width = w->total_cols;
      double d = max (0, margin);
      d = min (width / 2 - 1, d);
      /* Since MARGIN is positive, we cannot possibly have less than
	 one glyph for the marginal area.  */
      return max (1, (int) ((double) total_glyphs / width * d));
    }
  return 0;
}

/* Return true if ROW's hash value is correct.
   Optimized away if ENABLE_CHECKING is not defined.  */

static bool
verify_row_hash (struct glyph_row *row)
{
  return row->hash == row_hash (row);
}

/* Adjust glyph matrix MATRIX on window W or on a frame to changed
   window sizes.

   W is null if the function is called for a frame glyph matrix.
   Otherwise it is the window MATRIX is a member of.  X and Y are the
   indices of the first column and row of MATRIX within the frame
   matrix, if such a matrix exists.  They are zero for purely
   window-based redisplay.  DIM is the needed size of the matrix.

   In window-based redisplay, where no frame matrices exist, glyph
   matrices manage their own glyph storage.  Otherwise, they allocate
   storage from a common frame glyph pool which can be found in
   MATRIX->pool.

   The reason for this memory management strategy is to avoid complete
   frame redraws if possible.  When we allocate from a common pool, a
   change of the location or size of a sub-matrix within the pool
   requires a complete redisplay of the frame because we cannot easily
   make sure that the current matrices of all windows still agree with
   what is displayed on the screen.  While this is usually fast, it
   leads to screen flickering.  */

static void
adjust_glyph_matrix (struct window *w, struct glyph_matrix *matrix, int x, int y, struct dim dim)
{
  int i;
  int new_rows;
  bool marginal_areas_changed_p = 0;
  bool header_line_changed_p = 0;
  bool header_line_p = 0;
  int left = -1, right = -1;
  int window_width = -1, window_height = -1;

  /* See if W had a header line that has disappeared now, or vice versa.
     Get W's size.  */
  if (w)
    {
      window_box (w, ANY_AREA, 0, 0, &window_width, &window_height);

      header_line_p = window_wants_header_line (w);
      header_line_changed_p = header_line_p != matrix->header_line_p;
    }
  matrix->header_line_p = header_line_p;

  /* If POOL is null, MATRIX is a window matrix for window-based redisplay.
     Do nothing if MATRIX' size, position, vscroll, and marginal areas
     haven't changed.  This optimization is important because preserving
     the matrix means preventing redisplay.  */
  eassume (w != NULL || matrix->pool != NULL);
  if (matrix->pool == NULL)
    {
      left = margin_glyphs_to_reserve (w, dim.width, w->left_margin_cols);
      right = margin_glyphs_to_reserve (w, dim.width, w->right_margin_cols);
      eassert (left >= 0 && right >= 0);
      marginal_areas_changed_p = (left != matrix->left_margin_glyphs
				  || right != matrix->right_margin_glyphs);

      if (!marginal_areas_changed_p
	  && !XFRAME (w->frame)->fonts_changed
	  && !header_line_changed_p
	  && matrix->window_pixel_left == WINDOW_LEFT_PIXEL_EDGE (w)
	  && matrix->window_pixel_top == WINDOW_TOP_PIXEL_EDGE (w)
	  && matrix->window_height == window_height
	  && matrix->window_vscroll == w->vscroll
	  && matrix->window_width == window_width)
	return;
    }

  /* Enlarge MATRIX->rows if necessary.  New rows are cleared.  */
  if (matrix->rows_allocated < dim.height)
    {
      int old_alloc = matrix->rows_allocated;
      new_rows = dim.height - matrix->rows_allocated;
      matrix->rows = xpalloc (matrix->rows, &matrix->rows_allocated,
			      new_rows, INT_MAX, sizeof *matrix->rows);
      memset (matrix->rows + old_alloc, 0,
	      (matrix->rows_allocated - old_alloc) * sizeof *matrix->rows);
    }
  else
    new_rows = 0;

  /* If POOL is not null, MATRIX is a frame matrix or a window matrix
     on a frame not using window-based redisplay.  Set up pointers for
     each row into the glyph pool.  */
  if (matrix->pool)
    {
      eassert (matrix->pool->glyphs);

      if (w)
	{
	  left = margin_glyphs_to_reserve (w, dim.width,
					   w->left_margin_cols);
	  right = margin_glyphs_to_reserve (w, dim.width,
					    w->right_margin_cols);
	}
      else
	left = right = 0;

      for (i = 0; i < dim.height; ++i)
	{
	  struct glyph_row *row = &matrix->rows[i];

	  row->glyphs[LEFT_MARGIN_AREA]
	    = (matrix->pool->glyphs
	       + (y + i) * matrix->pool->ncolumns
	       + x);

	  if (w == NULL
	      || (row == matrix->rows + dim.height - 1
		  && window_wants_mode_line (w))
	      || (row == matrix->rows && matrix->header_line_p))
	    {
	      row->glyphs[TEXT_AREA]
		= row->glyphs[LEFT_MARGIN_AREA];
	      row->glyphs[RIGHT_MARGIN_AREA]
		= row->glyphs[TEXT_AREA] + dim.width;
	      row->glyphs[LAST_AREA]
		= row->glyphs[RIGHT_MARGIN_AREA];
	    }
	  else
	    {
	      row->glyphs[TEXT_AREA]
		= row->glyphs[LEFT_MARGIN_AREA] + left;
	      row->glyphs[RIGHT_MARGIN_AREA]
		= row->glyphs[TEXT_AREA] + dim.width - left - right;
	      row->glyphs[LAST_AREA]
		= row->glyphs[LEFT_MARGIN_AREA] + dim.width;
	    }
	}

      matrix->left_margin_glyphs = left;
      matrix->right_margin_glyphs = right;
    }
  else
    {
      /* If MATRIX->pool is null, MATRIX is responsible for managing
	 its own memory.  It is a window matrix for window-based redisplay.
	 Allocate glyph memory from the heap.  */
      if (dim.width > matrix->matrix_w
	  || new_rows
	  || header_line_changed_p
	  || marginal_areas_changed_p)
	{
	  struct glyph_row *row = matrix->rows;
	  struct glyph_row *end = row + matrix->rows_allocated;

	  while (row < end)
	    {
	      row->glyphs[LEFT_MARGIN_AREA]
		= xnrealloc (row->glyphs[LEFT_MARGIN_AREA],
			     dim.width, sizeof (struct glyph));

	      /* The mode line, if displayed, never has marginal areas.  */
	      if ((row == matrix->rows + dim.height - 1
		   && !(w && window_wants_mode_line (w)))
		  || (row == matrix->rows && matrix->header_line_p))
		{
		  row->glyphs[TEXT_AREA]
		    = row->glyphs[LEFT_MARGIN_AREA];
		  row->glyphs[RIGHT_MARGIN_AREA]
		    = row->glyphs[TEXT_AREA] + dim.width;
		  row->glyphs[LAST_AREA]
		    = row->glyphs[RIGHT_MARGIN_AREA];
		}
	      else
		{
		  row->glyphs[TEXT_AREA]
		    = row->glyphs[LEFT_MARGIN_AREA] + left;
		  row->glyphs[RIGHT_MARGIN_AREA]
		    = row->glyphs[TEXT_AREA] + dim.width - left - right;
		  row->glyphs[LAST_AREA]
		    = row->glyphs[LEFT_MARGIN_AREA] + dim.width;
		}
	      ++row;
	    }
	}

      eassert (left >= 0 && right >= 0);
      matrix->left_margin_glyphs = left;
      matrix->right_margin_glyphs = right;
    }

  /* Number of rows to be used by MATRIX.  */
  matrix->nrows = dim.height;
  eassert (matrix->nrows >= 0);

  if (w)
    {
      if (matrix == w->current_matrix)
	{
	  /* Mark rows in a current matrix of a window as not having
	     valid contents.  It's important to not do this for
	     desired matrices.  When Emacs starts, it may already be
	     building desired matrices when this function runs.  */
	  if (window_width < 0)
	    window_width = window_box_width (w, -1);

	  /* Optimize the case that only the height has changed (C-x 2,
	     upper window).  Invalidate all rows that are no longer part
	     of the window.  */
	  if (!marginal_areas_changed_p
	      && !header_line_changed_p
	      && new_rows == 0
	      && dim.width == matrix->matrix_w
	      && matrix->window_pixel_left == WINDOW_LEFT_PIXEL_EDGE (w)
	      && matrix->window_pixel_top == WINDOW_TOP_PIXEL_EDGE (w)
	      && matrix->window_width == window_width)
	    {
	      /* Find the last row in the window.  */
	      for (i = 0; i < matrix->nrows && matrix->rows[i].enabled_p; ++i)
		if (MATRIX_ROW_BOTTOM_Y (matrix->rows + i) >= window_height)
		  {
		    ++i;
		    break;
		  }

	      /* Window end is invalid, if inside of the rows that
		 are invalidated below.  */
	      if (w->window_end_vpos >= i)
		w->window_end_valid = 0;

	      while (i < matrix->nrows)
		matrix->rows[i++].enabled_p = false;
	    }
	  else
	    {
	      for (i = 0; i < matrix->nrows; ++i)
		matrix->rows[i].enabled_p = false;
	    }
	  /* We've disabled the mode-line row, so force redrawing of
	     the mode line, if any, since otherwise it will remain
	     disabled in the current matrix, and expose events won't
	     redraw it.  */
	  if (window_wants_mode_line (w))
	    w->update_mode_line = 1;
	}
      else if (matrix == w->desired_matrix)
	{
	  /* Rows in desired matrices always have to be cleared;
	     redisplay expects this is the case when it runs, so it
	     had better be the case when we adjust matrices between
	     redisplays.  */
	  for (i = 0; i < matrix->nrows; ++i)
	    matrix->rows[i].enabled_p = false;
	}
    }


  /* Remember last values to be able to optimize frame redraws.  */
  matrix->matrix_x = x;
  matrix->matrix_y = y;
  matrix->matrix_w = dim.width;
  matrix->matrix_h = dim.height;

  /* Record the top y location and height of W at the time the matrix
     was last adjusted.  This is used to optimize redisplay above.  */
  if (w)
    {
      matrix->window_pixel_left = WINDOW_LEFT_PIXEL_EDGE (w);
      matrix->window_pixel_top = WINDOW_TOP_PIXEL_EDGE (w);
      matrix->window_height = window_height;
      matrix->window_width = window_width;
      matrix->window_vscroll = w->vscroll;
    }
}


/* Reverse the contents of rows in MATRIX between START and END.  The
   contents of the row at END - 1 end up at START, END - 2 at START +
   1 etc.  This is part of the implementation of rotate_matrix (see
   below).  */

static void
reverse_rows (struct glyph_matrix *matrix, int start, int end)
{
  int i, j;

  for (i = start, j = end - 1; i < j; ++i, --j)
    {
      /* Non-ISO HP/UX compiler doesn't like auto struct
	 initialization.  */
      struct glyph_row temp;
      temp = matrix->rows[i];
      matrix->rows[i] = matrix->rows[j];
      matrix->rows[j] = temp;
    }
}


/* Rotate the contents of rows in MATRIX in the range FIRST .. LAST -
   1 by BY positions.  BY < 0 means rotate left, i.e. towards lower
   indices.  (Note: this does not copy glyphs, only glyph pointers in
   row structures are moved around).

   The algorithm used for rotating the vector was, I believe, first
   described by Kernighan.  See the vector R as consisting of two
   sub-vectors AB, where A has length BY for BY >= 0.  The result
   after rotating is then BA.  Reverse both sub-vectors to get ArBr
   and reverse the result to get (ArBr)r which is BA.  Similar for
   rotating right.  */

void
rotate_matrix (struct glyph_matrix *matrix, int first, int last, int by)
{
  if (by < 0)
    {
      /* Up (rotate left, i.e. towards lower indices).  */
      by = -by;
      reverse_rows (matrix, first, first + by);
      reverse_rows (matrix, first + by, last);
      reverse_rows (matrix, first, last);
    }
  else if (by > 0)
    {
      /* Down (rotate right, i.e. towards higher indices).  */
      reverse_rows (matrix, last - by, last);
      reverse_rows (matrix, first, last - by);
      reverse_rows (matrix, first, last);
    }
}


/* Increment buffer positions in glyph rows of MATRIX.  Do it for rows
   with indices START <= index < END.  Increment positions by DELTA/
   DELTA_BYTES.  */

void
increment_matrix_positions (struct glyph_matrix *matrix, int start, int end,
			    ptrdiff_t delta, ptrdiff_t delta_bytes)
{
  /* Check that START and END are reasonable values.  */
  eassert (start >= 0 && start <= matrix->nrows);
  eassert (end >= 0 && end <= matrix->nrows);
  eassert (start <= end);

  for (; start < end; ++start)
    increment_row_positions (matrix->rows + start, delta, delta_bytes);
}


/* Clear the enable_p flags in a range of rows in glyph matrix MATRIX.
   START and END are the row indices of the first and last + 1 row to clear.  */

void
clear_glyph_matrix_rows (struct glyph_matrix *matrix, int start, int end)
{
  eassert (start <= end);
  eassert (start >= 0 && (start < matrix->nrows
			  /* matrix->nrows can be 0 for the initial frame.  */
			  || (matrix->nrows == 0)));
  eassert (end >= 0 && end <= matrix->nrows);

  for (; start < end; ++start)
    matrix->rows[start].enabled_p = false;
}


/* Clear MATRIX.

   Empty all rows in MATRIX by clearing their enabled_p flags.
   The function prepare_desired_row will eventually really clear a row
   when it sees one with a false enabled_p flag.

   Reset update hints to default values.  The only update hint
   currently present is the flag MATRIX->no_scrolling_p.  */

void
clear_glyph_matrix (struct glyph_matrix *matrix)
{
  if (matrix)
    {
      clear_glyph_matrix_rows (matrix, 0, matrix->nrows);
      matrix->no_scrolling_p = 0;
    }
}


/* Shift part of the glyph matrix MATRIX of window W up or down.
   Increment y-positions in glyph rows between START and END by DY,
   and recompute their visible height.  */

void
shift_glyph_matrix (struct window *w, struct glyph_matrix *matrix, int start, int end, int dy)
{
  int min_y, max_y;

  eassert (start <= end);
  eassert (start >= 0 && start < matrix->nrows);
  eassert (end >= 0 && end <= matrix->nrows);

  min_y = WINDOW_HEADER_LINE_HEIGHT (w);
  max_y = WINDOW_BOX_HEIGHT_NO_MODE_LINE (w);

  for (; start < end; ++start)
    {
      struct glyph_row *row = &matrix->rows[start];

      row->y += dy;
      row->visible_height = row->height;

      if (row->y < min_y)
	row->visible_height -= min_y - row->y;
      if (row->y + row->height > max_y)
	row->visible_height -= row->y + row->height - max_y;
      if (row->fringe_bitmap_periodic_p)
	row->redraw_fringe_bitmaps_p = 1;
    }
}


/* Mark all rows in current matrices of frame F as invalid.  Marking
   invalid is done by setting enabled_p to zero for all rows in a
   current matrix.  */

void
clear_current_matrices (register struct frame *f)
{
  /* Clear frame current matrix, if we have one.  */
  if (f->current_matrix)
    clear_glyph_matrix (f->current_matrix);

#if defined (HAVE_X_WINDOWS) && ! defined (USE_X_TOOLKIT) && ! defined (USE_GTK)
  /* Clear the matrix of the menu bar window, if such a window exists.
     The menu bar window is currently used to display menus on X when
     no toolkit support is compiled in.  */
  if (WINDOWP (f->menu_bar_window))
    clear_glyph_matrix (XWINDOW (f->menu_bar_window)->current_matrix);
#endif

#if defined (HAVE_WINDOW_SYSTEM) && ! defined (USE_GTK) && ! defined (HAVE_NS)
  /* Clear the matrix of the tool-bar window, if any.  */
  if (WINDOWP (f->tool_bar_window))
    clear_glyph_matrix (XWINDOW (f->tool_bar_window)->current_matrix);
#endif

  /* Clear current window matrices.  */
  eassert (WINDOWP (FRAME_ROOT_WINDOW (f)));
  clear_window_matrices (XWINDOW (FRAME_ROOT_WINDOW (f)), 0);
}


/* Clear out all display lines of F for a coming redisplay.  */

void
clear_desired_matrices (register struct frame *f)
{
  if (f->desired_matrix)
    clear_glyph_matrix (f->desired_matrix);

#if defined (HAVE_X_WINDOWS) && ! defined (USE_X_TOOLKIT) && ! defined (USE_GTK)
  if (WINDOWP (f->menu_bar_window))
    clear_glyph_matrix (XWINDOW (f->menu_bar_window)->desired_matrix);
#endif

#if defined (HAVE_WINDOW_SYSTEM) && ! defined (USE_GTK) && ! defined (HAVE_NS)
  if (WINDOWP (f->tool_bar_window))
    clear_glyph_matrix (XWINDOW (f->tool_bar_window)->desired_matrix);
#endif

  /* Do it for window matrices.  */
  eassert (WINDOWP (FRAME_ROOT_WINDOW (f)));
  clear_window_matrices (XWINDOW (FRAME_ROOT_WINDOW (f)), 1);
}


/* Clear matrices in window tree rooted in W.  If DESIRED_P,
   clear desired matrices, otherwise clear current matrices.  */

static void
clear_window_matrices (struct window *w, bool desired_p)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	clear_window_matrices (XWINDOW (w->contents), desired_p);
      else
	{
	  if (desired_p)
	    clear_glyph_matrix (w->desired_matrix);
	  else
	    {
	      clear_glyph_matrix (w->current_matrix);
	      w->window_end_valid = 0;
	    }
	}

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}



/***********************************************************************
			      Glyph Rows

      See dispextern.h for an overall explanation of glyph rows.
 ***********************************************************************/

/* Clear glyph row ROW.  NOTE: this code relies on the current
   layout of `glyphs' and `used' fields of `struct glyph_row'.  */

void
clear_glyph_row (struct glyph_row *row)
{
  enum { off = offsetof (struct glyph_row, used) };

  /* Zero everything except pointers in `glyphs'.  */
  memset (row->used, 0, sizeof *row - off);
}


/* Make ROW an empty, enabled row of canonical character height,
   in window W starting at y-position Y.  */

void
blank_row (struct window *w, struct glyph_row *row, int y)
{
  int min_y, max_y;

  min_y = WINDOW_HEADER_LINE_HEIGHT (w);
  max_y = WINDOW_BOX_HEIGHT_NO_MODE_LINE (w);

  clear_glyph_row (row);
  row->y = y;
  row->ascent = row->phys_ascent = 0;
  row->height = row->phys_height = FRAME_LINE_HEIGHT (XFRAME (w->frame));
  row->visible_height = row->height;

  if (row->y < min_y)
    row->visible_height -= min_y - row->y;
  if (row->y + row->height > max_y)
    row->visible_height -= row->y + row->height - max_y;

  row->enabled_p = true;
}


/* Increment buffer positions in glyph row ROW.  DELTA and DELTA_BYTES
   are the amounts by which to change positions.  Note that the first
   glyph of the text area of a row can have a buffer position even if
   the used count of the text area is zero.  Such rows display line
   ends.  */

static void
increment_row_positions (struct glyph_row *row,
			 ptrdiff_t delta, ptrdiff_t delta_bytes)
{
  int area, i;

  /* Increment start and end positions.  */
  MATRIX_ROW_START_CHARPOS (row) += delta;
  MATRIX_ROW_START_BYTEPOS (row) += delta_bytes;
  MATRIX_ROW_END_CHARPOS (row) += delta;
  MATRIX_ROW_END_BYTEPOS (row) += delta_bytes;
  CHARPOS (row->start.pos) += delta;
  BYTEPOS (row->start.pos) += delta_bytes;
  CHARPOS (row->end.pos) += delta;
  BYTEPOS (row->end.pos) += delta_bytes;

  if (!row->enabled_p)
    return;

  /* Increment positions in glyphs.  */
  for (area = 0; area < LAST_AREA; ++area)
    for (i = 0; i < row->used[area]; ++i)
      if (BUFFERP (row->glyphs[area][i].object)
	  && row->glyphs[area][i].charpos > 0)
	row->glyphs[area][i].charpos += delta;

  /* Capture the case of rows displaying a line end.  */
  if (row->used[TEXT_AREA] == 0
      && MATRIX_ROW_DISPLAYS_TEXT_P (row))
    row->glyphs[TEXT_AREA]->charpos += delta;
}


#if 0
/* Swap glyphs between two glyph rows A and B.  This exchanges glyph
   contents, i.e. glyph structure contents are exchanged between A and
   B without changing glyph pointers in A and B.  */

static void
swap_glyphs_in_rows (struct glyph_row *a, struct glyph_row *b)
{
  int area;

  for (area = 0; area < LAST_AREA; ++area)
    {
      /* Number of glyphs to swap.  */
      int max_used = max (a->used[area], b->used[area]);

      /* Start of glyphs in area of row A.  */
      struct glyph *glyph_a = a->glyphs[area];

      /* End + 1 of glyphs in area of row A.  */
      struct glyph *glyph_a_end = a->glyphs[max_used];

      /* Start of glyphs in area of row B.  */
      struct glyph *glyph_b = b->glyphs[area];

      while (glyph_a < glyph_a_end)
	{
	  /* Non-ISO HP/UX compiler doesn't like auto struct
             initialization.  */
	  struct glyph temp;
	  temp = *glyph_a;
	  *glyph_a = *glyph_b;
	  *glyph_b = temp;
	  ++glyph_a;
	  ++glyph_b;
	}
    }
}

#endif /* 0 */

/* Exchange pointers to glyph memory between glyph rows A and B.  Also
   exchange the used[] array and the hash values of the rows, because
   these should all go together for the row's hash value to be
   correct.  */

static void
swap_glyph_pointers (struct glyph_row *a, struct glyph_row *b)
{
  int i;
  unsigned hash_tem = a->hash;

  for (i = 0; i < LAST_AREA + 1; ++i)
    {
      struct glyph *temp = a->glyphs[i];

      a->glyphs[i] = b->glyphs[i];
      b->glyphs[i] = temp;
      if (i < LAST_AREA)
	{
	  short used_tem = a->used[i];

	  a->used[i] = b->used[i];
	  b->used[i] = used_tem;
	}
    }
  a->hash = b->hash;
  b->hash = hash_tem;
}


/* Copy glyph row structure FROM to glyph row structure TO, except that
   glyph pointers, the `used' counts, and the hash values in the structures
   are left unchanged.  NOTE: this code relies on the current layout of
   `glyphs', `used', `hash' and `x' fields of `struct glyph_row'.  */

static void
copy_row_except_pointers (struct glyph_row *to, struct glyph_row *from)
{
  enum { off = offsetof (struct glyph_row, x) };

  memcpy (&to->x, &from->x, sizeof *to - off);
}


/* Assign glyph row FROM to glyph row TO.  This works like a structure
   assignment TO = FROM, except that glyph pointers are not copied but
   exchanged between TO and FROM.  Pointers must be exchanged to avoid
   a memory leak.  */

static void
assign_row (struct glyph_row *to, struct glyph_row *from)
{
  swap_glyph_pointers (to, from);
  copy_row_except_pointers (to, from);
}


/* Test whether the glyph memory of the glyph row WINDOW_ROW, which is
   a row in a window matrix, is a slice of the glyph memory of the
   glyph row FRAME_ROW which is a row in a frame glyph matrix.  Value
   is true if the glyph memory of WINDOW_ROW is part of the glyph
   memory of FRAME_ROW.  */

#ifdef GLYPH_DEBUG

static bool
glyph_row_slice_p (struct glyph_row *window_row, struct glyph_row *frame_row)
{
  struct glyph *window_glyph_start = window_row->glyphs[0];
  struct glyph *frame_glyph_start = frame_row->glyphs[0];
  struct glyph *frame_glyph_end = frame_row->glyphs[LAST_AREA];

  return (frame_glyph_start <= window_glyph_start
	  && window_glyph_start < frame_glyph_end);
}

#endif /* GLYPH_DEBUG */

#if 0

/* Find the row in the window glyph matrix WINDOW_MATRIX being a slice
   of ROW in the frame matrix FRAME_MATRIX.  Value is null if no row
   in WINDOW_MATRIX is found satisfying the condition.  */

static struct glyph_row *
find_glyph_row_slice (struct glyph_matrix *window_matrix,
		      struct glyph_matrix *frame_matrix, int row)
{
  int i;

  eassert (row >= 0 && row < frame_matrix->nrows);

  for (i = 0; i < window_matrix->nrows; ++i)
    if (glyph_row_slice_p (window_matrix->rows + i,
			   frame_matrix->rows + row))
      break;

  return i < window_matrix->nrows ? window_matrix->rows + i : 0;
}

#endif /* 0 */

/* Prepare ROW for display in windows W.  Desired rows are cleared
   lazily, i.e. they are only marked as to be cleared by setting their
   enabled_p flag to zero.  When a row is to be displayed, a prior
   call to this function really clears it.  In addition, this function
   makes sure the marginal areas of ROW are in sync with the window's
   display margins.  MODE_LINE_P non-zero means we are preparing a
   glyph row for header line or mode line.  */

void
prepare_desired_row (struct window *w, struct glyph_row *row, bool mode_line_p)
{
  if (!row->enabled_p)
    {
      bool rp = row->reversed_p;

      clear_glyph_row (row);
      row->enabled_p = true;
      row->reversed_p = rp;
    }
  if (mode_line_p)
    {
      /* Mode and header lines, if displayed, never have marginal
	 areas.  If we are called with MODE_LINE_P non-zero, we are
	 displaying the mode/header line in this window, and so the
	 marginal areas of this glyph row should be eliminated.  This
	 is needed when the mode/header line is switched on in a
	 window that has display margins.  */
      if (w->left_margin_cols > 0)
	row->glyphs[TEXT_AREA] = row->glyphs[LEFT_MARGIN_AREA];
      if (w->right_margin_cols > 0)
	row->glyphs[RIGHT_MARGIN_AREA] = row->glyphs[LAST_AREA];
    }
  else
    {
      /* The real number of glyphs reserved for the margins is
	 recorded in the glyph matrix, and can be different from
	 window's left_margin_cols and right_margin_cols; see
	 margin_glyphs_to_reserve for when that happens.  */
      int left = w->desired_matrix->left_margin_glyphs;
      int right = w->desired_matrix->right_margin_glyphs;

      /* Make sure the marginal areas of this row are in sync with
	 what the window wants, when the row actually displays text
	 and not header/mode line.  */
      if (w->left_margin_cols > 0
	  && (left != row->glyphs[TEXT_AREA] - row->glyphs[LEFT_MARGIN_AREA]))
	row->glyphs[TEXT_AREA] = row->glyphs[LEFT_MARGIN_AREA] + left;
      if (w->right_margin_cols > 0
	  && (right != row->glyphs[LAST_AREA] - row->glyphs[RIGHT_MARGIN_AREA]))
	row->glyphs[RIGHT_MARGIN_AREA] = row->glyphs[LAST_AREA] - right;
    }
}


/* Return a hash code for glyph row ROW, which may
   be from current or desired matrix of frame F.  */

static unsigned
line_hash_code (struct frame *f, struct glyph_row *row)
{
  unsigned hash = 0;

  if (row->enabled_p)
    {
      struct glyph *glyph = row->glyphs[TEXT_AREA];
      struct glyph *end = glyph + row->used[TEXT_AREA];

      while (glyph < end)
	{
	  int c = glyph->u.ch;
	  int face_id = glyph->face_id;
	  if (FRAME_MUST_WRITE_SPACES (f))
	    c -= SPACEGLYPH;
	  hash = (((hash << 4) + (hash >> 24)) & 0x0fffffff) + c;
	  hash = (((hash << 4) + (hash >> 24)) & 0x0fffffff) + face_id;
	  ++glyph;
	}

      if (hash == 0)
	hash = 1;
    }

  return hash;
}


/* Return the cost of drawing line VPOS in MATRIX, which may
   be current or desired matrix of frame F.  The cost equals
   the number of characters in the line.  If must_write_spaces
   is zero, leading and trailing spaces are ignored.  */

static int
line_draw_cost (struct frame *f, struct glyph_matrix *matrix, int vpos)
{
  struct glyph_row *row = matrix->rows + vpos;
  struct glyph *beg = row->glyphs[TEXT_AREA];
  struct glyph *end = beg + row->used[TEXT_AREA];
  int len;
  Lisp_Object *glyph_table_base = GLYPH_TABLE_BASE;
  ptrdiff_t glyph_table_len = GLYPH_TABLE_LENGTH;

  /* Ignore trailing and leading spaces if we can.  */
  if (!FRAME_MUST_WRITE_SPACES (f))
    {
      /* Skip from the end over trailing spaces.  */
      while (end > beg && CHAR_GLYPH_SPACE_P (*(end - 1)))
	--end;

      /* All blank line.  */
      if (end == beg)
	return 0;

      /* Skip over leading spaces.  */
      while (CHAR_GLYPH_SPACE_P (*beg))
	++beg;
    }

  /* If we don't have a glyph-table, each glyph is one character,
     so return the number of glyphs.  */
  if (glyph_table_base == 0)
    len = end - beg;
  else
    {
      /* Otherwise, scan the glyphs and accumulate their total length
	 in LEN.  */
      len = 0;
      while (beg < end)
	{
	  GLYPH g;

	  SET_GLYPH_FROM_CHAR_GLYPH (g, *beg);

	  if (GLYPH_INVALID_P (g)
	      || GLYPH_SIMPLE_P (glyph_table_base, glyph_table_len, g))
	    len += 1;
	  else
	    len += GLYPH_LENGTH (glyph_table_base, g);

	  ++beg;
	}
    }

  return len;
}


/* Return true if the glyph rows A and B have equal contents.
   MOUSE_FACE_P means compare the mouse_face_p flags of A and B, too.  */

static bool
row_equal_p (struct glyph_row *a, struct glyph_row *b, bool mouse_face_p)
{
  eassert (verify_row_hash (a));
  eassert (verify_row_hash (b));

  if (a == b)
    return 1;
  else if (a->hash != b->hash)
    return 0;
  else
    {
      struct glyph *a_glyph, *b_glyph, *a_end;
      int area;

      if (mouse_face_p && a->mouse_face_p != b->mouse_face_p)
	return 0;

      /* Compare glyphs.  */
      for (area = LEFT_MARGIN_AREA; area < LAST_AREA; ++area)
	{
	  if (a->used[area] != b->used[area])
	    return 0;

	  a_glyph = a->glyphs[area];
	  a_end = a_glyph + a->used[area];
	  b_glyph = b->glyphs[area];

	  while (a_glyph < a_end
		 && GLYPH_EQUAL_P (a_glyph, b_glyph))
	    ++a_glyph, ++b_glyph;

	  if (a_glyph != a_end)
	    return 0;
	}

      if (a->fill_line_p != b->fill_line_p
	  || a->cursor_in_fringe_p != b->cursor_in_fringe_p
	  || a->left_fringe_bitmap != b->left_fringe_bitmap
	  || a->left_fringe_face_id != b->left_fringe_face_id
	  || a->left_fringe_offset != b->left_fringe_offset
	  || a->right_fringe_bitmap != b->right_fringe_bitmap
	  || a->right_fringe_face_id != b->right_fringe_face_id
	  || a->right_fringe_offset != b->right_fringe_offset
	  || a->fringe_bitmap_periodic_p != b->fringe_bitmap_periodic_p
	  || a->overlay_arrow_bitmap != b->overlay_arrow_bitmap
	  || a->exact_window_width_line_p != b->exact_window_width_line_p
	  || a->overlapped_p != b->overlapped_p
	  || (MATRIX_ROW_CONTINUATION_LINE_P (a)
	      != MATRIX_ROW_CONTINUATION_LINE_P (b))
	  || a->reversed_p != b->reversed_p
	  /* Different partially visible characters on left margin.  */
	  || a->x != b->x
	  /* Different height.  */
	  || a->ascent != b->ascent
	  || a->phys_ascent != b->phys_ascent
	  || a->phys_height != b->phys_height
	  || a->visible_height != b->visible_height)
	return 0;
    }

  return 1;
}



/***********************************************************************
			      Glyph Pool

     See dispextern.h for an overall explanation of glyph pools.
 ***********************************************************************/

/* Allocate a glyph_pool structure.  The structure returned is initialized
   with zeros.  If GLYPH_DEBUG and ENABLE_CHECKING are in effect, the global
   variable glyph_pool_count is incremented for each pool allocated.  */

static struct glyph_pool *
new_glyph_pool (void)
{
  struct glyph_pool *result = xzalloc (sizeof *result);

#if defined GLYPH_DEBUG && defined ENABLE_CHECKING
  /* For memory leak and double deletion checking.  */
  ++glyph_pool_count;
#endif

  return result;
}


/* Free a glyph_pool structure POOL.  The function may be called with
   a null POOL pointer.  If GLYPH_DEBUG and ENABLE_CHECKING are in effect,
   global variable glyph_pool_count is decremented with every pool structure
   freed.  If this count gets negative, more structures were freed than
   allocated, i.e. one structure must have been freed more than once or
   a bogus pointer was passed to free_glyph_pool.  */

static void
free_glyph_pool (struct glyph_pool *pool)
{
  if (pool)
    {
#if defined GLYPH_DEBUG && defined ENABLE_CHECKING
      /* More freed than allocated?  */
      --glyph_pool_count;
      eassert (glyph_pool_count >= 0);
#endif
      xfree (pool->glyphs);
      xfree (pool);
    }
}


/* Enlarge a glyph pool POOL.  MATRIX_DIM gives the number of rows and
   columns we need.  This function never shrinks a pool.  The only
   case in which this would make sense, would be when a frame's size
   is changed from a large value to a smaller one.  But, if someone
   does it once, we can expect that he will do it again.

   Return true if the pool changed in a way which makes
   re-adjusting window glyph matrices necessary.  */

static bool
realloc_glyph_pool (struct glyph_pool *pool, struct dim matrix_dim)
{
  ptrdiff_t needed;
  bool changed_p;

  changed_p = (pool->glyphs == 0
	       || matrix_dim.height != pool->nrows
	       || matrix_dim.width != pool->ncolumns);

  /* Enlarge the glyph pool.  */
  if (INT_MULTIPLY_WRAPV (matrix_dim.height, matrix_dim.width, &needed))
    memory_full (SIZE_MAX);
  if (needed > pool->nglyphs)
    {
      ptrdiff_t old_nglyphs = pool->nglyphs;
      pool->glyphs = xpalloc (pool->glyphs, &pool->nglyphs,
			      needed - old_nglyphs, -1, sizeof *pool->glyphs);
      memclear (pool->glyphs + old_nglyphs,
		(pool->nglyphs - old_nglyphs) * sizeof *pool->glyphs);
    }

  /* Remember the number of rows and columns because (a) we use them
     to do sanity checks, and (b) the number of columns determines
     where rows in the frame matrix start---this must be available to
     determine pointers to rows of window sub-matrices.  */
  pool->nrows = matrix_dim.height;
  pool->ncolumns = matrix_dim.width;

  return changed_p;
}



/***********************************************************************
			      Debug Code
 ***********************************************************************/

#ifdef GLYPH_DEBUG


/* Flush standard output.  This is sometimes useful to call from the debugger.
   XXX Maybe this should be changed to flush the current terminal instead of
   stdout.
*/

void flush_stdout (void) EXTERNALLY_VISIBLE;

void
flush_stdout (void)
{
  fflush (stdout);
}


/* Check that no glyph pointers have been lost in MATRIX.  If a
   pointer has been lost, e.g. by using a structure assignment between
   rows, at least one pointer must occur more than once in the rows of
   MATRIX.  */

void
check_matrix_pointer_lossage (struct glyph_matrix *matrix)
{
  int i, j;

  for (i = 0; i < matrix->nrows; ++i)
    for (j = 0; j < matrix->nrows; ++j)
      eassert (i == j
	       || (matrix->rows[i].glyphs[TEXT_AREA]
		   != matrix->rows[j].glyphs[TEXT_AREA]));
}


/* Get a pointer to glyph row ROW in MATRIX, with bounds checks.  */

struct glyph_row *
matrix_row (struct glyph_matrix *matrix, int row)
{
  eassert (matrix && matrix->rows);
  eassert (row >= 0 && row < matrix->nrows);

  /* That's really too slow for normal testing because this function
     is called almost everywhere.  Although---it's still astonishingly
     fast, so it is valuable to have for debugging purposes.  */
#if 0
  check_matrix_pointer_lossage (matrix);
#endif

  return matrix->rows + row;
}


#if 0 /* This function makes invalid assumptions when text is
	 partially invisible.  But it might come handy for debugging
	 nevertheless.  */

/* Check invariants that must hold for an up to date current matrix of
   window W.  */

static void
check_matrix_invariants (struct window *w)
{
  struct glyph_matrix *matrix = w->current_matrix;
  int yb = window_text_bottom_y (w);
  struct glyph_row *row = matrix->rows;
  struct glyph_row *last_text_row = NULL;
  struct buffer *saved = current_buffer;
  struct buffer *buffer = XBUFFER (w->contents);
  int c;

  /* This can sometimes happen for a fresh window.  */
  if (matrix->nrows < 2)
    return;

  set_buffer_temp (buffer);

  /* Note: last row is always reserved for the mode line.  */
  while (MATRIX_ROW_DISPLAYS_TEXT_P (row)
	 && MATRIX_ROW_BOTTOM_Y (row) < yb)
    {
      struct glyph_row *next = row + 1;

      if (MATRIX_ROW_DISPLAYS_TEXT_P (row))
	last_text_row = row;

      /* Check that character and byte positions are in sync.  */
      eassert (MATRIX_ROW_START_BYTEPOS (row)
	       == CHAR_TO_BYTE (MATRIX_ROW_START_CHARPOS (row)));
      eassert (BYTEPOS (row->start.pos)
	       == CHAR_TO_BYTE (CHARPOS (row->start.pos)));

      /* CHAR_TO_BYTE aborts when invoked for a position > Z.  We can
	 have such a position temporarily in case of a minibuffer
	 displaying something like `[Sole completion]' at its end.  */
      if (MATRIX_ROW_END_CHARPOS (row) < BUF_ZV (current_buffer))
	{
	  eassert (MATRIX_ROW_END_BYTEPOS (row)
		   == CHAR_TO_BYTE (MATRIX_ROW_END_CHARPOS (row)));
	  eassert (BYTEPOS (row->end.pos)
		   == CHAR_TO_BYTE (CHARPOS (row->end.pos)));
	}

      /* Check that end position of `row' is equal to start position
	 of next row.  */
      if (next->enabled_p && MATRIX_ROW_DISPLAYS_TEXT_P (next))
	{
	  eassert (MATRIX_ROW_END_CHARPOS (row)
		   == MATRIX_ROW_START_CHARPOS (next));
	  eassert (MATRIX_ROW_END_BYTEPOS (row)
		   == MATRIX_ROW_START_BYTEPOS (next));
	  eassert (CHARPOS (row->end.pos) == CHARPOS (next->start.pos));
	  eassert (BYTEPOS (row->end.pos) == BYTEPOS (next->start.pos));
	}
      row = next;
    }

  eassert (w->current_matrix->nrows == w->desired_matrix->nrows);
  eassert (w->desired_matrix->rows != NULL);
  set_buffer_temp (saved);
}

#endif /* 0  */

#endif /* GLYPH_DEBUG */



/**********************************************************************
		 Allocating/ Adjusting Glyph Matrices
 **********************************************************************/

/* Allocate glyph matrices over a window tree for a frame-based
   redisplay

   X and Y are column/row within the frame glyph matrix where
   sub-matrices for the window tree rooted at WINDOW must be
   allocated.  DIM_ONLY_P means that the caller of this
   function is only interested in the result matrix dimension, and
   matrix adjustments should not be performed.

   The function returns the total width/height of the sub-matrices of
   the window tree.  If called on a frame root window, the computation
   will take the mini-buffer window into account.

   *WINDOW_CHANGE_FLAGS is set to a bit mask with bits

   NEW_LEAF_MATRIX set if any window in the tree did not have a
   glyph matrices yet, and

   CHANGED_LEAF_MATRIX set if the dimension or location of a matrix of
   any window in the tree will be changed or have been changed (see
   DIM_ONLY_P)

   *WINDOW_CHANGE_FLAGS must be initialized by the caller of this
   function.

   Windows are arranged into chains of windows on the same level
   through the next fields of window structures.  Such a level can be
   either a sequence of horizontally adjacent windows from left to
   right, or a sequence of vertically adjacent windows from top to
   bottom.  Each window in a horizontal sequence can be either a leaf
   window or a vertical sequence; a window in a vertical sequence can
   be either a leaf or a horizontal sequence.  All windows in a
   horizontal sequence have the same height, and all windows in a
   vertical sequence have the same width.

   This function uses, for historical reasons, a more general
   algorithm to determine glyph matrix dimensions that would be
   necessary.

   The matrix height of a horizontal sequence is determined by the
   maximum height of any matrix in the sequence.  The matrix width of
   a horizontal sequence is computed by adding up matrix widths of
   windows in the sequence.

   |<------- result width ------->|
   +---------+----------+---------+ ---
   |         |		|	  |  |
   |         |		|	  |
   +---------+		|	  |  result height
	     |		+---------+
	     |		|            |
	     +----------+	    ---

   The matrix width of a vertical sequence is the maximum matrix width
   of any window in the sequence.  Its height is computed by adding up
   matrix heights of windows in the sequence.

   |<---- result width -->|
   +---------+		    ---
   |         |               |
   |         |               |
   +---------+--+            |
   |		|            |
   |		|	     result height
   |		|
   +------------+---------+  |
   |			  |  |
   |			  |  |
   +------------+---------+ ---  */

/* Bit indicating that a new matrix will be allocated or has been
   allocated.  */

#define NEW_LEAF_MATRIX		(1 << 0)

/* Bit indicating that a matrix will or has changed its location or
   size.  */

#define CHANGED_LEAF_MATRIX	(1 << 1)

static struct dim
allocate_matrices_for_frame_redisplay (Lisp_Object window, int x, int y,
				       bool dim_only_p, int *window_change_flags)
{
  struct frame *f = XFRAME (WINDOW_FRAME (XWINDOW (window)));
  int x0 = x, y0 = y;
  int wmax = 0, hmax = 0;
  struct dim total;
  struct dim dim;
  struct window *w;
  bool in_horz_combination_p;

  /* What combination is WINDOW part of?  Compute this once since the
     result is the same for all windows in the `next' chain.  The
     special case of a root window (parent equal to nil) is treated
     like a vertical combination because a root window's `next'
     points to the mini-buffer window, if any, which is arranged
     vertically below other windows.  */
  in_horz_combination_p
    = (!NILP (XWINDOW (window)->parent)
       && WINDOW_HORIZONTAL_COMBINATION_P (XWINDOW (XWINDOW (window)->parent)));

  /* For WINDOW and all windows on the same level.  */
  do
    {
      w = XWINDOW (window);

      /* Get the dimension of the window sub-matrix for W, depending
	 on whether this is a combination or a leaf window.  */
      if (WINDOWP (w->contents))
	dim = allocate_matrices_for_frame_redisplay (w->contents, x, y,
						     dim_only_p,
						     window_change_flags);
      else
	{
	  /* If not already done, allocate sub-matrix structures.  */
	  if (w->desired_matrix == NULL)
	    {
	      w->desired_matrix = new_glyph_matrix (f->desired_pool);
	      w->current_matrix = new_glyph_matrix (f->current_pool);
	      *window_change_flags |= NEW_LEAF_MATRIX;
	    }

	  /* Width and height MUST be chosen so that there are no
	     holes in the frame matrix.  */
	  dim.width = required_matrix_width (w);
	  dim.height = required_matrix_height (w);

	  /* Will matrix be re-allocated?  */
	  if (x != w->desired_matrix->matrix_x
	      || y != w->desired_matrix->matrix_y
	      || dim.width != w->desired_matrix->matrix_w
	      || dim.height != w->desired_matrix->matrix_h
	      || (margin_glyphs_to_reserve (w, dim.width,
					    w->left_margin_cols)
		  != w->desired_matrix->left_margin_glyphs)
	      || (margin_glyphs_to_reserve (w, dim.width,
					    w->right_margin_cols)
		  != w->desired_matrix->right_margin_glyphs))
	    *window_change_flags |= CHANGED_LEAF_MATRIX;

	  /* Actually change matrices, if allowed.  Do not consider
	     CHANGED_LEAF_MATRIX computed above here because the pool
	     may have been changed which we don't know here.  We trust
	     that we only will be called with DIM_ONLY_P when
	     necessary.  */
	  if (!dim_only_p)
	    {
	      adjust_glyph_matrix (w, w->desired_matrix, x, y, dim);
	      adjust_glyph_matrix (w, w->current_matrix, x, y, dim);
	    }
	}

      /* If we are part of a horizontal combination, advance x for
	 windows to the right of W; otherwise advance y for windows
	 below W.  */
      if (in_horz_combination_p)
	x += dim.width;
      else
        y += dim.height;

      /* Remember maximum glyph matrix dimensions.  */
      wmax = max (wmax, dim.width);
      hmax = max (hmax, dim.height);

      /* Next window on same level.  */
      window = w->next;
    }
  while (!NILP (window));

  /* Set `total' to the total glyph matrix dimension of this window
     level.  In a vertical combination, the width is the width of the
     widest window; the height is the y we finally reached, corrected
     by the y we started with.  In a horizontal combination, the total
     height is the height of the tallest window, and the width is the
     x we finally reached, corrected by the x we started with.  */
  if (in_horz_combination_p)
    {
      total.width = x - x0;
      total.height = hmax;
    }
  else
    {
      total.width = wmax;
      total.height = y - y0;
    }

  return total;
}


/* Return the required height of glyph matrices for window W.  */

static int
required_matrix_height (struct window *w)
{
#ifdef HAVE_WINDOW_SYSTEM
  struct frame *f = XFRAME (w->frame);

  if (FRAME_WINDOW_P (f))
    {
      /* https://lists.gnu.org/r/emacs-devel/2015-11/msg00194.html  */
      int ch_height = max (FRAME_SMALLEST_FONT_HEIGHT (f), 1);
      int window_pixel_height = window_box_height (w) + eabs (w->vscroll);

      return (((window_pixel_height + ch_height - 1)
	       / ch_height) * w->nrows_scale_factor
	      /* One partially visible line at the top and
		 bottom of the window.  */
	      + 2
	      /* 2 for header and mode line.  */
	      + 2);
    }
#endif /* HAVE_WINDOW_SYSTEM */

  return WINDOW_TOTAL_LINES (w);
}


/* Return the required width of glyph matrices for window W.  */

static int
required_matrix_width (struct window *w)
{
#ifdef HAVE_WINDOW_SYSTEM
  struct frame *f = XFRAME (w->frame);
  if (FRAME_WINDOW_P (f))
    {
      /* https://lists.gnu.org/r/emacs-devel/2015-11/msg00194.html  */
      int ch_width = max (FRAME_SMALLEST_CHAR_WIDTH (f), 1);

      /* Compute number of glyphs needed in a glyph row.  */
      return (((WINDOW_PIXEL_WIDTH (w) + ch_width - 1)
	       / ch_width) * w->ncols_scale_factor
	      /* 2 partially visible columns in the text area.  */
	      + 2
	      /* One partially visible column at the right
		 edge of each marginal area.  */
	      + 1 + 1);
    }
#endif /* HAVE_WINDOW_SYSTEM */

  return w->total_cols;
}


/* Allocate window matrices for window-based redisplay.  W is the
   window whose matrices must be allocated/reallocated.  */

static void
allocate_matrices_for_window_redisplay (struct window *w)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	allocate_matrices_for_window_redisplay (XWINDOW (w->contents));
      else
	{
	  /* W is a leaf window.  */
	  struct dim dim;

	  /* If matrices are not yet allocated, allocate them now.  */
	  if (w->desired_matrix == NULL)
	    {
	      w->desired_matrix = new_glyph_matrix (NULL);
	      w->current_matrix = new_glyph_matrix (NULL);
	    }

	  dim.width = required_matrix_width (w);
	  dim.height = required_matrix_height (w);
	  adjust_glyph_matrix (w, w->desired_matrix, 0, 0, dim);
	  adjust_glyph_matrix (w, w->current_matrix, 0, 0, dim);
	}

      w = NILP (w->next) ? NULL : XWINDOW (w->next);
    }
}

/* Allocate/reallocate glyph matrices of a single frame F.
   This function must be called when a new frame is created,
   its size changes, or its window configuration changes.  */

void
adjust_frame_glyphs (struct frame *f)
{
  /* Block input so that expose events and other events that access
     glyph matrices are not processed while we are changing them.  */
  block_input ();

  if (FRAME_WINDOW_P (f))
    adjust_frame_glyphs_for_window_redisplay (f);
  else
    adjust_frame_glyphs_for_frame_redisplay (f);

  /* Don't forget the buffer for decode_mode_spec.  */
  adjust_decode_mode_spec_buffer (f);

  f->glyphs_initialized_p = 1;

  unblock_input ();
}

/* Return true if any window in the tree has nonzero window margins.  See
   the hack at the end of adjust_frame_glyphs_for_frame_redisplay.  */
static bool
showing_window_margins_p (struct window *w)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	{
	  if (showing_window_margins_p (XWINDOW (w->contents)))
	    return 1;
	}
      else if (w->left_margin_cols > 0 || w->right_margin_cols > 0)
	return 1;

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
  return 0;
}


/* In the window tree with root W, build current matrices of leaf
   windows from the frame's current matrix.  */

static void
fake_current_matrices (Lisp_Object window)
{
  struct window *w;

  for (; !NILP (window); window = w->next)
    {
      w = XWINDOW (window);

      if (WINDOWP (w->contents))
	fake_current_matrices (w->contents);
      else
	{
	  int i;
	  struct frame *f = XFRAME (w->frame);
	  struct glyph_matrix *m = w->current_matrix;
	  struct glyph_matrix *fm = f->current_matrix;

	  eassert (m->matrix_h == WINDOW_TOTAL_LINES (w));
	  eassert (m->matrix_w == WINDOW_TOTAL_COLS (w));

	  for (i = 0; i < m->matrix_h; ++i)
	    {
	      struct glyph_row *r = m->rows + i;
	      struct glyph_row *fr = fm->rows + i + WINDOW_TOP_EDGE_LINE (w);

	      eassert (r->glyphs[TEXT_AREA] >= fr->glyphs[TEXT_AREA]
		       && r->glyphs[LAST_AREA] <= fr->glyphs[LAST_AREA]);

	      r->enabled_p = fr->enabled_p;
	      if (r->enabled_p)
		{
		  r->used[LEFT_MARGIN_AREA] = m->left_margin_glyphs;
		  r->used[RIGHT_MARGIN_AREA] = m->right_margin_glyphs;
		  r->used[TEXT_AREA] = (m->matrix_w
					- r->used[LEFT_MARGIN_AREA]
					- r->used[RIGHT_MARGIN_AREA]);
		  r->mode_line_p = 0;
		}
	    }
	}
    }
}


/* Save away the contents of frame F's current frame matrix.  Value is
   a glyph matrix holding the contents of F's current frame matrix.  */

static struct glyph_matrix *
save_current_matrix (struct frame *f)
{
  int i;
  struct glyph_matrix *saved = xzalloc (sizeof *saved);
  saved->nrows = f->current_matrix->nrows;
  saved->rows = xzalloc (saved->nrows * sizeof *saved->rows);

  for (i = 0; i < saved->nrows; ++i)
    {
      struct glyph_row *from = f->current_matrix->rows + i;
      struct glyph_row *to = saved->rows + i;
      ptrdiff_t nbytes = from->used[TEXT_AREA] * sizeof (struct glyph);

      to->glyphs[TEXT_AREA] = xmalloc (nbytes);
      memcpy (to->glyphs[TEXT_AREA], from->glyphs[TEXT_AREA], nbytes);
      to->used[TEXT_AREA] = from->used[TEXT_AREA];
      to->enabled_p = from->enabled_p;
      to->hash = from->hash;
      if (from->used[LEFT_MARGIN_AREA])
	{
	  nbytes = from->used[LEFT_MARGIN_AREA] * sizeof (struct glyph);
	  to->glyphs[LEFT_MARGIN_AREA] = xmalloc (nbytes);
	  memcpy (to->glyphs[LEFT_MARGIN_AREA],
		  from->glyphs[LEFT_MARGIN_AREA], nbytes);
	  to->used[LEFT_MARGIN_AREA] = from->used[LEFT_MARGIN_AREA];
	}
      if (from->used[RIGHT_MARGIN_AREA])
	{
	  nbytes = from->used[RIGHT_MARGIN_AREA] * sizeof (struct glyph);
	  to->glyphs[RIGHT_MARGIN_AREA] = xmalloc (nbytes);
	  memcpy (to->glyphs[RIGHT_MARGIN_AREA],
		  from->glyphs[RIGHT_MARGIN_AREA], nbytes);
	  to->used[RIGHT_MARGIN_AREA] = from->used[RIGHT_MARGIN_AREA];
	}
    }

  return saved;
}


/* Restore the contents of frame F's current frame matrix from SAVED,
   and free memory associated with SAVED.  */

static void
restore_current_matrix (struct frame *f, struct glyph_matrix *saved)
{
  int i;

  for (i = 0; i < saved->nrows; ++i)
    {
      struct glyph_row *from = saved->rows + i;
      struct glyph_row *to = f->current_matrix->rows + i;
      ptrdiff_t nbytes = from->used[TEXT_AREA] * sizeof (struct glyph);

      memcpy (to->glyphs[TEXT_AREA], from->glyphs[TEXT_AREA], nbytes);
      to->used[TEXT_AREA] = from->used[TEXT_AREA];
      xfree (from->glyphs[TEXT_AREA]);
      nbytes = from->used[LEFT_MARGIN_AREA] * sizeof (struct glyph);
      if (nbytes)
	{
	  memcpy (to->glyphs[LEFT_MARGIN_AREA],
		  from->glyphs[LEFT_MARGIN_AREA], nbytes);
	  to->used[LEFT_MARGIN_AREA] = from->used[LEFT_MARGIN_AREA];
	  xfree (from->glyphs[LEFT_MARGIN_AREA]);
	}
      else
	to->used[LEFT_MARGIN_AREA] = 0;
      nbytes = from->used[RIGHT_MARGIN_AREA] * sizeof (struct glyph);
      if (nbytes)
	{
	  memcpy (to->glyphs[RIGHT_MARGIN_AREA],
		  from->glyphs[RIGHT_MARGIN_AREA], nbytes);
	  to->used[RIGHT_MARGIN_AREA] = from->used[RIGHT_MARGIN_AREA];
	  xfree (from->glyphs[RIGHT_MARGIN_AREA]);
	}
      else
	to->used[RIGHT_MARGIN_AREA] = 0;
    }

  xfree (saved->rows);
  xfree (saved);
}



/* Allocate/reallocate glyph matrices of a single frame F for
   frame-based redisplay.  */

static void
adjust_frame_glyphs_for_frame_redisplay (struct frame *f)
{
  struct dim matrix_dim;
  bool pool_changed_p;
  int window_change_flags;
  int top_window_y;

  if (!FRAME_LIVE_P (f))
    return;

  top_window_y = FRAME_TOP_MARGIN (f);

  /* Allocate glyph pool structures if not already done.  */
  if (f->desired_pool == NULL)
    {
      f->desired_pool = new_glyph_pool ();
      f->current_pool = new_glyph_pool ();
    }

  /* Allocate frames matrix structures if needed.  */
  if (f->desired_matrix == NULL)
    {
      f->desired_matrix = new_glyph_matrix (f->desired_pool);
      f->current_matrix = new_glyph_matrix (f->current_pool);
    }

  /* Compute window glyph matrices.  (This takes the mini-buffer
     window into account).  The result is the size of the frame glyph
     matrix needed.  The variable window_change_flags is set to a bit
     mask indicating whether new matrices will be allocated or
     existing matrices change their size or location within the frame
     matrix.  */
  window_change_flags = 0;
  matrix_dim
    = allocate_matrices_for_frame_redisplay (FRAME_ROOT_WINDOW (f),
					     0, top_window_y,
					     1,
					     &window_change_flags);

  /* Add in menu bar lines, if any.  */
  matrix_dim.height += top_window_y;

  /* Enlarge pools as necessary.  */
  pool_changed_p = realloc_glyph_pool (f->desired_pool, matrix_dim);
  realloc_glyph_pool (f->current_pool, matrix_dim);

  /* Set up glyph pointers within window matrices.  Do this only if
     absolutely necessary since it requires a frame redraw.  */
  if (pool_changed_p || window_change_flags)
    {
      /* Do it for window matrices.  */
      allocate_matrices_for_frame_redisplay (FRAME_ROOT_WINDOW (f),
					     0, top_window_y, 0,
					     &window_change_flags);

      /* Size of frame matrices must equal size of frame.  Note
	 that we are called for X frames with window widths NOT equal
	 to the frame width (from CHANGE_FRAME_SIZE_1).  */
      if (matrix_dim.width != FRAME_TOTAL_COLS (f)
	  || matrix_dim.height != FRAME_TOTAL_LINES (f))
	return;

      eassert (matrix_dim.width == FRAME_TOTAL_COLS (f)
	       && matrix_dim.height == FRAME_TOTAL_LINES (f));

      /* Pointers to glyph memory in glyph rows are exchanged during
	 the update phase of redisplay, which means in general that a
	 frame's current matrix consists of pointers into both the
	 desired and current glyph pool of the frame.  Adjusting a
	 matrix sets the frame matrix up so that pointers are all into
	 the same pool.  If we want to preserve glyph contents of the
	 current matrix over a call to adjust_glyph_matrix, we must
	 make a copy of the current glyphs, and restore the current
	 matrix' contents from that copy.  */
      if (display_completed
	  && !FRAME_GARBAGED_P (f)
	  && matrix_dim.width == f->current_matrix->matrix_w
	  && matrix_dim.height == f->current_matrix->matrix_h
	  /* For some reason, the frame glyph matrix gets corrupted if
	     any of the windows contain margins.  I haven't been able
	     to hunt down the reason, but for the moment this prevents
	     the problem from manifesting. -- cyd  */
	  && !showing_window_margins_p (XWINDOW (FRAME_ROOT_WINDOW (f))))
	{
	  struct glyph_matrix *copy = save_current_matrix (f);
	  adjust_glyph_matrix (NULL, f->desired_matrix, 0, 0, matrix_dim);
	  adjust_glyph_matrix (NULL, f->current_matrix, 0, 0, matrix_dim);
	  restore_current_matrix (f, copy);
	  fake_current_matrices (FRAME_ROOT_WINDOW (f));
	}
      else
	{
	  adjust_glyph_matrix (NULL, f->desired_matrix, 0, 0, matrix_dim);
	  adjust_glyph_matrix (NULL, f->current_matrix, 0, 0, matrix_dim);
	  SET_FRAME_GARBAGED (f);
	}
    }
}


/* Allocate/reallocate glyph matrices of a single frame F for
   window-based redisplay.  */

static void
adjust_frame_glyphs_for_window_redisplay (struct frame *f)
{
  eassert (FRAME_WINDOW_P (f) && FRAME_LIVE_P (f));

  /* Allocate/reallocate window matrices.  */
  allocate_matrices_for_window_redisplay (XWINDOW (FRAME_ROOT_WINDOW (f)));

#if defined (HAVE_X_WINDOWS) && ! defined (USE_X_TOOLKIT) && ! defined (USE_GTK)
  /* Allocate/ reallocate matrices of the dummy window used to display
     the menu bar under X when no X toolkit support is available.  */
  {
    /* Allocate a dummy window if not already done.  */
    struct window *w;
    if (NILP (f->menu_bar_window))
      {
	Lisp_Object frame;
	fset_menu_bar_window (f, make_window ());
	w = XWINDOW (f->menu_bar_window);
	XSETFRAME (frame, f);
	wset_frame (w, frame);
	w->pseudo_window_p = 1;
      }
    else
      w = XWINDOW (f->menu_bar_window);

    /* Set window dimensions to frame dimensions and allocate or
       adjust glyph matrices of W.  */
    w->pixel_left = 0;
    w->left_col = 0;
    w->pixel_top = 0;
    w->top_line = 0;
    w->pixel_width = (FRAME_PIXEL_WIDTH (f)
		      - 2 * FRAME_INTERNAL_BORDER_WIDTH (f));
    w->total_cols = FRAME_TOTAL_COLS (f);
    w->pixel_height = FRAME_MENU_BAR_HEIGHT (f);
    w->total_lines = FRAME_MENU_BAR_LINES (f);
    allocate_matrices_for_window_redisplay (w);
  }
#endif

#if defined (HAVE_WINDOW_SYSTEM) && ! defined (USE_GTK) && ! defined (HAVE_NS)
  {
    /* Allocate/ reallocate matrices of the tool bar window.  If we
       don't have a tool bar window yet, make one.  */
    struct window *w;
    if (NILP (f->tool_bar_window))
      {
	Lisp_Object frame;
	fset_tool_bar_window (f, make_window ());
	w = XWINDOW (f->tool_bar_window);
	XSETFRAME (frame, f);
	wset_frame (w, frame);
	w->pseudo_window_p = 1;
      }
    else
      w = XWINDOW (f->tool_bar_window);

    w->pixel_left = 0;
    w->left_col = 0;
    w->pixel_top = FRAME_MENU_BAR_HEIGHT (f);
    w->top_line = FRAME_MENU_BAR_LINES (f);
    w->total_cols = FRAME_TOTAL_COLS (f);
    w->pixel_width = (FRAME_PIXEL_WIDTH (f)
		       - 2 * FRAME_INTERNAL_BORDER_WIDTH (f));
    w->total_lines = FRAME_TOOL_BAR_LINES (f);
    w->pixel_height = FRAME_TOOL_BAR_HEIGHT (f);
    allocate_matrices_for_window_redisplay (w);
  }
#endif
}


/* Re-allocate buffer for decode_mode_spec on frame F.  */

static void
adjust_decode_mode_spec_buffer (struct frame *f)
{
  int frame_message_buf_size = FRAME_MESSAGE_BUF_SIZE (f);

  eassert (frame_message_buf_size >= 0);
  f->decode_mode_spec_buffer = xrealloc (f->decode_mode_spec_buffer,
					 frame_message_buf_size + 1);
}



/**********************************************************************
			Freeing Glyph Matrices
 **********************************************************************/

/* Free glyph memory for a frame F.  F may be null.  This function can
   be called for the same frame more than once.  The root window of
   F may be nil when this function is called.  This is the case when
   the function is called when F is destroyed.  */

void
free_glyphs (struct frame *f)
{
  if (f && f->glyphs_initialized_p)
    {
      /* Block interrupt input so that we don't get surprised by an X
         event while we're in an inconsistent state.  */
      block_input ();
      f->glyphs_initialized_p = 0;

      /* Release window sub-matrices.  */
      if (!NILP (f->root_window))
        free_window_matrices (XWINDOW (f->root_window));

#if defined (HAVE_X_WINDOWS) && ! defined (USE_X_TOOLKIT) && ! defined (USE_GTK)
      /* Free the dummy window for menu bars without X toolkit and its
	 glyph matrices.  */
      if (!NILP (f->menu_bar_window))
	{
	  struct window *w = XWINDOW (f->menu_bar_window);
	  free_glyph_matrix (w->desired_matrix);
	  free_glyph_matrix (w->current_matrix);
	  w->desired_matrix = w->current_matrix = NULL;
	  fset_menu_bar_window (f, Qnil);
	}
#endif

#if defined (HAVE_WINDOW_SYSTEM) && ! defined (USE_GTK) && ! defined (HAVE_NS)
      /* Free the tool bar window and its glyph matrices.  */
      if (!NILP (f->tool_bar_window))
	{
	  struct window *w = XWINDOW (f->tool_bar_window);
	  free_glyph_matrix (w->desired_matrix);
	  free_glyph_matrix (w->current_matrix);
	  w->desired_matrix = w->current_matrix = NULL;
	  fset_tool_bar_window (f, Qnil);
	}
#endif

      /* Release frame glyph matrices.  Reset fields to zero in
	 case we are called a second time.  */
      if (f->desired_matrix)
	{
	  free_glyph_matrix (f->desired_matrix);
	  free_glyph_matrix (f->current_matrix);
	  f->desired_matrix = f->current_matrix = NULL;
	}

      /* Release glyph pools.  */
      if (f->desired_pool)
	{
	  free_glyph_pool (f->desired_pool);
	  free_glyph_pool (f->current_pool);
	  f->desired_pool = f->current_pool = NULL;
	}

      unblock_input ();
    }
}


/* Free glyph sub-matrices in the window tree rooted at W.  This
   function may be called with a null pointer, and it may be called on
   the same tree more than once.  */

void
free_window_matrices (struct window *w)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	free_window_matrices (XWINDOW (w->contents));
      else
	{
	  /* This is a leaf window.  Free its memory and reset fields
	     to zero in case this function is called a second time for
	     W.  */
	  free_glyph_matrix (w->current_matrix);
	  free_glyph_matrix (w->desired_matrix);
	  w->current_matrix = w->desired_matrix = NULL;
	}

      /* Next window on same level.  */
      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}


/* Check glyph memory leaks.  This function is called from
   shut_down_emacs.  Note that frames are not destroyed when Emacs
   exits.  We therefore free all glyph memory for all active frames
   explicitly and check that nothing is left allocated.  */

void
check_glyph_memory (void)
{
  Lisp_Object tail, frame;

  /* Free glyph memory for all frames.  */
  FOR_EACH_FRAME (tail, frame)
    free_glyphs (XFRAME (frame));

#if defined GLYPH_DEBUG && defined ENABLE_CHECKING
  /* Check that nothing is left allocated.  */
  eassert (glyph_matrix_count == 0);
  eassert (glyph_pool_count == 0);
#endif
}



/**********************************************************************
		       Building a Frame Matrix
 **********************************************************************/

/* Most of the redisplay code works on glyph matrices attached to
   windows.  This is a good solution most of the time, but it is not
   suitable for terminal code.  Terminal output functions cannot rely
   on being able to set an arbitrary terminal window.  Instead they
   must be provided with a view of the whole frame, i.e. the whole
   screen.  We build such a view by constructing a frame matrix from
   window matrices in this section.

   Windows that must be updated have their must_be_updated_p flag set.
   For all such windows, their desired matrix is made part of the
   desired frame matrix.  For other windows, their current matrix is
   made part of the desired frame matrix.

   +-----------------+----------------+
   |     desired     |   desired      |
   |                 |                |
   +-----------------+----------------+
   |               current            |
   |                                  |
   +----------------------------------+

   Desired window matrices can be made part of the frame matrix in a
   cheap way: We exploit the fact that the desired frame matrix and
   desired window matrices share their glyph memory.  This is not
   possible for current window matrices.  Their glyphs are copied to
   the desired frame matrix.  The latter is equivalent to
   preserve_other_columns in the old redisplay.

   Used glyphs counters for frame matrix rows are the result of adding
   up glyph lengths of the window matrices.  A line in the frame
   matrix is enabled, if a corresponding line in a window matrix is
   enabled.

   After building the desired frame matrix, it will be passed to
   terminal code, which will manipulate both the desired and current
   frame matrix.  Changes applied to the frame's current matrix have
   to be visible in current window matrices afterwards, of course.

   This problem is solved like this:

   1. Window and frame matrices share glyphs.  Window matrices are
   constructed in a way that their glyph contents ARE the glyph
   contents needed in a frame matrix.  Thus, any modification of
   glyphs done in terminal code will be reflected in window matrices
   automatically.

   2. Exchanges of rows in a frame matrix done by terminal code are
   intercepted by hook functions so that corresponding row operations
   on window matrices can be performed.  This is necessary because we
   use pointers to glyphs in glyph row structures.  To satisfy the
   assumption of point 1 above that glyphs are updated implicitly in
   window matrices when they are manipulated via the frame matrix,
   window and frame matrix must of course agree where to find the
   glyphs for their rows.  Possible manipulations that must be
   mirrored are assignments of rows of the desired frame matrix to the
   current frame matrix and scrolling the current frame matrix.  */

/* Build frame F's desired matrix from window matrices.  Only windows
   which have the flag must_be_updated_p set have to be updated.  Menu
   bar lines of a frame are not covered by window matrices, so make
   sure not to touch them in this function.  */

static void
build_frame_matrix (struct frame *f)
{
  int i;

  /* F must have a frame matrix when this function is called.  */
  eassert (!FRAME_WINDOW_P (f));

  /* Clear all rows in the frame matrix covered by window matrices.
     Menu bar lines are not covered by windows.  */
  for (i = FRAME_TOP_MARGIN (f); i < f->desired_matrix->nrows; ++i)
    clear_glyph_row (MATRIX_ROW (f->desired_matrix, i));

  /* Build the matrix by walking the window tree.  */
  build_frame_matrix_from_window_tree (f->desired_matrix,
				       XWINDOW (FRAME_ROOT_WINDOW (f)));
}


/* Walk a window tree, building a frame matrix MATRIX from window
   matrices.  W is the root of a window tree.  */

static void
build_frame_matrix_from_window_tree (struct glyph_matrix *matrix, struct window *w)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	build_frame_matrix_from_window_tree (matrix, XWINDOW (w->contents));
      else
	build_frame_matrix_from_leaf_window (matrix, w);

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}


/* Add a window's matrix to a frame matrix.  FRAME_MATRIX is the
   desired frame matrix built.  W is a leaf window whose desired or
   current matrix is to be added to FRAME_MATRIX.  W's flag
   must_be_updated_p determines which matrix it contributes to
   FRAME_MATRIX.  If W->must_be_updated_p, W's desired matrix
   is added to FRAME_MATRIX, otherwise W's current matrix is added.
   Adding a desired matrix means setting up used counters and such in
   frame rows, while adding a current window matrix to FRAME_MATRIX
   means copying glyphs.  The latter case corresponds to
   preserve_other_columns in the old redisplay.  */

static void
build_frame_matrix_from_leaf_window (struct glyph_matrix *frame_matrix, struct window *w)
{
  struct glyph_matrix *window_matrix;
  int window_y, frame_y;
  /* If non-zero, a glyph to insert at the right border of W.  */
  GLYPH right_border_glyph;

  SET_GLYPH_FROM_CHAR (right_border_glyph, 0);

  /* Set window_matrix to the matrix we have to add to FRAME_MATRIX.  */
  if (w->must_be_updated_p)
    {
      window_matrix = w->desired_matrix;

      /* Decide whether we want to add a vertical border glyph.  */
      if (!WINDOW_RIGHTMOST_P (w))
	{
	  struct Lisp_Char_Table *dp = window_display_table (w);
	  Lisp_Object gc;

	  SET_GLYPH_FROM_CHAR (right_border_glyph, '|');
	  if (dp
	      && (gc = DISP_BORDER_GLYPH (dp), GLYPH_CODE_P (gc)))
	    {
	      SET_GLYPH_FROM_GLYPH_CODE (right_border_glyph, gc);
	      spec_glyph_lookup_face (w, &right_border_glyph);
	    }

	  if (GLYPH_FACE (right_border_glyph) <= 0)
	    SET_GLYPH_FACE (right_border_glyph, VERTICAL_BORDER_FACE_ID);
	}
    }
  else
    window_matrix = w->current_matrix;

  /* For all rows in the window matrix and corresponding rows in the
     frame matrix.  */
  window_y = 0;
  frame_y = window_matrix->matrix_y;
  while (window_y < window_matrix->nrows)
    {
      struct glyph_row *frame_row = frame_matrix->rows + frame_y;
      struct glyph_row *window_row = window_matrix->rows + window_y;
      bool current_row_p = window_matrix == w->current_matrix;

      /* Fill up the frame row with spaces up to the left margin of the
	 window row.  */
      fill_up_frame_row_with_spaces (frame_row, window_matrix->matrix_x);

      /* Fill up areas in the window matrix row with spaces.  */
      fill_up_glyph_row_with_spaces (window_row);

      /* If only part of W's desired matrix has been built, and
         window_row wasn't displayed, use the corresponding current
         row instead.  */
      if (window_matrix == w->desired_matrix
	  && !window_row->enabled_p)
	{
	  window_row = w->current_matrix->rows + window_y;
	  current_row_p = 1;
	}

      if (current_row_p)
	{
	  /* Copy window row to frame row.  */
	  memcpy (frame_row->glyphs[TEXT_AREA] + window_matrix->matrix_x,
		  window_row->glyphs[0],
		  window_matrix->matrix_w * sizeof (struct glyph));
	}
      else
	{
	  eassert (window_row->enabled_p);

	  /* Only when a desired row has been displayed, we want
	     the corresponding frame row to be updated.  */
	  frame_row->enabled_p = true;

          /* Maybe insert a vertical border between horizontally adjacent
	     windows.  */
          if (GLYPH_CHAR (right_border_glyph) != 0)
	    {
              struct glyph *border = window_row->glyphs[LAST_AREA] - 1;
	      SET_CHAR_GLYPH_FROM_GLYPH (*border, right_border_glyph);
	    }

#ifdef GLYPH_DEBUG
	  /* Window row window_y must be a slice of frame row
	     frame_y.  */
	  eassert (glyph_row_slice_p (window_row, frame_row));

	  /* If rows are in sync, we don't have to copy glyphs because
	     frame and window share glyphs.  */

	  strcpy (w->current_matrix->method, w->desired_matrix->method);
	  add_window_display_history (w, w->current_matrix->method, 0);
#endif
	}

      /* Set number of used glyphs in the frame matrix.  Since we fill
         up with spaces, and visit leaf windows from left to right it
         can be done simply.  */
      frame_row->used[TEXT_AREA]
	= window_matrix->matrix_x + window_matrix->matrix_w;

      /* Next row.  */
      ++window_y;
      ++frame_y;
    }
}

/* Given a user-specified glyph, possibly including a Lisp-level face
   ID, return a glyph that has a realized face ID.
   This is used for glyphs displayed specially and not part of the text;
   for instance, vertical separators, truncation markers, etc.  */

void
spec_glyph_lookup_face (struct window *w, GLYPH *glyph)
{
  int lface_id = GLYPH_FACE (*glyph);
  /* Convert the glyph's specified face to a realized (cache) face.  */
  if (lface_id > 0)
    {
      int face_id = merge_faces (XFRAME (w->frame),
				 Qt, lface_id, DEFAULT_FACE_ID);
      SET_GLYPH_FACE (*glyph, face_id);
    }
}

/* Add spaces to a glyph row ROW in a window matrix.

   Each row has the form:

   +---------+-----------------------------+------------+
   | left    |	text			   | right	|
   +---------+-----------------------------+------------+

   Left and right marginal areas are optional.  This function adds
   spaces to areas so that there are no empty holes between areas.
   In other words:  If the right area is not empty, the text area
   is filled up with spaces up to the right area.   If the text area
   is not empty, the left area is filled up.

   To be called for frame-based redisplay, only.  */

static void
fill_up_glyph_row_with_spaces (struct glyph_row *row)
{
  fill_up_glyph_row_area_with_spaces (row, LEFT_MARGIN_AREA);
  fill_up_glyph_row_area_with_spaces (row, TEXT_AREA);
  fill_up_glyph_row_area_with_spaces (row, RIGHT_MARGIN_AREA);
}


/* Fill area AREA of glyph row ROW with spaces.  To be called for
   frame-based redisplay only.  */

static void
fill_up_glyph_row_area_with_spaces (struct glyph_row *row, int area)
{
  if (row->glyphs[area] < row->glyphs[area + 1])
    {
      struct glyph *end = row->glyphs[area + 1];
      struct glyph *text = row->glyphs[area] + row->used[area];

      while (text < end)
	*text++ = space_glyph;
      row->used[area] = text - row->glyphs[area];
    }
}


/* Add spaces to the end of ROW in a frame matrix until index UPTO is
   reached.  In frame matrices only one area, TEXT_AREA, is used.  */

void
fill_up_frame_row_with_spaces (struct glyph_row *row, int upto)
{
  int i = row->used[TEXT_AREA];
  struct glyph *glyph = row->glyphs[TEXT_AREA];

  while (i < upto)
    glyph[i++] = space_glyph;

  row->used[TEXT_AREA] = i;
}



/**********************************************************************
      Mirroring operations on frame matrices in window matrices
 **********************************************************************/

/* Set frame being updated via frame-based redisplay to F.  This
   function must be called before updates to make explicit that we are
   working on frame matrices or not.  */

static void
set_frame_matrix_frame (struct frame *f)
{
  frame_matrix_frame = f;
}


/* Make sure glyph row ROW in CURRENT_MATRIX is up to date.
   DESIRED_MATRIX is the desired matrix corresponding to
   CURRENT_MATRIX.  The update is done by exchanging glyph pointers
   between rows in CURRENT_MATRIX and DESIRED_MATRIX.  If
   frame_matrix_frame is non-null, this indicates that the exchange is
   done in frame matrices, and that we have to perform analogous
   operations in window matrices of frame_matrix_frame.  */

static void
make_current (struct glyph_matrix *desired_matrix, struct glyph_matrix *current_matrix, int row)
{
  struct glyph_row *current_row = MATRIX_ROW (current_matrix, row);
  struct glyph_row *desired_row = MATRIX_ROW (desired_matrix, row);
  bool mouse_face_p = current_row->mouse_face_p;

  /* Do current_row = desired_row.  This exchanges glyph pointers
     between both rows, and does a structure assignment otherwise.  */
  assign_row (current_row, desired_row);

  /* Enable current_row to mark it as valid.  */
  current_row->enabled_p = true;
  current_row->mouse_face_p = mouse_face_p;

  /* If we are called on frame matrices, perform analogous operations
     for window matrices.  */
  if (frame_matrix_frame)
    mirror_make_current (XWINDOW (frame_matrix_frame->root_window), row);
}


/* W is the root of a window tree.  FRAME_ROW is the index of a row in
   W's frame which has been made current (by swapping pointers between
   current and desired matrix).  Perform analogous operations in the
   matrices of leaf windows in the window tree rooted at W.  */

static void
mirror_make_current (struct window *w, int frame_row)
{
  while (w)
    {
      if (WINDOWP (w->contents))
 	mirror_make_current (XWINDOW (w->contents), frame_row);
      else
	{
	  /* Row relative to window W.  Don't use FRAME_TO_WINDOW_VPOS
	     here because the checks performed in debug mode there
	     will not allow the conversion.  */
	  int row = frame_row - w->desired_matrix->matrix_y;

	  /* If FRAME_ROW is within W, assign the desired row to the
	     current row (exchanging glyph pointers).  */
	  if (row >= 0 && row < w->desired_matrix->matrix_h)
	    {
	      struct glyph_row *current_row
		= MATRIX_ROW (w->current_matrix, row);
	      struct glyph_row *desired_row
		= MATRIX_ROW (w->desired_matrix, row);

	      if (desired_row->enabled_p)
		assign_row (current_row, desired_row);
	      else
		swap_glyph_pointers (desired_row, current_row);
	      current_row->enabled_p = true;

	      /* Set the Y coordinate of the mode/header line's row.
		 It is needed in draw_row_with_mouse_face to find the
		 screen coordinates.  (Window-based redisplay sets
		 this in update_window, but no one seems to do that
		 for frame-based redisplay.)  */
	      if (current_row->mode_line_p)
		current_row->y = row;
	    }
	}

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}


/* Perform row dance after scrolling.  We are working on the range of
   lines UNCHANGED_AT_TOP + 1 to UNCHANGED_AT_TOP + NLINES (not
   including) in MATRIX.  COPY_FROM is a vector containing, for each
   row I in the range 0 <= I < NLINES, the index of the original line
   to move to I.  This index is relative to the row range, i.e. 0 <=
   index < NLINES.  RETAINED_P is a vector containing zero for each
   row 0 <= I < NLINES which is empty.

   This function is called from do_scrolling and do_direct_scrolling.  */

void
mirrored_line_dance (struct glyph_matrix *matrix, int unchanged_at_top, int nlines,
		     int *copy_from, char *retained_p)
{
  /* A copy of original rows.  */
  struct glyph_row *old_rows;

  /* Rows to assign to.  */
  struct glyph_row *new_rows = MATRIX_ROW (matrix, unchanged_at_top);

  int i;

  /* Make a copy of the original rows.  */
  USE_SAFE_ALLOCA;
  SAFE_NALLOCA (old_rows, 1, nlines);
  memcpy (old_rows, new_rows, nlines * sizeof *old_rows);

  /* Assign new rows, maybe clear lines.  */
  for (i = 0; i < nlines; ++i)
    {
      bool enabled_before_p = new_rows[i].enabled_p;

      eassert (i + unchanged_at_top < matrix->nrows);
      eassert (unchanged_at_top + copy_from[i] < matrix->nrows);
      new_rows[i] = old_rows[copy_from[i]];
      new_rows[i].enabled_p = enabled_before_p;

      /* RETAINED_P is zero for empty lines.  */
      if (!retained_p[copy_from[i]])
	new_rows[i].enabled_p = false;
    }

  /* Do the same for window matrices, if MATRIX is a frame matrix.  */
  if (frame_matrix_frame)
    mirror_line_dance (XWINDOW (frame_matrix_frame->root_window),
		       unchanged_at_top, nlines, copy_from, retained_p);

  SAFE_FREE ();
}


/* Synchronize glyph pointers in the current matrix of window W with
   the current frame matrix.  */

static void
sync_window_with_frame_matrix_rows (struct window *w)
{
  struct frame *f = XFRAME (w->frame);
  struct glyph_row *window_row, *window_row_end, *frame_row;
  int left, right, x, width;

  /* Preconditions: W must be a live window on a tty frame.  */
  eassert (BUFFERP (w->contents));
  eassert (!FRAME_WINDOW_P (f));

  left = margin_glyphs_to_reserve (w, 1, w->left_margin_cols);
  right = margin_glyphs_to_reserve (w, 1, w->right_margin_cols);
  x = w->current_matrix->matrix_x;
  width = w->current_matrix->matrix_w;

  window_row = w->current_matrix->rows;
  window_row_end = window_row + w->current_matrix->nrows;
  frame_row = f->current_matrix->rows + WINDOW_TOP_EDGE_LINE (w);

  for (; window_row < window_row_end; ++window_row, ++frame_row)
    {
      window_row->glyphs[LEFT_MARGIN_AREA]
	= frame_row->glyphs[0] + x;
      window_row->glyphs[TEXT_AREA]
	= window_row->glyphs[LEFT_MARGIN_AREA] + left;
      window_row->glyphs[LAST_AREA]
	= window_row->glyphs[LEFT_MARGIN_AREA] + width;
      window_row->glyphs[RIGHT_MARGIN_AREA]
	= window_row->glyphs[LAST_AREA] - right;
    }
}


/* Return the window in the window tree rooted in W containing frame
   row ROW.  Value is null if none is found.  */

static struct window *
frame_row_to_window (struct window *w, int row)
{
  struct window *found = NULL;

  while (w && !found)
    {
      if (WINDOWP (w->contents))
 	found = frame_row_to_window (XWINDOW (w->contents), row);
      else if (row >= WINDOW_TOP_EDGE_LINE (w)
	       && row < WINDOW_BOTTOM_EDGE_LINE (w))
	found = w;

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }

  return found;
}


/* Perform a line dance in the window tree rooted at W, after
   scrolling a frame matrix in mirrored_line_dance.

   We are working on the range of lines UNCHANGED_AT_TOP + 1 to
   UNCHANGED_AT_TOP + NLINES (not including) in W's frame matrix.
   COPY_FROM is a vector containing, for each row I in the range 0 <=
   I < NLINES, the index of the original line to move to I.  This
   index is relative to the row range, i.e. 0 <= index < NLINES.
   RETAINED_P is a vector containing zero for each row 0 <= I < NLINES
   which is empty.  */

static void
mirror_line_dance (struct window *w, int unchanged_at_top, int nlines, int *copy_from, char *retained_p)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	mirror_line_dance (XWINDOW (w->contents), unchanged_at_top,
			   nlines, copy_from, retained_p);
      else
	{
	  /* W is a leaf window, and we are working on its current
	     matrix m.  */
	  struct glyph_matrix *m = w->current_matrix;
	  int i;
	  bool sync_p = 0;
	  struct glyph_row *old_rows;

	  /* Make a copy of the original rows of matrix m.  */
	  USE_SAFE_ALLOCA;
	  SAFE_NALLOCA (old_rows, 1, m->nrows);
	  memcpy (old_rows, m->rows, m->nrows * sizeof *old_rows);

	  for (i = 0; i < nlines; ++i)
	    {
	      /* Frame relative line assigned to.  */
	      int frame_to = i + unchanged_at_top;

	      /* Frame relative line assigned.  */
	      int frame_from = copy_from[i] + unchanged_at_top;

	      /* Window relative line assigned to.  */
	      int window_to = frame_to - m->matrix_y;

	      /* Window relative line assigned.  */
	      int window_from = frame_from - m->matrix_y;

	      /* Is assigned line inside window?  */
	      bool from_inside_window_p
		= window_from >= 0 && window_from < m->matrix_h;

	      /* Is assigned to line inside window?  */
	      bool to_inside_window_p
		= window_to >= 0 && window_to < m->matrix_h;

	      if (from_inside_window_p && to_inside_window_p)
		{
		  /* Do the assignment.  The enabled_p flag is saved
		     over the assignment because the old redisplay did
		     that.  */
		  bool enabled_before_p = m->rows[window_to].enabled_p;
		  m->rows[window_to] = old_rows[window_from];
		  m->rows[window_to].enabled_p = enabled_before_p;

		  /* If frame line is empty, window line is empty, too.  */
		  if (!retained_p[copy_from[i]])
		    m->rows[window_to].enabled_p = false;
		}
	      else if (to_inside_window_p)
		{
		  /* A copy between windows.  This is an infrequent
		     case not worth optimizing.  */
		  struct frame *f = XFRAME (w->frame);
		  struct window *root = XWINDOW (FRAME_ROOT_WINDOW (f));
		  struct window *w2;
		  struct glyph_matrix *m2;
		  int m2_from;

		  w2 = frame_row_to_window (root, frame_from);
		  /* ttn@surf.glug.org: when enabling menu bar using `emacs
		     -nw', FROM_FRAME sometimes has no associated window.
		     This check avoids a segfault if W2 is null.  */
		  if (w2)
		    {
		      m2 = w2->current_matrix;
		      m2_from = frame_from - m2->matrix_y;
		      copy_row_except_pointers (m->rows + window_to,
						m2->rows + m2_from);

		      /* If frame line is empty, window line is empty, too.  */
		      if (!retained_p[copy_from[i]])
			m->rows[window_to].enabled_p = false;
		    }
		  sync_p = 1;
		}
	      else if (from_inside_window_p)
		sync_p = 1;
	    }

	  /* If there was a copy between windows, make sure glyph
	     pointers are in sync with the frame matrix.  */
	  if (sync_p)
	    sync_window_with_frame_matrix_rows (w);

	  /* Check that no pointers are lost.  */
	  CHECK_MATRIX (m);

	  SAFE_FREE ();
	}

      /* Next window on same level.  */
      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}


#ifdef GLYPH_DEBUG

/* Check that window and frame matrices agree about their
   understanding where glyphs of the rows are to find.  For each
   window in the window tree rooted at W, check that rows in the
   matrices of leaf window agree with their frame matrices about
   glyph pointers.  */

static void
check_window_matrix_pointers (struct window *w)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	check_window_matrix_pointers (XWINDOW (w->contents));
      else
	{
	  struct frame *f = XFRAME (w->frame);
	  check_matrix_pointers (w->desired_matrix, f->desired_matrix);
	  check_matrix_pointers (w->current_matrix, f->current_matrix);
	}

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}


/* Check that window rows are slices of frame rows.  WINDOW_MATRIX is
   a window and FRAME_MATRIX is the corresponding frame matrix.  For
   each row in WINDOW_MATRIX check that it's a slice of the
   corresponding frame row.  If it isn't, abort.  */

static void
check_matrix_pointers (struct glyph_matrix *window_matrix,
		       struct glyph_matrix *frame_matrix)
{
  /* Row number in WINDOW_MATRIX.  */
  int i = 0;

  /* Row number corresponding to I in FRAME_MATRIX.  */
  int j = window_matrix->matrix_y;

  /* For all rows check that the row in the window matrix is a
     slice of the row in the frame matrix.  If it isn't we didn't
     mirror an operation on the frame matrix correctly.  */
  while (i < window_matrix->nrows)
    {
      if (!glyph_row_slice_p (window_matrix->rows + i,
			      frame_matrix->rows + j))
        emacs_abort ();
      ++i, ++j;
    }
}

#endif /* GLYPH_DEBUG */



/**********************************************************************
		      VPOS and HPOS translations
 **********************************************************************/

#ifdef GLYPH_DEBUG

/* Translate vertical position VPOS which is relative to window W to a
   vertical position relative to W's frame.  */

static int
window_to_frame_vpos (struct window *w, int vpos)
{
  eassert (!FRAME_WINDOW_P (XFRAME (w->frame)));
  eassert (vpos >= 0 && vpos <= w->desired_matrix->nrows);
  vpos += WINDOW_TOP_EDGE_LINE (w);
  eassert (vpos >= 0 && vpos <= FRAME_TOTAL_LINES (XFRAME (w->frame)));
  return vpos;
}


/* Translate horizontal position HPOS which is relative to window W to
   a horizontal position relative to W's frame.  */

static int
window_to_frame_hpos (struct window *w, int hpos)
{
  eassert (!FRAME_WINDOW_P (XFRAME (w->frame)));
  hpos += WINDOW_LEFT_EDGE_COL (w);
  return hpos;
}

#endif /* GLYPH_DEBUG */



/**********************************************************************
			    Redrawing Frames
 **********************************************************************/

/* Redraw frame F.  */

void
redraw_frame (struct frame *f)
{
  /* Error if F has no glyphs.  */
  eassert (f->glyphs_initialized_p);
  update_begin (f);
  if (FRAME_MSDOS_P (f))
    FRAME_TERMINAL (f)->set_terminal_modes_hook (FRAME_TERMINAL (f));
  clear_frame (f);
  clear_current_matrices (f);
  update_end (f);
  fset_redisplay (f);
  /* Mark all windows as inaccurate, so that every window will have
     its redisplay done.  */
  mark_window_display_accurate (FRAME_ROOT_WINDOW (f), 0);
  set_window_update_flags (XWINDOW (FRAME_ROOT_WINDOW (f)), true);
  f->garbaged = false;
}

DEFUN ("redraw-frame", Fredraw_frame, Sredraw_frame, 0, 1, 0,
       doc: /* Clear frame FRAME and output again what is supposed to appear on it.
If FRAME is omitted or nil, the selected frame is used.  */)
  (Lisp_Object frame)
{
  redraw_frame (decode_live_frame (frame));
  return Qnil;
}

DEFUN ("redraw-display", Fredraw_display, Sredraw_display, 0, 0, "",
       doc: /* Clear and redisplay all visible frames.  */)
  (void)
{
  Lisp_Object tail, frame;

  FOR_EACH_FRAME (tail, frame)
    if (FRAME_VISIBLE_P (XFRAME (frame)))
      redraw_frame (XFRAME (frame));

  return Qnil;
}



/***********************************************************************
			     Frame Update
 ***********************************************************************/

/* Update frame F based on the data in desired matrices.

   If FORCE_P, don't let redisplay be stopped by detecting pending input.
   If INHIBIT_HAIRY_ID_P, don't try scrolling.

   Value is true if redisplay was stopped due to pending input.  */

bool
update_frame (struct frame *f, bool force_p, bool inhibit_hairy_id_p)
{
  /* True means display has been paused because of pending input.  */
  bool paused_p;
  struct window *root_window = XWINDOW (f->root_window);

  if (redisplay_dont_pause)
    force_p = true;
  else if (!force_p && detect_input_pending_ignore_squeezables ())
    {
      paused_p = true;
      goto do_pause;
    }

  if (FRAME_WINDOW_P (f))
    {
      /* We are working on window matrix basis.  All windows whose
	 flag must_be_updated_p is set have to be updated.  */

      /* Record that we are not working on frame matrices.  */
      set_frame_matrix_frame (NULL);

      /* Update all windows in the window tree of F, maybe stopping
	 when pending input is detected.  */
      update_begin (f);

#if defined (HAVE_X_WINDOWS) && ! defined (USE_X_TOOLKIT) && ! defined (USE_GTK)
      /* Update the menu bar on X frames that don't have toolkit
	 support.  */
      if (WINDOWP (f->menu_bar_window))
	update_window (XWINDOW (f->menu_bar_window), true);
#endif

#if defined (HAVE_WINDOW_SYSTEM) && ! defined (USE_GTK) && ! defined (HAVE_NS)
      /* Update the tool-bar window, if present.  */
      if (WINDOWP (f->tool_bar_window))
	{
	  struct window *w = XWINDOW (f->tool_bar_window);

	  /* Update tool-bar window.  */
	  if (w->must_be_updated_p)
	    {
	      Lisp_Object tem;

	      update_window (w, true);
	      w->must_be_updated_p = false;

	      /* Swap tool-bar strings.  We swap because we want to
		 reuse strings.  */
	      tem = f->current_tool_bar_string;
	      fset_current_tool_bar_string (f, f->desired_tool_bar_string);
	      fset_desired_tool_bar_string (f, tem);
	    }
	}
#endif

      /* Update windows.  */
      paused_p = update_window_tree (root_window, force_p);
      update_end (f);
    }
  else
    {
      /* We are working on frame matrix basis.  Set the frame on whose
	 frame matrix we operate.  */
      set_frame_matrix_frame (f);

      /* Build F's desired matrix from window matrices.  */
      build_frame_matrix (f);

      /* Update the display.  */
      update_begin (f);
      paused_p = update_frame_1 (f, force_p, inhibit_hairy_id_p, 1, false);
      update_end (f);

      if (FRAME_TERMCAP_P (f) || FRAME_MSDOS_P (f))
        {
          if (FRAME_TTY (f)->termscript)
	    fflush_unlocked (FRAME_TTY (f)->termscript);
	  if (FRAME_TERMCAP_P (f))
	    fflush_unlocked (FRAME_TTY (f)->output);
        }

      /* Check window matrices for lost pointers.  */
#ifdef GLYPH_DEBUG
      check_window_matrix_pointers (root_window);
      add_frame_display_history (f, paused_p);
#endif
    }

 do_pause:
  /* Reset flags indicating that a window should be updated.  */
  set_window_update_flags (root_window, false);

  display_completed = !paused_p;
  return paused_p;
}

/* Update a TTY frame F that has a menu dropped down over some of its
   glyphs.  This is like the second part of update_frame, but it
   doesn't call build_frame_matrix, because we already have the
   desired matrix prepared, and don't want it to be overwritten by the
   text of the normal display.

   ROW and COL, if non-negative, are the row and column of the TTY
   frame where to position the cursor after the frame update is
   complete.  Negative values mean ask update_frame_1 to position the
   cursor "normally", i.e. at point in the selected window.  */
void
update_frame_with_menu (struct frame *f, int row, int col)
{
  struct window *root_window = XWINDOW (f->root_window);
  bool paused_p, cursor_at_point_p;

  eassert (FRAME_TERMCAP_P (f));

  /* We are working on frame matrix basis.  Set the frame on whose
     frame matrix we operate.  */
  set_frame_matrix_frame (f);

  /* Update the display.  */
  update_begin (f);
  cursor_at_point_p = !(row >= 0 && col >= 0);
  /* Force update_frame_1 not to stop due to pending input, and not
     try scrolling.  */
  paused_p = update_frame_1 (f, 1, 1, cursor_at_point_p, true);
  /* ROW and COL tell us where in the menu to position the cursor, so
     that screen readers know the active region on the screen.  */
  if (!cursor_at_point_p)
    cursor_to (f, row, col);
  update_end (f);

  if (FRAME_TTY (f)->termscript)
    fflush_unlocked (FRAME_TTY (f)->termscript);
  fflush_unlocked (FRAME_TTY (f)->output);
  /* Check window matrices for lost pointers.  */
#if GLYPH_DEBUG
#if 0
      /* We cannot possibly survive the matrix pointers check, since
	 we have overwritten parts of the frame glyph matrix without
	 making any updates to the window matrices.  */
  check_window_matrix_pointers (root_window);
#endif
  add_frame_display_history (f, paused_p);
#endif

  /* Reset flags indicating that a window should be updated.  */
  set_window_update_flags (root_window, false);
  display_completed = !paused_p;
}


/************************************************************************
			 Window-based updates
 ************************************************************************/

/* Perform updates in window tree rooted at W.
   If FORCE_P, don't stop updating if input is pending.  */

static bool
update_window_tree (struct window *w, bool force_p)
{
  bool paused_p = 0;

  while (w && !paused_p)
    {
      if (WINDOWP (w->contents))
	paused_p |= update_window_tree (XWINDOW (w->contents), force_p);
      else if (w->must_be_updated_p)
	paused_p |= update_window (w, force_p);

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }

  return paused_p;
}


/* Update window W if its flag must_be_updated_p is set.
   If FORCE_P, don't stop updating if input is pending.  */

void
update_single_window (struct window *w)
{
  if (w->must_be_updated_p)
    {
      struct frame *f = XFRAME (WINDOW_FRAME (w));

      /* Record that this is not a frame-based redisplay.  */
      set_frame_matrix_frame (NULL);

      /* Update W.  */
      update_begin (f);
      update_window (w, true);
      update_end (f);

      /* Reset flag in W.  */
      w->must_be_updated_p = false;
    }
}

#ifdef HAVE_WINDOW_SYSTEM

/* Redraw lines from the current matrix of window W that are
   overlapped by other rows.  YB is bottom-most y-position in W.  */

static void
redraw_overlapped_rows (struct window *w, int yb)
{
  int i;
  struct frame *f = XFRAME (WINDOW_FRAME (w));

  /* If rows overlapping others have been changed, the rows being
     overlapped have to be redrawn.  This won't draw lines that have
     already been drawn in update_window_line because overlapped_p in
     desired rows is 0, so after row assignment overlapped_p in
     current rows is 0.  */
  for (i = 0; i < w->current_matrix->nrows; ++i)
    {
      struct glyph_row *row = w->current_matrix->rows + i;

      if (!row->enabled_p)
	break;
      else if (row->mode_line_p)
	continue;

      if (row->overlapped_p)
	{
	  enum glyph_row_area area;

	  for (area = LEFT_MARGIN_AREA; area < LAST_AREA; area = area + 1)
	    {
	      output_cursor_to (w, i, 0, row->y,
				area == TEXT_AREA ? row->x : 0);
	      if (row->used[area])
		FRAME_RIF (f)->write_glyphs (w, row, row->glyphs[area],
                                             area, row->used[area]);
	      FRAME_RIF (f)->clear_end_of_line (w, row, area, -1);
	    }

	  row->overlapped_p = 0;
	}

      if (MATRIX_ROW_BOTTOM_Y (row) >= yb)
	break;
    }
}


/* Redraw lines from the current matrix of window W that overlap
   others.  YB is bottom-most y-position in W.  */

static void
redraw_overlapping_rows (struct window *w, int yb)
{
  int i, bottom_y;
  struct glyph_row *row;
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));

  for (i = 0; i < w->current_matrix->nrows; ++i)
    {
      row = w->current_matrix->rows + i;

      if (!row->enabled_p)
	break;
      else if (row->mode_line_p)
	continue;

      bottom_y = MATRIX_ROW_BOTTOM_Y (row);

      if (row->overlapping_p)
	{
	  int overlaps = 0;

	  if (MATRIX_ROW_OVERLAPS_PRED_P (row) && i > 0
	      && !MATRIX_ROW (w->current_matrix, i - 1)->overlapped_p)
	    overlaps |= OVERLAPS_PRED;
	  if (MATRIX_ROW_OVERLAPS_SUCC_P (row) && bottom_y < yb
	      && !MATRIX_ROW (w->current_matrix, i + 1)->overlapped_p)
	    overlaps |= OVERLAPS_SUCC;

	  if (overlaps)
	    {
	      if (row->used[LEFT_MARGIN_AREA])
		rif->fix_overlapping_area (w, row, LEFT_MARGIN_AREA, overlaps);

	      if (row->used[TEXT_AREA])
		rif->fix_overlapping_area (w, row, TEXT_AREA, overlaps);

	      if (row->used[RIGHT_MARGIN_AREA])
		rif->fix_overlapping_area (w, row, RIGHT_MARGIN_AREA, overlaps);

	      /* Record in neighbor rows that ROW overwrites part of
		 their display.  */
	      if (overlaps & OVERLAPS_PRED)
		MATRIX_ROW (w->current_matrix, i - 1)->overlapped_p = 1;
	      if (overlaps & OVERLAPS_SUCC)
		MATRIX_ROW (w->current_matrix, i + 1)->overlapped_p = 1;
	    }
	}

      if (bottom_y >= yb)
	break;
    }
}

#endif /* HAVE_WINDOW_SYSTEM */


#if defined GLYPH_DEBUG && 0

/* Check that no row in the current matrix of window W is enabled
   which is below what's displayed in the window.  */

static void
check_current_matrix_flags (struct window *w)
{
  bool last_seen_p = 0;
  int i, yb = window_text_bottom_y (w);

  for (i = 0; i < w->current_matrix->nrows - 1; ++i)
    {
      struct glyph_row *row = MATRIX_ROW (w->current_matrix, i);
      if (!last_seen_p && MATRIX_ROW_BOTTOM_Y (row) >= yb)
	last_seen_p = 1;
      else if (last_seen_p && row->enabled_p)
	emacs_abort ();
    }
}

#endif /* GLYPH_DEBUG */


/* Update display of window W.
   If FORCE_P, don't stop updating when input is pending.  */

static bool
update_window (struct window *w, bool force_p)
{
  struct glyph_matrix *desired_matrix = w->desired_matrix;
  bool paused_p;
  int preempt_count = baud_rate / 2400 + 1;
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));
#ifdef GLYPH_DEBUG
  /* Check that W's frame doesn't have glyph matrices.  */
  eassert (FRAME_WINDOW_P (XFRAME (WINDOW_FRAME (w))));
#endif

  /* Check pending input the first time so that we can quickly return.  */
  if (!force_p)
    detect_input_pending_ignore_squeezables ();

  /* If forced to complete the update, or if no input is pending, do
     the update.  */
  if (force_p || !input_pending || !NILP (do_mouse_tracking))
    {
      struct glyph_row *row, *end;
      struct glyph_row *mode_line_row;
      struct glyph_row *header_line_row;
      int yb;
      bool changed_p = 0, mouse_face_overwritten_p = 0;
      int n_updated = 0;

      rif->update_window_begin_hook (w);
      yb = window_text_bottom_y (w);
      row = MATRIX_ROW (desired_matrix, 0);
      end = MATRIX_MODE_LINE_ROW (desired_matrix);

      /* Take note of the header line, if there is one.  We will
	 update it below, after updating all of the window's lines.  */
      if (row->mode_line_p)
	{
	  header_line_row = row;
	  ++row;
	}
      else
	header_line_row = NULL;

      /* Update the mode line, if necessary.  */
      mode_line_row = MATRIX_MODE_LINE_ROW (desired_matrix);
      if (mode_line_row->mode_line_p && mode_line_row->enabled_p)
	{
	  mode_line_row->y = yb + WINDOW_SCROLL_BAR_AREA_HEIGHT (w);
	  update_window_line (w, MATRIX_ROW_VPOS (mode_line_row,
						  desired_matrix),
			      &mouse_face_overwritten_p);
	}

      /* Find first enabled row.  Optimizations in redisplay_internal
	 may lead to an update with only one row enabled.  There may
	 be also completely empty matrices.  */
      while (row < end && !row->enabled_p)
	++row;

      /* Try reusing part of the display by copying.  */
      if (row < end && !desired_matrix->no_scrolling_p)
	{
	  int rc = scrolling_window (w, header_line_row != NULL);
	  if (rc < 0)
	    {
	      /* All rows were found to be equal.  */
	      paused_p = 0;
	      goto set_cursor;
	    }
	  else if (rc > 0)
	    {
	      /* We've scrolled the display.  */
	      force_p = 1;
	      changed_p = 1;
	    }
	}

      /* Update the rest of the lines.  */
      for (; row < end && (force_p || !input_pending); ++row)
	/* scrolling_window resets the enabled_p flag of the rows it
	   reuses from current_matrix.  */
	if (row->enabled_p)
	  {
	    int vpos = MATRIX_ROW_VPOS (row, desired_matrix);
	    int i;

	    /* We'll have to play a little bit with when to
	       detect_input_pending.  If it's done too often,
	       scrolling large windows with repeated scroll-up
	       commands will too quickly pause redisplay.  */
	    if (!force_p && ++n_updated % preempt_count == 0)
	      detect_input_pending_ignore_squeezables ();
	    changed_p |= update_window_line (w, vpos,
					     &mouse_face_overwritten_p);

	    /* Mark all rows below the last visible one in the current
	       matrix as invalid.  This is necessary because of
	       variable line heights.  Consider the case of three
	       successive redisplays, where the first displays 5
	       lines, the second 3 lines, and the third 5 lines again.
	       If the second redisplay wouldn't mark rows in the
	       current matrix invalid, the third redisplay might be
	       tempted to optimize redisplay based on lines displayed
	       in the first redisplay.  */
	    if (MATRIX_ROW_BOTTOM_Y (row) >= yb)
	      for (i = vpos + 1; i < w->current_matrix->nrows - 1; ++i)
		SET_MATRIX_ROW_ENABLED_P (w->current_matrix, i, false);
	  }

      /* Was display preempted?  */
      paused_p = row < end;

    set_cursor:

      /* Update the header line after scrolling because a new header
	 line would otherwise overwrite lines at the top of the window
	 that can be scrolled.  */
      if (header_line_row && header_line_row->enabled_p)
	{
	  header_line_row->y = 0;
	  update_window_line (w, 0, &mouse_face_overwritten_p);
	}

      /* Fix the appearance of overlapping/overlapped rows.  */
      if (!paused_p && !w->pseudo_window_p)
	{
#ifdef HAVE_WINDOW_SYSTEM
	  if (changed_p && rif->fix_overlapping_area)
	    {
	      redraw_overlapped_rows (w, yb);
	      redraw_overlapping_rows (w, yb);
	    }
#endif

	  /* Make cursor visible at cursor position of W.  */
	  set_window_cursor_after_update (w);

#if 0 /* Check that current matrix invariants are satisfied.  This is
	 for debugging only.  See the comment of check_matrix_invariants.  */
	  IF_DEBUG (check_matrix_invariants (w));
#endif
	}

#ifdef GLYPH_DEBUG
      /* Remember the redisplay method used to display the matrix.  */
      strcpy (w->current_matrix->method, w->desired_matrix->method);
#endif

#ifdef HAVE_WINDOW_SYSTEM
      update_window_fringes (w, 0);
#endif

      /* End the update of window W.  Don't set the cursor if we
         paused updating the display because in this case,
         set_window_cursor_after_update hasn't been called, and
         W->output_cursor doesn't contain the cursor location.  */
      rif->update_window_end_hook (w, !paused_p, mouse_face_overwritten_p);
    }
  else
    paused_p = 1;

#ifdef GLYPH_DEBUG
  /* check_current_matrix_flags (w); */
  add_window_display_history (w, w->current_matrix->method, paused_p);
#endif

  xwidget_end_redisplay (w, w->current_matrix);
  clear_glyph_matrix (desired_matrix);

  return paused_p;
}


/* Update the display of area AREA in window W, row number VPOS.
   AREA can be either LEFT_MARGIN_AREA or RIGHT_MARGIN_AREA.  */

static void
update_marginal_area (struct window *w, struct glyph_row *updated_row,
		      enum glyph_row_area area, int vpos)
{
  struct glyph_row *desired_row = MATRIX_ROW (w->desired_matrix, vpos);
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));

  /* Set cursor to start of glyphs, write them, and clear to the end
     of the area.  I don't think that something more sophisticated is
     necessary here, since marginal areas will not be the default.  */
  output_cursor_to (w, vpos, 0, desired_row->y, 0);
  if (desired_row->used[area])
    rif->write_glyphs (w, updated_row, desired_row->glyphs[area],
		       area, desired_row->used[area]);
  rif->clear_end_of_line (w, updated_row, area, -1);
}


/* Update the display of the text area of row VPOS in window W.
   Value is true if display has changed.  */

static bool
update_text_area (struct window *w, struct glyph_row *updated_row, int vpos)
{
  struct glyph_row *current_row = MATRIX_ROW (w->current_matrix, vpos);
  struct glyph_row *desired_row = MATRIX_ROW (w->desired_matrix, vpos);
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));
  bool changed_p = 0;

  /* If rows are at different X or Y, or rows have different height,
     or the current row is marked invalid, write the entire line.  */
  if (!current_row->enabled_p
      || desired_row->y != current_row->y
      || desired_row->ascent != current_row->ascent
      || desired_row->phys_ascent != current_row->phys_ascent
      || desired_row->phys_height != current_row->phys_height
      || desired_row->visible_height != current_row->visible_height
      || current_row->overlapped_p
      /* This next line is necessary for correctly redrawing
	 mouse-face areas after scrolling and other operations.
	 However, it causes excessive flickering when mouse is moved
	 across the mode line.  Luckily, turning it off for the mode
	 line doesn't seem to hurt anything. -- cyd.
         But it is still needed for the header line. -- kfs.  */
      || (current_row->mouse_face_p
	  && !(current_row->mode_line_p && vpos > 0))
      || current_row->x != desired_row->x)
    {
      output_cursor_to (w, vpos, 0, desired_row->y, desired_row->x);

      if (desired_row->used[TEXT_AREA])
	rif->write_glyphs (w, updated_row, desired_row->glyphs[TEXT_AREA],
			   TEXT_AREA, desired_row->used[TEXT_AREA]);

      /* Clear to end of window.  */
      rif->clear_end_of_line (w, updated_row, TEXT_AREA, -1);
      changed_p = 1;

      /* This erases the cursor.  We do this here because
         notice_overwritten_cursor cannot easily check this, which
         might indicate that the whole functionality of
         notice_overwritten_cursor would better be implemented here.
         On the other hand, we need notice_overwritten_cursor as long
         as mouse highlighting is done asynchronously outside of
         redisplay.  */
      if (vpos == w->phys_cursor.vpos)
	w->phys_cursor_on_p = 0;
    }
  else
    {
      int stop, i, x;
      struct glyph *current_glyph = current_row->glyphs[TEXT_AREA];
      struct glyph *desired_glyph = desired_row->glyphs[TEXT_AREA];
      bool overlapping_glyphs_p = current_row->contains_overlapping_glyphs_p;
      int desired_stop_pos = desired_row->used[TEXT_AREA];
      bool abort_skipping = 0;

      /* If the desired row extends its face to the text area end, and
	 unless the current row also does so at the same position,
	 make sure we write at least one glyph, so that the face
	 extension actually takes place.  */
      if (MATRIX_ROW_EXTENDS_FACE_P (desired_row)
	  && (desired_stop_pos < current_row->used[TEXT_AREA]
	      || (desired_stop_pos == current_row->used[TEXT_AREA]
		  && !MATRIX_ROW_EXTENDS_FACE_P (current_row))))
	--desired_stop_pos;

      stop = min (current_row->used[TEXT_AREA], desired_stop_pos);
      i = 0;
      x = desired_row->x;

      /* Loop over glyphs that current and desired row may have
	 in common.  */
      while (i < stop)
	{
	  bool can_skip_p = !abort_skipping;

	  /* Skip over glyphs that both rows have in common.  These
	     don't have to be written.  We can't skip if the last
	     current glyph overlaps the glyph to its right.  For
	     example, consider a current row of `if ' with the `f' in
	     Courier bold so that it overlaps the ` ' to its right.
	     If the desired row is ` ', we would skip over the space
	     after the `if' and there would remain a pixel from the
	     `f' on the screen.  */
	  if (overlapping_glyphs_p && i > 0)
	    {
	      struct glyph *glyph = &current_row->glyphs[TEXT_AREA][i - 1];
	      int left, right;

	      rif->get_glyph_overhangs (glyph, XFRAME (w->frame),
					&left, &right);
	      can_skip_p = (right == 0 && !abort_skipping);
	    }

	  if (can_skip_p)
	    {
	      int start_hpos = i;

	      while (i < stop
		     && GLYPH_EQUAL_P (desired_glyph, current_glyph))
		{
		  x += desired_glyph->pixel_width;
		  ++desired_glyph, ++current_glyph, ++i;
		}

	      /* Consider the case that the current row contains "xxx
		 ppp ggg" in italic Courier font, and the desired row
		 is "xxx ggg".  The character `p' has lbearing, `g'
		 has not.  The loop above will stop in front of the
		 first `p' in the current row.  If we would start
		 writing glyphs there, we wouldn't erase the lbearing
		 of the `p'.  The rest of the lbearing problem is then
		 taken care of by draw_glyphs.  */
	      if (overlapping_glyphs_p
		  && i > 0
		  && i < current_row->used[TEXT_AREA]
		  && (current_row->used[TEXT_AREA]
		      != desired_row->used[TEXT_AREA]))
		{
		  int left, right;

		  rif->get_glyph_overhangs (current_glyph,
					    XFRAME (w->frame),
					    &left, &right);
		  while (left > 0 && i > 0)
		    {
		      --i, --desired_glyph, --current_glyph;
		      x -= desired_glyph->pixel_width;
		      left -= desired_glyph->pixel_width;
		    }

		  /* Abort the skipping algorithm if we end up before
		     our starting point, to avoid looping (bug#1070).
		     This can happen when the lbearing is larger than
		     the pixel width.  */
		  abort_skipping = (i < start_hpos);
		}
	    }

	  /* Try to avoid writing the entire rest of the desired row
	     by looking for a resync point.  This mainly prevents
	     mode line flickering in the case the mode line is in
	     fixed-pitch font, which it usually will be.  */
	  if (i < desired_row->used[TEXT_AREA])
	    {
	      int start_x = x, start_hpos = i;
	      struct glyph *start = desired_glyph;
	      int current_x = x;
	      bool skip_first_p = !can_skip_p;

	      /* Find the next glyph that's equal again.  */
	      while (i < stop
		     && (skip_first_p
			 || !GLYPH_EQUAL_P (desired_glyph, current_glyph))
		     && x == current_x)
		{
		  x += desired_glyph->pixel_width;
		  current_x += current_glyph->pixel_width;
		  ++desired_glyph, ++current_glyph, ++i;
		  skip_first_p = 0;
		}

	      if (i == start_hpos || x != current_x)
		{
		  i = start_hpos;
		  x = start_x;
		  desired_glyph = start;
		  break;
		}

	      output_cursor_to (w, vpos, start_hpos, desired_row->y, start_x);
	      rif->write_glyphs (w, updated_row, start,
				 TEXT_AREA, i - start_hpos);
	      changed_p = 1;
	    }
	}

      /* Write the rest.  */
      if (i < desired_row->used[TEXT_AREA])
	{
	  output_cursor_to (w, vpos, i, desired_row->y, x);
	  rif->write_glyphs (w, updated_row, desired_glyph,
			     TEXT_AREA, desired_row->used[TEXT_AREA] - i);
	  changed_p = 1;
	}

      /* Maybe clear to end of line.  */
      if (MATRIX_ROW_EXTENDS_FACE_P (desired_row))
	{
	  /* If new row extends to the end of the text area, nothing
	     has to be cleared, if and only if we did a write_glyphs
	     above.  This is made sure by setting desired_stop_pos
	     appropriately above.  */
	  eassert (i < desired_row->used[TEXT_AREA]
		   || ((desired_row->used[TEXT_AREA]
			== current_row->used[TEXT_AREA])
		       && MATRIX_ROW_EXTENDS_FACE_P (current_row)));
	}
      else if (MATRIX_ROW_EXTENDS_FACE_P (current_row))
	{
	  /* If old row extends to the end of the text area, clear.  */
	  if (i >= desired_row->used[TEXT_AREA])
	    output_cursor_to (w, vpos, i, desired_row->y,
			      desired_row->pixel_width);
	  rif->clear_end_of_line (w, updated_row, TEXT_AREA, -1);
	  changed_p = 1;
	}
      else if (desired_row->pixel_width < current_row->pixel_width)
	{
	  /* Otherwise clear to the end of the old row.  Everything
	     after that position should be clear already.  */
	  int xlim;

	  if (i >= desired_row->used[TEXT_AREA])
	    output_cursor_to (w, vpos, i, desired_row->y,
			      desired_row->pixel_width);

	  /* If cursor is displayed at the end of the line, make sure
	     it's cleared.  Nowadays we don't have a phys_cursor_glyph
	     with which to erase the cursor (because this method
	     doesn't work with lbearing/rbearing), so we must do it
	     this way.  */
	  if (vpos == w->phys_cursor.vpos
	      && (desired_row->reversed_p
		  ? (w->phys_cursor.hpos < 0)
		  : (w->phys_cursor.hpos >= desired_row->used[TEXT_AREA])))
	    {
	      w->phys_cursor_on_p = 0;
	      xlim = -1;
	    }
	  else
	    xlim = current_row->pixel_width;
	  rif->clear_end_of_line (w, updated_row, TEXT_AREA, xlim);
	  changed_p = 1;
	}
    }

  return changed_p;
}


/* Update row VPOS in window W.  Value is true if display has been changed.  */

static bool
update_window_line (struct window *w, int vpos, bool *mouse_face_overwritten_p)
{
  struct glyph_row *current_row = MATRIX_ROW (w->current_matrix, vpos);
  struct glyph_row *desired_row = MATRIX_ROW (w->desired_matrix, vpos);
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));
  bool changed_p = 0;

  /* A row can be completely invisible in case a desired matrix was
     built with a vscroll and then make_cursor_line_fully_visible shifts
     the matrix.  Make sure to make such rows current anyway, since
     we need the correct y-position, for example, in the current matrix.  */
  if (desired_row->mode_line_p
      || desired_row->visible_height > 0)
    {
      eassert (desired_row->enabled_p);

      /* Update display of the left margin area, if there is one.  */
      if (!desired_row->full_width_p && w->left_margin_cols > 0)
	{
	  changed_p = 1;
	  update_marginal_area (w, desired_row, LEFT_MARGIN_AREA, vpos);
	  /* Setting this flag will ensure the vertical border, if
	     any, between this window and the one on its left will be
	     redrawn.  This is necessary because updating the left
	     margin area can potentially draw over the border.  */
	  current_row->redraw_fringe_bitmaps_p = 1;
	}

      /* Update the display of the text area.  */
      if (update_text_area (w, desired_row, vpos))
	{
	  changed_p = 1;
	  if (current_row->mouse_face_p)
	    *mouse_face_overwritten_p = 1;
	}

      /* Update display of the right margin area, if there is one.  */
      if (!desired_row->full_width_p && w->right_margin_cols > 0)
	{
	  changed_p = 1;
	  update_marginal_area (w, desired_row, RIGHT_MARGIN_AREA, vpos);
	}

      /* Draw truncation marks etc.  */
      if (!current_row->enabled_p
	  || desired_row->y != current_row->y
	  || desired_row->visible_height != current_row->visible_height
	  || desired_row->cursor_in_fringe_p != current_row->cursor_in_fringe_p
	  || desired_row->overlay_arrow_bitmap != current_row->overlay_arrow_bitmap
	  || current_row->redraw_fringe_bitmaps_p
	  || desired_row->mode_line_p != current_row->mode_line_p
	  || desired_row->exact_window_width_line_p != current_row->exact_window_width_line_p
	  || (MATRIX_ROW_CONTINUATION_LINE_P (desired_row)
	      != MATRIX_ROW_CONTINUATION_LINE_P (current_row)))
	rif->after_update_window_line_hook (w, desired_row);
    }

  /* Update current_row from desired_row.  */
  make_current (w->desired_matrix, w->current_matrix, vpos);
  return changed_p;
}


/* Set the cursor after an update of window W.  This function may only
   be called from update_window.  */

static void
set_window_cursor_after_update (struct window *w)
{
  struct frame *f = XFRAME (w->frame);
  int cx, cy, vpos, hpos;

  /* Not intended for frame matrix updates.  */
  eassert (FRAME_WINDOW_P (f));

  if (cursor_in_echo_area
      && !NILP (echo_area_buffer[0])
      /* If we are showing a message instead of the mini-buffer,
	 show the cursor for the message instead.  */
      && XWINDOW (minibuf_window) == w
      && EQ (minibuf_window, echo_area_window)
      /* These cases apply only to the frame that contains
	 the active mini-buffer window.  */
      && FRAME_HAS_MINIBUF_P (f)
      && EQ (FRAME_MINIBUF_WINDOW (f), echo_area_window))
    {
      cx = cy = vpos = hpos = 0;

      /* If the mini-buffer is several lines high, find the last
	 line that has any text on it.  Note: either all lines
	 are enabled or none.  Otherwise we wouldn't be able to
	 determine Y.  */
      struct glyph_row *last_row = NULL;
      int yb = window_text_bottom_y (w);

      for (struct glyph_row *row = w->current_matrix->rows;
	   row->enabled_p && (!last_row || MATRIX_ROW_BOTTOM_Y (row) <= yb);
	   row++)
	if (row->used[TEXT_AREA] && row->glyphs[TEXT_AREA][0].charpos >= 0)
	  last_row = row;

      if (last_row)
	{
	  struct glyph *start = last_row->glyphs[TEXT_AREA];
	  struct glyph *last = start + last_row->used[TEXT_AREA] - 1;

	  while (last > start && last->charpos < 0)
	    --last;

	  for (struct glyph *glyph = start; glyph < last; glyph++)
	    {
	      cx += glyph->pixel_width;
	      hpos++;
	    }

	  cy = last_row->y;
	  vpos = MATRIX_ROW_VPOS (last_row, w->current_matrix);
	}
    }
  else
    {
      cx = w->cursor.x;
      cy = w->cursor.y;
      hpos = w->cursor.hpos;
      vpos = w->cursor.vpos;
    }

  /* Window cursor can be out of sync for horizontally split windows.
     Horizontal position is -1 when cursor is on the left fringe.   */
  hpos = clip_to_bounds (-1, hpos, w->current_matrix->matrix_w - 1);
  vpos = clip_to_bounds (0, vpos, w->current_matrix->nrows - 1);
  output_cursor_to (w, vpos, hpos, cy, cx);
}


/* Set WINDOW->must_be_updated_p to ON_P for all windows in
   the window tree rooted at W.  */

static void
set_window_update_flags (struct window *w, bool on_p)
{
  while (w)
    {
      if (WINDOWP (w->contents))
	set_window_update_flags (XWINDOW (w->contents), on_p);
      else
	w->must_be_updated_p = on_p;

      w = NILP (w->next) ? 0 : XWINDOW (w->next);
    }
}



/***********************************************************************
			Window-Based Scrolling
 ***********************************************************************/

/* Structure describing rows in scrolling_window.  */

struct row_entry
{
  /* Number of occurrences of this row in desired and current matrix.  */
  int old_uses, new_uses;

  /* Vpos of row in new matrix.  */
  int new_line_number;

  /* Bucket index of this row_entry in the hash table row_table.  */
  ptrdiff_t bucket;

  /* The row described by this entry.  */
  struct glyph_row *row;

  /* Hash collision chain.  */
  struct row_entry *next;
};

/* A pool to allocate row_entry structures from, and the size of the
   pool.  The pool is reallocated in scrolling_window when we find
   that we need a larger one.  */

static struct row_entry *row_entry_pool;
static ptrdiff_t row_entry_pool_size;

/* Index of next free entry in row_entry_pool.  */

static ptrdiff_t row_entry_idx;

/* The hash table used during scrolling, and the table's size.  This
   table is used to quickly identify equal rows in the desired and
   current matrix.  */

static struct row_entry **row_table;
static ptrdiff_t row_table_size;

/* Vectors of pointers to row_entry structures belonging to the
   current and desired matrix, and the size of the vectors.  */

static struct row_entry **old_lines, **new_lines;
static ptrdiff_t old_lines_size, new_lines_size;

/* A pool to allocate run structures from, and its size.  */

static struct run *run_pool;
static ptrdiff_t runs_size;

/* A vector of runs of lines found during scrolling.  */

static struct run **runs;

/* Add glyph row ROW to the scrolling hash table.  */

static struct row_entry *
add_row_entry (struct glyph_row *row)
{
  struct row_entry *entry;
  ptrdiff_t i = row->hash % row_table_size;

  entry = row_table[i];
  eassert (entry || verify_row_hash (row));
  while (entry && !row_equal_p (entry->row, row, 1))
    entry = entry->next;

  if (entry == NULL)
    {
      entry = row_entry_pool + row_entry_idx++;
      entry->row = row;
      entry->old_uses = entry->new_uses = 0;
      entry->new_line_number = 0;
      entry->bucket = i;
      entry->next = row_table[i];
      row_table[i] = entry;
    }

  return entry;
}


/* Try to reuse part of the current display of W by scrolling lines.
   HEADER_LINE_P means W has a header line.

   The algorithm is taken from Communications of the ACM, Apr78 "A
   Technique for Isolating Differences Between Files."  It should take
   O(N) time.

   A short outline of the steps of the algorithm

   1. Skip lines equal at the start and end of both matrices.

   2. Enter rows in the current and desired matrix into a symbol
   table, counting how often they appear in both matrices.

   3. Rows that appear exactly once in both matrices serve as anchors,
   i.e. we assume that such lines are likely to have been moved.

   4. Starting from anchor lines, extend regions to be scrolled both
   forward and backward.

   Value is

   -1	if all rows were found to be equal.
   0	to indicate that we did not scroll the display, or
   1	if we did scroll.  */

static int
scrolling_window (struct window *w, bool header_line_p)
{
  struct glyph_matrix *desired_matrix = w->desired_matrix;
  struct glyph_matrix *current_matrix = w->current_matrix;
  int yb = window_text_bottom_y (w);
  ptrdiff_t i;
  int j, first_old, first_new, last_old, last_new;
  int nruns, run_idx;
  ptrdiff_t n;
  struct row_entry *entry;
  struct redisplay_interface *rif = FRAME_RIF (XFRAME (WINDOW_FRAME (w)));

  /* Skip over rows equal at the start.  */
  for (i = header_line_p; i < current_matrix->nrows - 1; ++i)
    {
      struct glyph_row *d = MATRIX_ROW (desired_matrix, i);
      struct glyph_row *c = MATRIX_ROW (current_matrix, i);

      if (c->enabled_p
	  && d->enabled_p
	  && !d->redraw_fringe_bitmaps_p
	  && c->y == d->y
	  && MATRIX_ROW_BOTTOM_Y (c) <= yb
	  && MATRIX_ROW_BOTTOM_Y (d) <= yb
	  && row_equal_p (c, d, 1))
	{
	  assign_row (c, d);
	  d->enabled_p = false;
	}
      else
	break;
    }

#ifdef HAVE_XWIDGETS
  /* Currently this seems needed to detect xwidget movement reliably. */
    return 0;
#endif

  /* Give up if some rows in the desired matrix are not enabled.  */
  if (! MATRIX_ROW_ENABLED_P (desired_matrix, i))
    return -1;

  first_old = first_new = i;

  /* Set last_new to the index + 1 of the row that reaches the
     bottom boundary in the desired matrix.  Give up if we find a
     disabled row before we reach the bottom boundary.  */
  i = first_new + 1;
  while (i < desired_matrix->nrows - 1)
    {
      int bottom;

      if (! MATRIX_ROW_ENABLED_P (desired_matrix, i))
	return 0;
      bottom = MATRIX_ROW_BOTTOM_Y (MATRIX_ROW (desired_matrix, i));
      if (bottom <= yb)
	++i;
      if (bottom >= yb)
	break;
    }

  last_new = i;

  /* Set last_old to the index + 1 of the row that reaches the bottom
     boundary in the current matrix.  We don't look at the enabled
     flag here because we plan to reuse part of the display even if
     other parts are disabled.  */
  i = first_old + 1;
  while (i < current_matrix->nrows - 1)
    {
      int bottom = MATRIX_ROW_BOTTOM_Y (MATRIX_ROW (current_matrix, i));
      if (bottom <= yb)
	++i;
      if (bottom >= yb)
	break;
    }

  last_old = i;

  /* Skip over rows equal at the bottom.  */
  i = last_new;
  j = last_old;
  while (i - 1 > first_new
         && j - 1 > first_old
         && MATRIX_ROW_ENABLED_P (current_matrix, j - 1)
	 && (MATRIX_ROW (current_matrix, j - 1)->y
	     == MATRIX_ROW (desired_matrix, i - 1)->y)
	 && !MATRIX_ROW (desired_matrix, i - 1)->redraw_fringe_bitmaps_p
         && row_equal_p (MATRIX_ROW (desired_matrix, i - 1),
                         MATRIX_ROW (current_matrix, j - 1), 1))
    --i, --j;
  last_new = i;
  last_old = j;

  /* Nothing to do if all rows are equal.  */
  if (last_new == first_new)
    return 0;

  /* Check for integer overflow in size calculation.

     If next_almost_prime checks (N) for divisibility by 2..10, then
     it can return at most N + 10, e.g., next_almost_prime (1) == 11.
     So, set next_almost_prime_increment_max to 10.

     It's just a coincidence that next_almost_prime_increment_max ==
     NEXT_ALMOST_PRIME_LIMIT - 1.  If NEXT_ALMOST_PRIME_LIMIT were
     13, then next_almost_prime_increment_max would be 14, e.g.,
     because next_almost_prime (113) would be 127.  */
  {
    verify (NEXT_ALMOST_PRIME_LIMIT == 11);
    enum { next_almost_prime_increment_max = 10 };
    ptrdiff_t row_table_max =
      (min (PTRDIFF_MAX, SIZE_MAX) / (3 * sizeof *row_table)
       - next_almost_prime_increment_max);
    ptrdiff_t current_nrows_max = row_table_max - desired_matrix->nrows;
    if (current_nrows_max < current_matrix->nrows)
      memory_full (SIZE_MAX);
  }

  /* Reallocate vectors, tables etc. if necessary.  */

  if (current_matrix->nrows > old_lines_size)
    old_lines = xpalloc (old_lines, &old_lines_size,
			 current_matrix->nrows - old_lines_size,
			 INT_MAX, sizeof *old_lines);

  if (desired_matrix->nrows > new_lines_size)
    new_lines = xpalloc (new_lines, &new_lines_size,
			 desired_matrix->nrows - new_lines_size,
			 INT_MAX, sizeof *new_lines);

  n = desired_matrix->nrows;
  n += current_matrix->nrows;
  if (row_table_size < 3 * n)
    {
      ptrdiff_t size = next_almost_prime (3 * n);
      row_table = xnrealloc (row_table, size, sizeof *row_table);
      row_table_size = size;
      memset (row_table, 0, size * sizeof *row_table);
    }

  if (n > row_entry_pool_size)
    row_entry_pool = xpalloc (row_entry_pool, &row_entry_pool_size,
			      n - row_entry_pool_size,
			      -1, sizeof *row_entry_pool);

  if (desired_matrix->nrows > runs_size)
    {
      runs = xnrealloc (runs, desired_matrix->nrows, sizeof *runs);
      run_pool = xnrealloc (run_pool, desired_matrix->nrows, sizeof *run_pool);
      runs_size = desired_matrix->nrows;
    }

  nruns = run_idx = 0;
  row_entry_idx = 0;

  /* Add rows from the current and desired matrix to the hash table
     row_hash_table to be able to find equal ones quickly.  */

  for (i = first_old; i < last_old; ++i)
    {
      if (MATRIX_ROW_ENABLED_P (current_matrix, i))
	{
	  entry = add_row_entry (MATRIX_ROW (current_matrix, i));
	  old_lines[i] = entry;
	  ++entry->old_uses;
	}
      else
	old_lines[i] = NULL;
    }

  for (i = first_new; i < last_new; ++i)
    {
      eassert (MATRIX_ROW_ENABLED_P (desired_matrix, i));
      entry = add_row_entry (MATRIX_ROW (desired_matrix, i));
      ++entry->new_uses;
      entry->new_line_number = i;
      new_lines[i] = entry;
    }

  /* Identify moves based on lines that are unique and equal
     in both matrices.  */
  for (i = first_old; i < last_old;)
    if (old_lines[i]
	&& old_lines[i]->old_uses == 1
        && old_lines[i]->new_uses == 1)
      {
	int p, q;
	int new_line = old_lines[i]->new_line_number;
	struct run *run = run_pool + run_idx++;

	/* Record move.  */
	run->current_vpos = i;
	run->current_y = MATRIX_ROW (current_matrix, i)->y;
	run->desired_vpos = new_line;
	run->desired_y = MATRIX_ROW (desired_matrix, new_line)->y;
	run->nrows = 1;
	run->height = MATRIX_ROW (current_matrix, i)->height;

	/* Extend backward.  */
	p = i - 1;
	q = new_line - 1;
	while (p > first_old
	       && q > first_new
	       && old_lines[p] == new_lines[q])
	  {
	    int h = MATRIX_ROW (current_matrix, p)->height;
	    --run->current_vpos;
	    --run->desired_vpos;
	    ++run->nrows;
	    run->height += h;
	    run->desired_y -= h;
	    run->current_y -= h;
	    --p, --q;
	  }

	/* Extend forward.  */
	p = i + 1;
	q = new_line + 1;
	while (p < last_old
	       && q < last_new
	       && old_lines[p] == new_lines[q])
	  {
	    int h = MATRIX_ROW (current_matrix, p)->height;
	    ++run->nrows;
	    run->height += h;
	    ++p, ++q;
	  }

	/* Insert run into list of all runs.  Order runs by copied
	   pixel lines.  Note that we record runs that don't have to
	   be copied because they are already in place.  This is done
	   because we can avoid calling update_window_line in this
	   case.  */
	for (p = 0; p < nruns && runs[p]->height > run->height; ++p)
	  ;
	for (q = nruns; q > p; --q)
	  runs[q] = runs[q - 1];
	runs[p] = run;
	++nruns;

	i += run->nrows;
      }
    else
      ++i;

  /* Do the moves.  Do it in a way that we don't overwrite something
     we want to copy later on.  This is not solvable in general
     because there is only one display and we don't have a way to
     exchange areas on this display.  Example:

          +-----------+       +-----------+
          |     A     |       |     B     |
          +-----------+  -->  +-----------+
          |     B     |       |     A     |
          +-----------+       +-----------+

     Instead, prefer bigger moves, and invalidate moves that would
     copy from where we copied to.  */

  for (i = 0; i < nruns; ++i)
    if (runs[i]->nrows > 0)
      {
	struct run *r = runs[i];

	/* Copy on the display.  */
	if (r->current_y != r->desired_y)
	  {
	    rif->clear_window_mouse_face (w);
	    rif->scroll_run_hook (w, r);
	  }

	/* Truncate runs that copy to where we copied to, and
	   invalidate runs that copy from where we copied to.  */
	for (j = nruns - 1; j > i; --j)
	  {
	    struct run *p = runs[j];
	    bool truncated_p = 0;

	    if (p->nrows > 0
		&& p->desired_y < r->desired_y + r->height
		&& p->desired_y + p->height > r->desired_y)
	      {
		if (p->desired_y < r->desired_y)
		  {
		    p->nrows = r->desired_vpos - p->desired_vpos;
		    p->height = r->desired_y - p->desired_y;
		    truncated_p = 1;
		  }
		else
		  {
		    int nrows_copied = (r->desired_vpos + r->nrows
					- p->desired_vpos);

		    if (p->nrows <= nrows_copied)
		      p->nrows = 0;
		    else
		      {
			int height_copied = (r->desired_y + r->height
					     - p->desired_y);

			p->current_vpos += nrows_copied;
			p->desired_vpos += nrows_copied;
			p->nrows -= nrows_copied;
			p->current_y += height_copied;
			p->desired_y += height_copied;
			p->height -= height_copied;
			truncated_p = 1;
		      }
		  }
	      }

	    if (r->current_y != r->desired_y
		/* The condition below is equivalent to
		   ((p->current_y >= r->desired_y
		     && p->current_y < r->desired_y + r->height)
		    || (p->current_y + p->height > r->desired_y
			&& (p->current_y + p->height
			    <= r->desired_y + r->height)))
		   because we have 0 < p->height <= r->height.  */
		&& p->current_y < r->desired_y + r->height
		&& p->current_y + p->height > r->desired_y)
	      p->nrows = 0;

	    /* Reorder runs by copied pixel lines if truncated.  */
	    if (truncated_p && p->nrows > 0)
	      {
		int k = nruns - 1;

		while (runs[k]->nrows == 0 || runs[k]->height < p->height)
		  k--;
		memmove (runs + j, runs + j + 1, (k - j) * sizeof (*runs));
		runs[k] = p;
	      }
	  }

	/* Assign matrix rows.  */
	for (j = 0; j < r->nrows; ++j)
	  {
	    struct glyph_row *from, *to;
	    bool to_overlapped_p;

	    to = MATRIX_ROW (current_matrix, r->desired_vpos + j);
	    from = MATRIX_ROW (desired_matrix, r->desired_vpos + j);
	    to_overlapped_p = to->overlapped_p;
	    from->redraw_fringe_bitmaps_p = from->fringe_bitmap_periodic_p;
	    assign_row (to, from);
	    /* The above `assign_row' actually does swap, so if we had
	       an overlap in the copy destination of two runs, then
	       the second run would assign a previously disabled bogus
	       row.  But thanks to the truncation code in the
	       preceding for-loop, we no longer have such an overlap,
	       and thus the assigned row should always be enabled.  */
	    eassert (to->enabled_p);
	    from->enabled_p = false;
	    to->overlapped_p = to_overlapped_p;
	  }
      }

  /* Clear the hash table, for the next time.  */
  for (i = 0; i < row_entry_idx; ++i)
    row_table[row_entry_pool[i].bucket] = NULL;

  /* Value is 1 to indicate that we scrolled the display.  */
  return nruns > 0;
}



/************************************************************************
			 Frame-Based Updates
 ************************************************************************/

/* Update the desired frame matrix of frame F.

   FORCE_P means that the update should not be stopped by pending input.
   INHIBIT_ID_P means that scrolling by insert/delete should not be tried.
   SET_CURSOR_P false means do not set cursor at point in selected window.

   Value is true if update was stopped due to pending input.  */

static bool
update_frame_1 (struct frame *f, bool force_p, bool inhibit_id_p,
		bool set_cursor_p, bool updating_menu_p)
{
  /* Frame matrices to work on.  */
  struct glyph_matrix *current_matrix = f->current_matrix;
  struct glyph_matrix *desired_matrix = f->desired_matrix;
  int i;
  bool pause_p;
  int preempt_count = baud_rate / 2400 + 1;

  eassert (current_matrix && desired_matrix);

  if (baud_rate != FRAME_COST_BAUD_RATE (f))
    calculate_costs (f);

  if (preempt_count <= 0)
    preempt_count = 1;

  if (!force_p && detect_input_pending_ignore_squeezables ())
    {
      pause_p = 1;
      goto do_pause;
    }

  /* If we cannot insert/delete lines, it's no use trying it.  */
  if (!FRAME_LINE_INS_DEL_OK (f))
    inhibit_id_p = 1;

  /* See if any of the desired lines are enabled; don't compute for
     i/d line if just want cursor motion.  */
  for (i = 0; i < desired_matrix->nrows; i++)
    if (MATRIX_ROW_ENABLED_P (desired_matrix, i))
      break;

  /* Try doing i/d line, if not yet inhibited.  */
  if (!inhibit_id_p && i < desired_matrix->nrows)
    force_p |= scrolling (f);

  /* Update the individual lines as needed.  Do bottom line first.  */
  if (MATRIX_ROW_ENABLED_P (desired_matrix, desired_matrix->nrows - 1))
    update_frame_line (f, desired_matrix->nrows - 1, updating_menu_p);

  /* Now update the rest of the lines.  */
  for (i = 0; i < desired_matrix->nrows - 1 && (force_p || !input_pending); i++)
    {
      if (MATRIX_ROW_ENABLED_P (desired_matrix, i))
	{
	  if (FRAME_TERMCAP_P (f))
	    {
	      /* Flush out every so many lines.
		 Also flush out if likely to have more than 1k buffered
		 otherwise.   I'm told that some telnet connections get
		 really screwed by more than 1k output at once.  */
	      FILE *display_output = FRAME_TTY (f)->output;
	      if (display_output)
		{
		  ptrdiff_t outq = __fpending (display_output);
		  if (outq > 900
		      || (outq > 20 && ((i - 1) % preempt_count == 0)))
		    fflush_unlocked (display_output);
		}
	    }

	  if (!force_p && (i - 1) % preempt_count == 0)
	    detect_input_pending_ignore_squeezables ();

	  update_frame_line (f, i, updating_menu_p);
	}
    }

  pause_p = 0 < i && i < FRAME_TOTAL_LINES (f) - 1;

  /* Now just clean up termcap drivers and set cursor, etc.  */
  if (!pause_p && set_cursor_p)
    {
      if ((cursor_in_echo_area
	   /* If we are showing a message instead of the mini-buffer,
	      show the cursor for the message instead of for the
	      (now hidden) mini-buffer contents.  */
	   || (EQ (minibuf_window, selected_window)
	       && EQ (minibuf_window, echo_area_window)
	       && !NILP (echo_area_buffer[0])))
	  /* These cases apply only to the frame that contains
	     the active mini-buffer window.  */
	  && FRAME_HAS_MINIBUF_P (f)
	  && EQ (FRAME_MINIBUF_WINDOW (f), echo_area_window))
	{
	  int top = WINDOW_TOP_EDGE_LINE (XWINDOW (FRAME_MINIBUF_WINDOW (f)));
	  int col;

	  /* Put cursor at the end of the prompt.  If the mini-buffer
	     is several lines high, find the last line that has
	     any text on it.  */
	  int row = FRAME_TOTAL_LINES (f);
	  do
	    {
	      row--;
	      col = 0;

	      if (MATRIX_ROW_ENABLED_P (current_matrix, row))
		{
		  /* Frame rows are filled up with spaces that
		     must be ignored here.  */
		  struct glyph_row *r = MATRIX_ROW (current_matrix, row);
		  struct glyph *start = r->glyphs[TEXT_AREA];

		  col = r->used[TEXT_AREA];
		  while (0 < col && start[col - 1].charpos < 0)
		    col--;
		}
	    }
	  while (row > top && col == 0);

	  /* Make sure COL is not out of range.  */
	  if (col >= FRAME_CURSOR_X_LIMIT (f))
	    {
	      /* If we have another row, advance cursor into it.  */
	      if (row < FRAME_TOTAL_LINES (f) - 1)
		{
		  col = FRAME_LEFT_SCROLL_BAR_COLS (f);
		  row++;
		}
	      /* Otherwise move it back in range.  */
	      else
		col = FRAME_CURSOR_X_LIMIT (f) - 1;
	    }

	  cursor_to (f, row, col);
	}
      else
	{
	  /* We have only one cursor on terminal frames.  Use it to
	     display the cursor of the selected window.  */
	  struct window *w = XWINDOW (FRAME_SELECTED_WINDOW (f));
	  if (w->cursor.vpos >= 0
	      /* The cursor vpos may be temporarily out of bounds
	         in the following situation:  There is one window,
		 with the cursor in the lower half of it.  The window
		 is split, and a message causes a redisplay before
	         a new cursor position has been computed.  */
	      && w->cursor.vpos < WINDOW_TOTAL_LINES (w))
	    {
	      int x = WINDOW_TO_FRAME_HPOS (w, w->cursor.hpos);
	      int y = WINDOW_TO_FRAME_VPOS (w, w->cursor.vpos);

	      x += max (0, w->left_margin_cols);
	      cursor_to (f, y, x);
	    }
	}
    }

 do_pause:

  clear_desired_matrices (f);
  return pause_p;
}


/* Do line insertions/deletions on frame F for frame-based redisplay.  */

static bool
scrolling (struct frame *frame)
{
  int unchanged_at_top, unchanged_at_bottom;
  int window_size;
  int changed_lines;
  int i;
  int height = FRAME_TOTAL_LINES (frame);
  int free_at_end_vpos = height;
  struct glyph_matrix *current_matrix = frame->current_matrix;
  struct glyph_matrix *desired_matrix = frame->desired_matrix;
  verify (sizeof (int) <= sizeof (unsigned));
  verify (alignof (unsigned) % alignof (int) == 0);
  unsigned *old_hash;
  USE_SAFE_ALLOCA;
  SAFE_NALLOCA (old_hash, 4, height);
  unsigned *new_hash = old_hash + height;
  int *draw_cost = (int *) (new_hash + height);
  int *old_draw_cost = draw_cost + height;
  old_hash = ptr_bounds_clip (old_hash, height * sizeof *old_hash);
  new_hash = ptr_bounds_clip (new_hash, height * sizeof *new_hash);
  draw_cost = ptr_bounds_clip (draw_cost, height * sizeof *draw_cost);
  old_draw_cost = ptr_bounds_clip (old_draw_cost,
				   height * sizeof *old_draw_cost);

  eassert (current_matrix);

  /* Compute hash codes of all the lines.  Also calculate number of
     changed lines, number of unchanged lines at the beginning, and
     number of unchanged lines at the end.  */
  changed_lines = 0;
  unchanged_at_top = 0;
  unchanged_at_bottom = height;
  for (i = 0; i < height; i++)
    {
      /* Give up on this scrolling if some old lines are not enabled.  */
      if (!MATRIX_ROW_ENABLED_P (current_matrix, i))
	{
	  SAFE_FREE ();
	  return false;
	}
      old_hash[i] = line_hash_code (frame, MATRIX_ROW (current_matrix, i));
      if (! MATRIX_ROW_ENABLED_P (desired_matrix, i))
	{
	  /* This line cannot be redrawn, so don't let scrolling mess it.  */
	  new_hash[i] = old_hash[i];
#undef INFINITY
#define INFINITY 1000000	/* Taken from scroll.c */
#undef C_INFINITY
#define C_INFINITY 1000000	/* Taken from scroll.c */
	  draw_cost[i] = INFINITY;
	}
      else
	{
	  new_hash[i] = line_hash_code (frame, MATRIX_ROW (desired_matrix, i));
	  draw_cost[i] = line_draw_cost (frame, desired_matrix, i);
	}

      if (old_hash[i] != new_hash[i])
	{
	  changed_lines++;
	  unchanged_at_bottom = height - i - 1;
	}
      else if (i == unchanged_at_top)
	unchanged_at_top++;
      old_draw_cost[i] = line_draw_cost (frame, current_matrix, i);
    }

  /* If changed lines are few, don't allow preemption, don't scroll.  */
  if ((!FRAME_SCROLL_REGION_OK (frame)
       && changed_lines < baud_rate / 2400)
      || unchanged_at_bottom == height)
    {
      SAFE_FREE ();
      return true;
    }

  window_size = (height - unchanged_at_top
		 - unchanged_at_bottom);

  if (FRAME_SCROLL_REGION_OK (frame))
    free_at_end_vpos -= unchanged_at_bottom;
  else if (FRAME_MEMORY_BELOW_FRAME (frame))
    free_at_end_vpos = -1;

  /* Do id/calc only if small window, or slow terminal, or many lines
     in common between current frame and desired frame.  But the
     window size must be at least 2.  */
  if ((FRAME_SCROLL_REGION_OK (frame)
       || window_size < 18 || baud_rate <= 2400
       || (window_size
	   < 10 * scrolling_max_lines_saved (unchanged_at_top,
					     height - unchanged_at_bottom,
					     old_hash, new_hash, draw_cost)))
      && 2 <= window_size)
    scrolling_1 (frame, window_size, unchanged_at_top, unchanged_at_bottom,
		 draw_cost + unchanged_at_top - 1,
		 old_draw_cost + unchanged_at_top - 1,
		 old_hash + unchanged_at_top - 1,
		 new_hash + unchanged_at_top - 1,
		 free_at_end_vpos - unchanged_at_top);

  SAFE_FREE ();
  return false;
}


/* Count the number of blanks at the start of the vector of glyphs R
   which is LEN glyphs long.  */

static int
count_blanks (struct glyph *r, int len)
{
  int i;

  for (i = 0; i < len; ++i)
    if (!CHAR_GLYPH_SPACE_P (r[i]))
      break;

  return i;
}


/* Count the number of glyphs in common at the start of the glyph
   vectors STR1 and STR2.  END1 is the end of STR1 and END2 is the end
   of STR2.  Value is the number of equal glyphs equal at the start.  */

static int
count_match (struct glyph *str1, struct glyph *end1, struct glyph *str2, struct glyph *end2)
{
  struct glyph *p1 = str1;
  struct glyph *p2 = str2;

  while (p1 < end1
	 && p2 < end2
	 && GLYPH_CHAR_AND_FACE_EQUAL_P (p1, p2))
    ++p1, ++p2;

  return p1 - str1;
}


/* Char insertion/deletion cost vector, from term.c */

#define char_ins_del_cost(f) (&char_ins_del_vector[FRAME_TOTAL_COLS ((f))])


/* Perform a frame-based update on line VPOS in frame FRAME.  */

static void
update_frame_line (struct frame *f, int vpos, bool updating_menu_p)
{
  struct glyph *obody, *nbody, *op1, *op2, *np1, *nend;
  int tem;
  int osp, nsp, begmatch, endmatch, olen, nlen;
  struct glyph_matrix *current_matrix = f->current_matrix;
  struct glyph_matrix *desired_matrix = f->desired_matrix;
  struct glyph_row *current_row = MATRIX_ROW (current_matrix, vpos);
  struct glyph_row *desired_row = MATRIX_ROW (desired_matrix, vpos);
  bool must_write_whole_line_p;
  bool write_spaces_p = FRAME_MUST_WRITE_SPACES (f);
  bool colored_spaces_p = (FACE_FROM_ID (f, DEFAULT_FACE_ID)->background
			   != FACE_TTY_DEFAULT_BG_COLOR);

  if (colored_spaces_p)
    write_spaces_p = 1;

  /* Current row not enabled means it has unknown contents.  We must
     write the whole desired line in that case.  */
  must_write_whole_line_p = !current_row->enabled_p;
  if (must_write_whole_line_p)
    {
      obody = 0;
      olen = 0;
    }
  else
    {
      obody = MATRIX_ROW_GLYPH_START (current_matrix, vpos);
      olen = current_row->used[TEXT_AREA];

      /* Ignore trailing spaces, if we can.  */
      if (!write_spaces_p)
	while (olen > 0 && CHAR_GLYPH_SPACE_P (obody[olen-1]))
	  olen--;
    }

  current_row->enabled_p = true;
  current_row->used[TEXT_AREA] = desired_row->used[TEXT_AREA];

  /* For some reason, cursor is sometimes moved behind our back when a
     frame with a TTY menu is redrawn.  Homing the cursor as below
     fixes that.  */
  if (updating_menu_p)
    cursor_to (f, 0, 0);

  /* If desired line is empty, just clear the line.  */
  if (!desired_row->enabled_p)
    {
      nlen = 0;
      goto just_erase;
    }

  nbody = desired_row->glyphs[TEXT_AREA];
  nlen = desired_row->used[TEXT_AREA];
  nend = nbody + nlen;

  /* If display line has unknown contents, write the whole line.  */
  if (must_write_whole_line_p)
    {
      /* Ignore spaces at the end, if we can.  */
      if (!write_spaces_p)
	while (nlen > 0 && CHAR_GLYPH_SPACE_P (nbody[nlen - 1]))
	  --nlen;

      /* Write the contents of the desired line.  */
      if (nlen)
	{
          cursor_to (f, vpos, 0);
	  write_glyphs (f, nbody, nlen);
	}

      /* Don't call clear_end_of_line if we already wrote the whole
	 line.  The cursor will not be at the right margin in that
	 case but in the line below.  */
      if (nlen < FRAME_TOTAL_COLS (f))
	{
	  cursor_to (f, vpos, nlen);
          clear_end_of_line (f, FRAME_TOTAL_COLS (f));
	}
      else
	/* Make sure we are in the right row, otherwise cursor movement
	   with cmgoto might use `ch' in the wrong row.  */
	cursor_to (f, vpos, 0);

      make_current (desired_matrix, current_matrix, vpos);
      return;
    }

  /* Pretend trailing spaces are not there at all,
     unless for one reason or another we must write all spaces.  */
  if (!write_spaces_p)
    while (nlen > 0 && CHAR_GLYPH_SPACE_P (nbody[nlen - 1]))
      nlen--;

  /* If there's no i/d char, quickly do the best we can without it.  */
  if (!FRAME_CHAR_INS_DEL_OK (f))
    {
      int i, j;

      /* Find the first glyph in desired row that doesn't agree with
	 a glyph in the current row, and write the rest from there on.  */
      for (i = 0; i < nlen; i++)
	{
	  if (i >= olen || !GLYPH_EQUAL_P (nbody + i, obody + i))
	    {
	      /* Find the end of the run of different glyphs.  */
	      j = i + 1;
	      while (j < nlen
		     && (j >= olen
			 || !GLYPH_EQUAL_P (nbody + j, obody + j)
			 || CHAR_GLYPH_PADDING_P (nbody[j])))
		++j;

	      /* Output this run of non-matching chars.  */
	      cursor_to (f, vpos, i);
	      write_glyphs (f, nbody + i, j - i);
	      i = j - 1;

	      /* Now find the next non-match.  */
	    }
	}

      /* Clear the rest of the line, or the non-clear part of it.  */
      if (olen > nlen)
	{
	  cursor_to (f, vpos, nlen);
	  clear_end_of_line (f, olen);
	}

      /* Make current row = desired row.  */
      make_current (desired_matrix, current_matrix, vpos);
      return;
    }

  /* Here when CHAR_INS_DEL_OK != 0, i.e. we can insert or delete
     characters in a row.  */

  if (!olen)
    {
      /* If current line is blank, skip over initial spaces, if
	 possible, and write the rest.  */
      if (write_spaces_p)
	nsp = 0;
      else
	nsp = count_blanks (nbody, nlen);

      if (nlen > nsp)
	{
	  cursor_to (f, vpos, nsp);
	  write_glyphs (f, nbody + nsp, nlen - nsp);
	}

      /* Exchange contents between current_frame and new_frame.  */
      make_current (desired_matrix, current_matrix, vpos);
      return;
    }

  /* Compute number of leading blanks in old and new contents.  */
  osp = count_blanks (obody, olen);
  nsp = (colored_spaces_p ? 0 : count_blanks (nbody, nlen));

  /* Compute number of matching chars starting with first non-blank.  */
  begmatch = count_match (obody + osp, obody + olen,
			  nbody + nsp, nbody + nlen);

  /* Spaces in new match implicit space past the end of old.  */
  /* A bug causing this to be a no-op was fixed in 18.29.  */
  if (!write_spaces_p && osp + begmatch == olen)
    {
      np1 = nbody + nsp;
      while (np1 + begmatch < nend && CHAR_GLYPH_SPACE_P (np1[begmatch]))
	++begmatch;
    }

  /* Avoid doing insert/delete char
     just cause number of leading spaces differs
     when the following text does not match.  */
  if (begmatch == 0 && osp != nsp)
    osp = nsp = min (osp, nsp);

  /* Find matching characters at end of line */
  op1 = obody + olen;
  np1 = nbody + nlen;
  op2 = op1 + begmatch - min (olen - osp, nlen - nsp);
  while (op1 > op2
	 && GLYPH_EQUAL_P (op1 - 1, np1 - 1))
    {
      op1--;
      np1--;
    }
  endmatch = obody + olen - op1;

  /* tem gets the distance to insert or delete.
     endmatch is how many characters we save by doing so.
     Is it worth it?  */

  tem = (nlen - nsp) - (olen - osp);
  if (endmatch && tem
      && (!FRAME_CHAR_INS_DEL_OK (f)
          || endmatch <= char_ins_del_cost (f)[tem]))
    endmatch = 0;

  /* nsp - osp is the distance to insert or delete.
     If that is nonzero, begmatch is known to be nonzero also.
     begmatch + endmatch is how much we save by doing the ins/del.
     Is it worth it?  */

  if (nsp != osp
      && (!FRAME_CHAR_INS_DEL_OK (f)
	  || begmatch + endmatch <= char_ins_del_cost (f)[nsp - osp]))
    {
      begmatch = 0;
      endmatch = 0;
      osp = nsp = min (osp, nsp);
    }

  /* Now go through the line, inserting, writing and
     deleting as appropriate.  */

  if (osp > nsp)
    {
      cursor_to (f, vpos, nsp);
      delete_glyphs (f, osp - nsp);
    }
  else if (nsp > osp)
    {
      /* If going to delete chars later in line
	 and insert earlier in the line,
	 must delete first to avoid losing data in the insert */
      if (endmatch && nlen < olen + nsp - osp)
	{
	  cursor_to (f, vpos, nlen - endmatch + osp - nsp);
	  delete_glyphs (f, olen + nsp - osp - nlen);
	  olen = nlen - (nsp - osp);
	}
      cursor_to (f, vpos, osp);
      insert_glyphs (f, 0, nsp - osp);
    }
  olen += nsp - osp;

  tem = nsp + begmatch + endmatch;
  if (nlen != tem || olen != tem)
    {
      if (!endmatch || nlen == olen)
	{
	  /* If new text being written reaches right margin, there is
	     no need to do clear-to-eol at the end of this function
	     (and it would not be safe, since cursor is not going to
	     be "at the margin" after the text is done).  */
	  if (nlen == FRAME_TOTAL_COLS (f))
	    olen = 0;

	  /* Function write_glyphs is prepared to do nothing
	     if passed a length <= 0.  Check it here to avoid
	     unnecessary cursor movement.  */
	  if (nlen - tem > 0)
	    {
	      cursor_to (f, vpos, nsp + begmatch);
	      write_glyphs (f, nbody + nsp + begmatch, nlen - tem);
	    }
	}
      else if (nlen > olen)
	{
	  /* Here, we used to have the following simple code:
	     ----------------------------------------
	     write_glyphs (nbody + nsp + begmatch, olen - tem);
	     insert_glyphs (nbody + nsp + begmatch + olen - tem, nlen - olen);
	     ----------------------------------------
	     but it doesn't work if nbody[nsp + begmatch + olen - tem]
	     is a padding glyph.  */
	  int out = olen - tem;	/* Columns to be overwritten originally.  */
	  int del;

	  cursor_to (f, vpos, nsp + begmatch);

	  /* Calculate columns we can actually overwrite.  */
	  while (CHAR_GLYPH_PADDING_P (nbody[nsp + begmatch + out]))
	    out--;
	  write_glyphs (f, nbody + nsp + begmatch, out);

	  /* If we left columns to be overwritten, we must delete them.  */
	  del = olen - tem - out;
	  if (del > 0)
	    delete_glyphs (f, del);

	  /* At last, we insert columns not yet written out.  */
	  insert_glyphs (f, nbody + nsp + begmatch + out, nlen - olen + del);
	  olen = nlen;
	}
      else if (olen > nlen)
	{
	  cursor_to (f, vpos, nsp + begmatch);
	  write_glyphs (f, nbody + nsp + begmatch, nlen - tem);
	  delete_glyphs (f, olen - nlen);
	  olen = nlen;
	}
    }

 just_erase:
  /* If any unerased characters remain after the new line, erase them.  */
  if (olen > nlen)
    {
      cursor_to (f, vpos, nlen);
      clear_end_of_line (f, olen);
    }

  /* Exchange contents between current_frame and new_frame.  */
  make_current (desired_matrix, current_matrix, vpos);
}



/***********************************************************************
		   X/Y Position -> Buffer Position
 ***********************************************************************/

/* Determine what's under window-relative pixel position (*X, *Y).
   Return the OBJECT (string or buffer) that's there.
   Return in *POS the position in that object.
   Adjust *X and *Y to character positions.
   Return in *DX and *DY the pixel coordinates of the click,
   relative to the top left corner of OBJECT, or relative to
   the top left corner of the character glyph at (*X, *Y)
   if OBJECT is nil.
   Return WIDTH and HEIGHT of the object at (*X, *Y), or zero
   if the coordinates point to an empty area of the display.  */

Lisp_Object
buffer_posn_from_coords (struct window *w, int *x, int *y, struct display_pos *pos, Lisp_Object *object, int *dx, int *dy, int *width, int *height)
{
  struct it it;
  Lisp_Object old_current_buffer = Fcurrent_buffer ();
  struct text_pos startp;
  Lisp_Object string;
  struct glyph_row *row;
#ifdef HAVE_WINDOW_SYSTEM
  struct image *img = 0;
#endif
  int x0, x1, to_x, it_vpos;
  void *itdata = NULL;

  /* We used to set current_buffer directly here, but that does the
     wrong thing with `face-remapping-alist' (bug#2044).  */
  Fset_buffer (w->contents);
  itdata = bidi_shelve_cache ();
  CLIP_TEXT_POS_FROM_MARKER (startp, w->start);
  start_display (&it, w, startp);
  x0 = *x;

  /* First, move to the beginning of the row corresponding to *Y.  We
     need to be in that row to get the correct value of base paragraph
     direction for the text at (*X, *Y).  */
  move_it_to (&it, -1, 0, *y, -1, MOVE_TO_X | MOVE_TO_Y);

  /* TO_X is the pixel position that the iterator will compute for the
     glyph at *X.  */
  to_x = x0;
  if (it.bidi_it.paragraph_dir == R2L)
    /* For lines in an R2L paragraph, we need to mirror TO_X wrt the
       text area.  This is because the iterator, even in R2L
       paragraphs, delivers glyphs as if they started at the left
       margin of the window.  (When we actually produce glyphs for
       display, we reverse their order in PRODUCE_GLYPHS, but the
       iterator doesn't know about that.)  The following line adjusts
       the pixel position to the iterator geometry, which is what
       move_it_* routines use.  (The -1 is because in a window whose
       text-area width is W, the rightmost pixel position is W-1, and
       it should be mirrored into zero pixel position.)  */
    to_x = window_box_width (w, TEXT_AREA) - to_x - 1;

  /* We need to add it.first_visible_x because iterator positions
     include the hscroll. */
  to_x += it.first_visible_x;

  /* If we are hscrolling only the current line, and Y is at the line
     containing point, augment TO_X with the hscroll amount of the
     current line.  */
  if (it.line_wrap == TRUNCATE
      && EQ (automatic_hscrolling, Qcurrent_line) && IT_CHARPOS (it) < PT)
    {
      struct it it2 = it;
      void *it2data = bidi_shelve_cache ();
      it2.last_visible_x = 1000000;
      /* If the line at Y shows point, the call below to
	 move_it_in_display_line will succeed in reaching point.  */
      move_it_in_display_line (&it2, PT, -1, MOVE_TO_POS);
      if (IT_CHARPOS (it2) >= PT)
	{
	  to_x += (w->hscroll - w->min_hscroll) * FRAME_COLUMN_WIDTH (it.f);
	  /* We need to pretend the window is hscrolled, so that
	     move_it_in_display_line below will DTRT with TO_X.  */
	  it.first_visible_x += w->hscroll * FRAME_COLUMN_WIDTH (it.f);
	  it.last_visible_x += w->hscroll * FRAME_COLUMN_WIDTH (it.f);
	}
      bidi_unshelve_cache (it2data, 0);
    }

  /* Now move horizontally in the row to the glyph under *X.  Second
     argument is ZV to prevent move_it_in_display_line from matching
     based on buffer positions.  */
  move_it_in_display_line (&it, ZV, to_x, MOVE_TO_X);
  bidi_unshelve_cache (itdata, 0);

  Fset_buffer (old_current_buffer);

  *dx = to_x - it.current_x;
  *dy = *y - it.current_y;

  string = w->contents;
  if (STRINGP (it.string))
    string = it.string;
  *pos = it.current;
  if (it.what == IT_COMPOSITION
      && it.cmp_it.nchars > 1
      && it.cmp_it.reversed_p)
    {
      /* The current display element is a grapheme cluster in a
	 composition.  In that case, we need the position of the first
	 character of the cluster.  But, as it.cmp_it.reversed_p is 1,
	 it.current points to the last character of the cluster, thus
	 we must move back to the first character of the same
	 cluster.  */
      CHARPOS (pos->pos) -= it.cmp_it.nchars - 1;
      if (STRINGP (it.string))
	BYTEPOS (pos->pos) = string_char_to_byte (string, CHARPOS (pos->pos));
      else
	BYTEPOS (pos->pos) = buf_charpos_to_bytepos (XBUFFER (w->contents),
						     CHARPOS (pos->pos));
    }

#ifdef HAVE_WINDOW_SYSTEM
  if (it.what == IT_IMAGE)
    {
      /* Note that this ignores images that are fringe bitmaps,
	 because their image ID is zero, and so IMAGE_OPT_FROM_ID will
	 return NULL.  This is okay, since fringe bitmaps are not
	 displayed in the text area, and so are never the object we
	 are interested in.  */
      img = IMAGE_OPT_FROM_ID (it.f, it.image_id);
      if (img && !NILP (img->spec))
	*object = img->spec;
    }
#endif

  /* IT's vpos counts from the glyph row that includes the window's
     start position, i.e. it excludes the header-line row, but
     MATRIX_ROW includes the header-line row.  Adjust for a possible
     header-line row.  */
  it_vpos = it.vpos + window_wants_header_line (w);
  if (it_vpos < w->current_matrix->nrows
      && (row = MATRIX_ROW (w->current_matrix, it_vpos),
	  row->enabled_p))
    {
      if (it.hpos < row->used[TEXT_AREA])
	{
	  struct glyph *glyph = row->glyphs[TEXT_AREA] + it.hpos;
#ifdef HAVE_WINDOW_SYSTEM
	  if (img)
	    {
	      *dy -= row->ascent - glyph->ascent;
	      *dx += glyph->slice.img.x;
	      *dy += glyph->slice.img.y;
	      /* Image slices positions are still relative to the entire image */
	      *width = img->width;
	      *height = img->height;
	    }
	  else
#endif
	    {
	      *width = glyph->pixel_width;
	      *height = glyph->ascent + glyph->descent;
	    }
	}
      else
	{
	  *width = 0;
	  *height = row->height;
	}
    }
  else
    {
      *width = *height = 0;
    }

  /* Add extra (default width) columns if clicked after EOL. */
  x1 = max (0, it.current_x + it.pixel_width);
  if (to_x > x1)
    it.hpos += (to_x - x1) / WINDOW_FRAME_COLUMN_WIDTH (w);

  *x = it.hpos;
  *y = it.vpos;

  return string;
}


/* Value is the string under window-relative coordinates X/Y in the
   mode line or header line (PART says which) of window W, or nil if none.
   *CHARPOS is set to the position in the string returned.  */

Lisp_Object
mode_line_string (struct window *w, enum window_part part,
		  int *x, int *y, ptrdiff_t *charpos, Lisp_Object *object,
		  int *dx, int *dy, int *width, int *height)
{
  struct glyph_row *row;
  struct glyph *glyph, *end;
  int x0, y0;
  Lisp_Object string = Qnil;

  if (part == ON_MODE_LINE)
    row = MATRIX_MODE_LINE_ROW (w->current_matrix);
  else
    row = MATRIX_HEADER_LINE_ROW (w->current_matrix);
  y0 = *y - row->y;
  *y = row - MATRIX_FIRST_TEXT_ROW (w->current_matrix);

  if (row->mode_line_p && row->enabled_p)
    {
      /* Find the glyph under X.  If we find one with a string object,
         it's the one we were looking for.  */
      glyph = row->glyphs[TEXT_AREA];
      end = glyph + row->used[TEXT_AREA];
      for (x0 = *x; glyph < end && x0 >= glyph->pixel_width; ++glyph)
	x0 -= glyph->pixel_width;
      *x = glyph - row->glyphs[TEXT_AREA];
      if (glyph < end)
	{
	  string = glyph->object;
	  *charpos = glyph->charpos;
	  *width = glyph->pixel_width;
	  *height = glyph->ascent + glyph->descent;
#ifdef HAVE_WINDOW_SYSTEM
	  if (glyph->type == IMAGE_GLYPH)
	    {
	      struct image *img;
	      img = IMAGE_OPT_FROM_ID (WINDOW_XFRAME (w), glyph->u.img_id);
	      if (img != NULL)
		*object = img->spec;
	      y0 -= row->ascent - glyph->ascent;
	    }
#endif
	}
      else
	{
	  /* Add extra (default width) columns if clicked after EOL. */
	  *x += x0 / WINDOW_FRAME_COLUMN_WIDTH (w);
	  *width = 0;
	  *height = row->height;
	}
    }
  else
    {
      *x = 0;
      x0 = 0;
      *width = *height = 0;
    }

  *dx = x0;
  *dy = y0;

  return string;
}


/* Value is the string under window-relative coordinates X/Y in either
   marginal area, or nil if none.  *CHARPOS is set to the position in
   the string returned.  */

Lisp_Object
marginal_area_string (struct window *w, enum window_part part,
		      int *x, int *y, ptrdiff_t *charpos, Lisp_Object *object,
		      int *dx, int *dy, int *width, int *height)
{
  struct glyph_row *row = w->current_matrix->rows;
  struct glyph *glyph, *end;
  int x0, y0, i, wy = *y;
  int area;
  Lisp_Object string = Qnil;

  if (part == ON_LEFT_MARGIN)
    area = LEFT_MARGIN_AREA;
  else if (part == ON_RIGHT_MARGIN)
    area = RIGHT_MARGIN_AREA;
  else
    emacs_abort ();

  for (i = 0; row->enabled_p && i < w->current_matrix->nrows; ++i, ++row)
    if (wy >= row->y && wy < MATRIX_ROW_BOTTOM_Y (row))
      break;
  y0 = *y - row->y;
  *y = row - MATRIX_FIRST_TEXT_ROW (w->current_matrix);

  if (row->enabled_p)
    {
      /* Find the glyph under X.  If we find one with a string object,
	 it's the one we were looking for.  */
      if (area == RIGHT_MARGIN_AREA)
	x0 = ((WINDOW_HAS_FRINGES_OUTSIDE_MARGINS (w)
	       ? WINDOW_LEFT_FRINGE_WIDTH (w)
	       : WINDOW_FRINGES_WIDTH (w))
	      + window_box_width (w, LEFT_MARGIN_AREA)
	      + window_box_width (w, TEXT_AREA));
      else
	x0 = (WINDOW_HAS_FRINGES_OUTSIDE_MARGINS (w)
	      ? WINDOW_LEFT_FRINGE_WIDTH (w)
	      : 0);

      glyph = row->glyphs[area];
      end = glyph + row->used[area];
      for (x0 = *x - x0; glyph < end && x0 >= glyph->pixel_width; ++glyph)
	x0 -= glyph->pixel_width;
      *x = glyph - row->glyphs[area];
      if (glyph < end)
	{
	  string = glyph->object;
	  *charpos = glyph->charpos;
	  *width = glyph->pixel_width;
	  *height = glyph->ascent + glyph->descent;
#ifdef HAVE_WINDOW_SYSTEM
	  if (glyph->type == IMAGE_GLYPH)
	    {
	      struct image *img;
	      img = IMAGE_OPT_FROM_ID (WINDOW_XFRAME (w), glyph->u.img_id);
	      if (img != NULL)
		*object = img->spec;
	      y0 -= row->ascent - glyph->ascent;
	      x0 += glyph->slice.img.x;
	      y0 += glyph->slice.img.y;
	    }
#endif
	}
      else
	{
	  /* Add extra (default width) columns if clicked after EOL. */
	  *x += x0 / WINDOW_FRAME_COLUMN_WIDTH (w);
	  *width = 0;
	  *height = row->height;
	}
    }
  else
    {
      x0 = 0;
      *x = 0;
      *width = *height = 0;
    }

  *dx = x0;
  *dy = y0;

  return string;
}


/***********************************************************************
			 Changing Frame Sizes
 ***********************************************************************/

#ifdef SIGWINCH

static void deliver_window_change_signal (int);

static void
handle_window_change_signal (int sig)
{
  int width, height;
  struct tty_display_info *tty;

  /* The frame size change obviously applies to a single
     termcap-controlled terminal, but we can't decide which.
     Therefore, we resize the frames corresponding to each tty.
  */
  for (tty = tty_list; tty; tty = tty->next) {

    if (! tty->term_initted)
      continue;

    /* Suspended tty frames have tty->input == NULL avoid trying to
       use it.  */
    if (!tty->input)
      continue;

    get_tty_size (fileno (tty->input), &width, &height);

    if (width > 5 && height > 2) {
      Lisp_Object tail, frame;

      FOR_EACH_FRAME (tail, frame)
        if (FRAME_TERMCAP_P (XFRAME (frame)) && FRAME_TTY (XFRAME (frame)) == tty)
          /* Record the new sizes, but don't reallocate the data
             structures now.  Let that be done later outside of the
             signal handler.  */
          change_frame_size (XFRAME (frame), width,
			     height - FRAME_MENU_BAR_LINES (XFRAME (frame)),
			     0, 1, 0, 0);
    }
  }
}

static void
deliver_window_change_signal (int sig)
{
  deliver_process_signal (sig, handle_window_change_signal);
}
#endif /* SIGWINCH */


/* Do any change in frame size that was requested by a signal.
   SAFE means this function is called from a place where it is
   safe to change frame sizes while a redisplay is in progress.  */

void
do_pending_window_change (bool safe)
{
  /* If window change signal handler should have run before, run it now.  */
  if (redisplaying_p && !safe)
    return;

  while (delayed_size_change)
    {
      Lisp_Object tail, frame;

      delayed_size_change = 0;

      FOR_EACH_FRAME (tail, frame)
	{
	  struct frame *f = XFRAME (frame);

	  if (f->new_height != 0 || f->new_width != 0)
	    change_frame_size (f, f->new_width, f->new_height,
			       0, 0, safe, f->new_pixelwise);
	}
    }
}


static void
change_frame_size_1 (struct frame *f, int new_width, int new_height,
		     bool pretend, bool delay, bool safe, bool pixelwise)
{
  /* If we can't deal with the change now, queue it for later.  */
  if (delay || (redisplaying_p && !safe))
    {
      f->new_width = new_width;
      f->new_height = new_height;
      f->new_pixelwise = pixelwise;
      delayed_size_change = 1;
    }
  else
    {
      /* This size-change overrides any pending one for this frame.  */
      f->new_height = 0;
      f->new_width = 0;
      f->new_pixelwise = 0;

      /* If an argument is zero, set it to the current value.  */
      if (pixelwise)
	{
	  new_width = (new_width <= 0) ? FRAME_TEXT_WIDTH (f) : new_width;
	  new_height = (new_height <= 0) ? FRAME_TEXT_HEIGHT (f) : new_height;
	}
      else
	{
	  new_width = (((new_width <= 0) ? FRAME_COLS (f) : new_width)
		       * FRAME_COLUMN_WIDTH (f));
	  new_height = (((new_height <= 0) ? FRAME_LINES (f) : new_height)
			* FRAME_LINE_HEIGHT (f));
	}

      /* Adjust frame size but make sure x_set_window_size does not
	 get called.  */
      adjust_frame_size (f, new_width, new_height, 5, pretend,
			 Qchange_frame_size);
    }
}


/* Change text height/width of frame F.  Values may be given as zero to
   indicate that no change is needed.

   If DELAY, assume we're being called from a signal handler, and queue
   the change for later - perhaps the next redisplay.  Since this tries
   to resize windows, we can't call it from a signal handler.

   SAFE means this function is called from a place where it's safe to
   change frame sizes while a redisplay is in progress.  */
void
change_frame_size (struct frame *f, int new_width, int new_height,
		   bool pretend, bool delay, bool safe, bool pixelwise)
{
  Lisp_Object tail, frame;

  if (FRAME_MSDOS_P (f))
    {
      /* On MS-DOS, all frames use the same screen, so a change in
         size affects all frames.  Termcap now supports multiple
         ttys. */
      FOR_EACH_FRAME (tail, frame)
	if (! FRAME_WINDOW_P (XFRAME (frame)))
	  change_frame_size_1 (XFRAME (frame), new_width, new_height,
			       pretend, delay, safe, pixelwise);
    }
  else
    change_frame_size_1 (f, new_width, new_height, pretend, delay, safe,
			 pixelwise);
}

/***********************************************************************
		   Terminal Related Lisp Functions
 ***********************************************************************/

DEFUN ("open-termscript", Fopen_termscript, Sopen_termscript,
       1, 1, "FOpen termscript file: ",
       doc: /* Start writing all terminal output to FILE as well as the terminal.
FILE = nil means just close any termscript file currently open.  */)
  (Lisp_Object file)
{
  struct tty_display_info *tty;

  if (! FRAME_TERMCAP_P (SELECTED_FRAME ())
      && ! FRAME_MSDOS_P (SELECTED_FRAME ()))
    error ("Current frame is not on a tty device");

  tty = CURTTY ();

  if (tty->termscript != 0)
    {
      block_input ();
      fclose (tty->termscript);
      tty->termscript = 0;
      unblock_input ();
    }

  if (! NILP (file))
    {
      file = Fexpand_file_name (file, Qnil);
      tty->termscript = emacs_fopen (SSDATA (file), "w");
      if (tty->termscript == 0)
	report_file_error ("Opening termscript", file);
    }
  return Qnil;
}


DEFUN ("send-string-to-terminal", Fsend_string_to_terminal,
       Ssend_string_to_terminal, 1, 2, 0,
       doc: /* Send STRING to the terminal without alteration.
Control characters in STRING will have terminal-dependent effects.

Optional parameter TERMINAL specifies the tty terminal device to use.
It may be a terminal object, a frame, or nil for the terminal used by
the currently selected frame.  In batch mode, STRING is sent to stdout
when TERMINAL is nil.  */)
  (Lisp_Object string, Lisp_Object terminal)
{
  struct terminal *t = decode_live_terminal (terminal);
  FILE *out;

  /* ??? Perhaps we should do something special for multibyte strings here.  */
  CHECK_STRING (string);
  block_input ();

  if (t->type == output_initial)
    out = stdout;
  else if (t->type != output_termcap && t->type != output_msdos_raw)
    error ("Device %d is not a termcap terminal device", t->id);
  else
    {
      struct tty_display_info *tty = t->display_info.tty;

      if (! tty->output)
	error ("Terminal is currently suspended");

      if (tty->termscript)
	{
	  fwrite_unlocked (SDATA (string), 1, SBYTES (string), tty->termscript);
	  fflush_unlocked (tty->termscript);
	}
      out = tty->output;
    }
  fwrite_unlocked (SDATA (string), 1, SBYTES (string), out);
  fflush_unlocked (out);
  unblock_input ();
  return Qnil;
}


DEFUN ("ding", Fding, Sding, 0, 1, 0,
       doc: /* Beep, or flash the screen.
Also, unless an argument is given,
terminate any keyboard macro currently executing.  */)
  (Lisp_Object arg)
{
  if (!NILP (arg))
    {
      if (noninteractive)
	putchar_unlocked (07);
      else
	ring_bell (XFRAME (selected_frame));
    }
  else
    bitch_at_user ();

  return Qnil;
}

void
bitch_at_user (void)
{
  if (noninteractive)
    putchar_unlocked (07);
  else if (!INTERACTIVE)  /* Stop executing a keyboard macro.  */
    {
      const char *msg
	= "Keyboard macro terminated by a command ringing the bell";
      Fsignal (Quser_error, list1 (build_string (msg)));
    }
  else
    ring_bell (XFRAME (selected_frame));
}



/***********************************************************************
			  Sleeping, Waiting
 ***********************************************************************/

DEFUN ("sleep-for", Fsleep_for, Ssleep_for, 1, 2, 0,
       doc: /* Pause, without updating display, for SECONDS seconds.
SECONDS may be a floating-point value, meaning that you can wait for a
fraction of a second.  Optional second arg MILLISECONDS specifies an
additional wait period, in milliseconds; this is for backwards compatibility.
\(Not all operating systems support waiting for a fraction of a second.)  */)
  (Lisp_Object seconds, Lisp_Object milliseconds)
{
  double duration = extract_float (seconds);

  if (!NILP (milliseconds))
    {
      CHECK_NUMBER (milliseconds);
      duration += XINT (milliseconds) / 1000.0;
    }

  if (duration > 0)
    {
      struct timespec t = dtotimespec (duration);
      struct timespec tend = timespec_add (current_timespec (), t);

      /* wait_reading_process_output returns as soon as it detects
	 output from any subprocess, so we wait in a loop until the
	 time expires.  */
      do {
	wait_reading_process_output (min (t.tv_sec, WAIT_READING_MAX),
				     t.tv_nsec, 0, 0, Qnil, NULL, 0);
	t = timespec_sub (tend, current_timespec ());
      } while (timespec_sign (t) > 0);
    }

  return Qnil;
}


/* This is just like wait_reading_process_output, except that
   it does redisplay.

   TIMEOUT is number of seconds to wait (float or integer),
   or t to wait forever.
   READING is true if reading input.
   If DISPLAY_OPTION is >0 display process output while waiting.
   If DISPLAY_OPTION is >1 perform an initial redisplay before waiting.
*/

Lisp_Object
sit_for (Lisp_Object timeout, bool reading, int display_option)
{
  intmax_t sec;
  int nsec;
  bool do_display = display_option > 0;

  swallow_events (do_display);

  if ((detect_input_pending_run_timers (do_display))
      || !NILP (Vexecuting_kbd_macro))
    return Qnil;

  if (display_option > 1)
    redisplay_preserve_echo_area (2);

  if (INTEGERP (timeout))
    {
      sec = XINT (timeout);
      if (sec <= 0)
	return Qt;
      nsec = 0;
    }
  else if (FLOATP (timeout))
    {
      double seconds = XFLOAT_DATA (timeout);
      if (! (0 < seconds))
	return Qt;
      else
	{
	  struct timespec t = dtotimespec (seconds);
	  sec = min (t.tv_sec, WAIT_READING_MAX);
	  nsec = t.tv_nsec;
	}
    }
  else if (EQ (timeout, Qt))
    {
      sec = 0;
      nsec = 0;
    }
  else
    wrong_type_argument (Qnumberp, timeout);


#ifdef USABLE_SIGIO
  gobble_input ();
#endif

  wait_reading_process_output (sec, nsec, reading ? -1 : 1, do_display,
			       Qnil, NULL, 0);

  return detect_input_pending () ? Qnil : Qt;
}


DEFUN ("redisplay", Fredisplay, Sredisplay, 0, 1, 0,
       doc: /* Perform redisplay.
Optional arg FORCE, if non-nil, prevents redisplay from being
preempted by arriving input, even if `redisplay-dont-pause' is nil.
If `redisplay-dont-pause' is non-nil (the default), redisplay is never
preempted by arriving input, so FORCE does nothing.

Return t if redisplay was performed, nil if redisplay was preempted
immediately by pending input.  */)
  (Lisp_Object force)
{
  ptrdiff_t count;

  swallow_events (true);
  if ((detect_input_pending_run_timers (1)
       && NILP (force) && !redisplay_dont_pause)
      || !NILP (Vexecuting_kbd_macro))
    return Qnil;

  count = SPECPDL_INDEX ();
  if (!NILP (force) && !redisplay_dont_pause)
    specbind (Qredisplay_dont_pause, Qt);
  redisplay_preserve_echo_area (2);
  unbind_to (count, Qnil);
  return Qt;
}



/***********************************************************************
			 Other Lisp Functions
 ***********************************************************************/

/* A vector of size >= 2 * NFRAMES + 3 * NBUFFERS + 1, containing the
   session's frames, frame names, buffers, buffer-read-only flags, and
   buffer-modified-flags.  */

static Lisp_Object frame_and_buffer_state;


DEFUN ("frame-or-buffer-changed-p", Fframe_or_buffer_changed_p,
       Sframe_or_buffer_changed_p, 0, 1, 0,
       doc: /* Return non-nil if the frame and buffer state appears to have changed.
VARIABLE is a variable name whose value is either nil or a state vector
that will be updated to contain all frames and buffers,
aside from buffers whose names start with space,
along with the buffers' read-only and modified flags.  This allows a fast
check to see whether buffer menus might need to be recomputed.
If this function returns non-nil, it updates the internal vector to reflect
the current state.

If VARIABLE is nil, an internal variable is used.  Users should not
pass nil for VARIABLE.  */)
  (Lisp_Object variable)
{
  Lisp_Object state, tail, frame, buf;
  ptrdiff_t n, idx;

  if (! NILP (variable))
    {
      CHECK_SYMBOL (variable);
      state = Fsymbol_value (variable);
      if (! VECTORP (state))
	goto changed;
    }
  else
    state = frame_and_buffer_state;

  idx = 0;
  FOR_EACH_FRAME (tail, frame)
    {
      if (idx == ASIZE (state))
	goto changed;
      if (!EQ (AREF (state, idx++), frame))
	goto changed;
      if (idx == ASIZE (state))
	goto changed;
      if (!EQ (AREF (state, idx++), XFRAME (frame)->name))
	goto changed;
    }
  /* Check that the buffer info matches.  */
  FOR_EACH_LIVE_BUFFER (tail, buf)
    {
      /* Ignore buffers that aren't included in buffer lists.  */
      if (SREF (BVAR (XBUFFER (buf), name), 0) == ' ')
	continue;
      if (idx == ASIZE (state))
	goto changed;
      if (!EQ (AREF (state, idx++), buf))
	goto changed;
      if (idx == ASIZE (state))
	goto changed;
      if (!EQ (AREF (state, idx++), BVAR (XBUFFER (buf), read_only)))
	goto changed;
      if (idx == ASIZE (state))
	goto changed;
      if (!EQ (AREF (state, idx++), Fbuffer_modified_p (buf)))
	goto changed;
    }
  if (idx == ASIZE (state))
    goto changed;
  /* Detect deletion of a buffer at the end of the list.  */
  if (EQ (AREF (state, idx), Qlambda))
    return Qnil;

  /* Come here if we decide the data has changed.  */
 changed:
  /* Count the size we will need.
     Start with 1 so there is room for at least one lambda at the end.  */
  n = 1;
  FOR_EACH_FRAME (tail, frame)
    n += 2;
  FOR_EACH_LIVE_BUFFER (tail, buf)
    n += 3;
  /* Reallocate the vector if data has grown to need it,
     or if it has shrunk a lot.  */
  if (! VECTORP (state)
      || n > ASIZE (state)
      || n + 20 < ASIZE (state) / 2)
    /* Add 20 extra so we grow it less often.  */
    {
      state = Fmake_vector (make_number (n + 20), Qlambda);
      if (! NILP (variable))
	Fset (variable, state);
      else
	frame_and_buffer_state = state;
    }

  /* Record the new data in the (possibly reallocated) vector.  */
  idx = 0;
  FOR_EACH_FRAME (tail, frame)
    {
      ASET (state, idx, frame);
      idx++;
      ASET (state, idx, XFRAME (frame)->name);
      idx++;
    }
  FOR_EACH_LIVE_BUFFER (tail, buf)
    {
      /* Ignore buffers that aren't included in buffer lists.  */
      if (SREF (BVAR (XBUFFER (buf), name), 0) == ' ')
	continue;
      ASET (state, idx, buf);
      idx++;
      ASET (state, idx, BVAR (XBUFFER (buf), read_only));
      idx++;
      ASET (state, idx, Fbuffer_modified_p (buf));
      idx++;
    }
  /* Fill up the vector with lambdas (always at least one).  */
  ASET (state, idx, Qlambda);
  idx++;
  while (idx < ASIZE (state))
    {
      ASET (state, idx, Qlambda);
      idx++;
    }
  /* Make sure we didn't overflow the vector.  */
  eassert (idx <= ASIZE (state));
  return Qt;
}



/***********************************************************************
			    Initialization
***********************************************************************/

/* Initialization done when Emacs fork is started, before doing stty.
   Determine terminal type and set terminal_driver.  Then invoke its
   decoding routine to set up variables in the terminal package.  */

void
init_display (void)
{
  char *terminal_type;

  /* Construct the space glyph.  */
  space_glyph.type = CHAR_GLYPH;
  SET_CHAR_GLYPH (space_glyph, ' ', DEFAULT_FACE_ID, 0);
  space_glyph.charpos = -1;

  inverse_video = 0;
  cursor_in_echo_area = false;

  /* Now is the time to initialize this; it's used by init_sys_modes
     during startup.  */
  Vinitial_window_system = Qnil;

  /* SIGWINCH needs to be handled no matter what display we start
     with.  Otherwise newly opened tty frames will not resize
     automatically. */
#ifdef SIGWINCH
#ifndef CANNOT_DUMP
  if (initialized)
#endif /* CANNOT_DUMP */
    {
      struct sigaction action;
      emacs_sigaction_init (&action, deliver_window_change_signal);
      sigaction (SIGWINCH, &action, 0);
    }
#endif /* SIGWINCH */

  /* If running as a daemon, no need to initialize any frames/terminal,
     except on Windows, where we at least want to initialize it.  */
#ifndef WINDOWSNT
  if (IS_DAEMON)
      return;
#endif

  /* If the user wants to use a window system, we shouldn't bother
     initializing the terminal.  This is especially important when the
     terminal is so dumb that emacs gives up before and doesn't bother
     using the window system.

     If the DISPLAY environment variable is set and nonempty,
     try to use X, and die with an error message if that doesn't work.  */

#ifdef HAVE_X_WINDOWS
  if (! inhibit_window_system && ! display_arg)
    {
      char *display;
      display = getenv ("DISPLAY");
      display_arg = (display != 0 && *display != 0);

      if (display_arg && !x_display_ok (display))
	{
	  fprintf (stderr, "Display %s unavailable, simulating -nw\n",
		   display);
	  inhibit_window_system = 1;
	}
    }

  if (!inhibit_window_system && display_arg)
    {
      Vinitial_window_system = Qx;
#ifdef HAVE_X11
      Vwindow_system_version = make_number (11);
#endif
#ifdef USE_NCURSES
      /* In some versions of ncurses,
	 tputs crashes if we have not called tgetent.
	 So call tgetent.  */
      { char b[2044]; tgetent (b, "xterm");}
#endif
      return;
    }
#endif /* HAVE_X_WINDOWS */

#ifdef HAVE_NTGUI
  if (!inhibit_window_system)
    {
      Vinitial_window_system = Qw32;
      Vwindow_system_version = make_number (1);
      return;
    }
#endif /* HAVE_NTGUI */

#ifdef HAVE_NS
  if (!inhibit_window_system
#ifndef CANNOT_DUMP
     && initialized
#endif
      )
    {
      Vinitial_window_system = Qns;
      Vwindow_system_version = make_number (10);
      return;
    }
#endif

  /* If no window system has been specified, try to use the terminal.  */
  if (! isatty (STDIN_FILENO))
    fatal ("standard input is not a tty");

#ifdef WINDOWSNT
  terminal_type = (char *)"w32console";
#else
  terminal_type = getenv ("TERM");
#endif
  if (!terminal_type)
    {
#ifdef HAVE_WINDOW_SYSTEM
      if (! inhibit_window_system)
	fprintf (stderr, "Please set the environment variable DISPLAY or TERM (see 'tset').\n");
      else
#endif /* HAVE_WINDOW_SYSTEM */
	fprintf (stderr, "Please set the environment variable TERM; see 'tset'.\n");
      exit (1);
    }

  {
    struct terminal *t;
    struct frame *f = XFRAME (selected_frame);

    init_foreground_group ();

    /* Open a display on the controlling tty. */
    t = init_tty (0, terminal_type, 1); /* Errors are fatal. */

    /* Convert the initial frame to use the new display. */
    if (f->output_method != output_initial)
      emacs_abort ();
    f->output_method = t->type;
    f->terminal = t;

    t->reference_count++;
#ifdef MSDOS
    f->output_data.tty->display_info = &the_only_display_info;
#else
    if (f->output_method == output_termcap)
      create_tty_output (f);
#endif
    t->display_info.tty->top_frame = selected_frame;
    change_frame_size (XFRAME (selected_frame),
                       FrameCols (t->display_info.tty),
                       FrameRows (t->display_info.tty)
		       - FRAME_MENU_BAR_LINES (f), 0, 0, 1, 0);

    /* Delete the initial terminal. */
    if (--initial_terminal->reference_count == 0
        && initial_terminal->delete_terminal_hook)
      (*initial_terminal->delete_terminal_hook) (initial_terminal);

    /* Update frame parameters to reflect the new type. */
    AUTO_FRAME_ARG (tty_type_arg, Qtty_type, Ftty_type (selected_frame));
    Fmodify_frame_parameters (selected_frame, tty_type_arg);
    AUTO_FRAME_ARG (tty_arg, Qtty, (t->display_info.tty->name
				    ? build_string (t->display_info.tty->name)
				    : Qnil));
    Fmodify_frame_parameters (selected_frame, tty_arg);
  }

  {
    struct frame *sf = SELECTED_FRAME ();
    int width = FRAME_TOTAL_COLS (sf);
    int height = FRAME_TOTAL_LINES (sf);
    int area;

    /* If these sizes are so big they cause overflow, just ignore the
       change.  It's not clear what better we could do.  The rest of
       the code assumes that (width + 2) * height * sizeof (struct glyph)
       does not overflow and does not exceed PTRDIFF_MAX or SIZE_MAX.  */
    if (INT_ADD_WRAPV (width, 2, &area)
	|| INT_MULTIPLY_WRAPV (height, area, &area)
	|| min (PTRDIFF_MAX, SIZE_MAX) / sizeof (struct glyph) < area)
      fatal ("screen size %dx%d too big", width, height);
  }

  calculate_costs (XFRAME (selected_frame));

  /* Set up faces of the initial terminal frame of a dumped Emacs.  */
  if (initialized
      && !noninteractive
      && NILP (Vinitial_window_system))
    {
      /* For the initial frame, we don't have any way of knowing what
	 are the foreground and background colors of the terminal.  */
      struct frame *sf = SELECTED_FRAME ();

      FRAME_FOREGROUND_PIXEL (sf) = FACE_TTY_DEFAULT_FG_COLOR;
      FRAME_BACKGROUND_PIXEL (sf) = FACE_TTY_DEFAULT_BG_COLOR;
      call0 (intern ("tty-set-up-initial-frame-faces"));
    }
}



/***********************************************************************
			   Blinking cursor
 ***********************************************************************/

DEFUN ("internal-show-cursor", Finternal_show_cursor,
       Sinternal_show_cursor, 2, 2, 0,
       doc: /* Set the cursor-visibility flag of WINDOW to SHOW.
WINDOW nil means use the selected window.  SHOW non-nil means
show a cursor in WINDOW in the next redisplay.  SHOW nil means
don't show a cursor.  */)
  (Lisp_Object window, Lisp_Object show)
{
  /* Don't change cursor state while redisplaying.  This could confuse
     output routines.  */
  if (!redisplaying_p)
    decode_any_window (window)->cursor_off_p = NILP (show);
  return Qnil;
}


DEFUN ("internal-show-cursor-p", Finternal_show_cursor_p,
       Sinternal_show_cursor_p, 0, 1, 0,
       doc: /* Value is non-nil if next redisplay will display a cursor in WINDOW.
WINDOW nil or omitted means report on the selected window.  */)
  (Lisp_Object window)
{
  return decode_any_window (window)->cursor_off_p ? Qnil : Qt;
}

/***********************************************************************
			    Initialization
 ***********************************************************************/

void
syms_of_display (void)
{
  defsubr (&Sredraw_frame);
  defsubr (&Sredraw_display);
  defsubr (&Sframe_or_buffer_changed_p);
  defsubr (&Sopen_termscript);
  defsubr (&Sding);
  defsubr (&Sredisplay);
  defsubr (&Ssleep_for);
  defsubr (&Ssend_string_to_terminal);
  defsubr (&Sinternal_show_cursor);
  defsubr (&Sinternal_show_cursor_p);

#ifdef GLYPH_DEBUG
  defsubr (&Sdump_redisplay_history);
#endif

  staticpro (&frame_and_buffer_state, Fmake_vector (make_number (20), Qlambda));

  /* This is the "purpose" slot of a display table.  */
  DEFSYM (Qdisplay_table, "display-table");

  DEFSYM (Qredisplay_dont_pause, "redisplay-dont-pause");

  DEFVAR_INT ("baud-rate", baud_rate,
	      doc: /* The output baud rate of the terminal.
On most systems, changing this value will affect the amount of padding
and the other strategic decisions made during redisplay.  */);

  DEFVAR_BOOL ("inverse-video", inverse_video,
	       doc: /* Non-nil means invert the entire frame display.
This means everything is in inverse video which otherwise would not be.  */);

  DEFVAR_BOOL ("visible-bell", visible_bell,
	       doc: /* Non-nil means try to flash the frame to represent a bell.

See also `ring-bell-function'.  */);

  DEFVAR_BOOL ("no-redraw-on-reenter", no_redraw_on_reenter,
	       doc: /* Non-nil means no need to redraw entire frame after suspending.
A non-nil value is useful if the terminal can automatically preserve
Emacs's frame display when you reenter Emacs.
It is up to you to set this variable if your terminal can do that.  */);

  DEFVAR_LISP ("initial-window-system", Vinitial_window_system,
	       doc: /* Name of the window system that Emacs uses for the first frame.
The value is a symbol:
 nil for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame.

Use of this variable as a boolean is deprecated.  Instead,
use `display-graphic-p' or any of the other `display-*-p'
predicates which report frame's specific UI-related capabilities.  */);

  DEFVAR_KBOARD ("window-system", Vwindow_system,
		 doc: /* Name of window system through which the selected frame is displayed.
The value is a symbol:
 nil for a termcap frame (a character-only terminal),
 `x' for an Emacs frame that is really an X window,
 `w32' for an Emacs frame that is a window on MS-Windows display,
 `ns' for an Emacs frame on a GNUstep or Macintosh Cocoa display,
 `pc' for a direct-write MS-DOS frame.

Use of this variable as a boolean is deprecated.  Instead,
use `display-graphic-p' or any of the other `display-*-p'
predicates which report frame's specific UI-related capabilities.  */);

  DEFVAR_LISP ("window-system-version", Vwindow_system_version,
	       doc: /* The version number of the window system in use.
For X windows, this is 11.  */);

  DEFVAR_BOOL ("cursor-in-echo-area", cursor_in_echo_area,
	       doc: /* Non-nil means put cursor in minibuffer, at end of any message there.  */);

  DEFVAR_LISP ("glyph-table", Vglyph_table,
	       doc: /* Table defining how to output a glyph code to the frame.
If not nil, this is a vector indexed by glyph code to define the glyph.
Each element can be:
 integer: a glyph code which this glyph is an alias for.
 string: output this glyph using that string (not impl. in X windows).
 nil: this glyph mod 524288 is the code of a character to output,
    and this glyph / 524288 is the face number (see `face-id') to use
    while outputting it.  */);
  Vglyph_table = Qnil;

  DEFVAR_LISP ("standard-display-table", Vstandard_display_table,
	       doc: /* Display table to use for buffers that specify none.
It is also used for standard output and error streams.
See `buffer-display-table' for more information.  */);
  Vstandard_display_table = Qnil;

  DEFVAR_BOOL ("redisplay-dont-pause", redisplay_dont_pause,
	       doc: /* Nil means display update is paused when input is detected.  */);
  /* Contrary to expectations, a value of "false" can be detrimental to
     responsiveness since aborting a redisplay throws away some of the
     work already performed.  It's usually more efficient (and gives
     more prompt feedback to the user) to let the redisplay terminate,
     and just completely skip the next command's redisplay (which is
     done regardless of this setting if there's pending input at the
     beginning of the next redisplay).  */
  redisplay_dont_pause = true;

#ifdef CANNOT_DUMP
  if (noninteractive)
#endif
    {
      Vinitial_window_system = Qnil;
      Vwindow_system_version = Qnil;
    }
}
