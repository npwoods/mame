local image = imgtool.image:open("apple35_dc_mac_hfs", "apple35_dc_mac_hfs.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent;

dirent = directory();
assert(dirent.filename == "Desktop", "Expected 'Desktop' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 1536);
assert(dirent.directory == false);

dirent = directory();
assert(dirent.filename == "H\xC3\xA9ll\xC3\xB5", "Expected 'Héllõ' but instead got '" .. dirent.filename .. "'");
assert(dirent.directory == true);
local hello_directory_filename = dirent.filename;

dirent = directory();
assert(dirent.filename == "Other", "Expected 'Other' but instead got '" .. dirent.filename .. "'");
assert(dirent.directory == true);
local other_directory_filename = dirent.filename;

dirent = directory();
assert(dirent.filename == "W\xC3\xB4rld", "Expected 'Wôrld' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 1024);
assert(dirent.directory == false);

local hello_child_directory = partition:opendir(hello_directory_filename);
dirent = hello_child_directory();
assert(dirent.filename == "Inside Child Directory", "Expected 'Inside Child Directory' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 1024);
assert(dirent.directory == false);

local other_child_directory = partition:opendir(other_directory_filename);
dirent = other_child_directory();
assert(dirent.filename == "Hearts", "Expected 'Hearts' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 55296, "Expected 55296 but instead got " .. dirent.filesize);
assert(dirent.directory == false);

dirent = other_child_directory();
assert(dirent.filename == "StuntCopter", "Expected 'StuntCopter' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 23552, "Expected 23552 but instead got " .. dirent.filesize);
assert(dirent.directory == false);

local hearts_forks = partition:list_file_forks("Other:Hearts")
assert(#hearts_forks == 2)
assert(hearts_forks[1].name == "")
assert(hearts_forks[1].size == 0, "Expected 0 but instead got " .. hearts_forks[1].size)
assert(hearts_forks[2].name == "RESOURCE_FORK")
assert(hearts_forks[2].size ==  55174, "Expected 55174 but instead got " .. hearts_forks[2].size)

local stuntcopter_forks = partition:list_file_forks("Other:Stuntcopter")
assert(#stuntcopter_forks == 2)
assert(stuntcopter_forks[1].name == "")
assert(stuntcopter_forks[1].size == 0, "Expected 0 but instead got " .. stuntcopter_forks[1].size)
assert(stuntcopter_forks[2].name == "RESOURCE_FORK")
assert(stuntcopter_forks[2].size ==  23177, "Expected 23177 but instead got " .. stuntcopter_forks[2].size)

local world_forks = partition:list_file_forks("W\xC3\xB4rld")
assert(#world_forks == 2)
assert(world_forks[1].name == "")
assert(world_forks[1].size == 5, "Expected 5 but instead got " .. world_forks[1].size)
assert(world_forks[2].name == "RESOURCE_FORK")
assert(world_forks[2].size ==  332, "Expected 332 but instead got " .. world_forks[2].size)
