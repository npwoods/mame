local image = imgtool.image:open("coco_dmk_rsdos", "coco_dmk_rsdos.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent = directory();

assert(dirent.filename == "HELLO.TXT");
assert(dirent.filesize == 16);
