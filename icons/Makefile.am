iconsdir = $(datadir)/icons/hicolor

nobase_dist_icons_DATA = \
	hicolor/16x16/apps/hexviewer.png \
	hicolor/22x22/apps/hexviewer.png \
	hicolor/24x24/apps/hexviewer.png \
	hicolor/32x32/apps/hexviewer.png \
	hicolor/48x48/apps/hexviewer.png \
	hicolor/128x128/apps/hexviewer.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi