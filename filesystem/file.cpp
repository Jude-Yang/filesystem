#include "file.h"
using namespace std;

//ȫ�ֱ�������
const int Superblock_StartAddr = 0;																//������ ƫ�Ƶ�ַ,ռһ�����̿�
const int InodeBitmap_StartAddr = 1 * BLOCK_SIZE;													//inode ���ձ� ƫ�Ƶ�ַ��ռ�������̿飬����� 1024 ��inode��״̬
const int BlockBitmap_StartAddr = InodeBitmap_StartAddr + 2 * BLOCK_SIZE;							//������ձ� ƫ�Ƶ�ַ��ռ��ʮ�����̿飬����� 10240 �����̿飨5120KB����״̬
const int Inode_StartAddr = BlockBitmap_StartAddr + 20 * BLOCK_SIZE;								//inode�ڵ��� ƫ�Ƶ�ַ��ռ INODE_NUM/(BLOCK_SIZE/INODE_SIZE) �����̿�
const int Block_StartAddr = Inode_StartAddr + INODE_NUM / (BLOCK_SIZE / INODE_SIZE) * BLOCK_SIZE;	//block������ ƫ�Ƶ�ַ ��ռ INODE_NUM �����̿�

const int Sum_Size = Block_StartAddr + BLOCK_NUM * BLOCK_SIZE;									//��������ļ���С

																								//�����ļ�����С
const int File_Max_Size = 10 * BLOCK_SIZE +														//10��ֱ�ӿ�
BLOCK_SIZE / sizeof(int) * BLOCK_SIZE +								//һ����ӿ�
(BLOCK_SIZE / sizeof(int))*(BLOCK_SIZE / sizeof(int)) * BLOCK_SIZE;		//������ӿ�

int Root_Dir_Addr;							//��Ŀ¼inode��ַ
int Cur_Dir_Addr;							//��ǰĿ¼
char Cur_Dir_Name[310];						//��ǰĿ¼��
char Cur_Host_Name[110];					//��ǰ������
char Cur_User_Name[110];					//��ǰ��½�û���
char Cur_Group_Name[110];					//��ǰ��½�û�����
char Cur_User_Dir_Name[310];				//��ǰ��½�û�Ŀ¼��

bool isLogin;								//�Ƿ����û���½

FILE* fw;									//��������ļ� д�ļ�ָ��
FILE* fr;									//��������ļ� ���ļ�ָ��
SuperBlock *superblock = new SuperBlock;	//������ָ��
bool inode_bitmap[INODE_NUM];				//inodeλͼ
bool block_bitmap[BLOCK_NUM];				//���̿�λͼ

char buffer[10000000] = { 0 };				//10M������������������ļ�


int main()
{
	//����������ļ� 
	if ((fr = fopen(FILESYSNAME, "rb")) == NULL) {	//ֻ������������ļ����������ļ���
													//��������ļ������ڣ�����һ��
		fw = fopen(FILESYSNAME, "wb");	//ֻд����������ļ����������ļ���
		if (fw == NULL) {
			printf("��������ļ���ʧ��\n");
			return 0;	//���ļ�ʧ��
		}
		fr = fopen(FILESYSNAME, "rb");	//���ڿ��Դ���

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

		printf("�ļ�ϵͳ���ڸ�ʽ������\n");
		if (!Format()) {
			printf("�ļ�ϵͳ��ʽ��ʧ��\n");
			return 0;
		}
		printf("��ʽ�����\n");
		printf("����������е�һ�ε�½\n");
		system("pause");
		system("cls");


		if (!Install()) {
			printf("��װ�ļ�ϵͳʧ��\n");
			return 0;
		}
	}
	else {	//��������ļ��Ѵ���
		fread(buffer, Sum_Size, 1, fr);

		//ȡ���ļ������ݴ浽�����У���д��ʽ���ļ�֮����д���ļ���д��ʽ�򿪻�����ļ���
		fw = fopen(FILESYSNAME, "wb");	//ֻд����������ļ����������ļ���
		if (fw == NULL) {
			printf("��������ļ���ʧ��\n");
			return false;	//���ļ�ʧ��
		}
		fwrite(buffer, Sum_Size, 1, fw);

		/* ��ʾ�Ƿ�Ҫ��ʽ��
		* ��Ϊ���ǵ�һ�ε�½������ȥ��һ��
		* ������Ҫ�ֶ����ñ���
		Ready();
		system("pause");
		system("cls");
		*/

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

		if (!Install()) {
			printf("��װ�ļ�ϵͳʧ��\n");
			return 0;
		}

	}


	//testPrint();

	//��¼
	while (1) {
		if (isLogin) {	//��½�ɹ������ܽ���shell
			char str[100];
			char *p;
			if ((p = strstr(Cur_Dir_Name, Cur_User_Dir_Name)) == NULL)	//û�ҵ�
				printf("[%s@%s %s]# ", Cur_Host_Name, Cur_User_Name, Cur_Dir_Name);
			else
				printf("[%s@%s ~%s]# ", Cur_Host_Name, Cur_User_Name, Cur_Dir_Name + strlen(Cur_User_Dir_Name));
			//gets(str);
			//str = getchar();
			//cin.getline(cin, str, '\n');
			cin.getline(str, 100);
			cmd(str);
		}

		else {
			printf("��ӭ�����ȵ�¼\n");
			while (!login());	//��½
			printf("��½�ɹ���\n");
			//system("pause");
			system("cls");
		}
	}

	fclose(fw);		//�ͷ��ļ�ָ��
	fclose(fr);		//�ͷ��ļ�ָ��

	return 0;
}
