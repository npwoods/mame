// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/****************************************************************************

    opresolv.cpp

    Extensible ranged option resolution handling

****************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pool.h"
#include "corestr.h"
#include "opresolv.h"


namespace util {

/***************************************************************************
	option_resolution
***************************************************************************/

// -------------------------------------------------
//	resolve_single_param
// -------------------------------------------------

option_resolution::error option_resolution::resolve_single_param(const char *specification, option_resolution::entry *param_value,
	struct range *range, size_t range_count)
{
	int FLAG_IN_RANGE           = 0x01;
	int FLAG_IN_DEFAULT         = 0x02;
	int FLAG_DEFAULT_SPECIFIED  = 0x04;
	int FLAG_HALF_RANGE         = 0x08;

	int last_value = 0;
	int value = 0;
	int flags = 0;
	const char *s = specification;

	while(*s && !isalpha(*s))
	{
		if (*s == '-')
		{
			/* range specifier */
			if (flags & (FLAG_IN_RANGE|FLAG_IN_DEFAULT))
			{
				return error::SYNTAX;
			}
			flags |= FLAG_IN_RANGE;
			s++;

			if (range)
			{
				range->max = -1;
				if ((flags & FLAG_HALF_RANGE) == 0)
				{
					range->min = -1;
					flags |= FLAG_HALF_RANGE;
				}
			}
		}
		else if (*s == '[')
		{
			/* begin default value */
			if (flags & (FLAG_IN_DEFAULT|FLAG_DEFAULT_SPECIFIED))
			{
				return error::SYNTAX;
			}
			flags |= FLAG_IN_DEFAULT;
			s++;
		}
		else if (*s == ']')
		{
			/* end default value */
			if ((flags & FLAG_IN_DEFAULT) == 0)
			{
				return error::SYNTAX;
			}
			flags &= ~FLAG_IN_DEFAULT;
			flags |= FLAG_DEFAULT_SPECIFIED;
			s++;

			if (param_value && param_value->int_value() == -1)
				param_value->set_int_value(value);
		}
		else if (*s == '/')
		{
			/* value separator */
			if (flags & (FLAG_IN_DEFAULT|FLAG_IN_RANGE))
			{
				return error::SYNTAX;
			}
			s++;

			/* if we are spitting out ranges, complete the range */
			if (range && (flags & FLAG_HALF_RANGE))
			{
				range++;
				flags &= ~FLAG_HALF_RANGE;
				if (--range_count == 0)
					range = nullptr;
			}
		}
		else if (*s == ';')
		{
			/* basic separator */
			s++;
		}
		else if (isdigit(*s))
		{
			/* numeric value */
			last_value = value;
			value = 0;
			do
			{
				value *= 10;
				value += *s - '0';
				s++;
			}
			while(isdigit(*s));

			if (range)
			{
				if ((flags & FLAG_HALF_RANGE) == 0)
				{
					range->min = value;
					flags |= FLAG_HALF_RANGE;
				}
				range->max = value;
			}

			// if we have a value; check to see if it is out of range
			if (param_value && (param_value->int_value() != -1) && (param_value->int_value() != value))
			{
				if ((last_value < param_value->int_value()) && (param_value->int_value() < value))
				{
					if ((flags & FLAG_IN_RANGE) == 0)
						return error::PARAMOUTOFRANGE;
				}
			}
			flags &= ~FLAG_IN_RANGE;
		}
		else
		{
			return error::SYNTAX;
		}
	}

	// we can't have zero length guidelines strings
	if (s == specification)
	{
		return error::SYNTAX;
	}

