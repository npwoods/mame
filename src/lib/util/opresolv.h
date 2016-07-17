// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/****************************************************************************

    opresolv.h

    Extensible ranged option resolution handling

    An extensible mechanism for handling options is a major need in MESS and
    Imgtool.  Unfortunately, since we are using straight C for everything, it
    can be hard and awkward to create non-arcane mechanisms for representing
    these options.

    In this system, we have the following concepts:
    1.  An "option specification"; a string that represents what options are
        available, their defaults, and their allowed ranges.  Here is an
        example:

        Examples:
            "H[1]-2;T[35]/40/80;S[18]"
                Allow 1-2 heads; 35, 40 or 80 tracks, and 18 sectors,
                defaulting to 1 heads and 35 tracks.

            "N'Simon''s desk'"
                Simon's desk (strings are not subject to range checking)

    2.  An "option guide"; a struct that provides information about what the
        various members of the option specification mean (i.e. - H=heads)

    3.  An "option resolution"; an object that represents a set of interpreted
        options.  At this stage, the option bid has been processed and it is
        guaranteed that all options reside in their expected ranges.

    An option_resolution object is created based on an option guide and an
    option specification.  It is then possible to specify individual parameters
    to the option_resolution object.  Argument checks occur at this time.  When
    one is all done, you can then query the object for any given value.

****************************************************************************/

#ifndef __OPRESOLV_H__
#define __OPRESOLV_H__

#include <stdlib.h>
#include <vector>
#include <string>
#include <algorithm>


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

#define OPTION_GUIDE_START(option_guide_)                                   \
	const util::option_guide option_guide_ =										\
	{
#define OPTION_GUIDE_END                                                    \
	};
#define OPTION_GUIDE_EXTERN(option_guide_)                                  \
	extern const util::option_guide option_guide_
#define OPTION_INT(option_char, identifier, display_name)                   \
	{ util::option_guide::entry::option_type::INT, (option_char), (identifier), (display_name) },
#define OPTION_STRING(option_char, identifier, display_name)                \
	{ util::option_guide::entry::option_type::STRING, (option_char), (identifier), (display_name) },
#define OPTION_ENUM_START(option_char, identifier, display_name)            \
	{ util::option_guide::entry::option_type::ENUM_START, (option_char), (identifier), (display_name) },
#define OPTION_ENUM(value, identifier, display_name)                        \
	{ util::option_guide::entry::option_type::ENUM_VALUE, (value), (identifier), (display_name) },
#define OPTION_ENUM_END

namespace util {

// ======================> option_guide

class option_guide
{
public:
	class entry
	{
	public:
		enum class option_type
		{
			INT,
			STRING,
			ENUM_BEGIN,
			ENUM_VALUE
		};

		constexpr entry(option_type type, int parameter = 0, const char *identifier = nullptr, const char *display_name = nullptr)
			: m_type(type), m_parameter(parameter), m_identifier(identifier), m_display_name(display_name)
		{
		}

		// accessors
		const option_type type() const { return m_type; }
		int parameter() const { return m_parameter; }
		const char *identifier() const { return m_identifier; }
		const char *display_name() const { return m_display_name; }
		bool is_ranged() const { return type() == option_type::INT || type() == option_type::ENUM_BEGIN; }
	
	private:
		option_type	m_type;
		int			m_parameter;
		const char *m_identifier;
		const char *m_display_name;
	};

	typedef std::vector<option_guide::entry> entrylist;

	option_guide(std::initializer_list<entry> entries)
		: m_entries(entries)
	{
	}

	// methods
	const entrylist &entries() const { return m_entries; }
	const entry *find_entry(int parameter) const;

private:
	entrylist m_entries;
};

// ======================> option_resolution

class option_resolution
{
public:
	// TODO - audit unused parameters
	enum class error
	{
		SUCCESS,
		OUTOFMEMORY,
		PARAMOUTOFRANGE,
		PARAMNOTSPECIFIED,
		PARAMNOTFOUND,
		PARAMALREADYSPECIFIED,
		BADPARAM,
		SYNTAX,
		INTERNAL
	};

