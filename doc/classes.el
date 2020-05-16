;; $Id: classes.el,v 1.1.1.1 2020/05/16 02:37:00 rkiesling Exp $
;;
;; classes.el
;;
;; Displays the Ctalk class hierarchy in an Emacs window.
;;
;; To run, read this file into  an Emacs buffer, then enter the
;; following Emacs commands:
;;
;; M-x eval-buffer
;; M-x classes
;;
(defun classes ()
  (interactive)
  (start-process "ctalk-classes" "Ctalk Classes" "classes")
  (set-buffer "Ctalk Classes")
  (make-frame '((height . 35) (width . 40))))


