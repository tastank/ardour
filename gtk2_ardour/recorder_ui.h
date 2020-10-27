/*
 * Copyright (C) 2020 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __gtk_ardour_recorder_ui_h__
#define __gtk_ardour_recorder_ui_h__

#include <boost/shared_ptr.hpp>

#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>

#include "ardour/session_handle.h"

#include "gtkmm2ext/bindings.h"

#include "widgets/ardour_button.h"
#include "widgets/tabbable.h"

namespace ArdourWidgets {
	class FastMeter;
}

class RecorderUI : public ArdourWidgets::Tabbable, public ARDOUR::SessionHandlePtr, public PBD::ScopedConnectionList
{
public:
	RecorderUI ();
	~RecorderUI ();
	void set_session (ARDOUR::Session*);
	void cleanup ();

	Gtk::Window* use_own_window (bool and_fill_it);

private:
	void load_bindings ();
	void register_actions ();
	void update_title ();
	void session_going_away ();
	void parameter_changed (std::string const&);

	void start_updating ();
	void stop_updating ();
	void update_meters ();

	Gtkmm2ext::Bindings*       bindings;
	Gtk::VBox                 _content;
	Gtk::HBox                 _rulers;
	Gtk::HBox                 _meterarea;
	Gtk::ScrolledWindow       _scroller;
	sigc::connection          _fast_screen_update_connection;
	PBD::ScopedConnectionList _engine_connections;

	class InputMeter : public Gtk::VBox
	{
		public:
			InputMeter (std::string const&);
			~InputMeter ();
			void update (float, float);

		private:
			ArdourWidgets::FastMeter*   _meter;
			ArdourWidgets::ArdourButton _name_label;
	};

	std::vector<boost::shared_ptr<InputMeter> > _input_meters;
};

#endif /* __gtk_ardour_recorder_ui_h__ */
