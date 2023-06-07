# AquaFS: RocksDB Storage Backend for ZNS SSDs and SMR HDDs

AquaFS is a file system plugin that utilizes [RocksDB](https://github.com/facebook/rocksdb)'s FileSystem interface to
place files into zones on a raw zoned block device. By separating files into
zones and utilizing write life time hints to co-locate data of similar life
times the system write amplification is greatly reduced compared to
conventional block devices. AquaFS ensures that there is no background
garbage collection in the file system or on the disk, improving performance
in terms of throughput, tail latencies and disk endurance.

## Community
For help or questions about aquafs usage (e.g. "how do I do X?") see below, on join us on [Matrix](https://app.element.io/#/room/#zonedstorage-general:matrix.org), or on [Slack](https://join.slack.com/t/zonedstorage/shared_invite/zt-uyfut5xe-nKajp9YRnEWqiD4X6RkTFw).

To report a bug, file a documentation issue, or submit a feature request, please open a GitHub issue.

For release announcements and other discussions, please subscribe to this repository or join us on Matrix or Slack.

## Dependencies

AquaFS depends on[ libzbd ](https://github.com/westerndigitalcorporation/libzbd)
and Linux kernel 5.4 or later to perform zone management operations. To use
AquaFS on SSDs with Zoned Namespaces, Linux kernel 5.9 or later is required.
AquaFS works with RocksDB version v6.19.3 or later.

# Getting started

## Build

Download, build and install libzbd. See the libzbd [ README ](https://github.com/westerndigitalcorporation/libzbd/blob/master/README.md) 
for instructions.

Download rocksdb and the aquafs projects:
```
$ git clone https://github.com/facebook/rocksdb.git
$ cd rocksdb
$ git clone https://github.com/westerndigitalcorporation/aquafs plugin/aquafs
```

Build and install rocksdb with aquafs enabled:
```
$ DEBUG_LEVEL=0 ROCKSDB_PLUGINS=aquafs make -j48 db_bench install
```

Build the aquafs utility:
```
$ pushd
$ cd plugin/aquafs/util
$ make
$ popd
```

### Standalone build

If you want to build AquaFS without building RocksDB, here are the steps:

Build rocksdb manually:

```shell
# we have some patches there
git clone https://github.com/RethinkFS/rocksdb -b aquafs
# cd to the rocksdb source directory
cd rocksdb
# build static library and install to system. DO NOT `make -j`, which will run out of your memory
sudo DEBUG_LEVEL=0 USE_RTTI=1 make -j14 install
```

Build standalone AquaFS (default in this repo):

```shell
cmake -B build -S .
cmake --build build
```

Will link RocksDB as a static library from system. `aquafs` and `libaaquafs.a` will be generated in `build/`.

NOTE: Cannot "mount" AquaFS without RocksDB now. To test AquaFS, `db_bench` is required but it's in RocksDB.

![structure](https://user-images.githubusercontent.com/81512075/208292592-21d26151-34a6-4847-8c02-e8bf0ae9955e.png)

## Configure the IO Scheduler for the zoned block device

The IO scheduler must be set to deadline to avoid writes from being reordered.
This must be done every time the zoned name space is enumerated (e.g at boot).

```
echo deadline > /sys/class/block/<zoned block device>/queue/scheduler
```

## Creating a AquaFS file system

Before AquaFS can be used in RocksDB, the file system metadata and superblock must be set up.
This is done with the aquafs utility, using the mkfs command. A AquaFS filesystem can be created
on either a raw zoned block device or on a zonefs filesystem on a zoned block device. For a raw
zoned block device, the device is specified using `--zbd=<zoned block device>`:

```
./plugin/aquafs/util/aquafs mkfs --zbd=<zoned block device> --aux_path=<path to store LOG and LOCK files>
```

If using zonefs, the zonefs file system mountpoint is specified instead using `--zonefs=<zonefs mountpoint>`:

```
./plugin/aquafs/util/aquafs mkfs --zonefs=<zonefs mountpoint> --aux_path=<path to store LOG and LOCK files>
```

In general, all operations of the aquafs utility can target either a raw block device or a zonefs mountpoint.

When using zonefs, the zonefs volumes should be mounted with the option "explicit-open":

```
sudo mount -o explicit-open <zoned block device> <zonefs mountpoint>
```

## AquaFS on-disk file formats

AquaFS Version 1.0.0 and earlier uses version 1 of the on-disk format.
AquaFS Version 2.0.0 introduces breaking on-disk-format changes (inline extents, support for zones larged than 4GB).

To migrate between different versions of the on-disk file format, use the aquafs backup/restore commands.

```
# Backup the disk contents to the host file system using the version of aquafs that was used to store the current database
./plugin/aquafs/util/aquafs backup --path=<path to store backup> --zbd=<zoned block device>

# Switch to the new version of AquaFS you want to use (e.g 1.0.2 -> 2.0.0), rebuild and create a new file system
# Remove the current aux folder if needed.
./plugin/aquafs/util/aquafs mkfs --force --zbd=<zoned block device> --aux_path=<path to store LOG and LOCK files>

# Restore the database files to the new version of the file system
./plugin/aquafs/util/aquafs restore --path=<path to backup> --zbd=<zoned block device>

```

Likewise, it is possible to migrate between a raw zoned block device and a zonefs filesystem by using backup on one
and restore on the other. One thing to be aware of is that for a given block device, zonefs will expose one zone less
to aquafs as the zonefs formatting will consume one zone for the zonefs superblock.

## Testing with db_bench

To instruct db_bench to use aquafs on a specific zoned block device, the --fs_uri parameter is used.
The device name may be used by specifying `--fs_uri=aquafs://dev:<zoned block device name>` for a raw
block device, `--fs_uri=aquafs://zonefs:<zonefs mountpoint>` for a zonefs mountpoint or by specifying
a unique identifier for the created file system by specifying `--fs_uri=aquafs://uuid:<UUID>`. UUIDs
can be listed using `./plugin/aquafs/util/aquafs ls-uuid`

```
./db_bench --fs_uri=aquafs://dev:<zoned block device name> --benchmarks=fillrandom --use_direct_io_for_flush_and_compaction

```

## Performance testing

If you want to use db_bench for testing aquafs performance, there is a a convenience script
that runs the 'long' and 'quick' performance test sets with a good set of parameters
for the drive.

`cd tests; ./aquafs_base_performance.sh <zoned block device name> [ <zonefs mountpoint> ]`


## Crashtesting
To run the crashtesting scripts, Python3 is required.
Crashtesting is done through the modified db_crashtest.py
(original [db_crashtest.py](https://github.com/facebook/rocksdb/blob/main/tools/db_crashtest.py)).
It kills the DB at a random point in time (blackbox) or at predefined places
in the RocksDB code (whitebox) and checks for recovery.
For further reading visit the RocksDB [wiki](https://github.com/facebook/rocksdb/wiki/Stress-test).
However the goal for AquaFS crashtesting is to cover a specified set of
parameters rather than randomized continuous testing.

The convenience script can be used to run all crashtest sets defined in `tests/crashtest`.
```
cd tests; ./aquafs_base_crashtest.sh <zoned block device name>
```

## Prometheus Metrics Exporter

To export performance metrics to Prometheus, do the following:

Set environment variable AQUAFS_EXPORT_PROMETHEUS=y when building to enable
prometheus export of metrics. Exporter will listen on 127.0.0.1:8080.

**Requires prometheus-cpp-pull == 1.1.0**

# AquaFS Internals

## Architecture overview
![aquafs stack](https://user-images.githubusercontent.com/447288/84152469-fa3d6300-aa64-11ea-87c4-8a6653bb9d22.png)

AquaFS implements the FileSystem API, and stores all data files on to a raw 
zoned block device. Log and lock files are stored on the default file system
under a configurable directory. Zone management is done through libzbd and
AquaFS io is done through normal pread/pwrite calls.

## File system implementation

Files are mapped into into a set of extents:

* Extents are block-aligned, continuous regions on the block device
* Extents do not span across zones
* A zone may contain more than one extent
* Extents from different files may share zones

### Reclaim 

AquaFS is exceptionally lazy at current state of implementation and does 
not do any garbage collection whatsoever. As files gets deleted, the used
capacity zone counters drops and when it reaches zero, a zone can be reset
and reused.

###  Metadata 

Metadata is stored in a rolling log in the first zones of the block device.

Each valid meta data zone contains:

* A superblock with the current sequence number and global file system metadata
* At least one snapshot of all files in the file system
* Incremental file system updates (new files, new extents, deletes, renames etc)

# Contribution Guide

AquaFS uses clang-format with Google code style. You may run the following commands
before submitting a PR.

```bash
clang-format-11 -n -Werror --style=file fs/* util/aquafs.cc # Check for style issues
clang-format-11 -i --style=file fs/* util/aquafs.cc         # Auto-fix the style issues
```
