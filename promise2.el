(defun wait-for-promise (prom)
  (let* ((c (cons nil nil)))
    (jsmethod prom "then"
              `(lambda (x)
                 (setcar ,c t)
                 (setcdr ,c x)))
    (while (not (car c))
      (jsdrain)
      (sit-for 0.0))))
