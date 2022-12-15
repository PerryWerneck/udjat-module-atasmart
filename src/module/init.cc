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
 #include <udjat/moduleinfo.h>
 #include <udjat/factory.h>
 #include <udjat/tools/disk/stat.h>
 #include <unistd.h>
 #include <fstream>
 #include "private.h"

 using namespace std;

 static const Udjat::ModuleInfo moduleinfo{"ATA S.M.A.R.T. Disk Health Monitor"};

 class Module : public Udjat::Module, Udjat::Factory {
 public:

 	Module() : Udjat::Module("atasmart",moduleinfo), Udjat::Factory("atasmart",moduleinfo) {
 	};

 	virtual ~Module() {
 	}

	std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const override {

		const char * devname = node.attribute("device-name").as_string();

		if(*devname) {

			// Has device name, create a device node.
			return  make_shared<Smart::Agent>(devname,node);

		}

		// No device name, create a container with all physical disks.

		/// @brief Container with detected physical disks.
		class PhysicalDisks : public Abstract::Agent {
		public:
			PhysicalDisks(const pugi::xml_node &node) : Abstract::Agent("storage") {

				Object::properties.icon = "drive-multidisk";
				Object::properties.label = "Physical disks";

				load(node);

				// Load disks
				for(Disk::Stat &disk : Disk::Stat::get()) {

					if(disk.minor == 0 && !disk.name.empty()) {
						std::shared_ptr<Udjat::Abstract::Agent> agent = make_shared<Smart::Agent>((string{"/dev/"} + disk.name).c_str(),node);
						Udjat::Abstract::Agent::push_back(agent);
					}

				}

			}

			virtual ~PhysicalDisks() {
			}

			/// @brief Export device info.
			void get(const Udjat::Request &request, Udjat::Response &response) override {

				Abstract::Agent::get(request,response);

				Udjat::Value &devices = response["devices"];

				for(auto child : *this) {

					auto agent = dynamic_cast<Smart::Agent *>(child.get());
					if(!agent)
						continue;

					// It's an smart agent ...

					// ... Refresh agent data ...
					agent->Abstract::Agent::refresh(true);

					// ... and export it.
					Udjat::Value &device = devices.append();

					device["name"] = agent->name();
					device["device"] = agent->getDeviceName();
					device["summary"] = agent->summary();
					device["state"] = agent->state()->summary();

				}

			}

		};

		return make_shared<PhysicalDisks>(node);

	}

 };

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {
	return new ::Module();
 }

