local image = imgtool.image:open("pc_dsk_fat", "pc_dsk_fat.zip");
local partition = image:partitions()();
local directory = partition:opendir("\\");
local dirent;

dirent = directory();
assert(dirent.filename == "child-directory")
assert(dirent.directory == true);
local directory_filename = dirent.filename;

dirent = directory();
assert(dirent.filename == "\xC3\x85ccent-M\xC3\xA4rks.txt", "Expected 'Åccent-Märks.txt' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 9);
assert(dirent.directory == false);

dirent = directory();
assert(dirent.filename == "foo.txt", "Expected 'foo.txt' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 16);
assert(dirent.directory == false);

dirent = directory();
assert(dirent.filename == "this-filename-is-long.txt", "Expected 'this-filename-is-long.txt' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 7);
assert(dirent.directory == false);

local child_directory = partition:opendir(directory_filename);
dirent = child_directory();
assert(dirent.filename == "bar.txt", "Expected 'bar.txt' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 28);
assert(dirent.directory == false);

dirent = child_directory();
assert(dirent.filename == "this-filename-is-also-long.txt", "Expected 'this-filename-is-also-long.txt' but instead got '" .. dirent.filename .. "'");
assert(dirent.filesize == 7);
assert(dirent.directory == false);

