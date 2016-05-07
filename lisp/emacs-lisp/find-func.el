;;; find-func.el --- find the definition of the Emacs Lisp function near point  -*- lexical-binding:t -*-

;; Copyright (C) 1997, 1999, 2001-2016 Free Software Foundation, Inc.

;; Author: Jens Petersen <petersen@kurims.kyoto-u.ac.jp>
;; Maintainer: petersen@kurims.kyoto-u.ac.jp
;; Keywords: emacs-lisp, functions, variables
;; Created: 97/07/25

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
;; along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:
;;
;; The funniest thing about this is that I can't imagine why a package
;; so obviously useful as this hasn't been written before!!
;; ;;; find-func
;; (find-function-setup-keys)
;;
;; or just:
;;
;; (load "find-func")
;;
;; if you don't like the given keybindings and away you go!  It does
;; pretty much what you would expect, putting the cursor at the
;; definition of the function or variable at point.
;;
;; The code started out from `describe-function', `describe-key'
;; ("help.el") and `fff-find-loaded-emacs-lisp-function' (Noah Friedman's
;; "fff.el").

;;; Code:

;;; User variables:

(defgroup find-function nil
  "Finds the definition of the Emacs Lisp symbol near point."
;;   :prefix "find-function"
  :group 'lisp)

(defconst find-function-space-re "\\(?:\\s-\\|\n\\|;.*\n\\)+")

(defcustom find-function-regexp
  ;; Match things like (defun foo ...), (defmacro foo ...),
  ;; (define-skeleton foo ...), (define-generic-mode 'foo ...),
  ;;  (define-derived-mode foo ...), (define-minor-mode foo)
  (concat
   "^\\s-*(\\(def\\(ine-skeleton\\|ine-generic-mode\\|ine-derived-mode\\|\
ine\\(?:-global\\)?-minor-mode\\|ine-compilation-mode\\|un-cvs-mode\\|\
foo\\|\\(?:[^icfgv]\\|g[^r]\\)\\(\\w\\|\\s_\\)+\\*?\\)\\|easy-mmode-define-[a-z-]+\\|easy-menu-define\\|\
menu-bar-make-toggle\\)"
   find-function-space-re
   "\\('\\|(quote \\)?%s\\(\\s-\\|$\\|[()]\\)")
  "The regexp used by `find-function' to search for a function definition.
Note it must contain a `%s' at the place where `format'
should insert the function name.  The default value avoids `defconst',
`defgroup', `defvar', `defface'.

Please send improvements and fixes to the maintainer."
  :type 'regexp
  :group 'find-function
  :version "21.1")

(defcustom find-variable-regexp
  (concat
   "^\\s-*(\\(def[^fumag]\\(\\w\\|\\s_\\)+\\*?\\|\
easy-mmode-def\\(map\\|syntax\\)\\|easy-menu-define\\)"
   find-function-space-re
   "%s\\(\\s-\\|$\\)")
  "The regexp used by `find-variable' to search for a variable definition.
Note it must contain a `%s' at the place where `format'
should insert the variable name.  The default value
avoids `defun', `defmacro', `defalias', `defadvice', `defgroup', `defface'.

Please send improvements and fixes to the maintainer."
  :type 'regexp
  :group 'find-function
  :version "21.1")

(defcustom find-face-regexp
  (concat"^\\s-*(defface" find-function-space-re "%s\\(\\s-\\|$\\)")
  "The regexp used by `find-face' to search for a face definition.
Note it must contain a `%s' at the place where `format'
should insert the face name.

Please send improvements and fixes to the maintainer."
  :type 'regexp
  :group 'find-function
  :version "22.1")

