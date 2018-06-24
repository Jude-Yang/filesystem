#include "file.h"
using namespace std;

//����ʵ��

void ls(int parinoAddr)		//��ʾ��ǰĿ¼�µ������ļ����ļ��С���������ǰĿ¼��inode�ڵ��ַ 
{
	Inode cur;
	//ȡ�����inode
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	fflush(fr);

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
	int filemode;
	if (strcmp(Cur_User_Name, cur.i_uname) == 0)
		filemode = 6;
	else if (strcmp(Cur_User_Name, cur.i_gname) == 0)
		filemode = 3;
	else
		filemode = 0;

	//if (((cur.i_mode >> filemode >> 2) & 1) == 0) {
	//	//û�ж�ȡȨ��
	//	printf("Ȩ�޲��㣺�޶�ȡȨ��\n");
	//	return;
	//}

	//����ȡ�����̿�
	int i = 0;
	while (i<cnt && i<160) {
		DirItem dirlist[16] = { 0 };
		if (cur.i_dirBlock[i / 16] == -1) {
			i += 16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i / 16];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j<16 && i<cnt; j++) {
			Inode tmp;
			//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
			fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
			fread(&tmp, sizeof(Inode), 1, fr);
			fflush(fr);

			if (strcmp(dirlist[j].itemName, "") == 0) {
				continue;
			}

			//�����Ϣ
			if (((tmp.i_mode >> 9) & 1) == 1) {
				printf("d");
			}
			else {
				printf("-");
			}

			tm *ptr;	//�洢ʱ��
			ptr = gmtime(&tmp.i_mtime);

			//���Ȩ����Ϣ
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

			//����
			printf("%d\t", tmp.i_cnt);	//����
			printf("%s\t", tmp.i_uname);	//�ļ������û���
			printf("%s\t", tmp.i_gname);	//�ļ������û���
			printf("%d B\t", tmp.i_size);	//�ļ���С
			printf("%d.%d.%d %02d:%02d:%02d  ", 1900 + ptr->tm_year, ptr->tm_mon + 1, ptr->tm_mday, (8 + ptr->tm_hour) % 24, ptr->tm_min, ptr->tm_sec);	//��һ���޸ĵ�ʱ��
			printf("%s", dirlist[j].itemName);	//�ļ���
			printf("\n");
			i++;
		}

	}
	/*  δд�� */

}
void Ready()	//��¼ϵͳǰ��׼������,������ʼ��+ע��+��װ
{
	//��ʼ������
	isLogin = false;
	strcpy(Cur_User_Name, "root");
	strcpy(Cur_Group_Name, "root");

	//��ȡ������
	memset(Cur_Host_Name, 0, sizeof(Cur_Host_Name));
	DWORD k = 100;
	GetComputerName(Cur_Host_Name, &k);

	//��Ŀ¼inode��ַ ����ǰĿ¼��ַ������
	Root_Dir_Addr = Inode_StartAddr;	//��һ��inode��ַ
	Cur_Dir_Addr = Root_Dir_Addr;
	strcpy(Cur_Dir_Name, "/");


	char c;
	printf("�Ƿ��ʽ��?[y/n]");
	while (c = _getch()) {
		fflush(stdin);
		if (c == 'y') {
			printf("\n");
			printf("�ļ�ϵͳ���ڸ�ʽ������\n");
			if (!Format()) {
				printf("�ļ�ϵͳ��ʽ��ʧ��\n");
				return;
			}
			printf("��ʽ�����\n");
			break;
		}
		else if (c == 'n') {
			printf("\n");
			break;
		}
	}

	//printf("�����ļ�ϵͳ����\n");
	if (!Install()) {
		printf("��װ�ļ�ϵͳʧ��\n");
		return;
	}
	//printf("�������\n");
}
bool Format()	//��ʽ��һ����������ļ�
{
	int i, j;

	//��ʼ��������
	superblock->s_INODE_NUM = INODE_NUM;
	superblock->s_BLOCK_NUM = BLOCK_NUM;
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);
	superblock->s_INODE_SIZE = INODE_SIZE;
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;
	superblock->s_free_INODE_NUM = INODE_NUM;
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock->s_free_addr = Block_StartAddr;	//���п��ջָ��Ϊ��һ��block
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;
	superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
	superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
	superblock->s_Block_StartAddr = Block_StartAddr;
	superblock->s_Inode_StartAddr = Inode_StartAddr;
	//���п��ջ�ں��渳ֵ

	//��ʼ��inodeλͼ
	memset(inode_bitmap, 0, sizeof(inode_bitmap));
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(inode_bitmap, sizeof(inode_bitmap), 1, fw);

	//��ʼ��blockλͼ
	memset(block_bitmap, 0, sizeof(block_bitmap));
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(block_bitmap, sizeof(block_bitmap), 1, fw);

	//��ʼ�����̿��������ݳ������ӷ���֯	
	for (i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--) {	//һ��INODE_NUM/BLOCKS_PER_GROUP�飬һ��FREESTACKNUM��128�������̿� ����һ�����̿���Ϊ����
		if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			superblock->s_free[0] = -1;	//û����һ�����п���
		else
			superblock->s_free[0] = Block_StartAddr + (i + 1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
		for (j = 1; j<BLOCKS_PER_GROUP; j++) {
			superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
		}
		fseek(fw, Block_StartAddr + i*BLOCKS_PER_GROUP*BLOCK_SIZE, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//����������̿飬512�ֽ�
	}
	//������д�뵽��������ļ�
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	fflush(fw);

	//��ȡinodeλͼ
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//��ȡblockλͼ
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	fflush(fr);

	//������Ŀ¼ "/"
	Inode cur;

	//����inode
	int inoAddr = ialloc();

	//�����inode������̿�
	int blockAddr = balloc();

	//��������̿������һ����Ŀ "."
	DirItem dirlist[16] = { 0 };
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inodeAddr = inoAddr;

	//д�ش��̿�
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//��inode��ֵ
	cur.i_ino = 0;
	cur.i_atime = time(NULL);
	cur.i_ctime = time(NULL);
	cur.i_mtime = time(NULL);
	strcpy(cur.i_uname, Cur_User_Name);
	strcpy(cur.i_gname, Cur_Group_Name);
	cur.i_cnt = 1;	//һ�����ǰĿ¼,"."
	cur.i_dirBlock[0] = blockAddr;
	for (i = 1; i<10; i++) {
		cur.i_dirBlock[i] = -1;
	}
	cur.i_size = superblock->s_BLOCK_SIZE;
	cur.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
	cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;


	//д��inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);

	//����Ŀ¼�������ļ�
	mkdir(Root_Dir_Addr, "home");	//�û�Ŀ¼
	cd(Root_Dir_Addr, "home");
	mkdir(Cur_Dir_Addr, "root");

	cd(Cur_Dir_Addr, "..");
	mkdir(Cur_Dir_Addr, "etc");	//�����ļ�Ŀ¼
	cd(Cur_Dir_Addr, "etc");

	char buf[1000] = { 0 };

	cd(Cur_Dir_Addr, "..");	//�ص���Ŀ¼

	return true;
}
bool Install()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
	//��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ����ȡ��Ŀ¼����ȡetcĿ¼����ȡ����ԱadminĿ¼����ȡ�û�xiaoĿ¼����ȡ�û�passwd�ļ���

	//��ȡ������
	fseek(fr, Superblock_StartAddr, SEEK_SET);
	fread(superblock, sizeof(SuperBlock), 1, fr);

	//��ȡinodeλͼ
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//��ȡblockλͼ
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	return true;
}
void printSuperBlock()		//��ӡ��������Ϣ
{
	printf("\n");
	printf("����inode�� / ��inode�� ��%d / %d\n", superblock->s_free_INODE_NUM, superblock->s_INODE_NUM);
	printf("����block�� / ��block�� ��%d / %d\n", superblock->s_free_BLOCK_NUM, superblock->s_BLOCK_NUM);
	printf("��ϵͳ block��С��%d �ֽڣ�ÿ��inodeռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n", superblock->s_BLOCK_SIZE, superblock->s_INODE_SIZE, sizeof(Inode));
	printf("\tÿ���̿��飨���ж�ջ��������block������%d\n", superblock->s_blocks_per_group);
	printf("\t������ռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n", superblock->s_BLOCK_SIZE, superblock->s_SUPERBLOCK_SIZE);
	printf("���̷ֲ���\n");
	printf("\t�����鿪ʼλ�ã�%d B\n", superblock->s_Superblock_StartAddr);
	printf("\tinodeλͼ��ʼλ�ã�%d B\n", superblock->s_InodeBitmap_StartAddr);
	printf("\tblockλͼ��ʼλ�ã�%d B\n", superblock->s_BlockBitmap_StartAddr);
	printf("\tinode����ʼλ�ã�%d B\n", superblock->s_Inode_StartAddr);
	printf("\tblock����ʼλ�ã�%d B\n", superblock->s_Block_StartAddr);
	printf("\n");

	return;
}
void printInodeBitmap()	//��ӡinodeʹ�����
{
	printf("\n");
	printf("inodeʹ�ñ�[uesd:%d %d/%d]\n", superblock->s_INODE_NUM - superblock->s_free_INODE_NUM, superblock->s_free_INODE_NUM, superblock->s_INODE_NUM);
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
void printBlockBitmap(int num)	//��ӡblockʹ�����
{
	printf("\n");
	printf("block�����̿飩ʹ�ñ�[used:%d %d/%d]\n", superblock->s_BLOCK_NUM - superblock->s_free_BLOCK_NUM, superblock->s_free_BLOCK_NUM, superblock->s_BLOCK_NUM);
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
int balloc()	//���̿���亯��
{
	//ʹ�ó������еĿ��п��ջ
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if (superblock->s_free_BLOCK_NUM == 0) {	//ʣ����п���Ϊ0
		printf("û�п��п���Է���\n");
		return -1;	//û�пɷ���Ŀ��п飬����-1
	}
	else {	//����ʣ���
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	}
	//��ջ��ȡ��
	//�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
	int retAddr;

	if (top == 0) {
		retAddr = superblock->s_free_addr;
		superblock->s_free_addr = superblock->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��

															//ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ

															//ȡ����һ�����п��ջ������ԭ����
		fseek(fr, superblock->s_free_addr, SEEK_SET);
		fread(superblock->s_free, sizeof(superblock->s_free), 1, fr);
		fflush(fr);

		superblock->s_free_BLOCK_NUM--;

	}
	else {	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
		retAddr = superblock->s_free[top];	//���淵�ص�ַ
		superblock->s_free[top] = -1;	//��ջ��
		top--;		//ջ��ָ��-1
		superblock->s_free_BLOCK_NUM--;	//���п���-1

	}

	//���³�����
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);

	//����blockλͼ
	block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE] = 1;
	fseek(fw, (retAddr - Block_StartAddr) / BLOCK_SIZE + BlockBitmap_StartAddr, SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
	fflush(fw);

	return retAddr;

}
bool bfree(int addr)	//���̿��ͷź���
{
	//�ж�
	//�õ�ַ���Ǵ��̿����ʼ��ַ
	if ((addr - Block_StartAddr) % superblock->s_BLOCK_SIZE != 0) {
		printf("��ַ����,��λ�ò���block�����̿飩��ʼλ��\n");
		return false;
	}
	unsigned int bno = (addr - Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode�ڵ��
																			//�õ�ַ��δʹ�ã������ͷſռ�
	if (block_bitmap[bno] == 0) {
		printf("��block�����̿飩��δʹ�ã��޷��ͷ�\n");
		return false;
	}

	//�����ͷ�
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if (superblock->s_free_BLOCK_NUM == superblock->s_BLOCK_NUM) {	//û�зǿ��еĴ��̿�
		printf("û�зǿ��еĴ��̿飬�޷��ͷ�\n");
		return false;	//û�пɷ���Ŀ��п飬����-1
	}
	else {	//����
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;

		//���block����
		char tmp[BLOCK_SIZE] = { 0 };
		fseek(fw, addr, SEEK_SET);
		fwrite(tmp, sizeof(tmp), 1, fw);

		if (top == superblock->s_blocks_per_group - 1) {	//��ջ����

															//�ÿ��п���Ϊ�µĿ��п��ջ
			superblock->s_free[0] = superblock->s_free_addr;	//�µĿ��п��ջ��һ����ַָ��ɵĿ��п��ջָ��
			int i;
			for (i = 1; i<superblock->s_blocks_per_group; i++) {
				superblock->s_free[i] = -1;	//���ջԪ�ص�������ַ
			}
			fseek(fw, addr, SEEK_SET);
			fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//����������̿飬512�ֽ�

		}
		else {	//ջ��δ��
			top++;	//ջ��ָ��+1
			superblock->s_free[top] = addr;	//ջ���������Ҫ�ͷŵĵ�ַ����Ϊ�µĿ��п�
		}
	}


	//���³�����
	superblock->s_free_BLOCK_NUM++;	//���п���+1
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	//����blockλͼ
	block_bitmap[bno] = 0;
	fseek(fw, bno + BlockBitmap_StartAddr, SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[bno], sizeof(bool), 1, fw);
	fflush(fw);

	return true;
}
int ialloc()	//����i�ڵ�������������inode��ַ
{
	//��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
	if (superblock->s_free_INODE_NUM == 0) {
		printf("û�п���inode���Է���\n");
		return -1;
	}
	else {

		//˳����ҿ��е�inode
		int i;
		for (i = 0; i<superblock->s_INODE_NUM; i++) {
			if (inode_bitmap[i] == 0)	//�ҵ�����inode
				break;
		}


		//���³�����
		superblock->s_free_INODE_NUM--;	//����inode��-1
		fseek(fw, Superblock_StartAddr, SEEK_SET);
		fwrite(superblock, sizeof(SuperBlock), 1, fw);

		//����inodeλͼ
		inode_bitmap[i] = 1;
		fseek(fw, InodeBitmap_StartAddr + i, SEEK_SET);
		fwrite(&inode_bitmap[i], sizeof(bool), 1, fw);
		fflush(fw);

		return Inode_StartAddr + i*superblock->s_INODE_SIZE;
	}
}
bool mkdir(int parinoAddr, char name[])	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
{
	if (strlen(name) >= MAX_NAME_SIZE) {
		printf("�������Ŀ¼������\n");
		return false;
	}

	DirItem dirlist[16];	//��ʱĿ¼�嵥9

							//�������ַȡ��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int cnt = cur.i_cnt + 1;	//Ŀ¼����
	int posi = -1, posj = -1;
	while (i<160) {
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		int dno = i / 16;	//�ڵڼ���ֱ�ӿ���

		if (cur.i_dirBlock[dno] == -1) {
			i += 16;
			continue;
		}
		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j<16; j++) {

			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);
				if (((tmp.i_mode >> 9) & 1) == 1) {	//����Ŀ¼
					printf("Ŀ¼�Ѵ���\n");
					return false;
				}
			}
			else if (strcmp(dirlist[j].itemName, "") == 0) {
				//�ҵ�һ�����м�¼������Ŀ¼���������λ�� 
				//��¼���λ��
				if (posi == -1) {
					posi = dno;
					posj = j;
				}

			}
			i++;
		}

	}
	if (posi != -1) {	//�ҵ��������λ��

						//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName, name);	//Ŀ¼��
												//д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if (chiinoAddr == -1) {
			printf("inode����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

											  //��������Ŀ��inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		strcpy(p.i_uname, Cur_User_Name);
		strcpy(p.i_gname, Cur_Group_Name);
		p.i_cnt = 2;	//�������ǰĿ¼,"."��".."

						//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
		int curblockAddr = balloc();
		if (curblockAddr == -1) {
			printf("block����ʧ��\n");
			return false;
		}
		DirItem dirlist2[16] = { 0 };	//��ʱĿ¼���б� - 2
		strcpy(dirlist2[0].itemName, ".");
		strcpy(dirlist2[1].itemName, "..");
		dirlist2[0].inodeAddr = chiinoAddr;	//��ǰĿ¼inode��ַ
		dirlist2[1].inodeAddr = parinoAddr;	//��Ŀ¼inode��ַ

											//д�뵽��ǰĿ¼�Ĵ��̿�
		fseek(fw, curblockAddr, SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		p.i_dirBlock[0] = curblockAddr;
		int k;
		for (k = 1; k<10; k++) {
			p.i_dirBlock[k] = -1;
		}
		p.i_size = superblock->s_BLOCK_SIZE;
		p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
		p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

		//��inodeд�뵽�����inode��ַ
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//д��inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else {
		printf("û�ҵ�����Ŀ¼��,Ŀ¼����ʧ��");
		return false;
	}
}
bool create(int parinoAddr, char name[], char buf[])	//�����ļ��������ڸ�Ŀ¼�´����ļ����ļ����ݴ���buf
{
	if (strlen(name) >= MAX_NAME_SIZE) {
		printf("��������ļ�������\n");
		return false;
	}

	DirItem dirlist[16];	//��ʱĿ¼�嵥

							//�������ַȡ��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int posi = -1, posj = -1;	//�ҵ���Ŀ¼λ��
	int dno;
	int cnt = cur.i_cnt + 1;	//Ŀ¼����
	while (i<160) {
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		dno = i / 16;	//�ڵڼ���ֱ�ӿ���

		if (cur.i_dirBlock[dno] == -1) {
			i += 16;
			continue;
		}
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j<16; j++) {

			if (posi == -1 && strcmp(dirlist[j].itemName, "") == 0) {
				//�ҵ�һ�����м�¼�������ļ����������λ�� 
				posi = dno;
				posj = j;
			}
			else if (strcmp(dirlist[j].itemName, name) == 0) {
				//������ȡ��inode���ж��Ƿ����ļ�
				Inode cur2;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&cur2, sizeof(Inode), 1, fr);
				if (((cur2.i_mode >> 9) & 1) == 0) {	//���ļ������������ܴ����ļ�
					printf("�ļ��Ѵ���\n");
					buf[0] = '\0';
					return false;
				}
			}
			i++;
		}

	}
	if (posi != -1) {	//֮ǰ�ҵ�һ��Ŀ¼����
						//ȡ��֮ǰ�Ǹ�����Ŀ¼���Ӧ�Ĵ��̿�
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName, name);	//�ļ���
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if (chiinoAddr == -1) {
			printf("inode����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

											  //��������Ŀ��inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		strcpy(p.i_uname, Cur_User_Name);
		strcpy(p.i_gname, Cur_Group_Name);
		p.i_cnt = 1;	//ֻ��һ���ļ�ָ��


						//��buf���ݴ浽���̿� 
		int k;
		int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
		for (k = 0; k<len; k += superblock->s_BLOCK_SIZE) {	//���10�Σ�10�����̿죬�����5K
															//�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
			int curblockAddr = balloc();
			if (curblockAddr == -1) {
				printf("block����ʧ��\n");
				return false;
			}
			p.i_dirBlock[k / superblock->s_BLOCK_SIZE] = curblockAddr;
			//д�뵽��ǰĿ¼�Ĵ��̿�
			fseek(fw, curblockAddr, SEEK_SET);
			fwrite(buf + k, superblock->s_BLOCK_SIZE, 1, fw);
		}


		for (k = len / superblock->s_BLOCK_SIZE + 1; k<10; k++) {
			p.i_dirBlock[k] = -1;
		}
		if (len == 0) {	//����Ϊ0�Ļ�Ҳ�ָ���һ��block
			int curblockAddr = balloc();
			if (curblockAddr == -1) {
				printf("block����ʧ��\n");
				return false;
			}
			p.i_dirBlock[k / superblock->s_BLOCK_SIZE] = curblockAddr;
			//д�뵽��ǰĿ¼�Ĵ��̿�
			fseek(fw, curblockAddr, SEEK_SET);
			fwrite(buf, superblock->s_BLOCK_SIZE, 1, fw);

		}
		p.i_size = len;
		p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
		p.i_mode = 0;
		p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;

		//��inodeд�뵽�����inode��ַ
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//д��inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else
		return false;
}

void cd(int parinoAddr, char name[])	//���뵱ǰĿ¼�µ�nameĿ¼
{
	//ȡ����ǰĿ¼��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
	int i = 0;

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
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
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i / 16];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j<16; j++) {
			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);

				if (((tmp.i_mode >> 9) & 1) == 1) {
					//�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
					if (((tmp.i_mode >> filemode >> 0) & 1) == 0 && strcmp(Cur_User_Name, "root") != 0) {	//root�û�����Ŀ¼�����Բ鿴 
																											//û��ִ��Ȩ��
						printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
						return;
					}

					//�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼

					Cur_Dir_Addr = dirlist[j].inodeAddr;
					if (strcmp(dirlist[j].itemName, ".") == 0) {
						//��Ŀ¼������
					}
					else if (strcmp(dirlist[j].itemName, "..") == 0) {
						//��һ��Ŀ¼
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
					//�ҵ���Ŀ¼��������Ŀ¼��������
				}

			}

			i++;
		}

	}

	//û�ҵ�
	printf("û�и�Ŀ¼\n");
	return;

}

