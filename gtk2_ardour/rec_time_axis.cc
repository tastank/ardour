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

#include <list>

#include <sigc++/bind.h>

#include "pbd/unwind.h"

#include "ardour/logmeter.h"
#include "ardour/session.h"
#include "ardour/route.h"
#include "ardour/route_group.h"
#include "ardour/meter.h"

#include "ardour/audio_track.h"
#include "ardour/midi_track.h"

#include "gtkmm2ext/gtk_ui.h"
#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/rgb_macros.h"

#include "widgets/tooltips.h"

#include "gui_thread.h"
#include "ardour_window.h"
#include "context_menu_helper.h"
#include "ui_config.h"
#include "utils.h"

#include "rec_time_axis.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace ArdourWidgets;
using namespace ARDOUR_UI_UTILS;
using namespace PBD;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace std;

PBD::Signal1<void,RecTimeAxis*> RecTimeAxis::CatchDeletion;

#define PX_SCALE(pxmin, dflt) rint(std::max((double)pxmin, (double)dflt * UIConfiguration::instance().get_ui_scale()))

RecTimeAxis::RecTimeAxis (Session* s, boost::shared_ptr<ARDOUR::Route> rt)
	: SessionHandlePtr (s)
	, RouteUI (s)
{
	RouteUI::set_route (rt);

	_route->DropReferences.connect (_route_connections, invalidator (*this), boost::bind (&RecTimeAxis::self_delete, this), gui_context());

	UI::instance()->theme_changed.connect (sigc::mem_fun(*this, &RecTimeAxis::on_theme_changed));
	UIConfiguration::instance().ColorsChanged.connect (sigc::mem_fun (*this, &RecTimeAxis::on_theme_changed));
	UIConfiguration::instance().DPIReset.connect (sigc::mem_fun (*this, &RecTimeAxis::on_theme_changed));

	Config->ParameterChanged.connect (*this, invalidator (*this), ui_bind (&RecTimeAxis::parameter_changed, this, _1), gui_context());
	s->config.ParameterChanged.connect (*this, invalidator (*this), ui_bind (&RecTimeAxis::parameter_changed, this, _1), gui_context());
}

RecTimeAxis::~RecTimeAxis ()
{
	CatchDeletion (this);
}

void
RecTimeAxis::self_delete ()
{
	delete this;
}

void
RecTimeAxis::set_session (Session* s)
{
	RouteUI::set_session (s);
	if (!s) return;
	s->config.ParameterChanged.connect (*this, invalidator (*this), ui_bind (&RecTimeAxis::parameter_changed, this, _1), gui_context());
}

void
RecTimeAxis::blink_rec_display (bool onoff)
{
	RouteUI::blink_rec_display (onoff);
}

std::string
RecTimeAxis::state_id() const
{
	if (_route) {
		return string_compose ("recta %1", _route->id().to_s());
	} else {
		return string ();
	}
}

void
RecTimeAxis::set_button_names()
{
	mute_button->set_text (S_("Mute|M"));
	monitor_input_button->set_text (S_("MonitorInput|I"));
	monitor_disk_button->set_text (S_("MonitorDisk|D"));

	/* Solo/Listen is N/A */
}

void
RecTimeAxis::route_property_changed (const PropertyChange& what_changed)
{
	if (!what_changed.contains (ARDOUR::Properties::name)) {
		return;
	}
	ENSURE_GUI_THREAD (*this, &RecTimeAxis::strip_name_changed, what_changed);
#if 0
	name_changed();
	set_tooltip (name_label, _route->name());
	if (level_meter) {
		set_tooltip (*level_meter, _route->name());
	}
#endif
}

void
RecTimeAxis::route_color_changed ()
{
#if 0
	number_label.set_fixed_colors (gdk_color_to_rgba (color()), gdk_color_to_rgba (color()));
#endif
}


void
RecTimeAxis::fast_update ()
{
}

void
RecTimeAxis::on_theme_changed()
{
}

void
RecTimeAxis::on_size_request (Gtk::Requisition* r)
{
	VBox::on_size_request(r);
}

void
RecTimeAxis::on_size_allocate (Gtk::Allocation& a)
{
	VBox::on_size_allocate(a);
}

void
RecTimeAxis::parameter_changed (std::string const & p)
{
}

string
RecTimeAxis::name () const
{
	return _route->name();
}

Gdk::Color
RecTimeAxis::color () const
{
	return RouteUI::route_color ();
}
