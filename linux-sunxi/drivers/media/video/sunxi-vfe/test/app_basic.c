
//#加了点注释

//#Rockie Cheng

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>            

#include <fcntl.h>             
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>         
#include <linux/videodev2.h>

#include <time.h>
#include <linux/fb.h>
#include <linux/sunxi_physmem.h>

//#include "../../../../../../linux-v2.6.36.4/include/linux/drv_display.h"//modify this
//#include "./../../../../../include/linux/drv_display_sun4i.h"//modify this

#include "./../../../../../include/linux/drv_display.h"//modify this

#define WR_FILE
//#define READ_NUM 5000
#define DISPLAY
#define LCD_WIDTH		    1280
#define LCD_HEIGHT	    800

#define CLEAR(x) memset (&(x), 0, sizeof (x))

int count;
static int fd_mem = -1;

struct buffer {
        void *                  start;
        size_t                  length;
};

struct size{
	int width;
	int height;
};

static char *           dev_name        = "/dev/video1";//摄像头设备名
static char *           mem_name        = "/dev/sunxi_mem";
static int              fd              = -1;
struct buffer *         buffers         = NULL;
static unsigned int     n_buffers       = 0;

FILE *file_fd;
static unsigned long file_length;
static unsigned char *file_name;

int disphd;
unsigned int hlay;
int sel = 0;//which screen 0/1
__disp_layer_info_t layer_para;
__disp_video_fb_t video_fb;
__u32 arg[4];

//struct timeval time_test;   
//struct timezone tz; 

struct size input_size;
struct size disp_size;
int  vfe_format;

__disp_pixel_fmt_t  disp_format;
__disp_pixel_mod_t  disp_mode;
__disp_pixel_seq_t	disp_seq;
int	 read_num = 50;

int  req_frame_num;
int	 fps=7;
int	 fps_test=0;
int	 invalid_ops=0;
int  invalid_fmt_test=0;
int	 control_test=0;
int  ioctl_test=0;
int	 lost_frame_test=0;
struct test_case{
		int 							  input_width;
		int									input_height;
		int 							  disp_width;
		int									disp_height;
		int 								csi_format;
		__disp_pixel_fmt_t 	disp_format;
		__disp_pixel_mod_t	disp_mode;
		__disp_pixel_seq_t	disp_seq;
}; 

#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))


