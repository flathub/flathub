
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

class FilterMixin(object):

	# add files types filters
	# when user select "Python files" for example,
	# only python files are displyed
	def add_filters(self, dialog):
		filter_text = Gtk.FileFilter()
		filter_text.set_name("Text files")
		filter_text.add_mime_type("text/plain")
		dialog.add_filter(filter_text)
		
		filter_py = Gtk.FileFilter()
		filter_py.set_name("Python files")
		filter_py.add_mime_type("text/x-python")
		dialog.add_filter(filter_py)
		
		filter_perl = Gtk.FileFilter()
		filter_perl.set_name("Perl files")
		filter_perl.add_mime_type("application/x-perl")
		dialog.add_filter(filter_perl)
		
		filter_c = Gtk.FileFilter()
		filter_c.set_name("C/C++ files")
		filter_c.add_mime_type("text/x-c")
		dialog.add_filter(filter_c)
		
		filter_makefile = Gtk.FileFilter()
		filter_makefile.set_name("Makefile files")
		filter_makefile.add_mime_type("text/x-makefile")
		dialog.add_filter(filter_makefile)
		
		filter_js = Gtk.FileFilter()
		filter_js.set_name("Javascript files")
		filter_js.add_mime_type("application/javascript")
		dialog.add_filter(filter_js)
		
		filter_html = Gtk.FileFilter()
		filter_html.set_name("HTML files")
		filter_html.add_mime_type("text/html")
		dialog.add_filter(filter_html)
		
		filter_css = Gtk.FileFilter()
		filter_css.set_name("CSS files")
		filter_css.add_mime_type("text/css")
		dialog.add_filter(filter_css)
						
		filter_json = Gtk.FileFilter()
		filter_json.set_name("JSON files")
		filter_json.add_mime_type("application/json")
		dialog.add_filter(filter_json)
			
		filter_xml = Gtk.FileFilter()
		filter_xml.set_name("XML files")
		filter_xml.add_mime_type("text/xml")
		dialog.add_filter(filter_xml)		
		
		filter_readme = Gtk.FileFilter()
		filter_readme.set_name("README files")
		filter_readme.add_mime_type("text/x-readme")
		dialog.add_filter(filter_readme)
		
		filter_dart = Gtk.FileFilter()
		filter_dart.set_name("Dart files")
		filter_dart.add_mime_type("application/vnd.dart")
		dialog.add_filter(filter_dart)
		
		filter_ruby = Gtk.FileFilter()
		filter_ruby.set_name("Ruby files")
		filter_ruby.add_mime_type("application/x-ruby")
		dialog.add_filter(filter_ruby)
			
		filter_latex = Gtk.FileFilter()
		filter_latex.set_name("Latex files")
		filter_latex.add_mime_type("application/x-latex")
		dialog.add_filter(filter_latex)
		
		filter_bib = Gtk.FileFilter()
		filter_bib.set_name("Bib files")
		filter_bib.add_mime_type("text/x-bibtex")
		dialog.add_filter(filter_bib)
		
		filter_sql = Gtk.FileFilter()
		filter_sql.set_name("SQL files")
		filter_sql.add_mime_type("application/x-sql")
		dialog.add_filter(filter_sql)
		
		filter_appache = Gtk.FileFilter()
		filter_appache.set_name("Appcache files")
		filter_appache.add_mime_type("text/cache-manifest")
		dialog.add_filter(filter_appache)

		filter_log = Gtk.FileFilter()
		filter_log.set_name("Log files")
		filter_log.add_mime_type("text/x-log")
		dialog.add_filter(filter_log)
		
		filter_cal = Gtk.FileFilter()
		filter_cal.set_name("Calendar files")
		filter_cal.add_mime_type("text/calendar")
		dialog.add_filter(filter_cal)
		
		filter_csv = Gtk.FileFilter()
		filter_csv.set_name("CSV files")
		filter_csv.add_mime_type("text/csv")
		filter_csv.add_mime_type("text/x-comma-separated-values")
		dialog.add_filter(filter_csv)
		
		filter_asm = Gtk.FileFilter()
		filter_asm.set_name("Assembly files")
		filter_asm.add_mime_type("text/x-asm")
		dialog.add_filter(filter_asm)	
		
		filter_fortran = Gtk.FileFilter()
		filter_fortran.set_name("Fortran files")
		filter_fortran.add_mime_type("text/x-fortran")
		dialog.add_filter(filter_fortran)
		
		filter_haskell = Gtk.FileFilter()
		filter_haskell.set_name("Haskell files")
		filter_haskell.add_mime_type("text/x-haskell")
		dialog.add_filter(filter_haskell)
		
		filter_java = Gtk.FileFilter()
		filter_java.set_name("Java files")
		filter_java.add_mime_type("text/x-java")
		dialog.add_filter(filter_java)
		
		filter_elisp = Gtk.FileFilter()
		filter_elisp.set_name("Emacs-Lisp files")
		filter_elisp.add_mime_type("text/x-emacs-lisp")
		dialog.add_filter(filter_elisp)
		
		filter_glade = Gtk.FileFilter()
		filter_glade.set_name("Glade files")
		filter_glade.add_mime_type("application/x-glade")
		dialog.add_filter(filter_glade)
		
		filter_any = Gtk.FileFilter()
		filter_any.set_name("Any files")
		filter_any.add_pattern("*")
		dialog.add_filter(filter_any)
		
		
		