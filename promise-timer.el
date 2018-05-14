(let* ((prom (jspromise (lambda (res rej) (run-with-timer 10 nil `(lambda () (funcall ,res 3)))))))
  (jsmethod prom "then" (lambda (x) (message "resolved to %S" x))))
