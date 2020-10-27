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

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

#include "ardour/audioengine.h"
#include "ardour/dB.h"
#include "ardour/logmeter.h"
#include "ardour/session.h"

#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/gtk_ui.h"
#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/window_title.h"

#include "widgets/fastmeter.h"
#include "widgets/tooltips.h"

#include "actions.h"
#include "ardour_ui.h"
#include "gui_thread.h"
#include "recorder_ui.h"
#include "timers.h"
#include "ui_config.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace Gtkmm2ext;
using namespace ArdourWidgets;
using namespace Gtk;
using namespace std;

RecorderUI::RecorderUI ()
	: Tabbable (_content, _("Recoder"), X_("recorder"))
{
	load_bindings ();
	register_actions ();

	_scroller.add (_meterarea);
	_meterarea.set_spacing (2);

	_content.pack_start (_rulers, true, true);
	_content.pack_start (_scroller, true, true);
	_content.set_data ("ardour-bindings", bindings);

	update_title ();

	_rulers.show ();
	_meterarea.show ();
	_scroller.show ();
	_content.show ();

	AudioEngine::instance ()->Running.connect (_engine_connections, invalidator (*this), boost::bind (&RecorderUI::start_updating, this), gui_context ());
	AudioEngine::instance ()->Stopped.connect (_engine_connections, invalidator (*this), boost::bind (&RecorderUI::stop_updating, this), gui_context ());
	AudioEngine::instance ()->Halted.connect (_engine_connections, invalidator (*this), boost::bind (&RecorderUI::stop_updating, this), gui_context ());

	//ARDOUR_UI::instance()->Escape.connect (*this, invalidator (*this), boost::bind (&RecorderUI::escape, this), gui_context());
}

RecorderUI::~RecorderUI ()
{
}

void
RecorderUI::cleanup ()
{
	stop_updating ();
	_engine_connections.drop_connections ();
}

Gtk::Window*
RecorderUI::use_own_window (bool and_fill_it)
{
	bool new_window = !own_window ();

	Gtk::Window* win = Tabbable::use_own_window (and_fill_it);

	if (win && new_window) {
		win->set_name ("RecorderWindow");
		ARDOUR_UI::instance ()->setup_toplevel_window (*win, _("Recorder"), this);
		win->signal_event ().connect (sigc::bind (sigc::ptr_fun (&Keyboard::catch_user_event_for_pre_dialog_focus), win));
		win->set_data ("ardour-bindings", bindings);
		update_title ();
#if 0 // TODO
		if (!win->get_focus()) {
			win->set_focus (scroller);
		}
#endif
	}

	//contents ().show_all ();

	return win;
}

void
RecorderUI::load_bindings ()
{
	bindings = Bindings::get_bindings (X_("Recorder"));
}

void
RecorderUI::register_actions ()
{
	Glib::RefPtr<ActionGroup> group = ActionManager::create_action_group (bindings, X_("Recorder"));
}

void
RecorderUI::set_session (ARDOUR::Session* s)
{
	SessionHandlePtr::set_session (s);

	if (!_session) {
		return;
	}

	_session->DirtyChanged.connect (_session_connections, invalidator (*this), boost::bind (&RecorderUI::update_title, this), gui_context ());
	_session->StateSaved.connect (_session_connections, invalidator (*this), boost::bind (&RecorderUI::update_title, this), gui_context ());

	_session->config.ParameterChanged.connect (_session_connections, invalidator (*this), boost::bind (&RecorderUI::parameter_changed, this, _1), gui_context ());
	Config->ParameterChanged.connect (*this, invalidator (*this), boost::bind (&RecorderUI::parameter_changed, this, _1), gui_context ());

	update_title ();
	start_updating ();
}

void
RecorderUI::session_going_away ()
{
	ENSURE_GUI_THREAD (*this, &RecorderUI::session_going_away);
	SessionHandlePtr::session_going_away ();
	update_title ();
}

void
RecorderUI::update_title ()
{
	if (!own_window ()) {
		return;
	}

	if (_session) {
		string n;

		if (_session->snap_name () != _session->name ()) {
			n = _session->snap_name ();
		} else {
			n = _session->name ();
		}

		if (_session->dirty ()) {
			n = "*" + n;
		}

		WindowTitle title (n);
		title += S_("Window|Recorder");
		title += Glib::get_application_name ();
		own_window ()->set_title (title.get_string ());

	} else {
		WindowTitle title (S_("Window|Recorder"));
		title += Glib::get_application_name ();
		own_window ()->set_title (title.get_string ());
	}
}

