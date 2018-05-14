(defun await (prom)
  (let ((c (cons nil nil)))
    (jsmethod prom "then" `(lambda (x) (setcar ',c t) (setcdr ',c x))))
  (while (not (car c))
    (sit-for 1.0))
  (cdr c))

