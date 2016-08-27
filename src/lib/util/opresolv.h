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
	{ util::option_guide::entry::option_type::ENUM_BEGIN, (option_char), (identifier), (display_name) },
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

	template<typename T>
	struct range
	{
		range() { min = -1; max = -1; }
		T min, max;
	};

	// class to represent specific entry
	class entry
	{
		friend class option_resolution;

	public:
		typedef std::vector<range<int> > rangelist;

		// ctor
		entry(const option_guide::entry &guide_entry);

		// returns true if this entry is subject to range constraints
		bool is_ranged() const { return !m_ranges.empty(); }

		// accessors
		bool is_pertinent() const { return m_is_pertinent; }
		const std::string &value() const;
		int value_int() const;
		const std::string &default_value() const { return m_default_value; }
		const rangelist &ranges() const { return m_ranges; }
		const option_guide::entrylist::const_iterator enum_value_begin() const { return m_enum_value_begin; }
		const option_guide::entrylist::const_iterator enum_value_end() const { return m_enum_value_end; }

		// accessors for guide data
		option_guide::entry::option_type option_type() const { return m_guide_entry.type(); }
		const char *display_name() const { return m_guide_entry.display_name(); }
		const char *identifier() const { return m_guide_entry.identifier(); }
		int parameter() const { return m_guide_entry.parameter(); }


		// mutators
		bool set_value(const std::string &value);
		bool set_value(int value) { return set_value(numeric_value(value)); }

		// higher level operations
		bool can_bump_lower() const;
		bool can_bump_higher() const;
		bool bump_lower();
		bool bump_higher();

	private:
		// references to the option guide
		const option_guide::entry &				m_guide_entry;
		option_guide::entrylist::const_iterator	m_enum_value_begin;
		option_guide::entrylist::const_iterator	m_enum_value_end;

		// runtime state
		bool									m_is_pertinent;
		std::string								m_value;
		std::string								m_default_value;
		rangelist								m_ranges;

		// methods
		void parse_specification(const char *specification);
		void set_enum_value_range(option_guide::entrylist::const_iterator begin, option_guide::entrylist::const_iterator end);
		static std::string numeric_value(int value);
		rangelist::const_iterator find_in_ranges(int value) const;
	};

	// ctor/dtor
	option_resolution(const option_guide &guide);
	~option_resolution();

	// sets a specification
	void set_specification(const std::string &specification);

	// entry lookup - note that we're not exposing m_entries directly because we don't really
	// have a way of making the list be immutable but the members be mutable at the same time
	std::vector<entry>::iterator entries_begin() { return m_entries.begin(); }
	std::vector<entry>::iterator entries_end() { return m_entries.end(); }
	entry *find(int parameter);
	entry *find(const std::string &identifier);
	int lookup_int(int parameter);
	const std::string &lookup_string(int parameter);

	// misc
	static const char *error_string(error err);
	static error get_default(const char *specification, int option_char, int *val);

private:
	std::vector<entry> m_entries;

	error set_parameter(std::vector<entry>::iterator iter, const std::string &value);

	static const char *lookup_in_specification(const char *specification, const option_guide::entry &option);
	const entry *lookup_entry(int option_char) const;
};


} // namespace util

#endif /* __OPRESOLV_H__ */
