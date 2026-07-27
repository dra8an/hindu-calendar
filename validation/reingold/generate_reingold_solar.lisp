;;;; generate_reingold_solar.lisp
;;;;
;;;; Emit Surya Siddhanta sankranti moments from the Reingold/Dershowitz
;;;; calendar.l, for comparison against drikpanchang.com's "Suryasiddhanta
;;;; Panjika" (drik-arithmetic=suryasiddhanta).
;;;;
;;;; Why moments rather than dates:
;;;;
;;;; calendar.l's `hindu-solar-from-fixed` is documented as the *Orissa* rule
;;;; (critical time = sunrise of the following day).  The Bengali panjika uses
;;;; a midnight-based rule instead.  Emitting whole dates would therefore
;;;; conflate two unknowns -- the arithmetic and the critical-time rule.
;;;;
;;;; So we emit the raw moment at which the Surya Siddhanta solar longitude
;;;; crosses each 30-degree boundary.  Downstream we can apply any candidate
;;;; critical-time rule to those moments and see which reproduces the scraped
;;;; Bengali dates.  That isolates the arithmetic from the rule.
;;;;
;;;; Usage:
;;;;   CALENDAR_L_PATH=/tmp/calendar-code2/calendar.l \
;;;;     sbcl --script validation/reingold/generate_reingold_solar.lisp > out.csv
;;;;
;;;; Output CSV: rashi,greg_year,greg_month,greg_day,ist_hour,ist_minute,ist_second,rd_moment
;;;;   rashi     1..12 (1 = Mesha).  The sankranti INTO this rashi.
;;;;   greg_*    Gregorian date of the moment, in IST.
;;;;   ist_*     Clock time in IST (UTC+5:30), matching drikpanchang display.
;;;;   rd_moment Raw R/D moment (local time at hindu-location), full precision.

(defvar *calendar-path*
  (or (sb-ext:posix-getenv "CALENDAR_L_PATH")
      "/tmp/calendar-code2/calendar.l"))

(defvar *start-year*
  (parse-integer (or (sb-ext:posix-getenv "START_YEAR") "1900")))
(defvar *end-year*
  (parse-integer (or (sb-ext:posix-getenv "END_YEAR") "2050")))

;; calendar.l does (in-package "CC4"), so the package must exist first.
(defpackage "CC4" (:use "COMMON-LISP"))
(in-package "CC4")

;; calendar.l has undefined vars for unrelated calendars; muffle the noise.
(handler-bind ((warning #'muffle-warning))
  (load cl-user::*calendar-path*))

;;; Match the scrape: drikpanchang was scraped for New Delhi, so the Hindu
;;; location must be New Delhi rather than calendar.l's default Ujjain.
;;; `sidereal-start` is derived from hindu-location, so it must be recomputed.
(handler-bind ((sb-ext:defconstant-uneql #'continue))
  (defconstant hindu-location
    (location (angle 28 36 50)     ; 28.6139 N
              (angle 77 12 32)     ; 77.2090 E
              (mt 0)
              (hr (+ 5 1/2))))     ; UTC+5:30
  (defconstant sidereal-start
    (precession (universal-from-local
                 (mesha-samkranti (ce 285))
                 hindu-location))))

(format *error-output* "hindu-location overridden to New Delhi~%")
(format *error-output* "range: ~D-01-01 .. ~D-12-31~%"
        cl-user::*start-year* cl-user::*end-year*)
(force-output *error-output*)

(defvar *start-fixed*
  (fixed-from-gregorian (gregorian-date cl-user::*start-year* january 1)))
(defvar *end-fixed*
  (fixed-from-gregorian (gregorian-date cl-user::*end-year* december 31)))

;;; Convert an R/D moment (local time at hindu-location) to IST standard time.
(defun ist-from-local (tee)
  (standard-from-universal (universal-from-local tee hindu-location)
                           hindu-location))

(format t "rashi,greg_year,greg_month,greg_day,ist_hour,ist_minute,ist_second,rd_moment~%")

(let ((count 0)
      (tee *start-fixed*))
  (loop
    (let* ((lon (hindu-solar-longitude tee))
           ;; Next 30-degree boundary strictly after the current longitude.
           (target (mod (* 30 (1+ (floor lon 30))) 360))
           (moment (hindu-solar-longitude-at-or-after (deg target) tee)))
      (when (> moment *end-fixed*) (return))
      (let* ((ist (ist-from-local moment))
             (day (fixed-from-moment ist))
             (g (gregorian-from-fixed day))
             (secs (round (* (time-from-moment ist) 86400)))
             (h (floor secs 3600))
             (m (floor (mod secs 3600) 60))
             (s (mod secs 60))
             (rashi (1+ (floor target 30))))
        (format t "~D,~D,~D,~D,~D,~D,~D,~,6F~%"
                rashi
                (standard-year g) (standard-month g) (standard-day g)
                h m s (float moment 1.0L0))
        (incf count)
        (when (zerop (mod count 200))
          (format *error-output* "  ~D sankrantis (~D-~2,'0D)...~%"
                  count (standard-year g) (standard-month g))
          (force-output *error-output*)))
      ;; Step past this crossing so the next iteration finds the following one.
      (setf tee (+ moment 1))))
  (format *error-output* "Done: ~D sankrantis emitted.~%" count))