//////////////////////////////////////////////////////
//获取一帧数据
//////////////////////////////////////////////////////
static int read_frame (void)
{
	struct v4l2_buffer buf;
	unsigned int i;
	unsigned int pic_size;
	
	pic_size = input_size.width*input_size.height*3>>1;
	CLEAR (buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	ioctl (fd, VIDIOC_DQBUF, &buf); //出列采集的帧缓冲
	
	assert (buf.index < n_buffers);
//	printf ("buf.index dq is %d,\n",buf.index);
//	printf ("buf.m.offset = 0x%x\n",buf.m.offset);		
	//disp_set_addr(320,240,&buf.m.offset);
//	printf("disp addr = %x\n",buf.m.offset);
	disp_set_addr(disp_size.width, disp_size.height,&buf.m.offset);
 
	//printf ("press ENTER to continue!\n");
	//getchar();
#ifdef WR_FILE
	if(count == 50) {
	  printf("file length = %d\n",buffers[buf.index].length);
	  printf("file start = %d\n",buffers[buf.index].start);
	  file_fd = fopen("/sdcard/fb.bin","w");
	  
	  fwrite(buffers[buf.index].start/*+ALIGN_4K(pic_size)*/, 2592*1936*2, 1, file_fd); //将其写入文件中
	  if (-1 == ioctl (fd_mem, SUNXI_MEM_FLUSH_CACHE_ALL, NULL))
		printf("mem_flush error!\n");
	  
	  fclose(file_fd);
	  //file_fd = fopen("/mnt/fb.bin","a");
	}
	/*else if (count <=49)
	{
	    fwrite(buffers[buf.index].start+ALIGN_4K(pic_size), 1312*2*484*1.5, 1, file_fd); //将其写入文件中
	    if (-1 == ioctl (fd_mem, SUNXI_MEM_FLUSH_CACHE_ALL, NULL))
		printf("mem_flush error!\n");
	}*/

	if (count == 0 )
		fclose(file_fd);
#endif	
	ioctl (fd, VIDIOC_QBUF, &buf); //再将其入列

	return 1;
}

int disp_int(int w,int h)
{
	/*display start*/ 
    //unsigned int h,w;
    __u32 id = 0;
	
    //h= 480;
    //w= 640;

	if((disphd = open("/dev/disp",O_RDWR)) == -1)
	{
		printf("open file /dev/disp fail. \n");
		return 0;
	}

    arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_ON, (void*)arg);

    //layer0
    arg[0] = 0;
    arg[1] = DISP_LAYER_WORK_MODE_SCALER;
    hlay = ioctl(disphd, DISP_CMD_LAYER_REQUEST, (void*)arg);
    if(hlay == 0)
    {
        printf("request layer0 fail\n");
        return 0;
    }
	printf("video layer hdl:%d\n", hlay);

    layer_para.mode = DISP_LAYER_WORK_MODE_SCALER; 
    layer_para.pipe = 0; 
    layer_para.fb.addr[0]       = 0;//your Y address,modify this 
    layer_para.fb.addr[1]       = 0; //your C address,modify this 
    layer_para.fb.addr[2]       = 0; 
    layer_para.fb.size.width    = w;
    layer_para.fb.size.height   = h;
    layer_para.fb.mode          = disp_mode;///DISP_MOD_INTERLEAVED;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_UV_COMBINED;
    layer_para.fb.format        = disp_format;//DISP_FORMAT_YUV420;//DISP_FORMAT_YUV422;//DISP_FORMAT_YUV420;
    layer_para.fb.br_swap       = 0;
    layer_para.fb.seq           = disp_seq;//DISP_SEQ_UVUV;//DISP_SEQ_YUYV;//DISP_SEQ_YVYU;//DISP_SEQ_UYVY;//DISP_SEQ_VYUY//DISP_SEQ_UVUV
    layer_para.ck_enable        = 0;
    layer_para.alpha_en         = 1; 
    layer_para.alpha_val        = 0xff;
    layer_para.src_win.x        = 0;
    layer_para.src_win.y        = 0;
    layer_para.src_win.width    = w;
    layer_para.src_win.height   = h;
    layer_para.scn_win.x        = 0;
    layer_para.scn_win.y        = 0;
    layer_para.scn_win.width    = LCD_WIDTH;//800;
    layer_para.scn_win.height   = LCD_HEIGHT;//480;
	arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#if 0
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_TOP,(void*)arg);
#endif
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_OPEN,(void*)arg);

#if 1
	int fb_fd;
	unsigned long fb_layer;
	void *addr = NULL;
	fb_fd = open("/dev/fb0", O_RDWR);
	if (ioctl(fb_fd, FBIOGET_LAYER_HDL_0, &fb_layer) == -1) {
		printf("get fb layer handel\n");	
	}
	
	addr = malloc(LCD_WIDTH*LCD_HEIGHT*3);
	memset(addr, 0xff, LCD_WIDTH*LCD_HEIGHT*3);
	write(fb_fd, addr, LCD_WIDTH*LCD_HEIGHT*3);
	//memset(addr, 0x12, 800*480*3);

	printf("fb_layer hdl: %ld\n", fb_layer);
	close(fb_fd);

	arg[0] = 0;
	arg[1] = fb_layer;
	ioctl(disphd, DISP_CMD_LAYER_BOTTOM, (void *)arg);
#endif
}

void disp_start(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_START,  (void*)arg);
}

void disp_stop(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_STOP,  (void*)arg);
}

int disp_on()
{
		arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_ON, (void*)arg);
}

