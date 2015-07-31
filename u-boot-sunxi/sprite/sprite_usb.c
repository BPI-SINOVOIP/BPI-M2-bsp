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
#include <config.h>
#include <common.h>

download_info  dl_map;
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sprite_card_firmware_probe(void)
{
	uint  start;

	start = sprite_card_firmware_start();
	if(!start)
	{
		printf("sunxi sprite: read mbr error\n");

		return -1;
	}

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sprite_card_fetch_download_map(download_info *dl_info)
{
	//read dl info

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int card_start_fetch_part_data(HIMAGEITEM imghd, HIMAGEITEM imgitemhd, queue_data qdata)
{
	//��ȡ�ɹ�
	if(Img_ReadItemData(imghd, imgitemhd, (void *)qdata.data, qdata.length))
	{
		return 0;
	}
	return -1;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int sunxi_sprite_card_download_part(void)
{
	uint part_datasize;
    int  i = 0;
    int  format;
    uint base_start, base_length;
    queue_data  qdata;
    uint origin_verify, active_verify;

    for(i=0;i<dl_map.download_count;i++)
    {
    	//����һ����������Ҫ��д��������
    	//�������buffer
    	sunxi_queue_reset();
    	//USB������Ҫ��������Ϊdl_filename������
    	base_part_start = dl_map.part_info[i].addrlo;
        part_flash_size  = dl_map.part_info[i].lenlo;
    	//��������ֱ�Ӷ�ȡ
    	//���߶��������ж��Զ�ִ�еķ�ʽ����ͣ�����buffer�ռ�
    	//�������ɣ�û����buffer���ã���������ʱ�жϲ�ѯ�Ƿ����¿ռ�
		imgitemhd = Img_OpenItem(imghd, dl_map.part_info[i].dl_style, dl_map.part_info[i].dl_filename);
		if(!imgitemhd)
		{
			printf("sprite error: open part %s failed\n", dl_map.part_info[i].dl_filename);

			return -1;
		}
    	part_datasize = Img_GetItemSize(imghd, imgitemhd);
        if(part_datasize > part_flash_size)      //��������С�Ƿ�Ϸ�
        {
        	//data is larger than part size
        	printf("sunxi sprite: data size is larger than part %s size\n", dl_map.part_info[i].dl_filename);

        	return -1;
        }
        //��ʼ��ȡ�������ݣ���һ�����������ж����ݵĸ�ʽ
		sunxi_queue_pick(&qdata);
        if(!card_start_fetch_part_data(imghd, imgitemhd, qdata))
        {
        	printf("card sprite error: read sdcard fail\n");

        	return -1;
        }
        //�жϷ������ݵĸ�ʽ
        format = sunxi_sprite_probe_part_data_format(qdata.data);
        if(format == SUNXI_SPRITE_FORMAT_RAW)
        {
        	ret = sunxi_sprite_download_raw(qdata, base_part_start, part_flash_size);
        }
        //else if(format == SUNXI_SPRITE_FORMAT_SPARSE)
        else
        {
        	ret = sunxi_sprite_download_sparse(qdata, base_part_start, part_flash_size);
        }
        if(ret < 0)
        {
        	printf("sunxi sprite: download part data error\n");

        	return -1;
        }
		Img_CloseItem(imghd, imgitemhd);
		//У������
        if(dl_map.one_part_info[i].vf_filename)
        {
        	imgitemhd = Img_OpenItem(imghd, dl_map.part_info[i].dl_style, dl_map.part_info[i].vf_filename);
			if(!imgitemhd)
			{
				printf("sprite error: open part %s failed\n", dl_map.part_info[i].vf_filename);
				Img_CloseItem(imghd, imgitemhd);
				continue;
			}
        	if(!Img_ReadItemData(imghd, imgitemhd, (void *)&origin_verify, sizeof(int)))   //��������
	        {
	            printf("sprite update warning: fail to read data from %s\n", dl_info->one_part_info[i].vf_filename);
				Img_CloseItem(imghd, imgitemhd);
	            continue;
	        }
        	if(format == SUNXI_SPRITE_FORMAT_RAW)
        	{
                active_verify = sunxi_sprite_part_rawdata_verify(base_start, base_length);
            }
            else
            {
            	active_verify = sunxi_sprite_part_sparsedata_verify();
            }
            if(origin_verify != active_verify)
            {
            	printf("sunxi sprite: part %s verify error\n", dl_map.one_part_info[i].dl_filename);
            	printf("origin checksum=%x, active checksum=%x\n", origin_verify, active_verify);
            	Img_CloseItem(imghd, imgitemhd);

            	return -1;
            }
            Img_CloseItem(imghd, imgitemhd);
        }
        else
        {
        	printf("sunxi sprite: part %s not need to verify\n", dl_map.one_part_info[i].dl_filename);
        }
    }

    return 0;
}

