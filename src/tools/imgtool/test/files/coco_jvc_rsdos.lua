local image = imgtool.image:open("coco_jvc_rsdos", "coco_jvc_rsdos.dsk");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent = directory();

assert(dirent.filename == "HELLO.TXT");
assert(dirent.filesize == 16);
