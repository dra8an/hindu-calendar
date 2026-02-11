;;; generate_reingold.lisp — Generate Hindu calendar data from Reingold/Dershowitz
;;;
;;; Usage: Set CALENDAR_L_PATH env var, then:
;;;   CALENDAR_L_PATH=/path/to/calendar.l sbcl --noinform --non-interactive --load generate_reingold.lisp
;;;
;;; Output CSV to stdout. Progress to stderr.
;;;
;;; Output CSV columns:
;;;   year,month,day,hl_tithi,hl_masa,hl_adhika,hl_vikrama,al_tithi,al_masa,al_adhika,al_vikrama
;;;
;;; Where hl_ = hindu-lunar-from-fixed, al_ = astro-hindu-lunar-from-fixed

;; Get calendar.l path from environment variable
(defvar *calendar-path*
  (or (sb-ext:posix-getenv "CALENDAR_L_PATH")
      (error "Set CALENDAR_L_PATH environment variable to path of calendar.l")))

;; Create the CC4 package that calendar.l expects
(defpackage "CC4" (:use "COMMON-LISP"))
(in-package "CC4")

;; Suppress warnings during load (calendar.l has undefined vars for
;; Icelandic calendar constants etc. that don't affect Hindu functions)
(handler-bind ((warning #'muffle-warning))
  (load cl-user::*calendar-path*))

;; Override hindu-location from Ujjain to New Delhi (28.6139°N, 77.2090°E, UTC+5:30)
;; This isolates diffs to ephemeris/model only, eliminating location as a variable.
;; Original: (location (angle 23 9 0) (angle 75 46 6) (mt 0) (hr (+ 5 461/9000)))
(handler-bind ((sb-ext:defconstant-uneql #'continue))
  (defconstant hindu-location
    (location (angle 28 36 50)    ; 28.6139°N = 28°36'50"
              (angle 77 12 32)    ; 77.2090°E = 77°12'32"
              (mt 0)
              (hr (+ 5 1/2))))    ; UTC+5:30
  ;; Recompute sidereal-start since it depends on hindu-location
  (defconstant sidereal-start
    (precession (universal-from-local
                 (mesha-samkranti (ce 285))
                 hindu-location))))

(format *error-output* "hindu-location overridden to New Delhi~%")
(force-output *error-output*)

;; Helper: convert boolean to 0/1
(defun bool-to-int (x)
  (if x 1 0))

;; Fixed date range: 1900-01-01 to 2050-12-31
(defvar *start-fixed* (fixed-from-gregorian (gregorian-date 1900 1 1)))
(defvar *end-fixed*   (fixed-from-gregorian (gregorian-date 2050 12 31)))

;; Write CSV header
(format t "year,month,day,hl_tithi,hl_masa,hl_adhika,hl_vikrama,al_tithi,al_masa,al_adhika,al_vikrama~%")

;; Progress tracking to stderr
(defvar *total* (1+ (- *end-fixed* *start-fixed*)))
(defvar *count* 0)

;; Iterate over every date
(loop for fixed from *start-fixed* to *end-fixed* do
  (let* ((greg (gregorian-from-fixed fixed))
         (gy (first greg))
         (gm (second greg))
         (gd (third greg))
         ;; Hindu lunar (Surya Siddhanta with epicycles)
         (hl (hindu-lunar-from-fixed fixed))
         (hl-tithi   (hindu-lunar-day hl))
         (hl-masa    (hindu-lunar-month hl))
         (hl-adhika  (bool-to-int (hindu-lunar-leap-month hl)))
         (hl-vikrama (hindu-lunar-year hl))
         ;; Astro Hindu lunar (astronomical ephemeris)
         (al (astro-hindu-lunar-from-fixed fixed))
         (al-tithi   (hindu-lunar-day al))
         (al-masa    (hindu-lunar-month al))
         (al-adhika  (bool-to-int (hindu-lunar-leap-month al)))
         (al-vikrama (hindu-lunar-year al)))
    ;; Write CSV row
    (format t "~D,~D,~D,~D,~D,~D,~D,~D,~D,~D,~D~%"
            gy gm gd
            hl-tithi hl-masa hl-adhika hl-vikrama
            al-tithi al-masa al-adhika al-vikrama)
    ;; Progress every 10000 dates
    (incf *count*)
    (when (zerop (mod *count* 10000))
      (format *error-output* "Progress: ~D/~D (~,1F%)~%"
              *count* *total* (* 100.0 (/ *count* *total*)))
      (force-output *error-output*))))

;; Final progress
(format *error-output* "Done: ~D dates processed.~%" *count*)
(force-output *error-output*)
