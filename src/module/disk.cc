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

 #include "private.h"
 #include <udjat/temperature.h>

 using namespace std;

 namespace Smart {

	 Disk::Disk(const char *name) : d(nullptr) {

		if(sk_disk_open(name, &d) < 0) {
			throw system_error(errno, system_category(), string{"Can't open "} + name);
		}

	 }

	 Disk::~Disk() {
		sk_disk_free(d);
	 }

	 Disk & Disk::read() {

		// TODO: Reading SMART data might cause the disk to wake up from sleep. Hence from monitoring daemons make sure to call sk_disk_check_power_mode() to check wether the disk is sleeping and skip the read if so

		if(sk_disk_smart_read_data(d) < 0) {
			throw system_error(errno, system_category(), "Can't read S.M.A.R.T. data");
		}
		return *this;
	 }

	 const SkIdentifyParsedData * Disk::identify() {
		const SkIdentifyParsedData *ipd;
		if(sk_disk_identify_parse(d, &ipd) < 0) {
			throw system_error(errno, system_category(), "Can't parse S.M.A.R.T. identify");
		}
		return ipd;
	 }

	 SkSmartOverall Disk::getOverral() {

		SkSmartOverall overall;

		if (sk_disk_smart_get_overall(d, &overall) < 0) {
			throw system_error(errno, system_category(), "Can't get S.M.A.R.T. overall state");
		}

		return overall;

	 }

	 uint64_t Disk::size() {

		uint64_t value;

		if(sk_disk_get_size(d,&value) < 0) {
			throw system_error(errno, system_category(), "Can't get S.M.A.R.T. disk size");
		}

		return value;

	 }

	uint64_t Disk::badsectors() {

		uint64_t value;

		if(sk_disk_smart_get_bad(d,&value) < 0) {
			throw system_error(errno, system_category(), "Can't get bad sectors");
		}

		return value;

	}

	uint64_t Disk::poweron() {

		uint64_t value;

		if(sk_disk_smart_get_power_on(d,&value) < 0) {
			throw system_error(errno, system_category(), "Can't get power on");
		}

		return value;

	}

	uint64_t Disk::powercicle() {

		uint64_t value;

		if(sk_disk_smart_get_power_cycle(d,&value) < 0) {
			throw system_error(errno, system_category(), "Can't get power cicle");
		}

		return value;

	}

	Temperature Disk::temperature() {

		uint64_t value;
		if(sk_disk_smart_get_temperature(d,&value) < 0) {
			throw system_error(errno, system_category(), "Can't get temperature");
		}

		return Temperature( ((float) value / 1000), Temperature::Kelvin);

	}

	string Disk::formattedSize() {

		static const struct {
			uint64_t value;
			const char *name;
		} values[] = {
			{ 1073741824LL,	"GB"	},
			{ 1048576LL,	"MB"	},
			{ 1024LL,		"KB"	}
		};

		uint64_t bytes = size();

		for(size_t ix = 0; ix < (sizeof(values)/sizeof(values[0])); ix++) {
			if(bytes > values[ix].value) {
				return std::to_string(bytes / values[ix].value) + " " + values[ix].name;
			}
		}

		return std::to_string(bytes);

	 }

 }

