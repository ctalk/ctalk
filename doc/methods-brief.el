;; $Id: methods-brief.el,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $
;;
;;  methods-brief.el
;;
;;  Asks for the name of a Ctalk class and displays a brief listing
;;  of the class's methods.
;;
;;  To run, read this file into an Emacs buffer, then execute the commands,
;;
;;  M-x eval-buffer
;;  M-x methods-brief

(defun methods-brief ()
  (interactive)
  (setq classname (read-from-minibuffer "Ctalk class: "))
  (setq method-buffer-name (format "%s Methods | Brief Listing" classname))
  (start-process "ctalk-methods-brief" method-buffer-name "methods" classname)
  (set-buffer method-buffer-name)
  (make-frame '((height . 35) (width . 40))))
