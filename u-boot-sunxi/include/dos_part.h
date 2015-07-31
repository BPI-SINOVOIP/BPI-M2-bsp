/*
 * (C) Copyright 2012
 *     wangflord@allwinnertech.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 */

#ifndef __DOS_PART_H_
#define __DOS_PART_H_

//��׼������Ϣ
#pragma pack(push, 1)
typedef struct tag_part_info_stand
{
	char 				indicator;			//��ʾ�÷����Ƿ��ǻ����
	char				start_head;			//������ʼ�Ĵ�ͷ
	short				start_sector:6;		//������ʼ������
	short				start_cylinder:10;	//������ʼ������
	char				part_type;			//��������			00H:û��ָ��  01H:DOS12	02H:xenix	04H:DOS16 05H:��չ����	06H:FAT16
											//					07H:NTFS	  0BH:FAT32
	char				end_head;			//���������Ĵ�ͷ
	short				end_sector:6;		//��������������
	short				end_cylinder:10;	//��������������
	//int					start_sectors;		//��ʼ������ǰ��˵��������Ӳ�̵ĸ��������������߼�����
	//int					total_sectors;		//�����е���������
	short				start_sectorl;
	short				start_sectorh;
	short				total_sectorsl;
	short				total_sectorsh;
}
part_info_stand;
#pragma pack(pop)

//��׼MBR
#pragma pack(push, 1)
typedef struct tag_mbr_stand
{
	char				mbr[0x89];			//��������¼
	char				err_info[0x135];	//������Ϣ
	part_info_stand		part_info[4];		//��������
	short				end_flag;			//�̶�ֵ 0x55aa
}
mbr_stand;
#pragma pack(pop)


#endif	/* __BOOT_STANDBY_H_ */