(defcustom find-feature-regexp
  (concat ";;; Code:")
  "The regexp used by `xref-find-definitions' when searching for a feature definition.
Note it must contain a `%s' at the place where `format'
should insert the feature name."
  ;; We search for ";;; Code" rather than (feature '%s) because the
  ;; former is near the start of the code, and the latter is very
  ;; uninteresting. If the regexp is not found, just goes to
  ;; (point-min), which is acceptable in this case.
  :type 'regexp
  :group 'xref
  :version "25.0")

(defcustom find-alias-regexp
  "(defalias +'%s"
  "The regexp used by `xref-find-definitions' to search for an alias definition.
Note it must contain a `%s' at the place where `format'
should insert the feature name."
  :type 'regexp
  :group 'xref
  :version "25.0")

(defvar find-function-regexp-alist
  '((nil . find-function-regexp)
    (defvar . find-variable-regexp)
    (defface . find-face-regexp)
    (feature . find-feature-regexp)
    (defalias . find-alias-regexp))
  "Alist mapping definition types into regexp variables.
Each regexp variable's value should actually be a format string
to be used to substitute the desired symbol name into the regexp.
Instead of regexp variable, types can be mapped to functions as well,
in which case the function is called with one argument (the object
we're looking for) and it should search for it.")
(put 'find-function-regexp-alist 'risky-local-variable t)

(defcustom find-function-source-path nil
  "The default list of directories where `find-function' searches.

If this variable is nil then `find-function' searches `load-path' by
default."
  :type '(repeat directory)
  :group 'find-function)

(defcustom find-function-recenter-line 1
  "The window line-number from which to start displaying a symbol definition.
A value of nil implies center the beginning of the definition.
See `find-function' and `find-variable'."
  :type '(choice (const :tag "Center" nil)
		 integer)
  :group 'find-function
  :version "20.3")

(defcustom find-function-after-hook nil
  "Hook run after finding symbol definition.

See the functions `find-function' and `find-variable'."
  :type 'hook
  :group 'find-function
  :version "20.3")

;;; Functions:

(defun find-library-suffixes ()
  (let ((suffixes nil))
    (dolist (suffix (get-load-suffixes) (nreverse suffixes))
      (unless (string-match "elc" suffix) (push suffix suffixes)))))

(defun find-library--load-name (library)
  (let ((name library))
    (dolist (dir load-path)
      (let ((rel (file-relative-name library dir)))
        (if (and (not (string-match "\\`\\.\\./" rel))
                 (< (length rel) (length name)))
            (setq name rel))))
    (unless (equal name library) name)))

(defun find-library-name (library)
  "Return the absolute file name of the Emacs Lisp source of LIBRARY.
LIBRARY should be a string (the name of the library)."
  ;; If the library is byte-compiled, try to find a source library by
  ;; the same name.
  (when (string-match "\\.el\\(c\\(\\..*\\)?\\)\\'" library)
    (setq library (replace-match "" t t library)))
  (or
   (locate-file library
                (or find-function-source-path load-path)
                (find-library-suffixes))
   (locate-file library
                (or find-function-source-path load-path)
                load-file-rep-suffixes)
   (when (file-name-absolute-p library)
     (let ((rel (find-library--load-name library)))
       (when rel
         (or
          (locate-file rel
                       (or find-function-source-path load-path)
                       (find-library-suffixes))
          (locate-file rel
                       (or find-function-source-path load-path)
                       load-file-rep-suffixes)))))
   (find-library--from-load-path library)
   (error "Can't find library %s" library)))

(defun find-library--from-load-path (library)
  ;; In `load-history', the file may be ".elc", ".el", ".el.gz", and
  ;; LIBRARY may be "foo.el" or "foo", so make sure that we get all
  ;; potential matches, and then see whether any of them lead us to an
  ;; ".el" or an ".el.gz" file.
  (let* ((elc-regexp "\\.el\\(c\\(\\..*\\)?\\)\\'")
         (suffix-regexp
          (concat "\\("
                  (mapconcat 'regexp-quote (find-library-suffixes) "\\'\\|")
                  "\\|" elc-regexp "\\)\\'"))
         (potentials
          (mapcar
           (lambda (entry)
             (if (string-match suffix-regexp (car entry))
                 (replace-match "" t t (car entry))
               (car entry)))
           (seq-filter
            (lambda (entry)
              (string-match
               (concat "\\`"
                       (regexp-quote
                        (replace-regexp-in-string suffix-regexp "" library))
                       suffix-regexp)
               (file-name-nondirectory (car entry))))
            load-history)))
         result)
    (dolist (file potentials)
      (dolist (suffix (find-library-suffixes))
        (when (not result)
          (cond ((file-exists-p file)
                 (setq result file))
                ((file-exists-p (concat file suffix))
                 (setq result (concat file suffix)))))))
    result))

