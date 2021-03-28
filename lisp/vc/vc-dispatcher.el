;;; vc-dispatcher.el -- generic command-dispatcher facility.  -*- lexical-binding: t -*-

;; Copyright (C) 2008-2021 Free Software Foundation, Inc.

;; Author: FSF (see below for full credits)
;; Keywords: vc tools
;; Package: vc

;; This file is part of GNU Emacs.

;; GNU Emacs is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.

;;; Credits:

;; Designed and implemented by Eric S. Raymond, originally as part of VC mode.
;; Stefan Monnier and Dan Nicolaescu contributed substantial work on the
;; vc-dir front end.

;;; Commentary:

;; Goals:
;;
;; There is a class of front-ending problems that Emacs might be used
;; to address that involves selecting sets of files, or possibly
;; directories, and passing the selection set to slave commands.  The
;; prototypical example, from which this code is derived, is talking
;; to version-control systems.
;;
;; vc-dispatcher.el is written to decouple the UI issues in such front
;; ends from their application-specific logic.  It also provides a
;; service layer for running the slave commands either synchronously
;; or asynchronously and managing the message/error logs from the
;; command runs.
;;
;; Similar UI problems can be expected to come up in applications
;; areas other than VCSes; IDEs and document search are two obvious ones.
;; This mode is intended to ensure that the Emacs interfaces for all such
;; beasts are consistent and carefully designed.  But even if nothing
;; but VC ever uses it, getting the layer separation right will be
;; a valuable thing.

;; Dispatcher's universe:
;;
;; The universe consists of the file tree rooted at the current
;; directory.  The dispatcher's upper layer deduces some subset
;; of the file tree from the state of the currently visited buffer
;; and returns that subset, presumably to a client mode.
;;
;; The user may be looking at either of two different views; a buffer
;; visiting a file, or a directory buffer generated by vc-dispatcher.
;;
;; The lower layer of this mode runs commands in subprocesses, either
;; synchronously or asynchronously.  Commands may be launched in one
;; of two ways: they may be run immediately, or the calling mode can
;; create a closure associated with a text-entry buffer, to be
;; executed when the user types C-c to ship the buffer contents.  In
;; either case the command messages and error (if any) will remain
;; available in a status buffer.

;; Special behavior of dispatcher directory buffers:
;;
;; In dispatcher directory buffers, facilities to perform basic
;; navigation and selection operations are provided by keymap and menu
;; entries that dispatcher sets up itself, so they'll be uniform
;; across all dispatcher-using client modes.  Client modes are
;; expected to append to these to provide mode-specific bindings.
;;
;; The standard map associates a 'state' slot (that the client mode
;; may set) with each directory entry.  The dispatcher knows nothing
;; about the semantics of individual states, but mark and unmark commands
;; treat all entries with the same state as the currently selected one as
;; a unit.

;; The interface:
;;
;; The main interface to the lower level is vc-do-command.  This launches a
;; command, synchronously or asynchronously, making the output available
;; in a command log buffer.  Two other functions, (vc-start-logentry) and
;; (vc-finish-logentry), allow you to associate a command closure with an
;; annotation buffer so that when the user confirms the comment the closure
;; is run (with the comment as part of its context).
;;
;; The interface to the upper level has the two main entry points (vc-dir)
;; and (vc-dispatcher-selection-set) and a couple of convenience functions.
;; (vc-dir) sets up a dispatcher browsing buffer; (vc-dispatcher-selection-set)
;; returns a selection set of files, either the marked files in a browsing
;; buffer or the singleton set consisting of the file visited by the current
;; buffer (when that is appropriate).  It also does what is needed to ensure
;; that on-disk files and the contents of their visiting Emacs buffers
;; coincide.
;;
;; When the client mode adds a local vc-mode-line-hook to a buffer, it
;; will be called with the buffer file name as argument whenever the
;; dispatcher resyncs the buffer.

;; To do:
;;
;; - log buffers need font-locking.
;;

