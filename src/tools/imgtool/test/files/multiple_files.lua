local image = imgtool.image:open("coco_jvc_rsdos", "multiple_files.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent;

dirent = directory();
assert(dirent.filename == "HELLO.BAS", "Expected HELLO.BAS but instead got '" .. dirent.filename .. "'");
assert(dirent.attributes == "0 B");
assert(dirent.filesize == 33);

dirent = directory();
assert(dirent.filename == "HELLO.TXT", "Expected HELLO.TXT but instead got '" .. dirent.filename .. "'");
assert(dirent.attributes == "1 A");
assert(dirent.filesize == 12);

dirent = directory();
assert(dirent.filename == "ML.BIN", "Expected ML.BIN but instead got '" .. dirent.filename .. "'");
assert(dirent.attributes == "2 B");
assert(dirent.filesize == 42);
