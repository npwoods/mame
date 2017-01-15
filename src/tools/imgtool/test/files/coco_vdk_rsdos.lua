local image = imgtool.image:open("coco_vdk_rsdos", "coco_vdk_rsdos.zip");
local partition = image:partitions()();
local directory = partition:opendir("");
local dirent = directory();

assert(dirent.filename == "HELLO.TXT");
assert(dirent.filesize == 16);
