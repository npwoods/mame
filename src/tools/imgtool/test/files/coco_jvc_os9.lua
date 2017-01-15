local image = imgtool.image:open("coco_jvc_os9", "coco_jvc_os9.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent;

dirent = directory();
assert(dirent.filename == "FOO.TXT", "Expected 'FOO.TXT' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 13);
assert(dirent.directory == false);

dirent = directory();
assert(dirent.filename == "MYDIR")
assert(dirent.directory == true);
local directory_filename = dirent.filename;

local child_directory = partition:opendir(directory_filename);
dirent = child_directory();
assert(dirent.filename == "BAR.TXT", "Expected 'BAR.TXT' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 19);
assert(dirent.directory == false);
