# ZNS

#### block based flash的问题

Flash必须先擦除再写，擦除的最小粒度大于写入的最小粒度 =>异地更新 =>使用FTL负责地址映射，垃圾回收、磨损均衡、坏块管理等 =>不同文件的数据杂糅在一起 =>先移动有效数据到OP，才能擦除数据（垃圾回收）=>OP浪费空间，移动有效数据浪费时间=>换一种思路

#### ZNS的效果

ZNS-SSD的吞吐和延时不受GC影响，且不需要额外的OP空间。 zenfs是端到端的数据访问，不会再走内核庞大的I/O调度逻辑。

#### ZNS的细节

将整个SSD内部的存储区域分为多个zone 空间。不同的zone 空间之间的数据可以是独立的。最主要的是，每一个zone 内部的写入 只允许顺序写，可以随机读。为了保证zone 内部的顺序写，在ZNS内部想要覆盖写一段LBA地址的话需要先reset（清理当前地址的数据），才能重新顺序写这一段逻辑地址空间。

> 不需要垃圾回收，需要上层软件配合，保证一个zone在全部数据失效的时候擦除。

**ZNS 的结构：Zone**

摘自 `/usr/include/linux/blkzoned.h`：

```c
/**
 * enum blk_zone_type - Types of zones allowed in a zoned device.
 *
 * @BLK_ZONE_TYPE_CONVENTIONAL: The zone has no write pointer and can be writen
 *                              randomly. Zone reset has no effect on the zone.
 * @BLK_ZONE_TYPE_SEQWRITE_REQ: The zone must be written sequentially
 * @BLK_ZONE_TYPE_SEQWRITE_PREF: The zone can be written non-sequentially
 *
 * Any other value not defined is reserved and must be considered as invalid.
 */
enum blk_zone_type {
	BLK_ZONE_TYPE_CONVENTIONAL	= 0x1,
	BLK_ZONE_TYPE_SEQWRITE_REQ	= 0x2,
	BLK_ZONE_TYPE_SEQWRITE_PREF	= 0x3,
};
```

摘自 `libzbd/zdb.h`：

```c
/**
 * @brief Zone types
 *
 * @ZBD_ZONE_TYPE_CNV: The zone has no write pointer and can be writen
 *		       randomly. Zone reset has no effect on the zone.
 * @ZBD_ZONE_TYPE_SWR: The zone must be written sequentially
 * @ZBD_ZONE_TYPE_SWP: The zone can be written randomly
 */
enum zbd_zone_type {
	ZBD_ZONE_TYPE_CNV	= BLK_ZONE_TYPE_CONVENTIONAL,
	ZBD_ZONE_TYPE_SWR	= BLK_ZONE_TYPE_SEQWRITE_REQ,
	ZBD_ZONE_TYPE_SWP	= BLK_ZONE_TYPE_SEQWRITE_PREF,
};
```

Zone 的分类：

1. Conventional Zone
   1. 块内地址能够随机写
   2. 没有写指针机制（与普通 SSD 一致）
   3. Reset 对这种 Zone 无效
2. Sequentially Write Required Zone
   1. 不能随机写，必须从当前 Zone 的写指针开始顺序写
   2. 可以 Reset 来清除指针值

#### Zenfs

**Linux内核对ZNS的支持**

ZNS的特性 需要内核支持，所以开发了ZBD(zoned block device) 内核子系统 来提供通用的块层访问接口。除了支持内核通过ZBD 访问ZNS之外，还提供了用户API ioctl进行一些基础数据的访问，包括：当前环境 zone 设备的枚举，展示已有的zone的信息 ，管理某一个具体的zone(比如reset)。

在近期，为了更友好得评估ZNS-ssd的性能，在ZBD 上支持了暴露 per zone capacity 和 active zones limit。

**Zenfs代码分析**

TODO