;; General customization
(defcustom vc-logentry-check-hook nil
  "Normal hook run by `vc-finish-logentry'.
Use this to impose your own rules on the entry in addition to any the
dispatcher client mode imposes itself."
  :type 'hook
  :group 'vc)

(defcustom vc-delete-logbuf-window t
  "If non-nil, delete the log buffer and window after each logical action.
If nil, bury that buffer instead.
This is most useful if you have multiple windows on a frame and would like to
preserve the setting."
  :type 'boolean
  :group 'vc)

(defcustom vc-command-messages nil
  "If non-nil, display run messages from back-end commands."
  :type 'boolean
  :group 'vc)

(defcustom vc-suppress-confirm nil
  "If non-nil, treat user as expert; suppress yes-no prompts on some things."
  :type 'boolean
  :group 'vc)

;; Variables the user doesn't need to know about.

(defvar vc-log-operation nil)
(defvar vc-log-after-operation-hook nil
  "Name of the hook run at the end of `vc-finish-logentry'.
BEWARE: Despite its name, this variable is not itself a hook!")
(defvar vc-log-fileset)

;; In a log entry buffer, this is a local variable
;; that points to the buffer for which it was made
;; (either a file, or a directory buffer).
(defvar vc-parent-buffer nil)
(put 'vc-parent-buffer 'permanent-local t)
(defvar vc-parent-buffer-name nil)
(put 'vc-parent-buffer-name 'permanent-local t)

;; Common command execution logic

(defun vc-process-filter (p s)
  "An alternative output filter for async process P.
One difference with the default filter is that this inserts S after markers.
Another is that undo information is not kept."
  (let ((buffer (process-buffer p)))
    (when (buffer-live-p buffer)
      (with-current-buffer buffer
        (save-excursion
          (let ((buffer-undo-list t)
                (inhibit-read-only t))
            (goto-char (process-mark p))
            (insert s)
            (set-marker (process-mark p) (point))))))))

(defun vc-setup-buffer (buf)
  "Prepare BUF for executing a slave command and make it current."
  (let ((camefrom (current-buffer))
	(olddir default-directory))
    (set-buffer (get-buffer-create buf))
    (let ((oldproc (get-buffer-process (current-buffer))))
      ;; If we wanted to wait for oldproc to finish before doing
      ;; something, we'd have used vc-eval-after.
      ;; Use `delete-process' rather than `kill-process' because we don't
      ;; want any of its output to appear from now on.
      (when oldproc (delete-process oldproc)))
    (kill-all-local-variables)
    (setq-local vc-parent-buffer camefrom)
    (setq-local vc-parent-buffer-name
                (concat " from " (buffer-name camefrom)))
    (setq default-directory olddir)
    (let ((buffer-undo-list t)
          (inhibit-read-only t))
      (erase-buffer))))

(defvar vc-sentinel-movepoint)          ;Dynamically scoped.

(defun vc--process-sentinel (p code)
  (let ((buf (process-buffer p)))
    ;; Impatient users sometime kill "slow" buffers; check liveness
    ;; to avoid "error in process sentinel: Selecting deleted buffer".
    (when (buffer-live-p buf)
      (with-current-buffer buf
        (setq mode-line-process
              (let ((status (process-status p)))
                ;; Leave mode-line uncluttered, normally.
                (unless (eq 'exit status)
                  (format " (%s)" status))))
        (let (vc-sentinel-movepoint
              (m (process-mark p)))
          ;; Normally, we want async code such as sentinels to not move point.
          (save-excursion
            (goto-char m)
            ;; Each sentinel may move point and the next one should be run
            ;; at that new point.  We could get the same result by having
            ;; each sentinel read&set process-mark, but since `cmd' needs
            ;; to work both for async and sync processes, this would be
            ;; difficult to achieve.
            (vc-exec-after code)
            (move-marker m (point)))
          ;; But sometimes the sentinels really want to move point.
          (when vc-sentinel-movepoint
	    (let ((win (get-buffer-window (current-buffer) 0)))
	      (if (not win)
		  (goto-char vc-sentinel-movepoint)
		(with-selected-window win
		  (goto-char vc-sentinel-movepoint))))))))))

(defun vc-set-mode-line-busy-indicator ()
  (setq mode-line-process
	(concat " " (propertize "[waiting...]"
                                'face 'mode-line-emphasis
                                'help-echo
                                "A command is in progress in this buffer"))))

(defun vc-exec-after (code)
  "Eval CODE when the current buffer's process is done.
If the current buffer has no process, just evaluate CODE.
Else, add CODE to the process' sentinel.
CODE should be a function of no arguments."
  (let ((proc (get-buffer-process (current-buffer))))
    (cond
     ;; If there's no background process, just execute the code.
     ;; We used to explicitly call delete-process on exited processes,
     ;; but this led to timing problems causing process output to be
     ;; lost.  Terminated processes get deleted automatically
     ;; anyway. -- cyd
     ((or (null proc) (eq (process-status proc) 'exit))
      ;; Make sure we've read the process's output before going further.
      (when proc (accept-process-output proc))
      (if (functionp code) (funcall code) (eval code t)))
     ;; If a process is running, add CODE to the sentinel
     ((eq (process-status proc) 'run)
      (vc-set-mode-line-busy-indicator)
      (letrec ((fun (lambda (p _msg)
                      (remove-function (process-sentinel p) fun)
                      (vc--process-sentinel p code))))
        (add-function :after (process-sentinel proc) fun)))
     (t (error "Unexpected process state"))))
  nil)

(defmacro vc-run-delayed (&rest body)
  (declare (indent 0) (debug t))
  `(vc-exec-after (lambda () ,@body)))

(defvar vc-post-command-functions nil
  "Hook run at the end of `vc-do-command'.
Each function is called inside the buffer in which the command was run
and is passed 3 arguments: the COMMAND, the FILES and the FLAGS.")

(defvar w32-quote-process-args)

(defun vc-delistify (filelist)
  "Smash a FILELIST into a file list string suitable for info messages."
  ;; FIXME what about file names with spaces?
  (if (not filelist) "."  (mapconcat #'identity filelist " ")))

(defcustom vc-tor nil
  "If non-nil, communicate with the repository site via Tor.
See https://2019.www.torproject.org/about/overview.html.en and
the man pages for \"torsocks\" for more details about Tor."
  :type 'boolean
  :version "27.1"
  :group 'vc)

;;;###autoload
(defun vc-do-command (buffer okstatus command file-or-list &rest flags)
  "Execute a slave command, notifying user and checking for errors.
Output from COMMAND goes to BUFFER, or the current buffer if
BUFFER is t.  If the destination buffer is not already current,
set it up properly and erase it.  The command is considered
successful if its exit status does not exceed OKSTATUS (if
OKSTATUS is nil, that means to ignore error status, if it is
`async', that means not to wait for termination of the
subprocess; if it is t it means to ignore all execution errors).
FILE-OR-LIST is the name of a working file; it may be a list of
files or be nil (to execute commands that don't expect a file
name or set of files).  If an optional list of FLAGS is present,
that is inserted into the command line before the filename.
Return the return value of the slave command in the synchronous
case, and the process object in the asynchronous case."
  ;; FIXME: file-relative-name can return a bogus result because
  ;; it doesn't look at the actual file-system to see if symlinks
  ;; come into play.
  (let* ((files
	  (mapcar (lambda (f) (file-relative-name (expand-file-name f)))
		  (if (listp file-or-list) file-or-list (list file-or-list))))
	 ;; Keep entire commands in *Messages* but avoid resizing the
	 ;; echo area.  Messages in this function are formatted in
	 ;; a such way that the important parts are at the beginning,
	 ;; due to potential truncation of long messages.
	 (message-truncate-lines t)
	 (full-command
	  (concat (if vc-tor "torsocks " "")
                  (if (string= (substring command -1) "\n")
		      (substring command 0 -1)
		    command)
		  " " (vc-delistify flags)
		  " " (vc-delistify files))))
    (save-current-buffer
      (unless (or (eq buffer t)
		  (and (stringp buffer)
		       (string= (buffer-name) buffer))
		  (eq buffer (current-buffer)))
	(vc-setup-buffer buffer))
      ;; If there's some previous async process still running, just kill it.
      (let ((squeezed (remq nil flags))
	    (inhibit-read-only t)
	    (status 0))
	(when files
	  (setq squeezed (nconc squeezed files)))
	(let (;; Since some functions need to parse the output
	      ;; from external commands, set LC_MESSAGES to C.
	      (process-environment (cons "LC_MESSAGES=C" process-environment))
	      (w32-quote-process-args t))
	  (if (eq okstatus 'async)
	      ;; Run asynchronously.
	      (let ((proc
		     (let ((process-connection-type nil))
		       (apply #'start-file-process command (current-buffer)
                              command squeezed))))
		(when vc-command-messages
		  (let ((inhibit-message (eq (selected-window) (active-minibuffer-window))))
		    (message "Running in background: %s" full-command)))
                ;; Get rid of the default message insertion, in case we don't
                ;; set a sentinel explicitly.
		(set-process-sentinel proc #'ignore)
		(set-process-filter proc #'vc-process-filter)
		(setq status proc)
		(when vc-command-messages
		  (vc-run-delayed
		    (let ((message-truncate-lines t)
			  (inhibit-message (eq (selected-window) (active-minibuffer-window))))
		      (message "Done in background: %s" full-command)))))
	    ;; Run synchronously
	    (when vc-command-messages
	      (let ((inhibit-message (eq (selected-window) (active-minibuffer-window))))
		(message "Running in foreground: %s" full-command)))
	    (let ((buffer-undo-list t))
	      (setq status (apply #'process-file command nil t nil squeezed)))
	    (when (and (not (eq t okstatus))
		       (or (not (integerp status))
			   (and okstatus (< okstatus status))))
              (unless (eq ?\s (aref (buffer-name (current-buffer)) 0))
                (pop-to-buffer (current-buffer))
                (goto-char (point-min))
                (shrink-window-if-larger-than-buffer))
	      (error "Failed (%s): %s"
		     (if (integerp status) (format "status %d" status) status)
		     full-command))
	    (when vc-command-messages
	      (let ((inhibit-message (eq (selected-window) (active-minibuffer-window))))
		(message "Done (status=%d): %s" status full-command)))))
	(vc-run-delayed
	  (run-hook-with-args 'vc-post-command-functions
			      command file-or-list flags))
	status))))

(defun vc-do-async-command (buffer root command &rest args)
  "Run COMMAND asynchronously with ARGS, displaying the result.
Send the output to BUFFER, which should be a buffer or the name
of a buffer, which is created.
ROOT should be the directory in which the command should be run.
Display the buffer in some window, but don't select it."
  (let* ((dir default-directory)
	 (inhibit-read-only t)
	 window new-window-start)
    (setq buffer (get-buffer-create buffer))
    (if (get-buffer-process buffer)
	(error "Another VC action on %s is running" root))
    (with-current-buffer buffer
      (setq default-directory root)
      (goto-char (point-max))
      (unless (eq (point) (point-min))
	(insert "\n"))
      (setq new-window-start (point))
      (insert "Running \"" command)
      (dolist (arg args)
	(insert " " arg))
      (insert "\"...\n")
      ;; Run in the original working directory.
      (let ((default-directory dir))
	(apply #'vc-do-command t 'async command nil args)))
    (setq window (display-buffer buffer))
    (if window
	(set-window-start window new-window-start))
    buffer))

(defvar compilation-error-regexp-alist)

(defun vc-compilation-mode (backend)
  "Setup `compilation-mode' after with the appropriate `compilation-error-regexp-alist'."
  (require 'compile)
  (let* ((error-regexp-alist
          (vc-make-backend-sym backend 'error-regexp-alist))
	 (error-regexp-alist (and (boundp error-regexp-alist)
				  (symbol-value error-regexp-alist))))
    (let ((compilation-error-regexp-alist error-regexp-alist))
      (compilation-mode))
    (setq-local compilation-error-regexp-alist
                error-regexp-alist)))

(declare-function vc-dir-refresh "vc-dir" ())

(defun vc-set-async-update (process-buffer)
  "Set a `vc-exec-after' action appropriate to the current buffer.
This action will update the current buffer after the current
asynchronous VC command has completed.  PROCESS-BUFFER is the
buffer for the asynchronous VC process.

If the current buffer is a VC Dir buffer, call `vc-dir-refresh'.
If the current buffer is a Dired buffer, revert it."
  (let* ((buf (current-buffer))
	 (tick (buffer-modified-tick buf)))
    (cond
     ((derived-mode-p 'vc-dir-mode)
      (with-current-buffer process-buffer
	(vc-run-delayed
	 (if (buffer-live-p buf)
             (with-current-buffer buf
               (vc-dir-refresh))))))
     ((derived-mode-p 'dired-mode)
      (with-current-buffer process-buffer
	(vc-run-delayed
	 (and (buffer-live-p buf)
              (= (buffer-modified-tick buf) tick)
              (with-current-buffer buf
                (revert-buffer)))))))))

;; These functions are used to ensure that the view the user sees is up to date
;; even if the dispatcher client mode has messed with file contents (as in,
;; for example, VCS keyword expansion).

(declare-function view-mode-exit "view" (&optional exit-only exit-action all-win))

(defun vc-position-context (posn)
  "Save a bit of the text around POSN in the current buffer.
Used to help us find the corresponding position again later
if markers are destroyed or corrupted."
  ;; A lot of this was shamelessly lifted from Sebastian Kremer's
  ;; rcs.el mode.
  (list posn
	(buffer-size)
	(buffer-substring posn
			  (min (point-max) (+ posn 100)))))

(defun vc-find-position-by-context (context)
  "Return the position of CONTEXT in the current buffer.
If CONTEXT cannot be found, return nil."
  (let ((context-string (nth 2 context)))
    (if (equal "" context-string)
	(point-max)
      (save-excursion
	(let ((diff (- (nth 1 context) (buffer-size))))
	  (when (< diff 0) (setq diff (- diff)))
	  (goto-char (nth 0 context))
	  (if (or (search-forward context-string nil t)
		  ;; Can't use search-backward since the match may continue
		  ;; after point.
		  (progn (goto-char (- (point) diff (length context-string)))
			 ;; goto-char doesn't signal an error at
			 ;; beginning of buffer like backward-char would
			 (search-forward context-string nil t)))
	      ;; to beginning of OSTRING
	      (- (point) (length context-string))))))))

(defun vc-context-matches-p (posn context)
  "Return t if POSN matches CONTEXT, nil otherwise."
  (let* ((context-string (nth 2 context))
	 (len (length context-string))
	 (end (+ posn len)))
    (if (> end (1+ (buffer-size)))
	nil
      (string= context-string (buffer-substring posn end)))))

(defun vc-buffer-context ()
  "Return a list (POINT-CONTEXT MARK-CONTEXT REPARSE).
Used by `vc-restore-buffer-context' to later restore the context."
  (let ((point-context (vc-position-context (point)))
	;; Use mark-marker to avoid confusion in transient-mark-mode.
	(mark-context (when (eq (marker-buffer (mark-marker)) (current-buffer))
			 (vc-position-context (mark-marker))))
	;; Make the right thing happen in transient-mark-mode.
	(mark-active nil))
    (list point-context mark-context)))

(defun vc-restore-buffer-context (context)
  "Restore point/mark, and reparse any affected compilation buffers.
CONTEXT is that which `vc-buffer-context' returns."
  (let ((point-context (nth 0 context))
	(mark-context (nth 1 context)))
    ;; if necessary, restore point and mark
    (if (not (vc-context-matches-p (point) point-context))
	(let ((new-point (vc-find-position-by-context point-context)))
	  (when new-point (goto-char new-point))))
    (and mark-active
         mark-context
         (not (vc-context-matches-p (mark) mark-context))
         (let ((new-mark (vc-find-position-by-context mark-context)))
           (when new-mark (set-mark new-mark))))))

(defun vc-revert-buffer-internal (&optional arg no-confirm)
  "Revert buffer, keeping point and mark where user expects them.
Try to be clever in the face of changes due to expanded version-control
key words.  This is important for typeahead to work as expected.
ARG and NO-CONFIRM are passed on to `revert-buffer'."
  (interactive "P")
  (widen)
  (let ((context (vc-buffer-context)))
    ;; Use save-excursion here, because it may be able to restore point
    ;; and mark properly even in cases where vc-restore-buffer-context
    ;; would fail.  However, save-excursion might also get it wrong --
    ;; in this case, vc-restore-buffer-context gives it a second try.
    (save-excursion
      ;; t means don't call normal-mode;
      ;; that's to preserve various minor modes.
      (revert-buffer arg no-confirm t))
    (vc-restore-buffer-context context)))

(defvar-local vc-mode-line-hook nil)
(put 'vc-mode-line-hook 'permanent-local t)

(defvar view-old-buffer-read-only)

(defun vc-resynch-window (file &optional keep noquery reset-vc-info)
  "If FILE is in the current buffer, either revert or unvisit it.
The choice between revert (to see expanded keywords) and unvisit
depends on KEEP.  NOQUERY if non-nil inhibits confirmation for
reverting.  NOQUERY should be t *only* if it is known the only
difference between the buffer and the file is due to
modifications by the dispatcher client code, rather than user
editing!"
  (and (string= buffer-file-name file)
       (if keep
	   (when (file-exists-p file)
	     (when reset-vc-info
	       (vc-file-clearprops file))
	     (vc-revert-buffer-internal t noquery)

	     ;; VC operations might toggle the read-only state.  In
	     ;; that case we need to adjust the `view-mode' status
	     ;; when `view-read-only' is non-nil.
             (and view-read-only
                  (if (file-writable-p file)
                      (and view-mode
                           (let ((view-old-buffer-read-only nil))
                             (view-mode-exit t)))
                    (and (not view-mode)
                         (not (eq (get major-mode 'mode-class) 'special))
                         (view-mode-enter))))

             ;; FIXME: Why use a hook?  Why pass it buffer-file-name?
	     (run-hook-with-args 'vc-mode-line-hook buffer-file-name))
	 (kill-buffer (current-buffer)))))

(declare-function vc-dir-resynch-file "vc-dir" (&optional fname))

(defun vc-resynch-buffers-in-directory (directory &optional keep noquery reset-vc-info)
  "Resync all buffers that visit files in DIRECTORY."
  (dolist (buffer (buffer-list))
    (let ((fname (buffer-file-name buffer)))
      (when (and fname (string-prefix-p directory fname))
	(with-current-buffer buffer
	  (vc-resynch-buffer fname keep noquery reset-vc-info))))))

(defun vc-resynch-buffer (file &optional keep noquery reset-vc-info)
  "If FILE is currently visited, resynch its buffer."
  (if (string= buffer-file-name file)
      (vc-resynch-window file keep noquery reset-vc-info)
    (if (file-directory-p file)
	(vc-resynch-buffers-in-directory file keep noquery reset-vc-info)
      (let ((buffer (get-file-buffer file)))
	(when buffer
	  (with-current-buffer buffer
	    (vc-resynch-window file keep noquery reset-vc-info))))))
  ;; Try to avoid unnecessary work, a *vc-dir* buffer is only present
  ;; if this is true.
  (when vc-dir-buffers
    (vc-dir-resynch-file file)))

(defun vc-buffer-sync (&optional not-urgent)
  "Make sure the current buffer and its working file are in sync.
NOT-URGENT means it is ok to continue if the user says not to save."
  (let (missing)
    (when (cond
           ((buffer-modified-p))
           ((not (file-exists-p buffer-file-name))
            (setq missing t)))
      (if (or vc-suppress-confirm
              (y-or-n-p (format "Buffer %s %s; save it? "
                                (buffer-name)
                                (if missing
                                    "is missing on disk"
                                  "modified"))))
          (save-buffer)
        (unless not-urgent
          (error "Aborted"))))))

;; Command closures

;; Set up key bindings for use while editing log messages

(declare-function log-edit-empty-buffer-p "log-edit" ())

(defun vc-log-edit (fileset mode backend)
  "Set up `log-edit' for use on FILE."
  (setq default-directory
	(buffer-local-value 'default-directory vc-parent-buffer))
  (require 'log-edit)
  (log-edit 'vc-finish-logentry
	    ;; Setup a new log message if the log buffer is "empty",
	    ;; or was previously used for a different set of files.
	    (or (log-edit-empty-buffer-p)
		(and (local-variable-p 'vc-log-fileset)
		     (not (equal vc-log-fileset fileset))))
	    `((log-edit-listfun
               . (lambda ()
                   ;; FIXME: When fileset includes directories, and
                   ;; there are relevant ChangeLog files inside their
                   ;; children, we don't find them.  Either handle it
                   ;; in `log-edit-insert-changelog-entries' by
                   ;; walking down the file trees, or somehow pass
                   ;; `fileset-only-files' from `vc-next-action'
                   ;; through to this function.
                   (let ((root (vc-root-dir)))
                     ;; Returns paths relative to the root, so that
                     ;; `log-edit-changelog-insert-entries'
                     ;; substitutes them in correctly later, even when
                     ;; `vc-checkin' was called from a file buffer, or
                     ;; a non-root VC-Dir buffer.
                     (mapcar
                      (lambda (file) (file-relative-name file root))
                      ',fileset))))
	      (log-edit-diff-function . vc-diff)
	      (log-edit-vc-backend . ,backend)
	      (vc-log-fileset . ,fileset))
	    nil
	    mode)
  (set-buffer-modified-p nil)
  (setq buffer-file-name nil))

(defun vc-start-logentry (files comment initial-contents msg logbuf mode action &optional after-hook backend)
  "Accept a comment for an operation on FILES.
If COMMENT is nil, pop up a LOGBUF buffer, emit MSG, and set the
action on close to ACTION.  If COMMENT is a string and
INITIAL-CONTENTS is non-nil, then COMMENT is used as the initial
contents of the log entry buffer.  If COMMENT is a string and
INITIAL-CONTENTS is nil, do action immediately as if the user had
entered COMMENT.  If COMMENT is t, also do action immediately with an
empty comment.  Remember the file's buffer in `vc-parent-buffer'
\(current one if no file).  Puts the log-entry buffer in major-mode
MODE, defaulting to `log-edit-mode' if MODE is nil.
AFTER-HOOK specifies the local value for `vc-log-after-operation-hook'.
BACKEND, if non-nil, specifies a VC backend for the Log Edit buffer."
  (let ((parent
         (if (vc-dispatcher-browsing)
             ;; If we are called from a directory browser, the parent buffer is
             ;; the current buffer.
             (current-buffer)
           (if (and files (equal (length files) 1))
               (get-file-buffer (car files))
             (current-buffer)))))
    (if (and comment (not initial-contents))
	(set-buffer (get-buffer-create logbuf))
      (pop-to-buffer (get-buffer-create logbuf)))
    (setq-local vc-parent-buffer parent)
    (setq-local vc-parent-buffer-name
                (concat " from " (buffer-name vc-parent-buffer)))
    (vc-log-edit files mode backend)
    (make-local-variable 'vc-log-after-operation-hook)
    (when after-hook
      (setq vc-log-after-operation-hook after-hook))
    (setq-local vc-log-operation action)
    (when comment
      (erase-buffer)
      (when (stringp comment) (insert comment)))
    (if (or (not comment) initial-contents)
	(message "%s  Type C-c C-c when done" msg)
      (vc-finish-logentry (eq comment t)))))

;; vc-finish-logentry is typically called from a log-edit buffer (see
;; vc-start-logentry).
(defun vc-finish-logentry (&optional nocomment)
  "Complete the operation implied by the current log entry.
Use the contents of the current buffer as a check-in or registration
comment.  If the optional arg NOCOMMENT is non-nil, then don't check
the buffer contents as a comment."
  (interactive)
  ;; Check and record the comment, if any.
  (unless nocomment
    (run-hooks 'vc-logentry-check-hook))
  ;; Sync parent buffer in case the user modified it while editing the comment.
  ;; But not if it is a vc-dir buffer.
  (with-current-buffer vc-parent-buffer
    (or (vc-dispatcher-browsing) (vc-buffer-sync)))
  (unless vc-log-operation
    (error "No log operation is pending"))

  ;; save the parameters held in buffer-local variables
  (let ((logbuf (current-buffer))
	(log-operation vc-log-operation)
        ;; FIXME: When coming from VC-Dir, we should check that the
        ;; set of selected files is still equal to vc-log-fileset,
        ;; to avoid surprises.
	(log-fileset vc-log-fileset)
	(log-entry (buffer-string))
	(after-hook vc-log-after-operation-hook))
    (pop-to-buffer vc-parent-buffer)
    ;; OK, do it to it
    (save-excursion
      (funcall log-operation
	       log-fileset
	       log-entry))
    (setq vc-log-operation nil)

    ;; Quit windows on logbuf.
    (cond
     ((not logbuf))
     (vc-delete-logbuf-window
      (quit-windows-on logbuf t (selected-frame)))
     (t
      (quit-windows-on logbuf nil 0)))

    ;; Now make sure we see the expanded headers
    (when log-fileset
      (mapc
       (lambda (file) (vc-resynch-buffer file t t))
       log-fileset))
    (run-hooks after-hook 'vc-finish-logentry-hook)))

(defun vc-dispatcher-browsing ()
  "Are we in a directory browser buffer?"
  (or (derived-mode-p 'vc-dir-mode)
      (derived-mode-p 'dired-mode)))

;; These are unused.
;; (defun vc-dispatcher-in-fileset-p (fileset)
;;   (let ((member nil))
;;     (while (and (not member) fileset)
;;       (let ((elem (pop fileset)))
;;         (if (if (file-directory-p elem)
;;                 (eq t (compare-strings buffer-file-name nil (length elem)
;;                                        elem nil nil))
;;               (eq (current-buffer) (get-file-buffer elem)))
;;             (setq member t))))
;;     member))

;; (defun vc-dispatcher-selection-set (&optional observer)
;;   "Deduce a set of files to which to apply an operation.  Return a cons
;; cell (SELECTION . FILESET), where SELECTION is what the user chose
;; and FILES is the flist with any directories replaced by the listed files
;; within them.

;; If we're in a directory display, the fileset is the list of marked files (if
;; there is one) else the file on the current line.  If not in a directory
;; display, but the current buffer visits a file, the fileset is a singleton
;; containing that file.  Otherwise, throw an error."
;;   (let ((selection
;;          (cond
;;           ;; Browsing with vc-dir
;;           ((vc-dispatcher-browsing)
;; 	   ;; If no files are marked, temporarily mark current file
;; 	   ;; and choose on that basis (so we get subordinate files)
;; 	   (if (not (vc-dir-marked-files))
;; 		 (prog2
;; 		   (vc-dir-mark-file)
;; 		   (cons (vc-dir-marked-files) (vc-dir-marked-only-files))
;; 		   (vc-dir-unmark-all-files t))
;; 	     (cons (vc-dir-marked-files) (vc-dir-marked-only-files))))
;;           ;; Visiting an eligible file
;;           ((buffer-file-name)
;;            (cons (list buffer-file-name) (list buffer-file-name)))
;;           ;; No eligible file -- if there's a parent buffer, deduce from there
;;           ((and vc-parent-buffer (or (buffer-file-name vc-parent-buffer)
;;                                      (with-current-buffer vc-parent-buffer
;;                                        (vc-dispatcher-browsing))))
;;            (with-current-buffer vc-parent-buffer
;;              (vc-dispatcher-selection-set)))
;;           ;; No good set here, throw error
;;           (t (error "No fileset is available here")))))
;;     ;; We assume, in order to avoid unpleasant surprises to the user,
;;     ;; that a fileset is not in good shape to be handed to the user if the
;;     ;; buffers visiting the fileset don't match the on-disk contents.
;;     (unless observer
;;       (save-some-buffers
;;        nil (lambda () (vc-dispatcher-in-fileset-p (cdr selection)))))
;;     selection))

(provide 'vc-dispatcher)

;;; vc-dispatcher.el ends here
