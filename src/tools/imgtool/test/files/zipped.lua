local alpha_image = imgtool.image:open("coco_jvc_rsdos", "zipped.zip/alpha.dsk");
local alpha_partition = alpha_image:partitions()();
local alpha_directory = alpha_partition:opendir("");
local alpha_dirent = alpha_directory();

assert(alpha_dirent.filename == "ALPHA.TXT");
assert(alpha_dirent.filesize == 4);

local bravo_image = imgtool.image:open("coco_jvc_rsdos", "zipped.zip/bravo.dsk");
local bravo_partition = bravo_image:partitions()();
local bravo_directory = bravo_partition:opendir("");
local bravo_dirent = bravo_directory();

assert(bravo_dirent.filename == "BRAVO.TXT");
assert(bravo_dirent.filesize == 3);

local charlie_image = imgtool.image:open("coco_jvc_rsdos", "zipped.zip/charlie.dsk");
local charlie_partition = charlie_image:partitions()();
local charlie_directory = charlie_partition:opendir("");
local charlie_dirent = charlie_directory();

assert(charlie_dirent.filename == "CHARLIE.TXT");
assert(charlie_dirent.filesize == 4);

