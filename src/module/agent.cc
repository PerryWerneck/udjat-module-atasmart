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
 #include <udjat/state.h>
 #include <udjat/request.h>

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

	 Smart::Agent::Agent(const char *n) : Udjat::Agent<unsigned short>(getAgentName(n), SK_SMART_OVERALL_GOOD), name(Quark(n).c_str()) {
		init();
		setDefaultStates();
	 }

	 Smart::Agent::Agent(const char *n, const pugi::xml_node &node,bool name_from_xml) : Udjat::Agent<unsigned short>(getAgentName(n), SK_SMART_OVERALL_GOOD), name(Quark(n).c_str()) {
		init();
		load(node,name_from_xml);
		if(!hasStates()) {
			setDefaultStates();
		}
	 }

	 Smart::Agent::Agent(const pugi::xml_node &node)
		: Agent(node.attribute("device-name").as_string(),node,true) {
	 }

	 void Smart::Agent::init() {

		icon = "drive-harddisk";

		if(!(this->label && *this->label)) {
			string label{"Hard disk "};

			const char * ptr = strrchr(this->name,'/');
			if(ptr) {
				label += (ptr+1);
			} else {
				label += name;
			}

			this->label = Quark(label).c_str();
		}

		// Get data from disk.

		try {

			Smart::Disk disk(name);

			auto ipd = disk.read().identify();

			string summary{ipd->model};

			try {

				string sz = disk.formattedSize();

				summary += " (" + sz + ")";

			} catch(const std::exception &e) {

				cerr << name << "\t" << e.what();

			}

			this->summary = Quark(summary).c_str();

		} catch(const std::exception &e) {

			error("Error '{}' getting device information",e.what());

		}

	}

	/// @brief Get device status, update internal state.
	bool Smart::Agent::refresh() {

		try {

			set(Smart::Disk(name).read().getOverral());

		} catch(const std::exception &e) {

			failed( (string{"Can't get overall state of "} + name).c_str(), e);

		}

		return true;

	}

	/// @brief Export device info.
	void Smart::Agent::get(const Udjat::Request &request, Udjat::Response &response) {

		Udjat::Abstract::Agent::get(request,response);

		try {

			Smart::Disk disk(name);
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

		} catch(const exception &e) {

			error("Error '{}' getting device info",e.what());

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
				"${agent.name} Health is Good",
				""
			},
			{
				SK_SMART_OVERALL_BAD_ATTRIBUTE_IN_THE_PAST,
				"badonthepast",
				Udjat::ready,
				"Pre fail in the past on ${agent.name}",
				"At least one pre-fail attribute exceeded its threshold in the past on ${agent.name}"
			},
			{
				SK_SMART_OVERALL_BAD_SECTOR,
				"badsector",
				Udjat::warning,
				"Bad sector on ${name}",
				"At least one bad sector on ${agent.name}"
			},
			{
				SK_SMART_OVERALL_BAD_ATTRIBUTE_NOW,
				"badattribute",
				Udjat::error,
				"Pre fail exceeded on ${name}",
				"At least one pre-fail attribute is exceeding its threshold now on ${agent.name}"
			},
			{
				SK_SMART_OVERALL_BAD_SECTOR_MANY,
				"manybad",
				Udjat::error,
				"Many bad sectors on ${agent.name}",
				""
			},
			{
				SK_SMART_OVERALL_BAD_STATUS,
				"badstatus",
				Udjat::error,
				"Smart Self Assessment negative on ${agent.name}",
				""
			},

		};

		cout << this->getName() << "\tUsing default states" << endl;

		for(size_t ix = 0; ix < (sizeof(states)/ sizeof(states[0])); ix++) {

			string summary(states[ix].summary);
			string body(states[ix].body);

			expand(summary);
			expand(body);

			push_back(
				make_shared<Udjat::State<unsigned short>>(
					states[ix].name,
					states[ix].value,
					states[ix].level,
					Quark(summary).c_str(),
					Quark(body).c_str()
				)
			);

		}

	}

 }