void
RecorderUI::parameter_changed (string const& p)
{
}

void
RecorderUI::start_updating ()
{
	if (_input_meters.size ()) {
		stop_updating ();
	}

	PortManager::PortDPM const& dpm (AudioEngine::instance ()->input_meters ());

	for (PortManager::PortDPM::const_iterator i = dpm.begin (); i != dpm.end (); ++i) {
		_input_meters.push_back (boost::shared_ptr<RecorderUI::InputMeter> (new InputMeter (i->first)));
		_meterarea.pack_start (*_input_meters.back (), false, false);
		_input_meters.back ()->show ();
	}

	_fast_screen_update_connection = Timers::super_rapid_connect (sigc::mem_fun (*this, &RecorderUI::update_meters));
}

void
RecorderUI::stop_updating ()
{
	_fast_screen_update_connection.disconnect ();
	container_clear (_meterarea);
	_input_meters.clear ();
}

void
RecorderUI::update_meters ()
{
	if (!contents ().is_mapped ()) {
		return;
	}
	PortManager::PortDPM const& dpm (AudioEngine::instance ()->input_meters ());
	assert (dpm.size () == _input_meters.size ());
	std::vector<boost::shared_ptr<InputMeter>>::iterator m = _input_meters.begin ();
	for (PortManager::PortDPM::const_iterator i = dpm.begin (); i != dpm.end (); ++i, ++m) {
		(*m)->update (accurate_coefficient_to_dB (i->second.level), accurate_coefficient_to_dB (i->second.peak));
	}
}

/* ****************************************************************************/
#define PX_SCALE(px) std::max ((float)px, rintf ((float)px* UIConfiguration::instance ().get_ui_scale ()))

RecorderUI::InputMeter::InputMeter (std::string const& name)
{
	_meter = new FastMeter (
	    (uint32_t)floor (UIConfiguration::instance ().get_meter_hold ()),
	    18, FastMeter::Vertical, PX_SCALE (240),
	    UIConfiguration::instance ().color ("meter color0"),
	    UIConfiguration::instance ().color ("meter color1"),
	    UIConfiguration::instance ().color ("meter color2"),
	    UIConfiguration::instance ().color ("meter color3"),
	    UIConfiguration::instance ().color ("meter color4"),
	    UIConfiguration::instance ().color ("meter color5"),
	    UIConfiguration::instance ().color ("meter color6"),
	    UIConfiguration::instance ().color ("meter color7"),
	    UIConfiguration::instance ().color ("meter color8"),
	    UIConfiguration::instance ().color ("meter color9"),
	    UIConfiguration::instance ().color ("meter background bottom"),
	    UIConfiguration::instance ().color ("meter background top"),
	    0x991122ff, // red highlight gradient Bot
	    0x551111ff, // red highlight gradient Top
	    (115.0 * log_meter0dB (-18)),
	    89.125,  // 115.0 * log_meter0dB(-9);
	    106.375, // 115.0 * log_meter0dB(-3);
	    115.0,   // 115.0 * log_meter0dB(0);
	    (UIConfiguration::instance ().get_meter_style_led () ? 3 : 1));

	_name_label.set_corner_radius (2);
	_name_label.set_elements ((ArdourButton::Element) (ArdourButton::Edge | ArdourButton::Body | ArdourButton::Text | ArdourButton::Inactive));
	_name_label.set_name ("meterbridge label");
	_name_label.set_angle (-90.0);
	_name_label.set_text_ellipsize (Pango::ELLIPSIZE_MIDDLE);
	_name_label.set_layout_ellipsize_width (48 * PANGO_SCALE);
	_name_label.set_alignment (-1.0, .5);

	int nh = 88 * UIConfiguration::instance ().get_ui_scale ();
	_name_label.set_size_request (PX_SCALE (18), nh);
	_name_label.set_layout_ellipsize_width (nh * PANGO_SCALE);

	_name_label.set_text (name);
	set_tooltip (_name_label, Gtkmm2ext::markup_escape_text (name));

	pack_start (*_meter, false, false);
	pack_start (_name_label, false, false);

	_meter->show ();
	_name_label.show ();
}

RecorderUI::InputMeter::~InputMeter ()
{
	delete _meter;
}

void
RecorderUI::InputMeter::update (float l, float p)
{
	_meter->set (log_meter0dB (l), log_meter0dB (p));
}
