## RAID on ZNS

本方向将研究 RAID 技术在 ZNS 上的应用。

处于简化问题考虑，我们只研究相同型号 SSD 下的 RAID。

### 问题

1. RAID 种类和性能？[ZFS在不同 RAID 策略下的性能评测](https://www.liujason.com/article/679.html)

2. RAID 在 SSD 上如何实现比较好？

   Parity-Stream Separation and SLC/MLC Convertible Programming for Lifespan and Performance Improvement of SSD RAIDs

3. ZNS 的 Zones 有两种：`seq` / `cnv`。前者不能随机写入，只能顺序写入；后者可以随机写入，就像普通的硬盘一样。**如何利用这两种 Zones 的特点在 ZNS 上运行 RAID 的算法？**

   1. 两种 Zone 的兼容问题
      1. Cnv Zones 兼容 Seq Zones，不过兼容会造成 FTL Map 空间的浪费
   2. SSD 块大小兼容问题
      1. 不同的 Seq 可以看作最小长度的多个 Seq Zones
      2. 不同的 Cnv Zones 可以划分为更小的小块来尽可能抹平
   3. Seq Zones 上如何实现各种 RAID 算法

4. RAID 的性能参数选择评估？[如何设置 RAID 组的 Strip Size](https://blog.csdn.net/TV8MbnO2Y2RfU/article/details/78103790)

RAID 是指独立冗余磁盘阵列（Redundant Array of Independent Disks），它是一种数据存储技术，通过将多个硬盘组合起来，实现数据的备份、容错和性能优化。

### 实现原理

经过一些讨论，我们认为基于 Zones 的混合 RAID 是可以实现的。

#### RAID0 逻辑

RAID0（磁盘条带化）是一种基本的RAID级别，它将两个或多个磁盘驱动器组合成一个逻辑卷。在RAID0中，数据被分割成固定大小的块，并沿着所有磁盘驱动器进行交错写入，从而提高了读写性能。

RAID0的工作原理如下：

1. 数据被分割成固定大小的块。
2. 这些块按照一定的规则，如轮流或按块交替，分配到不同的物理磁盘驱动器上。
3. 每个磁盘驱动器只存储一部分数据，因此所有磁盘驱动器都可以同时读写数据，从而提高了读写速度。
4. 当需要读取数据时，RAID控制器会从所有驱动器中读取数据块，然后将它们组合成完整的数据块，并将其发送给请求数据的主机。

需要注意的是，RAID0没有冗余性，因此如果其中一个磁盘驱动器故障，整个系统将无法访问存储在该磁盘驱动器上的所有数据。因此，在使用RAID0时，应该备份重要的数据以防止数据丢失。

基于 Zones 的 RAID0，实际上就是将不同 Device 上的多个 Zones 当作一个 Device 上的来处理，并且尽量均衡分配读写负载。具体到两种 Zone 类型 `seq`/`conv`，要分别把设备上的两种 Zone 作为两类处理。

对 `conv` 类型，可以按照传统的 RAID0 逻辑处理，按相同大小的小块（条带）平均写入到不同设备上的 Zones 上即可。

对 `seq` 类型，则不能按照传统形式的 RAID0 逻辑来处理。如果分配了相同大小的条带，那么这些 Zones 的写指针就并不能抽象为逐个逐步写满的，而是同时写的，造成不能单独地对这些 Zones 进行 Reset 操作，并且需要特殊考虑这些 Zones 的占用大小。

要解决这问题，一种方案是修改 ZenFS 的 Zone Management 逻辑：

1. 对一个 RAID 组内的 Zones，不能单独设置其中单独一个 Zone 的 Reset，只能对整个 RAID 组内所有 Zones 进行 Reset；
2. 考虑 Zones 的已用容量的时候，只能考虑整个 RAID 的已用容量。

另一种方案是将一个 RAID 区域抽象为一个 Zone：

1. 结构上，这个抽象的 RAID Zone 包含来自多个 Devices 的多个 Zones；
2. 逻辑上，屏蔽了对实际 Zone 的 Reset 和求写指针操作，抽象为一个统一的 RAID Zone 的 Reset 和写指针操作；
3. 会造成一些限制，例如显著增大了 Zone Size，再一次降低了 Zones 管理能力。

还有一种方案是让 ZenFS 支持不同大小的 Zones。

实现上可以先采用抽象为 RAID Zone 来实现 RAID0 的方案，之后再修改 ZenFS 的逻辑。

#### RAID1 逻辑

上文中谈论的 `seq` Zones 的限制，对其他大部分 RAID 类型同样成立，不过 RAID1 由于其 1:1 镜像的特殊性，可以做到更简单直接的抽象。具体而言，将 N 个 Device 上所有的 `seq` 同时映射到一个 RAID Zone，写的时候直接写 N 份，读的时候做一下读负载均衡。如此实现的话则不需要考虑 RAID 条带分割的问题。

#### RAID5 逻辑

#### 动态 RAID 逻辑

为了实现动态的分块 RAID 逻辑分配，需要让 ZenFS 支持文件系统的动态扩容和缩容，同时需要找到存储 Zone - RAID 分配策略这一 `map` 数据的位置。

ZenFS 目前应该是不支持动态缩小容量的，当数据后端的 Zone 数量小于 Superblocks 中记录的值的时候，会拒绝挂载。但是如果 Zone 数量大于 Superblocks 中的记录，目前暂时没有找到类似的处理策略，可能能够用运行中修改 `ZenFS::superblock_.nr_zones_` 的方法来扩容，但是结果未知，可能需要测试。如果需要动态缩小容量，可能需要使用 GC 逻辑。

存储额外的 RAID Zone 映射，可能需要使用 `aux_path`……
#### 异步读写
目前使用`io_uring`异步读写多个device，以达到提高读写性能的目的。

`liburing`的使用：https://arthurchiao.art/blog/intro-to-io-uring-zh/#27-%E7%94%A8%E6%88%B7%E7%A9%BA%E9%97%B4%E5%BA%93-liburing
