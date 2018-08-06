(let*
    ((prom (jspromise
            (lambda (res rej)
              (run-with-timer 10 nil `(lambda () (funcall ,res 3))))))
     (c (cons nil nil)))
  (jsmethod prom "then" `(lambda (x) (setcar ',c t) (setcdr ',c x)))
  (while (not (car c))
    (message "%S %S" (jsdrain) c)
    (sit-for 1.0))
  (message "resolved to %S" (cdr c)))

