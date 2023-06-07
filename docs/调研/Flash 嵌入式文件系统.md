# Flash 嵌入式文件系统

**嵌入式系统中 Flash 和文件系统的应用**

{% embed url="https://elinux.org/images/4/44/Squashfs_eng.pdf" %}

![](<Flash 嵌入式文件系统.assets/image (4).png>)![](<Flash 嵌入式文件系统.assets/image (12).png>)

![](<Flash 嵌入式文件系统.assets/image (10).png>)

多数嵌入式解决方案都是把压缩后的固件存储到 Flash 中，启动时解压到 RAM 中再在 RAM 上启动

可以改进的点：

1. 压缩状态下的文件系统无法直接 XIP，需要在内存中先解压。如果可以联系 DDR 的特性，能不能做到直接解压执行，或者边解压边执行？
2. Cramfs/Squashfs 都是只读文件系统，不能进行在线修改，在嵌入式场景下只能对整个文件系统做在线升级，不能做部分升级。
   1. 能不能改成可读写的文件系统？
   2. 能不能做部分 OTA？能不能做分区？

**wear leveling：磨损均衡**

Linux MTD 设备专门用于 Flash 这种存储介质，提供读、写、擦除方法。

在 Linux MTD 设备中，没有算法专门做磨损均衡，但是 UBI 有这个算法。![](<Flash 嵌入式文件系统.assets/image (3).png>)![](<Flash 嵌入式文件系统.assets/image (1).png>)

UBIFS 运行在 UBI 之上，所以这个文件系统本身并不需要考虑磨损均衡，这是下一层的 UBI 逻辑。

yaffs2 等就没有磨损均衡了。

_**UBI：Unsorted Block Images**_

{% embed url="http://www.linux-mtd.infradead.org/doc/ubi.ppt" %}

![](<Flash 嵌入式文件系统.assets/image.png>)![](<Flash 嵌入式文件系统.assets/image (5).png>)

