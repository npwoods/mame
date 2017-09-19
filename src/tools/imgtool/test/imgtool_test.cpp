#include <lua.hpp>
#include <iostream>
#include <memory>
#include "sol2/sol.hpp"

#include "imgtool.h"

// TODO - move Win32 stuff into OSD code
#ifdef WIN32
#include <windows.h>
#include "strconv.h"
#endif

//-------------------------------------------------
//  raise_error
//-------------------------------------------------

static void raise_error(imgtoolerr_t err)
{
	std::string error_string = imgtool_error(err);
	throw sol::error(std::move(error_string));
}

// ======================> l_directory

class l_directory
{
public:
	l_directory(sol::state &lua, imgtool::directory::ptr &&directory)
		: m_lua(lua)
		, m_directory(directory.release())
	{
	}

	sol::object iterate()
	{
		imgtool_dirent ent;
		imgtoolerr_t err = m_directory->get_next(ent);
		if (err)
			raise_error(err);

		sol::object result;
		if (!ent.eof)
		{
			sol::table table = m_lua.create_table();
			table["filename"] = std::string(ent.filename);
			table["attributes"] = std::string(ent.attr);
			table["filesize"] = ent.filesize;
			table["creation_time"] = ent.creation_time;
			table["lastmodified_time"] = ent.lastmodified_time;
			table["lastaccess_time"] = ent.lastaccess_time;
			table["softlink"] = std::string(ent.softlink);
			table["comment"] = std::string(ent.comment);	
			table["corrupt"] = ent.corrupt ? true : false;
			table["directory"] = ent.directory ? true : false;
			table["hardlink"] = ent.hardlink ? true : false;
			result = sol::make_object(m_lua, table);
		}
		else
		{
			result = sol::make_object(m_lua, sol::nil);
		}
		return result;
	}

private:
	sol::state &m_lua;
	std::shared_ptr<imgtool::directory> m_directory;
};

// ======================> l_partition

class l_partition
{
public:
	l_partition(sol::state &lua, imgtool::partition::ptr &&partition)
		: m_lua(lua)
		, m_partition(partition.release())
	{
	}

	uint64_t freespace()
	{
		uint64_t result;
		imgtoolerr_t err = m_partition->get_free_space(result);
		if (err)
			raise_error(err);

		return result;
	}

	l_directory opendir(const std::string &path)
	{
		imgtool::directory::ptr directory;
		imgtoolerr_t err = imgtool::directory::open(*m_partition.get(), path, directory);
		if (err)
			raise_error(err);

		return l_directory(m_lua, std::move(directory));
	}

	sol::object list_file_forks(const std::string &path)
	{
		std::vector<imgtool::fork_entry> forks;
		imgtoolerr_t err = m_partition->list_file_forks(path.c_str(), forks);
		if (err)
			raise_error(err);

		sol::table table = m_lua.create_table();
		for (auto i = 0; i < forks.size(); i++)
		{
			sol::table subtable = m_lua.create_table();
			subtable["name"] = forks[i].name();
			subtable["size"] = forks[i].size();
			table.add(subtable);
		}

		return table;
	}

private:
	sol::state &m_lua;
	std::shared_ptr<imgtool::partition> m_partition;
};

// ======================> l_partition_iterator

class l_partition_iterator
{
public:
	l_partition_iterator(sol::state &lua, std::shared_ptr<imgtool::image> image)
		: m_lua(lua)
		, m_image(image)
		, m_i(0)
	{
		imgtoolerr_t err = m_image->list_partitions(m_partitions);
		if (err)
			raise_error(err);
	}

	sol::object iterate()
	{
		imgtoolerr_t err;

		sol::object result;
		if (m_i < m_partitions.size())
		{
			imgtool::partition::ptr partition;
			err = imgtool::partition::open(*m_image.get(), m_i++, partition);
			if (err)
				raise_error(err);

			result = sol::make_object(m_lua, l_partition(m_lua, std::move(partition)));
		}
		else
		{
			result = sol::make_object(m_lua, sol::nil);
		}
		return result;
	}

private:
	sol::state &m_lua;
	std::shared_ptr<imgtool::image> m_image;
	std::vector<imgtool::partition_info> m_partitions;
	int m_i;
};

