;; $Id: methods-full.el,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $
;;
;;  methods-full.el
;;
;;  Asks for the name of a Ctalk class and displays a list of the
;;  class's method prototypes and documentation.
;;
;;  To run, read this file into an Emacs buffer, then execute the commands,
;;
;;  M-x eval-buffer
;;  M-x methods-full
;;

(defun methods-full ()
  (interactive)
  (setq classname (read-from-minibuffer "Ctalk class: "))
  (setq method-buffer-name (format "%s Methods | Full Listing" classname))
  (start-process "ctalk-methods-full" method-buffer-name "methods" 
		 "-p" "-d" classname)
  (set-buffer method-buffer-name)
  (make-frame '((height . 35) (width . 65))))