[Memory Technology Device (MTD) Subsystem for Linux.](http://www.linux-mtd.infradead.org/doc/ubi.html)

* UBI 不是闪存转换层 (FTL)，与 FTL 没有任何关系；
* UBI 与裸 Flash 一起工作，不适用于消费级闪存，例如 MMC、RS-MMC、eMMC、SD、mini-SD、micro-SD、CF、MemoryStick、USB 闪存驱动器等；相反，UBI 适用于原始闪存设备，这些设备主要用于嵌入式设备中，如手机等。

请不要混淆。请参阅[此处](http://www.linux-mtd.infradead.org/doc/ubifs.html#L\_raw\_vs\_ftl)了解有关原始闪存设备与 FTL 设备的更多信息。

UBI (Latin: "where?") 代表着 "Unsorted Block Images"。它是一个为裸 Flash 设备管理多个逻辑卷的卷管理系统，并将 I/O 负载（如均衡）分布在整个闪存芯片上。

在某种程度上，UBI 可以与逻辑卷管理器（LVM）进行比较。虽然 LVM 将逻辑扇区映射到物理扇区，但 UBI 则将逻辑擦除块映射到物理擦除块。但除了映射之外，UBI 还实现了全局均衡和透明的错误处理。

UBI 卷是一组连续的逻辑擦除块（LEB）。每个逻辑擦除块都会动态地映射到物理擦除块（PEB）。这个映射由 UBI 管理，并且对用户和高级软件来说是隐藏的。UBI 是提供全局均衡、每个物理擦除块擦除计数器以及透明地将数据从更磨损的物理擦除块移动到更不磨损的擦除块的基础机制。

当创建卷时，指定 UBI 卷大小，但可以稍后更改（可以动态调整卷大小）。有一些用户空间工具可用于操作 UBI 卷。

有两种类型的 UBI 卷：动态卷和静态卷。静态卷是只读的，并且其内容受到 CRC-32 校验和的保护，而动态卷是可读可写的，上层（例如，文件系统）负责确保数据完整性。

静态卷通常用于内核、initramfs 和 dtb。打开较大的静态卷可能会产生显著的惩罚，因为此时需要计算 CRC-32。如果您想将静态卷用于内核、initramfs 或 dtb 以外的任何内容，则可能会出现问题，最好改用动态卷。

UBI 知道坏的擦除块（即随着时间的推移而磨损的闪存部分）并使上层软件无需处理坏的擦除块。UBI 有一组保留的物理擦除块，在物理擦除块变坏时，它会透明地用好的物理擦除块替换它。 UBI 将新发现的坏物理擦除块中的数据移动到好的擦除块中。其结果是，UBI 卷的用户不会注意到 I/O 错误，因为 UBI 会透明地处理它们。

NAND 闪存也容易出现读写操作上的位翻转错误。通过 ECC 校验和可以纠正位翻转，但它们可能随着时间的推移而累积并导致数据丢失。UBI 通过将具有位翻转的物理擦除块中的数据移动到其他物理擦除块中来解决此问题。此过程称为**scrubbing**。Scrubbing 在后台透明地完成，上层软件不会注意到。

以下是 UBI 的主要特点：

* UBI 提供可以动态创建、删除或调整大小的卷；➡️类似 LVM 功能
* UBI 在整个闪存设备上实现磨损平衡（即，您可能认为您正在连续写入/擦除 UBI 卷的同一逻辑擦除块，但 UBI 将将其扩展到闪存芯片的所有物理擦除块）；➡️类似 FTL 的软件磨损均衡实现
* UBI 透明地处理坏的物理擦除块；➡️NandFlash 坏块处理
* UBI 通过刷卡来最小化数据丢失的可能性。➡️针对性预防 NandFlash 数据错误

有一个名为 `gluebi` 的附加驱动程序，它在 UBI 卷的顶部模拟 MTD 设备。这看起来有点奇怪，因为 UBI 在 MTD 设备的顶部工作，然后 `gluebi` 在其上模拟其他 MTD 设备，但这实际上是可行的，并使得现有软件（例如 JFFS2）能够在 UBI 卷上运行。但是，新软件可能会从UBI的高级功能中受益，并让UBI解决许多闪存技术带来的问题。➡️可以通过兼容层将上述功能添加到其他 MTD 文件系统而无需修改代码。

UBI 与 MTD 的比较：[https://www.cnblogs.com/gmpy/p/10874475.html](https://www.cnblogs.com/gmpy/p/10874475.html)

* Flash驱动直接操作设备，而MTD在Flash驱动之上，向上呈现统一的操作接口。所以MTD的"使命"是 **屏蔽不同 Flash 的操作差异，向上提供统一的操作接口** 。
* UBI基于MTD，那么UBI的目的是什么呢？ **在MTD上实现 Nand 特性的管理逻辑，向上屏蔽 Nand 的特性** 。

_**OverlayFS**_

OverlayFS 是一种联合文件系统，可以将多个文件系统层叠在一起，形成一个虚拟文件系统。OverlayFS 的特点包括：

* 可写层和只读层：OverlayFS 将多个文件系统层叠在一起，其中一个文件系统为可写层，其他文件系统为只读层。可写层可以包含新文件和修改文件，只读层包含原始文件和只读文件。
* 高效存储：OverlayFS 可以将多个文件系统的内容合并到一个虚拟文件系统中，避免重复存储和占用过多的存储空间。此外，OverlayFS 还支持 Copy-on-Write（写时复制）技术，可以减少文件系统的复制和移动操作，提高存储效率。
* 高性能：OverlayFS 可以在运行时将多个文件系统层叠在一起，形成一个虚拟文件系统。这可以减少文件系统的访问时间和提高系统性能。此外，OverlayFS 还支持内核级别的缓存和预读技术，可以提高文件系统的访问速度和性能。
* 支持多种文件系统格式：OverlayFS 支持多种文件系统格式，包括 ext4、XFS、Btrfs 等。这使得 OverlayFS 可以与不同的文件系统互操作，并提供更多的灵活性和可扩展性。

总的来说，OverlayFS 是一种高效存储和高性能的联合文件系统，可以将多个文件系统层叠在一起，形成一个虚拟文件系统。它适用于需要处理多个文件系统和提高系统性能的应用，例如容器、虚拟机、分布式存储等。

OpenWRT 中使用到了 OverlayFS 这一技术：[https://openwrt.org/zh/docs/techref/filesystems](https://openwrt.org/zh/docs/techref/filesystems)

![](<Flash 嵌入式文件系统.assets/image (8).png>)

OverlayFS基本结构：多个文件系统的堆叠和合并

将 FUSE 和 OverlayFS 结合：[https://github.com/containers/fuse-overlayfs](https://github.com/containers/fuse-overlayfs)

利用 OverlayFS 完成文件系统热迁移：[https://github.com/stanford-rc/fuse-migratefs](https://github.com/stanford-rc/fuse-migratefs)