int disp_set_addr(int w,int h,int *addr)
{
#if 0	
	layer_para.fb.addr[0]       = *addr;//your Y address,modify this 
    layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this 
    layer_para.fb.addr[2]       = *addr+w*h*3/2; 
    
    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#endif
	__disp_video_fb_t  fb_addr;	
	memset(&fb_addr, 0, sizeof(__disp_video_fb_t));

	fb_addr.interlace       = 0;
	fb_addr.top_field_first = 0;
	fb_addr.frame_rate      = 25;
	fb_addr.addr[0] = *addr;
//	fb_addr.addr[1] = *addr + w * h;
//	fb_addr.addr[2] = *addr + w*h*3/2;
	
	
	switch(vfe_format){
		case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:		
    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
    	fb_addr.addr[2]       = *addr+w*h*3/2; 
    	break;
    case V4L2_PIX_FMT_YUV420:
    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
    	fb_addr.addr[2]       = *addr+w*h*5/4;
    	break;
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV12:	
    case V4L2_PIX_FMT_HM12:	
    	fb_addr.addr[1]       = *addr+w*h; //your C address,modify this 
    	fb_addr.addr[2]       = layer_para.fb.addr[1];
    	break;
    
    default:
    	printf("vfe_format is not found!\n");
    	break;
    
  	}
  	
  	fb_addr.id = 0;  //TODO
  	
    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&fb_addr;
    ioctl(disphd, DISP_CMD_VIDEO_SET_FB, (void*)arg);
}