	struct range
	{
		range() { min = 0; max = 0; }
		int min, max;
	};

	template<typename T>
	class ranges
	{
	public:
		ranges(std::vector<range> &&ranges)
			: m_ranges(ranges)
		{
		}

		T minimum() const { return m_ranges[0].min; }
		T maximum() const { return m_ranges[m_ranges.size() - 1].max; }

		bool bump_higher(T &value) const
		{
			bool success = false;

			auto iter = find(value);
			if (iter != m_ranges.cend())
			{
				if (value < iter->max)
				{
					value++;
					success = true;
				}
				else if (iter + 1 != m_ranges.cend())
				{
					value = (iter + 1)->min;
					success = true;
				}
			}
			return success;
		}

		bool bump_lower(T &value) const
		{
			bool success = false;

			auto iter = find(value);
			if (iter != m_ranges.cend())
			{
				if (value > iter->min)
				{
					value--;
					success = true;
				}
				else if (iter != m_ranges.cbegin())
				{
					value = (iter - 1)->max;
					success = true;
				}
			}
			return success;
		}

	private:
		typedef std::vector<range> rangelist;
		rangelist m_ranges;

		rangelist::const_iterator find(T value) const
		{
			return std::find_if(
				m_ranges.cbegin(),
				m_ranges.cend(),
				[&](const range &r) { return r.min <= value && value <= r.max; });
		}

	};

	option_resolution(const option_guide &guide, const char *specification);
	~option_resolution();

	// processing options with option_resolution objects
	error set_parameter(int parameter, const std::string &value);
	error set_parameter(const char *param, const std::string &value);
	bool has_option(int option_char) const;
	int lookup_int(int option_char) const;
	const std::string &lookup_string(int option_char) const;
	std::vector<range> list_ranges(int option_char) const;
	const option_resolution::ranges<int> &lookup_ranges(int option_char) const;

	// accessors
	const char *specification() const { return m_specification; }
	const option_guide::entry *find_option(int option_char) const;
	const option_guide::entry *index_option(int indx) const;

	// processing option specifications
	static std::vector<range> list_ranges(const char *specification, int option_char);
	static error get_default(const char *specification, int option_char, int *val);
	static error is_valid_value(const char *specification, int option_char, int val);
	static bool contains(const char *specification, int option_char);

	// misc
	static const char *error_string(error err);

private:
	class entry
	{
	public:
		entry(const option_guide::entry &guide_entry);

		void set_enum_value_range(option_guide::entrylist::const_iterator begin, option_guide::entrylist::const_iterator end);
		void set_ranges(std::vector<range> &&range_vec);
		option_guide::entrylist::const_iterator	enum_value_begin() const;
		option_guide::entrylist::const_iterator	enum_value_end() const;

		const option_guide::entry &guide_entry() const { return m_guide_entry; }

		int int_value() const;
		void set_int_value(int i);

		const std::string &string_value() const;
		void set_string_value(const std::string &value);

		const ranges<int> &get_ranges() const;

	private:
		const option_guide::entry &				m_guide_entry;
		option_guide::entrylist::const_iterator	m_enum_value_begin;
		option_guide::entrylist::const_iterator	m_enum_value_end;
		std::string								m_value;
		std::unique_ptr<ranges<int> >			m_ranges;
	};

	const char *m_specification;
	std::vector<entry> m_entries;

	error set_parameter(std::vector<entry>::iterator iter, const std::string &value);

	static error resolve_single_param(const char *specification, entry *param_value,
		struct range *range, size_t range_count);
	static const char *lookup_in_specification(const char *specification, const option_guide::entry &option);
	const entry *lookup_entry(int option_char) const;
};


} // namespace util

#endif /* __OPRESOLV_H__ */
