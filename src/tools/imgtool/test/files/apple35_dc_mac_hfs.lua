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
assert(dirent.filename == "Hearts", "Expected 'Inside Child Directory' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 55296, "Expected 55296 but instead got " .. dirent.filesize);
assert(dirent.directory == false);

dirent = other_child_directory();
assert(dirent.filename == "StuntCopter", "Expected 'StuntCopter' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 23552, "Expected 23552 but instead got " .. dirent.filesize);
assert(dirent.directory == false);
