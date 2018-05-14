(let*
    ((prom (jspromise
            `(lambda (res rej)
               (message "making promise")
               (funcall res 3)
               (message "made promise")))))
  (message "%S" prom)
  (jsmethod prom "then"
            `(lambda (x) (message "resolved to %S" x))))
            (js "(() => { for(;;); })")))

(js "(new Promise(r => r(3))).then(x => console_log(x))")

(let*
    ((glob (jsglobal))
     (console (jsderef glob "console_log"))
     (log (jsderef console "all")))
  (message "%S" console)
  (message "%S" log)
  (jsmethod console "call" "hi"))
