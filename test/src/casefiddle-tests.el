;;; casefiddle-tests.el --- tests for casefiddle.c functions -*- lexical-binding: t -*-

;; Copyright (C) 2015-2016 Free Software Foundation, Inc.

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

;;; Code:

(require 'case-table)
(require 'ert)

(ert-deftest casefiddle-tests-char-properties ()
  "Sanity check of character Unicode properties."
  (should-not
   (let (errors)
     ;;            character  uppercase  lowercase  titlecase
     (dolist (test '((?A nil ?a nil)
                     (?a ?A nil ?A)
                     (?Ł nil ?ł nil)
                     (?ł ?Ł nil ?Ł)

                     (?Ǆ nil ?ǆ ?ǅ)
                     (?ǅ ?Ǆ ?ǆ ?ǅ)
                     (?ǆ ?Ǆ nil ?ǅ)

                     (?Σ nil ?σ nil)
                     (?σ ?Σ nil ?Σ)
                     (?ς ?Σ nil ?Σ)

                     (?ⅷ ?Ⅷ nil ?Ⅷ)
                     (?Ⅷ nil ?ⅷ nil)))
       (let ((ch (car test))
             (expected (cdr test))
             (props '(uppercase lowercase titlecase)))
         (while props
           (let ((got (get-char-code-property ch (car props))))
             (unless (equal (car expected) got)
               (push (format "\n%c %s; expected: %s but got: %s"
                             ch (car props) (car expected) got)
                     errors)))
           (setq props (cdr props) expected (cdr expected)))))
     (when errors
       (mapconcat (lambda (line) line) (nreverse errors) "")))))


(defconst casefiddle-tests--characters
  ;; character  uppercase  lowercase  titlecase
  '((?A ?A ?a ?A)
    (?a ?A ?a ?A)
    (?Ł ?Ł ?ł ?Ł)
    (?ł ?Ł ?ł ?Ł)

    ;; FIXME(bug#24603): Commented ones are what we want.
    ;;(?Ǆ ?Ǆ ?ǆ ?ǅ)
    (?Ǆ ?Ǆ ?ǆ ?Ǆ)
    ;;(?ǅ ?Ǆ ?ǆ ?ǅ)
    (?ǅ ?Ǆ ?ǆ ?Ǆ)
    ;;(?ǆ ?Ǆ ?ǆ ?ǅ)
    (?ǆ ?Ǆ ?ǆ ?Ǆ)

    (?Σ ?Σ ?σ ?Σ)
    (?σ ?Σ ?σ ?Σ)
    (?ς ?Σ ?ς ?Σ)

    (?Ⅷ ?Ⅷ ?ⅷ ?Ⅷ)
    (?ⅷ ?Ⅷ ?ⅷ ?Ⅷ)))


(ert-deftest casefiddle-tests-case-table ()
  "Sanity check of down and up case tables."
  (should-not
   (let (errors
         (up (case-table-get-table (current-case-table) 'up))
         (down (case-table-get-table (current-case-table) 'down)))
     (dolist (test casefiddle-tests--characters)
       (let ((ch (car test))
             (expected (cdr test))
             (props '(uppercase lowercase))
             (tabs (list up down)))
         (while props
           (let ((got (aref (car tabs) ch)))
             (unless (equal (car expected) got)
               (push (format "\n%c %s; expected: %s but got: %s"
                             ch (car props) (car expected) got)
                     errors)))
           (setq props (cdr props) tabs (cdr tabs) expected (cdr expected)))))
     (when errors
       (mapconcat (lambda (line) line) (nreverse errors) "")))))


(ert-deftest casefiddle-tests-casing-character ()
  (should-not
   (let (errors)
     (dolist (test casefiddle-tests--characters)
       (let ((ch (car test))
             (expected (cdr test))
             (funcs '(upcase downcase capitalize)))
         (while funcs
           (let ((got (funcall (car funcs) ch)))
             (unless (equal (car expected) got)
               (push (format "\n%c %s; expected: %s but got: %s"
                             ch (car funcs) (car expected) got)
                     errors)))
           (setq funcs (cdr funcs) expected (cdr expected)))))
     (when errors
       (mapconcat (lambda (line) line) (nreverse errors) "")))))


(ert-deftest casefiddle-tests-casing-word ()
  (with-temp-buffer
    (dolist (test '((upcase-word     . "FOO Bar")
                    (downcase-word   . "foo Bar")
                    (capitalize-word . "Foo Bar")))
      (dolist (back '(nil t))
        (delete-region (point-min) (point-max))
        (insert "foO Bar")
        (goto-char (+ (if back 4 0) (point-min)))
        (funcall (car test) (if back -1 1))
        (should (string-equal (cdr test) (buffer-string)))
        (should (equal (+ (if back 4 3) (point-min)) (point)))))))


(defun casefiddle-tests--test-casing (tests)
  (nreverse
   (cl-reduce
    (lambda (errors test)
      (let* ((input (car test))
             (expected (cdr test))
             (func-pairs '((upcase upcase-region)
                           (downcase downcase-region)
                           (capitalize capitalize-region)
                           (upcase-initials upcase-initials-region)))
             (get-string (lambda (func) (funcall func input)))
             (get-region (lambda (func)
                           (delete-region (point-min) (point-max))
                           (unwind-protect
                               (progn
                                 (unless (multibyte-string-p input)
                                   (toggle-enable-multibyte-characters))
                                 (insert input)
                                 (funcall func (point-min) (point-max))
                                 (buffer-string))
                             (unless (multibyte-string-p input)
                               (toggle-enable-multibyte-characters)))))
             (fmt-str (lambda (str)
                        (format "%s  (%sbyte; %d chars; %d bytes)"
                                str
                                (if (multibyte-string-p str) "multi" "uni")
                                (length str) (string-bytes str))))
             funcs getters)
        (while (and func-pairs expected)
          (setq funcs (car func-pairs)
                getters (list get-string get-region))
          (while (and funcs getters)
            (let ((got (funcall (car getters) (car funcs))))
              (unless (string-equal got (car expected))
                (let ((fmt (length (symbol-name (car funcs)))))
                  (setq fmt (format "\n%%%ds: %%s" (max fmt 8)))
                  (push (format (concat fmt fmt fmt)
                                (car funcs) (funcall fmt-str input)
                                "expected" (funcall fmt-str (car expected))
                                "but got" (funcall fmt-str got))
                        errors))))
            (setq funcs (cdr funcs) getters (cdr getters)))
          (setq func-pairs (cdr func-pairs) expected (cdr expected))))
      errors)
    (cons () tests))))

(ert-deftest casefiddle-tests-casing ()
  (should-not
   (with-temp-buffer
     (casefiddle-tests--test-casing
      ;; input     upper     lower    capitalize up-initials
      '(("Foo baR" "FOO BAR" "foo bar" "Foo Bar" "Foo BaR")
        ("Ⅷ ⅷ" "Ⅷ Ⅷ" "ⅷ ⅷ" "Ⅷ Ⅷ" "Ⅷ Ⅷ")
        ;; FIXME(bug#24603): Everything below is broken at the moment.
        ;; Here’s what should happen:
        ;;("ǄUNGLA" "ǄUNGLA" "ǆungla" "ǅungla" "ǅUNGLA")
        ;;("ǅungla" "ǄUNGLA" "ǆungla" "ǅungla" "ǅungla")
        ;;("ǆungla" "ǄUNGLA" "ǆungla" "ǅungla" "ǅungla")
        ;;("deﬁne" "DEFINE" "deﬁne" "Deﬁne" "Deﬁne")
        ;;("ﬁsh" "FIsh" "ﬁsh" "Fish" "Fish")
        ;;("Straße" "STRASSE" "straße" "Straße" "Straße")
        ;;("ΌΣΟΣ" "ΌΣΟΣ" "όσος" "Όσος" "Όσος")
        ;; And here’s what is actually happening:
        ("ǄUNGLA" "ǄUNGLA" "ǆungla" "Ǆungla" "ǄUNGLA")
        ("ǅungla" "ǄUNGLA" "ǆungla" "Ǆungla" "Ǆungla")
        ("ǆungla" "ǄUNGLA" "ǆungla" "Ǆungla" "Ǆungla")
        ("deﬁne" "DEﬁNE" "deﬁne" "Deﬁne" "Deﬁne")
        ("ﬁsh" "ﬁSH" "ﬁsh" "ﬁsh" "ﬁsh")
        ("Straße" "STRAßE" "straße" "Straße" "Straße")
        ("ΌΣΟΣ" "ΌΣΟΣ" "όσοσ" "Όσοσ" "ΌΣΟΣ")

        ("όσος" "ΌΣΟΣ" "όσος" "Όσος" "Όσος"))))))

(ert-deftest casefiddle-tests-casing-byte8 ()
  (should-not
   (with-temp-buffer
     (casefiddle-tests--test-casing
      '(("\xff Foo baR \xff"
         "\xff FOO BAR \xff"
         "\xff foo bar \xff"
         "\xff Foo Bar \xff"
         "\xff Foo BaR \xff")
        ("\xff Zażółć gĘŚlą \xff"
         "\xff ZAŻÓŁĆ GĘŚLĄ \xff"
         "\xff zażółć gęślą \xff"
         "\xff Zażółć Gęślą \xff"
         "\xff Zażółć GĘŚlą \xff"))))))

(ert-deftest casefiddle-tests-casing-byte8-with-changes ()
  (let ((tab (copy-case-table (standard-case-table)))
        (test '("\xff\xff\xef Foo baR \xcf\xcf"
                "\xef\xef\xef FOO BAR \xcf\xcf"
                "\xff\xff\xff foo bar \xcf\xcf"
                "\xef\xff\xff Foo Bar \xcf\xcf"
                "\xef\xff\xef Foo BaR \xcf\xcf"))
        (byte8 #x3FFF00))
    (should-not
     (with-temp-buffer
       (set-case-table tab)
       (set-case-syntax-pair (+ byte8 #xef) (+ byte8 #xff) tab)
       (casefiddle-tests--test-casing
        (list test
              (mapcar (lambda (str) (decode-coding-string str 'binary)) test)
              '("\xff\xff\xef Zażółć gĘŚlą \xcf\xcf"
                "\xef\xef\xef ZAŻÓŁĆ GĘŚLĄ \xcf\xcf"
                "\xff\xff\xff zażółć gęślą \xcf\xcf"
                "\xef\xff\xff Zażółć Gęślą \xcf\xcf"
                "\xef\xff\xef Zażółć GĘŚlą \xcf\xcf")))))))


;;; casefiddle-tests.el ends here