int disp_quit()
{
	__u32 arg[4];
	arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_OFF, (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_CLOSE,  (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_RELEASE,  (void*)arg);
    close (disphd);
}

int setExpMode(int exp_mode)
{
	int ret = -1;
	struct v4l2_control ctrl;

	memset((void*)&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	ctrl.value = exp_mode;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
	{
		printf("setExpMode failed! \n");
	}
	else 
	{
		printf("setExpMode ok!\n");
	}

	return ret;
}

int setExpWinMode(int exp_win_num)
{
	int ret = -1;
	struct v4l2_control ctrl;
	struct v4l2_win_coordinate  coor_xy;
	coor_xy.x1 = 2596/2 - 180;
	coor_xy.y1 = 1936/2 - 180;
	coor_xy.x2 = 2596/2 + 180;
	coor_xy.y2 = 1936/2 + 180;

	memset((void*)&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_AUTO_EXPOSURE_WIN_NUM;
	ctrl.value = exp_win_num;
	ctrl.user_pt = &coor_xy;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
	{
		printf("setExpWin failed! \n");
	}
	else 
	{
		printf("setExpWin ok!\n");
	}

	return ret;
}

int setSaturation(int saturation)
{
	int ret = -1;
	struct v4l2_control ctrl;

	memset((void*)&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_SATURATION;
	ctrl.value = saturation;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
	{
		printf("setSaturation failed! \n");
	}
	else 
	{
		printf("setSaturation ok!\n");
	}

	return ret;
}

int setContrast(int contrast)
{
	int ret = -1;
	struct v4l2_control ctrl;

	memset((void*)&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_CONTRAST;
	ctrl.value = contrast;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
	{
		printf("setContrast failed! \n");
	}
	else 
	{
		printf("setContrast ok!\n");
	}

	return ret;
}

int setBrightness(int brightness)
{
	int ret = -1;
	struct v4l2_control ctrl;

	memset((void*)&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_BRIGHTNESS;
	ctrl.value = brightness;
	ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
	{
		printf("setBrightness failed! \n");
	}
	else 
	{
		printf("setBrightness ok!\n");
	}

	return ret;
}





int main_test (unsigned int exp_val, unsigned int gain_val, int sa, int con, int bri,int vcm_length)
{
	struct v4l2_capability cap; 
	struct v4l2_format fmt;
	struct v4l2_pix_format sub_fmt;
	unsigned int i;
	enum v4l2_buf_type type;
	struct v4l2_cropcap cropcap;
	struct v4l2_input inp;
	struct v4l2_streamparm parms;

	fd_mem = open(mem_name, O_RDWR /* required */ | O_NONBLOCK, 0);
	
	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);//打开设备
	
	inp.index = 0;

	if (inp.type == V4L2_INPUT_TYPE_CAMERA)
		printf("enuminput type is V4L2_INPUT_TYPE_CAMERA!\n");
	
	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &inp))	//设置输入index
		printf("VIDIOC_S_INPUT error!\n");
	

		//Test VIDIOC_S_FMT
			CLEAR (fmt);
			fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			fmt.fmt.pix.width       = input_size.width; //320; 
			fmt.fmt.pix.height      = input_size.height; //240;
			fmt.fmt.pix.pixelformat = vfe_format;//V4L2_PIX_FMT_YUV422P;//V4L2_PIX_FMT_NV12;//V4L2_PIX_FMT_YUYV;
			fmt.fmt.pix.field       = V4L2_FIELD_NONE;
			int ret = -1;
			
			fmt.fmt.pix.subchannel = &sub_fmt;
			fmt.fmt.pix.subchannel->width = input_size.width;
			fmt.fmt.pix.subchannel->height = input_size.height;
			fmt.fmt.pix.subchannel->pixelformat = V4L2_PIX_FMT_YUV420;
			fmt.fmt.pix.subchannel->field = V4L2_FIELD_NONE;
			
			if (-1 == ioctl (fd, VIDIOC_S_FMT, &fmt)) //设置图像格式
			{
					printf("VIDIOC_S_FMT error!\n");
					ret = -1;
					return ret;
					//goto close;
			}
			
			disp_size.width = fmt.fmt.pix.width;
			disp_size.height = fmt.fmt.pix.height;
				

	//Test VIDIOC_S_PARM
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = fps;
	parms.parm.capture.capturemode = V4L2_MODE_VIDEO; //V4L2_MODE_IMAGE
	
	if (-1 == ioctl (fd, VIDIOC_S_PARM, &parms)) //设置帧率
			printf ("VIDIOC_S_PARM error\n");
				
	
	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count               = req_frame_num;	//申请buffer的数量，若是video则最小为3
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;
	
	ioctl (fd, VIDIOC_REQBUFS, &req); //申请缓冲，count是申请的数量

	buffers = calloc (req.count, sizeof (*buffers));//内存中建立对应空间

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) 
	{
	   struct v4l2_buffer buf;   //驱动中的一帧
	   CLEAR (buf);
	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = n_buffers;

	   if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf)) //映射用户空间
			printf ("VIDIOC_QUERYBUF error\n");

	   buffers[n_buffers].length = buf.length;
	   buffers[n_buffers].start  = mmap (NULL /* start anywhere */,    //通过mmap建立映射关系
								         buf.length,
								         PROT_READ | PROT_WRITE /* required */,
								         MAP_SHARED /* recommended */,
								         fd, buf.m.offset);
			
	   if (MAP_FAILED == buffers[n_buffers].start)
			printf ("mmap failed\n");
	}

	for (i = 0; i < n_buffers; ++i) 
	{
	   struct v4l2_buffer buf;
	   CLEAR (buf);

	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = i;

	   if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))//申请到的缓冲进入列队
		 printf ("VIDIOC_QBUF failed\n");
	}



 if (exp_val)
	{
		struct v4l2_queryctrl qc_gain,qc_exposure,qc_focus;
		struct v4l2_control ctrl_gain,ctrl_exposure,ctrl_focus;
		struct v4l2_control ctrl;
		

		CLEAR(qc_gain);
		CLEAR(qc_exposure);
		CLEAR(qc_focus);
		qc_gain.id = V4L2_CID_GAIN;
		qc_exposure.id = V4L2_CID_EXPOSURE;
		qc_focus.id = V4L2_CID_FOCUS_ABSOLUTE;

		if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &qc_gain))
		{
			printf("VIDIOC_QUERYCTRL %s failed!\n",qc_gain.name);
			//continue;
		}
		ctrl_gain.id = qc_gain.id;
		ctrl_gain.value = qc_gain.minimum + 16*gain_val*qc_gain.step;//set the sensor gain
		if (-1 == ioctl (fd, VIDIOC_S_CTRL, &ctrl_gain))
			printf("VIDIOC_S_CTRL %s failed!\n",qc_gain.name);
		else
		{
			printf("%s set to %x\n",qc_gain.name,ctrl_gain.value>>4);

		}

		if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &qc_exposure))
		{
			printf("VIDIOC_QUERYCTRL %s failed!\n",qc_exposure.name);
			//continue;
		}

		ctrl_exposure.id = qc_exposure.id;
		ctrl_exposure.value = qc_exposure.minimum + 16*exp_val*qc_exposure.step;//set the sensor

		if (-1 == ioctl (fd, VIDIOC_S_CTRL, &ctrl_exposure))
			printf("VIDIOC_S_CTRL %s failed!\n",qc_exposure.name);
		else
		{
			printf("%s set to %x\n",qc_exposure.name,ctrl_exposure.value>>4);

		}


		if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &qc_focus))
		{
			printf("VIDIOC_QUERYCTRL %s failed!\n",qc_focus.name);
			//continue;
		}

		ctrl_focus.id = qc_focus.id;
		ctrl_focus.value =  vcm_length;
		if (-1 == ioctl (fd, VIDIOC_S_CTRL, &ctrl_focus))
			printf("VIDIOC_S_CTRL failed!\n");
		else
		{
			printf("VIDIOC_S_CTRL succeed \n");

		}	

	}
					
