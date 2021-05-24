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

 #pragma once

 #include "private.h"
 #include <udjat/agent.h>
 #include <udjat/temperature.h>
 #include <string>
 #include <atasmart.h>

 using namespace Udjat;

 namespace Smart {

	/// @brief S.M.A.R.T. disk abstraction.
	class Disk {
	private:
		SkDisk *d;

	public:
		Disk(const char *name);
		~Disk();

		Disk & read();
		const SkIdentifyParsedData * identify();
		SkSmartOverall getOverral();

		uint64_t size();
		uint64_t badsectors();
		uint64_t poweron();
		uint64_t powercicle();

		Udjat::Temperature temperature();

		std::string formattedSize();

	};

	/// @brief S.M.A.R.T. agent.
	class Agent : public Udjat::Agent<unsigned short> {
	private:
		const char *name;

		/// @brief Load default states.
		void setDefaultStates();

		/// @brief Initialize
		void init();

	public:

		Agent(const char *name);
		Agent(const pugi::xml_node &node);
		Agent(const char *name, const pugi::xml_node &node);

		/// @brief Get device status, update internal state.
		void refresh() override;

		/// @brief Export device info.
		void get(const Udjat::Request &request, Udjat::Response &response) override;


		virtual ~Agent();

	};

 }

