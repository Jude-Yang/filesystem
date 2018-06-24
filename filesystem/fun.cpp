#include "file.h"
using namespace std;

//函数实现

void ls(int parinoAddr)		//显示当前目录下的所有文件和文件夹。参数：当前目录的inode节点地址 
{
	Inode cur;
	//取出这个inode
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	fflush(fr);

	//取出目录项数
	int cnt = cur.i_cnt;

	//判断文件模式。6为owner，3为group，0为other
	int filemode;
	if (strcmp(Cur_User_Name, cur.i_uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_User_Name, cur.i_gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	//if (((cur.i_mode >> filemode >> 2) & 1) == 0) {
	//	//没有读取权限
	//	printf("权限不足：无读取权限\n");
	//	return;
	//}

	//依次取出磁盘块
	int i = 0;
	while (i<cnt && i<160) {
		DirItem dirlist[16] = { 0 };
		if (cur.i_dirBlock[i / 16] == -1) {
			i += 16;
			continue;
		}
		//取出磁盘块
		int parblockAddr = cur.i_dirBlock[i / 16];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j<16 && i<cnt; j++) {
			Inode tmp;
			//取出该目录项的inode，判断该目录项是目录还是文件
			fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
			fread(&tmp, sizeof(Inode), 1, fr);
			fflush(fr);

			if (strcmp(dirlist[j].itemName, "") == 0) {
				continue;
			}

			//输出信息
			if (((tmp.i_mode >> 9) & 1) == 1) {
				printf("d");
			}
			else {
				printf("-");
			}

			tm *ptr;	//存储时间
			ptr = gmtime(&tmp.i_mtime);

			//输出权限信息
			int t = 8;
			while (t >= 0) {
				if (((tmp.i_mode >> t) & 1) == 1) {
					if (t % 3 == 2)	printf("r");
					if (t % 3 == 1)	printf("w");
					if (t % 3 == 0)	printf("x");
				}
				else {
					printf("-");
				}
				t--;
			}
			printf("\t");

			//其它
			printf("%d\t", tmp.i_cnt);	//链接
			printf("%s\t", tmp.i_uname);	//文件所属用户名
			printf("%s\t", tmp.i_gname);	//文件所属用户名
			printf("%d B\t", tmp.i_size);	//文件大小
			printf("%d.%d.%d %02d:%02d:%02d  ", 1900 + ptr->tm_year, ptr->tm_mon + 1, ptr->tm_mday, (8 + ptr->tm_hour) % 24, ptr->tm_min, ptr->tm_sec);	//上一次修改的时间
			printf("%s", dirlist[j].itemName);	//文件名
			printf("\n");
			i++;
		}

	}
	/*  未写完 */

}
void Ready()	//登录系统前的准备工作,变量初始化+注册+安装
{
	//初始化变量
	isLogin = false;
	strcpy(Cur_User_Name, "root");
	strcpy(Cur_Group_Name, "root");

	//获取主机名
	memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
	DWORD k = 100;
	GetComputerName(Cur_Host_Name, &k);

	//根目录inode地址 ，当前目录地址和名字
	Root_Dir_Addr = Inode_StartAddr;	//第一个inode地址
	Cur_Dir_Addr = Root_Dir_Addr;
	strcpy(Cur_Dir_Name, "/");


	char c;
	printf("是否格式化?[y/n]");
	while (c = _getch()) {
		fflush(stdin);
		if (c == 'y') {
			printf("\n");
			printf("文件系统正在格式化……\n");
			if (!Format()) {
				printf("文件系统格式化失败\n");
				return;
			}
			printf("格式化完成\n");
			break;
		}
		else if (c == 'n') {
			printf("\n");
			break;
		}
	}

	//printf("载入文件系统……\n");
	if (!Install()) {
		printf("安装文件系统失败\n");
		return;
	}
	//printf("载入完成\n");
}
bool Format()	//格式化一个虚拟磁盘文件
{
	int i, j;

	//初始化超级块
	superblock->s_INODE_NUM = INODE_NUM;
	superblock->s_BLOCK_NUM = BLOCK_NUM;
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);
	superblock->s_INODE_SIZE = INODE_SIZE;
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;
	superblock->s_free_INODE_NUM = INODE_NUM;
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock->s_free_addr = Block_StartAddr;	//空闲块堆栈指针为第一块block
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;
	superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
	superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
	superblock->s_Block_StartAddr = Block_StartAddr;
	superblock->s_Inode_StartAddr = Inode_StartAddr;
	//空闲块堆栈在后面赋值

	//初始化inode位图
	memset(inode_bitmap, 0, sizeof(inode_bitmap));
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(inode_bitmap, sizeof(inode_bitmap), 1, fw);

	//初始化block位图
	memset(block_bitmap, 0, sizeof(block_bitmap));
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(block_bitmap, sizeof(block_bitmap), 1, fw);

	//初始化磁盘块区，根据成组链接法组织	
	for (i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--) {	//一共INODE_NUM/BLOCKS_PER_GROUP组，一组FREESTACKNUM（128）个磁盘块 ，第一个磁盘块作为索引
		if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			superblock->s_free[0] = -1;	//没有下一个空闲块了
		else
			superblock->s_free[0] = Block_StartAddr + (i + 1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//指向下一个空闲块
		for (j = 1; j<BLOCKS_PER_GROUP; j++) {
			superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
		}
		fseek(fw, Block_StartAddr + i*BLOCKS_PER_GROUP*BLOCK_SIZE, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//填满这个磁盘块，512字节
	}
	//超级块写入到虚拟磁盘文件
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	fflush(fw);

	//读取inode位图
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//读取block位图
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	fflush(fr);

	//创建根目录 "/"
	Inode cur;

	//申请inode
	int inoAddr = ialloc();

	//给这个inode申请磁盘块
	int blockAddr = balloc();

	//在这个磁盘块里加入一个条目 "."
	DirItem dirlist[16] = { 0 };
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inodeAddr = inoAddr;

	//写回磁盘块
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//给inode赋值
	cur.i_ino = 0;
	cur.i_atime = time(NULL);
	cur.i_ctime = time(NULL);
	cur.i_mtime = time(NULL);
	strcpy(cur.i_uname, Cur_User_Name);
	strcpy(cur.i_gname, Cur_Group_Name);
	cur.i_cnt = 1;	//一个项，当前目录,"."
	cur.i_dirBlock[0] = blockAddr;
	for (i = 1; i<10; i++) {
		cur.i_dirBlock[i] = -1;
	}
	cur.i_size = superblock->s_BLOCK_SIZE;
	cur.i_indirBlock_1 = -1;	//没使用一级间接块
	cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;


	//写回inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);

	//创建目录及配置文件
	mkdir(Root_Dir_Addr, "home");	//用户目录
	cd(Root_Dir_Addr, "home");
	mkdir(Cur_Dir_Addr, "root");

	cd(Cur_Dir_Addr, "..");
	mkdir(Cur_Dir_Addr, "etc");	//配置文件目录
	cd(Cur_Dir_Addr, "etc");

	char buf[1000] = { 0 };

	cd(Cur_Dir_Addr, "..");	//回到根目录

	return true;
}
bool Install()	//安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
{
	//读写虚拟磁盘文件，读取超级块，读取inode位图，block位图，读取主目录，读取etc目录，读取管理员admin目录，读取用户xiao目录，读取用户passwd文件。

	//读取超级块
	fseek(fr, Superblock_StartAddr, SEEK_SET);
	fread(superblock, sizeof(SuperBlock), 1, fr);

	//读取inode位图
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//读取block位图
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	return true;
}
void printSuperBlock()		//打印超级块信息
{
	printf("\n");
	printf("空闲inode数 / 总inode数 ：%d / %d\n", superblock->s_free_INODE_NUM, superblock->s_INODE_NUM);
	printf("空闲block数 / 总block数 ：%d / %d\n", superblock->s_free_BLOCK_NUM, superblock->s_BLOCK_NUM);
	printf("本系统 block大小：%d 字节，每个inode占 %d 字节（真实大小：%d 字节）\n", superblock->s_BLOCK_SIZE, superblock->s_INODE_SIZE, sizeof(Inode));
	printf("\t每磁盘块组（空闲堆栈）包含的block数量：%d\n", superblock->s_blocks_per_group);
	printf("\t超级块占 %d 字节（真实大小：%d 字节）\n", superblock->s_BLOCK_SIZE, superblock->s_SUPERBLOCK_SIZE);
	printf("磁盘分布：\n");
	printf("\t超级块开始位置：%d B\n", superblock->s_Superblock_StartAddr);
	printf("\tinode位图开始位置：%d B\n", superblock->s_InodeBitmap_StartAddr);
	printf("\tblock位图开始位置：%d B\n", superblock->s_BlockBitmap_StartAddr);
	printf("\tinode区开始位置：%d B\n", superblock->s_Inode_StartAddr);
	printf("\tblock区开始位置：%d B\n", superblock->s_Block_StartAddr);
	printf("\n");

	return;
}
void printInodeBitmap()	//打印inode使用情况
{
	printf("\n");
	printf("inode使用表：[uesd:%d %d/%d]\n", superblock->s_INODE_NUM - superblock->s_free_INODE_NUM, superblock->s_free_INODE_NUM, superblock->s_INODE_NUM);
	int i;
	i = 0;
	printf("0 ");
	while (i<superblock->s_INODE_NUM) {
		if (inode_bitmap[i])
			printf("*");
		else
			printf(".");
		i++;
		if (i != 0 && i % 32 == 0) {
			printf("\n");
			if (i != superblock->s_INODE_NUM)
				printf("%d ", i / 32);
		}
	}
	printf("\n");
	printf("\n");
	return;
}
void printBlockBitmap(int num)	//打印block使用情况
{
	printf("\n");
	printf("block（磁盘块）使用表：[used:%d %d/%d]\n", superblock->s_BLOCK_NUM - superblock->s_free_BLOCK_NUM, superblock->s_free_BLOCK_NUM, superblock->s_BLOCK_NUM);
	int i;
	i = 0;
	printf("0 ");
	while (i<num) {
		if (block_bitmap[i])
			printf("*");
		else
			printf(".");
		i++;
		if (i != 0 && i % 32 == 0) {
			printf("\n");
			if (num == superblock->s_BLOCK_NUM)
				getchar();
			if (i != superblock->s_BLOCK_NUM)
				printf("%d ", i / 32);
		}
	}
	printf("\n");
	printf("\n");
	return;
}
int balloc()	//磁盘块分配函数
{
	//使用超级块中的空闲块堆栈
	//计算当前栈顶
	int top;	//栈顶指针
	if (superblock->s_free_BLOCK_NUM == 0) {	//剩余空闲块数为0
		printf("没有空闲块可以分配\n");
		return -1;	//没有可分配的空闲块，返回-1
	}
	else {	//还有剩余块
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	}
	//将栈顶取出
	//如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
	int retAddr;

	if (top == 0) {
		retAddr = superblock->s_free_addr;
		superblock->s_free_addr = superblock->s_free[0];	//取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

															//取出对应空闲块内容，覆盖原来的空闲块堆栈

															//取出下一个空闲块堆栈，覆盖原来的
		fseek(fr, superblock->s_free_addr, SEEK_SET);
		fread(superblock->s_free, sizeof(superblock->s_free), 1, fr);
		fflush(fr);

		superblock->s_free_BLOCK_NUM--;

	}
	else {	//如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
		retAddr = superblock->s_free[top];	//保存返回地址
		superblock->s_free[top] = -1;	//清栈顶
		top--;		//栈顶指针-1
		superblock->s_free_BLOCK_NUM--;	//空闲块数-1

	}

	//更新超级块
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);

	//更新block位图
	block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE] = 1;
	fseek(fw, (retAddr - Block_StartAddr) / BLOCK_SIZE + BlockBitmap_StartAddr, SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
	fwrite(&block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
	fflush(fw);

	return retAddr;

}
bool bfree(int addr)	//磁盘块释放函数
{
	//判断
	//该地址不是磁盘块的起始地址
	if ((addr - Block_StartAddr) % superblock->s_BLOCK_SIZE != 0) {
		printf("地址错误,该位置不是block（磁盘块）起始位置\n");
		return false;
	}
	unsigned int bno = (addr - Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode节点号
																			//该地址还未使用，不能释放空间
	if (block_bitmap[bno] == 0) {
		printf("该block（磁盘块）还未使用，无法释放\n");
		return false;
	}

	//可以释放
	//计算当前栈顶
	int top;	//栈顶指针
	if (superblock->s_free_BLOCK_NUM == superblock->s_BLOCK_NUM) {	//没有非空闲的磁盘块
		printf("没有非空闲的磁盘块，无法释放\n");
		return false;	//没有可分配的空闲块，返回-1
	}
	else {	//非满
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;

		//清空block内容
		char tmp[BLOCK_SIZE] = { 0 };
		fseek(fw, addr, SEEK_SET);
		fwrite(tmp, sizeof(tmp), 1, fw);

		if (top == superblock->s_blocks_per_group - 1) {	//该栈已满

															//该空闲块作为新的空闲块堆栈
			superblock->s_free[0] = superblock->s_free_addr;	//新的空闲块堆栈第一个地址指向旧的空闲块堆栈指针
			int i;
			for (i = 1; i<superblock->s_blocks_per_group; i++) {
				superblock->s_free[i] = -1;	//清空栈元素的其它地址
			}
			fseek(fw, addr, SEEK_SET);
			fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//填满这个磁盘块，512字节

		}
		else {	//栈还未满
			top++;	//栈顶指针+1
			superblock->s_free[top] = addr;	//栈顶放上这个要释放的地址，作为新的空闲块
		}
	}


	//更新超级块
	superblock->s_free_BLOCK_NUM++;	//空闲块数+1
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	//更新block位图
	block_bitmap[bno] = 0;
	fseek(fw, bno + BlockBitmap_StartAddr, SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
	fwrite(&block_bitmap[bno], sizeof(bool), 1, fw);
	fflush(fw);

	return true;
}
int ialloc()	//分配i节点区函数，返回inode地址
{
	//在inode位图中顺序查找空闲的inode，找到则返回inode地址。函数结束。
	if (superblock->s_free_INODE_NUM == 0) {
		printf("没有空闲inode可以分配\n");
		return -1;
	}
	else {

		//顺序查找空闲的inode
		int i;
		for (i = 0; i<superblock->s_INODE_NUM; i++) {
			if (inode_bitmap[i] == 0)	//找到空闲inode
				break;
		}


		//更新超级块
		superblock->s_free_INODE_NUM--;	//空闲inode数-1
		fseek(fw, Superblock_StartAddr, SEEK_SET);
		fwrite(superblock, sizeof(SuperBlock), 1, fw);

		//更新inode位图
		inode_bitmap[i] = 1;
		fseek(fw, InodeBitmap_StartAddr + i, SEEK_SET);
		fwrite(&inode_bitmap[i], sizeof(bool), 1, fw);
		fflush(fw);

		return Inode_StartAddr + i*superblock->s_INODE_SIZE;
	}
}
bool mkdir(int parinoAddr, char name[])	//目录创建函数。参数：上一层目录文件inode地址 ,要创建的目录名
{
	if (strlen(name) >= MAX_NAME_SIZE) {
		printf("超过最大目录名长度\n");
		return false;
	}

	DirItem dirlist[16];	//临时目录清单9

							//从这个地址取出inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int cnt = cur.i_cnt + 1;	//目录项数
	int posi = -1, posj = -1;
	while (i<160) {
		//160个目录项之内，可以直接在直接块里找
		int dno = i / 16;	//在第几个直接块里

		if (cur.i_dirBlock[dno] == -1) {
			i += 16;
			continue;
		}
		//取出这个直接块，要加入的目录条目的位置
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j<16; j++) {

			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);
				if (((tmp.i_mode >> 9) & 1) == 1) {	//不是目录
					printf("目录已存在\n");
					return false;
				}
			}
			else if (strcmp(dirlist[j].itemName, "") == 0) {
				//找到一个空闲记录，将新目录创建到这个位置 
				//记录这个位置
				if (posi == -1) {
					posi = dno;
					posj = j;
				}

			}
			i++;
		}

	}
	if (posi != -1) {	//找到这个空闲位置

						//取出这个直接块，要加入的目录条目的位置
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//创建这个目录项
		strcpy(dirlist[posj].itemName, name);	//目录名
												//写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
		int chiinoAddr = ialloc();	//分配当前节点地址 
		if (chiinoAddr == -1) {
			printf("inode分配失败\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

											  //设置新条目的inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		strcpy(p.i_uname, Cur_User_Name);
		strcpy(p.i_gname, Cur_Group_Name);
		p.i_cnt = 2;	//两个项，当前目录,"."和".."

						//分配这个inode的磁盘块，在磁盘号中写入两条记录 . 和 ..
		int curblockAddr = balloc();
		if (curblockAddr == -1) {
			printf("block分配失败\n");
			return false;
		}
		DirItem dirlist2[16] = { 0 };	//临时目录项列表 - 2
		strcpy(dirlist2[0].itemName, ".");
		strcpy(dirlist2[1].itemName, "..");
		dirlist2[0].inodeAddr = chiinoAddr;	//当前目录inode地址
		dirlist2[1].inodeAddr = parinoAddr;	//父目录inode地址

											//写入到当前目录的磁盘块
		fseek(fw, curblockAddr, SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		p.i_dirBlock[0] = curblockAddr;
		int k;
		for (k = 1; k<10; k++) {
			p.i_dirBlock[k] = -1;
		}
		p.i_size = superblock->s_BLOCK_SIZE;
		p.i_indirBlock_1 = -1;	//没使用一级间接块
		p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

		//将inode写入到申请的inode地址
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//将当前目录的磁盘块写回
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//写回inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else {
		printf("没找到空闲目录项,目录创建失败");
		return false;
	}
}
bool create(int parinoAddr, char name[], char buf[])	//创建文件函数，在该目录下创建文件，文件内容存在buf
{
	if (strlen(name) >= MAX_NAME_SIZE) {
		printf("超过最大文件名长度\n");
		return false;
	}

	DirItem dirlist[16];	//临时目录清单

							//从这个地址取出inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int posi = -1, posj = -1;	//找到的目录位置
	int dno;
	int cnt = cur.i_cnt + 1;	//目录项数
	while (i<160) {
		//160个目录项之内，可以直接在直接块里找
		dno = i / 16;	//在第几个直接块里

		if (cur.i_dirBlock[dno] == -1) {
			i += 16;
			continue;
		}
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j<16; j++) {

			if (posi == -1 && strcmp(dirlist[j].itemName, "") == 0) {
				//找到一个空闲记录，将新文件创建到这个位置 
				posi = dno;
				posj = j;
			}
			else if (strcmp(dirlist[j].itemName, name) == 0) {
				//重名，取出inode，判断是否是文件
				Inode cur2;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&cur2, sizeof(Inode), 1, fr);
				if (((cur2.i_mode >> 9) & 1) == 0) {	//是文件且重名，不能创建文件
					printf("文件已存在\n");
					buf[0] = '\0';
					return false;
				}
			}
			i++;
		}

	}
	if (posi != -1) {	//之前找到一个目录项了
						//取出之前那个空闲目录项对应的磁盘块
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//创建这个目录项
		strcpy(dirlist[posj].itemName, name);	//文件名
		int chiinoAddr = ialloc();	//分配当前节点地址 
		if (chiinoAddr == -1) {
			printf("inode分配失败\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

											  //设置新条目的inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		strcpy(p.i_uname, Cur_User_Name);
		strcpy(p.i_gname, Cur_Group_Name);
		p.i_cnt = 1;	//只有一个文件指向


						//将buf内容存到磁盘块 
		int k;
		int len = strlen(buf);	//文件长度，单位为字节
		for (k = 0; k<len; k += superblock->s_BLOCK_SIZE) {	//最多10次，10个磁盘快，即最多5K
															//分配这个inode的磁盘块，从控制台读取内容
			int curblockAddr = balloc();
			if (curblockAddr == -1) {
				printf("block分配失败\n");
				return false;
			}
			p.i_dirBlock[k / superblock->s_BLOCK_SIZE] = curblockAddr;
			//写入到当前目录的磁盘块
			fseek(fw, curblockAddr, SEEK_SET);
			fwrite(buf + k, superblock->s_BLOCK_SIZE, 1, fw);
		}


		for (k = len / superblock->s_BLOCK_SIZE + 1; k<10; k++) {
			p.i_dirBlock[k] = -1;
		}
		if (len == 0) {	//长度为0的话也分给它一个block
			int curblockAddr = balloc();
			if (curblockAddr == -1) {
				printf("block分配失败\n");
				return false;
			}
			p.i_dirBlock[k / superblock->s_BLOCK_SIZE] = curblockAddr;
			//写入到当前目录的磁盘块
			fseek(fw, curblockAddr, SEEK_SET);
			fwrite(buf, superblock->s_BLOCK_SIZE, 1, fw);

		}
		p.i_size = len;
		p.i_indirBlock_1 = -1;	//没使用一级间接块
		p.i_mode = 0;
		p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;

		//将inode写入到申请的inode地址
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//将当前目录的磁盘块写回
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//写回inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else
		return false;
}

void cd(int parinoAddr, char name[])	//进入当前目录下的name目录
{
	//取出当前目录的inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//依次取出inode对应的磁盘块，查找有没有名字为name的目录项
	int i = 0;

	//取出目录项数
	int cnt = cur.i_cnt;

	//判断文件模式。6为owner，3为group，0为other
	int filemode;
	if (strcmp(Cur_User_Name, cur.i_uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_User_Name, cur.i_gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	while (i<160) {
		DirItem dirlist[16] = { 0 };
		if (cur.i_dirBlock[i / 16] == -1) {
			i += 16;
			continue;
		}
		//取出磁盘块
		int parblockAddr = cur.i_dirBlock[i / 16];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j<16; j++) {
			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				//取出该目录项的inode，判断该目录项是目录还是文件
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);

				if (((tmp.i_mode >> 9) & 1) == 1) {
					//找到该目录，判断是否具有进入权限
					if (((tmp.i_mode >> filemode >> 0) & 1) == 0 && strcmp(Cur_User_Name, "root") != 0) {	//root用户所有目录都可以查看 
																											//没有执行权限
						printf("权限不足：无执行权限\n");
						return;
					}

					//找到该目录项，如果是目录，更换当前目录

					Cur_Dir_Addr = dirlist[j].inodeAddr;
					if (strcmp(dirlist[j].itemName, ".") == 0) {
						//本目录，不动
					}
					else if (strcmp(dirlist[j].itemName, "..") == 0) {
						//上一次目录
						int k;
						for (k = strlen(Cur_Dir_Name); k >= 0; k--)
							if (Cur_Dir_Name[k] == '/')
								break;
						Cur_Dir_Name[k] = '\0';
						if (strlen(Cur_Dir_Name) == 0)
							Cur_Dir_Name[0] = '/', Cur_Dir_Name[1] = '\0';
					}
					else {
						if (Cur_Dir_Name[strlen(Cur_Dir_Name) - 1] != '/')
							strcat(Cur_Dir_Name, "/");
						strcat(Cur_Dir_Name, dirlist[j].itemName);
					}

					return;
				}
				else {
					//找到该目录项，如果不是目录，继续找
				}

			}

			i++;
		}

	}

	//没找到
	printf("没有该目录\n");
	return;

}

void inUsername(char username[])	//输入用户名
{
	printf("username:");
	scanf("%s", username);	//用户名
}

void inPasswd(char passwd[])	//输入密码
{
	int plen = 0;
	char c;
	fflush(stdin);	//清空缓冲区
	printf("passwd:");
	while (c = _getch()) {
		if (c == '\r') {	//输入回车，密码确定
			passwd[plen] = '\0';
			fflush(stdin);	//清缓冲区
			printf("\n");
			break;
		}
		else if (c == '\b') {	//退格，删除一个字符
			if (plen != 0) {	//没有删到头
				plen--;
			}
		}
		else {	//密码字符
			passwd[plen++] = c;
		}
	}
}

bool login()	//登陆界面
{
	char username[100] = { 0 };
	char passwd[100] = { 0 };
	inUsername(username);	//输入用户名
	inPasswd(passwd);		//输入用户密码
	if (check(username, passwd)) {	//核对用户名和密码
		isLogin = true;
		return true;
	}
	else {
		isLogin = false;
		return false;
	}
}

bool check(char username[], char passwd[])	//核对用户名，密码
{
	if (!strcmp(username,"root")&&!strcmp(passwd,"root")) {
		return true;
	}
	else {
		printf("密码错误\n");
		cd(Cur_Dir_Addr, "..");	//回到根目录
		return false;
	}
}

void help()	//显示所有命令清单 
{
	printf("ls - 显示当前目录清单\n");
	printf("cd - 进入目录\n");
	printf("mkdir - 创建目录\n");
	printf("super - 查看超级块\n");
	printf("inode - 查看inode位图\n");
	printf("block - 查看block位图\n");
	printf("help - 显示命令清单\n");
	return;
}

void cmd(char str[])	//处理输入的命令
{
	char p1[100];
	char p2[100];
	char p3[100];
	char buf[100000];	//最大100K
	int tmp = 0;
	int i;
	sscanf(str, "%s", p1);
 if (strcmp(p1, "cd") == 0) {
		sscanf(str, "%s%s", p1, p2);
		cd(Cur_Dir_Addr, p2);
	}
 else if (strcmp(p1, "ls") == 0) {
	 ls(Cur_Dir_Addr);	//显示当前目录
 }
	else if (strcmp(p1, "mkdir") == 0) {
		sscanf(str, "%s%s", p1, p2);
		mkdir(Cur_Dir_Addr, p2);
	}
	else if (strcmp(p1, "super") == 0) {
		printSuperBlock();
	}
	else if (strcmp(p1, "inode") == 0) {
		printInodeBitmap();
	}
	else if (strcmp(p1, "create") == 0) {
		sscanf(str, "%s%s", p1, p2);
		char buf[] = "hello,wrld!";
		create(Cur_Dir_Addr, p2, buf);
	}
	else if (strcmp(p1, "block") == 0) {
		sscanf(str, "%s%s", p1, p2);
		tmp = 0;
		if ('0' <= p2[0] && p2[0] <= '9') {
			for (i = 0; p2[i]; i++)
				tmp = tmp * 10 + p2[i] - '0';
			printBlockBitmap(tmp);
		}
		else
			printBlockBitmap();
	}
	else if (strcmp(p1, "help") == 0) {
		help();
	}
	else if (strcmp(p1, "\0") == 0) {
		return;
	}
	else {
		printf("抱歉，没有该命令\n");
	}
	return;
}
