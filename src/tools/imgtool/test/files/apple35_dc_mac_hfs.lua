local image = imgtool.image:open("apple35_dc_mac_hfs", "apple35_dc_mac_hfs.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent;

dirent = directory();
assert(dirent.filename == "Desktop", "Expected 'Desktop' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 512);
assert(dirent.directory == false);

dirent = directory();
--assert(dirent.filename == "Héllõ", "Expected 'Héllõ' but instead got '" .. dirent.filename .. "'");
assert(dirent.directory == true);
local directory_filename = dirent.filename;

dirent = directory();
--assert(dirent.filename == "Wôrld", "Expected 'Wôrld' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 1024);
assert(dirent.directory == false);

local child_directory = partition:opendir(directory_filename);
dirent = child_directory();
assert(dirent.filename == "Inside Child Directory", "Expected 'Inside Child Directory' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 1024);
assert(dirent.directory == false);

