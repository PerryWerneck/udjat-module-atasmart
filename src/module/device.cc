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

 #include <device.h>
 #include <udjat/tools/quark.h>
 #include <udjat/state.h>
 #include <atasmart.h>

 using namespace Udjat;
 using namespace std;

 Device::Device(const char *n) : name(Quark(n).c_str()) {
 	setDefaultStates();
 }

 Device::Device(const char *n, const pugi::xml_node &node) : name(Quark(n).c_str()) {

#ifdef DEBUG
	cout << getName() << "\tCreating device " << n << endl;
#endif // DEBUG

 	Abstract::Agent::load(node);

 	if(!hasStates()) {
		setDefaultStates();
 	}

 }

 Device::Device(const pugi::xml_node &node) : Device(node.attribute("device-name").as_string(),node) {
 }

 /// @brief Get device status, update internal state.
 void Device::refresh() {

	SkDisk *d;

	if(sk_disk_open(name, &d) < 0) {

		failed(Quark(string{"Cant open "} + name).c_str(),errno);

	} else {

		SkSmartOverall overall;

		if(sk_disk_smart_read_data(d) < 0) {

			failed(Quark(string{"Can't read data from "} + name).c_str(),errno);

		} else if (sk_disk_smart_get_overall(d, &overall) < 0) {

			failed(Quark(string{"Can't get overall state of "} + name).c_str(),errno);

		} else {

			set(overall);

		}

		sk_disk_free(d);

	}

 }

 /// @brief Export device info.
 void Device::get(const Request &request, Response &response) {
 }

 Device::~Device() {
 }

 void Device::setDefaultStates() {

 	static const struct {
 		unsigned int			  value;		///< @brief Agent value for the state.
 		const char 				* name;			///< @brief State name.
 		Abstract::State::Level	  level;		///< @brief State level.
 		const char				* summary;		///< @brief State summary.
 		const char				* body;			///< @brief State description
 	} states[] = {

		{
 	        SK_SMART_OVERALL_GOOD,
			"good",
			Abstract::State::ready,
			"Disk is ok",
			""
		},
		{
			SK_SMART_OVERALL_BAD_ATTRIBUTE_IN_THE_PAST,
			"badonthepast",
			Abstract::State::ready,
			"Pre fail in the past",
			"At least one pre-fail attribute exceeded its threshold in the past"
		},
		{
			SK_SMART_OVERALL_BAD_SECTOR,
			"badsector",
			Abstract::State::warning,
			"Bad sector",
			"At least one bad sector"
		},
		{
			SK_SMART_OVERALL_BAD_ATTRIBUTE_NOW,
			"badattribute",
			Abstract::State::error,
			"Pre fail exceeded",
			"At least one pre-fail attribute is exceeding its threshold now"
		},
		{
			SK_SMART_OVERALL_BAD_SECTOR_MANY,
			"manybad",
			Abstract::State::error,
			"Many bad sectors",
			""
		},
		{
			SK_SMART_OVERALL_BAD_STATUS,
			"badstatus",
			Abstract::State::error,
			"Smart Self Assessment negative",
			""
		},

 	};

 	cout << this->getName() << "\tUsing default states" << endl;

 	for(size_t ix = 0; ix < (sizeof(states)/ sizeof(states[0])); ix++) {

		push_back(
			make_shared<State<unsigned int>>(
				states[ix].name,
				states[ix].value,
				states[ix].level,
				states[ix].summary,
				states[ix].body
			)
		);

 	}

 }


