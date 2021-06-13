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

 #include <config.h>
 #include <udjat/module.h>
 #include <udjat/factory.h>
 #include <unistd.h>
 #include <fstream>
 #include "private.h"

 using namespace std;

 static const Udjat::ModuleInfo moduleinfo{
	PACKAGE_NAME,								// The module name.
	"ATA S.M.A.R.T. Disk Health Monitoring", 	// The module description.
	PACKAGE_VERSION, 							// The module version.
	PACKAGE_URL, 								// The package URL.
	PACKAGE_BUGREPORT 							// The bug report address.
 };

 class Module : public Udjat::Module, Udjat::Factory {
 public:

 	Module() : Udjat::Module("atasmart",&moduleinfo), Udjat::Factory("atasmart",&moduleinfo) {
 	};

 	virtual ~Module() {
 	}

	void parse(Udjat::Abstract::Agent &parent, const pugi::xml_node &node) const override {

		/// @brief Container with all disks
		class Container : public Abstract::Agent {
		public:
			Container(const pugi::xml_node &node) : Abstract::Agent("storage") {

				icon = "drive-multidisk";
				label = "Physical disks";
				load(node);

				// Load disks
				std::ifstream proc("/proc/partitions");

				string dunno;
				getline(proc,dunno);

				do {

					uint16_t major, minor;
					int blocks;
					string name;

					proc >> major >> minor >> blocks >> name;

					if(minor == 0 && !name.empty()) {
						this->insert(make_shared<Smart::Agent>((string{"/dev/"} + name).c_str(),node,false));
					}

				} while(!proc.eof());

			}

			virtual ~Container() {
			}

			/// @brief Export device info.
			void get(const Udjat::Request &request, Udjat::Response &response) override {

				Abstract::Agent::get(request,response);

				Udjat::Value &devices = response["devices"];

				for(auto child : *this) {

					auto agent = dynamic_cast<Smart::Agent *>(child.get());
					if(!agent)
						continue;

					// Refresh agent data (if necessary).
					agent->Abstract::Agent::refresh(true);

					// It's an smart agent, export it.
					Udjat::Value &device = devices.append();

					device["name"] = agent->getName();
					device["device"] = agent->getDeviceName();
					device["summary"] = agent->getSummary();
					device["state"] = agent->getState()->getSummary();

				}

			}

		};

		const char * devname = node.attribute("device-name").as_string();

		if(*devname) {

			// Has device name, create a device node.
			parent.insert(make_shared<Smart::Agent>(devname,node,true));

		} else {

			// No device name, create a container with all detected devices.
			parent.insert(make_shared<Container>(node));

		}

	}

 };

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {
	return new ::Module();
 }

