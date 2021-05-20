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
 }

 Device::Device(const char *name, const pugi::xml_node &node) : Device(name) {

 	Abstract::Agent::load(node);

 }

 Device::Device(const pugi::xml_node &node) : Device(node.attribute("device-name").as_string(),node) {
 }

 /// @brief Get device status, update internal state.
 void Device::refresh() {

	SkDisk *d;

	if(sk_disk_open(name, &d) < 0) {

		activate(make_shared<Abstract::State>("error",Abstract::State::critical,"Cant open device"));

	} else {

		SkSmartOverall overall;

		if(sk_disk_smart_read_data(d) < 0) {

			activate(make_shared<Abstract::State>("error",Abstract::State::critical,"Can't read device data"));

		} else if (sk_disk_smart_get_overall(d, &overall) < 0) {

			activate(make_shared<Abstract::State>("error",Abstract::State::critical,"Can't get device overall state"));

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

