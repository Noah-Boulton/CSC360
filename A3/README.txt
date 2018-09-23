CSC360 Assignment 3 - Noah Boulton V00803598

Compilation:
	To compile the programs diskinfo, disklist, diskget, and diskput use make command.

Running:
	To run the programs use:	./diskinfo disk_image.IMA
						./disklist disk_image.IMA
						./diskget disk_image.IMA filename
						./diskput disk_image.IMA filename

Usage:
	diskinfo:	Reads the disk image and outputs: Operating system, label, free size of disk, number of root files, number of FAT copies, and sectors per FAT.

	disklist:	Reads the disk image and outputs the files in the root directory listing: F/D, size of the file, filename, and creation date and time.
	
	diskget:	Reads the disk image and copies the file specified by the user from the root directory to the current directory in linux.

	diskput:	Reads the disk image and copies the file specified by the user from the current directory in linux to the root directory.