(defvar find-function-C-source-directory
  (let ((dir (expand-file-name "src" source-directory)))
    (if (file-accessible-directory-p dir) dir))
  "Directory where the C source files of Emacs can be found.
If nil, do not try to find the source code of functions and variables
defined in C.")

(declare-function ad-get-advice-info "advice" (function))

(defun find-function-advised-original (func)
  "Return the original function definition of an advised function FUNC.
If FUNC is not a symbol, return it.  Else, if it's not advised,
return the symbol's function definition."
  (or (and (symbolp func)
           (featurep 'nadvice)
           (let ((ofunc (advice--symbol-function func)))
             (if (advice--p ofunc)
                 (advice--cd*r ofunc)
               ofunc)))
      func))

(defun find-function-C-source (fun-or-var file type)
  "Find the source location where FUN-OR-VAR is defined in FILE.
TYPE should be nil to find a function, or `defvar' to find a variable."
  (let ((dir (or find-function-C-source-directory
                 (read-directory-name "Emacs C source dir: " nil nil t))))
    (setq file (expand-file-name file dir))
    (if (file-readable-p file)
        (if (null find-function-C-source-directory)
            (setq find-function-C-source-directory dir))
      (error "The C source file %s is not available"
             (file-name-nondirectory file))))
  (unless type
    ;; Either or both an alias and its target might be advised.
    (setq fun-or-var (find-function-advised-original
		      (indirect-function
		       (find-function-advised-original fun-or-var)))))
  (with-current-buffer (find-file-noselect file)
    (goto-char (point-min))
    (unless (re-search-forward
	     (if type
		 (concat "DEFVAR[A-Z_]*[ \t\n]*([ \t\n]*\""
			 (regexp-quote (symbol-name fun-or-var))
			 "\"")
	       (concat "DEFUN[ \t\n]*([ \t\n]*\""
		       (regexp-quote (subr-name (advice--cd*r fun-or-var)))
		       "\""))
	     nil t)
      (error "Can't find source for %s" fun-or-var))
    (cons (current-buffer) (match-beginning 0))))

;;;###autoload
(defun find-library (library &optional other-window)
  "Find the Emacs Lisp source of LIBRARY.
LIBRARY should be a string (the name of the library).  If the
optional OTHER-WINDOW argument (i.e., the command argument) is
specified, pop to a different window before displaying the
buffer."
  (interactive
   (let* ((dirs (or find-function-source-path load-path))
          (suffixes (find-library-suffixes))
          (table (apply-partially 'locate-file-completion-table
                                  dirs suffixes))
	  (def (if (eq (function-called-at-point) 'require)
		   ;; `function-called-at-point' may return 'require
		   ;; with `point' anywhere on this line.  So wrap the
		   ;; `save-excursion' below in a `condition-case' to
		   ;; avoid reporting a scan-error here.
		   (condition-case nil
		       (save-excursion
			 (backward-up-list)
			 (forward-char)
			 (forward-sexp 2)
			 (thing-at-point 'symbol))
		     (error nil))
		 (thing-at-point 'symbol))))
     (when (and def (not (test-completion def table)))
       (setq def nil))
     (list
      (completing-read (if def
                           (format "Library name (default %s): " def)
			 "Library name: ")
		       table nil nil nil nil def)
      current-prefix-arg)))
  (prog1
      (funcall (if other-window
                   'pop-to-buffer
                 'pop-to-buffer-same-window)
               (find-file-noselect (find-library-name library)))
    (run-hooks 'find-function-after-hook)))

;;;###autoload
(defun find-function-search-for-symbol (symbol type library)
  "Search for SYMBOL's definition of type TYPE in LIBRARY.
Visit the library in a buffer, and return a cons cell (BUFFER . POSITION),
or just (BUFFER . nil) if the definition can't be found in the file.

If TYPE is nil, look for a function definition.
Otherwise, TYPE specifies the kind of definition,
and it is interpreted via `find-function-regexp-alist'.
The search is done in the source for library LIBRARY."
  (if (null library)
      (error "Don't know where `%s' is defined" symbol))
  ;; Some functions are defined as part of the construct
  ;; that defines something else.
  (while (and (symbolp symbol) (get symbol 'definition-name))
    (setq symbol (get symbol 'definition-name)))
  (if (string-match "\\`src/\\(.*\\.\\(c\\|m\\)\\)\\'" library)
      (find-function-C-source symbol (match-string 1 library) type)
    (when (string-match "\\.el\\(c\\)\\'" library)
      (setq library (substring library 0 (match-beginning 1))))
    ;; Strip extension from .emacs.el to make sure symbol is searched in
    ;; .emacs too.
    (when (string-match "\\.emacs\\(.el\\)" library)
      (setq library (substring library 0 (match-beginning 1))))
    (let* ((filename (find-library-name library))
	   (regexp-symbol (cdr (assq type find-function-regexp-alist))))
      (with-current-buffer (find-file-noselect filename)
	(let ((regexp (if (functionp regexp-symbol) regexp-symbol
                        (format (symbol-value regexp-symbol)
                                ;; Entry for ` (backquote) macro in loaddefs.el,
                                ;; (defalias (quote \`)..., has a \ but
                                ;; (symbol-name symbol) doesn't.  Add an
                                ;; optional \ to catch this.
                                (concat "\\\\?"
                                        (regexp-quote (symbol-name symbol))))))
	      (case-fold-search))
	  (with-syntax-table emacs-lisp-mode-syntax-table
	    (goto-char (point-min))
	    (if (if (functionp regexp)
                    (funcall regexp symbol)
                  (or (re-search-forward regexp nil t)
                      ;; `regexp' matches definitions using known forms like
                      ;; `defun', or `defvar'.  But some functions/variables
                      ;; are defined using special macros (or functions), so
                      ;; if `regexp' can't find the definition, we look for
                      ;; something of the form "(SOMETHING <symbol> ...)".
                      ;; This fails to distinguish function definitions from
                      ;; variable declarations (or even uses thereof), but is
                      ;; a good pragmatic fallback.
                      (re-search-forward
                       (concat "^([^ ]+" find-function-space-re "['(]?"
                               (regexp-quote (symbol-name symbol))
                               "\\_>")
                       nil t)))
		(progn
		  (beginning-of-line)
		  (cons (current-buffer) (point)))
	      (cons (current-buffer) nil))))))))

(defun find-function-library (function &optional lisp-only verbose)
  "Return the pair (ORIG-FUNCTION . LIBRARY) for FUNCTION.

ORIG-FUNCTION is the original name, after removing all advice and
resolving aliases.  LIBRARY is an absolute file name, a relative
file name inside the C sources directory, or a name of an
autoloaded feature.

If ORIG-FUNCTION is a built-in function and LISP-ONLY is non-nil,
signal an error.

If VERBOSE is non-nil, and FUNCTION is an alias, display a
message about the whole chain of aliases."
  (let ((def (if (symbolp function)
                 (find-function-advised-original function)))
        aliases)
    ;; FIXME for completeness, it might be nice to print something like:
    ;; foo (which is advised), which is an alias for bar (which is advised).
    (while (and def (symbolp def))
      (or (eq def function)
          (not verbose)
          (setq aliases (if aliases
                            (concat aliases
                                    (format-message
                                     ", which is an alias for `%s'"
                                     (symbol-name def)))
                          (format-message "`%s' is an alias for `%s'"
                                          function (symbol-name def)))))
      (setq function (find-function-advised-original function)
            def (find-function-advised-original function)))
    (if aliases
        (message "%s" aliases))
    (cons function
          (cond
           ((autoloadp def) (nth 1 def))
           ((subrp def)
            (if lisp-only
                (error "%s is a built-in function" function))
            (help-C-file-name def 'subr))
           ((symbol-file function 'defun))))))

;;;###autoload
(defun find-function-noselect (function &optional lisp-only)
  "Return a pair (BUFFER . POINT) pointing to the definition of FUNCTION.

Finds the source file containing the definition of FUNCTION
in a buffer and the point of the definition.  The buffer is
not selected.  If the function definition can't be found in
the buffer, returns (BUFFER).

If FUNCTION is a built-in function, this function normally
attempts to find it in the Emacs C sources; however, if LISP-ONLY
is non-nil, signal an error instead.

If the file where FUNCTION is defined is not known, then it is
searched for in `find-function-source-path' if non-nil, otherwise
in `load-path'."
  (if (not function)
    (error "You didn't specify a function"))
  (let ((func-lib (find-function-library function lisp-only t)))
    (find-function-search-for-symbol (car func-lib) nil (cdr func-lib))))

(defun find-function-read (&optional type)
  "Read and return an interned symbol, defaulting to the one near point.

If TYPE is nil, insist on a symbol with a function definition.
Otherwise TYPE should be `defvar' or `defface'.
If TYPE is nil, defaults using `function-called-at-point',
otherwise uses `variable-at-point'."
  (let* ((symb1 (cond ((null type) (function-called-at-point))
                      ((eq type 'defvar) (variable-at-point))
                      (t (variable-at-point t))))
         (symb  (unless (eq symb1 0) symb1))
         (predicate (cdr (assq type '((nil . fboundp)
                                      (defvar . boundp)
                                      (defface . facep)))))
         (prompt-type (cdr (assq type '((nil . "function")
                                        (defvar . "variable")
                                        (defface . "face")))))
         (prompt (concat "Find " prompt-type
                         (and symb (format " (default %s)" symb))
                         ": "))
         (enable-recursive-minibuffers t))
    (list (intern (completing-read
                   prompt obarray predicate
                   t nil nil (and symb (symbol-name symb)))))))

(defun find-function-do-it (symbol type switch-fn)
  "Find Emacs Lisp SYMBOL in a buffer and display it.
TYPE is nil to search for a function definition,
or else `defvar' or `defface'.

The variable `find-function-recenter-line' controls how
to recenter the display.  SWITCH-FN is the function to call
to display and select the buffer.
See also `find-function-after-hook'.

Set mark before moving, if the buffer already existed."
  (let* ((orig-point (point))
	(orig-buffers (buffer-list))
	(buffer-point (save-excursion
			(find-definition-noselect symbol type)))
	(new-buf (car buffer-point))
	(new-point (cdr buffer-point)))
    (when buffer-point
      (when (memq new-buf orig-buffers)
	(push-mark orig-point))
      (funcall switch-fn new-buf)
      (when new-point (goto-char new-point))
      (recenter find-function-recenter-line)
      (run-hooks 'find-function-after-hook))))

;;;###autoload
(defun find-function (function)
  "Find the definition of the FUNCTION near point.

Finds the source file containing the definition of the function
near point (selected by `function-called-at-point') in a buffer and
places point before the definition.
Set mark before moving, if the buffer already existed.

The library where FUNCTION is defined is searched for in
`find-function-source-path', if non-nil, otherwise in `load-path'.
See also `find-function-recenter-line' and `find-function-after-hook'."
  (interactive (find-function-read))
  (find-function-do-it function nil 'switch-to-buffer))

;;;###autoload
(defun find-function-other-window (function)
  "Find, in another window, the definition of FUNCTION near point.

See `find-function' for more details."
  (interactive (find-function-read))
  (find-function-do-it function nil 'switch-to-buffer-other-window))

;;;###autoload
(defun find-function-other-frame (function)
  "Find, in another frame, the definition of FUNCTION near point.

See `find-function' for more details."
  (interactive (find-function-read))
  (find-function-do-it function nil 'switch-to-buffer-other-frame))

;;;###autoload
(defun find-variable-noselect (variable &optional file)
  "Return a pair `(BUFFER . POINT)' pointing to the definition of VARIABLE.

Finds the library containing the definition of VARIABLE in a buffer and
the point of the definition.  The buffer is not selected.
If the variable's definition can't be found in the buffer, return (BUFFER).

The library where VARIABLE is defined is searched for in FILE or
`find-function-source-path', if non-nil, otherwise in `load-path'."
  (if (not variable)
      (error "You didn't specify a variable")
    (let ((library (or file
                       (symbol-file variable 'defvar)
                       (help-C-file-name variable 'var))))
      (find-function-search-for-symbol variable 'defvar library))))

;;;###autoload
(defun find-variable (variable)
  "Find the definition of the VARIABLE at or before point.

Finds the library containing the definition of the variable
near point (selected by `variable-at-point') in a buffer and
places point before the definition.

Set mark before moving, if the buffer already existed.

The library where VARIABLE is defined is searched for in
`find-function-source-path', if non-nil, otherwise in `load-path'.
See also `find-function-recenter-line' and `find-function-after-hook'."
  (interactive (find-function-read 'defvar))
  (find-function-do-it variable 'defvar 'switch-to-buffer))

;;;###autoload
(defun find-variable-other-window (variable)
  "Find, in another window, the definition of VARIABLE near point.

See `find-variable' for more details."
  (interactive (find-function-read 'defvar))
  (find-function-do-it variable 'defvar 'switch-to-buffer-other-window))

;;;###autoload
(defun find-variable-other-frame (variable)
  "Find, in another frame, the definition of VARIABLE near point.

See `find-variable' for more details."
  (interactive (find-function-read 'defvar))
  (find-function-do-it variable 'defvar 'switch-to-buffer-other-frame))

;;;###autoload
(defun find-definition-noselect (symbol type &optional file)
  "Return a pair `(BUFFER . POINT)' pointing to the definition of SYMBOL.
If the definition can't be found in the buffer, return (BUFFER).
TYPE says what type of definition: nil for a function, `defvar' for a
variable, `defface' for a face.  This function does not switch to the
buffer nor display it.

The library where SYMBOL is defined is searched for in FILE or
`find-function-source-path', if non-nil, otherwise in `load-path'."
  (cond
   ((not symbol)
    (error "You didn't specify a symbol"))
   ((null type)
    (find-function-noselect symbol))
   ((eq type 'defvar)
    (find-variable-noselect symbol file))
   (t
    (let ((library (or file (symbol-file symbol type))))
      (find-function-search-for-symbol symbol type library)))))

;; For symmetry, this should be called find-face; but some programs
;; assume that, if that name is defined, it means something else.
;;;###autoload
(defun find-face-definition (face)
  "Find the definition of FACE.  FACE defaults to the name near point.

Finds the Emacs Lisp library containing the definition of the face
near point (selected by `variable-at-point') in a buffer and
places point before the definition.

Set mark before moving, if the buffer already existed.

The library where FACE is defined is searched for in
`find-function-source-path', if non-nil, otherwise in `load-path'.
See also `find-function-recenter-line' and `find-function-after-hook'."
  (interactive (find-function-read 'defface))
  (find-function-do-it face 'defface 'switch-to-buffer))

(defun find-function-on-key-do-it (key find-fn)
  "Find the function that KEY invokes.  KEY is a string.
Set mark before moving, if the buffer already existed.

FIND-FN is the function to call to navigate to the function."
  (let (defn)
    (save-excursion
      (let* ((event (and (eventp key) (aref key 0))) ; Null event OK below.
	     (start (event-start event))
	     (modifiers (event-modifiers event))
	     (window (and (or (memq 'click modifiers) (memq 'down modifiers)
			      (memq 'drag modifiers))
			  (posn-window start))))
	;; For a mouse button event, go to the button it applies to
	;; to get the right key bindings.  And go to the right place
	;; in case the keymap depends on where you clicked.
	(when (windowp window)
	  (set-buffer (window-buffer window))
	  (goto-char (posn-point start)))
	(setq defn (key-binding key))))
    (let ((key-desc (key-description key)))
      (if (or (null defn) (integerp defn))
	  (message "%s is unbound" key-desc)
	(if (consp defn)
	    (message "%s runs %s" key-desc (prin1-to-string defn))
	  (funcall find-fn defn))))))

;;;###autoload
(defun find-function-on-key (key)
  "Find the function that KEY invokes.  KEY is a string.
Set mark before moving, if the buffer already existed."
  (interactive "kFind function on key: ")
  (find-function-on-key-do-it key #'find-function))

;;;###autoload
(defun find-function-on-key-other-window (key)
  "Find, in the other window, the function that KEY invokes.
See `find-function-on-key'."
  (interactive "kFind function on key: ")
  (find-function-on-key-do-it key #'find-function-other-window))

;;;###autoload
(defun find-function-on-key-other-frame (key)
  "Find, in the other frame, the function that KEY invokes.
See `find-function-on-key'."
  (interactive "kFind function on key: ")
  (find-function-on-key-do-it key #'find-function-other-frame))

;;;###autoload
(defun find-function-at-point ()
  "Find directly the function at point in the other window."
  (interactive)
  (let ((symb (function-called-at-point)))
    (when symb
      (find-function-other-window symb))))

;;;###autoload
(defun find-variable-at-point ()
  "Find directly the variable at point in the other window."
  (interactive)
  (let ((symb (variable-at-point)))
    (when (and symb (not (equal symb 0)))
      (find-variable-other-window symb))))

;;;###autoload
(defun find-function-setup-keys ()
  "Define some key bindings for the find-function family of functions."
  (define-key ctl-x-map "F" 'find-function)
  (define-key ctl-x-4-map "F" 'find-function-other-window)
  (define-key ctl-x-5-map "F" 'find-function-other-frame)
  (define-key ctl-x-map "K" 'find-function-on-key)
  (define-key ctl-x-4-map "K" 'find-function-on-key-other-window)
  (define-key ctl-x-5-map "K" 'find-function-on-key-other-frame)
  (define-key ctl-x-map "V" 'find-variable)
  (define-key ctl-x-4-map "V" 'find-variable-other-window)
  (define-key ctl-x-5-map "V" 'find-variable-other-frame))

(provide 'find-func)

;;; find-func.el ends here
