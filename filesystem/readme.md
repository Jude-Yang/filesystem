# 设计思路：
 忽略底层部分，专注于实现文件系统对文件的组织和管理。
所谓底层部分，主要是对磁盘的读写操作的接口实现，这需要系统驱动知识。
通过创建一个本地文件承载系统，即本地创建一个MingOS.sys的二进制文件，以文件读写模拟磁盘读写，这样就可以直接进行文件系统的功能实现。

# 系统结构
superblock+inode表+block表+inode区+block区。

分布情况：
![](./src/super.png)
### superblock:
包含系统的重要信息，包括inode/block的总量、使用量、剩余量， 以及文件系统的格式与相关信息等。
### inode:
记录文件的属性，一个文件占用一个inode，同时记录此文件的数据所在的 block 号码；
### block:
实际记录文件的内容，若文件太大时，会占用多个 block。

对于inode，一般来说需要占磁盘空间的百分之一，不过这是个小系统，总大小才5M多一点（当然你也可以自己设计规划系统大小），所以分配给inode区的空间很少，剩下的空间大部分是block区。

<pre>
//超级块
struct SuperBlock{
	unsigned short s_INODE_NUM;				//inode节点数，最多 65535
	unsigned int s_BLOCK_NUM;				//磁盘块块数，最多 4294967294
    unsigned short s_free_INODE_NUM;		//空闲inode节点数
	unsigned int s_free_BLOCK_NUM;			//空闲磁盘块数
	int s_free_addr;						//空闲块堆栈指针
	int s_free[BLOCKS_PER_GROUP];			//空闲块堆栈
    unsigned short s_BLOCK_SIZE;			//磁盘块大小
	unsigned short s_INODE_SIZE;			//inode大小
	unsigned short s_SUPERBLOCK_SIZE;		//超级块大小
	unsigned short s_blocks_per_group;		//每 blockgroup 的block数量
  //磁盘分布
	int s_Superblock_StartAddr;
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};
</pre>




<pre>
//inode节点
struct Inode{
	unsigned short i_ino;					//inode标识（编号）
	unsigned short i_mode;					//存取权限。r--读取，w--写，x--执行
	unsigned short i_cnt;					//链接数。有多少文件名指向这个inode
	//unsigned short i_uid;					//文件所属用户id
	//unsigned short i_gid;					//文件所属用户组id
	char i_uname[20];						//文件所属用户
	char i_gname[20];						//文件所属用户组
	unsigned int i_size;					//文件大小，单位为字节（B）
	time_t  i_ctime;						//inode上一次变动的时间
	time_t  i_mtime;						//文件内容上一次变动的时间
	time_t  i_atime;						//文件上一次打开的时间
	int i_dirBlock[10];						//10个直接块。10*512B = 5120B = 5KB
	int i_indirBlock_1;						//一级间接块。512B/4 * 512B = 128 * 512B = 64KB
	//unsigned int i_indirBlock_2;			//二级间接块。(512B/4)*(512B/4) * 512B = 128*128*512B = 8192KB = 8MB

};
</pre>

###### 使用二级inode节点映射：
前提：每块大小512字节，每个块号使用32位(4字节)
![](./src/inodeMap.png)

### blockGroup:

## 生成磁盘格式过程（格式化过程）
- 初始化超级块
- 初始化inode对照表和block对照表
- 使用成组链接法组织block块
###### 成组链接法：
![](./src/chengzu.png)

- 生成inode
  首先检测 *superblock->s_free_INODE_NUM* 是否为零查看是否有空闲inode可分配，若有则查找inode对照表查找空闲的inode，然后在文件系统中生成inode并更新 *superblock->s_free_INODE_NUM--* 和inode对照表

- 为inode分配block块
  首先检测 *superblock->s_free_BLOCK_NUM*是否为零查看是否有空闲block可分配，若有，则查找空闲块堆栈，取出栈顶分配给inode

  # 相关功能函数

### bool makedir(int parinoAddr,char name[])


  参数：
  - parinoAddr：上一层目录文件的inode地址
  - name：文件夹名字

<pre>
//目录项
struct DirItem{								//32字节，一个磁盘块能存 512/32=16 个目录项
	char itemName[MAX_NAME_SIZE];			//目录或者文件名
	int inodeAddr;							//目录项对应的inode节点地址
};
</pre>

首先根据parinoAddr在文件系统中找出所对应的地址，然后通过* innode->i_dirBlock *找出inode所对应的空闲block块，然后把文件夹名存入该空闲bolck块并把该文件名所占用的inode地址存入。
其次生成新的inode条目，此inode条目在物理地址上是之前生成inode的下一块，并为此inode映射block块，在此block块上创建两个名为“.”和“..”的文件目录，其中“.”记录的inode地址为此时创建文件夹的文件地址，而“..”记录的为传入的parinoAddr地址。

### void cd(int parinoAddr,char name[])
参数：
- parinoAddr：当前目录地址
- name:要打开的文件名

通过文件目录的地址在文件系统上找到对应的inode结构体，然后找出里面对应的非空的block块，将其地址取出来，然后找到里面存储文件名的结构体DirItem,通过* DirItem->itemName *找到对应的文件名，此时通过 * DirItem->itemName * 取出对应的inode地址。
然后把打开的文件夹的地址给* Cur_Dir_Name *

### bool create(int parinoAddr,char name[],char buf[])
参数：
- parinoAddr:
- name[]:文件名
- buf[]:

通过parinoAddr指针找到文件系统中的inode,再通过inode所映射的block块找到空闲块，再创建文件结构体DirItem，再给文件结构体DirItem分配节点地址
然后检测是否存在文件，若不存在，申请block块存放文件，然后将此block块映射到inode上，然后将文件写入block块，然后将申请的inode和block块写入文件系统

### 权限设置
默认文件权限为0755（OCT）