void inUsername(char username[])	//�����û���
{
	printf("username:");
	scanf("%s", username);	//�û���
}

void inPasswd(char passwd[])	//��������
{
	int plen = 0;
	char c;
	fflush(stdin);	//��ջ�����
	printf("passwd:");
	while (c = _getch()) {
		if (c == '\r') {	//����س�������ȷ��
			passwd[plen] = '\0';
			fflush(stdin);	//�建����
			printf("\n");
			break;
		}
		else if (c == '\b') {	//�˸�ɾ��һ���ַ�
			if (plen != 0) {	//û��ɾ��ͷ
				plen--;
			}
		}
		else {	//�����ַ�
			passwd[plen++] = c;
		}
	}
}

bool login()	//��½����
{
	char username[100] = { 0 };
	char passwd[100] = { 0 };
	inUsername(username);	//�����û���
	inPasswd(passwd);		//�����û�����
	if (check(username, passwd)) {	//�˶��û���������
		isLogin = true;
		return true;
	}
	else {
		isLogin = false;
		return false;
	}
}

bool check(char username[], char passwd[])	//�˶��û���������
{
	if (!strcmp(username,"root")&&!strcmp(passwd,"root")) {
		return true;
	}
	else {
		printf("�������\n");
		cd(Cur_Dir_Addr, "..");	//�ص���Ŀ¼
		return false;
	}
}

void help()	//��ʾ���������嵥 
{
	printf("ls - ��ʾ��ǰĿ¼�嵥\n");
	printf("cd - ����Ŀ¼\n");
	printf("mkdir - ����Ŀ¼\n");
	printf("super - �鿴������\n");
	printf("inode - �鿴inodeλͼ\n");
	printf("block - �鿴blockλͼ\n");
	printf("help - ��ʾ�����嵥\n");
	return;
}

void cmd(char str[])	//�������������
{
	char p1[100];
	char p2[100];
	char p3[100];
	char buf[100000];	//���100K
	int tmp = 0;
	int i;
	sscanf(str, "%s", p1);
 if (strcmp(p1, "cd") == 0) {
		sscanf(str, "%s%s", p1, p2);
		cd(Cur_Dir_Addr, p2);
	}
 else if (strcmp(p1, "ls") == 0) {
	 ls(Cur_Dir_Addr);	//��ʾ��ǰĿ¼
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
		printf("��Ǹ��û�и�����\n");
	}
	return;
}
