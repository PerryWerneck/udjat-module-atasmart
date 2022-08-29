/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 /**
  * @brief Implements SMART device object.
  *
  * <http://0pointer.de/blog/projects/being-smart.html>
  * <http://git.0pointer.net/libatasmart.git/tree/skdump.c>
  * <http://git.0pointer.net/libatasmart.git/tree/atasmart.c>
  *
  * @author perry.werneck@gmail.com
  *
  */

 #include "private.h"
 #include <udjat/tools/quark.h>
 #include <udjat/smart/disk.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <udjat/request.h>
 #include <udjat/tools/disk/stat.h>
 #include <sys/time.h>

 using Udjat::Quark;

 namespace Udjat {

	static const char * getAgentName(const char * devname) {

		if(devname && *devname) {

			const char * ptr = strrchr(devname,'/');
			if(ptr && (ptr+1))
				return Quark(ptr+1).c_str();
			return "disk";

		}

		throw runtime_error("Missing required attribute 'device-name'");

	}

	Smart::Agent::Agent(const char *n) : Udjat::Agent<unsigned short>(getAgentName(n), SK_SMART_OVERALL_GOOD), devicename(Quark(n).c_str()) {
		init();
		setDefaultStates();
	}

	Smart::Agent::Agent(const char *n, const pugi::xml_node &node) : Udjat::Agent<unsigned short>(getAgentName(n), SK_SMART_OVERALL_GOOD), devicename(Quark(n).c_str()) {

		init();
		load(node);

		if(!states.empty()) {
			setDefaultStates();
		}

		if(Attribute(node,"diskstats",true).as_bool(false)) {

			unit = Udjat::Disk::Unit::get(node);
			Udjat::Disk::Stat(devicename).reset(stats);

		}

	}

	Smart::Agent::Agent(const pugi::xml_node &node)
		: Agent(node.attribute("device-name").as_string(),node) {
	}

	void Smart::Agent::init() {

		Object::properties.icon = "drive-harddisk";

		if(!(Object::properties.label && *Object::properties.label)) {
			string label{"Hard disk "};

			const char * ptr = strrchr(this->devicename,'/');
			if(ptr) {
				label += (ptr+1);
			} else {
				label += devicename;
			}

			Object::properties.label = Quark(label).c_str();
		}

		// Get data from disk.

		try {

			Smart::Disk disk(devicename);

			auto ipd = disk.read().identify();

			string summary{ipd->model};

			try {

				string sz = disk.formattedSize();

				summary += " (" + sz + ")";

			} catch(const std::exception &e) {

				error() << e.what();

			}

			Object::properties.summary = Quark(summary).c_str();

		} catch(const std::exception &e) {

			error() << Logger::Message("Error '{}' getting device information",e) << endl;

		}

	}

	/// @brief Get device status, update internal state.
	bool Smart::Agent::refresh() {

		try {

			set(Smart::Disk(devicename).read().getOverral());


		} catch(const std::exception &e) {

			failed( (string{"Can't get overall state of "} + devicename).c_str(), e);

		}

		if(unit) {
			Udjat::Disk::Stat(devicename).compute(stats);

#ifdef DEBUG
			info() << "Read=" << (stats.read / unit->value) << " Write=" << (stats.write / unit->value) << endl;
#endif // DEBUG
		}

		return true;

	}

	/// @brief Export device info.
	void Smart::Agent::get(const Udjat::Request &request, Udjat::Response &response) {

		Udjat::Abstract::Agent::get(request,response);

		try {

			Smart::Disk disk(devicename);
			disk.read();

			response["temperature"] = disk.temperature().to_string().c_str();
			response["size"] = disk.formattedSize();

			if(disk.identify_is_available()) {
				auto ipd = disk.read().identify();
				response["serial"] = ipd->serial;
				response["firmware"] = ipd->firmware;
				response["model"] = ipd->model;
			} else {
				response["serial"] = "";
				response["firmware"] = "";
				response["model"] = "";
			}

			response["badsectors"] = disk.badsectors();
			// response["poweron"] = disk.poweron();
			response["powercicle"] = disk.powercicle();

			if(unit) {
				response["read"] = stats.read / unit->value;
				response["write"] = stats.write / unit->value;
			}

		} catch(const exception &e) {

			error() << "Error '" << e.what() << "' getting device info" << endl;

		}

	}

	Smart::Agent::~Agent() {
	}

	void Smart::Agent::setDefaultStates() {

		static const struct {
			unsigned int					  value;		///< @brief Agent value for the state.
			const char 						* name;			///< @brief State name.
			Udjat::Level					  level;		///< @brief State level.
			const char						* summary;		///< @brief State summary.
			const char						* body;			///< @brief State description
		} states[] = {

			{
				SK_SMART_OVERALL_GOOD,
				"good",
				Udjat::ready,
				N_( "${name} Health is Good" ),
				""
			},
			{
				SK_SMART_OVERALL_BAD_ATTRIBUTE_IN_THE_PAST,
				"badonthepast",
				Udjat::ready,
				N_( "Pre fail in the past on ${name}" ),
				N_( "At least one pre-fail attribute exceeded its threshold in the past on ${name}" )
			},
			{
				SK_SMART_OVERALL_BAD_SECTOR,
				"badsector",
				Udjat::warning,
				N_( "Bad sector on ${name}" ),
				N_( "At least one bad sector on ${name}" )
			},
			{
				SK_SMART_OVERALL_BAD_ATTRIBUTE_NOW,
				"badattribute",
				Udjat::error,
				N_( "Pre fail exceeded on ${name}" ),
				N_( "At least one pre-fail attribute is exceeding its threshold now on ${name}" )
			},
			{
				SK_SMART_OVERALL_BAD_SECTOR_MANY,
				"manybad",
				Udjat::error,
				N_( "Too many bad sectors on ${name}" ),
				""
			},
			{
				SK_SMART_OVERALL_BAD_STATUS,
				"badstatus",
				Udjat::error,
				N_( "Smart Self Assessment negative on ${name}" ),
				""
			},

		};

		info() << "Using default states" << endl;

		for(size_t ix = 0; ix < (sizeof(states)/ sizeof(states[0])); ix++) {

			this->states.push_back(
				make_shared<Udjat::State<unsigned short>>(
					states[ix].name,
					states[ix].value,
					states[ix].level,
#ifdef GETTEXT_PACKAGE
					Quark(expand(dgettext(GETTEXT_PACKAGE,states[ix].summary))).c_str(),
					Quark(expand(dgettext(GETTEXT_PACKAGE,states[ix].body))).c_str()
#else
					Quark(expand(states[ix].summary)).c_str(),
					Quark(expand(states[ix].body)).c_str()
#endif // GETTEXT_PACKAGE
				)
			);

		}

	}

 }