#ifdef DISPLAY				
				disp_int(disp_size.width,disp_size.height);
				disp_start();
#endif	
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (-1 == ioctl (fd, VIDIOC_STREAMON, &type)) {//开始捕捉图像数据
		printf ("VIDIOC_STREAMON failed\n");
		goto close;
	}
	else 
		printf ("VIDIOC_STREAMON ok\n");
	
  count = read_num;
        
	while(count-->0)
//	while(1)
	{
		//gettimeofday(&time_test,&tz);
		if (count == read_num - 100)
		{
	//	  setExpMode(0);
	//	  setExpWinMode(1);
	     setSaturation(sa);
	     setContrast(con);
		 setBrightness(bri);
		}
	        		
		for (;;) //这一段涉及到异步IO
		{
		   fd_set fds;
		   struct timeval tv;
		   int r;
		
		   FD_ZERO (&fds);//将指定的文件描述符集清空
		   FD_SET (fd, &fds);//在文件描述符集合中增加一个新的文件描述符
		
		   /* Timeout. */
		   tv.tv_sec = 2;
		   tv.tv_usec = 0;
		
		   r = select (fd + 1, &fds, NULL, NULL, &tv);//判断是否可读（即摄像头是否准备好），tv是定时
		
		   if (-1 == r) {
			if (EINTR == errno)
			 continue;
			printf ("select err\n");
								}
		   if (0 == r) {
			fprintf (stderr, "select timeout\n");
			exit (EXIT_FAILURE);
								}
								
#ifdef DISPLAY      
      if(count==read_num-1)
      	disp_on();                  
#endif		
		   if (read_frame ())//如果可读，执行read_frame ()函数，并跳出循环
		   break;
		} 
	}


close:	
	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type)) //停止捕捉图像数据
		printf ("VIDIOC_STREAMOFF failed\n");
	else
		printf ("VIDIOC_STREAMOFF ok\n");
		
unmap:
	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length)) {
			printf ("munmap error");
		}
	}
	
	disp_stop();
	disp_quit();
	close (fd);
	close(fd_mem);
	return 0;
}

int 
main(int argCount, char* argValue[])
{
	unsigned int exp_val, gain_val,vcm_length;
	int sa, con, bri;
     if(argCount < 2){
	 	read_num = 100;
		sa = 0;
		con = 0;
		bri = 0;
     }
	 else
	 {
	 	read_num = atoi(argValue[1]);
		if (read_num==0)
			read_num = 100;
		sa = 0;
		con = 0;
		bri = 0;
		if (argCount > 2)
		   exp_val= atoi(argValue[2]);
		if (argCount > 3)
		   gain_val = atoi(argValue[3]);
		if (argCount > 4)
		   vcm_length = atoi(argValue[4]);

	 }
	
		req_frame_num = 5;
		input_size.width  = 2592;//1600;//640;
		input_size.height = 1936;//1200;//480;
		printf("width = %d height = %d\n",input_size.width,input_size.height);
//		disp_size.width = 1280;//1600;//640;
//		disp_size.height = 1024;//1200;//480;
		vfe_format = V4L2_PIX_FMT_SBGGR10;//V4L2_PIX_FMT_NV12;
		
		disp_format= DISP_FORMAT_YUV420;
		disp_mode  = DISP_MOD_NON_MB_UV_COMBINED;
		disp_seq   = DISP_SEQ_UVUV;

printf("********************************************************************Read stream test start,capture 1000 frames,press to continue\n");
		getchar();
		
//		read_num = 1000;

		main_test(exp_val, gain_val, sa, con, bri,vcm_length);

printf("********************************************************************test done,press to end\n");	
		getchar();
			
	exit (EXIT_SUCCESS);
	return 0;
}
