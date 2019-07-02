(ql:quickload "cffi-grovel")
(ql:quickload "maxima-client")

(defun start-maxima ()
  (let ((path (make-pathname :directory "/app/maxima-client/")))
    (setq maxima-client.common:*font-directory* (merge-pathnames "fonts/" path))
    (setq maxima-client.common:*image-directory* (merge-pathnames "images/" path))
    (setq maxima-client.common:*info-directory* (merge-pathnames "infoparser/" path))
    (setq freetype2:*library* (freetype2:make-freetype))
    (maxima-client:maxima-client)))

#+nil
(defun start-maxima ()
  (setq freetype2:*library* (freetype2:make-freetype))
  (maxima-client:maxima-client))

(sb-ext:save-lisp-and-die "clim-maxima" :executable t :toplevel #'start-maxima)
