/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef  __SUNXI_MBR_H__
#define  __SUNXI_MBR_H__

#define     DOWNLOAD_MAP_NAME   "dlinfo.fex"
#define     SUNXI_MBR_NAME      "sunxi_mbr.fex"
/* MBR       */
#define     SUNXI_MBR_SIZE			    (16 * 1024)
#define     SUNXI_DL_SIZE				(16 * 1024)
#define   	SUNXI_MBR_MAGIC			    "softw411"
#define     SUNXI_MBR_MAX_PART_COUNT	120
#define     SUNXI_MBR_COPY_NUM          4    //mbr�ı�������
#define     SUNXI_MBR_RESERVED          (SUNXI_MBR_SIZE - 32 - 4 - (SUNXI_MBR_MAX_PART_COUNT * sizeof(sunxi_partition)))   //mbr�����Ŀռ�
#define     SUNXI_DL_RESERVED           (SUNXI_DL_SIZE - 32 - (SUNXI_MBR_MAX_PART_COUNT * sizeof(dl_one_part_info)))

#define     SUNXI_NOLOCK                (0)
#define     SUNXI_LOCKING               (0xAA)
#define     SUNXI_RELOCKING             (0xA0)
#define     SUNXI_UNLOCK                (0xA5)
/* partition information */
typedef struct sunxi_partition_t
{
	unsigned  int       addrhi;				//��ʼ��ַ, ������Ϊ��λ
	unsigned  int       addrlo;				//
	unsigned  int       lenhi;				//����
	unsigned  int       lenlo;				//
	unsigned  char      classname[16];		//���豸��
	unsigned  char      name[16];			//���豸��
	unsigned  int       user_type;          //�û�����
	unsigned  int       keydata;            //�ؼ����ݣ�Ҫ����������ʧ
	unsigned  int       ro;                 //��д����
	unsigned  int       sig_verify;			//ǩ����֤����
	unsigned  int       sig_erase;          //ǩ����������
	unsigned  int       sig_value[4];
	unsigned  int       sig_pubkey;
	unsigned  int       sig_pbumode;
	unsigned  char      reserved2[36];		//�������ݣ�ƥ�������Ϣ128�ֽ�
}__attribute__ ((packed))sunxi_partition;

/* mbr information */
typedef struct sunxi_mbr
{
	unsigned  int       crc32;				        // crc 1k - 4
	unsigned  int       version;			        // �汾��Ϣ�� 0x00000100
	unsigned  char 	    magic[8];			        //"softw311"
	unsigned  int 	    copy;				        //����
	unsigned  int 	    index;				        //�ڼ���MBR����
	unsigned  int       PartCount;			        //��������
	unsigned  int       stamp[1];					//����
	sunxi_partition     array[SUNXI_MBR_MAX_PART_COUNT];	//
	unsigned  int       lockflag;
	unsigned  char      res[SUNXI_MBR_RESERVED];
}__attribute__ ((packed)) sunxi_mbr_t;

typedef struct tag_one_part_info
{
	unsigned  char      name[16];           //����д���������豸��
	unsigned  int       addrhi;             //����д�����ĸߵ�ַ��������λ
	unsigned  int       addrlo;             //����д�����ĵ͵�ַ��������λ
	unsigned  int       lenhi;				//����д�����ĳ��ȣ���32λ��������λ
	unsigned  int       lenlo;				//����д�����ĳ��ȣ���32λ��������λ
	unsigned  char      dl_filename[16];    //����д�������ļ����ƣ����ȹ̶�16�ֽ�
	unsigned  char      vf_filename[16];    //����д������У���ļ����ƣ����ȹ̶�16�ֽ�
	unsigned  int       encrypt;            //����д�����������Ƿ���м��� 0:����   1��������
	unsigned  int       verify;             //����д�����������Ƿ����У�� 0:��У�� 1��У��
}__attribute__ ((packed)) dl_one_part_info;

//������д��Ϣ
typedef struct tag_download_info
{
	unsigned  int       crc32;				        		        //crc
	unsigned  int       version;                                    //�汾��  0x00000101
	unsigned  char 	    magic[8];			        		        //"softw311"
	unsigned  int       download_count;             		        //��Ҫ��д�ķ�������
	unsigned  int       stamp[3];									//����
	dl_one_part_info	one_part_info[SUNXI_MBR_MAX_PART_COUNT];	//��д��������Ϣ
	unsigned  char      res[SUNXI_DL_RESERVED];
}
__attribute__ ((packed)) sunxi_download_info;


#endif
