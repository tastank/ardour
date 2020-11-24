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

#ifndef __gtkardour_rec_time_axis_h__
#define __gtkardour_rec_time_axis_h__

#include <vector>
#include <cmath>

#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/separator.h>

#include "pbd/stateful.h"

#include "ardour/types.h"
#include "ardour/ardour.h"

#include "widgets/ardour_button.h"

#include "level_meter.h"
#include "route_ui.h"

namespace ARDOUR {
	class Route;
	class RouteGroup;
	class Session;
}

class RecTimeAxis : public Gtk::VBox, public AxisView, public RouteUI
{
public:
	RecTimeAxis (ARDOUR::Session*, boost::shared_ptr<ARDOUR::Route>);
	~RecTimeAxis ();

	/* AxisView */
	std::string name() const;
	Gdk::Color color () const;

	boost::shared_ptr<ARDOUR::Stripable> stripable() const { return RouteUI::stripable(); }

	void set_session (ARDOUR::Session* s);

	void fast_update ();

	static PBD::Signal1<void,RecTimeAxis*> CatchDeletion;

protected:
	void self_delete ();

	void on_size_allocate (Gtk::Allocation&);
	void on_size_request (Gtk::Requisition*);

	/* AxisView */
	std::string state_id() const;

	/* route UI */
	void set_button_names ();
	void blink_rec_display (bool onoff);

private:
	void on_theme_changed ();
	void parameter_changed (std::string const & p);

	/* RouteUI */
	void route_property_changed (const PBD::PropertyChange&);
	void route_color_changed ();

	PBD::ScopedConnectionList _route_connections;
	Gtk::Table hdr;
	//LevelMeterHBox *level_meter;
};

#endif
