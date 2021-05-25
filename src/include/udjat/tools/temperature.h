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

 #include <udjat/defs.h>
 #include <string>

 namespace Udjat {

	class UDJAT_API Temperature {
	public:

		/// @brief Temperature Unity
		enum Unity : char {
			Kelvin = 'K',
			Celsius = 'C',
			Fahrenheit = 'F'
		};

	private:
		Unity unity;
		float value;

	public:

		constexpr Temperature(const float v, const Unity u) : unity(u), value(v) { }

		Temperature & set(const enum Unity unity);

		std::string to_string() const;

		float as_celsius() const;
		float as_kelvin() const;
		float as_fahrenheit() const;

	};


 }