// ======================> l_image

class l_image
{
public:
	static l_image open(sol::state &lua, const std::string &modulename, const std::string &filename)
	{
		imgtoolerr_t err;

		imgtool::image::ptr image;
		err = imgtool::image::open(modulename, filename, OSD_FOPEN_READ, image);
		if (err)
			raise_error(err);

		return l_image(lua, std::move(image));
	}

	l_partition_iterator partitions()
	{
		return l_partition_iterator(m_lua, m_image);
	}

private:
	sol::state &m_lua;
	std::shared_ptr<imgtool::image> m_image;

	l_image(sol::state &lua, std::unique_ptr<imgtool::image> &&image)
		: m_lua(lua)
		, m_image(image.release())
	{
	}
};


//-------------------------------------------------
//  get_current_working_directory
//-------------------------------------------------

static std::string get_current_working_directory()
{
#ifdef WIN32
	TCHAR working_directory[MAX_PATH];
	GetCurrentDirectory(ARRAY_LENGTH(working_directory), working_directory);
	return osd::text::from_tstring(working_directory);
#else
	throw false; // not yet implemented
#endif
}


//-------------------------------------------------
//  set_current_working_directory
//-------------------------------------------------

static void set_current_working_directory(const std::string &working_directory)
{
#ifdef WIN32
	auto t_working_directory = osd::text::to_tstring(working_directory);
	SetCurrentDirectory(t_working_directory.c_str());
#else
	throw false; // not yet implemented
#endif
}


//-------------------------------------------------
//  get_directory_name
//-------------------------------------------------

static std::string get_directory_name(const std::string &filename)
{
	std::string full_path;
	osd_file::error ferr = osd_get_full_path(full_path, filename);
	if (ferr != osd_file::error::NONE)
		throw ferr;

	// find the last path separator
	auto const path_separator_pos = std::find_if(full_path.rbegin(), full_path.rend(), [](char c) { return (c == '\\' || c == '/' || c == ':'); });

	return path_separator_pos != full_path.rend()
		? std::string(full_path.begin(), path_separator_pos.base())
		: full_path;
}


//-------------------------------------------------
//  main
//-------------------------------------------------

int main(int argc, char *argv[])
{
	std::vector<std::string> args = osd_get_command_line(argc, argv);
	if (args.size() <= 1)
	{
		std::cerr << args[0] << ": Missing operand" << std::endl;
		return -1;
	}

	imgtool_init(true, nullptr);

	// initialize LUA/sol
	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math);

	// create a namespace
	sol::table imgtool_ns = lua.create_named_table("imgtool");

	// directory type
	imgtool_ns.new_usertype<l_directory>("directory",
		sol::meta_function::call, &l_directory::iterate);

	// partition type
	imgtool_ns.new_usertype<l_partition>("partition",
		"freespace", &l_partition::freespace,
		"opendir", &l_partition::opendir,
		"list_file_forks", &l_partition::list_file_forks);

	// parition iterator type
	imgtool_ns.new_usertype<l_partition_iterator>("partition_iterator",
		sol::meta_function::call, &l_partition_iterator::iterate);

	// image type
	imgtool_ns.new_usertype<l_image>("image",
		"open", [&lua](const std::string &dummy, const std::string &modulename, const std::string &filename) { return l_image::open(lua, modulename, filename); },
		"partitions", &l_image::partitions);

	// run each script
	int error_count = 0;
	for (int i = 1; i < args.size(); i++)
	{
		std::string script_basename = core_filename_extract_base(argv[i]);
		std::string script_directory = get_directory_name(argv[i]);

		// show message
		std::cout << "Running " << script_basename << "...";

		// scripts can reference files relative to the script - set the current working directory
		std::string old_current_directory = get_current_working_directory();	
		set_current_working_directory(script_directory);

		bool success;
		try
		{
			lua.script_file(script_basename);
			success = true;
		}
		catch (const sol::error &err)
		{
			std::cout << std::endl;
			std::cerr << err.what() << std::endl;
			success = false;
			error_count++;
		}

		if (success)
			std::cout << " success!" << std::endl;

		// restore the old current working directory
		set_current_working_directory(script_directory);
	}
	return error_count;
}