	return error::SUCCESS;
}


// -------------------------------------------------
//	lookup_in_specification
// -------------------------------------------------

const char *option_resolution::lookup_in_specification(const char *specification, const option_guide::entry &option)
{
	const char *s;
	s = strchr(specification, option.parameter());
	return s ? s + 1 : nullptr;
}


// -------------------------------------------------
//	ctor
// -------------------------------------------------

option_resolution::option_resolution(const option_guide &guide, const char *specification)
{
	// first count the number of options specified in the guide 
	auto option_count = guide.entries().size();

	// set up the entries list
	m_specification = specification;
	m_entries.reserve(option_count);

	// initialize each of the entries 
	for (auto index = 0; index < option_count; index++)
	{
		auto spec = lookup_in_specification(m_specification, guide.entries()[index]);
		if (spec != nullptr)
		{
			// create the entry
			entry entry(guide.entries()[index]);

			// if this is an enumeration, identify the values
			if (guide.entries()[index].type() == option_guide::entry::option_type::ENUM_BEGIN)
			{
				// enum values begin after the ENUM_BEGIN
				auto enum_value_begin = guide.entries().begin() + index + 1;
				auto enum_value_end = enum_value_begin;

				// and identify all entries of type ENUM_VALUE
				while (enum_value_end != guide.entries().end() && enum_value_end->type() == option_guide::entry::option_type::ENUM_VALUE)
				{
					index++;
					enum_value_end++;
				}

				// set the range
				entry.set_enum_value_range(enum_value_begin, enum_value_end);
			}

			// set default values for ints and enums
			if (guide.entries()[index].is_ranged())
			{
				entry.set_int_value(-1);
				resolve_single_param(spec, &entry, nullptr, 0);

				auto ranges = list_ranges(m_specification, guide.entries()[index].parameter());
				entry.set_ranges(std::move(ranges));
			}

			// and append it
			m_entries.push_back(std::move(entry));
		}
	}
}


// -------------------------------------------------
//	dtor
// -------------------------------------------------

option_resolution::~option_resolution()
{
}


// -------------------------------------------------
//	set_parameter
// -------------------------------------------------

option_resolution::error option_resolution::set_parameter(const char *param, const std::string &value)
{
	// find the appropriate entry
	auto iter = std::find_if(
		m_entries.begin(),
		m_entries.end(),
		[&](const option_resolution::entry &e) { return !strcmp(param, e.guide_entry().identifier()); });
	
	// fail if the parameter is unknown
	if (iter == m_entries.end())
		return error::PARAMNOTFOUND;

	option_resolution::entry &entry = *iter;
	option_guide::entrylist::const_iterator enum_value_iter;
	bool must_resolve = false;
	switch (entry.guide_entry().type())
	{
		case option_guide::entry::option_type::INT:
			entry.set_int_value(atoi(value.c_str()));
			must_resolve = true;
			break;

		case option_guide::entry::option_type::STRING:
			entry.set_string_value(value);
			break;

		case option_guide::entry::option_type::ENUM_BEGIN:
			// look up the enum value
			enum_value_iter = std::find_if(
				entry.enum_value_begin(),
				entry.enum_value_end(),
				[&](const option_guide::entry &e) { return !core_stricmp(value.c_str(), e.identifier()); });

			// fail if not found
			if (enum_value_iter == entry.enum_value_end())
				return error::BADPARAM;

			// set the value
			entry.set_int_value(enum_value_iter->parameter());
			must_resolve = true;
			break;

		default:
			assert(false);
			break;
	}

	// do a resolution step if necessary
	if (must_resolve)
	{
		auto option_specification = lookup_in_specification(m_specification, entry.guide_entry());
		auto err = resolve_single_param(option_specification, &entry, nullptr, 0);
		if (err != error::SUCCESS)
			return err;

		// did we not get a real value?
		if (entry.int_value() < 0)
			return error::PARAMNOTSPECIFIED;
	}

	return error::SUCCESS;
}


// -------------------------------------------------
//	lookup_entry
// -------------------------------------------------

const option_resolution::entry *option_resolution::lookup_entry(int option_char) const
{
	for (auto &entry : m_entries)
	{
		switch(entry.guide_entry().type())
		{
			case option_guide::entry::option_type::INT:
			case option_guide::entry::option_type::STRING:
			case option_guide::entry::option_type::ENUM_BEGIN:
				if (entry.guide_entry().parameter() == option_char)
					return &entry;
				break;

			default:
				assert(false);
				return nullptr;
		}
	}
	return nullptr;
}


// -------------------------------------------------
//	has_option
// -------------------------------------------------

bool option_resolution::has_option(int option_char) const
{
	return lookup_entry(option_char) != nullptr;
}


// -------------------------------------------------
//	lookup_int
// -------------------------------------------------

int option_resolution::lookup_int(int option_char) const
{
	auto entry = lookup_entry(option_char);
	assert(entry != nullptr);
	return entry->int_value();
}


// -------------------------------------------------
//	lookup_string
// -------------------------------------------------

const std::string &option_resolution::lookup_string(int option_char) const
{
	auto entry = lookup_entry(option_char);
	assert(entry != nullptr);
	return entry->string_value();
}


// -------------------------------------------------
//	find_option
// -------------------------------------------------

const option_guide::entry *option_resolution::find_option(int option_char) const
{
	auto entry = lookup_entry(option_char);
	return entry ? &entry->guide_entry() : nullptr;
}


// -------------------------------------------------
//	index_option
// -------------------------------------------------

const option_guide::entry *option_resolution::index_option(int indx) const
{
	if ((indx < 0) || (indx >= m_entries.size()))
		return nullptr;
	return &m_entries[indx].guide_entry();
}


// -------------------------------------------------
//	lookup_ranges
// -------------------------------------------------

const option_resolution::ranges<int> &option_resolution::lookup_ranges(int option_char) const
{
	auto entry = lookup_entry(option_char);
	assert(entry != nullptr);
	return entry->get_ranges();
}


// -------------------------------------------------
//	list_ranges
// -------------------------------------------------

std::vector<option_resolution::range> option_resolution::list_ranges(int option_char) const
{
	return list_ranges(m_specification, option_char);
}


// -------------------------------------------------
//	list_ranges
// -------------------------------------------------

std::vector<option_resolution::range> option_resolution::list_ranges(const char *specification, int option_char)
{
	specification = strchr(specification, option_char);
	assert(specification != nullptr);

	// TODO - resolve_single_param should really be changed here
	std::vector<range> result(100);

	auto err = resolve_single_param(specification + 1, nullptr, &result[0], result.size());
	(void)err;
	assert(err == error::SUCCESS);

	int count = 0;
	while (count < result.size() && (result[count].min != 0 || result[count].max != 0))
		count++;

	result.resize(count);
	return result;
}


// -------------------------------------------------
//	get_default
// -------------------------------------------------

option_resolution::error option_resolution::get_default(const char *specification, int option_char, int *val)
{
	assert(val);

	// clear out default
	*val = -1;

	specification = strchr(specification, option_char);
	if (!specification)
	{
		return error::SYNTAX;
	}

	option_guide::entry dummy(option_guide::entry::option_type::INT);
	entry ent(dummy);
	auto err = resolve_single_param(specification + 1, &ent, nullptr, 0);
	*val = ent.int_value();
	return err;
}


// -------------------------------------------------
//	list_ranges
// -------------------------------------------------

option_resolution::error option_resolution::is_valid_value(const char *specification, int option_char, int val)
{
	auto ranges = list_ranges(specification, option_char);
	for (auto &r : ranges)
	{
		if ((r.min <= val) && (r.max >= val))
			return error::SUCCESS;
	}
	return error::PARAMOUTOFRANGE;
}


// -------------------------------------------------
//	contains
// -------------------------------------------------

bool option_resolution::contains(const char *specification, int option_char)
{
	return strchr(specification, option_char) != nullptr;
}


// -------------------------------------------------
//	error_string
// -------------------------------------------------

const char *option_resolution::error_string(option_resolution::error err)
{
	switch (err)
	{
	case error::SUCCESS:				return "The operation completed successfully";
	case error::OUTOFMEMORY:			return "Out of memory";
	case error::PARAMOUTOFRANGE:		return "Parameter out of range";
	case error::PARAMNOTSPECIFIED:		return "Parameter not specified";
	case error::PARAMNOTFOUND:			return "Unknown parameter";
	case error::PARAMALREADYSPECIFIED:	return "Parameter specified multiple times";
	case error::BADPARAM:				return "Invalid parameter";
	case error::SYNTAX:					return "Syntax error";
	case error::INTERNAL:				return "Internal error";
	}
	return nullptr;
}


// -------------------------------------------------
//	entry::ctor
// -------------------------------------------------

option_resolution::entry::entry(const option_guide::entry &guide_entry)
	: m_guide_entry(guide_entry)
{
}


// -------------------------------------------------
//	entry::set_enum_value_range
// -------------------------------------------------

void option_resolution::entry::set_enum_value_range(option_guide::entrylist::const_iterator begin, option_guide::entrylist::const_iterator end)
{
	m_enum_value_begin = begin;
	m_enum_value_end = end;
}


// -------------------------------------------------
//	entry::set_ranges
// -------------------------------------------------

void option_resolution::entry::set_ranges(std::vector<range> &&range_vec)
{
	m_ranges = std::make_unique<ranges<int>>(std::move(range_vec));
}


// -------------------------------------------------
//	entry::get_ranges
// -------------------------------------------------

const option_resolution::ranges<int> &option_resolution::entry::get_ranges() const
{
	assert(m_ranges.get() != nullptr);
	return *m_ranges.get();
}


// -------------------------------------------------
//	entry::enum_value_begin
// -------------------------------------------------

option_guide::entrylist::const_iterator	option_resolution::entry::enum_value_begin() const
{
	assert(guide_entry().type() == option_guide::entry::option_type::ENUM_BEGIN);
	return m_enum_value_begin;
}


// -------------------------------------------------
//	entry::enum_value_end
// -------------------------------------------------

option_guide::entrylist::const_iterator	option_resolution::entry::enum_value_end() const
{
	assert(guide_entry().type() == option_guide::entry::option_type::ENUM_BEGIN);
	return m_enum_value_end;
}


// -------------------------------------------------
//	entry::int_value
// -------------------------------------------------

int option_resolution::entry::int_value() const
{
	return atoi(m_value.c_str());
}


// -------------------------------------------------
//	entry::set_int_value
// -------------------------------------------------

void option_resolution::entry::set_int_value(int i)
{
	m_value = string_format("%d", i);
}


// -------------------------------------------------
//	entry::string_value
// -------------------------------------------------

const std::string &option_resolution::entry::string_value() const
{
	return m_value;
}


// -------------------------------------------------
//	entry::set_string_value
// -------------------------------------------------

void option_resolution::entry::set_string_value(const std::string &value)
{
	m_value = value;
}


} // namespace util